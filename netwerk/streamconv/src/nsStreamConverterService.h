




































#ifndef __nsstreamconverterservice__h___
#define __nsstreamconverterservice__h___

#include "nsIStreamConverterService.h"
#include "nsIStreamListener.h"
#include "nsHashtable.h"
#include "nsCOMArray.h"
#include "nsIAtom.h"

class nsStreamConverterService : public nsIStreamConverterService {
public:    
    
    
    NS_DECL_ISUPPORTS


    
    
    NS_DECL_NSISTREAMCONVERTERSERVICE

    
    
    nsStreamConverterService();
    virtual ~nsStreamConverterService();

    
    nsresult Init();

private:
    
    nsresult FindConverter(const char *aContractID, nsCStringArray **aEdgeList);
    nsresult BuildGraph(void);
    nsresult AddAdjacency(const char *aContractID);
    nsresult ParseFromTo(const char *aContractID, nsCString &aFromRes, nsCString &aToRes);

    
    nsObjectHashtable              *mAdjacencyList;
};





enum BFScolors {white, gray, black};

struct BFSState {
    BFScolors   color;
    PRInt32     distance;
    nsCStringKey  *predecessor;
    ~BFSState() {
        delete predecessor;
    }
};


struct SCTableData {
    nsCStringKey *key;
    union _data {
        BFSState *state;
        nsCOMArray<nsIAtom> *edges;
    } data;

    SCTableData(nsCStringKey* aKey) : key(aKey) {
        data.state = nsnull;
    }
};
#endif 
