









































#include "nspr.h"
#include "secutil.h"
#include "plgetopt.h"
#include "secpkcs7.h"
#include "cert.h"
#include "certdb.h"
#include "nss.h"
#include "pk11pub.h"

#if defined(XP_UNIX)
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

#if (defined(XP_WIN) && !defined(WIN32)) || (defined(__sun) && !defined(SVR4))
extern int fwrite(char *, size_t, size_t, FILE*);
extern int fprintf(FILE *, char *, ...);
#endif



static void
Usage(char *progName)
{
    fprintf(stderr,
	    "Usage:  %s [-d dbdir] [-i input] [-o output]\n",
	    progName);
    fprintf(stderr,
	    "%-20s Key/Cert database directory (default is ~/.netscape)\n",
	    "-d dbdir");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    exit(-1);
}

static PRBool saw_content;

static void
PrintBytes(void *arg, const char *buf, unsigned long len)
{
    FILE *out;

    out = arg; 
    fwrite (buf, len, 1, out);

    saw_content = PR_TRUE;
}







static PRBool
decryption_allowed(SECAlgorithmID *algid, PK11SymKey *key)
{
    return PR_TRUE;
}

char* KeyDbPassword = 0;


char* MyPK11PasswordFunc (PK11SlotInfo *slot, PRBool retry, void* arg)
{
    char *ret=0;

    if (retry == PR_TRUE)
        return NULL;
    ret = PL_strdup (KeyDbPassword);
    return ret;
}

int
DecodeAndPrintFile(FILE *out, PRFileDesc *in, char *progName)
{
    SECItem derdata;
    SEC_PKCS7ContentInfo *cinfo = NULL;
    SEC_PKCS7DecoderContext *dcx;

    if (SECU_ReadDERFromFile(&derdata, in, PR_FALSE)) {
        SECU_PrintError(progName, "error converting der");
	return -1;
    }

    fprintf(out,
	    "Content printed between bars (newline added before second bar):");
    fprintf(out, "\n---------------------------------------------\n");

    saw_content = PR_FALSE;
    dcx = SEC_PKCS7DecoderStart(PrintBytes, out, NULL, NULL,
				NULL, NULL, decryption_allowed);
    if (dcx != NULL) {
#if 0	
	{
	    unsigned long i;
	    for (i = 0; i < derdata.len; i++)
		SEC_PKCS7DecoderUpdate(dcx, derdata.data + i, 1);
	}
#else
	SEC_PKCS7DecoderUpdate(dcx, (char *)derdata.data, derdata.len);
#endif
	cinfo = SEC_PKCS7DecoderFinish(dcx);
    }

    fprintf(out, "\n---------------------------------------------\n");

    if (cinfo == NULL)
	return -1;

    fprintf(out, "Content was%s encrypted.\n",
	    SEC_PKCS7ContentIsEncrypted(cinfo) ? "" : " not");

    if (SEC_PKCS7ContentIsSigned(cinfo)) {
	char *signer_cname, *signer_ename;
	SECItem *signing_time;

	if (saw_content) {
	    fprintf(out, "Signature is ");
	    PORT_SetError(0);
	    if (SEC_PKCS7VerifySignature(cinfo, certUsageEmailSigner, PR_FALSE))
		fprintf(out, "valid.\n");
	    else
		fprintf(out, "invalid (Reason: %s).\n",
			SECU_Strerror(PORT_GetError()));
	} else {
	    fprintf(out,
		    "Content is detached; signature cannot be verified.\n");
	}

	signer_cname = SEC_PKCS7GetSignerCommonName(cinfo);
	if (signer_cname != NULL) {
	    fprintf(out, "The signer's common name is %s\n", signer_cname);
	    PORT_Free(signer_cname);
	} else {
	    fprintf(out, "No signer common name.\n");
	}

	signer_ename = SEC_PKCS7GetSignerEmailAddress(cinfo);
	if (signer_ename != NULL) {
	    fprintf(out, "The signer's email address is %s\n", signer_ename);
	    PORT_Free(signer_ename);
	} else {
	    fprintf(out, "No signer email address.\n");
	}

	signing_time = SEC_PKCS7GetSigningTime(cinfo);
	if (signing_time != NULL) {
	    SECU_PrintTimeChoice(out, signing_time, "Signing time", 0);
	} else {
	    fprintf(out, "No signing time included.\n");
	}
    } else {
	fprintf(out, "Content was not signed.\n");
    }

    fprintf(out, "There were%s certs or crls included.\n",
	    SEC_PKCS7ContainsCertsOrCrls(cinfo) ? "" : " no");

    SEC_PKCS7DestroyContentInfo(cinfo);
    return 0;
}






int
main(int argc, char **argv)
{
    char *progName;
    FILE *outFile;
    PRFileDesc *inFile;
    PLOptState *optstate;
    PLOptStatus status;
    SECStatus rv;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    inFile = NULL;
    outFile = NULL;

    


    optstate = PL_CreateOptState(argc, argv, "d:i:o:p:");
    while ((status = PL_GetNextOpt(optstate)) == PL_OPT_OK) {
	switch (optstate->option) {
	  case 'd':
	    SECU_ConfigDirectory(optstate->value);
	    break;

	  case 'i':
	    inFile = PR_Open(optstate->value, PR_RDONLY, 0);
	    if (!inFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
			progName, optstate->value);
		return -1;
	    }
	    break;

	  case 'o':
	    outFile = fopen(optstate->value, "w");
	    if (!outFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for writing\n",
			progName, optstate->value);
		return -1;
	    }
	    break;

	  case 'p':
            KeyDbPassword = strdup (optstate->value);
            break;

	  default:
	    Usage(progName);
	    break;
	}
    }
    if (status == PL_OPT_BAD)
	Usage(progName);

    if (!inFile) inFile = PR_STDIN;
    if (!outFile) outFile = stdout;

    
    PR_Init(PR_SYSTEM_THREAD, PR_PRIORITY_NORMAL, 1);
    rv = NSS_Init(SECU_ConfigDirectory(NULL));
    if (rv != SECSuccess) {
	SECU_PrintPRandOSError(progName);
	return -1;
    }

    PK11_SetPasswordFunc (MyPK11PasswordFunc);

    if (DecodeAndPrintFile(outFile, inFile, progName)) {
	SECU_PrintError(progName, "problem decoding data");
	return -1;
    }
    
    if (NSS_Shutdown() != SECSuccess) {
        exit(1);
    }

    return 0;
}
