










#ifndef FragmentOrElement_h___
#define FragmentOrElement_h___

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "nsAttrAndChildArray.h"          
#include "nsCycleCollectionParticipant.h" 
#include "nsIContent.h"                   
#include "nsIDOMXPathNSResolver.h"        
#include "nsIInlineEventHandlers.h"       
#include "nsINodeList.h"                  
#include "nsIWeakReference.h"             
#include "nsNodeUtils.h"                  
#include "nsIHTMLCollection.h"

class ContentUnbinder;
class nsContentList;
class nsDOMAttributeMap;
class nsDOMTokenList;
class nsIControllers;
class nsICSSDeclaration;
class nsIDocument;
class nsDOMStringMap;
class nsINodeInfo;
class nsIURI;







class nsChildContentList MOZ_FINAL : public nsINodeList
{
public:
  nsChildContentList(nsINode* aNode)
    : mNode(aNode)
  {
    SetIsDOMBinding();
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsChildContentList)

  
  virtual JSObject* WrapObject(JSContext *cx,
                               JS::Handle<JSObject*> scope) MOZ_OVERRIDE;

  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent) MOZ_OVERRIDE;
  virtual nsIContent* Item(uint32_t aIndex) MOZ_OVERRIDE;

  void DropReference()
  {
    mNode = nullptr;
  }

  virtual nsINode* GetParentObject() MOZ_OVERRIDE
  {
    return mNode;
  }

private:
  
  nsINode* mNode;
};




class nsNode3Tearoff : public nsIDOMXPathNSResolver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_CYCLE_COLLECTION_CLASS(nsNode3Tearoff)

  NS_DECL_NSIDOMXPATHNSRESOLVER

  nsNode3Tearoff(nsINode *aNode) : mNode(aNode)
  {
  }

protected:
  virtual ~nsNode3Tearoff() {}

private:
  nsCOMPtr<nsINode> mNode;
};





class nsNodeWeakReference MOZ_FINAL : public nsIWeakReference
{
public:
  nsNodeWeakReference(nsINode* aNode)
    : mNode(aNode)
  {
  }

  ~nsNodeWeakReference();

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIWEAKREFERENCE

  void NoticeNodeDestruction()
  {
    mNode = nullptr;
  }

private:
  nsINode* mNode;
};




class nsNodeSupportsWeakRefTearoff MOZ_FINAL : public nsISupportsWeakReference
{
public:
  nsNodeSupportsWeakRefTearoff(nsINode* aNode)
    : mNode(aNode)
  {
  }

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSISUPPORTSWEAKREFERENCE

  NS_DECL_CYCLE_COLLECTION_CLASS(nsNodeSupportsWeakRefTearoff)

private:
  nsCOMPtr<nsINode> mNode;
};


class nsTouchEventReceiverTearoff;
class nsInlineEventHandlersTearoff;





namespace mozilla {
namespace dom {

class UndoManager;

class FragmentOrElement : public nsIContent
{
public:
  FragmentOrElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~FragmentOrElement();

  friend class ::nsTouchEventReceiverTearoff;
  friend class ::nsInlineEventHandlersTearoff;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_SIZEOF_EXCLUDING_THIS

  



  nsresult PostQueryInterface(REFNSIID aIID, void** aInstancePtr);

  
  virtual uint32_t GetChildCount() const MOZ_OVERRIDE;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const MOZ_OVERRIDE;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const MOZ_OVERRIDE;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const MOZ_OVERRIDE;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) MOZ_OVERRIDE;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) MOZ_OVERRIDE;
  virtual void GetTextContentInternal(nsAString& aTextContent) MOZ_OVERRIDE;
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      mozilla::ErrorResult& aError) MOZ_OVERRIDE;

  
  virtual already_AddRefed<nsINodeList> GetChildren(uint32_t aFilter) MOZ_OVERRIDE;
  virtual const nsTextFragment *GetText() MOZ_OVERRIDE;
  virtual uint32_t TextLength() const MOZ_OVERRIDE;
  virtual nsresult SetText(const PRUnichar* aBuffer, uint32_t aLength,
                           bool aNotify) MOZ_OVERRIDE;
  
  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }
  virtual nsresult AppendText(const PRUnichar* aBuffer, uint32_t aLength,
                              bool aNotify) MOZ_OVERRIDE;
  virtual bool TextIsOnlyWhitespace() MOZ_OVERRIDE;
  virtual void AppendTextTo(nsAString& aResult) MOZ_OVERRIDE;
  virtual nsIContent *GetBindingParent() const MOZ_OVERRIDE;
  virtual nsXBLBinding *GetXBLBinding() const MOZ_OVERRIDE;
  virtual void SetXBLBinding(nsXBLBinding* aBinding,
                             nsBindingManager* aOldBindingManager = nullptr) MOZ_OVERRIDE;
  virtual nsIContent *GetXBLInsertionParent() const;
  virtual void SetXBLInsertionParent(nsIContent* aContent);
  virtual bool IsLink(nsIURI** aURI) const MOZ_OVERRIDE;

  virtual void DestroyContent() MOZ_OVERRIDE;
  virtual void SaveSubtreeState() MOZ_OVERRIDE;

  virtual const nsAttrValue* DoGetClasses() const MOZ_OVERRIDE;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) MOZ_OVERRIDE;

  nsIHTMLCollection* Children();
  uint32_t ChildElementCount()
  {
    return Children()->Length();
  }

public:
  



  static void FireNodeInserted(nsIDocument* aDoc,
                               nsINode* aParent,
                               nsTArray<nsCOMPtr<nsIContent> >& aNodes);

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(FragmentOrElement)

  


  void FireNodeRemovedForChildren();

  virtual bool OwnedOnlyByTheDOMTree() MOZ_OVERRIDE
  {
    uint32_t rc = mRefCnt.get();
    if (GetParent()) {
      --rc;
    }
    rc -= mAttrsAndChildren.ChildCount();
    return rc == 0;
  }

  virtual bool IsPurple() MOZ_OVERRIDE
  {
    return mRefCnt.IsPurple();
  }

  virtual void RemovePurple() MOZ_OVERRIDE
  {
    mRefCnt.RemovePurple();
  }

  static void ClearContentUnbinder();
  static bool CanSkip(nsINode* aNode, bool aRemovingAllowed);
  static bool CanSkipInCC(nsINode* aNode);
  static bool CanSkipThis(nsINode* aNode);
  static void MarkNodeChildren(nsINode* aNode);
  static void InitCCCallbacks();
  static void MarkUserData(void* aObject, nsIAtom* aKey, void* aChild,
                           void *aData);
  static void MarkUserDataHandler(void* aObject, nsIAtom* aKey, void* aChild,
                                  void* aData);

protected:
  



  nsresult CopyInnerTo(FragmentOrElement* aDest);

public:
  
  
  







  class nsDOMSlots : public nsINode::nsSlots
  {
  public:
    nsDOMSlots();
    virtual ~nsDOMSlots();

    void Traverse(nsCycleCollectionTraversalCallback &cb, bool aIsXUL);
    void Unlink(bool aIsXUL);

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

    




    nsCOMPtr<nsICSSDeclaration> mStyle;

    



    nsDOMStringMap* mDataset; 

    



    nsRefPtr<UndoManager> mUndoManager;

    



    nsCOMPtr<nsICSSDeclaration> mSMILOverrideStyle;

    


    nsRefPtr<mozilla::css::StyleRule> mSMILOverrideStyleRule;

    



    nsRefPtr<nsDOMAttributeMap> mAttributeMap;

    union {
      



      nsIContent* mBindingParent;  

      


      nsIControllers* mControllers; 
    };

    


    nsRefPtr<nsContentList> mChildrenList;

    


    nsRefPtr<nsDOMTokenList> mClassList;

    


    nsRefPtr<nsXBLBinding> mXBLBinding;

    


    nsCOMPtr<nsIContent> mXBLInsertionParent;
  };

protected:
  
  virtual nsINode::nsSlots* CreateSlots() MOZ_OVERRIDE;

  nsDOMSlots *DOMSlots()
  {
    return static_cast<nsDOMSlots*>(Slots());
  }

  nsDOMSlots *GetExistingDOMSlots() const
  {
    return static_cast<nsDOMSlots*>(GetExistingSlots());
  }

  friend class ::ContentUnbinder;
  


  nsAttrAndChildArray mAttrsAndChildren;
};

} 
} 




class nsInlineEventHandlersTearoff MOZ_FINAL : public nsIInlineEventHandlers
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_FORWARD_NSIINLINEEVENTHANDLERS(mElement->)

  NS_DECL_CYCLE_COLLECTION_CLASS(nsInlineEventHandlersTearoff)

  nsInlineEventHandlersTearoff(mozilla::dom::FragmentOrElement *aElement) : mElement(aElement)
  {
  }

private:
  nsRefPtr<mozilla::dom::FragmentOrElement> mElement;
};

#define NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE                               \
    if (NS_SUCCEEDED(rv))                                                     \
      return rv;                                                              \
                                                                              \
    rv = FragmentOrElement::QueryInterface(aIID, aInstancePtr);               \
    NS_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_ELEMENT_INTERFACE_MAP_END                                          \
    {                                                                         \
      return PostQueryInterface(aIID, aInstancePtr);                          \
    }                                                                         \
                                                                              \
    NS_ADDREF(foundInterface);                                                \
                                                                              \
    *aInstancePtr = foundInterface;                                           \
                                                                              \
    return NS_OK;                                                             \
  }

#endif 
