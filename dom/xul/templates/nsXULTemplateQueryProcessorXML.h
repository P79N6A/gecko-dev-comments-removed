




#ifndef nsXULTemplateQueryProcessorXML_h__
#define nsXULTemplateQueryProcessorXML_h__

#include "nsIXULTemplateBuilder.h"
#include "nsIXULTemplateQueryProcessor.h"

#include "nsISimpleEnumerator.h"
#include "nsString.h"
#include "nsCOMArray.h"
#include "nsRefPtrHashtable.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsXMLBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIXMLHttpRequest.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/XPathEvaluator.h"
#include "mozilla/dom/XPathResult.h"

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

    
    mozilla::dom::XPathExpression* GetResultsExpression()
      { return mResultsExpr; }

    
    nsXMLBindingSet* GetBindingSet() { return mRequiredBindings; }

    
    void
    AddBinding(nsIAtom* aVar, nsAutoPtr<mozilla::dom::XPathExpression>&& aExpr)
    {
        if (!mRequiredBindings) {
            mRequiredBindings = new nsXMLBindingSet();
        }

        mRequiredBindings->AddBinding(aVar, mozilla::Move(aExpr));
    }

    nsXMLQuery(nsXULTemplateQueryProcessorXML* aProcessor,
               nsIAtom* aMemberVariable,
               nsAutoPtr<mozilla::dom::XPathExpression>&& aResultsExpr)
        : mProcessor(aProcessor),
          mMemberVariable(aMemberVariable),
          mResultsExpr(aResultsExpr)
    { }

  protected:
    ~nsXMLQuery() {}

    nsXULTemplateQueryProcessorXML* mProcessor;

    nsCOMPtr<nsIAtom> mMemberVariable;

    nsAutoPtr<mozilla::dom::XPathExpression> mResultsExpr;

    nsRefPtr<nsXMLBindingSet> mRequiredBindings;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsXMLQuery, NS_IXMLQUERY_IID)

class nsXULTemplateResultSetXML MOZ_FINAL : public nsISimpleEnumerator
{
private:

    
    nsCOMPtr<nsXMLQuery> mQuery;

    
    nsRefPtr<nsXMLBindingSet> mBindingSet;

    
    nsRefPtr<mozilla::dom::XPathResult> mResults;

    
    uint32_t mPosition;

    ~nsXULTemplateResultSetXML() {}

public:

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSISIMPLEENUMERATOR

    nsXULTemplateResultSetXML(nsXMLQuery* aQuery,
                              already_AddRefed<mozilla::dom::XPathResult> aResults,
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

    
    
    mozilla::dom::XPathExpression*
    CreateExpression(const nsAString& aExpr,
                     nsINode* aNode,
                     mozilla::ErrorResult& aRv);

private:

    ~nsXULTemplateQueryProcessorXML() {}

    bool mGenerationStarted;

    nsRefPtrHashtable<nsISupportsHashKey, nsXMLBindingSet> mRuleToBindingsMap;

    nsCOMPtr<mozilla::dom::Element> mRoot;

    nsRefPtr<mozilla::dom::XPathEvaluator> mEvaluator;

    nsCOMPtr<nsIXULTemplateBuilder> mTemplateBuilder;

    nsCOMPtr<nsIXMLHttpRequest> mRequest;
};


#endif 
