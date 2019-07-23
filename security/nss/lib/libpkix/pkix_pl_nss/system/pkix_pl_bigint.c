










































#include "pkix_pl_bigint.h"







static PKIX_Error *
pkix_pl_BigInt_Comparator(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext)
{
        PKIX_PL_BigInt *firstBigInt = NULL;
        PKIX_PL_BigInt *secondBigInt = NULL;
        char *firstPtr = NULL;
        char *secondPtr = NULL;
        PKIX_UInt32 firstLen, secondLen;

        PKIX_ENTER(BIGINT, "pkix_pl_BigInt_Comparator");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        PKIX_CHECK(pkix_CheckTypes
                    (firstObject, secondObject, PKIX_BIGINT_TYPE, plContext),
                    PKIX_ARGUMENTSNOTBIGINTS);

        
        firstBigInt = (PKIX_PL_BigInt*)firstObject;
        secondBigInt = (PKIX_PL_BigInt*)secondObject;

        *pResult = 0;
        firstPtr = firstBigInt->dataRep;
        secondPtr = secondBigInt->dataRep;
        firstLen = firstBigInt->length;
        secondLen = secondBigInt->length;

        if (firstLen < secondLen) {
                *pResult = -1;
        } else if (firstLen > secondLen) {
                *pResult = 1;
        } else if (firstLen == secondLen) {
                PKIX_BIGINT_DEBUG("\t\tCalling PORT_Memcmp).\n");
                *pResult = PORT_Memcmp(firstPtr, secondPtr, firstLen);
        }

cleanup:

        PKIX_RETURN(BIGINT);
}





static PKIX_Error *
pkix_pl_BigInt_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_BigInt *bigInt = NULL;

        PKIX_ENTER(BIGINT, "pkix_pl_BigInt_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_BIGINT_TYPE, plContext),
                    PKIX_OBJECTNOTBIGINT);

        bigInt = (PKIX_PL_BigInt*)object;

        PKIX_FREE(bigInt->dataRep);
        bigInt->dataRep = NULL;
        bigInt->length = 0;

cleanup:

        PKIX_RETURN(BIGINT);
}






static PKIX_Error *
pkix_pl_BigInt_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_BigInt *bigInt = NULL;
        char *outputText = NULL;
        PKIX_UInt32 i, j, lengthChars;

        PKIX_ENTER(BIGINT, "pkix_pl_BigInt_ToString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_BIGINT_TYPE, plContext),
                    PKIX_OBJECTNOTBIGINT);

        bigInt = (PKIX_PL_BigInt*)object;

        
        lengthChars = (bigInt->length * 2) + 1;

        PKIX_CHECK(PKIX_PL_Malloc
                    (lengthChars, (void **)&outputText, plContext),
                    PKIX_MALLOCFAILED);

        for (i = 0, j = 0; i < bigInt->length; i += 1, j += 2){
                outputText[j] = pkix_i2hex
                        ((char) ((*(bigInt->dataRep+i) & 0xf0) >> 4));
                outputText[j+1] = pkix_i2hex
                        ((char) (*(bigInt->dataRep+i) & 0x0f));
        }

        outputText[lengthChars-1] = '\0';

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    outputText,
                    0,
                    pString,
                    plContext),
                    PKIX_STRINGCREATEFAILED);

cleanup:

        PKIX_FREE(outputText);

        PKIX_RETURN(BIGINT);
}






static PKIX_Error *
pkix_pl_BigInt_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_BigInt *bigInt = NULL;

        PKIX_ENTER(BIGINT, "pkix_pl_BigInt_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_BIGINT_TYPE, plContext),
                    PKIX_OBJECTNOTBIGINT);

        bigInt = (PKIX_PL_BigInt*)object;

        PKIX_CHECK(pkix_hash
                    ((void *)bigInt->dataRep,
                    bigInt->length,
                    pHashcode,
                    plContext),
                    PKIX_HASHFAILED);

cleanup:

        PKIX_RETURN(BIGINT);
}





static PKIX_Error *
pkix_pl_BigInt_Equals(
        PKIX_PL_Object *first,
        PKIX_PL_Object *second,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 secondType;
        PKIX_Int32 cmpResult = 0;

        PKIX_ENTER(BIGINT, "pkix_pl_BigInt_Equals");
        PKIX_NULLCHECK_THREE(first, second, pResult);

        PKIX_CHECK(pkix_CheckType(first, PKIX_BIGINT_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTBIGINT);

        PKIX_CHECK(PKIX_PL_Object_GetType(second, &secondType, plContext),
                PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);

        *pResult = PKIX_FALSE;

        if (secondType != PKIX_BIGINT_TYPE) goto cleanup;

        PKIX_CHECK(pkix_pl_BigInt_Comparator
                (first, second, &cmpResult, plContext),
                PKIX_BIGINTCOMPARATORFAILED);

        *pResult = (cmpResult == 0);

cleanup:

        PKIX_RETURN(BIGINT);
}












PKIX_Error *
pkix_pl_BigInt_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(BIGINT, "pkix_pl_BigInt_RegisterSelf");

        entry.description = "BigInt";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_BigInt);
        entry.destructor = pkix_pl_BigInt_Destroy;
        entry.equalsFunction = pkix_pl_BigInt_Equals;
        entry.hashcodeFunction = pkix_pl_BigInt_Hashcode;
        entry.toStringFunction = pkix_pl_BigInt_ToString;
        entry.comparator = pkix_pl_BigInt_Comparator;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_BIGINT_TYPE] = entry;

        PKIX_RETURN(BIGINT);
}


























PKIX_Error *
pkix_pl_BigInt_CreateWithBytes(
        char *bytes,
        PKIX_UInt32 length,
        PKIX_PL_BigInt **pBigInt,
        void *plContext)
{
        PKIX_PL_BigInt *bigInt = NULL;

        PKIX_ENTER(BIGINT, "pkix_pl_BigInt_CreateWithBytes");
        PKIX_NULLCHECK_TWO(pBigInt, bytes);

        if (length == 0) {
                PKIX_ERROR(PKIX_BIGINTLENGTH0INVALID)
        }

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_BIGINT_TYPE,
                sizeof (PKIX_PL_BigInt),
                (PKIX_PL_Object **)&bigInt,
                plContext),
                PKIX_COULDNOTCREATEOBJECT);

        PKIX_CHECK(PKIX_PL_Malloc
                    (length, (void **)&(bigInt->dataRep), plContext),
                    PKIX_MALLOCFAILED);

        PKIX_BIGINT_DEBUG("\t\tCalling PORT_Memcpy).\n");
        (void) PORT_Memcpy(bigInt->dataRep, bytes, length);

        bigInt->length = length;

        *pBigInt = bigInt;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(bigInt);
        }

        PKIX_RETURN(BIGINT);
}






PKIX_Error *
PKIX_PL_BigInt_Create(
        PKIX_PL_String *stringRep,
        PKIX_PL_BigInt **pBigInt,
        void *plContext)
{
        PKIX_PL_BigInt *bigInt = NULL;
        char *asciiString = NULL;
        PKIX_UInt32 lengthBytes;
        PKIX_UInt32 lengthString;
        PKIX_UInt32 i;
        char currChar;

        PKIX_ENTER(BIGINT, "PKIX_PL_BigInt_Create");
        PKIX_NULLCHECK_TWO(pBigInt, stringRep);

        PKIX_CHECK(PKIX_PL_String_GetEncoded
                (stringRep,
                PKIX_ESCASCII,
                (void **)&asciiString,
                &lengthString,
                plContext),
                PKIX_STRINGGETENCODEDFAILED);

        if ((lengthString == 0) || ((lengthString % 2) != 0)){
                PKIX_ERROR(PKIX_SOURCESTRINGHASINVALIDLENGTH);
        }

        if (lengthString != 2){
                if ((asciiString[0] == '0') && (asciiString[1] == '0')){
                        PKIX_ERROR(PKIX_FIRSTDOUBLEHEXMUSTNOTBE00);
                }
        }

        for (i = 0; i < lengthString; i++) {
                currChar = asciiString[i];
                if (!PKIX_ISXDIGIT(currChar)){
                        PKIX_ERROR(PKIX_INVALIDCHARACTERINBIGINT);
                }
        }

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_BIGINT_TYPE,
                sizeof (PKIX_PL_BigInt),
                (PKIX_PL_Object **)&bigInt,
                plContext),
                PKIX_COULDNOTCREATEOBJECT);

        
        lengthBytes = lengthString/2;

        PKIX_CHECK(PKIX_PL_Malloc
                    (lengthBytes, (void **)&(bigInt->dataRep), plContext),
                    PKIX_MALLOCFAILED);

        for (i = 0; i < lengthString; i += 2){
                (bigInt->dataRep)[i/2] =
                        (pkix_hex2i(asciiString[i])<<4) |
                        pkix_hex2i(asciiString[i+1]);
        }

        bigInt->length = lengthBytes;

        *pBigInt = bigInt;

cleanup:

        PKIX_FREE(asciiString);

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(bigInt);
        }

        PKIX_RETURN(BIGINT);
}
