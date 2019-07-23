





































#ifndef nsXPathResult_h__
#define nsXPathResult_h__

#include "txExprResult.h"
#include "nsIDOMXPathResult.h"
#include "nsIDocument.h"
#include "nsStubMutationObserver.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"


#define NS_IXPATHRESULT_IID \
{ 0x15b9b301, 0x2012, 0x11d6, {0xa7, 0xf2, 0xe6, 0xd0, 0xa6, 0x78, 0x99, 0x5c }}

class nsIXPathResult : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXPATHRESULT_IID)
    virtual nsresult SetExprResult(txAExprResult *aExprResult,
                                   PRUint16 aResultType) = 0;
    virtual nsresult GetExprResult(txAExprResult **aExprResult) = 0;
    virtual nsresult Clone(nsIXPathResult **aResult) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIXPathResult, NS_IXPATHRESULT_IID)





class txResultHolder
{
public:
    ~txResultHolder()
    {
      releaseNodeSet();
    }

    txAExprResult *get()
    {
        return mResult;
    }
    void set(txAExprResult *aResult);

private:
    void releaseNodeSet();

    nsRefPtr<txAExprResult> mResult;
};




class nsXPathResult : public nsIDOMXPathResult,
                      public nsStubMutationObserver,
                      public nsIXPathResult
{
public:
    nsXPathResult();
    virtual ~nsXPathResult();

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIDOMXPATHRESULT

    
    NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
    NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
    NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED
    NS_DECL_NSIMUTATIONOBSERVER_NODEWILLBEDESTROYED

    
    nsresult SetExprResult(txAExprResult *aExprResult, PRUint16 aResultType);
    nsresult GetExprResult(txAExprResult **aExprResult);
    nsresult Clone(nsIXPathResult **aResult);

private:
    PRBool isSnapshot() const
    {
        return mResultType == UNORDERED_NODE_SNAPSHOT_TYPE ||
               mResultType == ORDERED_NODE_SNAPSHOT_TYPE;
    }
    PRBool isIterator() const
    {
        return mResultType == UNORDERED_NODE_ITERATOR_TYPE ||
               mResultType == ORDERED_NODE_ITERATOR_TYPE;
    }
    PRBool isNode() const
    {
        return mResultType == FIRST_ORDERED_NODE_TYPE ||
               mResultType == ANY_UNORDERED_NODE_TYPE;
    }

    void Invalidate();

    txResultHolder mResult;
    nsCOMPtr<nsIDocument> mDocument;
    PRUint32 mCurrentPos;
    PRUint16 mResultType;
    PRPackedBool mInvalidIteratorState;
};

#endif
