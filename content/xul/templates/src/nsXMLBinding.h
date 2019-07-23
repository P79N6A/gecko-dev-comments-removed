



































#ifndef nsXMLBinding_h__
#define nsXMLBinding_h__

#include "nsAutoPtr.h"
#include "nsIAtom.h"

class nsXULTemplateResultXML;
class nsXMLBindingValues;








struct nsXMLBinding {
  nsCOMPtr<nsIAtom> mVar;
  nsCOMPtr<nsIDOMXPathExpression> mExpr;

  nsAutoPtr<nsXMLBinding> mNext;

  nsXMLBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr)
    : mVar(aVar), mExpr(aExpr), mNext(nsnull)
  {
    MOZ_COUNT_CTOR(nsXMLBinding);
  }

  ~nsXMLBinding()
  {
    MOZ_COUNT_DTOR(nsXMLBinding);
  }
};





class nsXMLBindingSet
{
public:

  
  
  nsAutoRefCnt mRefCnt;

  
  nsAutoPtr<nsXMLBinding> mFirst;

public:

  nsrefcnt AddRef();
  nsrefcnt Release();
  NS_DECL_OWNINGTHREAD

  


  nsresult
  AddBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr);

  





  PRInt32
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

  PRInt32
  LookupTargetIndex(nsIAtom* aTargetVariable, nsXMLBinding** aBinding)
  {
    return mBindings ?
           mBindings->LookupTargetIndex(aTargetVariable, aBinding) : -1;
  }

  








  void
  GetAssignmentFor(nsXULTemplateResultXML* aResult,
                   nsXMLBinding* aBinding,
                   PRInt32 idx,
                   PRUint16 type,
                   nsIDOMXPathResult** aValue);

  void
  GetNodeAssignmentFor(nsXULTemplateResultXML* aResult,
                       nsXMLBinding* aBinding,
                       PRInt32 idx,
                       nsIDOMNode** aValue);

  void
  GetStringAssignmentFor(nsXULTemplateResultXML* aResult,
                         nsXMLBinding* aBinding,
                         PRInt32 idx,
                         nsAString& aValue);
};

#endif 
