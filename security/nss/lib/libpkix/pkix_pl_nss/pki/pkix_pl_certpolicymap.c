










































#include "pkix_pl_certpolicymap.h"

























PKIX_Error *
pkix_pl_CertPolicyMap_Create(
        PKIX_PL_OID *issuerDomainPolicy,
        PKIX_PL_OID *subjectDomainPolicy,
        PKIX_PL_CertPolicyMap **pCertPolicyMap,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *policyMap = NULL;

        PKIX_ENTER(CERTPOLICYMAP, "pkix_pl_CertPolicyMap_Create");

        PKIX_NULLCHECK_THREE
                (issuerDomainPolicy, subjectDomainPolicy, pCertPolicyMap);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_CERTPOLICYMAP_TYPE,
                sizeof (PKIX_PL_CertPolicyMap),
                (PKIX_PL_Object **)&policyMap,
                plContext),
                PKIX_COULDNOTCREATECERTPOLICYMAPOBJECT);

        PKIX_INCREF(issuerDomainPolicy);
        policyMap->issuerDomainPolicy = issuerDomainPolicy;

        PKIX_INCREF(subjectDomainPolicy);
        policyMap->subjectDomainPolicy = subjectDomainPolicy;

        *pCertPolicyMap = policyMap;

cleanup:

        PKIX_RETURN(CERTPOLICYMAP);
}





static PKIX_Error *
pkix_pl_CertPolicyMap_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *certMap = NULL;

        PKIX_ENTER(CERTPOLICYMAP, "pkix_pl_CertPolicyMap_Destroy");

        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYMAP_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYMAP);

        certMap = (PKIX_PL_CertPolicyMap*)object;

        PKIX_DECREF(certMap->issuerDomainPolicy);
        PKIX_DECREF(certMap->subjectDomainPolicy);

cleanup:

        PKIX_RETURN(CERTPOLICYMAP);
}





static PKIX_Error *
pkix_pl_CertPolicyMap_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *certMap = NULL;
        PKIX_PL_String *format = NULL;
        PKIX_PL_String *outString = NULL;
        PKIX_PL_String *issuerString = NULL;
        PKIX_PL_String *subjectString = NULL;

        PKIX_ENTER(CERTPOLICYMAP, "pkix_pl_CertPolicyMap_ToString");

        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYMAP_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYMAP);

        certMap = (PKIX_PL_CertPolicyMap *)object;

        PKIX_TOSTRING
                (certMap->issuerDomainPolicy,
                &issuerString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (certMap->subjectDomainPolicy,
                &subjectString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, "%s=>%s", 0, &format, plContext),
                PKIX_ERRORINSTRINGCREATE);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&outString, plContext, format, issuerString, subjectString),
                PKIX_ERRORINSPRINTF);

        *pString = outString;

cleanup:
        PKIX_DECREF(format);
        PKIX_DECREF(issuerString);
        PKIX_DECREF(subjectString);

        PKIX_RETURN(CERTPOLICYMAP);
}





static PKIX_Error *
pkix_pl_CertPolicyMap_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_UInt32 issuerHash = 0;
        PKIX_UInt32 subjectHash = 0;
        PKIX_PL_CertPolicyMap *certMap = NULL;

        PKIX_ENTER(CERTPOLICYMAP, "pkix_pl_CertPolicyMap_Hashcode");

        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYMAP_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYMAP);

        certMap = (PKIX_PL_CertPolicyMap *)object;

        PKIX_HASHCODE
                (certMap->issuerDomainPolicy,
                &issuerHash,
                plContext,
                PKIX_OBJECTHASHCODEFAILED);

        PKIX_HASHCODE
                (certMap->subjectDomainPolicy,
                &subjectHash,
                plContext,
                PKIX_OBJECTHASHCODEFAILED);

        *pHashcode = issuerHash*31 + subjectHash;

cleanup:

        PKIX_RETURN(CERTPOLICYMAP);
}





static PKIX_Error *
pkix_pl_CertPolicyMap_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *firstCertMap = NULL;
        PKIX_PL_CertPolicyMap *secondCertMap = NULL;
        PKIX_UInt32 secondType = 0;
        PKIX_Boolean compare = PKIX_FALSE;

        PKIX_ENTER(CERTPOLICYMAP, "pkix_pl_CertPolicyMap_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                (firstObject, PKIX_CERTPOLICYMAP_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTCERTPOLICYMAP);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        PKIX_CHECK(PKIX_PL_Object_GetType
                (secondObject, &secondType, plContext),
                PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_CERTPOLICYMAP_TYPE) {
                *pResult = PKIX_FALSE;
                goto cleanup;
        }

        firstCertMap = (PKIX_PL_CertPolicyMap *)firstObject;
        secondCertMap = (PKIX_PL_CertPolicyMap *)secondObject;

        PKIX_EQUALS
                (firstCertMap->issuerDomainPolicy,
                secondCertMap->issuerDomainPolicy,
                &compare,
                plContext,
                PKIX_OBJECTEQUALSFAILED);

        if (compare) {
                PKIX_EQUALS
                        (firstCertMap->subjectDomainPolicy,
                        secondCertMap->subjectDomainPolicy,
                        &compare,
                        plContext,
                        PKIX_OBJECTEQUALSFAILED);
        }

        *pResult = compare;

cleanup:

        PKIX_RETURN(CERTPOLICYMAP);
}





static PKIX_Error *
pkix_pl_CertPolicyMap_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *original = NULL;
        PKIX_PL_CertPolicyMap *copy = NULL;

        PKIX_ENTER(CERTPOLICYMAP, "pkix_pl_CertPolicyMap_Duplicate");

        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYMAP_TYPE, plContext),
                PKIX_OBJECTARGUMENTNOTPOLICYMAP);

        original = (PKIX_PL_CertPolicyMap *)object;

        PKIX_CHECK(pkix_pl_CertPolicyMap_Create
                (original->issuerDomainPolicy,
                original->subjectDomainPolicy,
                &copy,
                plContext),
                PKIX_CERTPOLICYMAPCREATEFAILED);

        *pNewObject = (PKIX_PL_Object *)copy;

cleanup:

        PKIX_RETURN(CERTPOLICYMAP);
}













PKIX_Error *
pkix_pl_CertPolicyMap_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTPOLICYMAP, "pkix_pl_CertPolicyMap_RegisterSelf");

        entry.description = "CertPolicyMap";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_CertPolicyMap);
        entry.destructor = pkix_pl_CertPolicyMap_Destroy;
        entry.equalsFunction = pkix_pl_CertPolicyMap_Equals;
        entry.hashcodeFunction = pkix_pl_CertPolicyMap_Hashcode;
        entry.toStringFunction = pkix_pl_CertPolicyMap_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_pl_CertPolicyMap_Duplicate;

        systemClasses[PKIX_CERTPOLICYMAP_TYPE] = entry;

        PKIX_RETURN(CERTPOLICYMAP);
}







PKIX_Error *
PKIX_PL_CertPolicyMap_GetIssuerDomainPolicy(
        PKIX_PL_CertPolicyMap *policyMapping,
        PKIX_PL_OID **pIssuerDomainPolicy,
        void *plContext)
{
        PKIX_ENTER
                (CERTPOLICYMAP, "PKIX_PL_CertPolicyMap_GetIssuerDomainPolicy");

        PKIX_NULLCHECK_TWO(policyMapping, pIssuerDomainPolicy);

        PKIX_INCREF(policyMapping->issuerDomainPolicy);
        *pIssuerDomainPolicy = policyMapping->issuerDomainPolicy;

cleanup:
        PKIX_RETURN(CERTPOLICYMAP);
}





PKIX_Error *
PKIX_PL_CertPolicyMap_GetSubjectDomainPolicy(
        PKIX_PL_CertPolicyMap *policyMapping,
        PKIX_PL_OID **pSubjectDomainPolicy,
        void *plContext)
{
        PKIX_ENTER
                (CERTPOLICYMAP, "PKIX_PL_CertPolicyMap_GetSubjectDomainPolicy");

        PKIX_NULLCHECK_TWO(policyMapping, pSubjectDomainPolicy);

        PKIX_INCREF(policyMapping->subjectDomainPolicy);
        *pSubjectDomainPolicy = policyMapping->subjectDomainPolicy;

cleanup:
        PKIX_RETURN(CERTPOLICYMAP);
}
