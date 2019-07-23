










































#include "pkix_pl_oid.h"







static PKIX_Error *
pkix_pl_OID_Comparator(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext)
{
        PKIX_PL_OID *firstOID = NULL;
        PKIX_PL_OID *secondOID = NULL;
        PKIX_UInt32 minLength;

        PKIX_ENTER(OID, "pkix_pl_OID_Comparator");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        PKIX_CHECK(pkix_CheckTypes
                    (firstObject, secondObject, PKIX_OID_TYPE, plContext),
                    PKIX_ARGUMENTSNOTOIDS);

        firstOID = (PKIX_PL_OID*)firstObject;
        secondOID = (PKIX_PL_OID*)secondObject;

        *pResult = 0;

        minLength = (firstOID->length < secondOID->length)?
                firstOID->length:
                secondOID->length;

        
        PKIX_OID_DEBUG("\tCalling PORT_Memcmp).\n");
        *pResult = PORT_Memcmp
                (firstOID->components,
                secondOID->components,
                minLength * sizeof (PKIX_UInt32));

cleanup:
        PKIX_RETURN(OID);
}





static PKIX_Error *
pkix_pl_OID_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_OID *oid = NULL;

        PKIX_ENTER(OID, "pkix_pl_OID_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OID_TYPE, plContext),
                    PKIX_OBJECTNOTANOID);

        oid = (PKIX_PL_OID*)object;

        PKIX_FREE(oid->components);
        oid->length = 0;

cleanup:

        PKIX_RETURN(OID);
}





static PKIX_Error *
pkix_pl_OID_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_OID *pkixOID = NULL;

        PKIX_ENTER(OID, "pkix_pl_OID_HashCode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OID_TYPE, plContext),
                    PKIX_OBJECTNOTANOID);

        pkixOID = (PKIX_PL_OID *)object;

        PKIX_CHECK(pkix_hash
                    ((unsigned char *)pkixOID->components,
                    pkixOID->length * sizeof (PKIX_UInt32),
                    pHashcode,
                    plContext),
                    PKIX_HASHFAILED);
cleanup:

        PKIX_RETURN(OID);
}





static PKIX_Error *
pkix_pl_OID_Equals(
        PKIX_PL_Object *first,
        PKIX_PL_Object *second,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 secondType;
        PKIX_Int32 cmpResult;

        PKIX_ENTER(OID, "pkix_pl_OID_Equals");
        PKIX_NULLCHECK_THREE(first, second, pResult);

        PKIX_CHECK(pkix_CheckType(first, PKIX_OID_TYPE, plContext),
                    PKIX_FIRSTARGUMENTNOTANOID);

        PKIX_CHECK(PKIX_PL_Object_GetType(second, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);

        *pResult = PKIX_FALSE;

        



        if ((secondType != PKIX_OID_TYPE)||
            (((PKIX_PL_OID*)first)->length !=
            ((PKIX_PL_OID*)second)->length)) {
                goto cleanup;
        }

        PKIX_CHECK(pkix_pl_OID_Comparator
                    (first, second, &cmpResult, plContext),
                    PKIX_OIDCOMPARATORFAILED);

        *pResult = (cmpResult == 0);

cleanup:

        PKIX_RETURN(OID);
}





static PKIX_Error *
pkix_pl_OID_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_UInt32 *components = NULL;
        PKIX_UInt32 length;
        char *ascii = NULL;

        PKIX_ENTER(OID, "pkix_pl_OID_toString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_OID_TYPE, plContext),
                    PKIX_OBJECTNOTANOID);

        components = ((PKIX_PL_OID*)object)->components;
        length = ((PKIX_PL_OID*)object)->length;

        PKIX_CHECK(pkix_pl_helperBytes2Ascii
                    (components, length, &ascii, plContext),
                    PKIX_HELPERBYTES2ASCIIFAILED);

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, ascii, 0, pString, plContext),
                PKIX_STRINGCREATEFAILED);

cleanup:

        PKIX_FREE(ascii);

        PKIX_RETURN(OID);
}












PKIX_Error *
pkix_pl_OID_RegisterSelf(
        void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(OID, "pkix_pl_OID_RegisterSelf");

        entry.description = "OID";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_OID);
        entry.destructor = pkix_pl_OID_Destroy;
        entry.equalsFunction = pkix_pl_OID_Equals;
        entry.hashcodeFunction = pkix_pl_OID_Hashcode;
        entry.toStringFunction = pkix_pl_OID_ToString;
        entry.comparator = pkix_pl_OID_Comparator;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_OID_TYPE] = entry;

        PKIX_RETURN(OID);
}































static PKIX_Error *
pkix_pl_OID_GetNextToken(
        char *input,
        char **pToken,
        char **pRem,
        void *plContext)
{
        char *token = input;

        PKIX_ENTER(OID, "pkix_pl_OID_GetNextToken");
        PKIX_NULLCHECK_TWO(pToken, pRem);

        if (token == NULL){
                *pToken = token;
                goto cleanup;
        }

        while (*input != '.' && *input != '\0'){
                input++;
        }

        if (*input == '.'){
                *input = 0;
                *pRem = input + 1;
        } else { 
                *pRem = NULL;
        }

        *pToken = token;

cleanup:

        PKIX_RETURN(OID);
}





















PKIX_Error *
pkix_pl_OID_GetCriticalExtensionOIDs(
        CERTCertExtension **extensions,
        PKIX_List **pOidsList,
        void *plContext)
{
        PKIX_List *oidsList = NULL;
        CERTCertExtension *extension = NULL;
        PKIX_PL_OID *pkixOID = NULL;
        SECItem critical;
        SECItem oid;
        char *oidAscii = NULL;

        PKIX_ENTER(OID, "pkix_pl_OID_GetCriticalExtensionOIDs");
        PKIX_NULLCHECK_ONE(pOidsList);

        PKIX_CHECK(PKIX_List_Create(&oidsList, plContext),
                    PKIX_LISTCREATEFAILED);

        if (extensions){

                while (*extensions){
                        extension = *extensions++;

                    PKIX_NULLCHECK_ONE(extension);

                    
                    critical = extension->critical;

                    if (critical.len != 0){

                            if (critical.data[0] == 0xff) {
                            oid = extension->id;

                            PKIX_CHECK(pkix_pl_oidBytes2Ascii
                                    (&oid, &oidAscii, plContext),
                                    PKIX_OIDBYTES2ASCIIFAILED);

                            PKIX_CHECK(PKIX_PL_OID_Create
                                    (oidAscii, &pkixOID, plContext),
                                    PKIX_OIDCREATEFAILED);

                            PKIX_CHECK(PKIX_List_AppendItem
                                    (oidsList,
                                    (PKIX_PL_Object *)pkixOID,
                                    plContext),
                                    PKIX_LISTAPPENDITEMFAILED);
                            }
                    }

                    PKIX_FREE(oidAscii);
                    PKIX_DECREF(pkixOID);
                }
        }

        *pOidsList = oidsList;
        oidsList = NULL;
        
cleanup:
        PKIX_DECREF(oidsList);
        PKIX_FREE(oidAscii);
        PKIX_DECREF(pkixOID);
        PKIX_RETURN(OID);
}






PKIX_Error *
PKIX_PL_OID_Create(
        char *stringRep,
        PKIX_PL_OID **pOID,
        void *plContext)
{
        PKIX_PL_OID *oid = NULL;
        char *strCpy1 = NULL;
        char *strCpy2 = NULL;
        char *token = NULL;
        PKIX_UInt32 numTokens, i, length;
        PKIX_UInt32 value;
        PKIX_Boolean firstFieldTwo;
        PKIX_UInt32 *components = NULL;
        char *rem = NULL;

        PKIX_ENTER(OID, "PKIX_PL_OID_Create");
        PKIX_NULLCHECK_TWO(pOID, stringRep);

        PKIX_OID_DEBUG("\tCalling PL_strlen).\n");
        length = PL_strlen(stringRep);

        if (length < 3) {
                PKIX_ERROR(PKIX_OIDLENGTHTOOSHORT);
        }

        for (i = 0; i < length; i++) {
                if ((!PKIX_ISDIGIT(stringRep[i]))&&(stringRep[i] != '.')) {
                        PKIX_ERROR(PKIX_ILLEGALCHARACTERINOID);
                }
        }

        
        if ((stringRep[0] == '.') ||
            (stringRep[length-1] == '.')||
            (PL_strstr(stringRep, "..") != NULL)) {
                PKIX_ERROR(PKIX_ILLEGALDOTINOID);
        }

        PKIX_OID_DEBUG("\tCalling PL_strdup).\n");

        strCpy1 = PL_strdup(stringRep);
        strCpy2 = PL_strdup(stringRep);

        

        PKIX_CHECK(pkix_pl_OID_GetNextToken
                    (strCpy1, &token, &rem, plContext),
                    PKIX_OIDGETNEXTTOKENFAILED);

        for (numTokens = 0; token != NULL; numTokens++){
                if (numTokens == 0) {
                        
                        PKIX_OID_DEBUG("\tCalling PORT_Atoi).\n");
                        value = PORT_Atoi(token);
                        if (value > 2) {
                                PKIX_ERROR(PKIX_FIRSTFIELDMUSTBEBETWEEN02);
                        }

                        
                        firstFieldTwo = (value == 2);
                } else if (numTokens == 1) {
                        PKIX_OID_DEBUG("\tCalling PORT_Atoi).\n");
                        value = PORT_Atoi(token);
                        if ((!firstFieldTwo)&&(value > 39)) {
                                PKIX_ERROR
                                        (PKIX_SECONDFIELDMUSTBEBETWEEN039);
                        }
                }

                
                if (pkix_pl_UInt32_Overflows(token)){
                        PKIX_ERROR(PKIX_OIDCOMPONENTTOOBIG);
                }

                PKIX_CHECK(pkix_pl_OID_GetNextToken
                            (rem, &token, &rem, plContext),
                            PKIX_OIDGETNEXTTOKENFAILED);
        }

        if (numTokens < 2) {
                PKIX_ERROR(PKIX_OIDNEEDS2ORMOREFIELDS);
        }

        PKIX_CHECK(PKIX_PL_Malloc
                    (numTokens * sizeof (PKIX_UInt32),
                    (void **)&components, plContext),
                    PKIX_MALLOCFAILED);

        PKIX_CHECK(pkix_pl_OID_GetNextToken
                    (strCpy2, &token, &rem, plContext),
                    PKIX_OIDGETNEXTTOKENFAILED);

        for (i = 0; token != NULL; i++){
                PKIX_OID_DEBUG("\tCalling PORT_Atoi).\n");
                components[i] = PORT_Atoi(token);

                PKIX_CHECK(pkix_pl_OID_GetNextToken
                            (rem, &token, &rem, plContext),
                            PKIX_OIDGETNEXTTOKENFAILED);
        }

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_OID_TYPE,
                    sizeof (PKIX_PL_OID),
                    (PKIX_PL_Object **)&oid,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        oid->length = numTokens;
        oid->components = components;

        *pOID = oid;

cleanup:

        if (strCpy1){
                PKIX_OID_DEBUG("\tCalling PL_strfree).\n");
                PL_strfree(strCpy1);
        }

        if (strCpy2){
                PKIX_OID_DEBUG("\tCalling PL_strfree).\n");
                PL_strfree(strCpy2);
        }

        if (PKIX_ERROR_RECEIVED){
                PKIX_FREE(components);
        }

        PKIX_RETURN(OID);
}
