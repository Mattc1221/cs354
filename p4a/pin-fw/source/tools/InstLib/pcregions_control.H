/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2017 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */

#ifndef PCREGIONS_CONTROL_H
#define PCREGIONS_CONTROL_H

/*! @defgroup CONTROLLER_PCREGIONS
  @ingroup CONTROLLER
   Controller for "regions" that are specified using pc+count.
   Use -pcregions:in pcregions.csv

   Regions are specified using a text file with the records of the form:
   # comment,thread-id,region-id,start-pc,start-pc-count,end-pc,end-pc-count,
     end-pc-relative-count, region-length, region-weight, region-multiplier,
     region-type
   [ fields after the first twelve are ignored, so are any lines beginning 
        with '#' ]
   
   end-pc-relative-count: is the count w.r.t. the beginning of the region.
    [ 
     Corner case: if start-pc==end-pc, the first occurrence of the
        (common) pc is *not* counted in end-pc-relative-count. This
        to account for the fact that when used for relogging, the 
        initial occurrence of the (common) pc will be skipped due to
        delay in region logging {unless '-log:precise_controller' is used}
    ]
    
    region-type : "simulation"|"warmup"

   Region weight can be computed two ways:
   1. region-weight : number_of_slices_in_cluster/total_number_of_slices
          # Here, all slices are considered equal

   2. alternate-region-weight : number_of_instructions_in_cluster
                                    /total_instruction_count 
          # Here, slices with higher instruction counts contribute more.

    If we use fixed-sized slices, both the weights are the same. However, 
    with variable length intervals, they will be different.

    The "BarrierPoint" paper used the second weight for their prediction. 
    Also, since they were directly computing run-time, the introduced the 
    idea of a region 'multiplier' 
       region-multipler = alternate-region-weight * total_number_of_slices

    That way:
        predicted_runtime = SUM( region_i_runtime * region_i_multiplier)

   Knobs:
   ------
    -pcregions:in foo.csv : input file
    -pcregions:end_only  report only region ends
    -pcregions:relative  use relative endPC-count 
    -pcregions:merge_warmup  ignore warmupEnd and regionStart
    -pcregions:no_warmup  ignore warmup regions 
    -pcregions:startpc_offset  Add 'offset' to Start PCs of all regions
    -pcregions:verbose : for getting informed about regions/events 
    -pcregions:out : Output file for regions skipped due to overlap 
        The idea is to feed this file with "-pcregions:in" to the next 
        invocation of the tool to process skipped regions.
        * If this knob is specified but no regions are skipped, the output
          file will be empty.

    Region processing:
    -----------------
    * The overall regions picture looks as follows:
        WARMUP--(SIM)REGION

        each sub-region has a start and end event. So there are four 
        events possible (two coinciding . warmup-end and sim-start)
            EVENT_WARMUP_START  : Beginning of warmup region
            EVENT_WARMUP_STOP  : End of warmup region
            EVENT_START        : Beginning of interval
            EVENT_STOP         : End of interval

    * Warmup region needs to specified explicitly in the pcregions file.
      For example, if we are using SimPoint output to generate the pcregion
      file, then the warmup could be N slices prior to the simulation
      region slice. 

    * 'region number' vs 'region id'
        o both > 0
        o region number starts at one and goes till the number 
             of regions in the input pcregions file.
        o region ids  can be arbitrary numbers > 0
            A pcregion file may have regions with ids sparsely assigned 
             * SimPoint generated cluster-IDs could be non-contiguous
                and region_id = cluster_id+1
             * This could also happen for example when the pcregion file
                 contains regions skipped during a previous relogging
                 session
    * We use 'region number' value of zero to indicate an invalid ROI, so
       all *valid* 'region number' are > 1 and that's the reason we
       size ROIinfo array at 1+#of_regions so we can index it starting at 1.
*/

#include <algorithm>
#include <sstream> 
#include <string.h>
#include <cctype>
#include "region_utils.H"

using namespace std;
namespace CONTROLLER{

typedef enum { 
    WARMUP_REGION,
    SIMULATION_REGION,
    INVALID_REGION,
}REGION_TYPE;

typedef set<ADDRINT>::iterator ADDRINT_SET_ITERATOR;

/*! @ingroup CONTROLLER_PCREGIONS
    This class is used for reading in the pcregions.csv file
*/
class PCREGION
{
    private:   
        friend class CONTROL_PCREGIONS; // allow it to set private fields
        ADDRINT _pcStart; // read in
        ADDRINT _pcEnd; // read in
        UINT64 _countStart; // read in
        UINT64 _countEnd; // read in
        UINT64 _countEndRelative; // read in
        UINT64 _length; // read in
        double _weight; // read in
        double _multiplier; // read in
        string _comment; // read in
        UINT32 _rid; // read in
        UINT32 _tid; // read in
        size_t _rno;  // assigned
        REGION_TYPE _rtype; // assigned
        UINT32 _parentSimulationRid; // N; assigned based on _rtype="warmupN"
        PCREGION * _parentSimulationPCRegion; // assigned 
        UINT32 _weightTimesHundredThousand; // computed + assigned
         // Convert input weight ('double' 0--1)  to  UINT32 to avoid
         // floating point code in the pintools.
    public:
        PCREGION()
        {
            _pcStart = 0; 
            _countStart = 0; 
            _pcEnd = 0; 
            _countEnd = 0; 
            _countEndRelative = 0; 
            _length = 0; 
            _weight = 0; 
            _multiplier = 0; 
            _rid = 0; 
            _tid = 0 ; 
            _rno = 0; 
            _parentSimulationRid = 0; 
            _parentSimulationPCRegion = NULL; 
            _rtype = INVALID_REGION; 
            _weightTimesHundredThousand = 0;
        }
        string GetComment() const { return _comment;}
        UINT32 GetRegionId() const { return _rid;}
        BOOL IsWarmupRegion() const { return _rtype==WARMUP_REGION;}
        UINT64 GetRegionStartCount() const { return _countStart; }
        UINT64 GetRegionEndCount() const { return _countEnd; }
        UINT64 GetRegionEndCountRelative() const { return _countEndRelative; }
        UINT64 GetRegionLength() const { return _length; }
        UINT64 GetRegionStartPC() const { return _pcStart; }
        UINT64 GetRegionEndPC() const { return _pcEnd; }
        double GetRegionMultiplier() const { return _multiplier;}
        PCREGION * GetParentSimulationRegion() 
            const { return _parentSimulationPCRegion;}
        UINT32 GetWeightTimesHundredThousand() const {
            return _weightTimesHundredThousand;}
};

typedef vector<PCREGION> PCREGION_VECTOR;

/*! @ingroup CONTROLLER_PCREGIONS
    This class is used for keeping track of each unique PC
     in the incoming pcregions.csv file.
    Initialized at instrumentation time, used/updated during analysis.
*/
struct ROIpcinfoStruct{
    ADDRINT pc; 
    UINT64 * execcount; // counts for various threads
    UINT32 * nextStartingRno; // starting region numbers for various threads
    UINT32 * nextEndingRno; // starting region numbers for various threads
};

/*! @ingroup CONTROLLER_PCREGIONS
    This class is used for keeping track of ROIs in the incoming pcregions.csv 
       file.
*/
struct ROIinfoStruct{
    BOOL startSeen;
    BOOL triggered;
    UINT32 friendRid;
    PCREGION *pcregion;
};

/*! @ingroup CONTROLLER_PCREGIONS
*/

class CONTROL_PCREGIONS
{
    private:
    static const UINT32 BUFSIZE=2000;  
    set<ADDRINT> _ROIpcset;
    map<ADDRINT, UINT32> _pcinfoIndex;
    struct ROIpcinfoStruct *_ROIpcinfo;
    struct ROIinfoStruct **_ROIinfo;
    UINT32 *_ROIcount; // number of regions, assumed contiguous starting from 1
    UINT32 _ROIpccount; // number of unique pcs that start/stop ROIs


    public:
    CONTROL_PCREGIONS(CONTROL_ARGS & control_args, 
        CONTROL_MANAGER* cm)
        : _control_args(control_args),
          _pcFileKnob(KNOB_MODE_WRITEONCE,
                     control_args.get_knob_family(),
                     "pcregions:in",
                     "",
                     "PC-Regions file",
                     control_args.get_prefix()),
          _pcEndOnlyKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:end_only",
                        "0",
                        "Print information only about end of region",
                        control_args.get_prefix()),
          _pcRelativeKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:relative",
                        "0",
                        "User relative count for endPC",
                        control_args.get_prefix()),
          _pcMergeWarmupKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:merge_warmup",
                        "0",
                        "Ignore warmup-end and the following region-start",
                        control_args.get_prefix()),
          _pcNoWarmupKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:no_warmup",
                        "0",
                        "Ignore warmup regions",
                        control_args.get_prefix()),
          _pcStartPCOffsetKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:startpc_offset",
                        "0",
                        "Add 'offset' to all PCs",
                        control_args.get_prefix()),
          _pcRidKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:rid",
                        "0",
                        "Only trigger region 'rid'.",
                        control_args.get_prefix()),
          _pcVerboseKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:verbose",
                        "0",
                        "Print information about regions/events ",
                        control_args.get_prefix()),
          _pcOutFileKnob(KNOB_MODE_WRITEONCE,
                        control_args.get_knob_family(),
                        "pcregions:out",
                        "",
                        "Output file containing regions skipped due to overlap",
                        control_args.get_prefix())
    {
        _cm = cm;
        _valid = true;
        _maxThreads = PIN_MAX_THREADS;
    }

    /*! @ingroup CONTROLLER_PCREGIONS
      Activate the controller if the -pcregions knob is provided
      @return 1 if controller can start an interval, otherwise 0
    */
    INT32 Activate(BOOL passContext)
    {
        if (strcmp(_pcFileKnob.Value().c_str(),"") == 0)
        {
            return 0;
        }
        _passContext = passContext;
        _active = true;
        PIN_AddThreadFiniFunction(ThreadFini, this);

        Allocate();

        ReadPCRegionsFile();

        ProcessPCRegions();

        if(_pcVerboseKnob) PrintPCRegions();

        TRACE_AddInstrumentFunction(Trace, this);

        return 1;
    }
    bool IsActive() const { return _active; };

    PCREGION * LastTriggeredRegion(THREADID tid) const { 
        return _last_triggered_pcregion[tid];}

    private:
    CONTROL_ARGS _control_args;
    bool _valid;
    CONTROLLER::CONTROL_MANAGER* _cm;

    VOID Allocate()
    {
        _pcregions = new PCREGION_VECTOR[_maxThreads];
        _last_triggered_pcregion = new PCREGION * [_maxThreads];
        memset(_last_triggered_pcregion , 0, 
               sizeof(_last_triggered_pcregion[0]) * _maxThreads);
        _last_fired_event = new EVENT_TYPE [_maxThreads];
        memset(_last_fired_event , EVENT_INVALID, 
               sizeof(_last_fired_event[0]) * _maxThreads);
        _ROIcount = new UINT32 [_maxThreads];
        memset(_ROIcount , 0, (sizeof(UINT32))*_maxThreads);
    }

    VOID ReadPCRegionsFile()
    {
        string filename = _pcFileKnob.Value().c_str();

        ifstream rfile(filename.c_str());

        if (!rfile.is_open())
        {
            cerr << "Could not open PCregions file " << 
                _pcFileKnob.Value().c_str() << endl;
            PIN_ExitApplication(-1);
        }

        UINT32 lineNum = 0;
        UINT32 recordNum = 0;
        PCREGION * pcregion = 0;
        while(true)
        {
            if( rfile.eof() )
            {
                break;
            }

            CHAR record[BUFSIZE+1];
            CHAR urecord[BUFSIZE+1];
            string field;

            double t_weight;
            string t_comment;
            double t_multiplier;
            INT32 t_rid;
            INT32 t_tid;
            UINT64 t_countStart;
            UINT64 t_countEnd;
            UINT64 t_countEndRelative;
            UINT64 t_length;
            ADDRINT t_pcStart;
            ADDRINT t_pcEnd;
            string t_typestr;

            rfile.getline(record, BUFSIZE);
            lineNum++;

            if(strlen(record)==0) continue;

            // Create a temporary record with lower case letters
            for(UINT32 i=0; i <= strlen(record); i++)
                urecord[i] = tolower(record[i]); 

            // first word "comment" : this is the header
            if(strncmp(urecord,"comment",7)==0) continue;

            // first letter '#' : this is a comment 
            if(urecord[0]=='#') continue;

            istringstream s(record);
            recordNum++;


            // cerr << "Record # " << recordNum << endl;
            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty comment field.");
            t_comment = field;
            // cerr << "Comment " << t_comment << endl;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty thread-id field.");
            t_tid = REGION_UTILS::StringToUINT32(field, "thread-id");
            // cerr << "thread-id " << t_tid << endl;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty region-id field.");
            t_rid = REGION_UTILS::StringToUINT32(field, "region-id");
            //cerr << "region-id " << t_rid << endl;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty start-pc field.");
            t_pcStart  = REGION_UTILS::StringToADDRINT(field, 
                                            "simulation-region-start-pc");
            if(_pcStartPCOffsetKnob)  
            {
                cerr << hex << "start-pc 0x" << t_pcStart  
                    << " adding offset 0x"
                    << hex << _pcStartPCOffsetKnob; 
                t_pcStart  += _pcStartPCOffsetKnob;
                cerr << " new start-pc 0x" << hex << t_pcStart  << endl;
            }
            _ROIpcset.insert(t_pcStart);

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty start-count field.");
            t_countStart  = REGION_UTILS::StringToUINT64(field, 
                                        "simulation-region-start-pc-count");
            //cerr << "start-count " << t_countStart << endl;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty end-pc field.");
            t_pcEnd  = REGION_UTILS::StringToADDRINT(field, 
                                            "simulation-region-end-pc");
            //cerr << "end-pc " << t_pcEnd << endl;
            _ROIpcset.insert(t_pcEnd);

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty end-count field.");
            t_countEnd  = REGION_UTILS::StringToUINT64(field, 
                                          "simulation-region-end-pc-count");
            //cerr << "end-count " << t_countEnd << endl;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty end-count-relative field.");
            t_countEndRelative  = REGION_UTILS::StringToUINT64(field, 
                               "simulation-region-end-pc-count-relative");
            //cerr << "end-count " << t_countEndRelative << endl;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty length field.");
            t_length  = REGION_UTILS::StringToUINT64(field, 
                               "simulation-region-length");
            //cerr << "length " << t_length;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty region-weight field.");
            t_weight  = REGION_UTILS::StringToDouble(field, "region-weight");
            ASSERT((t_weight >= 0), 
                    "region-weight (" + field + ") must be positive" );
            ASSERT((t_weight <= 1), 
                    "region-weight (" + field + ") must be between 0 and 1" );
            //cerr << "region-weight" << t_weight << endl;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty multiplier field.");
            t_multiplier  = REGION_UTILS::StringToDouble(field, 
                               "region-multiplier");
            //cerr << "multiplier " << t_multiplier;

            field.clear();
            getline(s, field, ',');
            ASSERT(!field.empty(), "Empty region-type field.");
            t_typestr  = field;
            //cerr << "region-type" << t_typestr << endl;


            string tail;

            s >> tail;

            if(!tail.empty())
                cerr << "WARNING: pcregions file '" << filename << 
                    "' line number " << dec << lineNum << 
                    ": ignoring fields : " << tail  << endl;

            _pcregions[t_tid].push_back(PCREGION());
            pcregion = & _pcregions[t_tid].back();
            pcregion->_comment = t_comment;
            pcregion->_rno = _pcregions[t_tid].size(); // starts at 1
            pcregion->_rid = t_rid;
            pcregion->_tid = t_tid;
            pcregion->_weight = t_weight;
            pcregion->_multiplier = t_multiplier;
            pcregion->_weightTimesHundredThousand = (UINT32)(t_weight*100000);
            pcregion->_countStart = t_countStart;
            pcregion->_countEnd = t_countEnd;
            pcregion->_countEndRelative = t_countEndRelative;
            pcregion->_length = t_length;
            pcregion->_pcStart = t_pcStart;
            pcregion->_pcEnd = t_pcEnd;
            if(t_typestr.compare(0,6,"warmup")==0)
            {
                pcregion->_rtype = WARMUP_REGION;
                istringstream iss(t_typestr);
                string tok;
                UINT32 parentSimulationRid = 0;
                if ( !getline(iss, tok, ':'))
                {
                    cerr << "Parent Rid id not embedded for warmup region " 
                        << t_typestr 
                        <<  " region type must be 'warmup:N' " << endl; 
                   PIN_ExitApplication(1);
                }
                getline(iss, tok); // Read "N"
                parentSimulationRid = atoi(tok.c_str()); 
                if ( parentSimulationRid == 0 )
                {
                    cerr << "Parent Rid could not be parsed for " 
                        << t_typestr <<  " token " << tok << endl; 
                    PIN_ExitApplication(1);
                }
                pcregion->_parentSimulationRid = parentSimulationRid;
            }
            else if(t_typestr == "simulation")
                pcregion->_rtype = SIMULATION_REGION;
            else 
                cerr << "WARNING: invalid region type" 
                     << t_typestr  
                     << "line number " << dec << lineNum << endl;
        }
        rfile.close();
    }

    VOID PrintPCRegions()
    {
        cerr << "ROIpcset: " << endl;
        for (ADDRINT_SET_ITERATOR it=_ROIpcset.begin();
                                 it!=_ROIpcset.end(); ++it)
            cerr << " 0x" << hex << *it;
        cerr << endl;

        for(UINT32 tid=0; tid < _maxThreads; tid++)
        {
            for ( UINT32 i = 0; i < _pcregions[tid].size(); i++ )
            {
                PCREGION * pcregion = & _pcregions[tid][i];
                cerr << "rno: " << pcregion->_rno
                << " rid " << pcregion->_rid
                << " comment '" << pcregion->_comment << "'"
                << " tid " << pcregion->_tid
                << " weight " << pcregion->_weight
                << " weightTimesHundredThousand " 
                << pcregion->_weightTimesHundredThousand
                << " multiplier " << pcregion->_multiplier
                << " pcStart 0x" << hex << pcregion->_pcStart
                << " countStart " << dec << pcregion->_countStart
                << " pcEnd 0x" << hex << pcregion->_pcEnd
                << " countEnd " << dec << pcregion->_countEnd
                << " countEndRelative " << dec << pcregion->_countEndRelative
                << " length " << dec << pcregion->_length
                << " region-type " << ((pcregion->_rtype == WARMUP_REGION) ?
                                    " 'warmup' ": " 'simulation' ")
                << " parent Rid " << dec << pcregion->_parentSimulationRid
                << endl;
            }

            if (_ROIcount[tid])
            {
                cerr << "ROIinfo:" << endl;
                for ( UINT32 i = 1; i <= _ROIcount[tid]; i++ )
                {
                    struct ROIinfoStruct * regioninfo = & _ROIinfo[tid][i];
                    cerr << " rno " << i
                    << " rid " << regioninfo->pcregion->_rid
                    << " startPC 0x" << hex <<  regioninfo->pcregion->_pcStart
                    << " startCount " << dec 
                        << regioninfo->pcregion->_countStart
                    << " endPC 0x" << hex <<  regioninfo->pcregion->_pcEnd
                    << " endCount " << dec << regioninfo->pcregion->_countEnd
                    << " endCountRelative " << dec 
                        << regioninfo->pcregion->_countEndRelative
                    << " length " << dec << regioninfo->pcregion->_length
                    << " weight " << regioninfo->pcregion->_weight
                    << " multiplier " << regioninfo->pcregion->_multiplier
                    << " startSeen " << regioninfo->startSeen
                    << " triggered " << regioninfo->triggered
                    << " friendRid " << regioninfo->friendRid
                    << endl;
                }
    
                cerr << "ROIpcinfo:" << endl;
                for ( UINT32 i = 1; i <= _ROIpccount; i++ )
                {
                    struct ROIpcinfoStruct * pcinfo = & _ROIpcinfo[i];
                    cerr << " PC 0x" << hex <<  pcinfo->pc
                    << " execcount " << dec <<  pcinfo->execcount[tid]
                    << " nextStartingRno " << pcinfo->nextStartingRno[tid]
                    << " nextEndingRno " << pcinfo->nextEndingRno[tid]
                    << endl;
                }
            }
        }
    }

    UINT32 RidToRno(UINT32 tid, UINT32 rid)
    {
        for ( UINT32 i = 0; i < _pcregions[tid].size(); i++ )
        {
            PCREGION * pcregion = & _pcregions[tid][i];
            if( pcregion->_rid == rid) return pcregion->_rno;
        }
        ASSERT(FALSE, "tid"+decstr(tid)+": Rid "+decstr(rid)
                +" does not exist!\n");
        return UINT32(-1);
    }

    VOID ProcessPCRegions()
    {
        if(_pcRelativeKnob)
            ASSERT((_pcRidKnob), 
                "'relative' knob can used be only with 'rid'");
        if(_pcRelativeKnob)
            ASSERT((_pcEndOnlyKnob), 
                "'relative' knob can used be only with 'end_only'");
        UINT32 pccount = _ROIpcset.size();//  number of unique PCs 
        _ROIpcinfo = new ROIpcinfoStruct [pccount+1]; // indexing starts at 1
        _ROIinfo = new ROIinfoStruct * [_maxThreads]; 
        UINT32 index=1;
        
        // Populate _ROIpcinfo[]
        for (ADDRINT_SET_ITERATOR it=_ROIpcset.begin(); it!=_ROIpcset.end(); 
                ++it)
        {
            _pcinfoIndex[*it] = index;
            _ROIpcinfo[index].pc = 0;
            _ROIpcinfo[index].execcount = new UINT64 [_maxThreads]; 
            memset(_ROIpcinfo[index].execcount, 0,
                 (sizeof(UINT64))*_maxThreads);
            _ROIpcinfo[index].nextStartingRno= new UINT32 [_maxThreads]; 
            memset(_ROIpcinfo[index].nextStartingRno, 0, 
                (sizeof(UINT32))*_maxThreads);
            _ROIpcinfo[index].nextEndingRno= new  UINT32 [_maxThreads]; 
            memset(_ROIpcinfo[index].nextEndingRno, 0, 
                (sizeof(UINT32))*_maxThreads);
            index++;
        }
        ASSERTX((index-1) == _ROIpcset.size());
        _ROIpccount = _ROIpcset.size();


        // Populate _ROIinfo[]
        for(UINT32 tid=0; tid < _maxThreads; tid++)
        {
            UINT32 tidNumRegions = _pcregions[tid].size(); 
            _ROIinfo[tid] = new ROIinfoStruct [tidNumRegions+1]; 
                    // indexing starts at 1
            _ROIcount[tid] = 0;

            for ( UINT32 i = 0; i < _pcregions[tid].size(); i++ )
            {
                PCREGION * pcregion = & _pcregions[tid][i];

                REGION_TYPE rtype = pcregion->_rtype;
                UINT32 rno = pcregion->_rno;
                UINT32 rid = pcregion->_rid;
                if(_pcNoWarmupKnob && (rtype == WARMUP_REGION)) 
                {
                    cerr << "Ignoring warmup: region id" <<  rno 
                        << " rid " << rid << endl;
                    continue;
                }
                ADDRINT pcStart = pcregion->_pcStart;
                UINT64 countStart = pcregion->_countStart;
                ADDRINT pcEnd = pcregion->_pcEnd;
                UINT64 countEnd = pcregion->_countEnd;
                UINT64 countEndRelative = pcregion->_countEndRelative;
                if(_pcRelativeKnob) countEnd = countEndRelative;
                UINT32 startPcIndex = _pcinfoIndex[pcStart];
                UINT32 endPcIndex = _pcinfoIndex[pcEnd];

                _ROIcount[tid]++;

                _ROIinfo[tid][rno].friendRid = 0;
                _ROIinfo[tid][rno].startSeen = FALSE;
                _ROIinfo[tid][rno].triggered = FALSE;
                _ROIinfo[tid][rno].pcregion = pcregion;

                //Link warmup and simulation ROIinfo via the 'friendRid' field
                if(rtype == WARMUP_REGION)
                {
                    UINT32 parentSimulationRid = 
                        pcregion->_parentSimulationRid;
                    UINT32 parentrno = RidToRno(tid, parentSimulationRid);
                    // This warmup region and its parent now become friends
                    _ROIinfo[tid][rno].friendRid = parentSimulationRid;
                    _ROIinfo[tid][parentrno].friendRid = rid;
                }

                if(_ROIpcinfo[startPcIndex].pc == 0)
                    _ROIpcinfo[startPcIndex].pc = pcStart;
                else
                    ASSERTX(_ROIpcinfo[startPcIndex].pc == pcStart);

                if(_ROIpcinfo[endPcIndex].pc == 0)
                    _ROIpcinfo[endPcIndex].pc = pcEnd;
                else
                    ASSERTX(_ROIpcinfo[endPcIndex].pc == pcEnd);

                if(_ROIpcinfo[startPcIndex].nextStartingRno[tid] == 0)
                    _ROIpcinfo[startPcIndex].nextStartingRno[tid] = rno;
                else
                {
                    UINT32 otherRno =  
                        _ROIpcinfo[startPcIndex].nextStartingRno[tid];
                    ASSERTX(_ROIinfo[tid][otherRno].pcregion->_pcStart
                                 == pcStart);
                    UINT64 otherRnocountStart = 
                        _ROIinfo[tid][otherRno].pcregion->_countStart;
                    if(otherRnocountStart > countStart)
                    {
                        // This Rno start fires earlier
                        _ROIpcinfo[startPcIndex].nextStartingRno[tid] = rno;
                    }
                    else if((otherRnocountStart == countStart) 
                                && (rtype == SIMULATION_REGION))
                    {
                        // The other ROI must be a warmup region
                        ASSERTX(_ROIinfo[tid][otherRno].pcregion->_rtype
                             == WARMUP_REGION);
                        // Prioritize simulation regions
                        _ROIpcinfo[startPcIndex].nextStartingRno[tid] = rno;
                    }
                }

                if(_ROIpcinfo[endPcIndex].nextEndingRno[tid] == 0)
                    _ROIpcinfo[endPcIndex].nextEndingRno[tid] = rno;
                else
                {
                    UINT32 otherRno =
                          _ROIpcinfo[endPcIndex].nextEndingRno[tid];
                    ASSERTX(_ROIinfo[tid][otherRno].pcregion->_pcEnd == pcEnd);
                    UINT64 otherRnocountEnd = 
                        _ROIinfo[tid][otherRno].pcregion->_countEnd;
                    if(otherRnocountEnd > countEnd)
                    {
                        // This Rno end fires earlier
                        _ROIpcinfo[endPcIndex].nextEndingRno[tid] = rno;
                    }
                    else if((otherRnocountEnd == countEnd) 
                                && (rtype == SIMULATION_REGION))
                    {
                        // The other ROI must be a warmup region
                        ASSERTX(_ROIinfo[tid][otherRno].pcregion->_rtype
                             == WARMUP_REGION);
                        // Prioritize simulation regions
                        _ROIpcinfo[endPcIndex].nextEndingRno[tid] = rno;
                    }
                }
            }

            if(_ROIcount[tid] && _pcRidKnob)
            {
                // This is the only rid to be triggered
                UINT32 rno = RidToRno(tid, _pcRidKnob);
                ADDRINT pcStart = _ROIinfo[tid][rno].pcregion->_pcStart;
                ADDRINT pcEnd =  _ROIinfo[tid][rno].pcregion->_pcEnd;
                UINT32 startPcIndex = _pcinfoIndex[pcStart];
                UINT32 endPcIndex = _pcinfoIndex[pcEnd];
                _ROIpcinfo[startPcIndex].nextStartingRno[tid] = rno;
                _ROIpcinfo[endPcIndex].nextEndingRno[tid] = rno;
            }

            // Now set parentSimulationPCRegion for warmup regions
            if (_ROIcount[tid])
            {
                for ( UINT32 i = 1; i <= _ROIcount[tid]; i++ )
                {
                    if(_ROIinfo[tid][i].pcregion->_rtype == WARMUP_REGION)
                    {
                        UINT32 simRid = 
                            _ROIinfo[tid][i].pcregion->_parentSimulationRid;
                        UINT32 simrno = RidToRno(tid, simRid);
                        PCREGION *warmupPCRegion = 
                            _ROIinfo[tid][i].pcregion;
                        // Make sure the simulation and warmup regions are 
                        // friends of each other.
                        UINT32 warmupRid = warmupPCRegion->_rid;
                        _ROIinfo[tid][simrno].friendRid = warmupRid;
                        PCREGION *simPCRegion = 
                            _ROIinfo[tid][simrno].pcregion;
                        warmupPCRegion->_parentSimulationPCRegion = 
                                simPCRegion;
                    }
                }
            }
        }
    }

    // The major purpose of pcregions controller is to support region
    // logging.
    // Some regions may get skipped due to warmup+simulation overlap as
    // logging of multiple regions at the same time is not supported.
    // This function creates a pcregions.csv file with the skipped region
    // so it can be used for the next iteration of region logging.
    static  VOID OutputSkippedRegion (CONTROL_PCREGIONS * cr, 
            THREADID tid, UINT32 rid) 
    {
        if (strcmp(cr->_pcOutFileKnob.Value().c_str(),"") == 0) return;

        if (!cr->xfile.is_open())
        {
            cr->xfile.open(cr->_pcOutFileKnob.Value().c_str());
            if (!cr->xfile.is_open())
            {
                cerr << "Could not open output  file " << 
                     cr->_pcOutFileKnob.Value().c_str() << endl;
            }
            cr->xfile << "#Generated automatically for skipped pc-regions\n"
                << "# comment,thread-id,region-id,start-pc,"
                "start-pc-count,end-pc,end-pc-count"
                ",end-pc-relative-count,region-length,"
                    "region-weight, region-multiplier, region-type"
                << endl << endl;
        }

        UINT32 rno = cr->RidToRno(tid, rid);
        struct ROIinfoStruct * regioninfo = & cr->_ROIinfo[tid][rno];

        cr->xfile << "# Region = " << rid << endl;
        cr->xfile << "#Start: pc : 0x" << hex 
                <<  regioninfo->pcregion->_pcStart 
                << " count: " << dec << regioninfo->pcregion->_countStart 
                << endl;
        cr->xfile << "#End: pc : 0x" << hex <<  regioninfo->pcregion->_pcEnd 
                << " count: " << dec << regioninfo->pcregion->_countEnd 
                << " relative_count: " << dec 
                << regioninfo->pcregion->_countEndRelative << endl;
        cr->xfile << "Skipped region"
        << "," << dec << tid
        << "," << dec << rid
        << ",0x" << hex << regioninfo->pcregion->_pcStart
        << "," << dec << regioninfo->pcregion->_countStart
        << ",0x" << hex << regioninfo->pcregion->_pcEnd
        << "," << dec << regioninfo->pcregion->_countEnd
        << "," << dec << regioninfo->pcregion->_countEndRelative
        << "," << dec << regioninfo->pcregion->_length
        << "," << regioninfo->pcregion->_weight
        << "," << regioninfo->pcregion->_multiplier;
        if(regioninfo->pcregion->_rtype == WARMUP_REGION)
        {
            cr->xfile << ",warmup:" << regioninfo->friendRid;
        }
        else
        {
            cr->xfile << ",simulation";
        }
        cr->xfile << endl << endl;
    }

    static  VOID ThreadFini(THREADID tid, const CONTEXT *ctxt,
                 INT32 flags, VOID *v)
    {
        CONTROL_PCREGIONS * cr = static_cast<CONTROL_PCREGIONS *>(v);
        if (cr->_ROIcount[tid])
        {
            if(cr->_pcVerboseKnob) 
            {
                UINT32 ROIpccount = cr->_ROIpcset.size();
                cerr << "ThreadFini(): ROIpcinfo for tid " << tid << endl;
                for ( UINT32 i = 1; i <= ROIpccount; i++ )
                {
                    struct ROIpcinfoStruct * pcinfo = & cr->_ROIpcinfo[i];
                    cerr << " PC 0x" << hex <<  pcinfo->pc
                    << " execcount " << pcinfo->execcount[tid]
                    << " nextStartingRno " << pcinfo->nextStartingRno[tid]
                    << " nextEndingRno " << pcinfo->nextEndingRno[tid]
                    << endl;
                }
                cerr << "ThreadFini(): ROI Summary for tid:" << dec << tid;
            }
            if(cr->_pcRelativeKnob) return; // skip error checking 
            for ( UINT32 rno = 1; rno <= cr->_ROIcount[tid]; rno++ )
            {
                if(cr->_pcRidKnob) 
                {
                     UINT32 ridKnobRno = cr->RidToRno(tid, cr->_pcRidKnob);
                     if( ridKnobRno != rno ) continue;
                }
                struct ROIinfoStruct * regioninfo = & cr->_ROIinfo[tid][rno];

                BOOL printflag = cr->_pcVerboseKnob 
                    || (regioninfo->startSeen || !regioninfo->triggered);

                UINT32 rid = regioninfo->pcregion->GetRegionId();

                // we print if either the verbose flag is on or if
                // there is an error
                if(printflag)
                {
                    if(!cr->_pcVerboseKnob) cerr << "ROI error for tid:" 
                        << dec << tid;
                    cerr << " rno " << rno
                    << ((regioninfo->pcregion->_rtype == WARMUP_REGION) ? 
                            " (warmup) ": " (simulation) ")
                    << " rid " << dec << rid 
                    << " startPC 0x" << hex <<  regioninfo->pcregion->_pcStart
                    << " startCount " << dec 
                        << regioninfo->pcregion->_countStart
                    << " endPC 0x" << hex <<  regioninfo->pcregion->_pcEnd
                    << " endCount " << dec << regioninfo->pcregion->_countEnd
                    << " endCountRelative " << dec 
                        << regioninfo->pcregion->_countEndRelative
                    << " length " << dec << regioninfo->pcregion->_length;
                    if(regioninfo->pcregion->_rtype == SIMULATION_REGION) 
                    {
                        cerr << " weight " << regioninfo->pcregion->_weight;
                        cerr << " multiplier " 
                            << regioninfo->pcregion->_multiplier;
                    }
                    if(regioninfo->startSeen)
                        cerr << " NEVER EXITED"; 
                    if(!regioninfo->triggered && !cr->_pcEndOnlyKnob ) 
                    {
                        cerr << " SKIPPED";
                        cerr << " friendRid " << dec << regioninfo->friendRid;
                        OutputSkippedRegion(cr, tid, rid); 
                        if(regioninfo->friendRid)
                            OutputSkippedRegion(cr, tid,
                                 regioninfo->friendRid); 
                    }
                    cerr << endl;
                }
            }
        }
    }

    static VOID Trace(TRACE trace, VOID * cregion)
    {
        CONTROL_PCREGIONS * cr = static_cast<CONTROL_PCREGIONS *>(cregion);
        for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); 
            bbl = BBL_Next(bbl))
        {
            for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); 
                 ins = INS_Next(ins))
            {
                ADDRINT pc = INS_Address(ins);
                if (cr->_ROIpcset.count(pc))
                {
                    CONTROL_ARGS * ca = &cr->_control_args;
                    UINT32 pcIndex = cr->_pcinfoIndex[pc];
                    INS_InsertIfCall(ins, IPOINT_BEFORE,
                             AFUNPTR(AdvanceIf),
                             IARG_CALL_ORDER, ca->get_instrument_order(),
                             IARG_FAST_ANALYSIS_CALL,
                             IARG_ADDRINT, &(cr->_ROIpcinfo[pcIndex]), 
                             IARG_ADDRINT, cr->_ROIinfo, 
                             IARG_THREAD_ID,
                             IARG_END);
            
                    if (cr->_passContext)
                    {
                        INS_InsertThenCall(ins, IPOINT_BEFORE,
                                   AFUNPTR(AdvanceThen),
                                   IARG_CALL_ORDER, ca->get_instrument_order(),
                                   IARG_FAST_ANALYSIS_CALL,
                                   IARG_ADDRINT, &(cr->_ROIpcinfo[pcIndex]), 
                                   IARG_ADDRINT, cr, 
                                   IARG_THREAD_ID,
                                   IARG_CONTEXT, 
                                   IARG_END);
                    }
                    else 
                    {
                        INS_InsertThenCall(ins, IPOINT_BEFORE,
                                   AFUNPTR(AdvanceThen), 
                                   IARG_CALL_ORDER, ca->get_instrument_order(),
                                   IARG_FAST_ANALYSIS_CALL,
                                   IARG_ADDRINT, &(cr->_ROIpcinfo[pcIndex]), 
                                   IARG_ADDRINT, cr, 
                                   IARG_THREAD_ID,
                                   // next pass a null instead as the context
                                   IARG_ADDRINT, static_cast<ADDRINT>(0),
                                   IARG_END);
                    }
                }
            
            }
        }
    }

    static ADDRINT PIN_FAST_ANALYSIS_CALL AdvanceIf(ROIpcinfoStruct * pi, 
            struct ROIinfoStruct **ri,
            UINT32 tid)
    {
        pi->execcount[tid]++;
        return (pi->nextEndingRno[tid] &&
          (pi->execcount[tid] == 
            ri[tid][pi->nextEndingRno[tid]].pcregion->_countEnd))
          ||
        (pi->nextStartingRno[tid] &&
          (pi->execcount[tid] == 
            ri[tid][pi->nextStartingRno[tid]].pcregion->_countStart));
    }

    static VOID PIN_FAST_ANALYSIS_CALL AdvanceThen(ROIpcinfoStruct * pi, 
            CONTROL_PCREGIONS * cr,
            UINT32 tid, CONTEXT * ctxt)
    {
        //Check for ROI end first as another ROI may start here
        
        UINT32 nextEndingRno = pi->nextEndingRno[tid];
        UINT64 nextEndingROIcount = (nextEndingRno ?
            cr->_ROIinfo[tid][nextEndingRno].pcregion->_countEnd:0);
        if(nextEndingRno && (pi->execcount[tid] == nextEndingROIcount)) 
        {
            UINT32 endingRno = pi->nextEndingRno[tid];
            UINT32 endingRid = cr->_ROIinfo[tid][endingRno].pcregion->_rid;
            if((cr->_ROIinfo[tid][endingRno].startSeen)
             || ((cr->_pcRidKnob == endingRno) && cr->_pcEndOnlyKnob))
            {
                EVENT_TYPE e = (cr->_ROIinfo[tid][endingRno].pcregion->_rtype 
                    == WARMUP_REGION) ? EVENT_WARMUP_STOP:EVENT_STOP;
                cr->_ROIinfo[tid][endingRno].startSeen = FALSE;
                if((cr->_pcRidKnob == endingRno) && cr->_pcEndOnlyKnob)
                {
                    cr->_last_triggered_pcregion[tid] = 
                        cr->_ROIinfo[tid][endingRno].pcregion;
                }
                if((cr->_pcMergeWarmupKnob) && (e == EVENT_WARMUP_STOP))
                {
                    //Skip firing this event
                }
                else
                {
                    cr->_last_fired_event[tid] = e; 
                    cr->_cm->Fire(e, ctxt, (VOID *)pi->pc, tid, TRUE);
                }
            }
            else
            {
                cerr << "Region " << dec << endingRid 
                    << " stop seen without start, ignoring."  << endl;
            }

            //update nextEndingRno
            UINT32 nextRno = 0;
            for(UINT32 rno=1; rno <= cr->_ROIcount[tid]; rno++)
            {
            // look for the roi with smallest endCount greater than execcount
                if(cr->_ROIinfo[tid][rno].pcregion->_pcEnd == pi->pc)
                {
                    if(cr->_ROIinfo[tid][rno].pcregion->_countEnd 
                        > pi->execcount[tid])
                    {
                    // endCount greater than execcount
                        if(nextRno == 0) // first candidate rno
                            nextRno = rno;
                        else if (cr->_ROIinfo[tid][rno].pcregion->_countEnd 
                            < cr->_ROIinfo[tid][nextRno].pcregion->_countEnd)
                        {
                            // this rno endCount is closer to execcount
                            nextRno = rno;
                        }
                        else if((cr->_ROIinfo[tid][rno].pcregion->_countEnd 
                            == cr->_ROIinfo[tid][nextRno].pcregion->_countEnd)
                                && (cr->_ROIinfo[tid][rno].pcregion->_rtype 
                                        == SIMULATION_REGION))
                        {
                            // The other ROI must be a warmup region
                            ASSERTX(cr->_ROIinfo[tid][nextRno].pcregion->_rtype 
                                        == WARMUP_REGION);
                            // Prioritize simulation regions
                            nextRno = rno;
                        }
                    }
                }
            }
            pi->nextEndingRno[tid] = nextRno;
        }

        UINT32 nextStartingRno = pi->nextStartingRno[tid];
        UINT64 nextStartingROIcount = (nextStartingRno ?
            cr->_ROIinfo[tid][nextStartingRno].pcregion->_countStart:0);
        if(nextStartingRno && (pi->execcount[tid] == nextStartingROIcount)) 
        {
            UINT32 startingRno = pi->nextStartingRno[tid];
            cr->_ROIinfo[tid][startingRno].startSeen = TRUE;
            cr->_ROIinfo[tid][startingRno].triggered = TRUE;
            cr->_last_triggered_pcregion[tid] = 
                cr->_ROIinfo[tid][startingRno].pcregion;
            if(!cr->_pcEndOnlyKnob)
            {
                EVENT_TYPE e = (cr->_ROIinfo[tid][startingRno].pcregion->_rtype 
                    == WARMUP_REGION) ? EVENT_WARMUP_START:EVENT_START;
                if(cr->_pcMergeWarmupKnob 
                    && cr->_ROIinfo[tid][startingRno].friendRid 
                    && (e == EVENT_START))
                {
                    // "merge_warmup" => Skip firing this event 
                    // no matter whether warmup fired or not
                }
                else
                {
                    if(!cr->_pcRidKnob || (cr->_pcRidKnob == startingRno) ) 
                    {
                        cr->_last_fired_event[tid] = e; 
                        cr->_cm->Fire(e, ctxt, (VOID *)pi->pc, tid, TRUE);
                    }
                }
            }

            //update nextStartingRno
            UINT32 nextRno = 0;
            for(UINT32 rno=1; rno <= cr->_ROIcount[tid]; rno++)
            {
            // look for the roi with smallest startCount greater than execcount
                if(cr->_ROIinfo[tid][rno].pcregion->_pcStart == pi->pc)
                {
                    if(cr->_ROIinfo[tid][rno].pcregion->_countStart 
                        > pi->execcount[tid])
                    {
                    // startCount greater than execcount
                        if(nextRno == 0) // first candidate rno
                            nextRno = rno;
                        else if (cr->_ROIinfo[tid][rno].pcregion->_countStart 
                           < cr->_ROIinfo[tid][nextRno].pcregion->_countStart)
                        {
                            // this rno startCount is closer to execcount
                            nextRno = rno;
                        }
                        else if((cr->_ROIinfo[tid][rno].pcregion->_countStart 
                            == cr->_ROIinfo[tid][nextRno].pcregion->_countStart)
                                && (cr->_ROIinfo[tid][rno].pcregion->_rtype 
                                        == SIMULATION_REGION))
                        {
                            // The other ROI must be a warmup region
                            ASSERTX(cr->_ROIinfo[tid][nextRno].pcregion->_rtype 
                                        == WARMUP_REGION);
                            // Prioritize simulation regions
                            nextRno = rno;
                        }
                    }
                }
            }
            pi->nextStartingRno[tid] = nextRno;
        }
    }

    KNOB<string> _pcFileKnob;
    KNOB<BOOL> _pcEndOnlyKnob;
    KNOB<BOOL> _pcRelativeKnob;
    KNOB<BOOL> _pcMergeWarmupKnob;
    KNOB<BOOL> _pcNoWarmupKnob;
    KNOB<UINT32> _pcStartPCOffsetKnob;
    KNOB<UINT32> _pcRidKnob;
    KNOB<BOOL> _pcVerboseKnob;
    KNOB<string> _pcOutFileKnob;
    PCREGION_VECTOR *_pcregions; // per thread vector containing region info
    bool _active;
    THREADID _maxThreads;
    PCREGION ** _last_triggered_pcregion;
    EVENT_TYPE * _last_fired_event;
    BOOL _passContext;
    ofstream xfile;  // for writing out regions excluded due to overlap
};
}
#endif

