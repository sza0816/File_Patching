// more to do: 
// delete all uneccesary printf
// see piazza for more edge cases
// line 268, getc err print to std???

#include <stdlib.h>
#include <stdio.h>

#include "fliki.h"
#include "global.h"
#include "debug.h"

static int lines=1;        // global variable used to keep track of the line number 
// static int ccopy;
static int dashLines=0;
static int getc_mark=0;
static char *Ap=hunk_additions_buffer+2;         // pointers that points to the two buffers
static char *Dp=hunk_deletions_buffer+2;
static int hunk_readen=0;
static int cAdd=0;

static struct hunk header;
static HUNK *header_p=&header;                      // global hunk pointer used in patch, to be updated

/**
 * @brief peek the next character
 * @param in the stream in which huncks are being read
 * @return the next character of the file
*/
int hunk_peek(FILE *in){
    int c;
    int s;
    if((c=fgetc(in))!=EOF){
        s=ungetc(c, in);
        if(s==EOF){return EOF;}
        return c;
    }
    return EOF;
}

/**
 * @brief Get the header of the next hunk in a diff file.
 * @details This function advances to the beginning of the next hunk
 * in a diff file, reads and parses the header line of the hunk,
 * and initializes a HUNK structure with the result.
 *
 * @param hp  Pointer to a HUNK structure provided by the caller.
 * Information about the next hunk will be stored in this structure.
 * @param in  Input stream from which hunks are being read.
 * @return 0  if the next hunk was successfully located and parsed,
 * EOF if end-of-file was encountered or there was an error reading
 * from the input stream, or ERR if the data in the input stream
 * could not be properly interpreted as a hunk.
 */

int hunk_next(HUNK *hp, FILE *in) {
    int Num_1st_mark=-1;    // remember if the line is started with a number
    int mark=0;          // keep track of the index of each line
    int c;
    int is_2d=0;           // mark if there is a d at 2nd place being find
    int is_4d=0;           // mark if there is a d at 4th place being find
    int old_start=-1, old_end=-1, new_start=-1,new_end=-1;
    int type;
    int fstLineNotNum=0;
    int s;
    hunk_readen=0;

    getc_mark=0;
    while((c=fgetc(in))!=EOF){
        // debug("c: %c\n",c);
        mark++;                        // mark increases as a char being read
        
        if(mark==1){          // the 1st char must be a number
            if(c>='0'&& c<='9'){
                old_start=c-'0'; 
                Num_1st_mark++;
                // debug("passed mark 1, old start=%d\n",old_start);
            }
            else if(c<'0' || c>'9'){
                if(lines==1){
                    fstLineNotNum=1;
                }
            }
        }
        else if(mark==2){                          // 2nd char cannot be a number
            if(c=='a' || c=='d'||c=='c' || c==','){
                if(c!=','){
                    type=c;
                    old_end=old_start;
                    if(c=='d'){
                        is_2d=1;                       // mark a d at 2nd place in order to check at the 4th place
                    }
                }
            }
            // debug("passed mark 2, old end=%d\n",old_end);
        }
        else if(mark==3 && c>='0'&& c<='9'){         // 3rd char must be number
            if(old_end!=-1){                           //xax
                new_start=c-'0';
                
                if((c=fgetc(in))==EOF || c=='\n'){     //check if next one is '\n', if yes, new_end=new_start
                    new_end=new_start;
                }
                ungetc(c,in);
            }
            else if(old_end==-1){                     // x,xax
                old_end=c-'0';
            }
            // debug("passed mark 3, new start=%d\n",new_start);
        }
        else if(mark==4){                           // 4th char cannot be a
            if(c==',' || c=='d' || c=='c'){
                if(c!=',' && is_2d!=1){
                    type=c;
                }
                if(c=='d'){                         //mark a d at 4th place in order to check at the 6th place
                    is_4d=1;
                }
            }
            // debug("passed mark 4\n");
        }
        else if(mark==5 && c>='0'&& c<='9'){          // 5th char must be a number
            if(new_start==-1){
                new_start=c-'0';
                
                if((c=fgetc(in))==EOF || c=='\n'){       //check if next is \n, if yes, new_end=new_start
                    new_end=new_start;
                }
                ungetc(c,in);
            }
            else if(new_start!=-1){
                new_end=c-'0';
            }
            // debug("passed mark 5, new end=%d\n", new_end);
        }
        else if(mark==6 && c==',' && is_4d==1){               // 6th char must be a ','
            // debug("returned error at mark 6\n");
            return ERR;
        }
        else if(mark==7 && c>='0'&& c<='9'){       // 7th char must be a number for new_end
        // debug("passed mark 7\n");
            new_end=c-'0';
        }
        else if(mark>7 && Num_1st_mark==0){
            // debug("everything else situation\n");
            return ERR;
        }
        //-------------------after handling ERR situations, update hunk---------------------

        if(c=='\n'){                   // if reached the end of line, clear mark and loop to next line
            mark=0;
            is_2d=0;
            is_4d=0;
            Num_1st_mark--;            // change mark to none as the line ends
            lines++;
            // debug("lines: %d\nis first char in first line a number? %s\n",lines,fstLineNotNum==0?("yes"):("no"));
            // debug("current char: %c\n",c);
            if(fstLineNotNum==1){
                // debug("first line not a number\n");
                fstLineNotNum=0;
                return ERR;
            }
            if(old_start>-1 && old_end>-1 && new_start>-1 && new_end>-1){
                if(type=='a'){
                    (*hp).type=HUNK_APPEND_TYPE;
                }
                else if(type=='d'){
                    (*hp).type=HUNK_DELETE_TYPE;
                }
                else if(type=='c'){
                    (*hp).type=HUNK_CHANGE_TYPE;
                }
                (*hp).serial++;
                (*hp).old_start=old_start;
                (*hp).old_end=old_end;
                (*hp).new_start=new_start;
                (*hp).new_end=new_end;

                // debug("\ncurrent old start: %d\ncurrent old end: %d\ncurrent new start: %d\ncurrent new end: %d\ncurrent serial number: %d\ncurrent type: %d\n", old_start, old_end, new_start, new_end, (*hp).serial,(*hp).type);
                // debug("current char: %c\n",c=='\n'? ('N'): (c));

                //move one more space down to the 1st char of the hunk body
                c=fgetc(in);
                if(c==EOF){
                    return EOF;
                }else if(c!='\n'){             // if moved to 1st char of next line, move back
                    s=ungetc(c,in);
                    if(s==EOF){return EOF;}
                }
                // debug("current char: %c\n",c=='\n'? ('N'): (c));
                return 0;
            } 
        }
    }
    // debug("reached end of file!!\n");
    return EOF;
}

/**
 * @brief  Get the next character from the data portion of the hunk.
 * @details  This function gets the next character from the data
 * portion of a hunk.  The data portion of a hunk consists of one
 * or both of a deletions section and an additions section,
 * depending on the hunk type (delete, append, or change).
 * Within each section is a series of lines that begin either with
 * the character sequence "< " (for deletions), or "> " (for additions).
 * For a change hunk, which has both a deletions section and an
 * additions section, the two sections are separated by a single
 * line containing the three-character sequence "---".
 * This function returns only characters that are actually part of
 * the lines to be deleted or added; characters from the special
 * sequences "< ", "> ", and "---\n" are not returned.
 * @param hdr  Data structure containing the header of the current
 * hunk.
 *  
 * @param in  The stream from which hunks are being read.
 * @return  A character that is the next character in the current
 * line of the deletions section or additions section, unless the
 * end of the section has been reached, in which case the special
 * value EOS is returned.  If the hunk is ill-formed; for example,
 * if it contains a line that is not terminated by a newline character,
 * or if end-of-file is reached in the middle of the hunk, or a hunk
 * of change type is missing an additions section, then the special
 * value ERR (error) is returned.  The value ERR will also be returned
 * if this function is called after the current hunk has been completely
 * read, unless an intervening call to hunk_next() has been made to
 * advance to the next hunk in the input.  Once ERR has been returned,
 * then further calls to this function will continue to return ERR,
 * until a successful call to call to hunk_next() has successfully
 * advanced to the next hunk.
 */

int hunk_getc(HUNK *hp, FILE *in) {
    int c;
    int s;
    int type=(*hp).type;     //get the current type in order to check for hunk validary
    int size;               // size of each line, used to convert into little endian
    short ending=0;         // short int that occupies 2 bytes, used for the end of a hunk
    char *cp;               // pointer that jumps between the other 2 pointers
    int temp;

    if(type==1){            // make cp jump to the right buffer, also update 2 buffer pointers 
        cp=Ap;
    }
    else if(type==2){
        cp=Dp;
    }
    else if(type==3){
        if(dashLines!=3){
            cp=Dp;
        }
        else if(dashLines==3){
            cp=Ap;
        }
    }

    while((c=fgetc(in))!=EOF){
        // debug("c: %c\n",c);
        getc_mark ++; 
        // ------------------check for EOS and ERR here----------------------
        if(getc_mark==1){

            // debug("hunk_readen: %d\n", hunk_readen);


            if(c>='0'&& c<='9'){                                    // if starts with number, return end of section

                s=ungetc(c,in);   // move back to \n, so that next time c move to the 1st char of next line
                if(s==EOF){ debug("can not push c back\n"); return ERR;}
                getc_mark--;

                if(hunk_readen==1){              // check if hunk has been readen during last call
                    debug("hunk has been readen last time! , current c: %c, current mark: %d\n",c,getc_mark);
                    return ERR;
                }
                
                else if(type==3 && dashLines==3 && c!='>' && cAdd==1 && hunk_readen==0){        // check if there is addition section after dash
                    // debug("current ccopy: %c\n",ccopy=='\n'? 'N':c);
                    s=ungetc(c,in);   // move back to \n, so that next time c move to the 1st char of next line
                    if(s==EOF){ debug("can not push c back\n"); return ERR;}
                    // ccopy=c;
                    cAdd=0;
                    dashLines=0;
                    debug("change section - no addition\n");
                    return ERR;
                }
                // ccopy=c;
                cAdd=0;
                dashLines=0;       // set the number of dash lines as 0 at the end of the hunk
                cp-=2;             // move cp 2 bytes back if reached the end of section
                *cp=ending;
                hunk_readen=1;
                debug("reached end of the hunk section , current c: %c, current mark: %d\n", c, getc_mark);
                return EOS;
            }
            else if(c!='<' && c!='>' && c!='-'){          // if not started with certain char, return ERR
                debug("ill-formed file, c = %c, mark = %d\n",c,getc_mark);
                s=ungetc(c,in);    // move back to \n, so that next time c move to the 1st char of next line
                if(s==EOF){ debug("can not push c back\n"); return ERR;}
                getc_mark--;

                return ERR;
            }
            else if(type==1 && c=='<'){                        // if type=1 and c!='>' , return err
                s=ungetc(c,in);   // move back to \n, so that next time c move to the 1st char of next line
                if(s==EOF){ debug("can not push c back\n"); return ERR;}
                getc_mark--;

                debug("Addition section - no addition sign\n");
                return ERR;
            }
            else if(type==2 && c=='>'){                         // if type=2 and c!='<',  return err
                s=ungetc(c,in);   // move back to \n, so that next time c move to the 1st char of next line
                if(s==EOF){ debug("can not push c back\n"); return ERR;}
                getc_mark--;

                debug("Deletion section - no deletion sign\n");
                return ERR;
            } 
            else if (type==3 && dashLines==0 && c!='<' && c!='-'){            // if type 3 is missing '<', return err
                s=ungetc(c,in);   // move back to \n, so that next time c move to the 1st char of next line
                if(s==EOF){ debug("can not push c back\n"); return ERR;}
                getc_mark--;

                debug("Change section - no deletion sign, dashlines: %d\n",dashLines);
                return ERR;
            }
            else if(type==3 && c=='-'){
                hunk_readen=0;
                debug("next is a dashline\n");
                for(int i=0;i<3;i++){                  // run 3 times to get to the next \n
                    c=fgetc(in);
                    if(c==EOF){return ERR;}
                    dashLines++;
                }
                if(c != '\n'){return ERR;}

                cAdd=1;
                cp-=2;
                *cp=ending;                      // add ending to the deletion buffer if there are 3 dashes

                cp=Ap; // update cp and Ap when there are 3 dashes

                getc_mark=0;
                debug("addition starts, buffer position: %ld, %ld\n",Ap-hunk_additions_buffer, cp-hunk_additions_buffer);
                debug("reached ---, mark = %d\n", getc_mark);

                debug("current c: %c\n", c);
                return EOS;
            }
        
            else if (type==3 && dashLines==3 && hunk_readen==0 && c=='>' && cAdd==1){
                cAdd=0;
            }
        
        }

        else if(getc_mark==2){       // check if 2nd position is an empty space
            if(c!=' ' && type!=3){
                s=ungetc(c,in);    // move back to \n, so that next time c move to the 1st char of next line
                if(s==EOF){ debug("can not push c back\n"); return ERR;}
                getc_mark--;
                debug("2nd position is not a space, %c\n",c);
                return ERR;
            }
        }
           
        if(c=='\n' && getc_mark!=0){
            *cp=c;
            size=getc_mark-1;                                   //keep track of size of each line
            cp=cp-size-1;                                       // move to the begining for inserting size
            
            *cp=(unsigned short)((size & 0xffff));              //convert the size into a short which is two bytes
            // debug("line size in little endian : %d\n", *cp);
            cp=cp+size+4;                                       // move down for new lines

            if(type==1){Ap=cp;}
            else if(type==2){Dp=cp;}
            else if(type==3){
                if(dashLines==3){Ap=cp;}
                else if(dashLines!=3){Dp=cp;}
            }
        }

        //------------------check for output here-------------------
        if(c!='<' && c!='>' && c!='-'&& getc_mark>2){                    //check for deletion and addition signs
            if(c=='\n'){
                debug("-------------------end of the line!---------------------\n");
                getc_mark=0;   
            }

            if(cp<hunk_additions_buffer+510 || cp<hunk_deletions_buffer+510){
                *cp=c;             // insert char into the buffer
                // debug("\nchar in delete buffer: %c\n", *Dp);

                // debug("cp is now %c, getc mark: %d\n", *cp=='\n'? 'N':(*cp), getc_mark);
            
                cp++;

                if(type==1){Ap=cp;}                //update Ap or Dp to the position of cp
                else if(type==2){Dp=cp;}
                else if(type==3){
                    if(dashLines==3){Ap=cp;}
                    else if(dashLines!=3){Dp=cp;}
                }
                // move to the next position
                // debug("current cp position: %ld\n",cp-hunk_deletions_buffer);
                // debug("current Dp position: %ld\n",Dp-hunk_deletions_buffer);
                // debug("current cp position: %ld\n",cp-hunk_additions_buffer);
                // debug("current Ap position: %ld\n", Ap-hunk_additions_buffer);
            }    

            // else if both >=510, add 00 to both buffers
            if(Ap>=hunk_additions_buffer+510){
                Ap=hunk_additions_buffer+510;
                *Ap=ending;
            }
            else if(Dp>=hunk_deletions_buffer+510){
                Dp=hunk_deletions_buffer+510;
                *Dp=ending;
            }

            // debug("current char in buffer: %c\n",*cp);
            // debug("pointer position in addition buffer: %ld, dashlines: %d\n", Ap-hunk_additions_buffer,dashLines);
            // debug("pointer position in deletion buffer: %ld\n", Dp-hunk_deletions_buffer);
            // debug("next char is %c, mark : %d\n", c=='\n'? ('N'): (c),getc_mark);
            // nextLineMark=0;
            
            return c;
        }


    }
    debug("reached end of file!!\n");
    hunk_readen=1;
    dashLines=0;
    cp-=2;
    *cp=ending;               // add two 0's as the end of the array
    return EOS;
}



/**
 * @brief  Print a hunk to an output stream.
 * @details  This function prints a representation of a hunk to a
 * specified output stream.  The printed representation will always
 * have an initial line that specifies the type of the hunk and
 * the line numbers in the "old" and "new" versions of the file,
 * in the same format as it would appear in a traditional diff file.
 * The printed representation may also include portions of the
 * lines to be deleted and/or inserted by this hunk, to the extent
 * that they are available.  This information is defined to be
 * available if the hunk is the current hunk, which has been completely
 * read, and a call to hunk_next() has not yet been made to advance
 * to the next hunk.  In this case, the lines to be printed will
 * be those that have been stored in the hunk_deletions_buffer
 * and hunk_additions_buffer array.  If there is no current hunk,
 * or the current hunk has not yet been completely read, then no
 * deletions or additions information will be printed.
 * If the lines stored in the hunk_deletions_buffer or
 * hunk_additions_buffer array were truncated due to there having
 * been more data than would fit in the buffer, then this function
 * will print an elipsis "...  " followed by a single newline character
 * after any such truncated lines, as an indication that truncation
 * has occurred.
 *
 * @param hp  Data structure giving the header information about the
 * hunk to be printed.
 * @param out  Output stream to which the hunk should be printed.
 */

void hunk_show(HUNK *hp, FILE *out) {
    char *tp;

    // debug("%s", "hunk_show:");

    // debug("%p\n", hunk_additions_buffer);
    // debug("%p\n", Ap-1);
    // debug("%p\n", Dp-1);

    // debug("%c\n", *(Ap-1));
    // debug("%c\n", *(Dp-1));
    // debug("%c\n", *(hunk_deletions_buffer+3));

    int type;                 // set up the type of the header
    if((*hp).type==1)
        type='a';
    else if((*hp).type==2)
        type='d';
    else if((*hp).type==3)
        type='c';

    //--------------------------------insert header into out--------------------------------
    if((*hp).old_start==(*hp).old_end){
        if((*hp).new_start==(*hp).new_end){
            fprintf(out,"%d%c%d\n",(*hp).old_start,type,(*hp).new_start);
        }
        else{
            fprintf(out,"%d%c%d,%d\n",(*hp).old_start,type, (*hp).new_start,(*hp).new_end);
        }
    }
    else if ((*hp).old_start!=(*hp).old_end)
    {
        if((*hp).new_start==(*hp).new_end){
            fprintf(out,"%d,%d%c%d\n",(*hp).old_start,(*hp).old_end,type,(*hp).new_start);
        }
        else{
           fprintf(out,"%d,%d%c%d,%d\n",(*hp).old_start,(*hp).old_end,type,(*hp).new_start,(*hp).new_end); 
        }
    }

    //----------------------------------insert hunk body into out----------------------------
    if(hunk_readen==1){        // make sure buffer pointers are at the end???
        
        if(type=='a'){
            fprintf(out, "%c",'>');
            for(tp=hunk_additions_buffer+2;tp != Ap;tp++){
                fprintf(out, "%c", *tp);
                if(*tp=='\n'){
                    tp+=3;
                    fprintf(out, "%c", '>');
                }
                if(tp==hunk_additions_buffer+510){
                    fprintf(out, "...\n");
                    tp=Ap;
                }

            }
        }
        else if (type=='d'){
            fprintf(out, "%c",'<');
            for (tp=hunk_deletions_buffer+2; tp != Dp; tp++){
                fprintf(out, "%c",*tp);
                if(*tp=='\n'){
                    tp+=3;
                    fprintf(out, "%c", '<');
                }
                if(tp==hunk_deletions_buffer+510){
                    fprintf(out, "...\n");
                    tp=Dp;
                }

            }
        }
        else if (type=='c'){
            fprintf(out, "%c",'<');

            for (tp=hunk_deletions_buffer+2; tp != Dp; tp++){
                fprintf(out, "%c",*tp);
                if(*tp=='\n'){
                    tp+=3;
                    fprintf(out, "%c", '<');
                }
                if(tp==hunk_deletions_buffer+510){
                    fprintf(out, "...\n");
                    tp=Dp;
                }

            }
            
            int temp=*(hunk_additions_buffer+3);

            debug("the beginning of additions buffer: %d\n",temp);

            if(temp!=0){
                debug("---\n");
                fprintf(out, "---\n");
            }
            

            fprintf(out, "%c",'>');
            for(tp=hunk_additions_buffer+2;tp != Ap;tp++){
                debug("%c",*tp);
                fprintf(out, "%c", *tp);
                if(*tp=='\n'){
                    tp+=3;
                    fprintf(out, "%c", '>');
                }
                if(tp==hunk_additions_buffer+510){
                    fprintf(out, "...\n");
                    tp=Ap;
                }

            }
        }
    }
}

/**
 * @brief  Patch a file as specified by a diff.
 * @details  This function reads a diff file from an input stream
 * and uses the information in it to transform a source file, read on
 * another input stream into a target file, which is written to an
 * output stream.  The transformation is performed "on-the-fly"
 * as the input is read, without storing either it or the diff file
 * in memory, and errors are reported as soon as they are detected.
 * This mode of operation implies that in general when an error is
 * detected, some amount of output might already have been produced.
 * In case of a fatal error, processing may terminate prematurely,
 * having produced only a truncated version of the result.
 * In case the diff file is empty, then the output should be an
 * unchanged copy of the input.
 *
 * This function checks for the following kinds of errors: ill-formed
 * diff file, failure of lines being deleted from the input to match
 * the corresponding deletion lines in the diff file, failure of the
 * line numbers specified in each "hunk" of the diff to match the line
 * numbers in the old and new versions of the file, and input/output
 * errors while reading the input or writing the output.  When any
 * error is detected, a report of the error is printed to stderr.
 * The error message will consist of a single line of text that describes
 * what went wrong, possibly followed by a representation of the current
 * hunk from the diff file, if the error pertains to that hunk or its
 * application to the input file.  If the "quiet mode" program option
 * has been specified, then the printing of error messages will be
 * suppressed.  This function returns immediately after issuing an
 * error report.
 *
 * The meaning of the old and new line numbers in a diff file is slightly
 * confusing.  The starting line number in the "old" file is the number
 * of the first affected line in case of a deletion or change hunk,
 * but it is the number of the line *preceding* the addition in case of
 * an addition hunk.  The starting line number in the "new" file is
 * the number of the first affected line in case of an addition or change
 * hunk, but it is the number of the line *preceding* the deletion in
 * case of a deletion hunk.
 *
 * @param in  Input stream from which the file to be patched is read.
 * @param out Output stream to which the patched file is to be written.
 * @param diff  Input stream from which the diff file is to be read.
 * @return 0 in case processing completes without any errors, and -1
 * if there were errors.  If no error is reported, then it is guaranteed
 * that the output is complete and correct.  If an error is reported,
 * then the output may be incomplete or incorrect.
 */

int patch(FILE *in, FILE *out, FILE *diff) {
    int new_lines=1;                             // mark the number of lines in input file && output file
    int c;                                  // used to loop chars in input file
    int os,oe,ns,ne, type;
    int hunk_c;
    int temp;
    int next_header;
    int old_lines=1;                   // temp int to mark old line number
    int tempL;


    next_header=hunk_next(header_p,diff);
    temp=next_header;

    while(next_header!=ERR && next_header!=EOF){                //look for a header in diff
        debug("next_header:%d", next_header);
        debug("ERR:%d\tEOF:%d", ERR, EOF);

        type=(*header_p).type;
        os=(*header_p).old_start;
        oe=(*header_p).old_end;
        ns=(*header_p).new_start;
        ne=(*header_p).new_end;
        debug("type: %d,    os: %d,    oe: %d,     ns: %d,      ne: %d\n", type, os, oe, ns, ne);

        // check if header ends is greater than starts
        if(oe<os || ne<ns){
            debug("1\n");
            if(global_options != QUIET_OPTION){
                fprintf(stderr, "invalid header\n");
                hunk_show(header_p,stderr);
            }
            return ERR;
        }

        if(type==1 || (type==3 && dashLines==3)){                                             // addition

            debug("addition type: %d\n", type);

            // --------------------while did not reach the line of addition, add input file chars to output --------------------------
            while (new_lines < os && (c=fgetc(in))!=EOF){      
                debug("new lines: %d", new_lines);
                debug("%c",c);                   // print the input char && write it to output 
                if(global_options!=NO_PATCH_OPTION){
                    fprintf(out, "%c",c);
                }
                if(c=='\n'){                     // if meet \n, add a line
                    new_lines++;
                    // old_lines++;
                    debug("old lines: %d\n", old_lines);
                }
            }

            debug("old lines: %d, new lines: %d\n", old_lines, new_lines);

            //--------------------- when lines equals the starting line, add hunk body to output file ----------------------------
            hunk_c=hunk_getc(header_p, diff);
            tempL=new_lines;
            while(hunk_c!=EOS && hunk_c != ERR){
                debug("%c",hunk_c);                // print hunk body char and write it to output
                if(global_options!=NO_PATCH_OPTION){
                    debug("new lines: %d", new_lines);
                    fprintf(out, "%c",hunk_c);
                }
                
                if(hunk_c=='\n'){
                    new_lines++;
                    debug("lines: %d\n",new_lines);
                }
                
                if(hunk_c!=EOS){
                    hunk_c=hunk_getc(header_p, diff);
                }
            }
            if(os==0){
                new_lines--;
                // old_lines--;
                tempL--;
            }
            // debug("hunk_c: %c, lines: %d\n", hunk_c, lines);
            //else if,  eof/eos - look for next, err - err 
            if(hunk_c==ERR){
                if(global_options!=QUIET_OPTION){
                    fprintf(stderr,"error in reading hunk chars for addition\n");
                    hunk_show(header_p,stderr);
                }
                return ERR;
            }
            else if (hunk_c==EOS && new_lines!=ne-ns+1+tempL){
                debug("new lines: %d, tempL: %d\n",new_lines, tempL);
                if(global_options!=QUIET_OPTION){
                    fprintf(stderr,"line number does not match for addition\n");
                    hunk_show(header_p,stderr);
                }
                return ERR;
            }

        }
        else if(type==2 || (type==3 && dashLines!=3)){                  // deletion

            debug("deletion type: %d\n", type);
            debug("new start: %d\n",ns);

            while (old_lines < ns && (c=fgetc(in))!=EOF){      // keep reading input file into output waiting for lines to match ns
                debug("%c",c);
                if(global_options!=NO_PATCH_OPTION){
                    fprintf(out, "%c",c);
                }
                if(c=='\n'){               // increment lines
                    old_lines++;
                    // new_lines++;
                    debug("move to next line in new & old");
                }
            }

            debug("old lines: %d\n", old_lines);

            tempL=old_lines;
            hunk_c=hunk_getc(header_p,diff);
            while(hunk_c!=EOS && hunk_c != ERR){
                c=fgetc(in);
                // debug("c after fgetc: %c\n",c);

                debug("c: %c, hunk c: %c\n", c, hunk_c==EOS? 'S': hunk_c);
                debug("c==hunk_c? %c\n",c==hunk_c? 'Y':'N');

                if(c=='\n'){
                    old_lines++;
                    debug("old line updated to %d", old_lines);
                }

                if (c==EOF && hunk_c!=EOS){               // err if c ends before getc
                    if(global_options!=QUIET_OPTION){
                        fprintf(stderr,"file ends before deletion complete\n");
                        hunk_show(header_p,stderr);
                    }
                    return ERR;
                }
                else if (c!=hunk_c && hunk_c!=EOS){                    // err if c does not match getc
                    debug("c: %c, hunk c: %c\n", c, hunk_c==EOS? 'S': hunk_c);
                    if(global_options!=QUIET_OPTION){
                        fprintf(stderr,"deletion char does not match\n");
                        hunk_show(header_p,stderr);
                    }
                    return ERR;
                }
                else if(hunk_c==EOS && old_lines!=oe-os+1+tempL){
                    if(global_options!=QUIET_OPTION){
                        fprintf(stderr, "addition lines does not match\n");
                        hunk_show(header_p,stderr);
                    }
                }


                if(hunk_c!=EOS){
                    hunk_c=hunk_getc(header_p,diff);
                }
                
            }
            if(hunk_c==ERR){
                if(global_options!=QUIET_OPTION){
                    fprintf(stderr,"err in getting hunk body for deletion\n");
                    hunk_show(header_p,stderr);
                }
                return ERR;
            }
        }


        if(hunk_readen==1){                  // look for next header if the hunk is readen
            next_header=hunk_next(header_p, diff);
            debug("next_header:%d", next_header);
        }
    }

    if(next_header==ERR){
        if(global_options!=QUIET_OPTION){
            fprintf(stderr,"error in looking for header\n");
            hunk_show(header_p,stderr); 
        }
        
        return ERR;
    }

    if(temp==EOF){
        while((c=fgetc(in))!=EOF){
            debug("%c",c);
            if(global_options!=NO_PATCH_OPTION){
                fprintf(out, "%c",c);
            }
        }
    }
    
    debug("patch completed\n");
    return 0;

}
