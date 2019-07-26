













#include "unicode/utypes.h"
#include "unicode/uobject.h"
#include "unicode/ustream.h"
#include "unicode/ucnv.h"
#include "unicode/uchar.h"
#include "unicode/utf16.h"
#include "ustr_cnv.h"
#include "cmemory.h"
#include <string.h>



#if U_IOSTREAM_SOURCE >= 199711

#define STD_NAMESPACE std::

#define STD_OSTREAM STD_NAMESPACE ostream
#define STD_ISTREAM STD_NAMESPACE istream

U_NAMESPACE_BEGIN

U_IO_API STD_OSTREAM & U_EXPORT2
operator<<(STD_OSTREAM& stream, const UnicodeString& str)
{
    if(str.length() > 0) {
        char buffer[200];
        UConverter *converter;
        UErrorCode errorCode = U_ZERO_ERROR;

        
        converter = u_getDefaultConverter(&errorCode);
        if(U_SUCCESS(errorCode)) {
            const UChar *us = str.getBuffer();
            const UChar *uLimit = us + str.length();
            char *s, *sLimit = buffer + (sizeof(buffer) - 1);
            do {
                errorCode = U_ZERO_ERROR;
                s = buffer;
                ucnv_fromUnicode(converter, &s, sLimit, &us, uLimit, 0, FALSE, &errorCode);
                *s = 0;

                
                if(s > buffer) {
                    stream << buffer;
                }
            } while(errorCode == U_BUFFER_OVERFLOW_ERROR);
            u_releaseDefaultConverter(converter);
        }
    }


    return stream;
}

U_IO_API STD_ISTREAM & U_EXPORT2
operator>>(STD_ISTREAM& stream, UnicodeString& str)
{
    
    if (stream.fail()) {
        return stream;
    }

    
    UChar uBuffer[16];
    char buffer[16];
    int32_t idx = 0;
    UConverter *converter;
    UErrorCode errorCode = U_ZERO_ERROR;

    
    converter = u_getDefaultConverter(&errorCode);
    if(U_SUCCESS(errorCode)) {
        UChar *us = uBuffer;
        const UChar *uLimit = uBuffer + sizeof(uBuffer)/sizeof(*uBuffer);
        const char *s, *sLimit;
        char ch;
        UChar ch32;
        UBool initialWhitespace = TRUE;
        UBool continueReading = TRUE;

        
        while (continueReading) {
            ch = stream.get();
            if (stream.eof()) {
                
                if (!initialWhitespace) {
                    stream.clear(stream.eofbit);
                }
                continueReading = FALSE;
            }
            sLimit = &ch + (int)continueReading;
            us = uBuffer;
            s = &ch;
            errorCode = U_ZERO_ERROR;
            




            ucnv_toUnicode(converter, &us, uLimit, &s, sLimit, 0, !continueReading, &errorCode);
            if(U_FAILURE(errorCode)) {
                
                stream.clear(stream.failbit);
                goto STOP_READING;
            }
            
            if (us != uBuffer) {
                
                int32_t uBuffSize = us-uBuffer;
                int32_t uBuffIdx = 0;
                while (uBuffIdx < uBuffSize) {
                    U16_NEXT(uBuffer, uBuffIdx, uBuffSize, ch32);
                    if (u_isWhitespace(ch32)) {
                        if (!initialWhitespace) {
                            buffer[idx++] = ch;
                            while (idx > 0) {
                                stream.putback(buffer[--idx]);
                            }
                            goto STOP_READING;
                        }
                        
                    }
                    else {
                        if (initialWhitespace) {
                            





                            str.truncate(0);
                            initialWhitespace = FALSE;
                        }
                        str.append(ch32);
                    }
                }
                idx = 0;
            }
            else {
                buffer[idx++] = ch;
            }
        }
STOP_READING:
        u_releaseDefaultConverter(converter);
    }


    return stream;
}

U_NAMESPACE_END

#endif
