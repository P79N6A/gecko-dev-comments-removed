



































#include <plarena.h>
#include <prio.h>
#include <prprf.h>
#include <seccomon.h>
#include <secmod.h>
#include <jar.h>
#include <secutil.h>





static char* PR_fgets(char *buf, int size, PRFileDesc *file);





int
Pk11Install_AddNewModule(char* moduleName, char* dllPath,
                              unsigned long defaultMechanismFlags,
                              unsigned long cipherEnableFlags)
{
	return (SECMOD_AddNewModule(moduleName, dllPath,
		SECMOD_PubMechFlagstoInternal(defaultMechanismFlags),
		SECMOD_PubCipherFlagstoInternal(cipherEnableFlags))
													== SECSuccess) ? 0 : -1;
}











short
Pk11Install_UserVerifyJar(JAR *jar, PRFileDesc *out, PRBool query)
{
	JAR_Context *ctx;
	JAR_Cert *fing;
	JAR_Item *item;
	char stdinbuf[80];
	int count=0;

	CERTCertificate *cert, *prev=NULL;

	PR_fprintf(out, "\nThis installation JAR file was signed by:\n");

	ctx = JAR_find(jar, NULL, jarTypeSign);

	while(JAR_find_next(ctx, &item) >= 0 ) {
		fing = (JAR_Cert*) item->data;
		cert = fing->cert;
		if(cert==prev) {
			continue;
		}

		count++;
		PR_fprintf(out, "----------------------------------------------\n");
		if(cert) {
			if(cert->nickname) {
				PR_fprintf(out, "**NICKNAME**\n%s\n", cert->nickname);
			}
			if(cert->subjectName) {
				PR_fprintf(out, "**SUBJECT NAME**\n%s\n", cert->subjectName); }
			if(cert->issuerName) {
				PR_fprintf(out, "**ISSUER NAME**\n%s\n", cert->issuerName);
			}
		} else {
			PR_fprintf(out, "No matching certificate could be found.\n");
		}
		PR_fprintf(out, "----------------------------------------------\n\n");

		prev=cert;
	}

	JAR_find_end(ctx);

	if(count==0) {
		PR_fprintf(out, "No signatures found: JAR FILE IS UNSIGNED.\n");
	}

	if(query) {
		PR_fprintf(out,
"Do you wish to continue this installation? (y/n) ");

		if(PR_fgets(stdinbuf, 80, PR_STDIN) != NULL) {
			char *response;

			if( (response=strtok(stdinbuf, " \t\n\r")) ) {
				if( !PL_strcasecmp(response, "y") ||
					!PL_strcasecmp(response, "yes") ) {
					return 0;
				}
			}
		}
	}

	return 1;
}







static char*
PR_fgets(char *buf, int size, PRFileDesc *file)
{
    int i;
    int status;
    char c;

    i=0;
    while(i < size-1) {
        status = PR_Read(file, (void*) &c, 1);
        if(status==-1) {
            return NULL;
        } else if(status==0) {
            break;
        }
        buf[i++] = c;
        if(c=='\n') {
            break;
        }
    }
    buf[i]='\0';

    return buf;
}






const char* mySECU_ErrorString(int16 errnum)
{
	return SECU_Strerror(errnum);
}
