




































#include <stdio.h>

#include "nsXPCOM.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"

#include "nsFileSpec.h"

#include "nscore.h"
#include "nspr.h"

#include "nsSimpleNotifier.h"




#include "nsISoftwareUpdate.h"
#include "nsSoftwareUpdateIIDs.h"

static nsISoftwareUpdate *softwareUpdate = NULL;
static NS_DEFINE_IID(kSoftwareUpdateCID, NS_SoftwareUpdate_CID);



static void
xpinstall_usage(int argc, char *argv[])
{
    fprintf(stderr, "Usage: %s [-f flags] [-a arguments] filename\n", argv[0]);
}

int
main(int argc, char **argv)
{
 
    for (int i = 1; i < argc; i++) 
    {
        if (argv[i][0] != '-')
            break;

        switch (argv[i][1]) 
        {
            case 'f':
            if (argv[i][2] == '\0' && i == argc) 
            {
                fputs("ERROR: missing path after -f\n", stderr);
                xpinstall_usage(argc, argv);
                return 1;
            }
            
            i++;
            break;

          default:
            fprintf(stderr, "unknown option %s\n", argv[i]);
            xpinstall_usage(argc, argv);
            return 1;
        }
    }



    nsCOMPtr<nsIServiceManager> servMan;
    NS_InitXPCOM2(getter_AddRefs(servMan), nsnull, nsnull);
    nsCOMPtr<nsIComponentRegistrar> registrar = do_QueryInterface(servMan);
    if (!registrar) {
        NS_ASSERTION(0, "Null nsIComponentRegistrar");
        return rv;
    }
    registrar->AutoRegister(nsnull);


    nsresult rv = CallCreateInstance(kSoftwareUpdateCID, &softwareUpdate);

    if (NS_SUCCEEDED(rv))
    {
        nsSimpleNotifier *progress = new nsSimpleNotifier();

        nsFileSpec jarFile(argv[i]);
        nsFileURL jarFileURL(jarFile);
        
        softwareUpdate->InstallJar(  nsString( jarFileURL.GetAsString() ) ,
                                     nsString( nsNSPRPath(jarFile) ), 
                                     0x0000FFFF,
                                     progress);
    }

    return rv;
}
