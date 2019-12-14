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

#include "ialarm.H"
#include "alarm_manager.H"
#include "parse_control.H"
#include "control_chain.H"
#include "alarms.H"
#include <iostream> 


using namespace CONTROLLER;


VOID IALARM::InsertIfCall_Count(IALARM* alarm, INS ins, UINT32 ninst, IPOINT point){
    INS_InsertIfCall(ins, point,
        AFUNPTR(Count),
        IARG_FAST_ANALYSIS_CALL,
        IARG_CALL_ORDER, alarm->GetInstrumentOrder(),
        IARG_ADDRINT, alarm,
        IARG_THREAD_ID,
        IARG_UINT32, ninst,
        IARG_END);
}

VOID IALARM::InsertThenCall_Fire(IALARM* alarm, INS ins, IPOINT point){
    if (alarm->_need_context){
        INS_InsertThenCall(ins, point,
            AFUNPTR(Fire),
            IARG_CALL_ORDER, alarm->GetInstrumentOrder(),
            IARG_ADDRINT, alarm,
            IARG_CONTEXT, 
            IARG_INST_PTR,
            IARG_THREAD_ID,
            IARG_END);
    }
    else{
        INS_InsertThenCall(ins, point,
            AFUNPTR(Fire),
            IARG_CALL_ORDER, alarm->GetInstrumentOrder(),
            IARG_ADDRINT, alarm,
            IARG_ADDRINT, static_cast<ADDRINT>(0), // pass a null as context, 
            IARG_INST_PTR,
            IARG_THREAD_ID,
            IARG_END);
    }
}

// Insert late file instrumentation
VOID IALARM::Insert_LateInstrumentation(IALARM* alarm, INS ins){

    // Check if late handler is set
    if (!alarm->_alarm_manager->HasLateHandler())
        return;

    // Determine ipoint 
    IPOINT ipoint = IPOINT_AFTER;
    if (INS_IsInterrupt(ins) || INS_IsSyscall(ins))
    {
        // We don't want the region of interest (in tracing)
        // to include these instructions. Since they close
        // the trace we can't take their next instruction,
        // therefore don't deliver the late handler at all.
        return;
    }

    if (INS_IsBranchOrCall(ins))
    {
        ipoint = IPOINT_TAKEN_BRANCH;
    }

    // Add if-then analysis routines
    INS_InsertIfCall(ins, ipoint,
        AFUNPTR(ActivateLate),
        IARG_FAST_ANALYSIS_CALL,
        IARG_CALL_ORDER, alarm->GetLateInstrumentOrder(),
        IARG_ADDRINT, alarm,
        IARG_END);
    INS_InsertThenCall(ins, ipoint,
        AFUNPTR(LateFire),
        IARG_CALL_ORDER, alarm->GetLateInstrumentOrder(),
        IARG_ADDRINT, alarm,
        IARG_CONTEXT, 
        IARG_INST_PTR,
        IARG_THREAD_ID,
        IARG_END);
}

ADDRINT PIN_FAST_ANALYSIS_CALL IALARM::Count(IALARM* ialarm, 
                                               UINT32 tid,
                                               UINT32 ninst){
    UINT32 armed = ialarm->_armed[tid];
    UINT32 correct_tid = (ialarm->_tid == tid) | (ialarm->_tid == ALL_THREADS);

    UINT32 should_count = armed & correct_tid;

    //if we are not in the correct thread
    ialarm->_thread_count[tid]._count += ninst*(should_count);

    return ialarm->_thread_count[tid]._count >= ialarm->_target_count._count;

}

//we want to generate the context only when we really need it.
//that is way most of the code is in the If instrumentation.
//even if the If instrumentation is be not inlined.
VOID IALARM::Fire(IALARM* ialarm, CONTEXT* ctxt, VOID * ip, UINT32 tid){

    // Check if flags was not already modified by another thread
    // in interactive controller
    if (ialarm->_alarm_manager->GetAlarmTypeFromManager() == ALARM_TYPE_INTERACTIVE)
    {
        ALARM_INTERACTIVE* interactive_alarm = static_cast<ALARM_INTERACTIVE*>(ialarm);
        if (!interactive_alarm->GetListener()->CheckClearSignal())
            return;
    }

    // Check if we need to activate late handler
    if (ialarm->_alarm_manager->HasLateHandler())
        ialarm->_activate_late_handler = TRUE;

    ialarm->_alarm_manager->Fire(ctxt, ip, tid);
}

// Late fire event
ADDRINT PIN_FAST_ANALYSIS_CALL IALARM::ActivateLate(IALARM* ialarm){
    return ialarm->_activate_late_handler;
}
VOID IALARM::LateFire(IALARM* ialarm, CONTEXT* ctxt, VOID * ip, UINT32 tid) {
    ialarm->_activate_late_handler = FALSE;
    ialarm->_alarm_manager->LateFire(ctxt, ip, tid);
}

VOID IALARM::Arm(){
    PIN_GetLock(&_lock,0);
    memset(_armed,1,sizeof(_armed));
    PIN_ReleaseLock(&_lock);
}

VOID IALARM::Disarm(THREADID tid){
    _armed[tid] = 0;
    _thread_count[tid]._count = 0;
}

VOID IALARM::Disarm(){
    PIN_GetLock(&_lock,0);
    memset(_armed,0,sizeof(_armed));
    memset(_thread_count,0,sizeof(_thread_count));
    PIN_ReleaseLock(&_lock);
}

UINT32 IALARM::GetInstrumentOrder(){
    return _alarm_manager->GetInsOrder();
}

UINT32 IALARM::GetLateInstrumentOrder(){
    return _alarm_manager->GetLateInsOrder();
}


VOID IALARM::TraceAddress(TRACE trace, VOID* v)
{
    IALARM* ialarm = static_cast<IALARM*>(v);
    ADDRINT trace_addr = TRACE_Address(trace);
    UINT32 trace_size = TRACE_Size(trace);

    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // Check Target
        // Get the last instruction in the BBL
        INS ins = BBL_InsTail(bbl);

        // Handle direct branches or calls
        if ( INS_IsDirectBranchOrCall(ins) ) 
        {
            // Get the target and compare it to the address we need
            ADDRINT target = INS_DirectBranchOrCallTargetAddress(ins);
            if (target == ialarm->_address)
            {
                InsertIfCall_Count(ialarm, ins, 1, IPOINT_TAKEN_BRANCH);
                InsertThenCall_Fire(ialarm, ins, IPOINT_TAKEN_BRANCH);

                // Add late handler instrumentation if needed
                Insert_LateInstrumentation(ialarm,ins);
            }
        }

        // Handle indirect branches or calls
        else if ( INS_IsIndirectBranchOrCall(ins)) 
        {
            InsertIfCall_Target(ialarm, ins);
            InsertThenCall_Fire(ialarm, ins, IPOINT_TAKEN_BRANCH);

            // Add late handler instrumentation if needed
            Insert_LateInstrumentation(ialarm,ins);
        }

        // If the address is not inside the trace then no need to check the
        // instructions in the BBL
        if (ialarm->_address < trace_addr ||  ialarm->_address > trace_addr+trace_size)
            continue;

        // Handle all other instructions in the BBL which may be before 
        // The instruction we are looking for
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            // Only relevant for instructions will fall through path
            if (!INS_HasFallThrough(ins))
                return;

            INS next_ins = INS_Next(ins);
            if (!INS_Valid(next_ins))
            {
                // If we reached end of BBL then get the next instruction
                BBL next_bbl = BBL_Next(bbl);
                if (BBL_Valid(next_bbl))
                {
                    next_ins = BBL_InsHead(next_bbl);
                }
            }

            // Compare the address of the next instruction to check if we 
            // encountered the address of the alarm
            if (INS_Valid(next_ins) && (INS_Address(next_ins) == ialarm->_address))
            {
                InsertIfCall_Count(ialarm, ins, 1, IPOINT_AFTER);
                InsertThenCall_Fire(ialarm, ins, IPOINT_AFTER);

                // Add late handler instrumentation if needed
                Insert_LateInstrumentation(ialarm,ins);
            }
        }
    }
}

// Instrumentation of indirect branch checking
VOID IALARM::InsertIfCall_Target(IALARM* alarm, INS ins){
    INS_InsertIfCall(ins, IPOINT_TAKEN_BRANCH,
        AFUNPTR(CheckTarget),
        IARG_FAST_ANALYSIS_CALL,
        IARG_CALL_ORDER, alarm->GetInstrumentOrder(),
        IARG_PTR, alarm,
        IARG_THREAD_ID,
        IARG_BRANCH_TARGET_ADDR,
        IARG_END);
}

// Check if we have reached the target we need
ADDRINT PIN_FAST_ANALYSIS_CALL IALARM::CheckTarget(IALARM* ialarm, 
                                                          THREADID tid,
                                                          ADDRINT branch_target) {
    BOOL armed = ialarm->_armed[tid];
    BOOL correct_tid = (ialarm->_tid == tid) | (ialarm->_tid == ALL_THREADS);

    return armed & correct_tid & (ialarm->_address == branch_target);
}
