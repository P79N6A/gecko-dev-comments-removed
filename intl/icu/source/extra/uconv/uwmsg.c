














#include "unicode/ucnv.h"
#include "unicode/ustring.h"
#include "unicode/umsg.h"
#include "unicode/uwmsg.h"
#include "unicode/ures.h"
#include "unicode/putil.h"
#include "cstring.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BUF_SIZE 128


static void
uprint(const UChar *s,
       int32_t sourceLen,
       FILE *f,
       UErrorCode *status)
{
    
    UConverter *converter;
    char buf [BUF_SIZE];
    const UChar *mySource;
    const UChar *mySourceEnd;
    char *myTarget;
    int32_t arraySize;

    if(s == 0) return;

    
    mySource     = s;
    mySourceEnd  = mySource + sourceLen;
    myTarget     = buf;
    arraySize    = BUF_SIZE;

    
    converter = ucnv_open(0, status);

    
    if(U_FAILURE(*status)) goto finish;

    
    do {
        
        *status = U_ZERO_ERROR;

        
        ucnv_fromUnicode(converter, &myTarget,  myTarget + arraySize,
            &mySource, mySourceEnd, NULL,
            TRUE, status);

        
        fwrite(buf, sizeof(char), myTarget - buf, f);

        
        myTarget     = buf;
        arraySize    = BUF_SIZE;
    }
    while(*status == U_BUFFER_OVERFLOW_ERROR); 

finish:

    
    ucnv_close(converter);
}

static UResourceBundle *gBundle = NULL;

U_STRING_DECL(gNoFormatting, " (UCONFIG_NO_FORMATTING see uconfig.h)", 38);

U_CFUNC UResourceBundle *u_wmsg_setPath(const char *path, UErrorCode *err)
{
  if(U_FAILURE(*err))
  {
    return 0;
  }

  if(gBundle != NULL)
  {
    *err = U_ILLEGAL_ARGUMENT_ERROR;
    return 0;
  }
  else
  {
    UResourceBundle *b = NULL;
    b = ures_open(path, NULL, err);
    if(U_FAILURE(*err))
    {
         return 0;
    }

    gBundle = b;

    U_STRING_INIT(gNoFormatting, " (UCONFIG_NO_FORMATTING see uconfig.h)", 38);
  }
  
  return gBundle;
}


U_CFUNC int u_wmsg(FILE *fp, const char *tag, ... )
{
    const UChar *msg;
    int32_t      msgLen;
    UErrorCode  err = U_ZERO_ERROR;
#if !UCONFIG_NO_FORMATTING
    va_list ap;
#endif
    UChar   result[4096];
    int32_t resultLength = UPRV_LENGTHOF(result);

    if(gBundle == NULL)
    {
#if 0
        fprintf(stderr, "u_wmsg: No path set!!\n"); 
#endif
        return -1;
    }

    msg = ures_getStringByKey(gBundle, tag, &msgLen, &err);

    if(U_FAILURE(err))
    {
        return -1;
    }

#if UCONFIG_NO_FORMATTING
    resultLength = sizeof(gNoFormatting) / U_SIZEOF_UCHAR;
    if((msgLen + resultLength) <= UPRV_LENGTHOF(result)) {
        memcpy(result, msg, msgLen * U_SIZEOF_UCHAR);
        memcpy(result + msgLen, gNoFormatting, resultLength);
        resultLength += msgLen;
        uprint(result, resultLength, fp, &err);
    } else {
        uprint(msg,msgLen, fp, &err);
    }
#else
    va_start(ap, tag);

    resultLength = u_vformatMessage(uloc_getDefault(), msg, msgLen, result, resultLength, ap, &err);

    va_end(ap);

    if(U_FAILURE(err))
    {
#if 0
        fprintf(stderr, "u_wmsg: failed to format %s:%s, err %s\n",
            uloc_getDefault(),
            tag,
            u_errorName(err));
#endif
        err = U_ZERO_ERROR;
        uprint(msg,msgLen, fp, &err);
        return -1;
    }

    uprint(result, resultLength, fp, &err);
#endif

    if(U_FAILURE(err))
    {
#if 0
        fprintf(stderr, "u_wmsg: failed to print %s: %s, err %s\n",
            uloc_getDefault(),
            tag,
            u_errorName(err));
#endif
        return -1;
    }

    return 0;
}


UChar **gInfoMessages = NULL;

UChar **gErrMessages = NULL;

static const UChar *fetchErrorName(UErrorCode err)
{
    if (!gInfoMessages) {
        gInfoMessages = (UChar **)malloc((U_ERROR_WARNING_LIMIT-U_ERROR_WARNING_START)*sizeof(UChar*));
        memset(gInfoMessages, 0, (U_ERROR_WARNING_LIMIT-U_ERROR_WARNING_START)*sizeof(UChar*));
    }
    if (!gErrMessages) {
        gErrMessages = (UChar **)malloc(U_ERROR_LIMIT*sizeof(UChar*));
        memset(gErrMessages, 0, U_ERROR_LIMIT*sizeof(UChar*));
    }
    if(err>=0)
        return gErrMessages[err];
    else
        return gInfoMessages[err-U_ERROR_WARNING_START];
}

U_CFUNC const UChar *u_wmsg_errorName(UErrorCode err)
{
    UChar *msg;
    int32_t msgLen;
    UErrorCode subErr = U_ZERO_ERROR;
    const char *textMsg = NULL;

    
    msg = (UChar*)fetchErrorName(err);

    if(msg)
    {
        return msg;
    }

    if(gBundle == NULL)
    {
        msg = NULL;
    }
    else
    {
        const char *errname = u_errorName(err);
        if (errname) {
            msg = (UChar*)ures_getStringByKey(gBundle, errname, &msgLen, &subErr);
            if(U_FAILURE(subErr))
            {
                msg = NULL;
            }
        }
    }

    if(msg == NULL)  
    {
        char error[128];
        textMsg = u_errorName(err);
        if (!textMsg) {
            sprintf(error, "UNDOCUMENTED ICU ERROR %d", err);
            textMsg = error;
        }
        msg = (UChar*)malloc((strlen(textMsg)+1)*sizeof(msg[0]));
        u_charsToUChars(textMsg, msg, (int32_t)(strlen(textMsg)+1));
    }

    if(err>=0)
        gErrMessages[err] = msg;
    else
        gInfoMessages[err-U_ERROR_WARNING_START] = msg;

    return msg;
}
