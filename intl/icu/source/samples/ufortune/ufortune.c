






















#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "unicode/udata.h"     
#include "unicode/ures.h"      
#include "unicode/ustdio.h"    
                               
                               
#include "unicode/ustring.h"

#ifndef UFORTUNE_NOSETAPPDATA







extern  const void U_IMPORT *fortune_resources_dat;
#endif

void u_write(const UChar *what, int len);





int main(int argc, char **argv)
{
    UBool              displayUsage  = FALSE;    
                                                 
    UBool              verbose       = FALSE;    
    char              *optionError   = NULL;     
                                                 
    char              *locale=NULL;              
                                                 
    const char *       programName   = argv[0];  


    UFILE             *u_stdout;                 
    UErrorCode         err           = U_ZERO_ERROR;   
                                                       

    UResourceBundle   *myResources;              
    UResourceBundle   *fortunes_r;

    int32_t            numFortunes;              
    int                i;

    const UChar       *resString;                
    int32_t            len;


    




    for (i=1; i<argc; i++) {
        if (strcmp(argv[i], "-l") ==0) {
            if (++i < argc) {
                locale = argv[i];
            }
            continue;
        }
        if (strcmp(argv[i], "-v") == 0) {
            verbose = TRUE;
            continue;}
        if (strcmp(argv[i], "-?") == 0 ||
            strcmp(argv[i], "--help") == 0) {
            displayUsage = TRUE;
            continue;}
        optionError = argv[i];
        displayUsage = TRUE;
        break;
    }

    



    u_stdout = u_finit(stdout, NULL ,  NULL );
    if (verbose) {
        u_fprintf(u_stdout, "%s:  checking output via icuio.\n", programName);
    }

#ifndef UFORTUNE_NOSETAPPDATA
    




    udata_setAppData("fortune_resources", &fortune_resources_dat, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: udata_setAppData failed with error \"%s\"\n", programName, u_errorName(err));
        exit(-1);
    }
#endif

    

    myResources = ures_open("fortune_resources", locale, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_open failed with error \"%s\"\n", programName, u_errorName(err));
        exit(-1);
    }
    if (verbose) {
        u_fprintf(u_stdout, "status from ures_open(\"fortune_resources\", %s) is %s\n",
            locale? locale: " ", u_errorName(err));
    }

    



    if (optionError != NULL) {
        const UChar *msg = ures_getStringByKey(myResources, "optionMessage", &len, &err);
        if (U_FAILURE(err)) {
            fprintf(stderr, "%s: ures_getStringByKey(\"optionMessage\") failed, %s\n",
                programName, u_errorName(err));
            exit(-1);
        }
        u_file_write(msg,  len, u_stdout);              
        u_fprintf(u_stdout, " %s\n", optionError);      
    }

    if (displayUsage) {
        const UChar *usage;
        int          returnValue=0;

        usage = ures_getStringByKey(myResources, "usage", &len, &err);
        if (U_FAILURE(err)) {
            fprintf(stderr, "%s: ures_getStringByKey(\"usage\") failed, %s\n", programName, u_errorName(err));
            exit(-1);
        }
        u_file_write(usage,  len, u_stdout);
        if (optionError != NULL) {returnValue = -1;}
        return returnValue;
    }

    


    fortunes_r = ures_getByKey(myResources, "fortunes", NULL, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_getByKey(\"fortunes\") failed, %s\n", programName, u_errorName(err));
        exit(-1);
    }


    



    numFortunes = ures_countArrayItems(myResources, "fortunes", &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_countArrayItems(\"fortunes\") failed, %s\n", programName, u_errorName(err));
        exit(-1);
    }
    if (numFortunes <= 0) {
        fprintf(stderr, "%s: no fortunes found.\n", programName);
        exit(-1);
    }

    i = (int)time(NULL) % numFortunes;    
    resString = ures_getStringByIndex(fortunes_r, i, &len, &err);
    if (U_FAILURE(err)) {
        fprintf(stderr, "%s: ures_getStringByIndex(%d) failed, %s\n", programName, i, u_errorName(err));
        exit(-1);
    }

    u_file_write(resString, len, u_stdout);      
	u_fputc(0x0a, u_stdout);                     

    return 0;
}

