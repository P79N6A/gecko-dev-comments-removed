




#ifndef nsXULTemplateQueryProcessorXML_h__
#define nsXULTemplateQueryProcessorXML_h__

#include "nsIXULTemplateBuilder.h"
#include "nsIXULTemplateQueryProcessor.h"

#include "nsISimpleEnumerator.h"
#include "nsString.h"
#include "nsCOMArray.h"
#include "nsRefPtrHashtable.h"
#include "nsIDOMElement.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMXPathExpression.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIDOMXPathResult.h"
#include "nsXMLBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIXMLHttpRequest.h"
#include "mozilla/Attributes.h"

class nsXULTemplateQueryProcessorXML;

#define NS_IXMLQUERY_IID \
  {0x0358d692, 0xccce, 0x4a97, \
    { 0xb2, 0x51, 0xba, 0x8f, 0x17, 0x0f, 0x3b, 0x6f }}
 
class nsXMLQuery MOZ_FINAL : public nsISupports
{
  public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_IXMLQUERY_IID)

    NS_DECL_ISUPPORTS

    
    nsXULTemplateQueryProcessorXML* Processor() { return mProcessor; }

    
    nsIAtom* GetMemberVariable() { return mMemberVariable; }

    
    nsIDOMXPathExpression* GetResultsExpression() { return mResultsExpr; }

    
    nsXMLBindingSet* GetBindingSet() { return mRequiredBindings; }

    
    nsresult
    AddBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr)
    {
        if (!mRequiredBindings) {
            mRequiredBindings = new nsXMLBindingSet();
            NS_ENSURE_TRUE(mRequiredBindings, NS_ERROR_OUT_OF_MEMORY);
        }

        return mRequiredBindings->AddBinding(aVar, aExpr);
    }

    nsXMLQuery(nsXULTemplateQueryProcessorXML* aProcessor,
                        nsIAtom* aMemberVariable,
                        nsIDOMXPathExpression* aResultsExpr)
        : mProcessor(aProcessor),
          mMemberVariable(aMemberVariable),
          mResultsExpr(aResultsExpr)
    { }

  protected:
    nsXULTemplateQueryProcessorXML* mProcessor;

    nsCOMPtr<nsIAtom> mMemberVariable;

    nsCOMPtr<nsIDOMXPathExpression> mResultsExpr;

    nsRefPtr<nsXMLBindingSet> mRequiredBindings;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXMLQuery, NS_IXMLQUERY_IID)

class nsXULTemplateResultSetXML MOZ_FINAL : public nsISimpleEnumerator
{
private:

    
    nsCOMPtr<nsXMLQuery> mQuery;

    
    nsRefPtr<nsXMLBindingSet> mBindingSet;

    
    nsCOMPtr<nsIDOMXPathResult> mResults;

    
    PRUint32 mPosition;

public:

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    nsXULTemplateResultSetXML(nsXMLQuery* aQuery,
                              nsIDOMXPathResult* aResults,
                              nsXMLBindingSet* aBindingSet)
        : mQuery(aQuery),
          mBindingSet(aBindingSet),
          mResults(aResults),
          mPosition(0)
    {}
};

class nsXULTemplateQueryProcessorXML MOZ_FINAL : public nsIXULTemplateQueryProcessor,
                                                 public nsIDOMEventListener
{
public:

    nsXULTemplateQueryProcessorXML()
        : mGenerationStarted(false)
    {}

    
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsXULTemplateQueryProcessorXML,
                                             nsIXULTemplateQueryProcessor)

    
    NS_DECL_NSIXULTEMPLATEQUERYPROCESSOR

    
    NS_DECL_NSIDOMEVENTLISTENER

    nsXMLBindingSet*
    GetOptionalBindingsForRule(nsIDOMNode* aRuleNode);

    
    
    nsresult
    CreateExpression(const nsAString& aExpr,
                     nsIDOMNode* aNode,
                     nsIDOMXPathExpression** aCompiledExpr);

private:

    bool mGenerationStarted;

    nsRefPtrHashtable<nsISupportsHashKey, nsXMLBindingSet> mRuleToBindingsMap;

    nsCOMPtr<nsIDOMElement> mRoot;

    nsCOMPtr<nsIDOMXPathEvaluator> mEvaluator;

    nsCOMPtr<nsIXULTemplateBuilder> mTemplateBuilder;

    nsCOMPtr<nsIXMLHttpRequest> mRequest;
};


#endif 
