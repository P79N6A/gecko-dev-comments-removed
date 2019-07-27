




#ifndef nsXMLBinding_h__
#define nsXMLBinding_h__

#include "nsAutoPtr.h"
#include "nsIAtom.h"
#include "mozilla/Attributes.h"

class nsINode;
class nsXULTemplateResultXML;
class nsXMLBindingValues;
namespace mozilla {
namespace dom {
class XPathResult;
}
}








struct nsXMLBinding {
  nsCOMPtr<nsIAtom> mVar;
  nsCOMPtr<nsIDOMXPathExpression> mExpr;

  nsAutoPtr<nsXMLBinding> mNext;

  nsXMLBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr)
    : mVar(aVar), mExpr(aExpr), mNext(nullptr)
  {
    MOZ_COUNT_CTOR(nsXMLBinding);
  }

  ~nsXMLBinding()
  {
    MOZ_COUNT_DTOR(nsXMLBinding);
  }
};





class nsXMLBindingSet MOZ_FINAL
{
  ~nsXMLBindingSet();

public:
  
  nsAutoPtr<nsXMLBinding> mFirst;

  NS_INLINE_DECL_REFCOUNTING(nsXMLBindingSet);

  


  void
  AddBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr);

  





  int32_t
  LookupTargetIndex(nsIAtom* aTargetVariable, nsXMLBinding** aBinding);

private:
  ~nsXMLBindingSet() {};
};




class nsXMLBindingValues
{
protected:

  
  nsRefPtr<nsXMLBindingSet> mBindings;

  




  nsTArray<nsRefPtr<mozilla::dom::XPathResult> > mValues;

public:

  nsXMLBindingValues() { MOZ_COUNT_CTOR(nsXMLBindingValues); }
  ~nsXMLBindingValues() { MOZ_COUNT_DTOR(nsXMLBindingValues); }

  nsXMLBindingSet* GetBindingSet() { return mBindings; }

  void SetBindingSet(nsXMLBindingSet* aBindings) { mBindings = aBindings; }

  int32_t
  LookupTargetIndex(nsIAtom* aTargetVariable, nsXMLBinding** aBinding)
  {
    return mBindings ?
           mBindings->LookupTargetIndex(aTargetVariable, aBinding) : -1;
  }

  







  mozilla::dom::XPathResult*
  GetAssignmentFor(nsXULTemplateResultXML* aResult,
                   nsXMLBinding* aBinding,
                   int32_t idx,
                   uint16_t type);

  nsINode*
  GetNodeAssignmentFor(nsXULTemplateResultXML* aResult,
                       nsXMLBinding* aBinding,
                       int32_t idx);

  void
  GetStringAssignmentFor(nsXULTemplateResultXML* aResult,
                         nsXMLBinding* aBinding,
                         int32_t idx,
                         nsAString& aValue);
};

#endif 
