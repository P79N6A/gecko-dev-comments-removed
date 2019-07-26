




#ifndef __nsstreamconverterservice__h___
#define __nsstreamconverterservice__h___

#include "nsIStreamConverterService.h"

#include "nsClassHashtable.h"
#include "nsCOMArray.h"
#include "nsTArrayForwardDeclare.h"

class nsCString;
class nsIAtom;

class nsStreamConverterService : public nsIStreamConverterService {
public:
    
    
    NS_DECL_ISUPPORTS


    
    
    NS_DECL_NSISTREAMCONVERTERSERVICE

    
    
    nsStreamConverterService();
    virtual ~nsStreamConverterService();

private:
    
    nsresult FindConverter(const char *aContractID, nsTArray<nsCString> **aEdgeList);
    nsresult BuildGraph(void);
    nsresult AddAdjacency(const char *aContractID);
    nsresult ParseFromTo(const char *aContractID, nsCString &aFromRes, nsCString &aToRes);

    
    nsClassHashtable<nsCStringHashKey, nsCOMArray<nsIAtom>> mAdjacencyList;
};

#endif 
