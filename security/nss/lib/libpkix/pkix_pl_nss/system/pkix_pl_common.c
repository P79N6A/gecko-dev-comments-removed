










































#include "pkix_pl_common.h"




















PKIX_Error *
pkix_LockObject(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Object *objectHeader;

        PKIX_ENTER(OBJECT, "pkix_LockObject");
        PKIX_NULLCHECK_ONE(object);

        if (object == (PKIX_PL_Object *)PKIX_ALLOC_ERROR()) {
                goto cleanup;
        }

        PKIX_OBJECT_DEBUG("\tShifting object pointer).\n");
        

        objectHeader = object-1;

        PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
        PR_Lock(objectHeader->lock);

cleanup:

        PKIX_RETURN(OBJECT);
}


















PKIX_Error *
pkix_UnlockObject(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Object *objectHeader;
        PRStatus result;

        PKIX_ENTER(OBJECT, "pkix_UnlockObject");
        PKIX_NULLCHECK_ONE(object);

        if (object == (PKIX_PL_Object *)PKIX_ALLOC_ERROR()) {
                goto cleanup;
        }

        PKIX_OBJECT_DEBUG("\tShifting object pointer).\n");
        

        objectHeader = object-1;

        PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
        result = PR_Unlock(objectHeader->lock);

        if (result == PR_FAILURE) {
                PKIX_OBJECT_DEBUG("\tPR_Unlock failed.).\n");
                PKIX_ERROR_FATAL(PKIX_ERRORUNLOCKINGOBJECT);
        }

cleanup:

        PKIX_RETURN(OBJECT);
}





















PKIX_Boolean
pkix_pl_UInt32_Overflows(char *string){
        char *firstNonZero = NULL;
        PKIX_UInt32 length, i;
        char *MAX_UINT32_STRING = "4294967295";

        PKIX_DEBUG_ENTER(OID);

        PKIX_OID_DEBUG("\tCalling PL_strlen).\n");
        length = PL_strlen(string);

        if (length < MAX_DIGITS_32){
                return (PKIX_FALSE);
        }

        firstNonZero = string;
        for (i = 0; i < length; i++){
                if (*string == '0'){
                        firstNonZero++;
                }
        }

        PKIX_OID_DEBUG("\tCalling PL_strlen).\n");
        length = PL_strlen(firstNonZero);

        if (length > MAX_DIGITS_32){
                return (PKIX_TRUE);
        }

        PKIX_OID_DEBUG("\tCalling PL_strlen).\n");
        if (length == MAX_DIGITS_32){
                PKIX_OID_DEBUG("\tCalling PORT_Strcmp).\n");
                if (PORT_Strcmp(firstNonZero, MAX_UINT32_STRING) > 0){
                        return (PKIX_TRUE);
                }
        }

        return (PKIX_FALSE);
}





































static PKIX_Error *
pkix_pl_getOIDToken(
        char *derBytes,
        PKIX_UInt32 index,
        PKIX_UInt32 *pToken,
        PKIX_UInt32 *pIndex,
        void *plContext)
{
        PKIX_UInt32 retval, i, tmp;

        PKIX_ENTER(OID, "pkix_pl_getOIDToken");
        PKIX_NULLCHECK_THREE(derBytes, pToken, pIndex);

        







        for (i = 0, retval = 0; i < 4; i++) {
            retval <<= 7;
            tmp = derBytes[index];
            index++;
            retval |= (tmp & 0x07f);
            if ((tmp & 0x080) == 0){
                    *pToken = retval;
                    *pIndex = index;
                    goto cleanup;
            }
        }

        PKIX_ERROR(PKIX_INVALIDENCODINGOIDTOKENVALUETOOBIG);

cleanup:

        PKIX_RETURN(OID);

}






























PKIX_Error *
pkix_pl_helperBytes2Ascii(
        PKIX_UInt32 *tokens,
        PKIX_UInt32 numTokens,
        char **pAscii,
        void *plContext)
{
        char *tempString = NULL;
        char *outputString = NULL;
        char *format = "%d";
        PKIX_UInt32 i = 0;
        PKIX_UInt32 outputLen = 0;
        PKIX_Int32 error;

        PKIX_ENTER(OBJECT, "pkix_pl_helperBytes2Ascii");
        PKIX_NULLCHECK_TWO(tokens, pAscii);

        if (numTokens == 0) {
                PKIX_ERROR_FATAL(PKIX_HELPERBYTES2ASCIINUMTOKENSZERO);
        }

        







        PKIX_CHECK(PKIX_PL_Malloc
                    (MAX_DIGITS_32 + 1, (void **)&tempString, plContext),
                    PKIX_MALLOCFAILED);

        for (i = 0; i < numTokens; i++){
                PKIX_OBJECT_DEBUG("\tCalling PR_snprintf).\n");
                error = PR_snprintf(tempString,
                                    MAX_DIGITS_32 + 1,
                                    format,
                                    tokens[i]);
                if (error == -1){
                        PKIX_ERROR(PKIX_PRSNPRINTFFAILED);
                }

                PKIX_OBJECT_DEBUG("\tCalling PL_strlen).\n");
                outputLen += PL_strlen(tempString);

                
                outputLen++;
        }

        
        PKIX_CHECK(PKIX_PL_Malloc
                    (outputLen, (void **)&outputString, plContext),
                    PKIX_MALLOCFAILED);

        *outputString = '\0';

        
        for (i = 0; i < numTokens; i++){

                PKIX_OBJECT_DEBUG("\tCalling PR_snprintf).\n");
                error = PR_snprintf(tempString,
                                    MAX_DIGITS_32 + 1,
                                    format,
                                    tokens[i]);
                if (error == -1){
                        PKIX_ERROR(PKIX_PRSNPRINTFFAILED);
                }

                PKIX_OBJECT_DEBUG("\tCalling PL_strcat).\n");
                (void) PL_strcat(outputString, tempString);

                
                if (i < (numTokens - 1)){
                        PKIX_OBJECT_DEBUG("\tCalling PL_strcat).\n");
                        (void) PL_strcat(outputString, ".");
                }
        }

        
        outputString[outputLen-1] = '\0';

        *pAscii = outputString;
        outputString = NULL;

cleanup:
        
        PKIX_FREE(outputString);
        PKIX_FREE(tempString);

        PKIX_RETURN(OBJECT);

}






























PKIX_Error *
pkix_pl_ipAddrBytes2Ascii(
        SECItem *secItem,
        char **pAscii,
        void *plContext)
{
        char *data = NULL;
        PKIX_UInt32 *tokens = NULL;
        PKIX_UInt32 numTokens = 0;
        PKIX_UInt32 i = 0;
        char *asciiString = NULL;

        PKIX_ENTER(OBJECT, "pkix_pl_ipAddrBytes2Ascii");
        PKIX_NULLCHECK_THREE(secItem, pAscii, secItem->data);

        if (secItem->len == 0) {
                PKIX_ERROR_FATAL(PKIX_IPADDRBYTES2ASCIIDATALENGTHZERO);
        }

        data = (char *)(secItem->data);
        numTokens = secItem->len;

        
        PKIX_CHECK(PKIX_PL_Malloc
                    (numTokens * sizeof (PKIX_UInt32),
                    (void **)&tokens,
                    plContext),
                    PKIX_MALLOCFAILED);

        
        for (i = 0; i < numTokens; i++){
                tokens[i] = data[i];
        }

        
        PKIX_CHECK(pkix_pl_helperBytes2Ascii
                    (tokens, numTokens, &asciiString, plContext),
                    PKIX_HELPERBYTES2ASCIIFAILED);

        *pAscii = asciiString;

cleanup:

        PKIX_FREE(tokens);

        PKIX_RETURN(OBJECT);
}

































PKIX_Error *
pkix_pl_oidBytes2Ascii(
        SECItem *secItem,
        char **pAscii,
        void *plContext)
{
        char *data = NULL;
        PKIX_UInt32 *tokens = NULL;
        PKIX_UInt32 token = 0;
        PKIX_UInt32 numBytes = 0;
        PKIX_UInt32 numTokens = 0;
        PKIX_UInt32 i = 0, x = 0, y = 0;
        PKIX_UInt32 index = 0;
        char *asciiString = NULL;

        PKIX_ENTER(OID, "pkix_pl_oidBytes2Ascii");
        PKIX_NULLCHECK_THREE(secItem, pAscii, secItem->data);

        if (secItem->len == 0) {
                PKIX_ERROR_FATAL(PKIX_OIDBYTES2ASCIIDATALENGTHZERO);
        }

        data = (char *)(secItem->data);
        numBytes = secItem->len;
        numTokens = 0;

        
        for (i = 0; i < numBytes; i++){
                if ((data[i] & 0x080) == 0){
                        numTokens++;
                }
        }

        
        if (numTokens == 0){
                PKIX_ERROR(PKIX_INVALIDDERENCODINGFOROID);
        }

        
        numTokens++;

        
        PKIX_CHECK(PKIX_PL_Malloc
                    (numTokens * sizeof (PKIX_UInt32),
                    (void **)&tokens,
                    plContext),
                    PKIX_MALLOCFAILED);

        
        for (i = 0; i < numTokens; i++){

                
                PKIX_CHECK(pkix_pl_getOIDToken
                            (data, index, &token, &index, plContext),
                            PKIX_GETOIDTOKENFAILED);

                if (i == 0){

                        






                        if (token < 40)
                                x = 0;
                        else if (token < 80)
                                x = 1;
                        else
                                x = 2;
                        y = token - (x * 40);

                        tokens[0] = x;
                        tokens[1] = y;
                        i++;
                } else {
                        tokens[i] = token;
                }
        }

        
        PKIX_CHECK(pkix_pl_helperBytes2Ascii
                    (tokens, numTokens, &asciiString, plContext),
                    PKIX_HELPERBYTES2ASCIIFAILED);

        *pAscii = asciiString;

cleanup:

        PKIX_FREE(tokens);
        PKIX_RETURN(OID);

}

































PKIX_Error *
pkix_UTF16_to_EscASCII(
        const void *utf16String,
        PKIX_UInt32 utf16Length,
        PKIX_Boolean debug,
        char **pDest,
        PKIX_UInt32 *pLength,
        void *plContext)
{
        char *destPtr = NULL;
        PKIX_UInt32 i, charLen;
        PKIX_UInt32 x = 0, y = 0, z = 0;
        unsigned char *utf16Char = (unsigned char *)utf16String;

        PKIX_ENTER(STRING, "pkix_UTF16_to_EscASCII");
        PKIX_NULLCHECK_THREE(utf16String, pDest, pLength);

        
        charLen = 4*utf16Length;

        
        if ((utf16Length % 2) != 0){
                PKIX_ERROR(PKIX_UTF16ALIGNMENTERROR);
        }

        
        for (i = 0; i < utf16Length; i += 2) {
                if ((utf16Char[i] == 0x00)&&
                        pkix_isPlaintext(utf16Char[i+1], debug)) {
                        if (utf16Char[i+1] == '&') {
                                
                                charLen -= 3;
                        } else {
                                
                                charLen -= 7;
                        }
                } else if ((utf16Char[i] >= 0xD8) && (utf16Char[i] <= 0xDB)) {
                        if ((i+3) >= utf16Length) {
                                PKIX_ERROR(PKIX_UTF16HIGHZONEALIGNMENTERROR);
                        } else if ((utf16Char[i+2] >= 0xDC)&&
                                (utf16Char[i+2] <= 0xDF)) {
                                
                                charLen -= 4;
                                
                                i += 2;
                        } else {
                                
                                PKIX_ERROR(PKIX_UTF16LOWZONEERROR);
                        }
                }
        }

        *pLength = charLen;

        
        charLen++;

        
        PKIX_CHECK(PKIX_PL_Malloc(charLen, (void **)pDest, plContext),
                    PKIX_MALLOCFAILED);

        destPtr = *pDest;
        for (i = 0; i < utf16Length; i += 2) {
                if ((utf16Char[i] == 0x00)&&
                    pkix_isPlaintext(utf16Char[i+1], debug)) {
                        
                        *destPtr++ = utf16Char[i+1];
                } else if ((utf16Char[i+1] == '&') && (utf16Char[i] == 0x00)){
                        *destPtr++ = '&';
                        *destPtr++ = 'a';
                        *destPtr++ = 'm';
                        *destPtr++ = 'p';
                        *destPtr++ = ';';
                } else if ((utf16Char[i] >= 0xD8)&&
                            (utf16Char[i] <= 0xDB)&&
                            (utf16Char[i+2] >= 0xDC)&&
                            (utf16Char[i+2] <= 0xDF)) {
                        





                        x = 0x0FFFF & ((utf16Char[i]<<8) | utf16Char[i+1]);
                        y = 0x0FFFF & ((utf16Char[i+2]<<8) | utf16Char[i+3]);
                        z = ((x - 0xD800) * 0x400 + (y - 0xDC00)) + 0x00010000;

                        
                        PKIX_STRING_DEBUG("\tCalling PR_snprintf).\n");
                        if (PR_snprintf(destPtr, 13, "&#x%08X;", z) ==
                            (PKIX_UInt32)(-1)) {
                                PKIX_ERROR(PKIX_PRSNPRINTFFAILED);
                        }
                        i += 2;
                        destPtr += 12;
                } else {
                        
                        PKIX_STRING_DEBUG("\tCalling PR_snprintf).\n");
                        if (PR_snprintf
                            (destPtr,
                            9,
                            "&#x%02X%02X;",
                            utf16Char[i],
                            utf16Char[i+1]) ==
                            (PKIX_UInt32)(-1)) {
                                PKIX_ERROR(PKIX_PRSNPRINTFFAILED);
                        }
                        destPtr += 8;
                }
        }
        *destPtr = '\0';

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_FREE(*pDest);
        }

        PKIX_RETURN(STRING);
}































PKIX_Error *
pkix_EscASCII_to_UTF16(
        const char *escAsciiString,
        PKIX_UInt32 escAsciiLen,
        PKIX_Boolean debug,
        void **pDest,
        PKIX_UInt32 *pLength,
        void *plContext)
{
        PKIX_UInt32 newLen, i, j, charSize;
        PKIX_UInt32 x = 0, y = 0, z = 0;
        unsigned char *destPtr = NULL;
        unsigned char testChar, testChar2;
        unsigned char *stringData = (unsigned char *)escAsciiString;

        PKIX_ENTER(STRING, "pkix_EscASCII_to_UTF16");
        PKIX_NULLCHECK_THREE(escAsciiString, pDest, pLength);

        if (escAsciiLen == 0) {
                PKIX_CHECK(PKIX_PL_Malloc(escAsciiLen, pDest, plContext),
                            PKIX_MALLOCFAILED);
                goto cleanup;
        }

        
        newLen = escAsciiLen*2;

        
        for (i = 0; i < escAsciiLen; i++) {
                if (!pkix_isPlaintext(stringData[i], debug)&&
                    (stringData[i] != '&')) {
                        PKIX_ERROR(PKIX_ILLEGALCHARACTERINESCAPEDASCII);
                } else if (PL_strstr(escAsciiString+i, "&amp;") ==
                            escAsciiString+i) {
                        
                        newLen -= 8;
                        i += 4;
                } else if ((PL_strstr(escAsciiString+i, "&#x") ==
                            escAsciiString+i)||
                            (PL_strstr(escAsciiString+i, "&#X") ==
                            escAsciiString+i)) {
                        if (((i+7) <= escAsciiLen)&&
                            (escAsciiString[i+7] == ';')) {
                                
                                newLen -= 14;
                                i += 7;
                        } else if (((i+11) <= escAsciiLen)&&
                                (escAsciiString[i+11] == ';')) {
                                
                                newLen -= 20;
                                i += 11;
                        } else {
                                PKIX_ERROR(PKIX_ILLEGALUSEOFAMP);
                        }
                }
        }

        PKIX_CHECK(PKIX_PL_Malloc(newLen, pDest, plContext),
                    PKIX_MALLOCFAILED);

        
        destPtr = (unsigned char *)*pDest;

        i = 0;
        while (i < escAsciiLen) {
                
                if (pkix_isPlaintext(escAsciiString[i], debug)) {
                        *destPtr++ = 0x00;
                        *destPtr++ = escAsciiString[i++];
                } else if (PL_strstr(escAsciiString+i, "&amp;") ==
                            escAsciiString+i) {
                        
                        *destPtr++ = 0x00;
                        *destPtr++ = '&';
                        i += 5;
                } else if (((PL_strstr(escAsciiString+i, "&#x") ==
                            escAsciiString+i)||
                            (PL_strstr(escAsciiString+i, "&#X") ==
                            escAsciiString+i))&&
                            ((i+7) <= escAsciiLen)) {

                        
                        charSize = (escAsciiString[i+7] == ';')?4:8;

                        
                        i += 3;

                        
                        if (((i+charSize) > escAsciiLen)||
                            (escAsciiString[i+charSize] != ';')) {
                                PKIX_ERROR(PKIX_TRUNCATEDUNICODEINESCAPEDASCII);
                        }

                        for (j = 0; j < charSize; j++) {
                                if (!PKIX_ISXDIGIT
                                    (escAsciiString[i+j])) {
                                        PKIX_ERROR(PKIX_ILLEGALUNICODECHARACTER);
                                } else if (charSize == 8) {
                                        x |= (pkix_hex2i
                                                        (escAsciiString[i+j]))
                                                        <<(4*(7-j));
                                }
                        }

                        testChar =
                                (pkix_hex2i(escAsciiString[i])<<4)|
                                pkix_hex2i(escAsciiString[i+1]);
                        testChar2 =
                                (pkix_hex2i(escAsciiString[i+2])<<4)|
                                pkix_hex2i(escAsciiString[i+3]);

                        if (charSize == 4) {
                                if ((testChar >= 0xD8)&&
                                    (testChar <= 0xDF)) {
                                        PKIX_ERROR(PKIX_ILLEGALSURROGATEPAIR);
                                } else if ((testChar == 0x00)&&
                                  pkix_isPlaintext(testChar2, debug)) {
                                      PKIX_ERROR(
                                          PKIX_ILLEGALCHARACTERINESCAPEDASCII);
                                }
                                *destPtr++ = testChar;
                                *destPtr++ = testChar2;
                        } else if (charSize == 8) {
                                
                                if (!((testChar == 0x00)&&
                                    ((testChar2 >= 0x01)&&
                                    (testChar2 <= 0x10)))) {
                                      PKIX_ERROR(
                                          PKIX_ILLEGALCHARACTERINESCAPEDASCII);
                                }
                                






                                x -= 0x00010000;
                                y = (x/0x400)+ 0xD800;
                                z = (x%0x400)+ 0xDC00;

                                
                                *destPtr++ = (y&0xFF00)>>8;
                                *destPtr++ = (y&0x00FF);
                                *destPtr++ = (z&0xFF00)>>8;
                                *destPtr++ = (z&0x00FF);
                        }
                        
                        i += charSize+1;
                } else {
                        
                        PKIX_ERROR(PKIX_ILLEGALCHARACTERINESCAPEDASCII);
                }
        }

        *pLength = newLen;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_FREE(*pDest);
        }

        PKIX_RETURN(STRING);
}































PKIX_Error *
pkix_UTF16_to_UTF8(
        const void *utf16String,
        PKIX_UInt32 utf16Length,
        PKIX_Boolean null_term,
        void **pDest,
        PKIX_UInt32 *pLength,
        void *plContext)
{
        PKIX_Boolean result;
        PKIX_UInt32 reallocLen;
        char *endPtr = NULL;

        PKIX_ENTER(STRING, "pkix_UTF16_to_UTF8");
        PKIX_NULLCHECK_THREE(utf16String, pDest, pLength);

        
        PKIX_CHECK(PKIX_PL_Calloc(1, utf16Length*2, pDest, plContext),
                    PKIX_CALLOCFAILED);

        PKIX_STRING_DEBUG("\tCalling PORT_UCS2_UTF8Conversion).\n");
        result = PORT_UCS2_UTF8Conversion
                (PKIX_FALSE, 
                (unsigned char *)utf16String,
                utf16Length,
                (unsigned char *)*pDest,
                utf16Length*2, 
                pLength);
        if (result == PR_FALSE){
                PKIX_ERROR(PKIX_PORTUCS2UTF8CONVERSIONFAILED);
        }

        reallocLen = *pLength;

        if (null_term){
                reallocLen++;
        }

        PKIX_CHECK(PKIX_PL_Realloc(*pDest, reallocLen, pDest, plContext),
                    PKIX_REALLOCFAILED);

        if (null_term){
                endPtr = (char*)*pDest + reallocLen - 1;
                *endPtr = '\0';
        }

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_FREE(*pDest);
        }

        PKIX_RETURN(STRING);
}




























PKIX_Error *
pkix_UTF8_to_UTF16(
        const void *utf8String,
        PKIX_UInt32 utf8Length,
        void **pDest,
        PKIX_UInt32 *pLength,
        void *plContext)
{
        PKIX_Boolean result;

        PKIX_ENTER(STRING, "pkix_UTF8_to_UTF16");
        PKIX_NULLCHECK_THREE(utf8String, pDest, pLength);

        
        PKIX_CHECK(PKIX_PL_Calloc(1, utf8Length*2, pDest, plContext),
                    PKIX_MALLOCFAILED);

        PKIX_STRING_DEBUG("\tCalling PORT_UCS2_UTF8Conversion).\n");
        result = PORT_UCS2_UTF8Conversion
                (PKIX_TRUE, 
                (unsigned char *)utf8String,
                utf8Length,
                (unsigned char *)*pDest,
                utf8Length*2, 
                pLength);
        if (result == PR_FALSE){
                PKIX_ERROR(PKIX_PORTUCS2UTF8CONVERSIONFAILED);
        }

        PKIX_CHECK(PKIX_PL_Realloc(*pDest, *pLength, pDest, plContext),
                    PKIX_REALLOCFAILED);

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_FREE(*pDest);
        }

        PKIX_RETURN(STRING);
}
