






































#include "nscore.h"

#include "nsISupports.h"
#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "xptinfo.h"
#include "nsServiceManagerUtils.h"

#include <stdio.h>






int main (int argc, char **argv) {
    int i;
    nsIID *iid1, *iid2, *iid3;
    char *name1, *name2, *name3;
    nsIInterfaceInfo *info2, *info3, *info4, *info5;

    nsCOMPtr<nsIInterfaceInfoManager> iim
        (do_GetService(NS_INTERFACEINFOMANAGER_SERVICE_CONTRACTID));

    fprintf(stderr, "\ngetting iid for 'nsISupports'\n");
    iim->GetIIDForName("nsISupports", &iid1);
    iim->GetNameForIID(iid1, &name1);
    fprintf(stderr, "%s iid %s\n", name1, iid1->ToString());

    fprintf(stderr, "\ngetting iid for 'nsIInputStream'\n");
    iim->GetIIDForName("nsIInputStream", &iid2);
    iim->GetNameForIID(iid2, &name2);
    fprintf(stderr, "%s iid %s\n", name2, iid2->ToString());

    fprintf(stderr, "iid: %s, name: %s\n", iid1->ToString(), name1);
    fprintf(stderr, "iid: %s, name: %s\n", iid2->ToString(), name2);

    fprintf(stderr, "\ngetting info for iid2 from above\n");
    iim->GetInfoForIID(iid2, &info2);
#ifdef DEBUG

#endif

    fprintf(stderr, "\ngetting iid for 'nsIInputStream'\n");
    iim->GetIIDForName("nsIInputStream", &iid3);
    iim->GetNameForIID(iid3, &name3);
    fprintf(stderr, "%s iid %s\n", name3, iid2->ToString());
    iim->GetInfoForIID(iid3, &info3);
#ifdef DEBUG

#endif

    fprintf(stderr, "\ngetting info for name 'nsIBidirectionalEnumerator'\n");
    iim->GetInfoForName("nsIBidirectionalEnumerator", &info4);
#ifdef DEBUG

#endif

    fprintf(stderr, "\nparams work?\n");
    fprintf(stderr, "\ngetting info for name 'nsIServiceManager'\n");
    iim->GetInfoForName("nsIServiceManager", &info5);
#ifdef DEBUG

#endif

    
    if (info5 == NULL) {
        fprintf(stderr, "\nNo nsIServiceManager; cannot continue.\n");
        return 1;
    }

    uint16 methodcount;
    info5->GetMethodCount(&methodcount);
    const nsXPTMethodInfo *mi;
    for (i = 0; i < methodcount; i++) {
        info5->GetMethodInfo(i, &mi);
        fprintf(stderr, "method %d, name %s\n", i, mi->GetName());
    }

    
    info5->GetMethodInfo(7, &mi);


    nsXPTParamInfo param2 = mi->GetParam(2);
    
    nsIID *nsISL;
    info5->GetIIDForParam(7, &param2, &nsISL);

    fprintf(stderr, "iid assoc'd with param 2 of method 7 of GetServiceWithListener - %s\n", nsISL->ToString());
    
    char *nsISLname;
    iim->GetNameForIID(nsISL, &nsISLname);
    fprintf(stderr, "which is called %s\n", nsISLname);

    fprintf(stderr, "\nhow about one defined in a different typelib\n");
    nsXPTParamInfo param3 = mi->GetParam(3);
    
    nsIID *nsISS;
    info5->GetIIDForParam(7, &param3, &nsISS);

    fprintf(stderr, "iid assoc'd with param 3 of method 7 of GetServiceWithListener - %s\n", nsISS->ToString());
    
    char *nsISSname;
    iim->GetNameForIID(nsISS, &nsISSname);
    fprintf(stderr, "which is called %s\n", nsISSname);

    return 0;
}    

