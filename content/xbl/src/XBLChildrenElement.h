





#ifndef nsXBLChildrenElement_h___
#define nsXBLChildrenElement_h___

#include "nsIDOMElement.h"
#include "nsINodeList.h"
#include "nsBindingManager.h"
#include "mozilla/dom/nsXMLElement.h"

class nsAnonymousContentList;

namespace mozilla {
namespace dom {

class ExplicitChildIterator;

class XBLChildrenElement : public nsXMLElement
{
public:
  friend class mozilla::dom::ExplicitChildIterator;
  friend class nsAnonymousContentList;
  XBLChildrenElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsXMLElement(aNodeInfo)
  {
  }
  ~XBLChildrenElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo() { return nullptr; }

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  virtual nsIAtom *GetIDAttributeName() const;
  virtual nsIAtom* DoGetID() const;
  virtual nsresult UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);

  void AppendInsertedChild(nsIContent* aChild,
                           nsBindingManager* aBindingManager)
  {
    mInsertedChildren.AppendElement(aChild);
    aBindingManager->SetInsertionParent(aChild, GetParent());
  }

  void InsertInsertedChildAt(nsIContent* aChild,
                             uint32_t aIndex,
                             nsBindingManager* aBindingManager)
  {
    mInsertedChildren.InsertElementAt(aIndex, aChild);
    aBindingManager->SetInsertionParent(aChild, GetParent());
  }

  void RemoveInsertedChild(nsIContent* aChild)
  {
    
    
    
    
    mInsertedChildren.RemoveElement(aChild);
  }

  void ClearInsertedChildren()
  {
    mInsertedChildren.Clear();
  }

  void ClearInsertedChildrenAndInsertionParents(nsBindingManager* aBindingManager)
  {
    for (uint32_t c = 0; c < mInsertedChildren.Length(); ++c) {
      aBindingManager->SetInsertionParent(mInsertedChildren[c], nullptr);
    }
    mInsertedChildren.Clear();
  }

  void MaybeSetupDefaultContent(nsBindingManager* aBindingManager)
  {
    if (!HasInsertedChildren()) {
      for (nsIContent* child = static_cast<nsINode*>(this)->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        aBindingManager->SetInsertionParent(child, GetParent());
      }
    }
  }

  void MaybeRemoveDefaultContent(nsBindingManager* aBindingManager)
  {
    if (!HasInsertedChildren()) {
      for (nsIContent* child = static_cast<nsINode*>(this)->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        aBindingManager->SetInsertionParent(child, nullptr);
      }
    }
  }

  uint32_t InsertedChildrenLength()
  {
    return mInsertedChildren.Length();
  }

  bool HasInsertedChildren()
  {
    return !mInsertedChildren.IsEmpty();
  }

  enum {
    NoIndex = uint32_t(-1)
  };
  uint32_t IndexOfInsertedChild(nsIContent* aChild)
  {
    return mInsertedChildren.IndexOf(aChild);
  }

  bool Includes(nsIContent* aChild)
  {
    NS_ASSERTION(!mIncludes.IsEmpty(),
                 "Shouldn't check for includes on default insertion point");
    return mIncludes.Contains(aChild->Tag());
  }

  bool IsDefaultInsertion()
  {
    return mIncludes.IsEmpty();
  }

  nsTArray<nsIContent*> mInsertedChildren;

private:
  nsTArray<nsCOMPtr<nsIAtom> > mIncludes;
};

} 
} 

class nsAnonymousContentList : public nsINodeList
{
public:
  nsAnonymousContentList(nsIContent* aParent)
    : mParent(aParent)
  {
    MOZ_COUNT_CTOR(nsAnonymousContentList);
    SetIsDOMBinding();
  }

  virtual ~nsAnonymousContentList()
  {
    MOZ_COUNT_DTOR(nsAnonymousContentList);
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsAnonymousContentList)
  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent);
  virtual nsINode* GetParentObject() { return mParent; }
  virtual nsIContent* Item(uint32_t aIndex);

  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

  bool IsListFor(nsIContent* aContent) {
    return mParent == aContent;
  }

private:
  nsCOMPtr<nsIContent> mParent;
};

#endif 
