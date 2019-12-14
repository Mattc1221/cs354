///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
// Main File:        hill_climb.c
// This File:        hill_climb.c
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
// Online sources:   For tokenization: http://www.tutorialspoint.com/c_standard
// 		     _library/c_function_strtok.htm
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>   
#include <stdlib.h>     
#include <string.h>  

/*
 * USE THIS:
 * Structure representing the Matrix
 * numRows: number of rows
 * numCols: number of columns
 * matrix: 2D heap allocated array of integers
 */
typedef struct {    
    int numRows;
    int numCols;
    int **matrix;
} Matrix;

/* TODO:
 * Get the dimensions of the matrix that are on
 * the first line of the input file.
 * 
 * infp: file pointer for input file
 * nRows: pointer to number of rows in the matrix   
 * nCols: pointer to number of columns in the matrix
 */
void get_dimensions(FILE *infp, int *nRows, int *nCols) {         
    // Read the dimensions on the first line from infp
    char line[10];
    fscanf(infp, "%s", line);
    char *token;

    /* get the first token */
    token = strtok(line, ",");
    if(token == NULL){
        printf("ERROR: Trouble reading # of rows\n");
        exit(1);
    }
    *nRows = atoi(token);
    
    /* walk through other tokens */
    token = strtok(NULL, ",");
    if(token == NULL){
        printf("ERROR: Trouble reading # of cols\n");
        exit(1);
    }
    *nCols = atoi(token);
}
/* TODO:
 * Continually find and print the largest neighbor
 * that is also larger than the current position
 * until nothing larger is found.       
 * 
 * outfp: file pointer for output file  
 * mstruct: pointer to the Matrix structure
 */
void hill_climb(FILE *outfp, Matrix *mstruct) {  
    int row = 0; // tracker for current row
    int col = 0; // tracker for current col
    int col_size = (*mstruct).numCols; // # of cols
    int row_size = (*mstruct).numRows; // # of rows
    int peak_found = 0; // 0 if peak not found, 1 otherwise

    while(peak_found != 1){
    	// output current position's value
	int curr = *(*((*mstruct).matrix + row) + col);
	fprintf(outfp, "%d ", curr);
	// find the largest neighber > current position
	int row_largest = row; // track the row of the largest neighbor
	int col_largest = col; // track the col of the largest neighbor
	// checks all cases with row -1
	if(row >= 1) {
	    // checks if the top left number in bigger than the current number
    	    if(col >= 1){ 
	    	if( *(*((*mstruct).matrix + row-1) + col-1) > curr){
		    // updates the current to the top left
		    curr = *(*((*mstruct).matrix + row-1) + col-1);
		    row_largest = row - 1;
		    col_largest = col - 1;
		}
	    }
	    // checks if the top middle number in bigger than the current number
	    if(*(*((*mstruct).matrix + row-1) + col) > curr){
		// updates the current to the top middle
		curr = *(*((*mstruct).matrix + row-1) + col);
                row_largest = row - 1;
                col_largest = col;
	    }
	    // checks if the top right number in bigger than the current number
	    if(col < col_size-1){ 
                if( *(*((*mstruct).matrix + row-1) + col+1) > curr){
                    // updates the current to the top right
		    curr = *(*((*mstruct).matrix + row-1) + col+1);
                    row_largest = row - 1;
                    col_largest = col + 1;
                }
            }
	}
	// checks all cases with row + 1
        if(row < row_size-1) {
            // checks if the bottom left is bigger than the current
            if(col >= 1){
                if( *(*((*mstruct).matrix + row+1) + col-1) > curr){
 		    // updates the current to the bottom left
                    curr = *(*((*mstruct).matrix + row+1) + col-1);
                    row_largest = row + 1;
                    col_largest = col - 1;
                }
            }
            // checks if the bottom middle is bigger than the current
            if(*(*((*mstruct).matrix + row+1) + col) > curr){
		// updates the current to the bottom middle
                curr = *(*((*mstruct).matrix + row+1) + col);
                row_largest = row + 1;
                col_largest = col;
            }
            // checks if the bottom right is bigger than the current
            if(col < col_size-1){
                if( *(*((*mstruct).matrix + row+1) + col+1) > curr){
		    // updates the current to the bottom right
                    curr = *(*((*mstruct).matrix + row+1) + col+1);
                    row_largest = row + 1;
                    col_largest = col + 1;
                }
            }
        }

	// checks left middle is bigger than the current 
	if(col > 0){
	    if(*(*((*mstruct).matrix + row) + col-1) > curr){
		// updates the current to left middle
	        curr = *(*((*mstruct).matrix + row) + col-1);
                row_largest = row;
                col_largest = col - 1;
	    }
	}

	// checks right middle case is bigger than the current
        if(col < col_size - 1){
            if(*(*((*mstruct).matrix + row) + col+1) > curr){
		// updates the current to the middle right
                curr = *(*((*mstruct).matrix + row) + col+1);
                row_largest = row;
                col_largest = col + 1;
            }
        }

	// test ending condition
	if(row_largest == row && col_largest == col){
	    peak_found = 1;
	}else{
	    // update the row and colum
	    row = row_largest;
	    col = col_largest;
	}
    }
    fprintf(outfp,"\n");
}    

/* TODO:
 * This program reads an m by n matrix from the input file
 * and outputs to a file the path determined from the
 * hill climb algorithm.    
 *
 * argc: CLA count
 * argv: CLA values
 */
int main(int argc, char *argv[]) {
    // Check if number of command-line arguments is correct
    if(argc != 3) {
	printf("ERROR: wrong number of arguments, expected input: ./hill_out <inputfile.txt> <outputfile.txt>\n");
	exit(1);
    }

    // Open the input file and check if it opened successfully
    FILE *infp;
    infp = fopen(argv[1], "r");
    if(infp == NULL){
	printf("ERROR: input file could not be opened\n");
	exit(1);
    }

    // Declare local variables including the Matrix structure 
    Matrix mstruc;

    // Call the function get_dimensions
    get_dimensions(infp, &mstruc.numRows, &mstruc.numCols);

    // Dynamically allocate a 2D array in the Matrix structure
    mstruc.matrix = malloc(sizeof(int*)*mstruc.numRows);
    for(int i = 0; i < mstruc.numRows; i++){
    	*(mstruc.matrix + i) = malloc(sizeof(int)*mstruc.numCols);
    }
   
    // Read the file line by line base on the number of rows
    char line[mstruc.numCols * 2];
    for(int i = 0; i< mstruc.numRows; i++){
	fscanf(infp, "%s", line);
	// Tokenize each line wrt comma to store the values in the matrix
	char *token;
	token = strtok(line, ",");
	*(*(mstruc.matrix+i) + 0) = atoi(token);
	for(int j = 1; j < mstruc.numCols; j++){
	    token = strtok(NULL, ",");
	    if(token == NULL){
	        printf("ERROR: Trouble reading data from file\n");
		exit(1);
	    }else{
	        *(*(mstruc.matrix +i) + j) = atoi(token);
	    }
	}
    } 
    
    // open the output file
    FILE *outfp;
    outfp = fopen(argv[2], "w");
    if(outfp == NULL){
        printf("ERROR: output file could not be opened\n");
        exit(1);
    }

    // Call the function hill_climb
    hill_climb(outfp, &mstruc);

    // Free the dynamically allocated memory
    for(int i = 0; i < mstruc.numRows; i++){
        free(*(mstruc.matrix + i));
    }
    free(mstruc.matrix);

    // Close the input file  
    fclose(infp);
    
    // Close the output file
    fclose(outfp);
    return 0; 
}   
