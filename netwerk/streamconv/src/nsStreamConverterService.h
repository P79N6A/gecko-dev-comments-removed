




#ifndef __nsstreamconverterservice__h___
#define __nsstreamconverterservice__h___

#include "nsIStreamConverterService.h"

template<class T> class nsTArray;
class nsObjectHashtable;
class nsCString;

class nsStreamConverterService : public nsIStreamConverterService {
public:    
    
    
    NS_DECL_ISUPPORTS


    
    
    NS_DECL_NSISTREAMCONVERTERSERVICE

    
    
    nsStreamConverterService();
    virtual ~nsStreamConverterService();

    
    nsresult Init();

private:
    
    nsresult FindConverter(const char *aContractID, nsTArray<nsCString> **aEdgeList);
    nsresult BuildGraph(void);
    nsresult AddAdjacency(const char *aContractID);
    nsresult ParseFromTo(const char *aContractID, nsCString &aFromRes, nsCString &aToRes);

    
    nsObjectHashtable              *mAdjacencyList;
};

#endif 
