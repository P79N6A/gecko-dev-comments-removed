



















#include <stdio.h>
#include "unicode/putil.h"
#include "cstring.h"
#include "gentest.h"

static void
incKey(char *key, char *limit) {
    char c;

    while(limit>key) {
        c=*--limit;
        if(c=='o') {
            *limit='1';
            break;
        } else {
            *limit='o';
        }
    }
}

U_CFUNC int
genres32(const char *prog, const char *path) {
    






    char key[20]="ooooooooooooooooo";
    char *limit;
    int i;
    char file[512];
    FILE *out;

    uprv_strcpy(file,path);
    if(file[strlen(file)-1]!=U_FILE_SEP_CHAR) {
        uprv_strcat(file,U_FILE_SEP_STRING);
    }
    uprv_strcat(file,"testtable32.txt");
    out = fopen(file, "w");
    
    puts("Generating testtable32.txt");
    if(out == NULL) {
        fprintf(stderr, "%s: Couldn't create resource test file %s\n",
                prog, file);
        return 1;
    }
    
    
    for(limit=key; *limit!=0; ++limit) {
    }

    
    fputs(
          "testtable32 {", out
    );

    
    for(i=0; i<66000; ++i) {
        if(i%10==0) {
            



            fprintf(out, "%s{\"\\U%08x\"}\n", key, i);
        } else {
            
            fprintf(out, "%s:int{%d}\n", key, i);
        }

        incKey(key, limit);
    }

    
    fputs(
          "}", out
    );

    fclose(out);
    return 0;
}
