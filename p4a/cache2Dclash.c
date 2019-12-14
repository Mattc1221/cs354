///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        cache2Dclash.c
// This File:        cache2Dclash.c
// Other Files:      N/A
// Semester:         CS 354 Fall 2019
//
// Author:           Matthew Chiang
// Email:            mchiang7@wisc.edu
// CS Login:         mchiang
//
/////////////////////////// OTHER SOURCES OF HELP /////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          N/A
//
// Online sources:   N/A
//                   
///////////////////////////////////////////////////////////////////////////////
int arr2D[128][8]; // 128 row by 8 col array

void main(){
	for(int i = 0; i< 100; i++){
		for(int row = 0; row< 128; row+=64){
			for(int col = 0; col < 8; col++){
				arr2D[row][col] = i + row + col;
			}
		}
	}
}
