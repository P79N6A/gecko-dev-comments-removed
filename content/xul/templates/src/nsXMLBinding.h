




#ifndef nsXMLBinding_h__
#define nsXMLBinding_h__

#include "nsAutoPtr.h"
#include "nsIAtom.h"
#include "nsCycleCollectionParticipant.h"
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

inline void
ImplCycleCollectionUnlink(nsXMLBinding* aBinding)
{
  while (aBinding) {
    aBinding->mExpr = nullptr;
    aBinding = aBinding->mNext;
  }
}

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsXMLBinding* aBinding,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  while (aBinding) {
    CycleCollectionNoteChild(aCallback, aBinding->mExpr.get(),
                             "nsXMLBinding::mExpr", aFlags);
    aBinding = aBinding->mNext;
  }
}





class nsXMLBindingSet MOZ_FINAL
{
  ~nsXMLBindingSet();

public:

  
  
  nsCycleCollectingAutoRefCnt mRefCnt;

  
  nsAutoPtr<nsXMLBinding> mFirst;

public:

  NS_METHOD_(MozExternalRefCountType) AddRef();
  NS_METHOD_(MozExternalRefCountType) Release();
  NS_DECL_OWNINGTHREAD
  NS_DECL_CYCLE_COLLECTION_NATIVE_CLASS(nsXMLBindingSet)

  


  void
  AddBinding(nsIAtom* aVar, nsIDOMXPathExpression* aExpr);

  





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
