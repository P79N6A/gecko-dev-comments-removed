




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

    NS_DECL_THREADSAFE_ISUPPORTS

    
    NS_IMETHOD EqualsNode(nsIRDFNode* aNode, bool* aResult) override;

    
    NS_IMETHOD Init(const char* aURI) override;
    NS_IMETHOD GetValue(char* *aURI) override;
    NS_IMETHOD GetValueUTF8(nsACString& aResult) override;
    NS_IMETHOD GetValueConst(const char** aURI) override;
    NS_IMETHOD EqualsString(const char* aURI, bool* aResult) override;
    NS_IMETHOD GetDelegate(const char* aKey, REFNSIID aIID, void** aResult) override;
    NS_IMETHOD ReleaseDelegate(const char* aKey) override;

    
    nsRDFResource(void);

protected:
    virtual ~nsRDFResource(void);
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
