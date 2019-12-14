////////////////////////////////////////////////////////////////////////////////
// Main File:        n_in_a_row.c
// This File:        n_in_a_row.c
// Other Files:      n/a
// Semester:         CS 354 Fall 2019
//
// Author:           Matthew Chiang
// Email:            mchiang7@wisc.edu
// CS Login:         mchiang
//
/////////////////////////// OTHER SOURCES OF HELP //////////////////////////////
//                   fully acknowledge and credit all sources of help,
//                   other than Instructors and TAs.
//
// Persons:          Identify persons by name, relationship to you, and email.
//                   Describe in detail the the ideas and help they provided.
//
// Online sources:   avoid web searches to solve your problems, but if you do
//                   search, be sure to include Web URLs and description of
//                   of any information you find.
////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>  
#include <stdlib.h>
#include <string.h>     

char *COMMA = ",";  

/* COMPLETED:       
 * Retrieves from the first line of the input file,
 * the size of the board (number of rows and columns).
 * 
 * fp: file pointer for input file
 * size: pointer to size
 */
void get_dimensions(FILE *fp, int *size) {      
    char *line = NULL;
    size_t len = 0;
    if (getline(&line, &len, fp) == -1) {
        printf("Error in reading the file\n");
        exit(1);
    }

    char *token = NULL;
    token = strtok(line, COMMA);
    *size = atoi(token);
}   


/* TODO:
 * Returns 1 if and only if the board is in a valid state.
 * Otherwise returns 0.
 * 
 * board: heap allocated 2D board
 * size: number of rows and columns
 */
int n_in_a_row(int **board, int size) {
    // variable declaration
    int x_wins = 0; // number of wins for x
    int o_wins = 0; // number of wins for o
    int count; // sums up the values -1 for 0 and 1 for x
    int x_count = 0; // tracks number of x's in the board
    int o_count = 0; // tracks number of y's in the board 
    // checking rows
    for(int i = 0; i < size; i++){
	count = 0;
        for(int j = 0; j < size; j++){
	    int x_or_o = *(*(board+i)+j);
	    if(x_or_o == 1) count += 1;
	    else if (x_or_o == 2) count -= 1;
	}
	if(count == 1 * size) x_wins += 1;
	else if (count == -1 * size) o_wins += 1;
    }
    
    // checking columns
    for(int i = 0; i < size; i++){
        count = 0;
        for(int j = 0; j < size; j++){
            int x_or_o = *(*(board+j)+i);
            if(x_or_o == 1) {
		count += 1;
		x_count++;
	    }
            else if (x_or_o == 2){
		count -= 1;
		o_count++;
	    }
        }
        if(count == 1 * size) x_wins += 1;
        else if (count == -1 * size) o_wins += 1;
    }

    // checking left->right diagonal
    count = 0;
    for(int i = 0; i < size; i++){
    	int x_or_o = *(*(board + i) + i);
	 if(x_or_o == 1) count += 1;
         else if (x_or_o == 2) count -= 1;
    }
    if(count == 1 * size) x_wins += 1;
    else if (count == -1 * size) o_wins += 1;

    // checking right->left diagonal
    count = 0;
    for(int i = 0; i < size; i++){
        int x_or_o = *(*(board + i) + (size - i));
         if(x_or_o == 1) count += 1;
         else if (x_or_o == 2) count -= 1;
    }
    if(count == 1 * size) x_wins += 1;
    else if (count == -1 * size) o_wins += 1;

    // checking for valid number of wins
    if(x_wins + o_wins > 1) return 0;

    // checking for valide number of x and o's
    int xo_diff = x_count - o_count;
    if(xo_diff > 1 || xo_diff < 0) return 0;
    
    // All failing cases have been covered
    return 1;   
}     


/* PARTIALLY COMPLETED:
 * This program prints Valid if the input file contains
 * a game board with either 1 or no winners; and where
 * there is at most 1 more X than O.
 * 
 * argc: CLA count
 * argv: CLA value
 */
int main(int argc, char *argv[]) {              

    //TODO: Check if number of command-line arguments is correct.
    if (argc != 2) {
        printf("Number of command-line arguments is not correct\n - Should be in format ./n_in_a_row <filename.txt>\n");
        exit(1);
    }


    //Open the file and check if it opened successfully.
    FILE *fp = fopen(*(argv + 1), "r");
    if (fp == NULL) {
        printf("Cannot open file for reading\n");
        exit(1);
    }


    //Declare local variables.
    int size;
    int **board; // stores the board
    int valid = 0; // determines if the board is in a valid state

    //TODO: Call get_dimensions to retrieve the board size.
    get_dimensions(fp, &size);

    //TODO: Dynamically allocate a 2D array of dimensions retrieved above.
    board = malloc(sizeof(int*)*size);
    for(int i = 0; i < size; i++){
	*(board+i) = malloc(sizeof(int)*size);
    }

    //Read the file line by line.
    //Tokenize each line wrt comma to store the values in your 2D array.
    char *line = NULL;
    size_t len = 0;
    char *token = NULL;
    for (int i = 0; i < size; i++) {

        if (getline(&line, &len, fp) == -1) {
            printf("Error while reading the file\n");
            exit(1);
        }

        token = strtok(line, COMMA);
        for (int j = 0; j < size; j++) {
            //TODO: Complete the line of code below
            //to initialize your 2D array.
            /* ADD ARRAY ACCESS CODE HERE */ *(*(board+i)+j)= atoi(token);
            token = strtok(NULL, COMMA);
        }
    }

    //TODO: Call the function n_in_a_row and print the appropriate
    //output depending on the function's return value.
    valid = n_in_a_row(board, size);
    if(valid == 1) printf("valid\n");
    else printf("invalid\n");


    //TODO: Free all dynamically allocated memory.
    for(int i = 0; i < size; i++){
        free(*(board+i));
    }
    free(board);

    //Close the file.
    if (fclose(fp) != 0) {
        printf("Error while closing the file\n");
        exit(1);
    } 

    return 0;       
}       

