




#ifndef nsXMLBinding_h__
#define nsXMLBinding_h__

#include "nsAutoPtr.h"
#include "nsIAtom.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"

class nsXULTemplateResultXML;
class nsXMLBindingValues;








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
public:

  
  
  nsCycleCollectingAutoRefCnt mRefCnt;

  
  nsAutoPtr<nsXMLBinding> mFirst;

public:

  NS_METHOD_(nsrefcnt) AddRef();
  NS_METHOD_(nsrefcnt) Release();
  NS_DECL_OWNINGTHREAD
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXMLBindingSet)

  


  nsresult
  AddBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr);

  





  int32_t
  LookupTargetIndex(nsIAtom* aTargetVariable, nsXMLBinding** aBinding);
};




class nsXMLBindingValues
{
protected:

  
  nsRefPtr<nsXMLBindingSet> mBindings;

  




  nsCOMArray<nsIDOMXPathResult> mValues;

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

  








  void
  GetAssignmentFor(nsXULTemplateResultXML* aResult,
                   nsXMLBinding* aBinding,
                   int32_t idx,
                   uint16_t type,
                   nsIDOMXPathResult** aValue);

  void
  GetNodeAssignmentFor(nsXULTemplateResultXML* aResult,
                       nsXMLBinding* aBinding,
                       int32_t idx,
                       nsIDOMNode** aValue);

  void
  GetStringAssignmentFor(nsXULTemplateResultXML* aResult,
                         nsXMLBinding* aBinding,
                         int32_t idx,
                         nsAString& aValue);
};

#endif 
