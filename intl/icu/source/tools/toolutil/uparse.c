



















#include "unicode/utypes.h"
#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/utf16.h"
#include "cstring.h"
#include "filestrm.h"
#include "uparse.h"
#include "ustr_imp.h"

#include <stdio.h>

U_CAPI const char * U_EXPORT2
u_skipWhitespace(const char *s) {
    while(U_IS_INV_WHITESPACE(*s)) {
        ++s;
    }
    return s;
}

U_CAPI char * U_EXPORT2
u_rtrim(char *s) {
    char *end=uprv_strchr(s, 0);
    while(s<end && U_IS_INV_WHITESPACE(*(end-1))) {
        *--end = 0;
    }
    return end;
}









static const char *
getMissingLimit(const char *s) {
    const char *s0=s;
    if(
        *(s=u_skipWhitespace(s))=='#' &&
        *(s=u_skipWhitespace(s+1))=='@' &&
        0==strncmp((s=u_skipWhitespace(s+1)), "missing", 7) &&
        *(s=u_skipWhitespace(s+7))==':'
    ) {
        return u_skipWhitespace(s+1);
    } else {
        return s0;
    }
}

U_CAPI void U_EXPORT2
u_parseDelimitedFile(const char *filename, char delimiter,
                     char *fields[][2], int32_t fieldCount,
                     UParseLineFn *lineFn, void *context,
                     UErrorCode *pErrorCode) {
    FileStream *file;
    char line[300];
    char *start, *limit;
    int32_t i, length;

    if(U_FAILURE(*pErrorCode)) {
        return;
    }

    if(fields==NULL || lineFn==NULL || fieldCount<=0) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    if(filename==NULL || *filename==0 || (*filename=='-' && filename[1]==0)) {
        filename=NULL;
        file=T_FileStream_stdin();
    } else {
        file=T_FileStream_open(filename, "r");
    }
    if(file==NULL) {
        *pErrorCode=U_FILE_ACCESS_ERROR;
        return;
    }

    while(T_FileStream_readLine(file, line, sizeof(line))!=NULL) {
        
        length=(int32_t)(u_rtrim(line)-line);

        




        start=(char *)getMissingLimit(line);
        if(start==line) {
            *pErrorCode=U_ZERO_ERROR;
        } else {
            *pErrorCode=U_USING_DEFAULT_WARNING;
        }

        
        if(*start==0 || *start=='#') {
            continue;
        }

        
        limit=uprv_strchr(start, '#');
        if(limit!=NULL) {
            
            while(limit>start && U_IS_INV_WHITESPACE(*(limit-1))) {
                --limit;
            }

            
            *limit=0;
        }

        
        if(u_skipWhitespace(start)[0]==0) {
            continue;
        }

        
        for(i=0; i<fieldCount; ++i) {
            
            limit=start;
            while(*limit!=delimiter && *limit!=0) {
                ++limit;
            }

            
            fields[i][0]=start;
            fields[i][1]=limit;

            
            start=limit;
            if(*start!=0) {
                ++start;
            } else if(i+1<fieldCount) {
                *pErrorCode=U_PARSE_ERROR;
                limit=line+length;
                i=fieldCount;
                break;
            }
        }

        
        if(U_FAILURE(*pErrorCode)) {
            break;
        }

        
        lineFn(context, fields, fieldCount, pErrorCode);
        if(U_FAILURE(*pErrorCode)) {
            break;
        }
    }

    if(filename!=NULL) {
        T_FileStream_close(file);
    }
}






U_CAPI int32_t U_EXPORT2
u_parseCodePoints(const char *s,
                  uint32_t *dest, int32_t destCapacity,
                  UErrorCode *pErrorCode) {
    char *end;
    uint32_t value;
    int32_t count;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(s==NULL || destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    count=0;
    for(;;) {
        s=u_skipWhitespace(s);
        if(*s==';' || *s==0) {
            return count;
        }

        
        value=(uint32_t)uprv_strtoul(s, &end, 16);
        if(end<=s || (!U_IS_INV_WHITESPACE(*end) && *end!=';' && *end!=0) || value>=0x110000) {
            *pErrorCode=U_PARSE_ERROR;
            return 0;
        }

        
        if(count<destCapacity) {
            dest[count++]=value;
        } else {
            *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
        }

        
        s=end;
    }
}







U_CAPI int32_t U_EXPORT2
u_parseString(const char *s,
              UChar *dest, int32_t destCapacity,
              uint32_t *pFirst,
              UErrorCode *pErrorCode) {
    char *end;
    uint32_t value;
    int32_t destLength;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(s==NULL || destCapacity<0 || (destCapacity>0 && dest==NULL)) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    if(pFirst!=NULL) {
        *pFirst=0xffffffff;
    }

    destLength=0;
    for(;;) {
        s=u_skipWhitespace(s);
        if(*s==';' || *s==0) {
            if(destLength<destCapacity) {
                dest[destLength]=0;
            } else if(destLength==destCapacity) {
                *pErrorCode=U_STRING_NOT_TERMINATED_WARNING;
            } else {
                *pErrorCode=U_BUFFER_OVERFLOW_ERROR;
            }
            return destLength;
        }

        
        value=(uint32_t)uprv_strtoul(s, &end, 16);
        if(end<=s || (!U_IS_INV_WHITESPACE(*end) && *end!=';' && *end!=0) || value>=0x110000) {
            *pErrorCode=U_PARSE_ERROR;
            return 0;
        }

        
        if(pFirst!=NULL) {
            *pFirst=value;
            pFirst=NULL;
        }

        
        if((destLength+U16_LENGTH(value))<=destCapacity) {
            U16_APPEND_UNSAFE(dest, destLength, value);
        } else {
            destLength+=U16_LENGTH(value);
        }

        
        s=end;
    }
}


U_CAPI int32_t U_EXPORT2
u_parseCodePointRangeAnyTerminator(const char *s,
                                   uint32_t *pStart, uint32_t *pEnd,
                                   const char **terminator,
                                   UErrorCode *pErrorCode) {
    char *end;
    uint32_t value;

    if(U_FAILURE(*pErrorCode)) {
        return 0;
    }
    if(s==NULL || pStart==NULL || pEnd==NULL) {
        *pErrorCode=U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    
    s=u_skipWhitespace(s);
    value=(uint32_t)uprv_strtoul(s, &end, 16);
    if(end<=s || value>=0x110000) {
        *pErrorCode=U_PARSE_ERROR;
        return 0;
    }
    *pStart=*pEnd=value;

    
    s=u_skipWhitespace(end);
    if(*s!='.' || s[1]!='.') {
        *terminator=end;
        return 1;
    }
    s=u_skipWhitespace(s+2);

    
    value=(uint32_t)uprv_strtoul(s, &end, 16);
    if(end<=s || value>=0x110000) {
        *pErrorCode=U_PARSE_ERROR;
        return 0;
    }
    *pEnd=value;

    
    if(value<*pStart) {
        *pErrorCode=U_PARSE_ERROR;
        return 0;
    }

    *terminator=end;
    return value-*pStart+1;
}

U_CAPI int32_t U_EXPORT2
u_parseCodePointRange(const char *s,
                      uint32_t *pStart, uint32_t *pEnd,
                      UErrorCode *pErrorCode) {
    const char *terminator;
    int32_t rangeLength=
        u_parseCodePointRangeAnyTerminator(s, pStart, pEnd, &terminator, pErrorCode);
    if(U_SUCCESS(*pErrorCode)) {
        terminator=u_skipWhitespace(terminator);
        if(*terminator!=';' && *terminator!=0) {
            *pErrorCode=U_PARSE_ERROR;
            return 0;
        }
    }
    return rangeLength;
}

U_CAPI int32_t U_EXPORT2
u_parseUTF8(const char *source, int32_t sLen, char *dest, int32_t destCapacity, UErrorCode *status) {
    const char *read = source;
    int32_t i = 0;
    unsigned int value = 0;
    if(sLen == -1) {
        sLen = (int32_t)strlen(source);
    }
    
    while(read < source+sLen) {
        sscanf(read, "%2x", &value);
        if(i < destCapacity) {
            dest[i] = (char)value;
        }
        i++;
        read += 2;
    }
    return u_terminateChars(dest, destCapacity, i, status);
}
