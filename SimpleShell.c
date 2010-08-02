/*
 * File: SimpleShell.c
 * Author: Chris Lockfort (clockfort@csh.rit.removethispartforspam.edu)
 * 
 * A rather simple shell written in C;
 * good as an illustrative example on basic syscalls
 * or the like.
 */

/*
* Copyright (c) 2010  Chris Lockfort
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


// Look, I really like C++; I'll be needing these :-)
#define true (1==1)
#define false (!true)
typedef int bool;

#define ARG_MAX (64)

//Debug mode toggle
const bool DEBUG_MODE=true;
//const bool DEBUG_MODE=false;

char* token_array[ARG_MAX];
char* prompt = ">";

void tokenize_str(char string[]);
bool has_quit(char string[]);
bool has_redirection();
bool has_backgrounding();
void fork_n_run();

/* Main shell process, gives user a prompt, gets user input, processes it,
 * and executes it appropriately.
 */
int main(int argc, char** argv) {
    char string[256];
    while(true){
        fputs(prompt, stdout);
        fflush(stdout);
        if ( fgets(string, sizeof string, stdin) != NULL ){
            char *nextln = strchr(string,'\n');
            if ( nextln != NULL )
                *nextln = '\0';
        }
        if( string[0]!= '\0' ){
            if( has_quit(string) ){return(EXIT_SUCCESS);}
            tokenize_str(string);
            for(int i=0; (i<ARG_MAX) && !(token_array[i]==NULL); i++){
                if(DEBUG_MODE){printf("token_array[%i]=%s\n",i,token_array[i]);}
            }
            fork_n_run();
        }
    }
} 

// Split a string into tokens (words, in this case args)
void tokenize_str(char string[]){
    if(DEBUG_MODE){printf ("DEBUG: Splitting string \"%s\" into tokens:\n",string);}
    token_array[0]=strtok (string," ");
    if(DEBUG_MODE){printf("token_array[0]=%s\n",token_array[0]);}
    for(int i=1; i<ARG_MAX; i++)
    {
        token_array[i]= strtok (NULL, " ");
        if(token_array[i]==NULL)
            break;
        if(DEBUG_MODE){printf("token_array[%i]=%s\n",i,token_array[i]);}
    }
}

//Should we just exit?
bool has_quit(char string[] ){
    if(strstr(string,"quit") == &string[0] ){
        if(DEBUG_MODE){printf("Quit called, gracefully exiting...\n");}
        return true;
    }
    return false;
}

//Adjust for I/O redirection in this commmand string
void redirect_io(){
    char string[256];
    if(DEBUG_MODE){printf("DEBUG: Entering I/O redirection phase\n");}

    for(int i=0; !(token_array[i]==NULL); i++){
        if(!memcmp(token_array[i],">",1))
            if(token_array[i+1]!=NULL){
                strcpy(string,(char *)token_array[i+1]);
                //truncate args at where > used to be
                token_array[i]=NULL;
                token_array[i+1]=NULL;
                if(DEBUG_MODE){printf("Redirecting I/O to: %s\n",string);}
                if  (freopen (string,  "w"  , stdout) == NULL) {
                    printf ("Error in I/O redirection, likely could not open file %s for writing.\n",string);
                    exit(EXIT_FAILURE);
                }
            }
    }

    for(int j=0; !(token_array[j]==NULL); j++){
        if(!memcmp(token_array[j],"<",1) && j!=0){
            if(token_array[j-1]!=NULL){
                if(DEBUG_MODE){printf("DEBUG: < section \n");}
                strcpy(string,(char *)token_array[j+1]);
                for(int k=j; ;k++){
                    token_array[k]=token_array[k+2];
                    if(token_array[k]==NULL)
                        break;
                }
                if(DEBUG_MODE){printf("Redirecting I/O from: %s\n",string);}
                if  (freopen (string,  "r"  , stdin) == NULL) {
                    printf ("Error in I/O redirection, likely could not open file %s for reading.\n",string);
                    exit(EXIT_FAILURE);
                }
            }
        }
    }

}

//Is there backgrounding in this command string?
bool has_backgrounding(){
    for(int i=0; token_array[i]!=NULL; i++){
        if(!memcmp(token_array[i],"&",1) && token_array[i+1]==NULL){
                token_array[i]=NULL;//truncate args at where & used to be
                return true;
        }
    }
    return false;
}

//make syscalls fork and exec as necessary
void fork_n_run(){ //I had an ex-girlfriend like this...
    pid_t pid;
    int retvar;//child's return value, useful for debugging
    bool background_me=has_backgrounding();

    switch(pid=fork()) {
        case -1: //fork error
            perror("DEBUG: vfork() failure");
            exit(EXIT_FAILURE);

        case 0: //We're the child
            if(DEBUG_MODE){printf("DEBUG: Child created.\n");}
            redirect_io();
            execvp(token_array[0],token_array);
            exit(EXIT_SUCCESS);
            break;

        default: //We're the parent
            //if we want to background, don't wait for child process
            if(!background_me){
                wait(&retvar);
                if(DEBUG_MODE){printf("DEBUG: child retvar was: %i\n", retvar);}
            }
    }

}
