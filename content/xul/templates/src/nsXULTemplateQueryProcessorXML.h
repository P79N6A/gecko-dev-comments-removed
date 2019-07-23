



































#ifndef nsXULTemplateQueryProcessorXML_h__
#define nsXULTemplateQueryProcessorXML_h__

#include "nsIXULTemplateQueryProcessor.h"

#include "nsISimpleEnumerator.h"
#include "nsString.h"
#include "nsCOMArray.h"
#include "nsRefPtrHashtable.h"
#include "nsIDOMElement.h"
#include "nsIDOMXPathExpression.h"
#include "nsIDOMXPathEvaluator.h"
#include "nsIDOMXPathResult.h"
#include "nsXMLBinding.h"

class nsXULTemplateQueryProcessorXML;

#define NS_IXMLQUERY_IID \
  {0x0358d692, 0xccce, 0x4a97, \
    { 0xb2, 0x51, 0xba, 0x8f, 0x17, 0x0f, 0x3b, 0x6f }}
 
class nsXMLQuery : public nsISupports
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

class nsXULTemplateResultSetXML : public nsISimpleEnumerator
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

class nsXULTemplateQueryProcessorXML : public nsIXULTemplateQueryProcessor
{
public:

    nsXULTemplateQueryProcessorXML()
        : mGenerationStarted(PR_FALSE)
    {}

    
    NS_DECL_ISUPPORTS

    
    NS_DECL_NSIXULTEMPLATEQUERYPROCESSOR

    nsXMLBindingSet*
    GetOptionalBindingsForRule(nsIDOMNode* aRuleNode);

    
    
    nsresult
    CreateExpression(const nsAString& aExpr,
                     nsIDOMNode* aNode,
                     nsIDOMXPathExpression** aCompiledExpr);

private:

    PRBool mGenerationStarted;

    nsRefPtrHashtable<nsISupportsHashKey, nsXMLBindingSet> mRuleToBindingsMap;

    nsCOMPtr<nsIDOMElement> mRoot;

    nsCOMPtr<nsIDOMXPathEvaluator> mEvaluator;
};


#endif 
