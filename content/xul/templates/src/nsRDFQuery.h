



































#ifndef nsRDFQuery_h__
#define nsRDFQuery_h__

#include "nsAutoPtr.h"
#include "nsISimpleEnumerator.h"
#include "nsCycleCollectionParticipant.h"

#define NS_ITEMPLATERDFQUERY_IID \
  {0x8929ff60, 0x1c9c, 0x4d87, \
    { 0xac, 0x02, 0x09, 0x14, 0x15, 0x3b, 0x48, 0xc4 }}





class nsITemplateRDFQuery : public nsISupports
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_ITEMPLATERDFQUERY_IID)

    
    virtual nsXULTemplateQueryProcessorRDF* Processor() = 0;  

    
    virtual nsIAtom* GetMemberVariable() = 0; 

    
    virtual void GetQueryNode(nsIDOMNode** aQueryNode) = 0;

    
    virtual void ClearCachedResults() = 0;
};

class nsRDFQuery : public nsITemplateRDFQuery
{
public:

    nsRDFQuery(nsXULTemplateQueryProcessorRDF* aProcessor)
      : mProcessor(aProcessor),
        mSimple(PR_FALSE),
        mRoot(nsnull),
        mCachedResults(nsnull)
    { }

    ~nsRDFQuery() { Finish(); }

    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS(nsRDFQuery)

    



    TestNode* GetRoot() { return mRoot; };

    void SetRoot(TestNode* aRoot) { mRoot = aRoot; };

    void GetQueryNode(nsIDOMNode** aQueryNode)
    {
       *aQueryNode = mQueryNode;
       NS_IF_ADDREF(*aQueryNode);
    }

    void SetQueryNode(nsIDOMNode* aQueryNode)
    {
       mQueryNode = aQueryNode;
    }

    
    
    
    
    
    nsresult SetCachedResults(nsXULTemplateQueryProcessorRDF* aProcessor,
                              const InstantiationSet& aInstantiations);

    
    
    
    void UseCachedResults(nsISimpleEnumerator** aResults);

    
    void ClearCachedResults()
    {
        mCachedResults = nsnull;
    }

    nsXULTemplateQueryProcessorRDF* Processor() { return mProcessor; }

    nsIAtom* GetMemberVariable() { return mMemberVariable; }

    PRBool IsSimple() { return mSimple; }

    void SetSimple() { mSimple = PR_TRUE; }

    
    nsCOMPtr<nsIAtom> mRefVariable;
    nsCOMPtr<nsIAtom> mMemberVariable;

protected:

    nsXULTemplateQueryProcessorRDF* mProcessor;

    
    PRBool mSimple;

    


    TestNode *mRoot;

    
    nsCOMPtr<nsIDOMNode> mQueryNode;

    
    nsCOMPtr<nsISimpleEnumerator> mCachedResults;

    void Finish();
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsRDFQuery, NS_ITEMPLATERDFQUERY_IID)

#endif 
