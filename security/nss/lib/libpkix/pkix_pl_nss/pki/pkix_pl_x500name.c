










































#include "pkix_pl_x500name.h"
























static PKIX_Error *
pkix_pl_X500Name_ToString_Helper(
        PKIX_PL_X500Name *name,
        PKIX_PL_String **pString,
        void *plContext)
{
        CERTName *nssDN = NULL;
        char *utf8String = NULL;
        PKIX_UInt32 utf8Length;

        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_ToString_Helper");
        PKIX_NULLCHECK_TWO(name, pString);
        nssDN = &name->nssDN;

        
        utf8String = CERT_NameToAsciiInvertible(nssDN, CERT_N2A_INVERTIBLE);
        if (!utf8String){
                PKIX_ERROR(PKIX_CERTNAMETOASCIIFAILED);
        }

        PKIX_X500NAME_DEBUG("\t\tCalling PL_strlen).\n");
        utf8Length = PL_strlen(utf8String);

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_UTF8, utf8String, utf8Length, pString, plContext),
                    PKIX_STRINGCREATEFAILED);

cleanup:

        PR_Free(utf8String);

        PKIX_RETURN(X500NAME);
}





static PKIX_Error *
pkix_pl_X500Name_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_X500Name *name = NULL;

        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_X500NAME_TYPE, plContext),
                    PKIX_OBJECTNOTANX500NAME);

        name = (PKIX_PL_X500Name *)object;

        

        if (name->arena) {
            PORT_FreeArena(name->arena, PR_FALSE);
            name->arena = NULL;
        }

cleanup:

        PKIX_RETURN(X500NAME);
}





static PKIX_Error *
pkix_pl_X500Name_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_X500Name *name = NULL;
        char *string = NULL;
        PKIX_UInt32 strLength = 0;

        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_toString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_X500NAME_TYPE, plContext),
                    PKIX_OBJECTNOTANX500NAME);

        name = (PKIX_PL_X500Name *)object;
        string = CERT_NameToAscii(&name->nssDN);
        if (!string){
                PKIX_ERROR(PKIX_CERTNAMETOASCIIFAILED);
        }
        strLength = PL_strlen(string);

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII, string, strLength, pString, plContext),
                    PKIX_STRINGCREATEFAILED);

cleanup:

        PKIX_RETURN(X500NAME);
}





static PKIX_Error *
pkix_pl_X500Name_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_X500Name *name = NULL;
        SECItem *derBytes = NULL;
        PKIX_UInt32 nameHash;

        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_X500NAME_TYPE, plContext),
                    PKIX_OBJECTNOTANX500NAME);

        name = (PKIX_PL_X500Name *)object;

        

        derBytes = &name->derName;

        PKIX_CHECK(pkix_hash
                    (derBytes->data, derBytes->len, &nameHash, plContext),
                    PKIX_HASHFAILED);

        *pHashcode = nameHash;

cleanup:

        PKIX_RETURN(X500NAME);
}






static PKIX_Error *
pkix_pl_X500Name_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 secondType;

        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType(firstObject, PKIX_X500NAME_TYPE, plContext),
                    PKIX_FIRSTOBJECTARGUMENTNOTANX500NAME);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_X500NAME_TYPE) goto cleanup;

        PKIX_CHECK(
            PKIX_PL_X500Name_Match((PKIX_PL_X500Name *)firstObject,
                                   (PKIX_PL_X500Name *)secondObject,
                                   pResult, plContext),
            PKIX_X500NAMEMATCHFAILED);

cleanup:

        PKIX_RETURN(X500NAME);
}












PKIX_Error *
pkix_pl_X500Name_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_RegisterSelf");

        entry.description = "X500Name";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_X500Name);
        entry.destructor = pkix_pl_X500Name_Destroy;
        entry.equalsFunction = pkix_pl_X500Name_Equals;
        entry.hashcodeFunction = pkix_pl_X500Name_Hashcode;
        entry.toStringFunction = pkix_pl_X500Name_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_X500NAME_TYPE] = entry;

        PKIX_RETURN(X500NAME);
}

#ifdef BUILD_LIBPKIX_TESTS



























PKIX_Error *
pkix_pl_X500Name_CreateFromUtf8(
        char *stringRep,
        PKIX_PL_X500Name **pName,
        void *plContext)
{
        PKIX_PL_X500Name *x500Name = NULL;
        PRArenaPool *arena = NULL;
        CERTName *nssDN = NULL;
        SECItem *resultSecItem = NULL;
        
        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_CreateFromUtf8");
        PKIX_NULLCHECK_TWO(pName, stringRep);

        nssDN = CERT_AsciiToName(stringRep);
        if (nssDN == NULL) {
            PKIX_ERROR(PKIX_COULDNOTCREATENSSDN);
        }

        arena = nssDN->arena;

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_X500NAME_TYPE,
                    sizeof (PKIX_PL_X500Name),
                    (PKIX_PL_Object **)&x500Name,
                    plContext),
                    PKIX_COULDNOTCREATEX500NAMEOBJECT);

        
        x500Name->arena = arena;
        x500Name->nssDN.arena = arena;
        x500Name->nssDN.rdns = nssDN->rdns;
        
        resultSecItem =
            SEC_ASN1EncodeItem(arena, &x500Name->derName, nssDN,
                               CERT_NameTemplate);
        
        if (resultSecItem == NULL){
            PKIX_ERROR(PKIX_SECASN1ENCODEITEMFAILED);
        }

        *pName = x500Name;

cleanup:

        if (PKIX_ERROR_RECEIVED){
            if (x500Name) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object*)x500Name,
                                      plContext);
            } else if (nssDN) {
                CERT_DestroyName(nssDN);
            }
        }

        PKIX_RETURN(X500NAME);
}
#endif 


























PKIX_Error *
pkix_pl_X500Name_GetCERTName(
        PKIX_PL_X500Name *xname,
        CERTName **pCERTName,
        void *plContext)
{
        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_GetCERTName");
        PKIX_NULLCHECK_TWO(xname, pCERTName);

        *pCERTName = &xname->nssDN;

        PKIX_RETURN(X500NAME);
}







PKIX_Error *
PKIX_PL_X500Name_CreateFromCERTName(
        SECItem *derName,
        CERTName *name, 
        PKIX_PL_X500Name **pName,
        void *plContext)
{
        PRArenaPool *arena = NULL;
        SECStatus rv = SECFailure;
        PKIX_PL_X500Name *x500Name = NULL;

        PKIX_ENTER(X500NAME, "PKIX_PL_X500Name_CreateFromCERTName");
        PKIX_NULLCHECK_ONE(pName);
        if (derName == NULL && name == NULL) {
            PKIX_ERROR(PKIX_NULLARGUMENT);
        }

        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (arena == NULL) {
            PKIX_ERROR(PKIX_OUTOFMEMORY);
        }
        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_X500NAME_TYPE,
                    sizeof (PKIX_PL_X500Name),
                    (PKIX_PL_Object **)&x500Name,
                    plContext),
                    PKIX_COULDNOTCREATEX500NAMEOBJECT);

        x500Name->arena = arena;
        x500Name->nssDN.arena = NULL;

        if (derName != NULL) {
            rv = SECITEM_CopyItem(arena, &x500Name->derName, derName);
            if (rv == SECFailure) {
                PKIX_ERROR(PKIX_OUTOFMEMORY);
            }
        }            

        if (name != NULL) {
            rv = CERT_CopyName(arena, &x500Name->nssDN, name);
            if (rv == SECFailure) {
                PKIX_ERROR(PKIX_CERTCOPYNAMEFAILED);
            }
        } else {
            rv = SEC_QuickDERDecodeItem(arena, &x500Name->nssDN,
                                        CERT_NameTemplate,
                                        &x500Name->derName);
            if (rv == SECFailure) {
                PKIX_ERROR(PKIX_SECQUICKDERDECODERFAILED);
            }
        }

        *pName = x500Name;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
            if (x500Name) {
                PKIX_PL_Object_DecRef((PKIX_PL_Object*)x500Name,
                                      plContext);
            } else if (arena) {                
                PORT_FreeArena(arena, PR_FALSE);
            }
        }

        PKIX_RETURN(X500NAME);
}

#ifdef BUILD_LIBPKIX_TESTS






PKIX_Error *
PKIX_PL_X500Name_Create(
        PKIX_PL_String *stringRep,
        PKIX_PL_X500Name **pName,
        void *plContext)
{
        char *utf8String = NULL;
        PKIX_UInt32 utf8Length = 0;

        PKIX_ENTER(X500NAME, "PKIX_PL_X500Name_Create");
        PKIX_NULLCHECK_TWO(pName, stringRep);

        








        PKIX_CHECK(PKIX_PL_String_GetEncoded
                    (stringRep,
                    PKIX_UTF8_NULL_TERM,
                    (void **)&utf8String,
                    &utf8Length,
                    plContext),
                    PKIX_STRINGGETENCODEDFAILED);

        PKIX_CHECK(
            pkix_pl_X500Name_CreateFromUtf8(utf8String,
                                            pName, plContext),
            PKIX_X500NAMECREATEFROMUTF8FAILED);

cleanup:
        PKIX_FREE(utf8String);

        PKIX_RETURN(X500NAME);
}
#endif 




PKIX_Error *
PKIX_PL_X500Name_Match(
        PKIX_PL_X500Name *firstX500Name,
        PKIX_PL_X500Name *secondX500Name,
        PKIX_Boolean *pResult,
        void *plContext)
{
        SECItem *firstDerName = NULL;
        SECItem *secondDerName = NULL;
        SECComparison cmpResult;

        PKIX_ENTER(X500NAME, "PKIX_PL_X500Name_Match");
        PKIX_NULLCHECK_THREE(firstX500Name, secondX500Name, pResult);

        if (firstX500Name == secondX500Name){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        firstDerName = &firstX500Name->derName;
        secondDerName = &secondX500Name->derName;

        PKIX_NULLCHECK_TWO(firstDerName->data, secondDerName->data);

        cmpResult = SECITEM_CompareItem(firstDerName, secondDerName);
        if (cmpResult != SECEqual) {
            cmpResult = CERT_CompareName(&firstX500Name->nssDN,
                                         &secondX500Name->nssDN);
        }

        *pResult = (cmpResult == SECEqual);
                   
cleanup:

        PKIX_RETURN(X500NAME);
}



























PKIX_Error *
pkix_pl_X500Name_GetDERName(
        PKIX_PL_X500Name *xname,
        PRArenaPool *arena,
        SECItem **pDERName,
        void *plContext)
{
        SECItem *derName = NULL;

        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_GetDERName");

        PKIX_NULLCHECK_THREE(xname, arena, pDERName);

        
        if (xname->derName.data == NULL) {
            *pDERName = NULL;
            goto cleanup;
        }

        derName = SECITEM_ArenaDupItem(arena, &xname->derName);
        if (derName == NULL) {
            PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        *pDERName = derName;
cleanup:

        PKIX_RETURN(X500NAME);
}



























PKIX_Error *
pkix_pl_X500Name_GetCommonName(
        PKIX_PL_X500Name *xname,
        unsigned char **pCommonName,
        void *plContext)
{
        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_GetCommonName");
        PKIX_NULLCHECK_TWO(xname, pCommonName);

        *pCommonName = (unsigned char *)CERT_GetCommonName(&xname->nssDN);

        PKIX_RETURN(X500NAME);
}



























PKIX_Error *
pkix_pl_X500Name_GetCountryName(
        PKIX_PL_X500Name *xname,
        unsigned char **pCountryName,
        void *plContext)
{
        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_GetCountryName");
        PKIX_NULLCHECK_TWO(xname, pCountryName);

        *pCountryName = (unsigned char*)CERT_GetCountryName(&xname->nssDN);

        PKIX_RETURN(X500NAME);
}



























PKIX_Error *
pkix_pl_X500Name_GetOrgName(
        PKIX_PL_X500Name *xname,
        unsigned char **pOrgName,
        void *plContext)
{
        PKIX_ENTER(X500NAME, "pkix_pl_X500Name_GetOrgName");
        PKIX_NULLCHECK_TWO(xname, pOrgName);

        *pOrgName = (unsigned char*)CERT_GetOrgName(&xname->nssDN);

        PKIX_RETURN(X500NAME);
}
