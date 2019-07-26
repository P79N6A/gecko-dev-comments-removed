









#include <errno.h>
#include <stdio.h>
#include "secport.h"
#include "ssl.h"

int main(int argc, char **argv)
{
    const PRUint16 *cipherSuites = SSL_ImplementedCiphers;
    int i;
    int errCount = 0;

    fputs("This version of libSSL supports these cipher suites:\n\n", stdout);

    
    for (i = 0; i < SSL_NumImplementedCiphers; i++) {
	PRUint16  suite = cipherSuites[i];
	SECStatus rv;
	PRBool    enabled;
	PRErrorCode err;
	SSLCipherSuiteInfo info; 

        rv = SSL_CipherPrefGetDefault(suite, &enabled);
	if (rv != SECSuccess) {
	    err = PR_GetError();
	    ++errCount;
	    fprintf(stderr,
	    "SSL_CipherPrefGetDefault didn't like value 0x%04x (i = %d): %s\n",
	    	   suite, i, PORT_ErrorToString(err));
	    continue;
	} 
	rv = SSL_GetCipherSuiteInfo(suite, &info, (int)(sizeof info));
	if (rv != SECSuccess) {
	    err = PR_GetError();
	    ++errCount;
	    fprintf(stderr,
	    "SSL_GetCipherSuiteInfo didn't like value 0x%04x (i = %d): %s\n",
	    	   suite, i, PORT_ErrorToString(err));
	    continue;
	}
	fprintf(stdout, 
		"%s:\n" 
		"  0x%04hx %-5s %-5s %-8s %3hd %-6s %-8s %-4s %-8s %-11s\n",
		info.cipherSuiteName, info.cipherSuite, 
		info.keaTypeName, info.authAlgorithmName, info.symCipherName, 
		info.effectiveKeyBits, info.macAlgorithmName, 
		enabled           ? "Enabled"     : "Disabled",
		info.isFIPS       ? "FIPS" : 
		  (SSL_IS_SSL2_CIPHER(info.cipherSuite) ? "SSL2" : ""),
		info.isExportable ? "Export"      : "Domestic",
		info.nonStandard  ? "nonStandard" : "");
    }
    return errCount;
}
