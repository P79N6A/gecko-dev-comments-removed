




































#ifndef nsRDFResource_h__
#define nsRDFResource_h__

#include "nsCOMPtr.h"
#include "nsIRDFNode.h"
#include "nsIRDFResource.h"
#include "nscore.h"
#include "nsStringGlue.h"
#include "rdf.h"

class nsIRDFService;





class nsRDFResource : public nsIRDFResource {
public:

    NS_DECL_ISUPPORTS

    
    NS_IMETHOD EqualsNode(nsIRDFNode* aNode, PRBool* aResult);

    
    NS_IMETHOD Init(const char* aURI);
    NS_IMETHOD GetValue(char* *aURI);
    NS_IMETHOD GetValueUTF8(nsACString& aResult);
    NS_IMETHOD GetValueConst(const char** aURI);
    NS_IMETHOD EqualsString(const char* aURI, PRBool* aResult);
    NS_IMETHOD GetDelegate(const char* aKey, REFNSIID aIID, void** aResult);
    NS_IMETHOD ReleaseDelegate(const char* aKey);

    
    nsRDFResource(void);
    virtual ~nsRDFResource(void);

protected:
    static nsIRDFService* gRDFService;
    static nsrefcnt gRDFServiceRefCnt;

protected:
    nsCString mURI;

    struct DelegateEntry {
        nsCString             mKey;
        nsCOMPtr<nsISupports> mDelegate;
        DelegateEntry*        mNext;
    };

    DelegateEntry* mDelegates;
};

#endif 
