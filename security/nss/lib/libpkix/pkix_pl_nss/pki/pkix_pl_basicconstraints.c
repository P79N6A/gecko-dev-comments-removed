









#include "pkix_pl_basicconstraints.h"

























PKIX_Error *
pkix_pl_CertBasicConstraints_Create(
        PKIX_Boolean isCA,
        PKIX_Int32 pathLen,
        PKIX_PL_CertBasicConstraints **pObject,
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *basic = NULL;

        PKIX_ENTER(CERTBASICCONSTRAINTS,
                    "pkix_pl_CertBasicConstraints_Create");
        PKIX_NULLCHECK_ONE(pObject);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_CERTBASICCONSTRAINTS_TYPE,
                    sizeof (PKIX_PL_CertBasicConstraints),
                    (PKIX_PL_Object **)&basic,
                    plContext),
                    PKIX_COULDNOTCREATECERTBASICCONSTRAINTSOBJECT);

        basic->isCA = isCA;

        
        basic->pathLen = pathLen;

        *pObject = basic;

cleanup:

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}





static PKIX_Error *
pkix_pl_CertBasicConstraints_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *certB = NULL;

        PKIX_ENTER(CERTBASICCONSTRAINTS,
                "pkix_pl_CertBasicConstraints_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_CERTBASICCONSTRAINTS_TYPE, plContext),
                    PKIX_OBJECTNOTCERTBASICCONSTRAINTS);

        certB = (PKIX_PL_CertBasicConstraints*)object;

        certB->isCA = PKIX_FALSE;
        certB->pathLen = 0;

cleanup:

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}





static PKIX_Error *
pkix_pl_CertBasicConstraints_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *certBasicConstraintsString = NULL;
        PKIX_PL_CertBasicConstraints *certB = NULL;
        PKIX_Boolean isCA = PKIX_FALSE;
        PKIX_Int32 pathLen = 0;
        PKIX_PL_String *outString = NULL;
        char *fmtString = NULL;
        PKIX_Boolean pathlenArg = PKIX_FALSE;

        PKIX_ENTER(CERTBASICCONSTRAINTS,
                "pkix_pl_CertBasicConstraints_toString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_CERTBASICCONSTRAINTS_TYPE, plContext),
                    PKIX_FIRSTARGUMENTNOTCERTBASICCONSTRAINTSOBJECT);

        certB = (PKIX_PL_CertBasicConstraints *)object;

        







        isCA = certB->isCA;

        if (isCA) {
                pathLen = certB->pathLen;

                if (pathLen == CERT_UNLIMITED_PATH_CONSTRAINT) {
                        
                        fmtString = "CA(-1)";
                        pathlenArg = PKIX_FALSE;
                } else {
                        
                        fmtString = "CA(%d)";
                        pathlenArg = PKIX_TRUE;
                }
        } else {
                
                fmtString = "~CA";
                pathlenArg = PKIX_FALSE;
        }

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    fmtString,
                    0,
                    &certBasicConstraintsString,
                    plContext),
                    PKIX_STRINGCREATEFAILED);

        if (pathlenArg) {
                PKIX_CHECK(PKIX_PL_Sprintf
                            (&outString,
                            plContext,
                            certBasicConstraintsString,
                            pathLen),
                            PKIX_SPRINTFFAILED);
        } else {
                PKIX_CHECK(PKIX_PL_Sprintf
                            (&outString,
                            plContext,
                            certBasicConstraintsString),
                            PKIX_SPRINTFFAILED);
        }

        *pString = outString;

cleanup:

        PKIX_DECREF(certBasicConstraintsString);

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}





static PKIX_Error *
pkix_pl_CertBasicConstraints_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *certB = NULL;
        PKIX_Boolean isCA = PKIX_FALSE;
        PKIX_Int32 pathLen = 0;
        PKIX_Int32 hashInput = 0;
        PKIX_UInt32 cbcHash = 0;

        PKIX_ENTER(CERTBASICCONSTRAINTS,
                "pkix_pl_CertBasicConstraints_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_CERTBASICCONSTRAINTS_TYPE, plContext),
                    PKIX_OBJECTNOTCERTBASICCONSTRAINTS);

        certB = (PKIX_PL_CertBasicConstraints *)object;

        





        isCA = certB->isCA;

        if (isCA) {
                pathLen = certB->pathLen;

                hashInput = pathLen + 1 - PKIX_UNLIMITED_PATH_CONSTRAINT;
        }

        PKIX_CHECK(pkix_hash
                    ((const unsigned char *)&hashInput,
                    sizeof (hashInput),
                    &cbcHash,
                    plContext),
                    PKIX_HASHFAILED);

        *pHashcode = cbcHash;

cleanup:

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}






static PKIX_Error *
pkix_pl_CertBasicConstraints_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *firstCBC = NULL;
        PKIX_PL_CertBasicConstraints *secondCBC = NULL;
        PKIX_UInt32 secondType;
        PKIX_Boolean firstIsCA = PKIX_FALSE;
        PKIX_Boolean secondIsCA = PKIX_FALSE;
        PKIX_Int32 firstPathLen = 0;
        PKIX_Int32 secondPathLen = 0;

        PKIX_ENTER(CERTBASICCONSTRAINTS,
                "pkix_pl_CertBasicConstraints_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                    (firstObject, PKIX_CERTBASICCONSTRAINTS_TYPE, plContext),
                    PKIX_FIRSTOBJECTNOTCERTBASICCONSTRAINTS);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_CERTBASICCONSTRAINTS_TYPE) {
                *pResult = PKIX_FALSE;
                goto cleanup;
        }

        firstCBC = (PKIX_PL_CertBasicConstraints *)firstObject;
        secondCBC = (PKIX_PL_CertBasicConstraints *)secondObject;

        



        firstIsCA = firstCBC->isCA;

        



        secondIsCA = secondCBC->isCA;

        


        if (secondIsCA != firstIsCA) {
                *pResult = PKIX_FALSE;
                goto cleanup;
        }

        



        if (!firstIsCA) {
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        firstPathLen = firstCBC->pathLen;
        secondPathLen = secondCBC->pathLen;

        *pResult = (secondPathLen == firstPathLen);

cleanup:

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}













PKIX_Error *
pkix_pl_CertBasicConstraints_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTBASICCONSTRAINTS,
                "pkix_pl_CertBasicConstraints_RegisterSelf");

        entry.description = "CertBasicConstraints";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_CertBasicConstraints);
        entry.destructor = pkix_pl_CertBasicConstraints_Destroy;
        entry.equalsFunction = pkix_pl_CertBasicConstraints_Equals;
        entry.hashcodeFunction = pkix_pl_CertBasicConstraints_Hashcode;
        entry.toStringFunction = pkix_pl_CertBasicConstraints_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_CERTBASICCONSTRAINTS_TYPE] = entry;

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}







PKIX_Error *
PKIX_PL_BasicConstraints_GetCAFlag(
        PKIX_PL_CertBasicConstraints *basicConstraints,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_ENTER(CERTBASICCONSTRAINTS,
                "PKIX_PL_BasicConstraintsGetCAFlag");
        PKIX_NULLCHECK_TWO(basicConstraints, pResult);

        *pResult = basicConstraints->isCA;

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}





PKIX_Error *
PKIX_PL_BasicConstraints_GetPathLenConstraint(
        PKIX_PL_CertBasicConstraints *basicConstraints,
        PKIX_Int32 *pPathLenConstraint,
        void *plContext)
{
        PKIX_ENTER(CERTBASICCONSTRAINTS,
                "PKIX_PL_BasicConstraintsGetPathLenConstraint");
        PKIX_NULLCHECK_TWO(basicConstraints, pPathLenConstraint);

        *pPathLenConstraint = basicConstraints->pathLen;

        PKIX_RETURN(CERTBASICCONSTRAINTS);
}
