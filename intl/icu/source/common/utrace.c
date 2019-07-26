










#define   UTRACE_IMPL
#include "unicode/utrace.h"
#include "utracimp.h"
#include "cstring.h"
#include "uassert.h"
#include "ucln_cmn.h"


static UTraceEntry     *pTraceEntryFunc = NULL;
static UTraceExit      *pTraceExitFunc  = NULL;
static UTraceData      *pTraceDataFunc  = NULL;
static const void      *gTraceContext   = NULL;

U_EXPORT int32_t
utrace_level = UTRACE_ERROR;

U_CAPI void U_EXPORT2
utrace_entry(int32_t fnNumber) {
    if (pTraceEntryFunc != NULL) {
        (*pTraceEntryFunc)(gTraceContext, fnNumber);
    }
}


static const char gExitFmt[]             = "Returns.";
static const char gExitFmtValue[]        = "Returns %d.";
static const char gExitFmtStatus[]       = "Returns.  Status = %d.";
static const char gExitFmtValueStatus[]  = "Returns %d.  Status = %d.";
static const char gExitFmtPtrStatus[]    = "Returns %d.  Status = %p.";

U_CAPI void U_EXPORT2
utrace_exit(int32_t fnNumber, int32_t returnType, ...) {
    if (pTraceExitFunc != NULL) {
        va_list     args;
        const char *fmt;

        switch (returnType) {
        case 0:
            fmt = gExitFmt;
            break;
        case UTRACE_EXITV_I32:
            fmt = gExitFmtValue;
            break;
        case UTRACE_EXITV_STATUS:
            fmt = gExitFmtStatus;
            break;
        case UTRACE_EXITV_I32 | UTRACE_EXITV_STATUS:
            fmt = gExitFmtValueStatus;
            break;
        case UTRACE_EXITV_PTR | UTRACE_EXITV_STATUS:
            fmt = gExitFmtPtrStatus;
            break;
        default:
            U_ASSERT(FALSE);
            fmt = gExitFmt;
        }

        va_start(args, returnType);
        (*pTraceExitFunc)(gTraceContext, fnNumber, fmt, args);
        va_end(args);
    }
}
 

 
U_CAPI void U_EXPORT2 
utrace_data(int32_t fnNumber, int32_t level, const char *fmt, ...) {
    if (pTraceDataFunc != NULL) {
           va_list args;
           va_start(args, fmt ); 
           (*pTraceDataFunc)(gTraceContext, fnNumber, level, fmt, args);
           va_end(args);
    }
}


static void outputChar(char c, char *outBuf, int32_t *outIx, int32_t capacity, int32_t indent) {
    int32_t i;
    








    if (*outIx==0 ||   
        (c!='\n' && c!=0 && *outIx < capacity && outBuf[(*outIx)-1]=='\n') ||  
        (c=='\n' && *outIx>=capacity))    
    {
        
        for(i=0; i<indent; i++) {
            if (*outIx < capacity) {
                outBuf[*outIx] = ' ';
            }
            (*outIx)++;
        }
    }

    if (*outIx < capacity) {
        outBuf[*outIx] = c;
    }
    if (c != 0) {
        


        (*outIx)++;
    }
}

static void outputHexBytes(int64_t val, int32_t charsToOutput,
                           char *outBuf, int32_t *outIx, int32_t capacity) {
    static const char gHexChars[] = "0123456789abcdef";
    int32_t shiftCount;
    for  (shiftCount=(charsToOutput-1)*4; shiftCount >= 0; shiftCount-=4) {
        char c = gHexChars[(val >> shiftCount) & 0xf];
        outputChar(c, outBuf, outIx, capacity, 0);
    }
}


static void outputPtrBytes(void *val, char *outBuf, int32_t *outIx, int32_t capacity) {
    int32_t  i;
    int32_t  incVal = 1;              
    char     *p     = (char *)&val;   

#if !U_IS_BIG_ENDIAN
    
    incVal = -1;
    p += sizeof(void *) - 1;
#endif

    

    for (i=0; i<sizeof(void *); i++) {
        outputHexBytes(*p, 2, outBuf, outIx, capacity);
        p += incVal;
    }
}

static void outputString(const char *s, char *outBuf, int32_t *outIx, int32_t capacity, int32_t indent) {
    int32_t i = 0;
    char    c;
    if (s==NULL) {
        s = "*NULL*";
    }
    do {
        c = s[i++];
        outputChar(c, outBuf, outIx, capacity, indent);
    } while (c != 0);
}
        


static void outputUString(const UChar *s, int32_t len, 
                          char *outBuf, int32_t *outIx, int32_t capacity, int32_t indent) {
    int32_t i = 0;
    UChar   c;
    if (s==NULL) {
        outputString(NULL, outBuf, outIx, capacity, indent);
        return;
    }

    for (i=0; i<len || len==-1; i++) {
        c = s[i];
        outputHexBytes(c, 4, outBuf, outIx, capacity);
        outputChar(' ', outBuf, outIx, capacity, indent);
        if (len == -1 && c==0) {
            break;
        }
    }
}
        
U_CAPI int32_t U_EXPORT2
utrace_vformat(char *outBuf, int32_t capacity, int32_t indent, const char *fmt, va_list args) {
    int32_t   outIx  = 0;
    int32_t   fmtIx  = 0;
    char      fmtC;
    char      c;
    int32_t   intArg;
    int64_t   longArg = 0;
    char      *ptrArg;

    

    for (;;) {
        fmtC = fmt[fmtIx++];
        if (fmtC != '%') {
            
            outputChar(fmtC, outBuf, &outIx, capacity, indent);
            if (fmtC == 0) {
                



                break;
            }
            continue;
        }

        
        fmtC = fmt[fmtIx++];

        switch (fmtC) {
        case 'c':
            
            c = (char)va_arg(args, int32_t);
            outputChar(c, outBuf, &outIx, capacity, indent);
            break;

        case 's':
            
            ptrArg = va_arg(args, char *);
            outputString((const char *)ptrArg, outBuf, &outIx, capacity, indent);
            break;

        case 'S':
            
            ptrArg = va_arg(args, void *);             
            intArg =(int32_t)va_arg(args, int32_t);    
            outputUString((const UChar *)ptrArg, intArg, outBuf, &outIx, capacity, indent);
            break;

        case 'b':
            
            intArg = va_arg(args, int);
            outputHexBytes(intArg, 2, outBuf, &outIx, capacity);
            break;

        case 'h':
            
            intArg = va_arg(args, int);
            outputHexBytes(intArg, 4, outBuf, &outIx, capacity);
            break;

        case 'd':
            
            intArg = va_arg(args, int);
            outputHexBytes(intArg, 8, outBuf, &outIx, capacity);
            break;

        case 'l':
            
            longArg = va_arg(args, int64_t);
            outputHexBytes(longArg, 16, outBuf, &outIx, capacity);
            break;
            
        case 'p':
            
            ptrArg = va_arg(args, void *);
            outputPtrBytes(ptrArg, outBuf, &outIx, capacity);
            break;

        case 0:
            



            outputChar('%', outBuf, &outIx, capacity, indent);
            fmtIx--;
            break;

        case 'v':
            {
                
                char     vectorType;
                int32_t  vectorLen;
                const char   *i8Ptr;
                int16_t  *i16Ptr;
                int32_t  *i32Ptr;
                int64_t  *i64Ptr;
                void     **ptrPtr;
                int32_t   charsToOutput = 0;
                int32_t   i;
                
                vectorType = fmt[fmtIx];    
                if (vectorType != 0) {
                    fmtIx++;
                }
                i8Ptr = (const char *)va_arg(args, void*);
                i16Ptr = (int16_t *)i8Ptr;
                i32Ptr = (int32_t *)i8Ptr;
                i64Ptr = (int64_t *)i8Ptr;
                ptrPtr = (void **)i8Ptr;
                vectorLen =(int32_t)va_arg(args, int32_t);
                if (ptrPtr == NULL) {
                    outputString("*NULL* ", outBuf, &outIx, capacity, indent);
                } else {
                    for (i=0; i<vectorLen || vectorLen==-1; i++) { 
                        switch (vectorType) {
                        case 'b':
                            charsToOutput = 2;
                            longArg = *i8Ptr++;
                            break;
                        case 'h':
                            charsToOutput = 4;
                            longArg = *i16Ptr++;
                            break;
                        case 'd':
                            charsToOutput = 8;
                            longArg = *i32Ptr++;
                            break;
                        case 'l':
                            charsToOutput = 16;
                            longArg = *i64Ptr++;
                            break;
                        case 'p':
                            charsToOutput = 0;
                            outputPtrBytes(*ptrPtr, outBuf, &outIx, capacity);
                            longArg = *ptrPtr==NULL? 0: 1;    
                            ptrPtr++;
                            break;
                        case 'c':
                            charsToOutput = 0;
                            outputChar(*i8Ptr, outBuf, &outIx, capacity, indent);
                            longArg = *i8Ptr;    
                            i8Ptr++;
                            break;
                        case 's':
                            charsToOutput = 0;
                            outputString(*ptrPtr, outBuf, &outIx, capacity, indent);
                            outputChar('\n', outBuf, &outIx, capacity, indent);
                            longArg = *ptrPtr==NULL? 0: 1;   
                            ptrPtr++;
                            break;

                        case 'S':
                            charsToOutput = 0;
                            outputUString((const UChar *)*ptrPtr, -1, outBuf, &outIx, capacity, indent);
                            outputChar('\n', outBuf, &outIx, capacity, indent);
                            longArg = *ptrPtr==NULL? 0: 1;   
                            ptrPtr++;
                            break;

                            
                        }
                        if (charsToOutput > 0) {
                            outputHexBytes(longArg, charsToOutput, outBuf, &outIx, capacity);
                            outputChar(' ', outBuf, &outIx, capacity, indent);
                        }
                        if (vectorLen == -1 && longArg == 0) {
                            break;
                        }
                    }
                }
                outputChar('[', outBuf, &outIx, capacity, indent);
                outputHexBytes(vectorLen, 8, outBuf, &outIx, capacity);
                outputChar(']', outBuf, &outIx, capacity, indent);
            }
            break;


        default:
            



             outputChar(fmtC, outBuf, &outIx, capacity, indent);
        }
    }
    outputChar(0, outBuf, &outIx, capacity, indent);  
    return outIx + 1;     
}




U_CAPI int32_t U_EXPORT2
utrace_format(char *outBuf, int32_t capacity,
                int32_t indent, const char *fmt,  ...) {
    int32_t retVal;
    va_list args;
    va_start(args, fmt ); 
    retVal = utrace_vformat(outBuf, capacity, indent, fmt, args);
    va_end(args);
    return retVal;
}


U_CAPI void U_EXPORT2
utrace_setFunctions(const void *context,
                    UTraceEntry *e, UTraceExit *x, UTraceData *d) {
    pTraceEntryFunc = e;
    pTraceExitFunc  = x;
    pTraceDataFunc  = d;
    gTraceContext   = context;
}


U_CAPI void U_EXPORT2
utrace_getFunctions(const void **context,
                    UTraceEntry **e, UTraceExit **x, UTraceData **d) {
    *e = pTraceEntryFunc;
    *x = pTraceExitFunc;
    *d = pTraceDataFunc;
    *context = gTraceContext;
}

U_CAPI void U_EXPORT2
utrace_setLevel(int32_t level) {
    if (level < UTRACE_OFF) {
        level = UTRACE_OFF;
    }
    if (level > UTRACE_VERBOSE) {
        level = UTRACE_VERBOSE;
    }
    utrace_level = level;
}

U_CAPI int32_t U_EXPORT2
utrace_getLevel() {
    return utrace_level;
}


U_CFUNC UBool 
utrace_cleanup() {
    pTraceEntryFunc = NULL;
    pTraceExitFunc  = NULL;
    pTraceDataFunc  = NULL;
    utrace_level    = UTRACE_OFF;
    gTraceContext   = NULL;
    return TRUE;
}


static const char * const
trFnName[] = {
    "u_init",
    "u_cleanup",
    NULL
};


static const char * const
trConvNames[] = {
    "ucnv_open",
    "ucnv_openPackage",
    "ucnv_openAlgorithmic",
    "ucnv_clone",
    "ucnv_close",
    "ucnv_flushCache",
    "ucnv_load",
    "ucnv_unload",
    NULL
};

    
static const char * const
trCollNames[] = {
    "ucol_open",
    "ucol_close",
    "ucol_strcoll",
    "ucol_getSortKey",
    "ucol_getLocale",
    "ucol_nextSortKeyPart",
    "ucol_strcollIter",
    NULL
};

                
U_CAPI const char * U_EXPORT2
utrace_functionName(int32_t fnNumber) {
    if(UTRACE_FUNCTION_START <= fnNumber && fnNumber < UTRACE_FUNCTION_LIMIT) {
        return trFnName[fnNumber];
    } else if(UTRACE_CONVERSION_START <= fnNumber && fnNumber < UTRACE_CONVERSION_LIMIT) {
        return trConvNames[fnNumber - UTRACE_CONVERSION_START];
    } else if(UTRACE_COLLATION_START <= fnNumber && fnNumber < UTRACE_COLLATION_LIMIT){
        return trCollNames[fnNumber - UTRACE_COLLATION_START];
    } else {
        return "[BOGUS Trace Function Number]";
    }
}

