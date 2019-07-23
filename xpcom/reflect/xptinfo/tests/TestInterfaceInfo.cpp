






































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
    iim->GetInfoForName("nsIComponentManager", &info5);
#ifdef DEBUG

#endif

    
    if (info5 == NULL) {
        fprintf(stderr, "\nNo nsIComponentManager; cannot continue.\n");
        return 1;
    }

    uint16 methodcount;
    info5->GetMethodCount(&methodcount);
    const nsXPTMethodInfo *mi;
    for (i = 0; i < methodcount; i++) {
        info5->GetMethodInfo(i, &mi);
        fprintf(stderr, "method %d, name %s\n", i, mi->GetName());
    }

    
    info5->GetMethodInfo(6, &mi);

    const nsXPTParamInfo& param2 = mi->GetParam(1);
    
    nsIID *nsISL;
    info5->GetIIDForParam(6, &param2, &nsISL);
    fprintf(stderr, "iid assoc'd with param 1 of method 6 - createInstanceByContractID - %s\n", nsISL->ToString());
    
    char *nsISLname;
    iim->GetNameForIID(nsISL, &nsISLname);
    fprintf(stderr, "which is called %s\n", nsISLname);

    fprintf(stderr, "\nNow check the last param\n");
    const nsXPTParamInfo& param3 = mi->GetParam(3);

    if (param3.GetType().TagPart() != nsXPTType::T_INTERFACE_IS) {
        fprintf(stderr, "Param 3 is not type interface is\n");
        
    }
    
    uint8 argnum;
    info5->GetInterfaceIsArgNumberForParam(6, &param3, &argnum);
    fprintf(stderr, "param 3 referrs to param %d of method 6 - createInstanceByContractID\n", (PRUint32)argnum);
    
    const nsXPTParamInfo& arg_param = mi->GetParam(argnum);
    const nsXPTType& arg_type = arg_param.GetType();
    
    if(!arg_type.IsPointer() || arg_type.TagPart() != nsXPTType::T_IID) {
        fprintf(stderr, "Param 3 of method 6 refers to a non IID parameter\n"); 
        
    }


    return 0;
}    

