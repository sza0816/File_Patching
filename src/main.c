#include <stdio.h>
#include <stdlib.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

#ifdef _STRING_H
#error "Do not #include <string.h>. You will get a ZERO."
#endif

#ifdef _STRINGS_H
#error "Do not #include <strings.h>. You will get a ZERO."
#endif

#ifdef _CTYPE_H
#error "Do not #include <ctype.h>. You will get a ZERO."
#endif

int main(int argc, char **argv)
{
    if(validargs(argc, argv))
        USAGE(*argv, EXIT_FAILURE);
    if(global_options == HELP_OPTION)
        USAGE(*argv, EXIT_SUCCESS);


    //-------------------------- TO BE IMPLEMENTED ------------------------
    debug("diff_filename in main: %s\n",diff_filename);
    FILE *f=fopen(diff_filename,"r");             // file pointer to the diff file

    if(fopen(diff_filename,"r")!=NULL){       // checks whether the file exists and open it
        debug("file exists.\n\n");

        

        // struct hunk empty_hunk;
        // HUNK *hp=&empty_hunk;
        // hunk_next(hp,f);

        // for (int i=0;i<280;i++){
        //     hunk_getc(hp, f);
        // }

        // FILE *out=fopen("./rsrc/empty", "r");
        // if(out!=NULL)
        //     hunk_show(hp, out);
        // fclose(out);



        int return_value=patch(stdin,stdout,f);

        fclose(f);                                 // close diff file
        if(return_value==0){
            debug("exit success in main\n");
            return EXIT_SUCCESS;
        }
        
    }
    else{
        debug("file doesn't exist.\n");
    }
    

    // --------------------------------------------------------------------
    return EXIT_FAILURE; 
}

/*
 * Just a reminder: All non-main functions should
 * be in another file not named main.c
 */
