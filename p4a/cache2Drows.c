///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        cache2Drows.c
// This File:        cache2Drows.c
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
int arr2D[3000][500]; // 3000 row by 500 col array

void main(){
	for(int row = 0; row < 3000; row++){
		for(int col = 0; col < 500; col++){
			arr2D[row][col] = row + col;
		}
	}
}
