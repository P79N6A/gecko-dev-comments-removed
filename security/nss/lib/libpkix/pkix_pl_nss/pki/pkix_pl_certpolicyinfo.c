









#include "pkix_pl_certpolicyinfo.h"



























PKIX_Error *
pkix_pl_CertPolicyInfo_Create(
        PKIX_PL_OID *oid,
        PKIX_List *qualifiers,
        PKIX_PL_CertPolicyInfo **pObject,
        void *plContext)
{
        PKIX_PL_CertPolicyInfo *policyInfo = NULL;

        PKIX_ENTER(CERTPOLICYINFO, "pkix_pl_CertPolicyInfo_Create");

        PKIX_NULLCHECK_TWO(oid, pObject);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_CERTPOLICYINFO_TYPE,
                sizeof (PKIX_PL_CertPolicyInfo),
                (PKIX_PL_Object **)&policyInfo,
                plContext),
                PKIX_COULDNOTCREATECERTPOLICYINFOOBJECT);

        PKIX_INCREF(oid);
        policyInfo->cpID = oid;

        PKIX_INCREF(qualifiers);
        policyInfo->policyQualifiers = qualifiers;

        *pObject = policyInfo;
        policyInfo = NULL;

cleanup:
        PKIX_DECREF(policyInfo);

        PKIX_RETURN(CERTPOLICYINFO);
}





static PKIX_Error *
pkix_pl_CertPolicyInfo_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_CertPolicyInfo *certPI = NULL;

        PKIX_ENTER(CERTPOLICYINFO, "pkix_pl_CertPolicyInfo_Destroy");

        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYINFO_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYINFO);

        certPI = (PKIX_PL_CertPolicyInfo*)object;

        PKIX_DECREF(certPI->cpID);
        PKIX_DECREF(certPI->policyQualifiers);

cleanup:

        PKIX_RETURN(CERTPOLICYINFO);
}





static PKIX_Error *
pkix_pl_CertPolicyInfo_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_CertPolicyInfo *certPI = NULL;
        PKIX_PL_String *oidString = NULL;
        PKIX_PL_String *listString = NULL;
        PKIX_PL_String *format = NULL;
        PKIX_PL_String *outString = NULL;

        PKIX_ENTER(CERTPOLICYINFO, "pkix_pl_CertPolicyInfo_ToString");

        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYINFO_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYINFO);

        certPI = (PKIX_PL_CertPolicyInfo *)object;

        PKIX_NULLCHECK_ONE(certPI->cpID);

        PKIX_TOSTRING
                (certPI->cpID,
                &oidString,
                plContext,
                PKIX_OIDTOSTRINGFAILED);

        PKIX_TOSTRING
                (certPI->policyQualifiers,
                &listString,
                plContext,
                PKIX_LISTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, "%s[%s]", 0, &format, plContext),
                PKIX_ERRORINSTRINGCREATE);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&outString, plContext, format, oidString, listString),
                PKIX_ERRORINSPRINTF);

        *pString = outString;

cleanup:

        PKIX_DECREF(oidString);
        PKIX_DECREF(listString);
        PKIX_DECREF(format);
        PKIX_RETURN(CERTPOLICYINFO);
}





static PKIX_Error *
pkix_pl_CertPolicyInfo_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_CertPolicyInfo *certPI = NULL;
        PKIX_UInt32 oidHash = 0;
        PKIX_UInt32 listHash = 0;

        PKIX_ENTER(CERTPOLICYINFO, "pkix_pl_CertPolicyInfo_Hashcode");

        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTPOLICYINFO_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYINFO);

        certPI = (PKIX_PL_CertPolicyInfo *)object;

        PKIX_NULLCHECK_ONE(certPI->cpID);

        PKIX_HASHCODE
                (certPI->cpID,
                &oidHash,
                plContext,
                PKIX_ERRORINOIDHASHCODE);

        PKIX_HASHCODE
                (certPI->policyQualifiers,
                &listHash,
                plContext,
                PKIX_ERRORINLISTHASHCODE);

        *pHashcode = (31 * oidHash) + listHash;

cleanup:

        PKIX_RETURN(CERTPOLICYINFO);
}






static PKIX_Error *
pkix_pl_CertPolicyInfo_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_CertPolicyInfo *firstCPI = NULL;
        PKIX_PL_CertPolicyInfo *secondCPI = NULL;
        PKIX_UInt32 secondType = 0;
        PKIX_Boolean compare = PKIX_FALSE;

        PKIX_ENTER(CERTPOLICYINFO, "pkix_pl_CertPolicyInfo_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                (firstObject, PKIX_CERTPOLICYINFO_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTCERTPOLICYINFO);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        PKIX_CHECK(PKIX_PL_Object_GetType
                (secondObject, &secondType, plContext),
                PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_CERTPOLICYINFO_TYPE) {
                *pResult = PKIX_FALSE;
                goto cleanup;
        }

        firstCPI = (PKIX_PL_CertPolicyInfo *)firstObject;
        secondCPI = (PKIX_PL_CertPolicyInfo *)secondObject;

        



        PKIX_NULLCHECK_TWO(firstCPI->cpID, secondCPI->cpID);

        PKIX_EQUALS
                (firstCPI->cpID,
                secondCPI->cpID,
                &compare,
                plContext,
                PKIX_OIDEQUALSFAILED);

        





        if (compare) {
                PKIX_EQUALS
                        (firstCPI->policyQualifiers,
                        secondCPI->policyQualifiers,
                        &compare,
                        plContext,
                        PKIX_LISTEQUALSFAILED);
        }

        *pResult = compare;

cleanup:

        PKIX_RETURN(CERTPOLICYINFO);
}













PKIX_Error *
pkix_pl_CertPolicyInfo_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTPOLICYINFO, "pkix_pl_CertPolicyInfo_RegisterSelf");

        entry.description = "CertPolicyInfo";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_CertPolicyInfo);
        entry.destructor = pkix_pl_CertPolicyInfo_Destroy;
        entry.equalsFunction = pkix_pl_CertPolicyInfo_Equals;
        entry.hashcodeFunction = pkix_pl_CertPolicyInfo_Hashcode;
        entry.toStringFunction = pkix_pl_CertPolicyInfo_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_CERTPOLICYINFO_TYPE] = entry;

        PKIX_RETURN(CERTPOLICYINFO);
}







PKIX_Error *
PKIX_PL_CertPolicyInfo_GetPolicyId(
        PKIX_PL_CertPolicyInfo *policyInfo,
        PKIX_PL_OID **pPolicyId,
        void *plContext)
{
        PKIX_ENTER(CERTPOLICYINFO, "PKIX_PL_CertPolicyInfo_GetPolicyId");

        PKIX_NULLCHECK_TWO(policyInfo, pPolicyId);

        PKIX_INCREF(policyInfo->cpID);

        *pPolicyId = policyInfo->cpID;

cleanup:
        PKIX_RETURN(CERTPOLICYINFO);
}





PKIX_Error *
PKIX_PL_CertPolicyInfo_GetPolQualifiers(
        PKIX_PL_CertPolicyInfo *policyInfo,
        PKIX_List **pQuals,
        void *plContext)
{
        PKIX_ENTER(CERTPOLICYINFO, "PKIX_PL_CertPolicyInfo_GetPolQualifiers");

        PKIX_NULLCHECK_TWO(policyInfo, pQuals);

        PKIX_INCREF(policyInfo->policyQualifiers);

        



        *pQuals = policyInfo->policyQualifiers;

cleanup:
        PKIX_RETURN(CERTPOLICYINFO);
}
