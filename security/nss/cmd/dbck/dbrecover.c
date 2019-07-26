



enum {
    dbInvalidCert = 0,
    dbNoSMimeProfile,
    dbOlderCert,
    dbBadCertificate,
    dbCertNotWrittenToDB
};

typedef struct dbRestoreInfoStr
{
    NSSLOWCERTCertDBHandle *handle;
    PRBool verbose;
    PRFileDesc *out;
    int nCerts;
    int nOldCerts;
    int dbErrors[5];
    PRBool removeType[3];
    PRBool promptUser[3];
} dbRestoreInfo;

char *
IsEmailCert(CERTCertificate *cert)
{
    char *email, *tmp1, *tmp2;
    PRBool isCA;
    int len;

    if (!cert->subjectName) {
	return NULL;
    }

    tmp1 = PORT_Strstr(cert->subjectName, "E=");
    tmp2 = PORT_Strstr(cert->subjectName, "MAIL=");
    


    if (!tmp1 && !tmp2 && !(cert->emailAddr && cert->emailAddr[0])) {
	return NULL;
    }

    
    isCA = CERT_IsCACert(cert, NULL);
    if (isCA)
	return NULL;

    

    
    if (cert->keyUsagePresent) {
	


	if (!((cert->rawKeyUsage & KU_DIGITAL_SIGNATURE) || 
              (cert->rawKeyUsage & KU_KEY_ENCIPHERMENT)))
	    return NULL;

	
	if (cert->rawKeyUsage & (KU_KEY_CERT_SIGN | KU_CRL_SIGN))
	    return NULL;
    }

    if (cert->emailAddr && cert->emailAddr[0]) {
	email = PORT_Strdup(cert->emailAddr);
    } else {
	if (tmp1)
	    tmp1 += 2; 
	else
	    tmp1 = tmp2 + 5; 
	len = strcspn(tmp1, ", ");
	email = (char*)PORT_Alloc(len+1);
	PORT_Strncpy(email, tmp1, len);
	email[len] = '\0';
    }

    return email;
}

SECStatus
deleteit(CERTCertificate *cert, void *arg)
{
    return SEC_DeletePermCertificate(cert);
}




SECStatus
deleteAllEntriesForCert(NSSLOWCERTCertDBHandle *handle, CERTCertificate *cert,
                        PRFileDesc *outfile)
{
#if 0
    certDBEntrySubject *subjectEntry;
    certDBEntryNickname *nicknameEntry;
    certDBEntrySMime *smimeEntry;
    int i;
#endif

    if (outfile) {
	PR_fprintf(outfile, "$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$\n\n");
	PR_fprintf(outfile, "Deleting redundant certificate:\n");
	dumpCertificate(cert, -1, outfile);
    }

    CERT_TraverseCertsForSubject(handle, cert->subjectList, deleteit, NULL);
#if 0
    CERT_LockDB(handle);
    subjectEntry = ReadDBSubjectEntry(handle, &cert->derSubject);
    
    PORT_Assert(subjectEntry);
    for (i=0; i<subjectEntry->ncerts; i++) {
	DeleteDBCertEntry(handle, &subjectEntry->certKeys[i]);
    }
    DeleteDBSubjectEntry(handle, &cert->derSubject);
    if (subjectEntry->emailAddr && subjectEntry->emailAddr[0]) {
	smimeEntry = ReadDBSMimeEntry(handle, subjectEntry->emailAddr);
	if (smimeEntry) {
	    if (SECITEM_ItemsAreEqual(&subjectEntry->derSubject,
	                              &smimeEntry->subjectName))
		
		DeleteDBSMimeEntry(handle, subjectEntry->emailAddr);
	    SEC_DestroyDBEntry((certDBEntry*)smimeEntry);
	}
    }
    if (subjectEntry->nickname) {
	nicknameEntry = ReadDBNicknameEntry(handle, subjectEntry->nickname);
	if (nicknameEntry) {
	    if (SECITEM_ItemsAreEqual(&subjectEntry->derSubject,
	                              &nicknameEntry->subjectName))
		
		DeleteDBNicknameEntry(handle, subjectEntry->nickname);
	    SEC_DestroyDBEntry((certDBEntry*)nicknameEntry);
	}
    }
    SEC_DestroyDBEntry((certDBEntry*)subjectEntry);
    CERT_UnlockDB(handle);
#endif
    return SECSuccess;
}

void
getCertsToDelete(char *numlist, int len, int *certNums, int nCerts)
{
    int j, num;
    char *numstr, *numend, *end;

    numstr = numlist;
    end = numstr + len - 1;
    while (numstr != end) {
	numend = strpbrk(numstr, ", \n");
	*numend = '\0';
	if (PORT_Strlen(numstr) == 0)
	    return;
	num = PORT_Atoi(numstr);
	if (numstr == numlist)
	    certNums[0] = num;
	for (j=1; j<nCerts+1; j++) {
	    if (num == certNums[j]) {
		certNums[j] = -1;
		break;
	    }
	}
	if (numend == end)
	    break;
	numstr = strpbrk(numend+1, "0123456789");
    }
}

PRBool
userSaysDeleteCert(CERTCertificate **certs, int nCerts,
                   int errtype, dbRestoreInfo *info, int *certNums)
{
    char response[32];
    int32 nb;
    int i;
    
    if (info->promptUser[errtype] == PR_FALSE)
	return (info->removeType[errtype]);
    switch (errtype) {
    case dbInvalidCert:
	PR_fprintf(PR_STDOUT, "********  Expired ********\n");
	PR_fprintf(PR_STDOUT, "Cert has expired.\n\n");
	dumpCertificate(certs[0], -1, PR_STDOUT);
	PR_fprintf(PR_STDOUT,
	           "Keep it? (y/n - this one, Y/N - all expired certs) [n] ");
	break;
    case dbNoSMimeProfile:
	PR_fprintf(PR_STDOUT, "********  No Profile ********\n");
	PR_fprintf(PR_STDOUT, "S/MIME cert has no profile.\n\n");
	dumpCertificate(certs[0], -1, PR_STDOUT);
	PR_fprintf(PR_STDOUT,
	      "Keep it? (y/n - this one, Y/N - all S/MIME w/o profile) [n] ");
	break;
    case dbOlderCert:
	PR_fprintf(PR_STDOUT, "*******  Redundant nickname/email *******\n\n");
	PR_fprintf(PR_STDOUT, "These certs have the same nickname/email:\n");
	for (i=0; i<nCerts; i++)
	    dumpCertificate(certs[i], i, PR_STDOUT);
	PR_fprintf(PR_STDOUT, 
	"Enter the certs you would like to keep from those listed above.\n");
	PR_fprintf(PR_STDOUT, 
	"Use a comma-separated list of the cert numbers (ex. 0, 8, 12).\n");
	PR_fprintf(PR_STDOUT, 
	"The first cert in the list will be the primary cert\n");
	PR_fprintf(PR_STDOUT, 
	" accessed by the nickname/email handle.\n");
	PR_fprintf(PR_STDOUT, 
	"List cert numbers to keep here, or hit enter\n");
	PR_fprintf(PR_STDOUT, 
	" to always keep only the newest cert:  ");
	break;
    default:
    }
    nb = PR_Read(PR_STDIN, response, sizeof(response));
    PR_fprintf(PR_STDOUT, "\n\n");
    if (errtype == dbOlderCert) {
	if (!isdigit(response[0])) {
	    info->promptUser[errtype] = PR_FALSE;
	    info->removeType[errtype] = PR_TRUE;
	    return PR_TRUE;
	}
	getCertsToDelete(response, nb, certNums, nCerts);
	return PR_TRUE;
    }
    
    if (response[0] == 'Y') {
	info->promptUser[errtype] = PR_FALSE;
	info->removeType[errtype] = PR_FALSE;
	return PR_FALSE;
    } else if (response[0] == 'N') {
	info->promptUser[errtype] = PR_FALSE;
	info->removeType[errtype] = PR_TRUE;
	return PR_TRUE;
    }
    return (response[0] != 'y') ? PR_TRUE : PR_FALSE;
}

SECStatus
addCertToDB(certDBEntryCert *certEntry, dbRestoreInfo *info, 
            NSSLOWCERTCertDBHandle *oldhandle)
{
    SECStatus rv = SECSuccess;
    PRBool allowOverride;
    PRBool userCert;
    SECCertTimeValidity validity;
    CERTCertificate *oldCert = NULL;
    CERTCertificate *dbCert = NULL;
    CERTCertificate *newCert = NULL;
    CERTCertTrust *trust;
    certDBEntrySMime *smimeEntry = NULL;
    char *email = NULL;
    char *nickname = NULL;
    int nCertsForSubject = 1;

    oldCert = CERT_DecodeDERCertificate(&certEntry->derCert, PR_FALSE,
                                        certEntry->nickname);
    if (!oldCert) {
	info->dbErrors[dbBadCertificate]++;
	SEC_DestroyDBEntry((certDBEntry*)certEntry);
	return SECSuccess;
    }

    oldCert->dbEntry = certEntry;
    oldCert->trust = &certEntry->trust;
    oldCert->dbhandle = oldhandle;

    trust = oldCert->trust;

    info->nOldCerts++;

    if (info->verbose)
	PR_fprintf(info->out, "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%\n\n");

    if (oldCert->nickname)
	nickname = PORT_Strdup(oldCert->nickname);

    
    


    userCert = (SEC_GET_TRUST_FLAGS(trust, trustSSL) & CERTDB_USER) ||
               (SEC_GET_TRUST_FLAGS(trust, trustEmail) & CERTDB_USER) ||
               (SEC_GET_TRUST_FLAGS(trust, trustObjectSigning) & CERTDB_USER);
    if (userCert)
	goto createcert;

    
    allowOverride = (PRBool)((oldCert->keyUsage == certUsageSSLServer) ||
                         (oldCert->keyUsage == certUsageSSLServerWithStepUp));
    validity = CERT_CheckCertValidTimes(oldCert, PR_Now(), allowOverride);
    
    if ((validity != secCertTimeValid) && 
	 userSaysDeleteCert(&oldCert, 1, dbInvalidCert, info, 0)) {
	info->dbErrors[dbInvalidCert]++;
	if (info->verbose) {
	    PR_fprintf(info->out, "Deleting expired certificate:\n");
	    dumpCertificate(oldCert, -1, info->out);
	}
	goto cleanup;
    }

    

    dbCert = CERT_FindCertByDERCert(info->handle, &oldCert->derCert);
    if (dbCert) {
	info->nCerts++;
	if (info->verbose) {
	    PR_fprintf(info->out, "Added certificate to database:\n");
	    dumpCertificate(oldCert, -1, info->out);
	}
	goto cleanup;
    }
    
    
    email = IsEmailCert(oldCert);

    


















createcert:

    
    if (!nickname && userCert)
	nickname = PORT_Strdup(oldCert->subjectName);

    
    newCert = CERT_NewTempCertificate(info->handle, &oldCert->derCert, 
                                      nickname, PR_FALSE, PR_TRUE);
    if (!newCert) {
	PR_fprintf(PR_STDERR, "Unable to create new certificate.\n");
	dumpCertificate(oldCert, -1, PR_STDERR);
	info->dbErrors[dbBadCertificate]++;
	goto cleanup;
    }

    
    rv = CERT_AddTempCertToPerm(newCert, nickname, oldCert->trust);
    if (rv) {
	PR_fprintf(PR_STDERR, "Failed to write temp cert to perm database.\n");
	dumpCertificate(oldCert, -1, PR_STDERR);
	info->dbErrors[dbCertNotWrittenToDB]++;
	goto cleanup;
    }

    if (info->verbose) {
	PR_fprintf(info->out, "Added certificate to database:\n");
	dumpCertificate(oldCert, -1, info->out);
    }

    



    if (smimeEntry) { 
#if 0
	UpdateSubjectWithEmailAddr(newCert, email);
#endif
	SECItem emailProfile, profileTime;
	rv = CERT_FindFullSMimeProfile(oldCert, &emailProfile, &profileTime);
	
	if (rv == SECSuccess)
	    rv = CERT_SaveSMimeProfile(newCert, &emailProfile, &profileTime);
    }

    info->nCerts++;

cleanup:

    if (nickname)
	PORT_Free(nickname);
    if (email)
	PORT_Free(email);
    if (oldCert)
	CERT_DestroyCertificate(oldCert);
    if (dbCert)
	CERT_DestroyCertificate(dbCert);
    if (newCert)
	CERT_DestroyCertificate(newCert);
    if (smimeEntry)
	SEC_DestroyDBEntry((certDBEntry*)smimeEntry);
    return SECSuccess;
}

#if 0
SECStatus
copyDBEntry(SECItem *data, SECItem *key, certDBEntryType type, void *pdata)
{
    SECStatus rv;
    NSSLOWCERTCertDBHandle *newdb = (NSSLOWCERTCertDBHandle *)pdata;
    certDBEntryCommon common;
    SECItem dbkey;

    common.type = type;
    common.version = CERT_DB_FILE_VERSION;
    common.flags = data->data[2];
    common.arena = NULL;

    dbkey.len = key->len + SEC_DB_KEY_HEADER_LEN;
    dbkey.data = (unsigned char *)PORT_Alloc(dbkey.len*sizeof(unsigned char));
    PORT_Memcpy(&dbkey.data[SEC_DB_KEY_HEADER_LEN], key->data, key->len);
    dbkey.data[0] = type;

    rv = WriteDBEntry(newdb, &common, &dbkey, data);

    PORT_Free(dbkey.data);
    return rv;
}
#endif

int
certIsOlder(CERTCertificate **cert1, CERTCertificate** cert2)
{
    return !CERT_IsNewer(*cert1, *cert2);
}

int
findNewestSubjectForEmail(NSSLOWCERTCertDBHandle *handle, int subjectNum,
                          certDBArray *dbArray, dbRestoreInfo *info,
                          int *subjectWithSMime, int *smimeForSubject)
{
    int newestSubject;
    int subjectsForEmail[50];
    int i, j, ns, sNum;
    certDBEntryListNode *subjects = &dbArray->subjects;
    certDBEntryListNode *smime = &dbArray->smime;
    certDBEntrySubject *subjectEntry1, *subjectEntry2;
    certDBEntrySMime *smimeEntry;
    CERTCertificate **certs;
    CERTCertificate *cert;
    CERTCertTrust *trust;
    PRBool userCert;
    int *certNums;

    ns = 0;
    subjectEntry1 = (certDBEntrySubject*)&subjects.entries[subjectNum];
    subjectsForEmail[ns++] = subjectNum;

    *subjectWithSMime = -1;
    *smimeForSubject = -1;
    newestSubject = subjectNum;

    cert = CERT_FindCertByKey(handle, &subjectEntry1->certKeys[0]);
    if (cert) {
	trust = cert->trust;
	userCert = (SEC_GET_TRUST_FLAGS(trust, trustSSL) & CERTDB_USER) ||
	          (SEC_GET_TRUST_FLAGS(trust, trustEmail) & CERTDB_USER) ||
	         (SEC_GET_TRUST_FLAGS(trust, trustObjectSigning) & CERTDB_USER);
	CERT_DestroyCertificate(cert);
    }

    





    
    for (i=subjectNum+1; i<subjects.numEntries; i++) {
	subjectEntry2 = (certDBEntrySubject*)&subjects.entries[i];
	if (!subjectEntry2)
	    continue;
	if (subjectEntry2->emailAddr && subjectEntry2->emailAddr[0] &&
	     PORT_Strcmp(subjectEntry1->emailAddr, 
	                 subjectEntry2->emailAddr) == 0) {
	    
	    subjectsForEmail[ns++] = i;
	}
    }

    
    for (i=0; i<smime.numEntries; i++) {
	smimeEntry = (certDBEntrySMime*)&smime.entries[i];
	if (smimeEntry->common.arena == NULL)
	    continue;
	if (smimeEntry->emailAddr && smimeEntry->emailAddr[0] && 
	    PORT_Strcmp(subjectEntry1->emailAddr, smimeEntry->emailAddr) == 0) {
	    
	    for (j=0; j<ns && *subjectWithSMime < 0; j++) {
		sNum = subjectsForEmail[j];
		subjectEntry2 = (certDBEntrySubject*)&subjects.entries[sNum];
		if (SECITEM_ItemsAreEqual(&smimeEntry->subjectName,
		                          &subjectEntry2->derSubject)) {
		    
		    *subjectWithSMime = sNum;
		    *smimeForSubject = i;
		}
	    }
	    SEC_DestroyDBEntry((certDBEntry*)smimeEntry);
	    PORT_Memset(smimeEntry, 0, sizeof(certDBEntry));
	    break;
	}
    }

    if (ns <= 1)
	return subjectNum;

    if (userCert)
	return *subjectWithSMime;

    
    certs = (CERTCertificate**)PORT_Alloc(ns*sizeof(CERTCertificate*));
    certNums = (int*)PORT_Alloc((ns+1)*sizeof(int));
    certNums[0] = 0;
    for (i=0; i<ns; i++) {
	sNum = subjectsForEmail[i];
	subjectEntry1 = (certDBEntrySubject*)&subjects.entries[sNum];
	certs[i] = CERT_FindCertByKey(handle, &subjectEntry1->certKeys[0]);
	certNums[i+1] = i;
    }
    
    qsort(certs, ns, sizeof(CERTCertificate*), 
          (int (*)(const void *, const void *))certIsOlder);
    newestSubject = -1;
    for (i=0; i<ns; i++) {
	sNum = subjectsForEmail[i];
	subjectEntry1 = (certDBEntrySubject*)&subjects.entries[sNum];
	if (SECITEM_ItemsAreEqual(&subjectEntry1->derSubject,
	                          &certs[0]->derSubject))
	    newestSubject = sNum;
	else
	    SEC_DestroyDBEntry((certDBEntry*)subjectEntry1);
    }
    if (info && userSaysDeleteCert(certs, ns, dbOlderCert, info, certNums)) {
	for (i=1; i<ns+1; i++) {
	    if (certNums[i] >= 0 && certNums[i] != certNums[0]) {
		deleteAllEntriesForCert(handle, certs[certNums[i]], info->out);
		info->dbErrors[dbOlderCert]++;
	    }
	}
    }
    CERT_DestroyCertArray(certs, ns);
    return newestSubject;
}

NSSLOWCERTCertDBHandle *
DBCK_ReconstructDBFromCerts(NSSLOWCERTCertDBHandle *oldhandle, char *newdbname,
                            PRFileDesc *outfile, PRBool removeExpired,
                            PRBool requireProfile, PRBool singleEntry,
                            PRBool promptUser)
{
    SECStatus rv;
    dbRestoreInfo info;
    certDBEntryContentVersion *oldContentVersion;
    certDBArray dbArray;
    int i;

    PORT_Memset(&dbArray, 0, sizeof(dbArray));
    PORT_Memset(&info, 0, sizeof(info));
    info.verbose = (outfile) ? PR_TRUE : PR_FALSE;
    info.out = (outfile) ? outfile : PR_STDOUT;
    info.removeType[dbInvalidCert] = removeExpired;
    info.removeType[dbNoSMimeProfile] = requireProfile;
    info.removeType[dbOlderCert] = singleEntry;
    info.promptUser[dbInvalidCert]  = promptUser;
    info.promptUser[dbNoSMimeProfile]  = promptUser;
    info.promptUser[dbOlderCert]  = promptUser;

    
    info.handle = PORT_ZNew(NSSLOWCERTCertDBHandle);
    if (!info.handle) {
	fprintf(stderr, "unable to get database handle");
	return NULL;
    }

    
    rv = CERT_OpenCertDBFilename(info.handle, newdbname, PR_FALSE);

    if (rv) {
	fprintf(stderr, "could not open certificate database");
	goto loser;
    }

    





    fillDBEntryArray(oldhandle, certDBEntryTypeCert, &dbArray.certs);
    for (elem = PR_LIST_HEAD(&dbArray->certs.link);
         elem != &dbArray->certs.link; elem = PR_NEXT_LINK(elem)) {
	node = LISTNODE_CAST(elem);
	addCertToDB((certDBEntryCert*)&node->entry, &info, oldhandle);
	
    }
#if 0
    rv = nsslowcert_TraverseDBEntries(oldhandle, certDBEntryTypeSMimeProfile, 
                               copyDBEntry, info.handle);
#endif

    




#if 0
    CERT_RedoHandlesForSubjects(info.handle, singleEntry, &info);
#endif

    freeDBEntryList(&dbArray.certs.link);

    
    
    




    
    


#if 0
    oldContentVersion = ReadDBContentVersionEntry(oldhandle);
    CERT_SetDBContentVersion(oldContentVersion->contentVersion, info.handle); 
#endif

#if 0
    
    rv = nsslowcert_TraverseDBEntries(oldhandle, certDBEntryTypeRevocation, 
                               copyDBEntry, info.handle);
    
    rv = nsslowcert_TraverseDBEntries(oldhandle, certDBEntryTypeKeyRevocation, 
                               copyDBEntry, info.handle);
#endif

    PR_fprintf(info.out, "Database had %d certificates.\n", info.nOldCerts);

    PR_fprintf(info.out, "Reconstructed %d certificates.\n", info.nCerts);
    PR_fprintf(info.out, "(ax) Rejected %d expired certificates.\n", 
                       info.dbErrors[dbInvalidCert]);
    PR_fprintf(info.out, "(as) Rejected %d S/MIME certificates missing a profile.\n", 
                       info.dbErrors[dbNoSMimeProfile]);
    PR_fprintf(info.out, "(ar) Rejected %d certificates for which a newer certificate was found.\n", 
                       info.dbErrors[dbOlderCert]);
    PR_fprintf(info.out, "     Rejected %d corrupt certificates.\n", 
                       info.dbErrors[dbBadCertificate]);
    PR_fprintf(info.out, "     Rejected %d certificates which did not write to the DB.\n", 
                       info.dbErrors[dbCertNotWrittenToDB]);

    if (rv)
	goto loser;

    return info.handle;

loser:
    if (info.handle) 
	PORT_Free(info.handle);
    return NULL;
}

