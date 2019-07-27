










#ifndef FragmentOrElement_h___
#define FragmentOrElement_h___

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"
#include "nsAttrAndChildArray.h"          
#include "nsCycleCollectionParticipant.h" 
#include "nsIContent.h"                   
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
class nsIURI;

namespace mozilla {
namespace dom {
class Element;
}
}







class nsChildContentList final : public nsINodeList
{
public:
  explicit nsChildContentList(nsINode* aNode)
    : mNode(aNode)
  {
  }

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(nsChildContentList)

  
  virtual JSObject* WrapObject(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;

  
  NS_DECL_NSIDOMNODELIST

  
  virtual int32_t IndexOf(nsIContent* aContent) override;
  virtual nsIContent* Item(uint32_t aIndex) override;

  void DropReference()
  {
    mNode = nullptr;
  }

  virtual nsINode* GetParentObject() override
  {
    return mNode;
  }

private:
  ~nsChildContentList() {}

  
  nsINode* mNode;
};





class nsNodeWeakReference final : public nsIWeakReference
{
public:
  explicit nsNodeWeakReference(nsINode* aNode)
    : mNode(aNode)
  {
  }

  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIWEAKREFERENCE
  virtual size_t SizeOfOnlyThis(mozilla::MallocSizeOf aMallocSizeOf) const override;

  void NoticeNodeDestruction()
  {
    mNode = nullptr;
  }

private:
  ~nsNodeWeakReference();

  nsINode* mNode;
};




class nsNodeSupportsWeakRefTearoff final : public nsISupportsWeakReference
{
public:
  explicit nsNodeSupportsWeakRefTearoff(nsINode* aNode)
    : mNode(aNode)
  {
  }

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSISUPPORTSWEAKREFERENCE

  NS_DECL_CYCLE_COLLECTION_CLASS(nsNodeSupportsWeakRefTearoff)

private:
  ~nsNodeSupportsWeakRefTearoff() {}

  nsCOMPtr<nsINode> mNode;
};





namespace mozilla {
namespace dom {

class ShadowRoot;
class UndoManager;

class FragmentOrElement : public nsIContent
{
public:
  explicit FragmentOrElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);
  explicit FragmentOrElement(already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_SIZEOF_EXCLUDING_THIS

  
  virtual uint32_t GetChildCount() const override;
  virtual nsIContent *GetChildAt(uint32_t aIndex) const override;
  virtual nsIContent * const * GetChildArray(uint32_t* aChildCount) const override;
  virtual int32_t IndexOf(const nsINode* aPossibleChild) const override;
  virtual nsresult InsertChildAt(nsIContent* aKid, uint32_t aIndex,
                                 bool aNotify) override;
  virtual void RemoveChildAt(uint32_t aIndex, bool aNotify) override;
  virtual void GetTextContentInternal(nsAString& aTextContent,
                                      mozilla::ErrorResult& aError) override;
  virtual void SetTextContentInternal(const nsAString& aTextContent,
                                      mozilla::ErrorResult& aError) override;

  
  virtual already_AddRefed<nsINodeList> GetChildren(uint32_t aFilter) override;
  virtual const nsTextFragment *GetText() override;
  virtual uint32_t TextLength() const override;
  virtual nsresult SetText(const char16_t* aBuffer, uint32_t aLength,
                           bool aNotify) override;
  
  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }
  virtual nsresult AppendText(const char16_t* aBuffer, uint32_t aLength,
                              bool aNotify) override;
  virtual bool TextIsOnlyWhitespace() override;
  virtual bool HasTextForTranslation() override;
  virtual void AppendTextTo(nsAString& aResult) override;
  MOZ_WARN_UNUSED_RESULT
  virtual bool AppendTextTo(nsAString& aResult, const mozilla::fallible_t&) override;
  virtual nsIContent *GetBindingParent() const override;
  virtual nsXBLBinding *GetXBLBinding() const override;
  virtual void SetXBLBinding(nsXBLBinding* aBinding,
                             nsBindingManager* aOldBindingManager = nullptr) override;
  virtual ShadowRoot *GetShadowRoot() const override;
  virtual ShadowRoot *GetContainingShadow() const override;
  virtual nsTArray<nsIContent*> &DestInsertionPoints() override;
  virtual nsTArray<nsIContent*> *GetExistingDestInsertionPoints() const override;
  virtual void SetShadowRoot(ShadowRoot* aBinding) override;
  virtual nsIContent *GetXBLInsertionParent() const override;
  virtual void SetXBLInsertionParent(nsIContent* aContent) override;
  virtual bool IsLink(nsIURI** aURI) const override;

  virtual CustomElementData *GetCustomElementData() const override;
  virtual void SetCustomElementData(CustomElementData* aData) override;

  virtual void DestroyContent() override;
  virtual void SaveSubtreeState() override;

  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker) override;

  nsIHTMLCollection* Children();
  uint32_t ChildElementCount()
  {
    return Children()->Length();
  }

  





  void SetIsElementInStyleScopeFlagOnSubtree(bool aInStyleScope);

public:
  



  static void FireNodeInserted(nsIDocument* aDoc,
                               nsINode* aParent,
                               nsTArray<nsCOMPtr<nsIContent> >& aNodes);

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(FragmentOrElement)

  


  void FireNodeRemovedForChildren();

  virtual bool OwnedOnlyByTheDOMTree() override
  {
    uint32_t rc = mRefCnt.get();
    if (GetParent()) {
      --rc;
    }
    rc -= mAttrsAndChildren.ChildCount();
    return rc == 0;
  }

  virtual bool IsPurple() override
  {
    return mRefCnt.IsPurple();
  }

  virtual void RemovePurple() override
  {
    mRefCnt.RemovePurple();
  }

  static void ClearContentUnbinder();
  static bool CanSkip(nsINode* aNode, bool aRemovingAllowed);
  static bool CanSkipInCC(nsINode* aNode);
  static bool CanSkipThis(nsINode* aNode);
  static void RemoveBlackMarkedNode(nsINode* aNode);
  static void MarkNodeChildren(nsINode* aNode);
  static void InitCCCallbacks();
  static void MarkUserData(void* aObject, nsIAtom* aKey, void* aChild,
                           void *aData);

  


  static bool IsHTMLVoid(nsIAtom* aLocalName);
protected:
  virtual ~FragmentOrElement();

  



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

    


    nsRefPtr<ShadowRoot> mShadowRoot;

    


    nsRefPtr<ShadowRoot> mContainingShadow;

    



    nsTArray<nsIContent*> mDestInsertionPoints;

    


    nsRefPtr<nsXBLBinding> mXBLBinding;

    


    nsCOMPtr<nsIContent> mXBLInsertionParent;

    


    nsRefPtr<CustomElementData> mCustomElementData;
  };

protected:
  void GetMarkup(bool aIncludeSelf, nsAString& aMarkup);
  void SetInnerHTMLInternal(const nsAString& aInnerHTML, ErrorResult& aError);

  
  virtual nsINode::nsSlots* CreateSlots() override;

  nsDOMSlots *DOMSlots()
  {
    return static_cast<nsDOMSlots*>(Slots());
  }

  nsDOMSlots *GetExistingDOMSlots() const
  {
    return static_cast<nsDOMSlots*>(GetExistingSlots());
  }

  





  void SetIsElementInStyleScopeFlagOnShadowTree(bool aInStyleScope);

  friend class ::ContentUnbinder;
  


  nsAttrAndChildArray mAttrsAndChildren;
};

} 
} 

#define NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE                               \
    if (NS_SUCCEEDED(rv))                                                     \
      return rv;                                                              \
                                                                              \
    rv = FragmentOrElement::QueryInterface(aIID, aInstancePtr);               \
    NS_INTERFACE_TABLE_TO_MAP_SEGUE

#endif 
