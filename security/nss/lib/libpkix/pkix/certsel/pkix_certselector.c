










































#include "pkix_certselector.h"







static PKIX_Error *
pkix_CertSelector_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_CertSelector *selector = NULL;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTSELECTOR_TYPE, plContext),
                    PKIX_OBJECTNOTCERTSELECTOR);

        selector = (PKIX_CertSelector *)object;
        PKIX_DECREF(selector->params);
        PKIX_DECREF(selector->context);

cleanup:

        PKIX_RETURN(CERTSELECTOR);
}





static PKIX_Error *
pkix_CertSelector_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_CertSelector *certSelector = NULL;
        PKIX_CertSelector *certSelectorDuplicate = NULL;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Duplicate");
        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERTSELECTOR_TYPE, plContext),
                    PKIX_OBJECTNOTCERTSELECTOR);

        certSelector = (PKIX_CertSelector *)object;

        PKIX_CHECK(PKIX_CertSelector_Create
                    (certSelector->matchCallback,
                    certSelector->context,
                    &certSelectorDuplicate,
                    plContext),
                    PKIX_CERTSELECTORCREATEFAILED);

        PKIX_CHECK(PKIX_PL_Object_Duplicate
                    ((PKIX_PL_Object *)certSelector->params,
                    (PKIX_PL_Object **)&certSelectorDuplicate->params,
                    plContext),
                    PKIX_OBJECTDUPLICATEFAILED);

        *pNewObject = (PKIX_PL_Object *)certSelectorDuplicate;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(certSelectorDuplicate);
        }

        PKIX_RETURN(CERTSELECTOR);
}













































static PKIX_Error *
pkix_CertSelector_Match_BasicConstraint(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *basicConstraints = NULL;
        PKIX_Boolean caFlag = PKIX_FALSE; 
        PKIX_Int32 pathLength = 0;
        PKIX_Int32 minPathLength = 0;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_BasicConstraint");
        PKIX_NULLCHECK_THREE(params, cert, pResult);
        *pResult = PKIX_TRUE;

        PKIX_CHECK(PKIX_ComCertSelParams_GetBasicConstraints
                (params, &minPathLength, plContext),
                PKIX_COMCERTSELPARAMSGETBASICCONSTRAINTSFAILED);

        
        if (minPathLength == PKIX_CERTSEL_ALL_MATCH_MIN_PATHLENGTH) {
                goto cleanup;
        }

        PKIX_CHECK(PKIX_PL_Cert_GetBasicConstraints
                    (cert, &basicConstraints, plContext),
                    PKIX_CERTGETBASICCONSTRAINTSFAILED);

        if (basicConstraints != NULL) {
                PKIX_CHECK(PKIX_PL_BasicConstraints_GetCAFlag
                            (basicConstraints, &caFlag, plContext),
                            PKIX_BASICCONSTRAINTSGETCAFLAGFAILED);

                PKIX_CHECK(PKIX_PL_BasicConstraints_GetPathLenConstraint
                        (basicConstraints, &pathLength, plContext),
                        PKIX_BASICCONSTRAINTSGETPATHLENCONSTRAINTFAILED);
        }

        




        if (minPathLength >= 0){
                if ((!basicConstraints) || (caFlag == PKIX_FALSE)){
                        PKIX_ERROR(PKIX_CERTNOTALLOWEDTOSIGNCERTIFICATES);
                } else if ((pathLength != PKIX_UNLIMITED_PATH_CONSTRAINT) &&
                            (pathLength < minPathLength)){
                        PKIX_CERTSELECTOR_DEBUG
                            ("Basic Constraints path length match failed\n");
                        *pResult = PKIX_FALSE;
                        PKIX_ERROR(PKIX_PATHLENCONSTRAINTINVALID);
                }
        }

        
        if (minPathLength == PKIX_CERTSEL_ENDENTITY_MIN_PATHLENGTH) {
                if (caFlag == PKIX_TRUE) {
                    PKIX_CERTSELECTOR_DEBUG
                        ("Basic Constraints end-entity match failed\n");
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_PATHLENCONSTRAINTINVALID);
                }
        }

cleanup:

        PKIX_DECREF(basicConstraints);
        PKIX_RETURN(CERTSELECTOR);
}































static PKIX_Error *
pkix_CertSelector_Match_Policies(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 numConstraintPolicies = 0;
        PKIX_UInt32 numCertPolicies = 0;
        PKIX_UInt32 certPolicyIndex = 0;
        PKIX_Boolean result = PKIX_FALSE;
        PKIX_List *constraintPolicies = NULL; 
        PKIX_List *certPolicyInfos = NULL; 
        PKIX_PL_CertPolicyInfo *policyInfo = NULL;
        PKIX_PL_OID *polOID = NULL;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_Policies");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetPolicy
                (params, &constraintPolicies, plContext),
                PKIX_COMCERTSELPARAMSGETPOLICYFAILED);

        
        if (constraintPolicies) {
            PKIX_CHECK(PKIX_PL_Cert_GetPolicyInformation
                (cert, &certPolicyInfos, plContext),
                PKIX_CERTGETPOLICYINFORMATIONFAILED);

            
            if (!certPolicyInfos) {
                PKIX_CERTSELECTOR_DEBUG("Certificate has no policies\n");
                *pResult = PKIX_FALSE;
                PKIX_ERROR(PKIX_CERTSELECTORMATCHPOLICIESFAILED);
            }

            PKIX_CHECK(PKIX_List_GetLength
                (constraintPolicies, &numConstraintPolicies, plContext),
                PKIX_LISTGETLENGTHFAILED);

            if (numConstraintPolicies > 0) {

                PKIX_CHECK(PKIX_List_GetLength
                        (certPolicyInfos, &numCertPolicies, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                for (certPolicyIndex = 0;
                        ((!result) && (certPolicyIndex < numCertPolicies));
                        certPolicyIndex++) {

                        PKIX_CHECK(PKIX_List_GetItem
                                (certPolicyInfos,
                                certPolicyIndex,
                                (PKIX_PL_Object **)&policyInfo,
                                plContext),
                                PKIX_LISTGETELEMENTFAILED);
                        PKIX_CHECK(PKIX_PL_CertPolicyInfo_GetPolicyId
                                (policyInfo, &polOID, plContext),
                                PKIX_CERTPOLICYINFOGETPOLICYIDFAILED);

                        PKIX_CHECK(pkix_List_Contains
                                (constraintPolicies,
                                (PKIX_PL_Object *)polOID,
                                &result,
                                plContext),
                                PKIX_LISTCONTAINSFAILED);
                        PKIX_DECREF(policyInfo);
                        PKIX_DECREF(polOID);
                }
                if (!result) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHPOLICIESFAILED);
                }
            }
        }

cleanup:

        PKIX_DECREF(constraintPolicies);
        PKIX_DECREF(certPolicyInfos);
        PKIX_DECREF(policyInfo);
        PKIX_DECREF(polOID);

        PKIX_RETURN(CERTSELECTOR);

}































static PKIX_Error *
pkix_CertSelector_Match_CertificateValid(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_Date *validityTime = NULL;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_CertificateValid");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetCertificateValid
                (params, &validityTime, plContext),
                PKIX_COMCERTSELPARAMSGETCERTIFICATEVALIDFAILED);

        
        if (validityTime) {
                PKIX_CHECK(PKIX_PL_Cert_CheckValidity
                        (cert, validityTime, plContext),
                        PKIX_CERTCHECKVALIDITYFAILED);
        }

cleanup:
        if (PKIX_ERROR_RECEIVED) {
            *pResult = PKIX_FALSE;
        }
        PKIX_DECREF(validityTime);

        PKIX_RETURN(CERTSELECTOR);
}






























static PKIX_Error *
pkix_CertSelector_Match_NameConstraints(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_NameConstraints");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetNameConstraints
                (params, &nameConstraints, plContext),
                PKIX_COMCERTSELPARAMSGETNAMECONSTRAINTSFAILED);

        if (nameConstraints != NULL) {

                PKIX_CHECK(PKIX_PL_Cert_CheckNameConstraints
                    (cert, nameConstraints, plContext),
                    PKIX_CERTCHECKNAMECONSTRAINTSFAILED);
        }

cleanup:
        if (PKIX_ERROR_RECEIVED) {
            *pResult = PKIX_FALSE;
        }

        PKIX_DECREF(nameConstraints);
        PKIX_RETURN(CERTSELECTOR);
}






























static PKIX_Error *
pkix_CertSelector_Match_PathToNames(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_List *pathToNamesList = NULL;
        PKIX_Boolean passed = PKIX_FALSE;
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_PathToNames");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetPathToNames
                (params, &pathToNamesList, plContext),
                PKIX_COMCERTSELPARAMSGETPATHTONAMESFAILED);

        if (pathToNamesList != NULL) {

            PKIX_CHECK(PKIX_PL_Cert_GetNameConstraints
                    (cert, &nameConstraints, plContext),
                    PKIX_CERTGETNAMECONSTRAINTSFAILED);

            if (nameConstraints != NULL) {

                PKIX_CHECK(PKIX_PL_CertNameConstraints_CheckNamesInNameSpace
                        (pathToNamesList, nameConstraints, &passed, plContext),
                        PKIX_CERTNAMECONSTRAINTSCHECKNAMESINNAMESPACEFAILED);

                if (passed != PKIX_TRUE) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHPATHTONAMESFAILED);
                }
            }

        }

cleanup:

        PKIX_DECREF(nameConstraints);
        PKIX_DECREF(pathToNamesList);

        PKIX_RETURN(CERTSELECTOR);
}





























static PKIX_Error *
pkix_CertSelector_Match_SubjAltNames(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_List *subjAltNamesList = NULL;
        PKIX_List *certSubjAltNames = NULL;
        PKIX_PL_GeneralName *name = NULL;
        PKIX_Boolean checkPassed = PKIX_FALSE;
        PKIX_Boolean matchAll = PKIX_TRUE;
        PKIX_UInt32 i, numItems;
        PKIX_UInt32 matchCount = 0;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_SubjAltNames");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetMatchAllSubjAltNames
                    (params, &matchAll, plContext),
                    PKIX_COMCERTSELPARAMSGETMATCHALLSUBJALTNAMESFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubjAltNames
                    (params, &subjAltNamesList, plContext),
                    PKIX_COMCERTSELPARAMSGETSUBJALTNAMESFAILED);

        if (subjAltNamesList != NULL) {

            PKIX_CHECK(PKIX_PL_Cert_GetSubjectAltNames
                    (cert, &certSubjAltNames, plContext),
                    PKIX_CERTGETSUBJALTNAMESFAILED);

            if (certSubjAltNames == NULL) {
                *pResult = PKIX_FALSE;
                PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJALTNAMESFAILED);
            }

            PKIX_CHECK(PKIX_List_GetLength
                       (subjAltNamesList, &numItems, plContext),
                       PKIX_LISTGETLENGTHFAILED);
            
            for (i = 0; i < numItems; i++) {
                
                PKIX_CHECK(PKIX_List_GetItem
                           (subjAltNamesList,
                            i,
                            (PKIX_PL_Object **) &name,
                            plContext),
                           PKIX_LISTGETITEMFAILED);
                
                PKIX_CHECK(pkix_List_Contains
                           (certSubjAltNames,
                            (PKIX_PL_Object *) name,
                            &checkPassed,
                            plContext),
                           PKIX_LISTCONTAINSFAILED);
                
                PKIX_DECREF(name);
                
                if (checkPassed == PKIX_TRUE) {
                    
                    if (matchAll == PKIX_FALSE) {
                        
                        matchCount = numItems;
                        break;
                    } else {
                        
                        matchCount++;
                    }
                    
                }
                
            }
            
            if (matchCount != numItems) {
                *pResult = PKIX_FALSE;
                PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJALTNAMESFAILED);
            }
        }

cleanup:

        PKIX_DECREF(name);
        PKIX_DECREF(certSubjAltNames);
        PKIX_DECREF(subjAltNamesList);

        PKIX_RETURN(CERTSELECTOR);
}





























static PKIX_Error *
pkix_CertSelector_Match_ExtendedKeyUsage(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_List *extKeyUsageList = NULL;
        PKIX_List *certExtKeyUsageList = NULL;
        PKIX_PL_OID *ekuOid = NULL;
        PKIX_Boolean isContained = PKIX_FALSE;
        PKIX_UInt32 numItems = 0;
        PKIX_UInt32 i;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_ExtendedKeyUsage");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetExtendedKeyUsage
                    (params, &extKeyUsageList, plContext),
                    PKIX_COMCERTSELPARAMSGETEXTENDEDKEYUSAGEFAILED);

        if (extKeyUsageList == NULL) {
                goto cleanup;
        }

        PKIX_CHECK(PKIX_PL_Cert_GetExtendedKeyUsage
                    (cert, &certExtKeyUsageList, plContext),
                    PKIX_CERTGETEXTENDEDKEYUSAGEFAILED);

        if (certExtKeyUsageList != NULL) {

            PKIX_CHECK(PKIX_List_GetLength
                    (extKeyUsageList, &numItems, plContext),
                    PKIX_LISTGETLENGTHFAILED);

            for (i = 0; i < numItems; i++) {

                PKIX_CHECK(PKIX_List_GetItem
                    (extKeyUsageList, i, (PKIX_PL_Object **)&ekuOid, plContext),
                    PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(pkix_List_Contains
                    (certExtKeyUsageList,
                    (PKIX_PL_Object *)ekuOid,
                    &isContained,
                    plContext),
                    PKIX_LISTCONTAINSFAILED);

                PKIX_DECREF(ekuOid);

                if (isContained != PKIX_TRUE) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHEXTENDEDKEYUSAGEFAILED);
                }
            }
        }

cleanup:

        PKIX_DECREF(ekuOid);
        PKIX_DECREF(extKeyUsageList);
        PKIX_DECREF(certExtKeyUsageList);

        PKIX_RETURN(CERTSELECTOR);
}




























static PKIX_Error *
pkix_CertSelector_Match_KeyUsage(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_UInt32 keyUsage = 0;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_KeyUsage");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetKeyUsage
                    (params, &keyUsage, plContext),
                    PKIX_COMCERTSELPARAMSGETKEYUSAGEFAILED);

        if (keyUsage != 0) {

            PKIX_CHECK(PKIX_PL_Cert_VerifyKeyUsage
                        (cert, keyUsage, plContext),
                        PKIX_CERTVERIFYKEYUSAGEFAILED);

        }

cleanup:
        if (PKIX_ERROR_RECEIVED) {
            *pResult = PKIX_FALSE;
        }

        PKIX_RETURN(CERTSELECTOR);
}




























static PKIX_Error *
pkix_CertSelector_Match_SubjKeyId(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_ByteArray *selSubjKeyId = NULL;
        PKIX_PL_ByteArray *certSubjKeyId = NULL;
        PKIX_Boolean equals = PKIX_FALSE;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_SubjKeyId");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubjKeyIdentifier
                    (params, &selSubjKeyId, plContext),
                    PKIX_COMCERTSELPARAMSGETSUBJKEYIDENTIFIERFAILED);

        if (selSubjKeyId != NULL) {

                PKIX_CHECK(PKIX_PL_Cert_GetSubjectKeyIdentifier
                    (cert, &certSubjKeyId, plContext),
                    PKIX_CERTGETSUBJECTKEYIDENTIFIERFAILED);

                if (certSubjKeyId == NULL) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJKEYIDFAILED);
                }

                PKIX_CHECK(PKIX_PL_Object_Equals
                           ((PKIX_PL_Object *)selSubjKeyId,
                            (PKIX_PL_Object *)certSubjKeyId,
                            &equals,
                            plContext),
                           PKIX_OBJECTEQUALSFAILED);
                
                if (equals != PKIX_TRUE) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJKEYIDFAILED);
                }
        }

cleanup:

        PKIX_DECREF(selSubjKeyId);
        PKIX_DECREF(certSubjKeyId);

        PKIX_RETURN(CERTSELECTOR);
}




























static PKIX_Error *
pkix_CertSelector_Match_AuthKeyId(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_ByteArray *selAuthKeyId = NULL;
        PKIX_PL_ByteArray *certAuthKeyId = NULL;
        PKIX_Boolean equals = PKIX_FALSE;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_AuthKeyId");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetAuthorityKeyIdentifier
                    (params, &selAuthKeyId, plContext),
                    PKIX_COMCERTSELPARAMSGETAUTHORITYKEYIDENTIFIERFAILED);

        if (selAuthKeyId != NULL) {

                PKIX_CHECK(PKIX_PL_Cert_GetAuthorityKeyIdentifier
                    (cert, &certAuthKeyId, plContext),
                    PKIX_CERTGETAUTHORITYKEYIDENTIFIERFAILED);

                if (certAuthKeyId == NULL) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHAUTHKEYIDFAILED);
                }
                PKIX_CHECK(PKIX_PL_Object_Equals
                           ((PKIX_PL_Object *)selAuthKeyId,
                            (PKIX_PL_Object *)certAuthKeyId,
                            &equals,
                            plContext),
                           PKIX_OBJECTEQUALSFAILED);
                
                if (equals != PKIX_TRUE) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHAUTHKEYIDFAILED);
                }
        }

cleanup:

        PKIX_DECREF(selAuthKeyId);
        PKIX_DECREF(certAuthKeyId);

        PKIX_RETURN(CERTSELECTOR);
}




























static PKIX_Error *
pkix_CertSelector_Match_SubjPKAlgId(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_OID *selPKAlgId = NULL;
        PKIX_PL_OID *certPKAlgId = NULL;
        PKIX_Boolean equals = PKIX_FALSE;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_SubjPKAlgId");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubjPKAlgId
                    (params, &selPKAlgId, plContext),
                    PKIX_COMCERTSELPARAMSGETSUBJPKALGIDFAILED);

        if (selPKAlgId != NULL) {

                PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKeyAlgId
                    (cert, &certPKAlgId, plContext),
                    PKIX_CERTGETSUBJECTPUBLICKEYALGIDFAILED);

                if (certPKAlgId != NULL) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJPKALGIDFAILED);
                }
                PKIX_CHECK(PKIX_PL_Object_Equals
                           ((PKIX_PL_Object *)selPKAlgId,
                            (PKIX_PL_Object *)certPKAlgId,
                            &equals,
                            plContext),
                           PKIX_OBJECTEQUALSFAILED);
                
                if (equals != PKIX_TRUE) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJPKALGIDFAILED);
                }
        }

cleanup:

        PKIX_DECREF(selPKAlgId);
        PKIX_DECREF(certPKAlgId);

        PKIX_RETURN(CERTSELECTOR);
}




























static PKIX_Error *
pkix_CertSelector_Match_SubjPubKey(
        PKIX_ComCertSelParams *params,
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_PublicKey *selPK = NULL;
        PKIX_PL_PublicKey *certPK = NULL;
        PKIX_Boolean equals = PKIX_FALSE;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_Match_SubjPubKey");
        PKIX_NULLCHECK_THREE(params, cert, pResult);

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubjPubKey
                    (params, &selPK, plContext),
                    PKIX_COMCERTSELPARAMSGETSUBJPUBKEYFAILED);

        if (selPK != NULL) {

                PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                    (cert, &certPK, plContext),
                    PKIX_CERTGETSUBJECTPUBLICKEYFAILED);

                if (certPK == NULL) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJPUBKEYFAILED);
                }
                PKIX_CHECK(PKIX_PL_Object_Equals
                           ((PKIX_PL_Object *)selPK,
                            (PKIX_PL_Object *)certPK,
                            &equals,
                            plContext),
                           PKIX_OBJECTEQUALSFAILED);
                
                if (equals != PKIX_TRUE) {
                    *pResult = PKIX_FALSE;
                    PKIX_ERROR(PKIX_CERTSELECTORMATCHSUBJPUBKEYFAILED);
                }
        }

cleanup:

        PKIX_DECREF(selPK);
        PKIX_DECREF(certPK);

        PKIX_RETURN(CERTSELECTOR);
}







































static PKIX_Error *
pkix_CertSelector_DefaultMatch(
        PKIX_CertSelector *selector,
        PKIX_PL_Cert *cert,
        void *plContext)
{
        PKIX_ComCertSelParams *params = NULL;
        PKIX_PL_X500Name *certSubject = NULL;
        PKIX_PL_X500Name *selSubject = NULL;
        PKIX_PL_X500Name *certIssuer = NULL;
        PKIX_PL_X500Name *selIssuer = NULL;
        PKIX_PL_BigInt *certSerialNumber = NULL;
        PKIX_PL_BigInt *selSerialNumber = NULL;
        PKIX_PL_Cert *selCert = NULL;
        PKIX_PL_Date *selDate = NULL;
        PKIX_UInt32 selVersion = 0xFFFFFFFF;
        PKIX_UInt32 certVersion = 0;
        PKIX_Boolean result = PKIX_TRUE;
        PKIX_Boolean isLeafCert = PKIX_TRUE;

#ifdef PKIX_BUILDDEBUG
        PKIX_PL_String *certString = NULL;
        void *certAscii = NULL;
        PKIX_UInt32 certAsciiLen;
#endif

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_DefaultMatch");
        PKIX_NULLCHECK_TWO(selector, cert);

        PKIX_INCREF(selector->params);
        params = selector->params;

        
        PKIX_CHECK(PKIX_ComCertSelParams_GetLeafCertFlag
                    (params, &isLeafCert, plContext),
                    PKIX_COMCERTSELPARAMSGETLEAFCERTFLAGFAILED);

        if (params == NULL){
                goto cleanup;
        }

        PKIX_CHECK(PKIX_ComCertSelParams_GetVersion
                    (params, &selVersion, plContext),
                    PKIX_COMCERTSELPARAMSGETVERSIONFAILED);

        if (selVersion != 0xFFFFFFFF){
                PKIX_CHECK(PKIX_PL_Cert_GetVersion
                            (cert, &certVersion, plContext),
                            PKIX_CERTGETVERSIONFAILED);

                if (selVersion != certVersion) {
                        PKIX_ERROR(PKIX_CERTSELECTORMATCHCERTVERSIONFAILED);
                }
        }

        PKIX_CHECK(PKIX_ComCertSelParams_GetSubject
                    (params, &selSubject, plContext),
                    PKIX_COMCERTSELPARAMSGETSUBJECTFAILED);

        if (selSubject){
                PKIX_CHECK(PKIX_PL_Cert_GetSubject
                            (cert, &certSubject, plContext),
                            PKIX_CERTGETSUBJECTFAILED);

                if (certSubject){
                        PKIX_CHECK(PKIX_PL_X500Name_Match
                            (selSubject, certSubject, &result, plContext),
                            PKIX_X500NAMEMATCHFAILED);

                        if (result == PKIX_FALSE){
                            PKIX_ERROR(PKIX_CERTSELECTORMATCHCERTSUBJECTFAILED);
                        }
                } else { 
                        PKIX_ERROR(PKIX_CERTSELECTORMATCHCERTSUBJECTFAILED);
                }
        }

        PKIX_CHECK(PKIX_ComCertSelParams_GetIssuer
                    (params, &selIssuer, plContext),
                    PKIX_COMCERTSELPARAMSGETISSUERFAILED);

        if (selIssuer){
                PKIX_CHECK(PKIX_PL_Cert_GetIssuer
                            (cert, &certIssuer, plContext),
                            PKIX_CERTGETISSUERFAILED);

                PKIX_CHECK(PKIX_PL_X500Name_Match
                            (selIssuer, certIssuer, &result, plContext),
                            PKIX_X500NAMEMATCHFAILED);

                if (result == PKIX_FALSE){
                        PKIX_ERROR(PKIX_CERTSELECTORMATCHCERTISSUERFAILED);
                }
        }

        PKIX_CHECK(PKIX_ComCertSelParams_GetSerialNumber
                    (params, &selSerialNumber, plContext),
                    PKIX_COMCERTSELPARAMSGETSERIALNUMBERFAILED);

        if (selSerialNumber){
                PKIX_CHECK(PKIX_PL_Cert_GetSerialNumber
                            (cert, &certSerialNumber, plContext),
                            PKIX_CERTGETSERIALNUMBERFAILED);

                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object *)selSerialNumber,
                            (PKIX_PL_Object *)certSerialNumber,
                            &result,
                            plContext),
                            PKIX_OBJECTEQUALSFAILED);

                if (result == PKIX_FALSE){
                        PKIX_ERROR(PKIX_CERTSELECTORMATCHCERTSERIALNUMFAILED);
                }
        }

        PKIX_CHECK(PKIX_ComCertSelParams_GetCertificate
                    (params, &selCert, plContext),
                    PKIX_COMCERTSELPARAMSGETCERTIFICATEFAILED);

        if (selCert){
                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object *) selCert,
                            (PKIX_PL_Object *) cert,
                            &result,
                            plContext),
                            PKIX_OBJECTEQUALSFAILED);

                if (result == PKIX_FALSE){
                        PKIX_ERROR(PKIX_CERTSELECTORMATCHCERTOBJECTFAILED);
                }
        }

        PKIX_CHECK(PKIX_ComCertSelParams_GetCertificateValid
                    (params, &selDate, plContext),
                    PKIX_COMCERTSELPARAMSGETCERTIFICATEVALIDFAILED);

        if (selDate){
                PKIX_CHECK(PKIX_PL_Cert_CheckValidity
                            (cert, selDate, plContext),
                            PKIX_CERTCHECKVALIDITYFAILED);
        }

        PKIX_CHECK(pkix_CertSelector_Match_BasicConstraint
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHBASICCONSTRAINTFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_Policies
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHPOLICIESFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_CertificateValid
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHCERTIFICATEVALIDFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_NameConstraints
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHNAMECONSTRAINTSFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_PathToNames
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHPATHTONAMESFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_SubjAltNames
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHSUBJALTNAMESFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_VerifyCertAndKeyType(cert, !isLeafCert,
                                                     plContext),
                   PKIX_CERTVERIFYCERTTYPEFAILED);

        
        PKIX_CHECK(pkix_CertSelector_Match_ExtendedKeyUsage
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHEXTENDEDKEYUSAGEFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_KeyUsage
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHKEYUSAGEFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_SubjKeyId
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHSUBJKEYIDFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_AuthKeyId
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHAUTHKEYIDFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_SubjPKAlgId
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHSUBJPKALGIDFAILED);

        PKIX_CHECK(pkix_CertSelector_Match_SubjPubKey
                    (params, cert, &result, plContext),
                    PKIX_CERTSELECTORMATCHSUBJPUBKEYFAILED);

        


#ifdef PKIX_BUILDDEBUG

        PKIX_CHECK(pkix_pl_Cert_ToString_Helper
                    (cert, PKIX_TRUE, &certString, plContext),
                    PKIX_CERTTOSTRINGHELPERFAILED);

        PKIX_CHECK(PKIX_PL_String_GetEncoded
                (certString,
                PKIX_ESCASCII,
                &certAscii,
                &certAsciiLen,
                plContext),
                PKIX_STRINGGETENCODEDFAILED);

        PKIX_CERTSELECTOR_DEBUG_ARG("Cert Selected:\n%s\n", certAscii);

#endif

cleanup:

#ifdef PKIX_BUILDDEBUG
        PKIX_DECREF(certString);
        PKIX_FREE(certAscii);
#endif

        PKIX_DECREF(certSubject);
        PKIX_DECREF(selSubject);
        PKIX_DECREF(certIssuer);
        PKIX_DECREF(selIssuer);
        PKIX_DECREF(certSerialNumber);
        PKIX_DECREF(selSerialNumber);
        PKIX_DECREF(selCert);
        PKIX_DECREF(selDate);
        PKIX_DECREF(params);
        PKIX_RETURN(CERTSELECTOR);
}













PKIX_Error *
pkix_CertSelector_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTSELECTOR, "pkix_CertSelector_RegisterSelf");

        entry.description = "CertSelector";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_CertSelector);
        entry.destructor = pkix_CertSelector_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_CertSelector_Duplicate;

        systemClasses[PKIX_CERTSELECTOR_TYPE] = entry;

        PKIX_RETURN(CERTSELECTOR);
}







PKIX_Error *
PKIX_CertSelector_Create(
        PKIX_CertSelector_MatchCallback callback,
        PKIX_PL_Object *certSelectorContext,
        PKIX_CertSelector **pSelector,
        void *plContext)
{
        PKIX_CertSelector *selector = NULL;

        PKIX_ENTER(CERTSELECTOR, "PKIX_CertSelector_Create");
        PKIX_NULLCHECK_ONE(pSelector);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_CERTSELECTOR_TYPE,
                    sizeof (PKIX_CertSelector),
                    (PKIX_PL_Object **)&selector,
                    plContext),
                    PKIX_COULDNOTCREATECERTSELECTOROBJECT);

        





        if (callback){
                selector->matchCallback = callback;
        } else {
                selector->matchCallback = pkix_CertSelector_DefaultMatch;
        }

        
        selector->params = NULL;

        PKIX_INCREF(certSelectorContext);
        selector->context = certSelectorContext;

        *pSelector = selector;

cleanup:

        PKIX_RETURN(CERTSELECTOR);

}





PKIX_Error *
PKIX_CertSelector_GetMatchCallback(
        PKIX_CertSelector *selector,
        PKIX_CertSelector_MatchCallback *pCallback,
        void *plContext)
{
        PKIX_ENTER(CERTSELECTOR, "PKIX_CertSelector_GetMatchCallback");
        PKIX_NULLCHECK_TWO(selector, pCallback);

        *pCallback = selector->matchCallback;

        PKIX_RETURN(CERTSELECTOR);
}





PKIX_Error *
PKIX_CertSelector_GetCertSelectorContext(
        PKIX_CertSelector *selector,
        PKIX_PL_Object **pCertSelectorContext,
        void *plContext)
{
        PKIX_ENTER(CERTSELECTOR, "PKIX_CertSelector_GetCertSelectorContext");
        PKIX_NULLCHECK_TWO(selector, pCertSelectorContext);

        PKIX_INCREF(selector->context);

        *pCertSelectorContext = selector->context;

cleanup:
        PKIX_RETURN(CERTSELECTOR);
}





PKIX_Error *
PKIX_CertSelector_GetCommonCertSelectorParams(
        PKIX_CertSelector *selector,
        PKIX_ComCertSelParams **pParams,
        void *plContext)
{
        PKIX_ENTER(CERTSELECTOR,
                    "PKIX_CertSelector_GetCommonCertSelectorParams");

        PKIX_NULLCHECK_TWO(selector, pParams);

        PKIX_INCREF(selector->params);
        *pParams = selector->params;

cleanup:
        PKIX_RETURN(CERTSELECTOR);

}





PKIX_Error *
PKIX_CertSelector_SetCommonCertSelectorParams(
        PKIX_CertSelector *selector,
        PKIX_ComCertSelParams *params,
        void *plContext)
{
        PKIX_ENTER(CERTSELECTOR,
                    "PKIX_CertSelector_SetCommonCertSelectorParams");

        PKIX_NULLCHECK_ONE(selector);

        PKIX_DECREF(selector->params);
        PKIX_INCREF(params);
        selector->params = params;

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                    ((PKIX_PL_Object *)selector, plContext),
                    PKIX_OBJECTINVALIDATECACHEFAILED);

cleanup:

        PKIX_RETURN(CERTSELECTOR);

}





























PKIX_Error *
pkix_CertSelector_Select(
	PKIX_CertSelector *selector,
	PKIX_List *before,
	PKIX_List **pAfter,
	void *plContext)
{
	PKIX_UInt32 numBefore = 0;
	PKIX_UInt32 i = 0;
	PKIX_List *filtered = NULL;
	PKIX_PL_Cert *candidate = NULL;

        PKIX_ENTER(CERTSELECTOR, "PKIX_CertSelector_Select");
        PKIX_NULLCHECK_THREE(selector, before, pAfter);

        PKIX_CHECK(PKIX_List_Create(&filtered, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_GetLength(before, &numBefore, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (i = 0; i < numBefore; i++) {

                PKIX_CHECK(PKIX_List_GetItem
                        (before, i, (PKIX_PL_Object **)&candidate, plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK_ONLY_FATAL(selector->matchCallback
                        (selector, candidate, plContext),
                        PKIX_CERTSELECTORMATCHCALLBACKFAILED);

                if (!(PKIX_ERROR_RECEIVED)) {

                        PKIX_CHECK_ONLY_FATAL(PKIX_List_AppendItem
                                (filtered,
                                (PKIX_PL_Object *)candidate,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);
                }

                pkixTempErrorReceived = PKIX_FALSE;
                PKIX_DECREF(candidate);
        }

        PKIX_CHECK(PKIX_List_SetImmutable(filtered, plContext),
                PKIX_LISTSETIMMUTABLEFAILED);

        
        pkixTempErrorReceived = PKIX_FALSE;

        *pAfter = filtered;
        filtered = NULL;

cleanup:

        PKIX_DECREF(filtered);
        PKIX_DECREF(candidate);

        PKIX_RETURN(CERTSELECTOR);

}
