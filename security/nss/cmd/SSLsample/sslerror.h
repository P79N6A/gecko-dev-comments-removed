



































#include <stdio.h>
#include <string.h>
#include "nspr.h"

struct tuple_str {
    PRErrorCode	 errNum;
    const char * errString;
};

typedef struct tuple_str tuple_str;

#define ER2(a,b)   {a, b},
#define ER3(a,b,c) {a, c},

#include "secerr.h"
#include "sslerr.h"

const tuple_str errStrings[] = {


#include "SSLerrs.h"
#include "SECerrs.h"
#include "NSPRerrs.h"

};

const PRInt32 numStrings = sizeof(errStrings) / sizeof(tuple_str);




const char *
SSL_Strerror(PRErrorCode errNum) {
    PRInt32 low  = 0;
    PRInt32 high = numStrings - 1;
    PRInt32 i;
    PRErrorCode num;
    static int initDone;

    


    if (!initDone) {
	PRErrorCode lastNum = (PRInt32)0x80000000;
    	for (i = low; i <= high; ++i) {
	    num = errStrings[i].errNum;
	    if (num <= lastNum) {
	    	fprintf(stderr, 
"sequence error in error strings at item %d\n"
"error %d (%s)\n"
"should come after \n"
"error %d (%s)\n",
		        i, lastNum, errStrings[i-1].errString, 
			num, errStrings[i].errString);
	    }
	    lastNum = num;
	}
	initDone = 1;
    }

    
    while (low + 1 < high) {
    	i = (low + high) / 2;
	num = errStrings[i].errNum;
	if (errNum == num) 
	    return errStrings[i].errString;
        if (errNum < num)
	    high = i;
	else 
	    low = i;
    }
    if (errNum == errStrings[low].errNum)
    	return errStrings[low].errString;
    if (errNum == errStrings[high].errNum)
    	return errStrings[high].errString;
    return NULL;
}
