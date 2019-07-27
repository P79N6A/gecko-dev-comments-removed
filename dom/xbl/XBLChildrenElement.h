





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
  XBLChildrenElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsXMLElement(aNodeInfo)
  {
  }
  XBLChildrenElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo)
    : nsXMLElement(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const;

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

  void AppendInsertedChild(nsIContent* aChild)
  {
    mInsertedChildren.AppendElement(aChild);
    aChild->SetXBLInsertionParent(GetParent());
  }

  void InsertInsertedChildAt(nsIContent* aChild, uint32_t aIndex)
  {
    mInsertedChildren.InsertElementAt(aIndex, aChild);
    aChild->SetXBLInsertionParent(GetParent());
  }

  void RemoveInsertedChild(nsIContent* aChild)
  {
    
    
    
    
    mInsertedChildren.RemoveElement(aChild);
  }

  void ClearInsertedChildren()
  {
    for (uint32_t c = 0; c < mInsertedChildren.Length(); ++c) {
      mInsertedChildren[c]->SetXBLInsertionParent(nullptr);
    }
    mInsertedChildren.Clear();
  }

  void MaybeSetupDefaultContent()
  {
    if (!HasInsertedChildren()) {
      for (nsIContent* child = static_cast<nsINode*>(this)->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        child->SetXBLInsertionParent(GetParent());
      }
    }
  }

  void MaybeRemoveDefaultContent()
  {
    if (!HasInsertedChildren()) {
      for (nsIContent* child = static_cast<nsINode*>(this)->GetFirstChild();
           child;
           child = child->GetNextSibling()) {
        child->SetXBLInsertionParent(nullptr);
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

  int32_t IndexOfInsertedChild(nsIContent* aChild)
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

  nsIContent* InsertedChild(uint32_t aIndex)
  {
    return mInsertedChildren[aIndex];
  }

protected:
  ~XBLChildrenElement();

private:
  nsTArray<nsIContent*> mInsertedChildren; 
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

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsAnonymousContentList)
  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent);
  virtual nsINode* GetParentObject() { return mParent; }
  virtual nsIContent* Item(uint32_t aIndex);

  virtual JSObject* WrapObject(JSContext *cx) MOZ_OVERRIDE;

  bool IsListFor(nsIContent* aContent) {
    return mParent == aContent;
  }

private:
  virtual ~nsAnonymousContentList()
  {
    MOZ_COUNT_DTOR(nsAnonymousContentList);
  }

  nsCOMPtr<nsIContent> mParent;
};

#endif 
