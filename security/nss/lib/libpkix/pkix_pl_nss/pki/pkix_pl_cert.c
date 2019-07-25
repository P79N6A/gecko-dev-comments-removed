










































#include "pkix_pl_cert.h"

extern PKIX_PL_HashTable *cachedCertSigTable;


































static PKIX_Error *
pkix_pl_Cert_IsExtensionCritical(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 tag,
        PKIX_Boolean *pCritical,
        void *plContext)
{
        PKIX_Boolean criticality = PKIX_FALSE;
        CERTCertExtension **extensions = NULL;
        SECStatus rv;

        PKIX_ENTER(CERT, "pkix_pl_Cert_IsExtensionCritical");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pCritical);

        extensions = cert->nssCert->extensions;
        PKIX_NULLCHECK_ONE(extensions);

        PKIX_CERT_DEBUG("\t\tCalling CERT_GetExtenCriticality).\n");
        rv = CERT_GetExtenCriticality(extensions, tag, &criticality);
        if (SECSuccess == rv) {
                *pCritical = criticality;
        } else {
                *pCritical = PKIX_FALSE;
        }

        PKIX_RETURN(CERT);
}




























static PKIX_Error *
pkix_pl_Cert_DecodePolicyInfo(
        CERTCertificate *nssCert,
        PKIX_List **pCertPolicyInfos,
        void *plContext)
{

        SECStatus rv;
        SECItem encodedCertPolicyInfo;

        
        CERTCertificatePolicies *certPol = NULL;
        CERTPolicyInfo **policyInfos = NULL;

        
        PKIX_List *infos = NULL;

        PKIX_PL_OID *pkixOID = NULL;
        PKIX_List *qualifiers = NULL;
        PKIX_PL_CertPolicyInfo *certPolicyInfo = NULL;
        PKIX_PL_CertPolicyQualifier *certPolicyQualifier = NULL;
        PKIX_PL_ByteArray *qualifierArray = NULL;

        PKIX_ENTER(CERT, "pkix_pl_Cert_DecodePolicyInfo");
        PKIX_NULLCHECK_TWO(nssCert, pCertPolicyInfos);

        
        PKIX_CERT_DEBUG("\t\tCERT_FindCertExtension).\n");
        rv = CERT_FindCertExtension
                (nssCert,
                SEC_OID_X509_CERTIFICATE_POLICIES,
                &encodedCertPolicyInfo);
        if (SECSuccess != rv) {
                *pCertPolicyInfos = NULL;
                goto cleanup;
        }

        
        PKIX_CERT_DEBUG("\t\tCERT_DecodeCertificatePoliciesExtension).\n");
        certPol = CERT_DecodeCertificatePoliciesExtension
                (&encodedCertPolicyInfo);

        PORT_Free(encodedCertPolicyInfo.data);

        if (NULL == certPol) {
                PKIX_ERROR(PKIX_CERTDECODECERTIFICATEPOLICIESEXTENSIONFAILED);
        }

        



        policyInfos = certPol->policyInfos;
        if (!policyInfos) {
                *pCertPolicyInfos = NULL;
                goto cleanup;
        }

        
        PKIX_CHECK(PKIX_List_Create(&infos, plContext),
                PKIX_LISTCREATEFAILED);

        



        while (*policyInfos != NULL) {
                CERTPolicyInfo *policyInfo = *policyInfos;
                CERTPolicyQualifier **policyQualifiers =
                                          policyInfo->policyQualifiers;
                if (policyQualifiers) {
                        
                        PKIX_CHECK(PKIX_List_Create(&qualifiers, plContext),
                                PKIX_LISTCREATEFAILED);

                        while (*policyQualifiers != NULL) {
                            CERTPolicyQualifier *policyQualifier =
                                                         *policyQualifiers;

                            
                            PKIX_CHECK(PKIX_PL_OID_CreateBySECItem
                                (&policyQualifier->qualifierID,
                                 &pkixOID, plContext),
                                PKIX_OIDCREATEFAILED);

                            

                            PKIX_CHECK(PKIX_PL_ByteArray_Create
                                (policyQualifier->qualifierValue.data,
                                policyQualifier->qualifierValue.len,
                                &qualifierArray,
                                plContext),
                                PKIX_BYTEARRAYCREATEFAILED);

                            

                            PKIX_CHECK(pkix_pl_CertPolicyQualifier_Create
                                (pkixOID,
                                qualifierArray,
                                &certPolicyQualifier,
                                plContext),
                                PKIX_CERTPOLICYQUALIFIERCREATEFAILED);

                            PKIX_CHECK(PKIX_List_AppendItem
                                (qualifiers,
                                (PKIX_PL_Object *)certPolicyQualifier,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);

                            PKIX_DECREF(pkixOID);
                            PKIX_DECREF(qualifierArray);
                            PKIX_DECREF(certPolicyQualifier);

                            policyQualifiers++;
                        }

                        PKIX_CHECK(PKIX_List_SetImmutable
                                (qualifiers, plContext),
                                PKIX_LISTSETIMMUTABLEFAILED);
                }


                




                PKIX_CHECK(PKIX_PL_OID_CreateBySECItem
                        (&policyInfo->policyID, &pkixOID, plContext),
                        PKIX_OIDCREATEFAILED);

                
                PKIX_CHECK(pkix_pl_CertPolicyInfo_Create
                        (pkixOID, qualifiers, &certPolicyInfo, plContext),
                        PKIX_CERTPOLICYINFOCREATEFAILED);

                
                PKIX_CHECK(PKIX_List_AppendItem
                        (infos, (PKIX_PL_Object *)certPolicyInfo, plContext),
                        PKIX_LISTAPPENDITEMFAILED);

                PKIX_DECREF(pkixOID);
                PKIX_DECREF(qualifiers);
                PKIX_DECREF(certPolicyInfo);

                policyInfos++;
        }

        



        PKIX_CHECK(PKIX_List_SetImmutable(infos, plContext),
                PKIX_LISTSETIMMUTABLEFAILED);

        *pCertPolicyInfos = infos;
        infos = NULL;

cleanup:
        if (certPol) {
            PKIX_CERT_DEBUG
                ("\t\tCalling CERT_DestroyCertificatePoliciesExtension).\n");
            CERT_DestroyCertificatePoliciesExtension(certPol);
        }

        PKIX_DECREF(infos);
        PKIX_DECREF(pkixOID);
        PKIX_DECREF(qualifiers);
        PKIX_DECREF(certPolicyInfo);
        PKIX_DECREF(certPolicyQualifier);
        PKIX_DECREF(qualifierArray);

        PKIX_RETURN(CERT);
}



























static PKIX_Error *
pkix_pl_Cert_DecodePolicyMapping(
        CERTCertificate *nssCert,
        PKIX_List **pCertPolicyMaps,
        void *plContext)
{
        SECStatus rv;
        SECItem encodedCertPolicyMaps;

        
        CERTCertificatePolicyMappings *certPolMaps = NULL;
        CERTPolicyMap **policyMaps = NULL;

        
        PKIX_List *maps = NULL;

        PKIX_PL_OID *issuerDomainOID = NULL;
        PKIX_PL_OID *subjectDomainOID = NULL;
        PKIX_PL_CertPolicyMap *certPolicyMap = NULL;

        PKIX_ENTER(CERT, "pkix_pl_Cert_DecodePolicyMapping");
        PKIX_NULLCHECK_TWO(nssCert, pCertPolicyMaps);

        
        PKIX_CERT_DEBUG("\t\tCERT_FindCertExtension).\n");
        rv = CERT_FindCertExtension
                (nssCert, SEC_OID_X509_POLICY_MAPPINGS, &encodedCertPolicyMaps);
        if (SECSuccess != rv) {
                *pCertPolicyMaps = NULL;
                goto cleanup;
        }

        
        certPolMaps = CERT_DecodePolicyMappingsExtension
                (&encodedCertPolicyMaps);

        PORT_Free(encodedCertPolicyMaps.data);

        if (!certPolMaps) {
                PKIX_ERROR(PKIX_CERTDECODEPOLICYMAPPINGSEXTENSIONFAILED);
        }

        PKIX_NULLCHECK_ONE(certPolMaps->policyMaps);

        policyMaps = certPolMaps->policyMaps;

        
        PKIX_CHECK(PKIX_List_Create(&maps, plContext),
                PKIX_LISTCREATEFAILED);

        



        do {
                CERTPolicyMap *policyMap = *policyMaps;

                
                PKIX_CHECK(PKIX_PL_OID_CreateBySECItem
                        (&policyMap->issuerDomainPolicy,
                         &issuerDomainOID, plContext),
                        PKIX_OIDCREATEFAILED);

                
                PKIX_CHECK(PKIX_PL_OID_CreateBySECItem
                        (&policyMap->subjectDomainPolicy,
                         &subjectDomainOID, plContext),
                        PKIX_OIDCREATEFAILED);

                

                PKIX_CHECK(pkix_pl_CertPolicyMap_Create
                        (issuerDomainOID,
                        subjectDomainOID,
                        &certPolicyMap,
                        plContext),
                        PKIX_CERTPOLICYMAPCREATEFAILED);

                PKIX_CHECK(PKIX_List_AppendItem
                        (maps, (PKIX_PL_Object *)certPolicyMap, plContext),
                        PKIX_LISTAPPENDITEMFAILED);

                PKIX_DECREF(issuerDomainOID);
                PKIX_DECREF(subjectDomainOID);
                PKIX_DECREF(certPolicyMap);

                policyMaps++;
        } while (*policyMaps != NULL);

        PKIX_CHECK(PKIX_List_SetImmutable(maps, plContext),
                PKIX_LISTSETIMMUTABLEFAILED);

        *pCertPolicyMaps = maps;
        maps = NULL;

cleanup:
        if (certPolMaps) {
            PKIX_CERT_DEBUG
                ("\t\tCalling CERT_DestroyPolicyMappingsExtension).\n");
            CERT_DestroyPolicyMappingsExtension(certPolMaps);
        }

        PKIX_DECREF(maps);
        PKIX_DECREF(issuerDomainOID);
        PKIX_DECREF(subjectDomainOID);
        PKIX_DECREF(certPolicyMap);

        PKIX_RETURN(CERT);
}
































static PKIX_Error *
pkix_pl_Cert_DecodePolicyConstraints(
        CERTCertificate *nssCert,
        PKIX_Int32 *pExplicitPolicySkipCerts,
        PKIX_Int32 *pInhibitMappingSkipCerts,
        void *plContext)
{
        CERTCertificatePolicyConstraints policyConstraints;
        SECStatus rv;
        SECItem encodedCertPolicyConstraints;
        PKIX_Int32 explicitPolicySkipCerts = -1;
        PKIX_Int32 inhibitMappingSkipCerts = -1;

        PKIX_ENTER(CERT, "pkix_pl_Cert_DecodePolicyConstraints");
        PKIX_NULLCHECK_THREE
                (nssCert, pExplicitPolicySkipCerts, pInhibitMappingSkipCerts);

        
        PKIX_CERT_DEBUG("\t\tCalling CERT_FindCertExtension).\n");
        rv = CERT_FindCertExtension
                (nssCert,
                SEC_OID_X509_POLICY_CONSTRAINTS,
                &encodedCertPolicyConstraints);

        if (rv == SECSuccess) {

                policyConstraints.explicitPolicySkipCerts.data =
                        (unsigned char *)&explicitPolicySkipCerts;
                policyConstraints.inhibitMappingSkipCerts.data =
                        (unsigned char *)&inhibitMappingSkipCerts;

                
                rv = CERT_DecodePolicyConstraintsExtension
                        (&policyConstraints, &encodedCertPolicyConstraints);

                PORT_Free(encodedCertPolicyConstraints.data);

                if (rv != SECSuccess) {
                    PKIX_ERROR
                        (PKIX_CERTDECODEPOLICYCONSTRAINTSEXTENSIONFAILED);
                }
        }

        *pExplicitPolicySkipCerts = explicitPolicySkipCerts;
        *pInhibitMappingSkipCerts = inhibitMappingSkipCerts;

cleanup:
        PKIX_RETURN(CERT);
}


























PKIX_Error *
pkix_pl_Cert_DecodeInhibitAnyPolicy(
        CERTCertificate *nssCert,
        PKIX_Int32 *pSkipCerts,
        void *plContext)
{
        CERTCertificateInhibitAny inhibitAny;
        SECStatus rv;
        SECItem encodedCertInhibitAny;
        PKIX_Int32 skipCerts = -1;

        PKIX_ENTER(CERT, "pkix_pl_Cert_DecodeInhibitAnyPolicy");
        PKIX_NULLCHECK_TWO(nssCert, pSkipCerts);

        
        PKIX_CERT_DEBUG("\t\tCalling CERT_FindCertExtension).\n");
        rv = CERT_FindCertExtension
                (nssCert, SEC_OID_X509_INHIBIT_ANY_POLICY, &encodedCertInhibitAny);

        if (rv == SECSuccess) {
                inhibitAny.inhibitAnySkipCerts.data =
                        (unsigned char *)&skipCerts;

                
                rv = CERT_DecodeInhibitAnyExtension
                        (&inhibitAny, &encodedCertInhibitAny);

                PORT_Free(encodedCertInhibitAny.data);

                if (rv != SECSuccess) {
                        PKIX_ERROR(PKIX_CERTDECODEINHIBITANYEXTENSIONFAILED);
                }
        }

        *pSkipCerts = skipCerts;

cleanup:
        PKIX_RETURN(CERT);
}































static PKIX_Error *
pkix_pl_Cert_GetNssSubjectAltNames(
        PKIX_PL_Cert *cert,
        PKIX_Boolean hasLock,
        CERTGeneralName **pNssSubjAltNames,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        CERTGeneralName *nssOriginalAltName = NULL;
        PLArenaPool *arena = NULL;
        SECItem altNameExtension = {siBuffer, NULL, 0};
        SECStatus rv = SECFailure;

        PKIX_ENTER(CERT, "pkix_pl_Cert_GetNssSubjectAltNames");
        PKIX_NULLCHECK_THREE(cert, pNssSubjAltNames, cert->nssCert);

        nssCert = cert->nssCert;

        if ((cert->nssSubjAltNames == NULL) && (!cert->subjAltNamesAbsent)){

                if (!hasLock) {
                    PKIX_OBJECT_LOCK(cert);
                }

                if ((cert->nssSubjAltNames == NULL) &&
                    (!cert->subjAltNamesAbsent)){

                    PKIX_PL_NSSCALLRV(CERT, rv, CERT_FindCertExtension,
                        (nssCert,
                        SEC_OID_X509_SUBJECT_ALT_NAME,
                        &altNameExtension));

                    if (rv != SECSuccess) {
                        *pNssSubjAltNames = NULL;
                        cert->subjAltNamesAbsent = PKIX_TRUE;
                        goto cleanup;
                    }

                    if (cert->arenaNameConstraints == NULL) {
                        PKIX_PL_NSSCALLRV(CERT, arena, PORT_NewArena,
                                (DER_DEFAULT_CHUNKSIZE));

                        if (arena == NULL) {
                            PKIX_ERROR(PKIX_OUTOFMEMORY);
                        }
                        cert->arenaNameConstraints = arena;
                    }

                    PKIX_PL_NSSCALLRV
                        (CERT,
                        nssOriginalAltName,
                        (CERTGeneralName *) CERT_DecodeAltNameExtension,
                        (cert->arenaNameConstraints, &altNameExtension));

                    PKIX_PL_NSSCALL(CERT, PORT_Free, (altNameExtension.data));

                    if (nssOriginalAltName == NULL) {
                        PKIX_ERROR(PKIX_CERTDECODEALTNAMEEXTENSIONFAILED);
                    }
                    cert->nssSubjAltNames = nssOriginalAltName;

                }

                if (!hasLock) {
                    PKIX_OBJECT_UNLOCK(cert);
                }
        }

        *pNssSubjAltNames = cert->nssSubjAltNames;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}




























PKIX_Error *
pkix_pl_Cert_CheckExtendedKeyUsage(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 requiredExtendedKeyUsages,
        PKIX_Boolean *pPass,
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *basicConstraints = NULL;
        PKIX_UInt32 certType = 0;
        PKIX_UInt32 requiredKeyUsage = 0;
        PKIX_UInt32 requiredCertType = 0;
        PKIX_UInt32 requiredExtendedKeyUsage = 0;
        PKIX_UInt32 i;
        PKIX_Boolean isCA = PKIX_FALSE;
        SECStatus rv = SECFailure;

        PKIX_ENTER(CERT, "pkix_pl_Cert_CheckExtendKeyUsage");
        PKIX_NULLCHECK_THREE(cert, pPass, cert->nssCert);

        *pPass = PKIX_FALSE;

        PKIX_CERT_DEBUG("\t\tCalling cert_GetCertType).\n");
        cert_GetCertType(cert->nssCert);
        certType = cert->nssCert->nsCertType;

        PKIX_CHECK(PKIX_PL_Cert_GetBasicConstraints
                    (cert,
                    &basicConstraints,
                    plContext),
                    PKIX_CERTGETBASICCONSTRAINTFAILED);

        if (basicConstraints != NULL) {
                PKIX_CHECK(PKIX_PL_BasicConstraints_GetCAFlag
                    (basicConstraints, &isCA, plContext),
                    PKIX_BASICCONSTRAINTSGETCAFLAGFAILED);
        }

        i = 0;
        while (requiredExtendedKeyUsages != 0) {

                
                while (requiredExtendedKeyUsages != 0) {
                        if (((1 << i) & requiredExtendedKeyUsages) != 0) {
                                requiredExtendedKeyUsage = 1 << i;
                                break;
                        }
                        i++;
                }
                requiredExtendedKeyUsages ^= requiredExtendedKeyUsage;

                requiredExtendedKeyUsage = i;

                PKIX_PL_NSSCALLRV(CERT, rv, CERT_KeyUsageAndTypeForCertUsage,
                        (requiredExtendedKeyUsage,
                        isCA,
                        &requiredKeyUsage,
                        &requiredCertType));

                if (!(certType & requiredCertType)) {
                        goto cleanup;
                }

                PKIX_PL_NSSCALLRV(CERT, rv, CERT_CheckKeyUsage,
                        (cert->nssCert, requiredKeyUsage));
                if (rv != SECSuccess) {
                        goto cleanup;
                }
                i++;

        }

        *pPass = PKIX_TRUE;

cleanup:
        PKIX_DECREF(basicConstraints);
        PKIX_RETURN(CERT);
}



























PKIX_Error *
pkix_pl_Cert_ToString_Helper(
        PKIX_PL_Cert *cert,
        PKIX_Boolean partialString,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *certString = NULL;
        char *asciiFormat = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_UInt32 certVersion;
        PKIX_PL_BigInt *certSN = NULL;
        PKIX_PL_String *certSNString = NULL;
        PKIX_PL_X500Name *certIssuer = NULL;
        PKIX_PL_String *certIssuerString = NULL;
        PKIX_PL_X500Name *certSubject = NULL;
        PKIX_PL_String *certSubjectString = NULL;
        PKIX_PL_String *notBeforeString = NULL;
        PKIX_PL_String *notAfterString = NULL;
        PKIX_List *subjAltNames = NULL;
        PKIX_PL_String *subjAltNamesString = NULL;
        PKIX_PL_ByteArray *authKeyId = NULL;
        PKIX_PL_String *authKeyIdString = NULL;
        PKIX_PL_ByteArray *subjKeyId = NULL;
        PKIX_PL_String *subjKeyIdString = NULL;
        PKIX_PL_PublicKey *nssPubKey = NULL;
        PKIX_PL_String *nssPubKeyString = NULL;
        PKIX_List *critExtOIDs = NULL;
        PKIX_PL_String *critExtOIDsString = NULL;
        PKIX_List *extKeyUsages = NULL;
        PKIX_PL_String *extKeyUsagesString = NULL;
        PKIX_PL_CertBasicConstraints *basicConstraint = NULL;
        PKIX_PL_String *certBasicConstraintsString = NULL;
        PKIX_List *policyInfo = NULL;
        PKIX_PL_String *certPolicyInfoString = NULL;
        PKIX_List *certPolicyMappings = NULL;
        PKIX_PL_String *certPolicyMappingsString = NULL;
        PKIX_Int32 certExplicitPolicy = 0;
        PKIX_Int32 certInhibitMapping = 0;
        PKIX_Int32 certInhibitAnyPolicy = 0;
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;
        PKIX_PL_String *nameConstraintsString = NULL;
        PKIX_List *authorityInfoAccess = NULL;
        PKIX_PL_String *authorityInfoAccessString = NULL;
        PKIX_List *subjectInfoAccess = NULL;
        PKIX_PL_String *subjectInfoAccessString = NULL;

        PKIX_ENTER(CERT, "pkix_pl_Cert_ToString_Helper");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pString);

        



        if (partialString){
                asciiFormat =
                        "\t[Issuer:          %s\n"
                        "\t Subject:         %s]";
        } else {
                asciiFormat =
                        "[\n"
                        "\tVersion:         v%d\n"
                        "\tSerialNumber:    %s\n"
                        "\tIssuer:          %s\n"
                        "\tSubject:         %s\n"
                        "\tValidity: [From: %s\n"
                        "\t           To:   %s]\n"
                        "\tSubjectAltNames: %s\n"
                        "\tAuthorityKeyId:  %s\n"
                        "\tSubjectKeyId:    %s\n"
                        "\tSubjPubKeyAlgId: %s\n"
                        "\tCritExtOIDs:     %s\n"
                        "\tExtKeyUsages:    %s\n"
                        "\tBasicConstraint: %s\n"
                        "\tCertPolicyInfo:  %s\n"
                        "\tPolicyMappings:  %s\n"
                        "\tExplicitPolicy:  %d\n"
                        "\tInhibitMapping:  %d\n"
                        "\tInhibitAnyPolicy:%d\n"
                        "\tNameConstraints: %s\n"
                        "\tAuthorityInfoAccess: %s\n"
                        "\tSubjectInfoAccess: %s\n"
                        "\tCacheFlag:       %d\n"
                        "]\n";
        }



        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, asciiFormat, 0, &formatString, plContext),
                PKIX_STRINGCREATEFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetIssuer
                (cert, &certIssuer, plContext),
                PKIX_CERTGETISSUERFAILED);

        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object *)certIssuer, &certIssuerString, plContext),
                PKIX_X500NAMETOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetSubject(cert, &certSubject, plContext),
                PKIX_CERTGETSUBJECTFAILED);

        PKIX_TOSTRING(certSubject, &certSubjectString, plContext,
                PKIX_X500NAMETOSTRINGFAILED);

        if (partialString){
                PKIX_CHECK(PKIX_PL_Sprintf
                            (&certString,
                            plContext,
                            formatString,
                            certIssuerString,
                            certSubjectString),
                            PKIX_SPRINTFFAILED);

                *pString = certString;
                goto cleanup;
        }

        
        PKIX_CHECK(PKIX_PL_Cert_GetVersion(cert, &certVersion, plContext),
                PKIX_CERTGETVERSIONFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetSerialNumber(cert, &certSN, plContext),
                PKIX_CERTGETSERIALNUMBERFAILED);

        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object *)certSN, &certSNString, plContext),
                PKIX_BIGINTTOSTRINGFAILED);

        
        PKIX_CHECK(pkix_pl_Date_ToString_Helper
                (&(cert->nssCert->validity.notBefore),
                &notBeforeString,
                plContext),
                PKIX_DATETOSTRINGHELPERFAILED);

        
        PKIX_CHECK(pkix_pl_Date_ToString_Helper
                (&(cert->nssCert->validity.notAfter),
                &notAfterString,
                plContext),
                PKIX_DATETOSTRINGHELPERFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetSubjectAltNames
                (cert, &subjAltNames, plContext),
                PKIX_CERTGETSUBJECTALTNAMESFAILED);

        PKIX_TOSTRING(subjAltNames, &subjAltNamesString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetAuthorityKeyIdentifier
                (cert, &authKeyId, plContext),
                PKIX_CERTGETAUTHORITYKEYIDENTIFIERFAILED);

        PKIX_TOSTRING(authKeyId, &authKeyIdString, plContext,
                PKIX_BYTEARRAYTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetSubjectKeyIdentifier
                (cert, &subjKeyId, plContext),
                PKIX_CERTGETSUBJECTKEYIDENTIFIERFAILED);

        PKIX_TOSTRING(subjKeyId, &subjKeyIdString, plContext,
                PKIX_BYTEARRAYTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                    (cert, &nssPubKey, plContext),
                    PKIX_CERTGETSUBJECTPUBLICKEYFAILED);

        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object *)nssPubKey, &nssPubKeyString, plContext),
                PKIX_PUBLICKEYTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetCriticalExtensionOIDs
                (cert, &critExtOIDs, plContext),
                PKIX_CERTGETCRITICALEXTENSIONOIDSFAILED);

        PKIX_TOSTRING(critExtOIDs, &critExtOIDsString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetExtendedKeyUsage
                (cert, &extKeyUsages, plContext),
                PKIX_CERTGETEXTENDEDKEYUSAGEFAILED);

        PKIX_TOSTRING(extKeyUsages, &extKeyUsagesString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetBasicConstraints
                (cert, &basicConstraint, plContext),
                PKIX_CERTGETBASICCONSTRAINTSFAILED);

        PKIX_TOSTRING(basicConstraint, &certBasicConstraintsString, plContext,
                PKIX_CERTBASICCONSTRAINTSTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetPolicyInformation
                (cert, &policyInfo, plContext),
                PKIX_CERTGETPOLICYINFORMATIONFAILED);

        PKIX_TOSTRING(policyInfo, &certPolicyInfoString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetPolicyMappings
                (cert, &certPolicyMappings, plContext),
                PKIX_CERTGETPOLICYMAPPINGSFAILED);

        PKIX_TOSTRING(certPolicyMappings, &certPolicyMappingsString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetRequireExplicitPolicy
                (cert, &certExplicitPolicy, plContext),
                PKIX_CERTGETREQUIREEXPLICITPOLICYFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetPolicyMappingInhibited
                (cert, &certInhibitMapping, plContext),
                PKIX_CERTGETPOLICYMAPPINGINHIBITEDFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetInhibitAnyPolicy
                (cert, &certInhibitAnyPolicy, plContext),
                PKIX_CERTGETINHIBITANYPOLICYFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetNameConstraints
                (cert, &nameConstraints, plContext),
                PKIX_CERTGETNAMECONSTRAINTSFAILED);

        PKIX_TOSTRING(nameConstraints, &nameConstraintsString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetAuthorityInfoAccess
                (cert, &authorityInfoAccess, plContext),
                PKIX_CERTGETAUTHORITYINFOACCESSFAILED);

        PKIX_TOSTRING(authorityInfoAccess, &authorityInfoAccessString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        
        PKIX_CHECK(PKIX_PL_Cert_GetSubjectInfoAccess
                (cert, &subjectInfoAccess, plContext),
                PKIX_CERTGETSUBJECTINFOACCESSFAILED);

        PKIX_TOSTRING(subjectInfoAccess, &subjectInfoAccessString, plContext,
                PKIX_LISTTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                    (&certString,
                    plContext,
                    formatString,
                    certVersion + 1,
                    certSNString,
                    certIssuerString,
                    certSubjectString,
                    notBeforeString,
                    notAfterString,
                    subjAltNamesString,
                    authKeyIdString,
                    subjKeyIdString,
                    nssPubKeyString,
                    critExtOIDsString,
                    extKeyUsagesString,
                    certBasicConstraintsString,
                    certPolicyInfoString,
                    certPolicyMappingsString,
                    certExplicitPolicy,         
                    certInhibitMapping,         
                    certInhibitAnyPolicy,       
                    nameConstraintsString,
                    authorityInfoAccessString,
                    subjectInfoAccessString,
                    cert->cacheFlag),           
                    PKIX_SPRINTFFAILED);

        *pString = certString;

cleanup:
        PKIX_DECREF(certSN);
        PKIX_DECREF(certSNString);
        PKIX_DECREF(certIssuer);
        PKIX_DECREF(certIssuerString);
        PKIX_DECREF(certSubject);
        PKIX_DECREF(certSubjectString);
        PKIX_DECREF(notBeforeString);
        PKIX_DECREF(notAfterString);
        PKIX_DECREF(subjAltNames);
        PKIX_DECREF(subjAltNamesString);
        PKIX_DECREF(authKeyId);
        PKIX_DECREF(authKeyIdString);
        PKIX_DECREF(subjKeyId);
        PKIX_DECREF(subjKeyIdString);
        PKIX_DECREF(nssPubKey);
        PKIX_DECREF(nssPubKeyString);
        PKIX_DECREF(critExtOIDs);
        PKIX_DECREF(critExtOIDsString);
        PKIX_DECREF(extKeyUsages);
        PKIX_DECREF(extKeyUsagesString);
        PKIX_DECREF(basicConstraint);
        PKIX_DECREF(certBasicConstraintsString);
        PKIX_DECREF(policyInfo);
        PKIX_DECREF(certPolicyInfoString);
        PKIX_DECREF(certPolicyMappings);
        PKIX_DECREF(certPolicyMappingsString);
        PKIX_DECREF(nameConstraints);
        PKIX_DECREF(nameConstraintsString);
        PKIX_DECREF(authorityInfoAccess);
        PKIX_DECREF(authorityInfoAccessString);
        PKIX_DECREF(subjectInfoAccess);
        PKIX_DECREF(subjectInfoAccessString);
        PKIX_DECREF(formatString);

        PKIX_RETURN(CERT);
}





static PKIX_Error *
pkix_pl_Cert_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Cert *cert = NULL;

        PKIX_ENTER(CERT, "pkix_pl_Cert_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERT_TYPE, plContext),
                    PKIX_OBJECTNOTCERT);

        cert = (PKIX_PL_Cert*)object;

        PKIX_DECREF(cert->subject);
        PKIX_DECREF(cert->issuer);
        PKIX_DECREF(cert->subjAltNames);
        PKIX_DECREF(cert->publicKeyAlgId);
        PKIX_DECREF(cert->publicKey);
        PKIX_DECREF(cert->serialNumber);
        PKIX_DECREF(cert->critExtOids);
        PKIX_DECREF(cert->authKeyId);
        PKIX_DECREF(cert->subjKeyId);
        PKIX_DECREF(cert->extKeyUsages);
        PKIX_DECREF(cert->certBasicConstraints);
        PKIX_DECREF(cert->certPolicyInfos);
        PKIX_DECREF(cert->certPolicyMappings);
        PKIX_DECREF(cert->nameConstraints);
        PKIX_DECREF(cert->store);
        PKIX_DECREF(cert->authorityInfoAccess);
        PKIX_DECREF(cert->subjectInfoAccess);
        PKIX_DECREF(cert->crldpList);

        if (cert->arenaNameConstraints){
                
                PKIX_PL_NSSCALL(CERT, PORT_FreeArena,
                        (cert->arenaNameConstraints, PR_FALSE));

                cert->arenaNameConstraints = NULL;
                cert->nssSubjAltNames = NULL;
        }

        CERT_DestroyCertificate(cert->nssCert);
        cert->nssCert = NULL;

cleanup:
        PKIX_RETURN(CERT);
}





static PKIX_Error *
pkix_pl_Cert_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *certString = NULL;
        PKIX_PL_Cert *pkixCert = NULL;

        PKIX_ENTER(CERT, "pkix_pl_Cert_toString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERT_TYPE, plContext),
                    PKIX_OBJECTNOTCERT);

        pkixCert = (PKIX_PL_Cert *)object;

        PKIX_CHECK(pkix_pl_Cert_ToString_Helper
                    (pkixCert, PKIX_FALSE, &certString, plContext),
                    PKIX_CERTTOSTRINGHELPERFAILED);

        *pString = certString;

cleanup:
        PKIX_RETURN(CERT);
}





static PKIX_Error *
pkix_pl_Cert_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_Cert *pkixCert = NULL;
        CERTCertificate *nssCert = NULL;
        unsigned char *derBytes = NULL;
        PKIX_UInt32 derLength;
        PKIX_UInt32 certHash;

        PKIX_ENTER(CERT, "pkix_pl_Cert_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_CERT_TYPE, plContext),
                    PKIX_OBJECTNOTCERT);

        pkixCert = (PKIX_PL_Cert *)object;

        nssCert = pkixCert->nssCert;
        derBytes = (nssCert->derCert).data;
        derLength = (nssCert->derCert).len;

        PKIX_CHECK(pkix_hash(derBytes, derLength, &certHash, plContext),
                    PKIX_HASHFAILED);

        *pHashcode = certHash;

cleanup:
        PKIX_RETURN(CERT);
}






static PKIX_Error *
pkix_pl_Cert_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        CERTCertificate *firstCert = NULL;
        CERTCertificate *secondCert = NULL;
        PKIX_UInt32 secondType;
        PKIX_Boolean cmpResult;

        PKIX_ENTER(CERT, "pkix_pl_Cert_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType(firstObject, PKIX_CERT_TYPE, plContext),
                    PKIX_FIRSTOBJECTNOTCERT);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_CERT_TYPE) goto cleanup;

        firstCert = ((PKIX_PL_Cert *)firstObject)->nssCert;
        secondCert = ((PKIX_PL_Cert *)secondObject)->nssCert;

        PKIX_NULLCHECK_TWO(firstCert, secondCert);

        
        PKIX_CERT_DEBUG("\t\tCalling CERT_CompareCerts).\n");
        cmpResult = CERT_CompareCerts(firstCert, secondCert);

        *pResult = cmpResult;

cleanup:
        PKIX_RETURN(CERT);
}












PKIX_Error *
pkix_pl_Cert_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERT, "pkix_pl_Cert_RegisterSelf");

        entry.description = "Cert";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_Cert);
        entry.destructor = pkix_pl_Cert_Destroy;
        entry.equalsFunction = pkix_pl_Cert_Equals;
        entry.hashcodeFunction = pkix_pl_Cert_Hashcode;
        entry.toStringFunction = pkix_pl_Cert_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_CERT_TYPE] = entry;

        PKIX_RETURN(CERT);
}


























PKIX_Error *
pkix_pl_Cert_CreateWithNSSCert(
        CERTCertificate *nssCert,
        PKIX_PL_Cert **pCert,
        void *plContext)
{
        PKIX_PL_Cert *cert = NULL;

        PKIX_ENTER(CERT, "pkix_pl_Cert_CreateWithNSSCert");
        PKIX_NULLCHECK_TWO(pCert, nssCert);

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_CERT_TYPE,
                    sizeof (PKIX_PL_Cert),
                    (PKIX_PL_Object **)&cert,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        
        cert->nssCert = nssCert;

        
        









        cert->subject = NULL;
        cert->issuer = NULL;
        cert->subjAltNames = NULL;
        cert->subjAltNamesAbsent = PKIX_FALSE;
        cert->publicKeyAlgId = NULL;
        cert->publicKey = NULL;
        cert->serialNumber = NULL;
        cert->critExtOids = NULL;
        cert->subjKeyId = NULL;
        cert->subjKeyIdAbsent = PKIX_FALSE;
        cert->authKeyId = NULL;
        cert->authKeyIdAbsent = PKIX_FALSE;
        cert->extKeyUsages = NULL;
        cert->extKeyUsagesAbsent = PKIX_FALSE;
        cert->certBasicConstraints = NULL;
        cert->basicConstraintsAbsent = PKIX_FALSE;
        cert->certPolicyInfos = NULL;
        cert->policyInfoAbsent = PKIX_FALSE;
        cert->policyMappingsAbsent = PKIX_FALSE;
        cert->certPolicyMappings = NULL;
        cert->policyConstraintsProcessed = PKIX_FALSE;
        cert->policyConstraintsExplicitPolicySkipCerts = 0;
        cert->policyConstraintsInhibitMappingSkipCerts = 0;
        cert->inhibitAnyPolicyProcessed = PKIX_FALSE;
        cert->inhibitAnySkipCerts = 0;
        cert->nameConstraints = NULL;
        cert->nameConstraintsAbsent = PKIX_FALSE;
        cert->arenaNameConstraints = NULL;
        cert->nssSubjAltNames = NULL;
        cert->cacheFlag = PKIX_FALSE;
        cert->store = NULL;
        cert->authorityInfoAccess = NULL;
        cert->subjectInfoAccess = NULL;
        cert->isUserTrustAnchor = PKIX_FALSE;
        cert->crldpList = NULL;

        *pCert = cert;

cleanup:
        PKIX_RETURN(CERT);
}


























PKIX_Error *
pkix_pl_Cert_CreateToList(
        SECItem *derCertItem,
        PKIX_List *certList,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        PKIX_PL_Cert *cert = NULL;
        CERTCertDBHandle *handle;

        PKIX_ENTER(CERT, "pkix_pl_Cert_CreateToList");
        PKIX_NULLCHECK_TWO(derCertItem, certList);

        handle  = CERT_GetDefaultCertDB();
        nssCert = CERT_NewTempCertificate(handle, derCertItem,
					   NULL, 
					   PR_FALSE, 
					   PR_TRUE);
        if (!nssCert) {
            goto cleanup;
        }

        PKIX_CHECK(pkix_pl_Cert_CreateWithNSSCert
                   (nssCert, &cert, plContext),
                   PKIX_CERTCREATEWITHNSSCERTFAILED);

        nssCert = NULL;

        PKIX_CHECK(PKIX_List_AppendItem
                   (certList, (PKIX_PL_Object *) cert, plContext),
                   PKIX_LISTAPPENDITEMFAILED);

cleanup:
        if (nssCert) {
            CERT_DestroyCertificate(nssCert);
        }

        PKIX_DECREF(cert);
        PKIX_RETURN(CERT);
}








PKIX_Error *
PKIX_PL_Cert_Create(
        PKIX_PL_ByteArray *byteArray,
        PKIX_PL_Cert **pCert,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        SECItem *derCertItem = NULL;
        void *derBytes = NULL;
        PKIX_UInt32 derLength;
        PKIX_Boolean copyDER;
        PKIX_PL_Cert *cert = NULL;
        CERTCertDBHandle *handle;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_Create");
        PKIX_NULLCHECK_TWO(pCert, byteArray);

        PKIX_CHECK(PKIX_PL_ByteArray_GetPointer
                    (byteArray, &derBytes, plContext),
                    PKIX_BYTEARRAYGETPOINTERFAILED);

        PKIX_CHECK(PKIX_PL_ByteArray_GetLength
                    (byteArray, &derLength, plContext),
                    PKIX_BYTEARRAYGETLENGTHFAILED);

        derCertItem = SECITEM_AllocItem(NULL, NULL, derLength);
        if (derCertItem == NULL){
                PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        (void) PORT_Memcpy(derCertItem->data, derBytes, derLength);

        




        copyDER = PKIX_TRUE;
        handle  = CERT_GetDefaultCertDB();
        nssCert = CERT_NewTempCertificate(handle, derCertItem,
					   NULL, 
					   PR_FALSE, 
					   PR_TRUE);
        if (!nssCert){
                PKIX_ERROR(PKIX_CERTDECODEDERCERTIFICATEFAILED);
        }

        PKIX_CHECK(pkix_pl_Cert_CreateWithNSSCert
                (nssCert, &cert, plContext),
                PKIX_CERTCREATEWITHNSSCERTFAILED);

        *pCert = cert;

cleanup:
        if (derCertItem){
                SECITEM_FreeItem(derCertItem, PKIX_TRUE);
        }

        if (nssCert && PKIX_ERROR_RECEIVED){
                PKIX_CERT_DEBUG("\t\tCalling CERT_DestroyCertificate).\n");
                CERT_DestroyCertificate(nssCert);
                nssCert = NULL;
        }

        PKIX_FREE(derBytes);
        PKIX_RETURN(CERT);
}






PKIX_Error *
PKIX_PL_Cert_CreateFromCERTCertificate(
        const CERTCertificate *nssCert,
        PKIX_PL_Cert **pCert,
        void *plContext)
{
        void *buf = NULL;
        PKIX_UInt32 len;
        PKIX_PL_ByteArray *byteArray = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_CreateWithNssCert");
        PKIX_NULLCHECK_TWO(pCert, nssCert);

        buf = (void*)nssCert->derCert.data;
        len = nssCert->derCert.len;

        PKIX_CHECK(
            PKIX_PL_ByteArray_Create(buf, len, &byteArray, plContext),
            PKIX_BYTEARRAYCREATEFAILED);

        PKIX_CHECK(
            PKIX_PL_Cert_Create(byteArray, pCert, plContext),
            PKIX_CERTCREATEWITHNSSCERTFAILED);

#ifdef PKIX_UNDEF
        
        nssCert = CERT_DupCertificate(nssInCert);

        PKIX_CHECK(pkix_pl_Cert_CreateWithNSSCert
                (nssCert, &cert, plContext),
                PKIX_CERTCREATEWITHNSSCERTFAILED);
#endif 

cleanup:

#ifdef PKIX_UNDEF
        if (nssCert && PKIX_ERROR_RECEIVED){
                PKIX_CERT_DEBUG("\t\tCalling CERT_DestroyCertificate).\n");
                CERT_DestroyCertificate(nssCert);
                nssCert = NULL;
        }
#endif 

        PKIX_DECREF(byteArray);
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetVersion(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 *pVersion,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        PKIX_UInt32 myVersion = 1;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetVersion");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pVersion);

        nssCert = cert->nssCert;
        if (nssCert->version.data) {
                myVersion = *(nssCert->version.data);
        }

        if (myVersion > 2){
                PKIX_ERROR(PKIX_VERSIONVALUEMUSTBEV1V2ORV3);
        }

        *pVersion = myVersion;

cleanup:
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetSerialNumber(
        PKIX_PL_Cert *cert,
        PKIX_PL_BigInt **pSerialNumber,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        SECItem serialNumItem;
        PKIX_PL_BigInt *serialNumber = NULL;
        char *bytes = NULL;
        PKIX_UInt32 length;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetSerialNumber");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pSerialNumber);

        if (cert->serialNumber == NULL){

                PKIX_OBJECT_LOCK(cert);

                if (cert->serialNumber == NULL){

                        nssCert = cert->nssCert;
                        serialNumItem = nssCert->serialNumber;

                        length = serialNumItem.len;
                        bytes = (char *)serialNumItem.data;

                        PKIX_CHECK(pkix_pl_BigInt_CreateWithBytes
                                    (bytes, length, &serialNumber, plContext),
                                    PKIX_BIGINTCREATEWITHBYTESFAILED);

                        
                        cert->serialNumber = serialNumber;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->serialNumber);
        *pSerialNumber = cert->serialNumber;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetSubject(
        PKIX_PL_Cert *cert,
        PKIX_PL_X500Name **pCertSubject,
        void *plContext)
{
        PKIX_PL_X500Name *pkixSubject = NULL;
        CERTName *subjName = NULL;
        SECItem  *derSubjName = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetSubject");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pCertSubject);

        
        if (cert->subject == NULL){

                PKIX_OBJECT_LOCK(cert);

                if (cert->subject == NULL){

                        subjName = &cert->nssCert->subject;
                        derSubjName = &cert->nssCert->derSubject;

                        
                        if (derSubjName->data == NULL) {

                                pkixSubject = NULL;

                        } else {
                                PKIX_CHECK(PKIX_PL_X500Name_CreateFromCERTName
                                    (derSubjName, subjName, &pkixSubject,
                                     plContext),
                                    PKIX_X500NAMECREATEFROMCERTNAMEFAILED);

                        }
                        
                        cert->subject = pkixSubject;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->subject);
        *pCertSubject = cert->subject;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetIssuer(
        PKIX_PL_Cert *cert,
        PKIX_PL_X500Name **pCertIssuer,
        void *plContext)
{
        PKIX_PL_X500Name *pkixIssuer = NULL;
        SECItem  *derIssuerName = NULL;
        CERTName *issuerName = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetIssuer");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pCertIssuer);

        
        if (cert->issuer == NULL){

                PKIX_OBJECT_LOCK(cert);

                if (cert->issuer == NULL){

                        issuerName = &cert->nssCert->issuer;
                        derIssuerName = &cert->nssCert->derIssuer;

                        
                        PKIX_CHECK(PKIX_PL_X500Name_CreateFromCERTName
                                    (derIssuerName, issuerName,
                                     &pkixIssuer, plContext),
                                    PKIX_X500NAMECREATEFROMCERTNAMEFAILED);

                        
                        cert->issuer = pkixIssuer;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->issuer);
        *pCertIssuer = cert->issuer;

cleanup:
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetSubjectAltNames(
        PKIX_PL_Cert *cert,
        PKIX_List **pSubjectAltNames,  
        void *plContext)
{
        PKIX_PL_GeneralName *pkixAltName = NULL;
        PKIX_List *altNamesList = NULL;

        CERTGeneralName *nssOriginalAltName = NULL;
        CERTGeneralName *nssTempAltName = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetSubjectAltNames");
        PKIX_NULLCHECK_TWO(cert, pSubjectAltNames);

        
        if ((cert->subjAltNames == NULL) && (!cert->subjAltNamesAbsent)){

                PKIX_OBJECT_LOCK(cert);

                if ((cert->subjAltNames == NULL) &&
                    (!cert->subjAltNamesAbsent)){

                        PKIX_CHECK(pkix_pl_Cert_GetNssSubjectAltNames
                                (cert,
                                PKIX_TRUE,
                                &nssOriginalAltName,
                                plContext),
                                PKIX_CERTGETNSSSUBJECTALTNAMESFAILED);

                        if (nssOriginalAltName == NULL) {
                                cert->subjAltNamesAbsent = PKIX_TRUE;
                                pSubjectAltNames = NULL;
                                goto cleanup;
                        }

                        nssTempAltName = nssOriginalAltName;

                        PKIX_CHECK(PKIX_List_Create(&altNamesList, plContext),
                                PKIX_LISTCREATEFAILED);

                        do {
                            PKIX_CHECK(pkix_pl_GeneralName_Create
                                (nssTempAltName, &pkixAltName, plContext),
                                PKIX_GENERALNAMECREATEFAILED);

                            PKIX_CHECK(PKIX_List_AppendItem
                                (altNamesList,
                                (PKIX_PL_Object *)pkixAltName,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);

                            PKIX_DECREF(pkixAltName);

                            PKIX_CERT_DEBUG
                                ("\t\tCalling CERT_GetNextGeneralName).\n");
                            nssTempAltName = CERT_GetNextGeneralName
                                (nssTempAltName);

                        } while (nssTempAltName != nssOriginalAltName);

                        
                        cert->subjAltNames = altNamesList;
                        PKIX_CHECK(PKIX_List_SetImmutable
                                (cert->subjAltNames, plContext),
                                PKIX_LISTSETIMMUTABLEFAILED);

                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->subjAltNames);

        *pSubjectAltNames = cert->subjAltNames;

cleanup:
        PKIX_DECREF(pkixAltName);
        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(altNamesList);
        }
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetAllSubjectNames(
        PKIX_PL_Cert *cert,
        PKIX_List **pAllSubjectNames,  
        void *plContext)
{
        CERTGeneralName *nssOriginalSubjectName = NULL;
        CERTGeneralName *nssTempSubjectName = NULL;
        PKIX_List *allSubjectNames = NULL;
        PKIX_PL_GeneralName *pkixSubjectName = NULL;
        PLArenaPool *arena = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetAllSubjectNames");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pAllSubjectNames);


        if (cert->nssCert->subjectName == NULL){
                

                PKIX_CHECK(pkix_pl_Cert_GetNssSubjectAltNames
                            (cert,
                            PKIX_FALSE, 
                            &nssOriginalSubjectName,
                            plContext),
                            PKIX_CERTGETNSSSUBJECTALTNAMESFAILED);

        } else { 

                arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                if (arena == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                }

                
                PKIX_CERT_DEBUG("\t\tCalling CERT_GetCertificateNames\n");
                nssOriginalSubjectName =
                        CERT_GetCertificateNames(cert->nssCert, arena);
        }

        if (nssOriginalSubjectName == NULL) {
                pAllSubjectNames = NULL;
                goto cleanup;
        }

        nssTempSubjectName = nssOriginalSubjectName;

        PKIX_CHECK(PKIX_List_Create(&allSubjectNames, plContext),
                    PKIX_LISTCREATEFAILED);

        do {
                PKIX_CHECK(pkix_pl_GeneralName_Create
                            (nssTempSubjectName, &pkixSubjectName, plContext),
                            PKIX_GENERALNAMECREATEFAILED);

                PKIX_CHECK(PKIX_List_AppendItem
                            (allSubjectNames,
                            (PKIX_PL_Object *)pkixSubjectName,
                            plContext),
                            PKIX_LISTAPPENDITEMFAILED);

                PKIX_DECREF(pkixSubjectName);

                PKIX_CERT_DEBUG
                        ("\t\tCalling CERT_GetNextGeneralName).\n");
                nssTempSubjectName = CERT_GetNextGeneralName
                        (nssTempSubjectName);
        } while (nssTempSubjectName != nssOriginalSubjectName);

        *pAllSubjectNames = allSubjectNames;

cleanup:
        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(allSubjectNames);
        }

        if (arena){
                PORT_FreeArena(arena, PR_FALSE);
        }
        PKIX_DECREF(pkixSubjectName);
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetSubjectPublicKeyAlgId(
        PKIX_PL_Cert *cert,
        PKIX_PL_OID **pSubjKeyAlgId,
        void *plContext)
{
        PKIX_PL_OID *pubKeyAlgId = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetSubjectPublicKeyAlgId");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pSubjKeyAlgId);

        
        if (cert->publicKeyAlgId == NULL){
                PKIX_OBJECT_LOCK(cert);
                if (cert->publicKeyAlgId == NULL){
                        CERTCertificate *nssCert = cert->nssCert;
                        SECAlgorithmID *algorithm;
                        SECItem *algBytes;

                        algorithm = &nssCert->subjectPublicKeyInfo.algorithm;
                        algBytes = &algorithm->algorithm;
                        if (!algBytes->data || !algBytes->len) {
                            PKIX_ERROR_FATAL(PKIX_ALGORITHMBYTESLENGTH0);
                        }
                        PKIX_CHECK(PKIX_PL_OID_CreateBySECItem
                                    (algBytes, &pubKeyAlgId, plContext),
                                    PKIX_OIDCREATEFAILED);

                        
                        cert->publicKeyAlgId = pubKeyAlgId;
                        pubKeyAlgId = NULL;
                }
                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->publicKeyAlgId);
        *pSubjKeyAlgId = cert->publicKeyAlgId;

cleanup:
        PKIX_DECREF(pubKeyAlgId);
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetSubjectPublicKey(
        PKIX_PL_Cert *cert,
        PKIX_PL_PublicKey **pPublicKey,
        void *plContext)
{
        PKIX_PL_PublicKey *pkixPubKey = NULL;
        SECStatus rv;

        CERTSubjectPublicKeyInfo *from = NULL;
        CERTSubjectPublicKeyInfo *to = NULL;
        SECItem *fromItem = NULL;
        SECItem *toItem = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetSubjectPublicKey");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pPublicKey);

        
        if (cert->publicKey == NULL){

                PKIX_OBJECT_LOCK(cert);

                if (cert->publicKey == NULL){

                        
                        PKIX_CHECK(PKIX_PL_Object_Alloc
                                    (PKIX_PUBLICKEY_TYPE,
                                    sizeof (PKIX_PL_PublicKey),
                                    (PKIX_PL_Object **)&pkixPubKey,
                                    plContext),
                                    PKIX_COULDNOTCREATEOBJECT);

                        
                        pkixPubKey->nssSPKI = NULL;

                        
                        PKIX_CHECK(PKIX_PL_Malloc
                                    (sizeof (CERTSubjectPublicKeyInfo),
                                    (void **)&pkixPubKey->nssSPKI,
                                    plContext),
                                    PKIX_MALLOCFAILED);

                        to = pkixPubKey->nssSPKI;
                        from  = &cert->nssCert->subjectPublicKeyInfo;

                        PKIX_NULLCHECK_TWO(to, from);

                        PKIX_CERT_DEBUG
                                ("\t\tCalling SECOID_CopyAlgorithmID).\n");
                        rv = SECOID_CopyAlgorithmID
                                (NULL, &to->algorithm, &from->algorithm);
                        if (rv != SECSuccess) {
                                PKIX_ERROR(PKIX_SECOIDCOPYALGORITHMIDFAILED);
                        }

                        







                        toItem = &to->subjectPublicKey;
                        fromItem = &from->subjectPublicKey;

                        PKIX_NULLCHECK_TWO(toItem, fromItem);

                        toItem->type = fromItem->type;

                        toItem->data =
                                (unsigned char*) PORT_ZAlloc(fromItem->len);
                        if (!toItem->data){
                                PKIX_ERROR(PKIX_OUTOFMEMORY);
                        }

                        (void) PORT_Memcpy(toItem->data,
                                    fromItem->data,
                                    (fromItem->len + 7)>>3);
                        toItem->len = fromItem->len;

                        
                        cert->publicKey = pkixPubKey;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->publicKey);
        *pPublicKey = cert->publicKey;

cleanup:

        if (PKIX_ERROR_RECEIVED && pkixPubKey){
                PKIX_DECREF(pkixPubKey);
                cert->publicKey = NULL;
        }
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetCriticalExtensionOIDs(
        PKIX_PL_Cert *cert,
        PKIX_List **pList,  
        void *plContext)
{
        PKIX_List *oidsList = NULL;
        CERTCertExtension **extensions = NULL;
        CERTCertificate *nssCert = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetCriticalExtensionOIDs");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pList);

        
        if (cert->critExtOids == NULL) {

            PKIX_OBJECT_LOCK(cert);

            if (cert->critExtOids == NULL) {

                nssCert = cert->nssCert;

                









                extensions = nssCert->extensions;

                PKIX_CHECK(pkix_pl_OID_GetCriticalExtensionOIDs
                            (extensions, &oidsList, plContext),
                            PKIX_GETCRITICALEXTENSIONOIDSFAILED);

                
                cert->critExtOids = oidsList;
            }

            PKIX_OBJECT_UNLOCK(cert);
        }

        
        PKIX_DUPLICATE(cert->critExtOids, pList, plContext,
                PKIX_OBJECTDUPLICATELISTFAILED);

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetAuthorityKeyIdentifier(
        PKIX_PL_Cert *cert,
        PKIX_PL_ByteArray **pAuthKeyId,
        void *plContext)
{
        PKIX_PL_ByteArray *authKeyId = NULL;
        CERTCertificate *nssCert = NULL;
        CERTAuthKeyID *authKeyIdExtension = NULL;
        PLArenaPool *arena = NULL;
        SECItem retItem;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetAuthorityKeyIdentifier");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pAuthKeyId);

        
        if ((cert->authKeyId == NULL) && (!cert->authKeyIdAbsent)){

                PKIX_OBJECT_LOCK(cert);

                if ((cert->authKeyId == NULL) && (!cert->authKeyIdAbsent)){

                        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                        if (arena == NULL) {
                                PKIX_ERROR(PKIX_OUTOFMEMORY);
                        }

                        nssCert = cert->nssCert;

                        authKeyIdExtension =
                                CERT_FindAuthKeyIDExten(arena, nssCert);
                        if (authKeyIdExtension == NULL){
                                cert->authKeyIdAbsent = PKIX_TRUE;
                                *pAuthKeyId = NULL;
                                goto cleanup;
                        }

                        retItem = authKeyIdExtension->keyID;

                        if (retItem.len == 0){
                                cert->authKeyIdAbsent = PKIX_TRUE;
                                *pAuthKeyId = NULL;
                                goto cleanup;
                        }

                        PKIX_CHECK(PKIX_PL_ByteArray_Create
                                    (retItem.data,
                                    retItem.len,
                                    &authKeyId,
                                    plContext),
                                    PKIX_BYTEARRAYCREATEFAILED);

                        
                        cert->authKeyId = authKeyId;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->authKeyId);
        *pAuthKeyId = cert->authKeyId;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        if (arena){
                PORT_FreeArena(arena, PR_FALSE);
        }
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetSubjectKeyIdentifier(
        PKIX_PL_Cert *cert,
        PKIX_PL_ByteArray **pSubjKeyId,
        void *plContext)
{
        PKIX_PL_ByteArray *subjKeyId = NULL;
        CERTCertificate *nssCert = NULL;
        SECItem *retItem = NULL;
        SECStatus status;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetSubjectKeyIdentifier");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pSubjKeyId);

        
        if ((cert->subjKeyId == NULL) && (!cert->subjKeyIdAbsent)){

                PKIX_OBJECT_LOCK(cert);

                if ((cert->subjKeyId == NULL) && (!cert->subjKeyIdAbsent)){

                        retItem = SECITEM_AllocItem(NULL, NULL, 0);
                        if (retItem == NULL){
                                PKIX_ERROR(PKIX_OUTOFMEMORY);
                        }

                        nssCert = cert->nssCert;

                        status = CERT_FindSubjectKeyIDExtension
                                (nssCert, retItem);
                        if (status != SECSuccess) {
                                cert->subjKeyIdAbsent = PKIX_TRUE;
                                *pSubjKeyId = NULL;
                                goto cleanup;
                        }

                        PKIX_CHECK(PKIX_PL_ByteArray_Create
                                    (retItem->data,
                                    retItem->len,
                                    &subjKeyId,
                                    plContext),
                                    PKIX_BYTEARRAYCREATEFAILED);

                        
                        cert->subjKeyId = subjKeyId;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->subjKeyId);
        *pSubjKeyId = cert->subjKeyId;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        if (retItem){
                SECITEM_FreeItem(retItem, PKIX_TRUE);
        }
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetExtendedKeyUsage(
        PKIX_PL_Cert *cert,
        PKIX_List **pKeyUsage,  
        void *plContext)
{
        CERTOidSequence *extKeyUsage = NULL;
        CERTCertificate *nssCert = NULL;
        PKIX_PL_OID *pkixOID = NULL;
        PKIX_List *oidsList = NULL;
        SECItem **oids = NULL;
        SECItem encodedExtKeyUsage;
        SECStatus rv;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetExtendedKeyUsage");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pKeyUsage);

        
        if ((cert->extKeyUsages == NULL) && (!cert->extKeyUsagesAbsent)){

                PKIX_OBJECT_LOCK(cert);

                if ((cert->extKeyUsages == NULL) &&
                    (!cert->extKeyUsagesAbsent)){

                        nssCert = cert->nssCert;

                        rv = CERT_FindCertExtension
                                (nssCert, SEC_OID_X509_EXT_KEY_USAGE,
                                &encodedExtKeyUsage);
                        if (rv != SECSuccess){
                                cert->extKeyUsagesAbsent = PKIX_TRUE;
                                *pKeyUsage = NULL;
                                goto cleanup;
                        }

                        extKeyUsage =
                                CERT_DecodeOidSequence(&encodedExtKeyUsage);
                        if (extKeyUsage == NULL){
                                PKIX_ERROR(PKIX_CERTDECODEOIDSEQUENCEFAILED);
                        }

                        PORT_Free(encodedExtKeyUsage.data);

                        oids = extKeyUsage->oids;

                        if (!oids){
                                
                                cert->extKeyUsagesAbsent = PKIX_TRUE;
                                *pKeyUsage = NULL;
                                goto cleanup;
                        }

                        PKIX_CHECK(PKIX_List_Create(&oidsList, plContext),
                                    PKIX_LISTCREATEFAILED);

                        while (*oids){
                                SECItem *oid = *oids++;

                                PKIX_CHECK(PKIX_PL_OID_CreateBySECItem
                                            (oid, &pkixOID, plContext),
                                            PKIX_OIDCREATEFAILED);

                                PKIX_CHECK(PKIX_List_AppendItem
                                            (oidsList,
                                            (PKIX_PL_Object *)pkixOID,
                                            plContext),
                                            PKIX_LISTAPPENDITEMFAILED);
                                PKIX_DECREF(pkixOID);
                        }

                        
                        cert->extKeyUsages = oidsList;
                        oidsList = NULL;
                }

                PKIX_CHECK(PKIX_List_SetImmutable
                            (cert->extKeyUsages, plContext),
                            PKIX_LISTSETIMMUTABLEFAILED);

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->extKeyUsages);
        *pKeyUsage = cert->extKeyUsages;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);

        PKIX_DECREF(pkixOID);
        PKIX_DECREF(oidsList);
        CERT_DestroyOidSequence(extKeyUsage);

        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetBasicConstraints(
        PKIX_PL_Cert *cert,
        PKIX_PL_CertBasicConstraints **pBasicConstraints,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        CERTBasicConstraints nssBasicConstraint;
        SECStatus rv;
        PKIX_PL_CertBasicConstraints *basic;
        PKIX_Int32 pathLen = 0;
        PKIX_Boolean isCA = PKIX_FALSE;
        enum {
          realBC, synthBC, absentBC
        } constraintSource = absentBC;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetBasicConstraints");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pBasicConstraints);

        
        if ((cert->certBasicConstraints == NULL) &&
                (!cert->basicConstraintsAbsent)) {

                PKIX_OBJECT_LOCK(cert);

                if ((cert->certBasicConstraints == NULL) &&
                    (!cert->basicConstraintsAbsent)) {

                        nssCert = cert->nssCert;

                        PKIX_CERT_DEBUG(
                            "\t\tCalling Cert_FindBasicConstraintExten\n");
                        rv = CERT_FindBasicConstraintExten
                                (nssCert, &nssBasicConstraint);
                        if (rv == SECSuccess) {
                            constraintSource = realBC;
                        }

                        if (constraintSource == absentBC) {
                            


                            CERTCertTrust trust;
                            rv = CERT_GetCertTrust(nssCert, &trust);
                            if (rv == SECSuccess) {
                                int anyWantedFlag = CERTDB_TRUSTED_CA | CERTDB_VALID_CA;
                                if ((trust.sslFlags & anyWantedFlag) 
                                    || (trust.emailFlags & anyWantedFlag) 
                                    || (trust.objectSigningFlags & anyWantedFlag)) {

                                    constraintSource = synthBC;
                                }
                            }
                        }

                        if (constraintSource == absentBC) {
                            cert->basicConstraintsAbsent = PKIX_TRUE;
                            *pBasicConstraints = NULL;
                            goto cleanup;
                        }
                }

                if (constraintSource == synthBC) {
                    isCA = PKIX_TRUE;
                    pathLen = PKIX_UNLIMITED_PATH_CONSTRAINT;
                } else {
                    isCA = (nssBasicConstraint.isCA)?PKIX_TRUE:PKIX_FALSE;
    
                    
                    if (isCA) {
                        if (CERT_UNLIMITED_PATH_CONSTRAINT ==
                            nssBasicConstraint.pathLenConstraint) {
                            pathLen = PKIX_UNLIMITED_PATH_CONSTRAINT;
                        } else {
                            pathLen = nssBasicConstraint.pathLenConstraint;
                        }
                    }
                }

                PKIX_CHECK(pkix_pl_CertBasicConstraints_Create
                            (isCA, pathLen, &basic, plContext),
                            PKIX_CERTBASICCONSTRAINTSCREATEFAILED);

                
                cert->certBasicConstraints = basic;
        }

        PKIX_INCREF(cert->certBasicConstraints);
        *pBasicConstraints = cert->certBasicConstraints;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetPolicyInformation(
        PKIX_PL_Cert *cert,
        PKIX_List **pPolicyInfo,
        void *plContext)
{
        PKIX_List *policyList = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetPolicyInformation");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pPolicyInfo);

        
        if ((cert->certPolicyInfos == NULL) &&
                (!cert->policyInfoAbsent)) {

                PKIX_OBJECT_LOCK(cert);

                if ((cert->certPolicyInfos == NULL) &&
                    (!cert->policyInfoAbsent)) {

                        PKIX_CHECK(pkix_pl_Cert_DecodePolicyInfo
                                (cert->nssCert, &policyList, plContext),
                                PKIX_CERTDECODEPOLICYINFOFAILED);

                        if (!policyList) {
                                cert->policyInfoAbsent = PKIX_TRUE;
                                *pPolicyInfo = NULL;
                                goto cleanup;
                        }
                }

                PKIX_OBJECT_UNLOCK(cert);

                
                cert->certPolicyInfos = policyList;
                policyList = NULL;
        }

        PKIX_INCREF(cert->certPolicyInfos);
        *pPolicyInfo = cert->certPolicyInfos;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);

        PKIX_DECREF(policyList);
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetPolicyMappings(
        PKIX_PL_Cert *cert,
        PKIX_List **pPolicyMappings, 
        void *plContext)
{
        PKIX_List *policyMappings = NULL; 

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetPolicyMappings");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pPolicyMappings);

        
        if (!(cert->certPolicyMappings) && !(cert->policyMappingsAbsent)) {

                PKIX_OBJECT_LOCK(cert);

                if (!(cert->certPolicyMappings) &&
                    !(cert->policyMappingsAbsent)) {

                        PKIX_CHECK(pkix_pl_Cert_DecodePolicyMapping
                                (cert->nssCert, &policyMappings, plContext),
                                PKIX_CERTDECODEPOLICYMAPPINGFAILED);

                        if (!policyMappings) {
                                cert->policyMappingsAbsent = PKIX_TRUE;
                                *pPolicyMappings = NULL;
                                goto cleanup;
                        }
                }

                PKIX_OBJECT_UNLOCK(cert);

                
                cert->certPolicyMappings = policyMappings; 
                policyMappings = NULL;
        }

        PKIX_INCREF(cert->certPolicyMappings);
        *pPolicyMappings = cert->certPolicyMappings;
        
cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);

        PKIX_DECREF(policyMappings);
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetRequireExplicitPolicy(
        PKIX_PL_Cert *cert,
        PKIX_Int32 *pSkipCerts,
        void *plContext)
{
        PKIX_Int32 explicitPolicySkipCerts = 0;
        PKIX_Int32 inhibitMappingSkipCerts = 0;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetRequireExplicitPolicy");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pSkipCerts);

        if (!(cert->policyConstraintsProcessed)) {
                PKIX_OBJECT_LOCK(cert);

                if (!(cert->policyConstraintsProcessed)) {

                        



                        cert->policyConstraintsProcessed = PKIX_TRUE;
                        cert->policyConstraintsExplicitPolicySkipCerts = -1;
                        cert->policyConstraintsInhibitMappingSkipCerts = -1;

                        PKIX_CHECK(pkix_pl_Cert_DecodePolicyConstraints
                                (cert->nssCert,
                                &explicitPolicySkipCerts,
                                &inhibitMappingSkipCerts,
                                plContext),
                                PKIX_CERTDECODEPOLICYCONSTRAINTSFAILED);

                        cert->policyConstraintsExplicitPolicySkipCerts =
                                explicitPolicySkipCerts;
                        cert->policyConstraintsInhibitMappingSkipCerts =
                                inhibitMappingSkipCerts;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        *pSkipCerts = cert->policyConstraintsExplicitPolicySkipCerts;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetPolicyMappingInhibited(
        PKIX_PL_Cert *cert,
        PKIX_Int32 *pSkipCerts,
        void *plContext)
{
        PKIX_Int32 explicitPolicySkipCerts = 0;
        PKIX_Int32 inhibitMappingSkipCerts = 0;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetPolicyMappingInhibited");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pSkipCerts);

        if (!(cert->policyConstraintsProcessed)) {
                PKIX_OBJECT_LOCK(cert);

                if (!(cert->policyConstraintsProcessed)) {

                        



                        cert->policyConstraintsProcessed = PKIX_TRUE;
                        cert->policyConstraintsExplicitPolicySkipCerts = -1;
                        cert->policyConstraintsInhibitMappingSkipCerts = -1;

                        PKIX_CHECK(pkix_pl_Cert_DecodePolicyConstraints
                                (cert->nssCert,
                                &explicitPolicySkipCerts,
                                &inhibitMappingSkipCerts,
                                plContext),
                                PKIX_CERTDECODEPOLICYCONSTRAINTSFAILED);

                        cert->policyConstraintsExplicitPolicySkipCerts =
                                explicitPolicySkipCerts;
                        cert->policyConstraintsInhibitMappingSkipCerts =
                                inhibitMappingSkipCerts;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        *pSkipCerts = cert->policyConstraintsInhibitMappingSkipCerts;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetInhibitAnyPolicy(
        PKIX_PL_Cert *cert,
        PKIX_Int32 *pSkipCerts,
        void *plContext)
{
        PKIX_Int32 skipCerts = 0;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetInhibitAnyPolicy");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pSkipCerts);

        if (!(cert->inhibitAnyPolicyProcessed)) {

                PKIX_OBJECT_LOCK(cert);

                if (!(cert->inhibitAnyPolicyProcessed)) {

                        



                        cert->inhibitAnyPolicyProcessed = PKIX_TRUE;
                        cert->inhibitAnySkipCerts = -1;

                        PKIX_CHECK(pkix_pl_Cert_DecodeInhibitAnyPolicy
                                (cert->nssCert, &skipCerts, plContext),
                                PKIX_CERTDECODEINHIBITANYPOLICYFAILED);

                        cert->inhibitAnySkipCerts = skipCerts;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        *pSkipCerts = cert->inhibitAnySkipCerts;
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_AreCertPoliciesCritical(
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pCritical,
        void *plContext)
{
        PKIX_Boolean criticality = PKIX_FALSE;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_AreCertPoliciesCritical");
        PKIX_NULLCHECK_TWO(cert, pCritical);

        PKIX_CHECK(pkix_pl_Cert_IsExtensionCritical(
                cert,
                SEC_OID_X509_CERTIFICATE_POLICIES,
                &criticality,
                plContext),
                PKIX_CERTISEXTENSIONCRITICALFAILED);

        *pCritical = criticality;

cleanup:
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_VerifySignature(
        PKIX_PL_Cert *cert,
        PKIX_PL_PublicKey *pubKey,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        SECKEYPublicKey *nssPubKey = NULL;
        CERTSignedData *tbsCert = NULL;
        PKIX_PL_Cert *cachedCert = NULL;
        PKIX_Error *verifySig = NULL;
        PKIX_Error *cachedSig = NULL;
        SECStatus status;
        PKIX_Boolean certEqual = PKIX_FALSE;
        PKIX_Boolean certInHash = PKIX_FALSE;
        void* wincx = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_VerifySignature");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pubKey);

        verifySig = PKIX_PL_HashTable_Lookup
                        (cachedCertSigTable,
                        (PKIX_PL_Object *) pubKey,
                        (PKIX_PL_Object **) &cachedCert,
                        plContext);

        if (cachedCert != NULL && verifySig == NULL) {
                
                PKIX_EQUALS(cert, cachedCert, &certEqual, plContext,
                            PKIX_OBJECTEQUALSFAILED);
                if (certEqual == PKIX_TRUE) {
                        goto cleanup;
                }
                
                certInHash = PKIX_TRUE;
        }

        nssCert = cert->nssCert;
        tbsCert = &nssCert->signatureWrap;

        PKIX_CERT_DEBUG("\t\tCalling SECKEY_ExtractPublicKey).\n");
        nssPubKey = SECKEY_ExtractPublicKey(pubKey->nssSPKI);
        if (!nssPubKey){
                PKIX_ERROR(PKIX_SECKEYEXTRACTPUBLICKEYFAILED);
        }

        PKIX_CERT_DEBUG("\t\tCalling CERT_VerifySignedDataWithPublicKey).\n");

        PKIX_CHECK(pkix_pl_NssContext_GetWincx
                   ((PKIX_PL_NssContext *)plContext, &wincx),
                   PKIX_NSSCONTEXTGETWINCXFAILED);

        status = CERT_VerifySignedDataWithPublicKey(tbsCert, nssPubKey, wincx);

        if (status != SECSuccess) {
                PKIX_ERROR(PKIX_SIGNATUREDIDNOTVERIFYWITHTHEPUBLICKEY);
        }

        if (certInHash == PKIX_FALSE) {
                cachedSig = PKIX_PL_HashTable_Add
                        (cachedCertSigTable,
                        (PKIX_PL_Object *) pubKey,
                        (PKIX_PL_Object *) cert,
                        plContext);

                if (cachedSig != NULL) {
                        PKIX_DEBUG("PKIX_PL_HashTable_Add skipped: entry existed\n");
                }
        }

cleanup:
        if (nssPubKey){
                PKIX_CERT_DEBUG("\t\tCalling SECKEY_DestroyPublicKey).\n");
                SECKEY_DestroyPublicKey(nssPubKey);
        }

        PKIX_DECREF(cachedCert);
        PKIX_DECREF(verifySig);
        PKIX_DECREF(cachedSig);

        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_CheckValidity(
        PKIX_PL_Cert *cert,
        PKIX_PL_Date *date,
        void *plContext)
{
        SECCertTimeValidity val;
        PRTime timeToCheck;
        PKIX_Boolean allowOverride;
        SECCertificateUsage requiredUsages;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_CheckValidity");
        PKIX_NULLCHECK_ONE(cert);

        
        if (date != NULL){
                PKIX_CHECK(pkix_pl_Date_GetPRTime
                        (date, &timeToCheck, plContext),
                        PKIX_DATEGETPRTIMEFAILED);
        } else {
                timeToCheck = PR_Now();
        }

        requiredUsages = ((PKIX_PL_NssContext*)plContext)->certificateUsage;
        allowOverride =
            (PRBool)((requiredUsages & certificateUsageSSLServer) ||
                     (requiredUsages & certificateUsageSSLServerWithStepUp));
        val = CERT_CheckCertValidTimes(cert->nssCert, timeToCheck, allowOverride);
        if (val != secCertTimeValid){
                PKIX_ERROR(PKIX_CERTCHECKCERTVALIDTIMESFAILED);
        }

cleanup:
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetValidityNotAfter(
        PKIX_PL_Cert *cert,
        PKIX_PL_Date **pDate,
        void *plContext)
{
        PRTime prtime;
        SECStatus rv = SECFailure;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetValidityNotAfter");
        PKIX_NULLCHECK_TWO(cert, pDate);

        PKIX_DATE_DEBUG("\t\tCalling DER_DecodeTimeChoice).\n");
        rv = DER_DecodeTimeChoice(&prtime, &(cert->nssCert->validity.notAfter));
        if (rv != SECSuccess){
                PKIX_ERROR(PKIX_DERDECODETIMECHOICEFAILED);
        }

        PKIX_CHECK(pkix_pl_Date_CreateFromPRTime
                    (prtime, pDate, plContext),
                    PKIX_DATECREATEFROMPRTIMEFAILED);

cleanup:
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_VerifyCertAndKeyType(
        PKIX_PL_Cert *cert,
        PKIX_Boolean isChainCert,
        void *plContext)
{
    PKIX_PL_CertBasicConstraints *basicConstraints = NULL;
    SECCertificateUsage certificateUsage;
    SECCertUsage certUsage = 0;
    unsigned int requiredKeyUsage;
    unsigned int requiredCertType;
    unsigned int certType;
    SECStatus rv = SECSuccess;
    
    PKIX_ENTER(CERT, "PKIX_PL_Cert_VerifyCertType");
    PKIX_NULLCHECK_TWO(cert, plContext);
    
    certificateUsage = ((PKIX_PL_NssContext*)plContext)->certificateUsage;
    
    
    PORT_Assert(!(certificateUsage & (certificateUsage - 1)));
    
    
    while (0 != (certificateUsage = certificateUsage >> 1)) { certUsage++; }

    
    cert_GetCertType(cert->nssCert);
    certType = cert->nssCert->nsCertType;
    if (isChainCert ||
        (certUsage != certUsageVerifyCA && certUsage != certUsageAnyCA)) {
	rv = CERT_KeyUsageAndTypeForCertUsage(certUsage, isChainCert,
					      &requiredKeyUsage,
					      &requiredCertType);
        if (rv == SECFailure) {
            PKIX_ERROR(PKIX_UNSUPPORTEDCERTUSAGE);
        }
    } else {
        

	requiredKeyUsage = KU_KEY_CERT_SIGN;
	requiredCertType = NS_CERT_TYPE_CA;
    }
    if (CERT_CheckKeyUsage(cert->nssCert, requiredKeyUsage) != SECSuccess) {
        PKIX_ERROR(PKIX_CERTCHECKKEYUSAGEFAILED);
    }
    if (!(certType & requiredCertType)) {
        PKIX_ERROR(PKIX_CERTCHECKCERTTYPEFAILED);
    }
cleanup:
    PKIX_DECREF(basicConstraints);
    PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_VerifyKeyUsage(
        PKIX_PL_Cert *cert,
        PKIX_UInt32 keyUsage,
        void *plContext)
{
        CERTCertificate *nssCert = NULL;
        PKIX_UInt32 nssKeyUsage = 0;
        SECStatus status;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_VerifyKeyUsage");
        PKIX_NULLCHECK_TWO(cert, cert->nssCert);

        nssCert = cert->nssCert;

        
        if (!nssCert->keyUsagePresent){
                goto cleanup;
        }

        if (keyUsage & PKIX_DIGITAL_SIGNATURE){
                nssKeyUsage = nssKeyUsage | KU_DIGITAL_SIGNATURE;
        }

        if (keyUsage & PKIX_NON_REPUDIATION){
                nssKeyUsage = nssKeyUsage | KU_NON_REPUDIATION;
        }

        if (keyUsage & PKIX_KEY_ENCIPHERMENT){
                nssKeyUsage = nssKeyUsage | KU_KEY_ENCIPHERMENT;
        }

        if (keyUsage & PKIX_DATA_ENCIPHERMENT){
                nssKeyUsage = nssKeyUsage | KU_DATA_ENCIPHERMENT;
        }

        if (keyUsage & PKIX_KEY_AGREEMENT){
                nssKeyUsage = nssKeyUsage | KU_KEY_AGREEMENT;
        }

        if (keyUsage & PKIX_KEY_CERT_SIGN){
                nssKeyUsage = nssKeyUsage | KU_KEY_CERT_SIGN;
        }

        if (keyUsage & PKIX_CRL_SIGN){
                nssKeyUsage = nssKeyUsage | KU_CRL_SIGN;
        }

        if (keyUsage & PKIX_ENCIPHER_ONLY){
                nssKeyUsage = nssKeyUsage | 0x01;
        }

        if (keyUsage & PKIX_DECIPHER_ONLY){
                
                PKIX_ERROR(PKIX_DECIPHERONLYKEYUSAGENOTSUPPORTED);
        }

        status = CERT_CheckKeyUsage(nssCert, nssKeyUsage);
        if (status != SECSuccess) {
                PKIX_ERROR(PKIX_CERTCHECKKEYUSAGEFAILED);
        }

cleanup:
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetNameConstraints(
        PKIX_PL_Cert *cert,
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetNameConstraints");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pNameConstraints);

        
        if (cert->nameConstraints == NULL && !cert->nameConstraintsAbsent) {

                PKIX_OBJECT_LOCK(cert);

                if (cert->nameConstraints == NULL &&
                    !cert->nameConstraintsAbsent) {

                        PKIX_CHECK(pkix_pl_CertNameConstraints_Create
                                (cert->nssCert, &nameConstraints, plContext),
                                PKIX_CERTNAMECONSTRAINTSCREATEFAILED);

                        if (nameConstraints == NULL) {
                                cert->nameConstraintsAbsent = PKIX_TRUE;
                        }

                        cert->nameConstraints = nameConstraints;
                }

                PKIX_OBJECT_UNLOCK(cert);

        }

        PKIX_INCREF(cert->nameConstraints);

        *pNameConstraints = cert->nameConstraints;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_CheckNameConstraints(
        PKIX_PL_Cert *cert,
        PKIX_PL_CertNameConstraints *nameConstraints,
        void *plContext)
{
        PKIX_Boolean checkPass = PKIX_TRUE;
        CERTGeneralName *nssSubjectNames = NULL;
        PLArenaPool *arena = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_CheckNameConstraints");
        PKIX_NULLCHECK_ONE(cert);

        if (nameConstraints != NULL) {

                arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                if (arena == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                }

                
                PKIX_CERT_DEBUG
                    ("\t\tCalling CERT_GetConstrainedCertificateNames\n");
                nssSubjectNames = CERT_GetConstrainedCertificateNames
                        (cert->nssCert, arena, PR_TRUE);

                PKIX_CHECK(pkix_pl_CertNameConstraints_CheckNameSpaceNssNames
                        (nssSubjectNames,
                        nameConstraints,
                        &checkPass,
                        plContext),
                        PKIX_CERTNAMECONSTRAINTSCHECKNAMESPACENSSNAMESFAILED);

                if (checkPass != PKIX_TRUE) {
                        PKIX_ERROR(PKIX_CERTFAILEDNAMECONSTRAINTSCHECKING);
                }
        }

cleanup:
        if (arena){
                PORT_FreeArena(arena, PR_FALSE);
        }

        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_MergeNameConstraints(
        PKIX_PL_CertNameConstraints *firstNC,
        PKIX_PL_CertNameConstraints *secondNC,
        PKIX_PL_CertNameConstraints **pResultNC,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *mergedNC = NULL;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_MergeNameConstraints");
        PKIX_NULLCHECK_TWO(firstNC, pResultNC);

        if (secondNC == NULL) {

                PKIX_INCREF(firstNC);
                *pResultNC = firstNC;

                goto cleanup;
        }

        PKIX_CHECK(pkix_pl_CertNameConstraints_Merge
                (firstNC, secondNC, &mergedNC, plContext),
                PKIX_CERTNAMECONSTRAINTSMERGEFAILED);

        *pResultNC = mergedNC;

cleanup:
        PKIX_RETURN(CERT);
}








static SECStatus
pkix_pl_Cert_GetTrusted(void *plContext,
                        PKIX_PL_Cert *cert,
                        PKIX_Boolean *trusted,
                        PKIX_Boolean isCA)
{
        SECStatus rv;
        CERTCertificate *nssCert = NULL;
        SECCertUsage certUsage = 0;
        SECCertificateUsage certificateUsage;
        SECTrustType trustType;
        unsigned int trustFlags;
        unsigned int requiredFlags;
        CERTCertTrust trust;

        *trusted = PKIX_FALSE;

        
        if (plContext == NULL) {
                return SECSuccess;
        }

        certificateUsage = ((PKIX_PL_NssContext*)plContext)->certificateUsage;

        
        PORT_Assert(!(certificateUsage & (certificateUsage - 1)));

        
        while (0 != (certificateUsage = certificateUsage >> 1)) { certUsage++; }

        nssCert = cert->nssCert;

        if (!isCA) {
                PRBool prTrusted;
                unsigned int failedFlags;
                rv = cert_CheckLeafTrust(nssCert, certUsage,
                                         &failedFlags, &prTrusted);
                *trusted = (PKIX_Boolean) prTrusted;
                return rv;
        }
        rv = CERT_TrustFlagsForCACertUsage(certUsage, &requiredFlags,
                                           &trustType);
        if (rv != SECSuccess) {
                return SECSuccess;
        }

        rv = CERT_GetCertTrust(nssCert, &trust);
        if (rv != SECSuccess) {
                return SECSuccess;
        }
        trustFlags = SEC_GET_TRUST_FLAGS(&trust, trustType);
        


        if ((trustFlags == 0) && (trustType == trustTypeNone)) {
                trustFlags = trust.sslFlags | trust.emailFlags |
                             trust.objectSigningFlags;
        }
        if ((trustFlags & requiredFlags) == requiredFlags) {
                *trusted = PKIX_TRUE;
                return SECSuccess;
        }
        if ((trustFlags & CERTDB_TERMINAL_RECORD) &&
            ((trustFlags & (CERTDB_VALID_CA|CERTDB_TRUSTED)) == 0)) {
                return SECFailure;
        }
        return SECSuccess;
}





PKIX_Error *
PKIX_PL_Cert_IsCertTrusted(
        PKIX_PL_Cert *cert,
        PKIX_Boolean trustOnlyUserAnchors,
        PKIX_Boolean *pTrusted,
        void *plContext)
{
        PKIX_CertStore_CheckTrustCallback trustCallback = NULL;
        PKIX_Boolean trusted = PKIX_FALSE;
        SECStatus rv = SECFailure;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_IsCertTrusted");
        PKIX_NULLCHECK_TWO(cert, pTrusted);

        

        rv = pkix_pl_Cert_GetTrusted(plContext, cert, &trusted, PKIX_TRUE);
        if (rv != SECSuccess) {
                

                *pTrusted = PKIX_FALSE;
                PKIX_ERROR(PKIX_CERTISCERTTRUSTEDFAILED);
        }

        if (trustOnlyUserAnchors) {
            
            *pTrusted = cert->isUserTrustAnchor;
            goto cleanup;
        }

        
        if (plContext == NULL || cert->store == NULL) {
                *pTrusted = PKIX_FALSE;
                goto cleanup;
        }

        PKIX_CHECK(PKIX_CertStore_GetTrustCallback
                (cert->store, &trustCallback, plContext),
                PKIX_CERTSTOREGETTRUSTCALLBACKFAILED);

        PKIX_CHECK_ONLY_FATAL(trustCallback
                (cert->store, cert, &trusted, plContext),
                PKIX_CHECKTRUSTCALLBACKFAILED);

        

        if (PKIX_ERROR_RECEIVED || (trusted == PKIX_FALSE)) {
                *pTrusted = PKIX_FALSE;
                goto cleanup;
        }

        *pTrusted = trusted;

cleanup:
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_IsLeafCertTrusted(
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pTrusted,
        void *plContext)
{
        SECStatus rv;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_IsLeafCertTrusted");
        PKIX_NULLCHECK_TWO(cert, pTrusted);

        *pTrusted = PKIX_FALSE;

        rv = pkix_pl_Cert_GetTrusted(plContext, cert, pTrusted, PKIX_FALSE);
        if (rv != SECSuccess) {
                

                *pTrusted = PKIX_FALSE;
                PKIX_ERROR(PKIX_CERTISCERTTRUSTEDFAILED);
        }

cleanup:
        PKIX_RETURN(CERT);
}


PKIX_Error*
PKIX_PL_Cert_SetAsTrustAnchor(PKIX_PL_Cert *cert, 
                              void *plContext)
{
    PKIX_ENTER(CERT, "PKIX_PL_Cert_SetAsTrustAnchor");
    PKIX_NULLCHECK_ONE(cert);
    
    cert->isUserTrustAnchor = PKIX_TRUE;
    
    PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetCacheFlag(
        PKIX_PL_Cert *cert,
        PKIX_Boolean *pCacheFlag,
        void *plContext)
{
        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetCacheFlag");
        PKIX_NULLCHECK_TWO(cert, pCacheFlag);

        *pCacheFlag = cert->cacheFlag;

        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_SetCacheFlag(
        PKIX_PL_Cert *cert,
        PKIX_Boolean cacheFlag,
        void *plContext)
{
        PKIX_ENTER(CERT, "PKIX_PL_Cert_SetCacheFlag");
        PKIX_NULLCHECK_ONE(cert);

        cert->cacheFlag = cacheFlag;

        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_GetTrustCertStore(
        PKIX_PL_Cert *cert,
        PKIX_CertStore **pTrustCertStore,
        void *plContext)
{
        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetTrustCertStore");
        PKIX_NULLCHECK_TWO(cert, pTrustCertStore);

        PKIX_INCREF(cert->store);
        *pTrustCertStore = cert->store;

cleanup:
        PKIX_RETURN(CERT);
}




PKIX_Error *
PKIX_PL_Cert_SetTrustCertStore(
        PKIX_PL_Cert *cert,
        PKIX_CertStore *trustCertStore,
        void *plContext)
{
        PKIX_ENTER(CERT, "PKIX_PL_Cert_SetTrustCertStore");
        PKIX_NULLCHECK_TWO(cert, trustCertStore);

        PKIX_INCREF(trustCertStore);
        cert->store = trustCertStore;

cleanup:
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetAuthorityInfoAccess(
        PKIX_PL_Cert *cert,
        PKIX_List **pAiaList, 
        void *plContext)
{
        PKIX_List *aiaList = NULL; 
        SECItem *encodedAIA = NULL;
        CERTAuthInfoAccess **aia = NULL;
        PLArenaPool *arena = NULL;
        SECStatus rv;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetAuthorityInfoAccess");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pAiaList);

        
        if (cert->authorityInfoAccess == NULL) {

                PKIX_OBJECT_LOCK(cert);

                if (cert->authorityInfoAccess == NULL) {

                    PKIX_PL_NSSCALLRV(CERT, encodedAIA, SECITEM_AllocItem,
                        (NULL, NULL, 0));

                    if (encodedAIA == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                    }

                    PKIX_PL_NSSCALLRV(CERT, rv, CERT_FindCertExtension,
                        (cert->nssCert,
                        SEC_OID_X509_AUTH_INFO_ACCESS,
                        encodedAIA));

                    if (rv == SECFailure) {
                        goto cleanup;
                    }

                    PKIX_PL_NSSCALLRV(CERT, arena, PORT_NewArena,
                        (DER_DEFAULT_CHUNKSIZE));

                    if (arena == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                    }

                    PKIX_PL_NSSCALLRV
                        (CERT, aia, CERT_DecodeAuthInfoAccessExtension,
                        (arena, encodedAIA));

                    PKIX_CHECK(pkix_pl_InfoAccess_CreateList
                        (aia, &aiaList, plContext),
                        PKIX_INFOACCESSCREATELISTFAILED);

                    cert->authorityInfoAccess = aiaList;
                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->authorityInfoAccess);

        *pAiaList = cert->authorityInfoAccess;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        if (arena != NULL) {
                PORT_FreeArena(arena, PR_FALSE);
        }

        if (encodedAIA != NULL) {
                SECITEM_FreeItem(encodedAIA, PR_TRUE);
        }

        PKIX_RETURN(CERT);
}


static const unsigned char siaOIDString[] = {0x2b, 0x06, 0x01, 0x05, 0x05,
                                0x07, 0x01, 0x0b};
#define OI(x) { siDEROID, (unsigned char *)x, sizeof x }





PKIX_Error *
PKIX_PL_Cert_GetSubjectInfoAccess(
        PKIX_PL_Cert *cert,
        PKIX_List **pSiaList, 
        void *plContext)
{
        PKIX_List *siaList; 
        SECItem siaOID = OI(siaOIDString);
        SECItem *encodedSubjInfoAccess = NULL;
        CERTAuthInfoAccess **subjInfoAccess = NULL;
        PLArenaPool *arena = NULL;
        SECStatus rv;

        PKIX_ENTER(CERT, "PKIX_PL_Cert_GetSubjectInfoAccess");
        PKIX_NULLCHECK_THREE(cert, cert->nssCert, pSiaList);

        





        
        if (cert->subjectInfoAccess == NULL) {

                PKIX_OBJECT_LOCK(cert);

                if (cert->subjectInfoAccess == NULL) {

                    encodedSubjInfoAccess = SECITEM_AllocItem(NULL, NULL, 0);
                    if (encodedSubjInfoAccess == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                    }

                    PKIX_CERT_DEBUG
                        ("\t\tCalling CERT_FindCertExtensionByOID).\n");
                    rv = CERT_FindCertExtensionByOID
                                (cert->nssCert, &siaOID, encodedSubjInfoAccess);

                    if (rv == SECFailure) {
                        goto cleanup;
                    }

                    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                    if (arena == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                    }

                    





                    PKIX_CERT_DEBUG
                        ("\t\tCalling CERT_DecodeAuthInfoAccessExtension).\n");
                    subjInfoAccess = CERT_DecodeAuthInfoAccessExtension
                        (arena, encodedSubjInfoAccess);

                    PKIX_CHECK(pkix_pl_InfoAccess_CreateList
                            (subjInfoAccess, &siaList, plContext),
                            PKIX_INFOACCESSCREATELISTFAILED);

                    cert->subjectInfoAccess = siaList;

                }

                PKIX_OBJECT_UNLOCK(cert);
        }

        PKIX_INCREF(cert->subjectInfoAccess);
        *pSiaList = cert->subjectInfoAccess;

cleanup:
	PKIX_OBJECT_UNLOCK(lockedObject);
        if (arena != NULL) {
                PORT_FreeArena(arena, PR_FALSE);
        }

        if (encodedSubjInfoAccess != NULL) {
                SECITEM_FreeItem(encodedSubjInfoAccess, PR_TRUE);
        }
        PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetCrlDp(
    PKIX_PL_Cert *cert,
    PKIX_List **pDpList,
    void *plContext)
{
    PKIX_UInt32 dpIndex = 0;
    pkix_pl_CrlDp *dp = NULL; 
    CERTCrlDistributionPoints *dpoints = NULL;

    PKIX_ENTER(CERT, "PKIX_PL_Cert_GetCrlDp");
    PKIX_NULLCHECK_THREE(cert, cert->nssCert, pDpList);
                
    
    if (cert->crldpList == NULL) {
        PKIX_OBJECT_LOCK(cert);
        if (cert->crldpList != NULL) {
            goto cleanup;
        }
        PKIX_CHECK(PKIX_List_Create(&cert->crldpList, plContext),
                   PKIX_LISTCREATEFAILED);
        dpoints = CERT_FindCRLDistributionPoints(cert->nssCert);
        if (!dpoints || !dpoints->distPoints) {
            goto cleanup;
        }
        for (;dpoints->distPoints[dpIndex];dpIndex++) {
            PKIX_CHECK(
                pkix_pl_CrlDp_Create(dpoints->distPoints[dpIndex],
                                     &cert->nssCert->issuer,
                                     &dp, plContext),
                PKIX_CRLDPCREATEFAILED);
            

            PKIX_CHECK(
                PKIX_List_InsertItem(cert->crldpList, 0,
                                     (PKIX_PL_Object*)dp,
                                     plContext),
                PKIX_LISTAPPENDITEMFAILED);
            PKIX_DECREF(dp);
        }
    }
cleanup:
    PKIX_INCREF(cert->crldpList);
    *pDpList = cert->crldpList;

    PKIX_OBJECT_UNLOCK(lockedObject);
    PKIX_DECREF(dp);

    PKIX_RETURN(CERT);
}





PKIX_Error *
PKIX_PL_Cert_GetCERTCertificate(
        PKIX_PL_Cert *cert,
        CERTCertificate **pnssCert, 
        void *plContext)
{
    PKIX_ENTER(CERT, "PKIX_PL_Cert_GetNssCert");
    PKIX_NULLCHECK_TWO(cert, pnssCert);

    *pnssCert = CERT_DupCertificate(cert->nssCert);

    PKIX_RETURN(CERT);
}
