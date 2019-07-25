


































#include "prerror.h"
#include "secerr.h"
#include "secport.h"
#include "prinit.h"
#include "prprf.h"
#include "prtypes.h"
#include "prlog.h"
#include "plstr.h"
#include "nssutil.h"
#include <string.h>

#define ER3(name, value, str) {#name, str},

static const struct PRErrorMessage sectext[] = {
#include "SECerrs.h"
    {0,0}
};

static const struct PRErrorTable sec_et = {
    sectext, "secerrstrings", SEC_ERROR_BASE, 
        (sizeof sectext)/(sizeof sectext[0]) 
};

static PRStatus 
nss_InitializePRErrorTableOnce(void) {
    return PR_ErrorInstallTable(&sec_et);
}

static PRCallOnceType once;

PRStatus
NSS_InitializePRErrorTable(void)
{
    return PR_CallOnce(&once, nss_InitializePRErrorTableOnce);
}









static char *
nss_Strerror(PRErrorCode errNum)
{
    static int initDone;

    if (!initDone) {
    
    PRStatus rv = NSS_InitializePRErrorTable();
    
    if (rv != PR_SUCCESS) return NULL;
	initDone = 1;
    }

    return (char *) PR_ErrorToString(errNum, PR_LANGUAGE_I_DEFAULT);
}


#define EBUFF_SIZE 512
static char ebuf[EBUFF_SIZE];



















char *
NSS_Strerror(PRErrorCode errNum, ReportFormatType format)
{
    PRUint32 count;
    char *errname = (char *) PR_ErrorToName(errNum);
    char *errstr = nss_Strerror(errNum);

    if (!errstr) return NULL;

    if (format == formatSimple) return errstr;

    count = PR_snprintf(ebuf, EBUFF_SIZE, "[%d %s] %s",
	errNum, errname, errstr);

    PR_ASSERT(count != -1);

    return ebuf;
}






char *
NSS_StrerrorTS(PRErrorCode errNum, ReportFormatType format)
{
    char *errstr = NSS_Strerror(errNum, format);

    return PR_smprintf("[%d %s] %s",
	errNum, PR_ErrorToName(errNum), errstr ? errstr : "");
}













void
NSS_Perror(const char *s, ReportFormatType format)
{
    PRErrorCode err;
    char *errString;

    if (!s || PORT_Strlen(s) == 0) {
	return;
    }

    err = PORT_GetError();
    errString = NSS_Strerror(err, format);

    fprintf(stderr, "%s: ", s);

    if (errString != NULL && PORT_Strlen(errString) > 0) {
	fprintf(stderr, "%s\n", errString);
    } else {
	fprintf(stderr, "Unknown error: %d\n", (int)err);
    }
}
