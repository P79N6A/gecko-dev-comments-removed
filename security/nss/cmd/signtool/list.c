



































#include "signtool.h"
#include "pk11func.h"
#include "certdb.h"

static int	num_trav_certs = 0;
static SECStatus cert_trav_callback(CERTCertificate *cert, SECItem *k,
			void *data);





int
ListCerts(char *key, int list_certs)
{
    int	failed = 0;
    SECStatus rv;
    char	*ugly_list;
    CERTCertDBHandle * db;

    CERTCertificate * cert;
    CERTVerifyLog errlog;

    errlog.arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( errlog.arena == NULL) {
	out_of_memory();
    }
    errlog.head = NULL;
    errlog.tail = NULL;
    errlog.count = 0;

    ugly_list = PORT_ZAlloc (16);

    if (ugly_list == NULL) {
	out_of_memory();
    }

    *ugly_list = 0;

    db = CERT_GetDefaultCertDB();

    if (list_certs == 2) {
	PR_fprintf(outputFD, "\nS Certificates\n");
	PR_fprintf(outputFD, "- ------------\n");
    } else {
	PR_fprintf(outputFD, "\nObject signing certificates\n");
	PR_fprintf(outputFD, "---------------------------------------\n");
    }

    num_trav_certs = 0;

    
    rv = PK11_TraverseSlotCerts(cert_trav_callback, (void * )&list_certs,
         		&pwdata);

    if (rv) {
	PR_fprintf(outputFD, "**Traverse of non-internal DBs failed**\n");
	return - 1;
    }

    if (num_trav_certs == 0) {
	PR_fprintf(outputFD,
	    "You don't appear to have any object signing certificates.\n");
    }

    if (list_certs == 2) {
	PR_fprintf(outputFD, "- ------------\n");
    } else {
	PR_fprintf(outputFD, "---------------------------------------\n");
    }

    if (list_certs == 1) {
	PR_fprintf(outputFD,
	    "For a list including CA's, use \"%s -L\"\n", PROGRAM_NAME);
    }

    if (list_certs == 2) {
	PR_fprintf(outputFD,
	    "Certificates that can be used to sign objects have *'s to "
	    "their left.\n");
    }

    if (key) {
	

	cert = PK11_FindCertFromNickname(key, &pwdata);

	if (cert) {
	    PR_fprintf(outputFD,
	        "\nThe certificate with nickname \"%s\" was found:\n",
	         			 cert->nickname);
	    PR_fprintf(outputFD, "\tsubject name: %s\n", cert->subjectName);
	    PR_fprintf(outputFD, "\tissuer name: %s\n", cert->issuerName);

	    PR_fprintf(outputFD, "\n");

	    rv = CERT_CertTimesValid (cert);
	    if (rv != SECSuccess) {
		PR_fprintf(outputFD, "**This certificate is expired**\n");
	    } else {
		PR_fprintf(outputFD, "This certificate is not expired.\n");
	    }

	    rv = CERT_VerifyCert (db, cert, PR_TRUE,
	        certUsageObjectSigner, PR_Now(), &pwdata, &errlog);

	    if (rv != SECSuccess) {
		failed = 1;
		if (errlog.count > 0) {
		    PR_fprintf(outputFD,
		        "**Certificate validation failed for the "
		        "following reason(s):**\n");
		} else {
		    PR_fprintf(outputFD, "**Certificate validation failed**");
		}
	    } else {
		PR_fprintf(outputFD, "This certificate is valid.\n");
	    }
	    displayVerifyLog(&errlog);


	} else {
	    failed = 1;
	    PR_fprintf(outputFD,
	        "The certificate with nickname \"%s\" was NOT FOUND\n", key);
	}
    }

    if (errlog.arena != NULL) {
	PORT_FreeArena(errlog.arena, PR_FALSE);
    }

    if (failed) {
	return - 1;
    }
    return 0;
}






static SECStatus
cert_trav_callback(CERTCertificate *cert, SECItem *k, void *data)
{
    int	isSigningCert;
    int	list_certs = 1;

    char	*name, *issuerCN, *expires;
    CERTCertificate * issuerCert = NULL;

    if (data) {
	list_certs = *((int * )data);
    }

    if (cert->nickname) {
	name = cert->nickname;

	isSigningCert = cert->nsCertType & NS_CERT_TYPE_OBJECT_SIGNING;
	issuerCert = CERT_FindCertIssuer (cert, PR_Now(), certUsageObjectSigner);
	issuerCN = CERT_GetCommonName (&cert->issuer);

	if (!isSigningCert && list_certs == 1)
	    return (SECSuccess);

	

	if (name) {
	    int	rv;

	    num_trav_certs++;
	    if (list_certs == 2) {
		PR_fprintf(outputFD, "%s ", isSigningCert ? "*" : " ");
	    }
	    PR_fprintf(outputFD, "%s\n", name);

	    if (list_certs == 1) {
		if (issuerCert == NULL) {
		    PR_fprintf(outputFD,
		        "\t++ Error ++ Unable to find issuer certificate\n");
		    return SECSuccess; 
			   
		}
		if (issuerCN == NULL)
		    PR_fprintf(outputFD, "    Issued by: %s\n",
		         issuerCert->nickname);
		else
		    PR_fprintf(outputFD,
		        "    Issued by: %s (%s)\n", issuerCert->nickname,
		         issuerCN);

		expires = DER_TimeChoiceDayToAscii(&cert->validity.notAfter);

		if (expires)
		    PR_fprintf(outputFD, "    Expires: %s\n", expires);

		rv = CERT_CertTimesValid (cert);

		if (rv != SECSuccess)
		    PR_fprintf(outputFD, 
			"    ++ Error ++ THIS CERTIFICATE IS EXPIRED\n");

		if (rv == SECSuccess) {
		    rv = CERT_VerifyCertNow (cert->dbhandle, cert,
		        PR_TRUE, certUsageObjectSigner, &pwdata);

		    if (rv != SECSuccess) {
			rv = PORT_GetError();
			PR_fprintf(outputFD,
			"    ++ Error ++ THIS CERTIFICATE IS NOT VALID (%s)\n",
			     				secErrorString(rv));            
		    }
		}

		expires = DER_TimeChoiceDayToAscii(&issuerCert->validity.notAfter);
		if (expires == NULL) 
		    expires = "(unknown)";

		rv = CERT_CertTimesValid (issuerCert);

		if (rv != SECSuccess)
		    PR_fprintf(outputFD,
		        "    ++ Error ++ ISSUER CERT \"%s\" EXPIRED ON %s\n",
			issuerCert->nickname, expires);

		if (rv == SECSuccess) {
		    rv = CERT_VerifyCertNow (issuerCert->dbhandle, issuerCert, 
		        PR_TRUE, certUsageVerifyCA, &pwdata);
		    if (rv != SECSuccess) {
			rv = PORT_GetError();
			PR_fprintf(outputFD,
			"    ++ Error ++ ISSUER CERT \"%s\" IS NOT VALID (%s)\n",
			     issuerCert->nickname, secErrorString(rv));
		    }
		}
	    }
	}
    }

    return (SECSuccess);
}


