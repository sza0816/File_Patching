#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

/**
 * @brief Validates command line arguments passed to the program.
 * @details This function will validate all the arguments passed to the
 * program, returning 0 if validation succeeds and -1 if validation fails.
 * Upon successful return, the various options that were specified will be
 * encoded in the global variable 'global_options', where it will be
 * accessible elsewhere in the program.  For details of the required
 * encoding, see the assignment handout.
 *
 * @param argc The number of arguments passed to the program from the CLI.
 * @param argv The argument strings passed to the program from the CLI.
 * @return 0 if validation succeeds and -1 if validation fails.
 * @modifies global variable "global_options" to contain an encoded representation
 * of the selected program options.
 * @modifies global variable "diff_filename" to point to the name of the file
 * containing the diffs to be used.
 */

int validargs(int argc, char **argv) {
    int h=1;
    int n=0;
    int q=0;
    int i;
    int return_value=0;
    // long global_options;
    // char *diff_filename;
    char *temp;

    for (i=0;i<argc;i++){
            if(**(argv+i)=='-'){
                // printf("- detected\n");
                if(*(*(argv+i)+1)=='n'){
                    n=1;
                }
                else if(*(*(argv+i)+1)=='q'){
                    q=1;
                }
            }
        }
    if(argc<=1){
        //stderr invalid string length-too short
        // fprintf(stderr,"invalid string length\n");
        return_value=-1;
    }

    // printf("%s            line 41 \n", *(argv+1));
    // debug()
    // printf("have -h?    %s\n", *(*(argv+1)+1)=='h'? "yes":"no");
    
    else if(*(*(argv+1)+1)=='h'){
        return_value=0;
    }
    else if(*(*(argv+1)+1)!='h'){
        // printf("no -h\n");
        h=0;
       
        // printf("h=%d, n=%d, q=%d\n",h, n, q);        // print # of h, n, q
        // print the first char in the last argument to see if it is a '-'
        // printf("%d, %d\n",argc, *(argv+argc-1));

        //check if the last argument is a filename
        if(*(*(argv+argc-1))=='-'){
            // fprintf(stderr,"last argument is not a filename.\n");
            return_value=-1;
        }
        //modifies global variable diff_filename to point to the last argument
        else{
            diff_filename=*(argv+argc-1);
            // printf("diff_filename in validargs: %s\n",diff_filename);
        }
        
        
    }
    
    global_options+=4*q+2*n+h;                           //global options
    // printf("global_options: %ld\n",global_options);

    // printf("%s %s",EXIT_FAILURE,EXIT_SUCCESS);
    // printf("passed with code 0\n");
    return return_value;
}
