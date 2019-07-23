










































#include "pkix_pl_generalname.h"
























PKIX_Error *
pkix_pl_GeneralName_GetNssGeneralName(
        PKIX_PL_GeneralName *genName,
        CERTGeneralName **pNssGenName,
        void *plContext)
{
        CERTGeneralName *nssGenName = NULL;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_GetNssGeneralName");
        PKIX_NULLCHECK_THREE(genName, pNssGenName, genName->nssGeneralNameList);

        nssGenName = genName->nssGeneralNameList->name;

        *pNssGenName = nssGenName;

        PKIX_RETURN(GENERALNAME);
}





















static PKIX_Error *
pkix_pl_OtherName_Create(
        CERTGeneralName *nssAltName,
        OtherName **pOtherName,
        void *plContext)
{
        OtherName *otherName = NULL;
        SECItem secItemName;
        SECItem secItemOID;
        SECStatus rv;

        PKIX_ENTER(GENERALNAME, "pkix_pl_OtherName_Create");
        PKIX_NULLCHECK_TWO(nssAltName, pOtherName);

        PKIX_CHECK(PKIX_PL_Malloc
                    (sizeof (OtherName), (void **)&otherName, plContext),
                    PKIX_MALLOCFAILED);

        
        PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_CopyItem).\n");
        rv = SECITEM_CopyItem
                (NULL, &otherName->name, &nssAltName->name.OthName.name);
        if (rv != SECSuccess) {
                PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        
        PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_CopyItem).\n");
        rv = SECITEM_CopyItem
                (NULL, &otherName->oid, &nssAltName->name.OthName.oid);
        if (rv != SECSuccess) {
                PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        *pOtherName = otherName;

cleanup:

        if (otherName && PKIX_ERROR_RECEIVED){
                secItemName = otherName->name;
                secItemOID = otherName->oid;

                PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_FreeItem).\n");
                SECITEM_FreeItem(&secItemName, PR_FALSE);

                PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_FreeItem).\n");
                SECITEM_FreeItem(&secItemOID, PR_FALSE);

                PKIX_FREE(otherName);
                otherName = NULL;
        }

        PKIX_RETURN(GENERALNAME);
}





















static PKIX_Error *
pkix_pl_DirectoryName_Create(
        CERTGeneralName *nssAltName,
        PKIX_PL_X500Name **pX500Name,
        void *plContext)
{
        PKIX_PL_X500Name *pkixDN = NULL;
        CERTName *dirName = NULL;
        PKIX_PL_String *pkixDNString = NULL;
        char *utf8String = NULL;

        PKIX_ENTER(GENERALNAME, "pkix_pl_DirectoryName_Create");
        PKIX_NULLCHECK_TWO(nssAltName, pX500Name);

        dirName = &nssAltName->name.directoryName;

        PKIX_CHECK(PKIX_PL_X500Name_CreateFromCERTName(NULL, dirName, 
                                                       &pkixDN, plContext),
                   PKIX_X500NAMECREATEFROMCERTNAMEFAILED);

        *pX500Name = pkixDN;

cleanup:

        PR_Free(utf8String);
        PKIX_DECREF(pkixDNString);

        PKIX_RETURN(GENERALNAME);
}





















PKIX_Error *
pkix_pl_GeneralName_Create(
        CERTGeneralName *nssAltName,
        PKIX_PL_GeneralName **pGenName,
        void *plContext)
{
        PKIX_PL_GeneralName *genName = NULL;
        PKIX_PL_X500Name *pkixDN = NULL;
        PKIX_PL_OID *pkixOID = NULL;
        OtherName *otherName = NULL;
        CERTGeneralNameList *nssGenNameList = NULL;
        CERTGeneralNameType nameType;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_Create");
        PKIX_NULLCHECK_TWO(nssAltName, pGenName);

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_GENERALNAME_TYPE,
                    sizeof (PKIX_PL_GeneralName),
                    (PKIX_PL_Object **)&genName,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        nameType = nssAltName->type;

        














        PKIX_GENERALNAME_DEBUG("\t\tCalling CERT_CreateGeneralNameList).\n");
        nssGenNameList = CERT_CreateGeneralNameList(nssAltName);

        if (nssGenNameList == NULL) {
                PKIX_ERROR(PKIX_CERTCREATEGENERALNAMELISTFAILED);
        }

        genName->nssGeneralNameList = nssGenNameList;

        
        genName->type = nameType;
        genName->directoryName = NULL;
        genName->OthName = NULL;
        genName->other = NULL;
        genName->oid = NULL;

        switch (nameType){
        case certOtherName:

                PKIX_CHECK(pkix_pl_OtherName_Create
                            (nssAltName, &otherName, plContext),
                            PKIX_OTHERNAMECREATEFAILED);

                genName->OthName = otherName;
                break;

        case certDirectoryName:

                PKIX_CHECK(pkix_pl_DirectoryName_Create
                            (nssAltName, &pkixDN, plContext),
                            PKIX_DIRECTORYNAMECREATEFAILED);

                genName->directoryName = pkixDN;
                break;
        case certRegisterID:
                PKIX_CHECK(PKIX_PL_OID_CreateBySECItem(&nssAltName->name.other,
                                                       &pkixOID, plContext),
                            PKIX_OIDCREATEFAILED);

                genName->oid = pkixOID;
                break;
        case certDNSName:
        case certEDIPartyName:
        case certIPAddress:
        case certRFC822Name:
        case certX400Address:
        case certURI:
                genName->other = SECITEM_DupItem(&nssAltName->name.other);
                if (!genName->other) {
                    PKIX_ERROR(PKIX_OUTOFMEMORY);
                }     
                break;
        default:
                PKIX_ERROR(PKIX_NAMETYPENOTSUPPORTED);
        }

        *pGenName = genName;
        genName = NULL;

cleanup:
        PKIX_DECREF(genName);

        PKIX_RETURN(GENERALNAME);
}























static PKIX_Error *
pkix_pl_GeneralName_ToString_Helper(
        PKIX_PL_GeneralName *name,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_X500Name *pkixDN = NULL;
        PKIX_PL_OID *pkixOID = NULL;
        char *x400AsciiName = NULL;
        char *ediPartyName = NULL;
        char *asciiName = NULL;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_ToString_Helper");
        PKIX_NULLCHECK_TWO(name, pString);

        switch (name->type) {
        case certRFC822Name:
        case certDNSName:
        case certURI:
                




                PKIX_NULLCHECK_ONE(name->other);

                PKIX_CHECK(PKIX_PL_String_Create(PKIX_UTF8,
                                                (name->other)->data,
                                                (name->other)->len,
                                                pString,
                                                plContext),
                            PKIX_STRINGCREATEFAILED);
                break;
        case certEDIPartyName:
                
                ediPartyName = "EDIPartyName: <DER-encoded value>";
                PKIX_CHECK(PKIX_PL_String_Create(PKIX_ESCASCII,
                                                ediPartyName,
                                                0,
                                                pString,
                                                plContext),
                            PKIX_STRINGCREATEFAILED);
                break;
        case certX400Address:
                
                x400AsciiName = "X400Address: <DER-encoded value>";
                PKIX_CHECK(PKIX_PL_String_Create(PKIX_ESCASCII,
                                                x400AsciiName,
                                                0,
                                                pString,
                                                plContext),
                            PKIX_STRINGCREATEFAILED);
                break;
        case certIPAddress:
                PKIX_CHECK(pkix_pl_ipAddrBytes2Ascii
                            (name->other, &asciiName, plContext),
                            PKIX_IPADDRBYTES2ASCIIFAILED);

                PKIX_CHECK(PKIX_PL_String_Create(PKIX_ESCASCII,
                                                asciiName,
                                                0,
                                                pString,
                                                plContext),
                            PKIX_STRINGCREATEFAILED);
                break;
        case certOtherName:
                PKIX_NULLCHECK_ONE(name->OthName);

                
                
                PKIX_CHECK(pkix_pl_oidBytes2Ascii
                            (&name->OthName->oid, &asciiName, plContext),
                            PKIX_OIDBYTES2ASCIIFAILED);

                PKIX_CHECK(PKIX_PL_String_Create
                            (PKIX_ESCASCII,
                            asciiName,
                            0,
                            pString,
                            plContext),
                            PKIX_STRINGCREATEFAILED);
                break;
        case certRegisterID:
                pkixOID = name->oid;
                PKIX_CHECK(PKIX_PL_Object_ToString
                            ((PKIX_PL_Object *)pkixOID, pString, plContext),
                            PKIX_OIDTOSTRINGFAILED);
                break;
        case certDirectoryName:
                pkixDN = name->directoryName;
                PKIX_CHECK(PKIX_PL_Object_ToString
                            ((PKIX_PL_Object *)pkixDN, pString, plContext),
                            PKIX_X500NAMETOSTRINGFAILED);
                break;
        default:
                PKIX_ERROR
                        (PKIX_TOSTRINGFORTHISGENERALNAMETYPENOTSUPPORTED);
        }

cleanup:

        PKIX_FREE(asciiName);

        PKIX_RETURN(GENERALNAME);
}





static PKIX_Error *
pkix_pl_GeneralName_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_GeneralName *name = NULL;
        SECItem secItemName;
        SECItem secItemOID;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_GENERALNAME_TYPE, plContext),
                    PKIX_OBJECTNOTGENERALNAME);

        name = (PKIX_PL_GeneralName *)object;

        PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_FreeItem).\n");
        SECITEM_FreeItem(name->other, PR_TRUE);
        name->other = NULL;

        if (name->OthName){
                secItemName = name->OthName->name;
                secItemOID = name->OthName->oid;

                PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_FreeItem).\n");
                SECITEM_FreeItem(&secItemName, PR_FALSE);

                PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_FreeItem).\n");
                SECITEM_FreeItem(&secItemOID, PR_FALSE);

                PKIX_FREE(name->OthName);
                name->OthName = NULL;
        }

        if (name->nssGeneralNameList != NULL) {
                PKIX_GENERALNAME_DEBUG
                        ("\t\tCalling CERT_DestroyGeneralNameList).\n");
                CERT_DestroyGeneralNameList(name->nssGeneralNameList);
        }

        PKIX_DECREF(name->directoryName);
        PKIX_DECREF(name->oid);

cleanup:

        PKIX_RETURN(GENERALNAME);
}





static PKIX_Error *
pkix_pl_GeneralName_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *nameString = NULL;
        PKIX_PL_GeneralName *name = NULL;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_toString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(object, PKIX_GENERALNAME_TYPE, plContext),
                    PKIX_OBJECTNOTGENERALNAME);

        name = (PKIX_PL_GeneralName *)object;

        PKIX_CHECK(pkix_pl_GeneralName_ToString_Helper
                    (name, &nameString, plContext),
                    PKIX_GENERALNAMETOSTRINGHELPERFAILED);

        *pString = nameString;

cleanup:



        PKIX_RETURN(GENERALNAME);
}





static PKIX_Error *
pkix_pl_GeneralName_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_GeneralName *name = NULL;
        PKIX_UInt32 firstHash, secondHash, nameHash;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_GENERALNAME_TYPE, plContext),
                    PKIX_OBJECTNOTGENERALNAME);

        name = (PKIX_PL_GeneralName *)object;

        switch (name->type) {
        case certRFC822Name:
        case certDNSName:
        case certX400Address:
        case certEDIPartyName:
        case certURI:
        case certIPAddress:
                PKIX_NULLCHECK_ONE(name->other);
                PKIX_CHECK(pkix_hash
                            ((const unsigned char *)
                            name->other->data,
                            name->other->len,
                            &nameHash,
                            plContext),
                            PKIX_HASHFAILED);
                break;
        case certRegisterID:
                PKIX_CHECK(PKIX_PL_Object_Hashcode
                            ((PKIX_PL_Object *)name->oid,
                            &nameHash,
                            plContext),
                            PKIX_OIDHASHCODEFAILED);
                break;
        case certOtherName:
                PKIX_NULLCHECK_ONE(name->OthName);
                PKIX_CHECK(pkix_hash
                            ((const unsigned char *)
                            name->OthName->oid.data,
                            name->OthName->oid.len,
                            &firstHash,
                            plContext),
                            PKIX_HASHFAILED);

                PKIX_CHECK(pkix_hash
                            ((const unsigned char *)
                            name->OthName->name.data,
                            name->OthName->name.len,
                            &secondHash,
                            plContext),
                            PKIX_HASHFAILED);

                nameHash = firstHash + secondHash;
                break;
        case certDirectoryName:
                PKIX_CHECK(PKIX_PL_Object_Hashcode
                            ((PKIX_PL_Object *)
                            name->directoryName,
                            &nameHash,
                            plContext),
                            PKIX_X500NAMEHASHCODEFAILED);
                break;
        }

        *pHashcode = nameHash;

cleanup:

        PKIX_RETURN(GENERALNAME);

}





static PKIX_Error *
pkix_pl_GeneralName_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_GeneralName *firstName = NULL;
        PKIX_PL_GeneralName *secondName = NULL;
        PKIX_UInt32 secondType;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                    (firstObject, PKIX_GENERALNAME_TYPE, plContext),
                    PKIX_FIRSTOBJECTNOTGENERALNAME);

        



        if (firstObject == secondObject){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;
        PKIX_CHECK(PKIX_PL_Object_GetType
                    (secondObject, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);
        if (secondType != PKIX_GENERALNAME_TYPE){
                goto cleanup;
        }

        firstName = (PKIX_PL_GeneralName *)firstObject;
        secondName = (PKIX_PL_GeneralName *)secondObject;

        if (firstName->type != secondName->type){
                goto cleanup;
        }

        switch (firstName->type) {
        case certRFC822Name:
        case certDNSName:
        case certX400Address:
        case certEDIPartyName:
        case certURI:
        case certIPAddress:
                PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_CompareItem).\n");
                if (SECITEM_CompareItem(firstName->other,
                                        secondName->other) != SECEqual) {
                        goto cleanup;
                }
                break;
        case certRegisterID:
                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object *)firstName->oid,
                            (PKIX_PL_Object *)secondName->oid,
                            pResult,
                            plContext),
                            PKIX_OIDEQUALSFAILED);
                goto cleanup;
        case certOtherName:
                PKIX_NULLCHECK_TWO(firstName->OthName, secondName->OthName);
                PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_CompareItem).\n");
                if (SECITEM_CompareItem(&firstName->OthName->oid,
                                        &secondName->OthName->oid)
                    != SECEqual ||
                    SECITEM_CompareItem(&firstName->OthName->name,
                                        &secondName->OthName->name)
                    != SECEqual) {
                        goto cleanup;
                }
                break;
        case certDirectoryName:
                PKIX_CHECK(PKIX_PL_Object_Equals
                            ((PKIX_PL_Object *)firstName->directoryName,
                            (PKIX_PL_Object *)secondName->directoryName,
                            pResult,
                            plContext),
                            PKIX_X500NAMEEQUALSFAILED);
                goto cleanup;
        }

        *pResult = PKIX_TRUE;

cleanup:

        PKIX_RETURN(GENERALNAME);
}












PKIX_Error *
pkix_pl_GeneralName_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(GENERALNAME, "pkix_pl_GeneralName_RegisterSelf");

        entry.description = "GeneralName";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_GeneralName);
        entry.destructor = pkix_pl_GeneralName_Destroy;
        entry.equalsFunction = pkix_pl_GeneralName_Equals;
        entry.hashcodeFunction = pkix_pl_GeneralName_Hashcode;
        entry.toStringFunction = pkix_pl_GeneralName_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_GENERALNAME_TYPE] = entry;

        PKIX_RETURN(GENERALNAME);
}



#ifdef BUILD_LIBPKIX_TESTS



PKIX_Error *
PKIX_PL_GeneralName_Create(
        PKIX_UInt32 nameType,
        PKIX_PL_String *stringRep,
        PKIX_PL_GeneralName **pGName,
        void *plContext)
{
        PKIX_PL_X500Name *pkixDN = NULL;
        PKIX_PL_OID *pkixOID = NULL;
        SECItem *secItem = NULL;
        char *asciiString = NULL;
        PKIX_UInt32 length = 0;
        PKIX_PL_GeneralName *genName = NULL;
        CERTGeneralName *nssGenName = NULL;
        CERTGeneralNameList *nssGenNameList = NULL;
        CERTName *nssCertName = NULL;
        PLArenaPool *arena = NULL;

        PKIX_ENTER(GENERALNAME, "PKIX_PL_GeneralName_Create");
        PKIX_NULLCHECK_TWO(pGName, stringRep);

        PKIX_CHECK(PKIX_PL_String_GetEncoded
                    (stringRep,
                    PKIX_ESCASCII,
                    (void **)&asciiString,
                    &length,
                    plContext),
                    PKIX_STRINGGETENCODEDFAILED);

        
        PKIX_GENERALNAME_DEBUG("\t\tCalling PL_strlen).\n");
        length = PL_strlen(asciiString);
        PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_AllocItem).\n");
        secItem = SECITEM_AllocItem(NULL, NULL, length);
        PKIX_GENERALNAME_DEBUG("\t\tCalling PORT_Memcpy).\n");
        (void) PORT_Memcpy(secItem->data, asciiString, length);
        PKIX_CERT_DEBUG("\t\tCalling PORT_NewArena).\n");
        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (arena == NULL) {
                PKIX_ERROR(PKIX_OUTOFMEMORY);
        }
        PKIX_GENERALNAME_DEBUG("\t\tCalling CERT_NewGeneralName).\n");
        nssGenName = CERT_NewGeneralName(arena, nameType);
        if (nssGenName == NULL) {
                PKIX_ERROR(PKIX_ALLOCATENEWCERTGENERALNAMEFAILED);
        }

        switch (nameType) {
        case certRFC822Name:
        case certDNSName:
        case certURI:
                nssGenName->name.other = *secItem;
                break;

        case certDirectoryName:

                PKIX_CHECK(PKIX_PL_X500Name_Create
                            (stringRep, &pkixDN, plContext),
                            PKIX_X500NAMECREATEFAILED);

                PKIX_GENERALNAME_DEBUG("\t\tCalling CERT_AsciiToName).\n");
                nssCertName = CERT_AsciiToName(asciiString);
                nssGenName->name.directoryName = *nssCertName;
                break;

        case certRegisterID:
                PKIX_CHECK(PKIX_PL_OID_Create
                            (asciiString, &pkixOID, plContext),
                            PKIX_OIDCREATEFAILED);
                nssGenName->name.other = *secItem;
                break;
        default:
                
                PKIX_ERROR(PKIX_UNABLETOCREATEGENERALNAMEOFTHISTYPE);
        }

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_GENERALNAME_TYPE,
                    sizeof (PKIX_PL_GeneralName),
                    (PKIX_PL_Object **)&genName,
                    plContext),
                    PKIX_COULDNOTCREATEOBJECT);

        
        nssGenName->type = nameType;
        PKIX_GENERALNAME_DEBUG("\t\tCalling CERT_CreateGeneralNameList).\n");
        nssGenNameList = CERT_CreateGeneralNameList(nssGenName);
        if (nssGenNameList == NULL) {
                PKIX_ERROR(PKIX_CERTCREATEGENERALNAMELISTFAILED);
        }
        genName->nssGeneralNameList = nssGenNameList;

        
        genName->type = nameType;
        genName->directoryName = pkixDN;
        genName->OthName = NULL;
        genName->other = secItem;
        genName->oid = pkixOID;

        *pGName = genName;
cleanup:

        PKIX_FREE(asciiString);

        if (nssCertName != NULL) {
                PKIX_CERT_DEBUG("\t\tCalling CERT_DestroyName).\n");
                CERT_DestroyName(nssCertName);
        }

        if (arena){ 
                PKIX_CERT_DEBUG("\t\tCalling PORT_FreeArena).\n");
                PORT_FreeArena(arena, PR_FALSE);
        }

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(pkixDN);
                PKIX_DECREF(pkixOID);

                PKIX_GENERALNAME_DEBUG("\t\tCalling SECITEM_FreeItem).\n");
                if (secItem){
                        SECITEM_FreeItem(secItem, PR_TRUE);
                        secItem = NULL;
                }
        }

        PKIX_RETURN(GENERALNAME);
}

#endif 
