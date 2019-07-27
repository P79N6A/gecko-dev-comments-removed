




#ifndef nsXMLBinding_h__
#define nsXMLBinding_h__

#include "nsAutoPtr.h"
#include "nsIAtom.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/XPathExpression.h"

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
  nsAutoPtr<mozilla::dom::XPathExpression> mExpr;

  nsAutoPtr<nsXMLBinding> mNext;

  nsXMLBinding(nsIAtom* aVar, nsAutoPtr<mozilla::dom::XPathExpression>&& aExpr)
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
  AddBinding(nsIAtom* aVar, nsAutoPtr<mozilla::dom::XPathExpression>&& aExpr);

  





  int32_t
  LookupTargetIndex(nsIAtom* aTargetVariable, nsXMLBinding** aBinding);
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
