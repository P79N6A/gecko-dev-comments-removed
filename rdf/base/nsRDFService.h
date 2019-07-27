


















#ifndef nsRDFService_h__
#define nsRDFService_h__

#include "nsIRDFService.h"
#include "nsWeakReference.h"
#include "nsIFactory.h"
#include "nsCOMPtr.h"
#include "pldhash.h"
#include "nsString.h"

struct PLHashTable;
class nsIRDFLiteral;
class nsIRDFInt;
class nsIRDFDate;
class BlobImpl;

class RDFServiceImpl MOZ_FINAL : public nsIRDFService,
                                 public nsSupportsWeakReference
{
protected:
    PLHashTable* mNamedDataSources;
    PLDHashTable mResources;
    PLDHashTable mLiterals;
    PLDHashTable mInts;
    PLDHashTable mDates;
    PLDHashTable mBlobs;

    nsAutoCString mLastURIPrefix;
    nsCOMPtr<nsIFactory> mLastFactory;
    nsCOMPtr<nsIFactory> mDefaultResourceFactory;

    RDFServiceImpl();
    nsresult Init();
    virtual ~RDFServiceImpl();

public:
    static RDFServiceImpl *gRDFService NS_VISIBILITY_HIDDEN;
    static nsresult CreateSingleton(nsISupports* aOuter,
                                    const nsIID& aIID, void **aResult);

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIRDFSERVICE

    
    nsresult RegisterLiteral(nsIRDFLiteral* aLiteral);
    nsresult UnregisterLiteral(nsIRDFLiteral* aLiteral);
    nsresult RegisterInt(nsIRDFInt* aInt);
    nsresult UnregisterInt(nsIRDFInt* aInt);
    nsresult RegisterDate(nsIRDFDate* aDate);
    nsresult UnregisterDate(nsIRDFDate* aDate);
    nsresult RegisterBlob(BlobImpl* aBlob);
    nsresult UnregisterBlob(BlobImpl* aBlob);

    nsresult GetDataSource(const char *aURI, bool aBlock, nsIRDFDataSource **aDataSource );
};

#endif 
