









#include "pkix_pl_certpolicyqualifier.h"
























PKIX_Error *
pkix_pl_CertPolicyQualifier_Create(
        PKIX_PL_OID *oid,
        PKIX_PL_ByteArray *qualifier,
        PKIX_PL_CertPolicyQualifier **pObject,
        void *plContext)
{
        PKIX_PL_CertPolicyQualifier *qual = NULL;

        PKIX_ENTER(CERTPOLICYQUALIFIER, "pkix_pl_CertPolicyQualifier_Create");

        PKIX_NULLCHECK_THREE(oid, qualifier, pObject);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_CERTPOLICYQUALIFIER_TYPE,
                sizeof (PKIX_PL_CertPolicyQualifier),
                (PKIX_PL_Object **)&qual,
                plContext),
                PKIX_COULDNOTCREATECERTPOLICYQUALIFIEROBJECT);

        PKIX_INCREF(oid);
        qual->policyQualifierId = oid;

        PKIX_INCREF(qualifier);
        qual->qualifier = qualifier;

        *pObject = qual;
        qual = NULL;

cleanup:
        PKIX_DECREF(qual);

        PKIX_RETURN(CERTPOLICYQUALIFIER);
}





static PKIX_Error *
pkix_pl_CertPolicyQualifier_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_CertPolicyQualifier *certPQ = NULL;

        PKIX_ENTER(CERTPOLICYQUALIFIER, "pkix_pl_CertPolicyQualifier_Destroy");

        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYQUALIFIER_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYQUALIFIER);

        certPQ = (PKIX_PL_CertPolicyQualifier*)object;

        PKIX_DECREF(certPQ->policyQualifierId);
        PKIX_DECREF(certPQ->qualifier);

cleanup:

        PKIX_RETURN(CERTPOLICYQUALIFIER);
}





static PKIX_Error *
pkix_pl_CertPolicyQualifier_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_CertPolicyQualifier *certPQ = NULL;
        char *asciiFormat = "%s:%s";
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *pqIDString = NULL;
        PKIX_PL_String *pqValString = NULL;
        PKIX_PL_String *outString = NULL;

        PKIX_ENTER(CERTPOLICYQUALIFIER, "pkix_pl_CertPolicyQualifier_ToString");

        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYQUALIFIER_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYQUALIFIER);

        certPQ = (PKIX_PL_CertPolicyQualifier *)object;

        



        PKIX_NULLCHECK_TWO(certPQ->policyQualifierId, certPQ->qualifier);

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, asciiFormat, 0, &formatString, plContext),
                PKIX_STRINGCREATEFAILED);

        PKIX_TOSTRING(certPQ->policyQualifierId, &pqIDString, plContext,
                PKIX_OIDTOSTRINGFAILED);

        PKIX_CHECK(pkix_pl_ByteArray_ToHexString
                (certPQ->qualifier, &pqValString, plContext),
                PKIX_BYTEARRAYTOHEXSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&outString, plContext, formatString, pqIDString, pqValString),
                PKIX_SPRINTFFAILED);

        *pString = outString;

cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(pqIDString);
        PKIX_DECREF(pqValString);
        PKIX_RETURN(CERTPOLICYQUALIFIER);
}





static PKIX_Error *
pkix_pl_CertPolicyQualifier_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_CertPolicyQualifier *certPQ = NULL;
        PKIX_UInt32 cpidHash = 0;
        PKIX_UInt32 cpqHash = 0;

        PKIX_ENTER(CERTPOLICYQUALIFIER, "pkix_pl_CertPolicyQualifier_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYQUALIFIER_TYPE, plContext),
                PKIX_OBJECTNOTCERTPOLICYQUALIFIER);

        certPQ = (PKIX_PL_CertPolicyQualifier *)object;

        PKIX_NULLCHECK_TWO(certPQ->policyQualifierId, certPQ->qualifier);

        PKIX_HASHCODE(certPQ->policyQualifierId, &cpidHash, plContext,
                PKIX_ERRORINOIDHASHCODE);

        PKIX_HASHCODE(certPQ->qualifier, &cpqHash, plContext,
                PKIX_ERRORINBYTEARRAYHASHCODE);

        *pHashcode = cpidHash*31 + cpqHash;

cleanup:

        PKIX_RETURN(CERTPOLICYQUALIFIER);
}






static PKIX_Error *
pkix_pl_CertPolicyQualifier_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_CertPolicyQualifier *firstCPQ = NULL;
        PKIX_PL_CertPolicyQualifier *secondCPQ = NULL;
        PKIX_UInt32 secondType = 0;
        PKIX_Boolean compare = PKIX_FALSE;

        PKIX_ENTER(CERTPOLICYQUALIFIER, "pkix_pl_CertPolicyQualifier_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                (firstObject, PKIX_CERTPOLICYQUALIFIER_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTCERTPOLICYQUALIFIER);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        PKIX_CHECK(PKIX_PL_Object_GetType
                (secondObject, &secondType, plContext),
                PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_CERTPOLICYQUALIFIER_TYPE) {
                *pResult = PKIX_FALSE;
                goto cleanup;
        }

        firstCPQ = (PKIX_PL_CertPolicyQualifier *)firstObject;
        secondCPQ = (PKIX_PL_CertPolicyQualifier *)secondObject;

        



        PKIX_NULLCHECK_TWO
                (firstCPQ->policyQualifierId, secondCPQ->policyQualifierId);

        PKIX_EQUALS
                (firstCPQ->policyQualifierId,
                secondCPQ->policyQualifierId,
                &compare,
                plContext,
                PKIX_OIDEQUALSFAILED);

        





        if (compare) {
                PKIX_NULLCHECK_TWO(firstCPQ->qualifier, secondCPQ->qualifier);

                PKIX_EQUALS
                        (firstCPQ->qualifier,
                        secondCPQ->qualifier,
                        &compare,
                        plContext,
                        PKIX_BYTEARRAYEQUALSFAILED);
        }

        *pResult = compare;

cleanup:

        PKIX_RETURN(CERTPOLICYQUALIFIER);
}













PKIX_Error *
pkix_pl_CertPolicyQualifier_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTPOLICYQUALIFIER,
                "pkix_pl_CertPolicyQualifier_RegisterSelf");

        entry.description = "CertPolicyQualifier";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_CertPolicyQualifier);
        entry.destructor = pkix_pl_CertPolicyQualifier_Destroy;
        entry.equalsFunction = pkix_pl_CertPolicyQualifier_Equals;
        entry.hashcodeFunction = pkix_pl_CertPolicyQualifier_Hashcode;
        entry.toStringFunction = pkix_pl_CertPolicyQualifier_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_CERTPOLICYQUALIFIER_TYPE] = entry;

        PKIX_RETURN(CERTPOLICYQUALIFIER);
}







PKIX_Error *
PKIX_PL_PolicyQualifier_GetPolicyQualifierId(
        PKIX_PL_CertPolicyQualifier *policyQualifierInfo,
        PKIX_PL_OID **pPolicyQualifierId,
        void *plContext)
{
        PKIX_ENTER(CERTPOLICYQUALIFIER,
                "PKIX_PL_PolicyQualifier_GetPolicyQualifierId");

        PKIX_NULLCHECK_TWO(policyQualifierInfo, pPolicyQualifierId);

        PKIX_INCREF(policyQualifierInfo->policyQualifierId);

        *pPolicyQualifierId = policyQualifierInfo->policyQualifierId;

cleanup:
        PKIX_RETURN(CERTPOLICYQUALIFIER);
}





PKIX_Error *
PKIX_PL_PolicyQualifier_GetQualifier(
        PKIX_PL_CertPolicyQualifier *policyQualifierInfo,
        PKIX_PL_ByteArray **pQualifier,
        void *plContext)
{
        PKIX_ENTER(CERTPOLICYQUALIFIER, "PKIX_PL_PolicyQualifier_GetQualifier");

        PKIX_NULLCHECK_TWO(policyQualifierInfo, pQualifier);

        PKIX_INCREF(policyQualifierInfo->qualifier);

        *pQualifier = policyQualifierInfo->qualifier;

cleanup:
        PKIX_RETURN(CERTPOLICYQUALIFIER);
}
