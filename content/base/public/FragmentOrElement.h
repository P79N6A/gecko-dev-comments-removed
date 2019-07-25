










#ifndef FragmentOrElement_h___
#define FragmentOrElement_h___

#include "nsAttrAndChildArray.h"          
#include "nsCOMPtr.h"                     
#include "nsCycleCollectionParticipant.h" 
#include "nsIContent.h"                   
#include "nsIDOMNodeSelector.h"           
#include "nsIDOMTouchEvent.h"             
#include "nsIDOMXPathNSResolver.h"        
#include "nsIInlineEventHandlers.h"       
#include "nsINodeList.h"                  
#include "nsIWeakReference.h"             
#include "nsNodeUtils.h"                  

class ContentUnbinder;
class nsContentList;
class nsDOMAttributeMap;
class nsDOMTokenList;
class nsIControllers;
class nsICSSDeclaration;
class nsIDocument;
class nsIDOMDOMStringMap;
class nsIDOMNamedNodeMap;
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

  
  virtual JSObject* WrapObject(JSContext *cx, JSObject *scope,
                               bool *triedToWrap);

  
  NS_DECL_NSIDOMNODELIST

  
  virtual PRInt32 IndexOf(nsIContent* aContent);

  void DropReference()
  {
    mNode = nullptr;
  }

  virtual nsINode* GetParentObject()
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




class nsNodeSelectorTearoff MOZ_FINAL : public nsIDOMNodeSelector
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMNODESELECTOR

  NS_DECL_CYCLE_COLLECTION_CLASS(nsNodeSelectorTearoff)

  nsNodeSelectorTearoff(nsINode *aNode) : mNode(aNode)
  {
  }

private:
  ~nsNodeSelectorTearoff() {}

private:
  nsCOMPtr<nsINode> mNode;
};


class nsTouchEventReceiverTearoff;
class nsInlineEventHandlersTearoff;





namespace mozilla {
namespace dom {

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

  
  virtual PRUint32 GetChildCount() const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual nsIContent * const * GetChildArray(PRUint32* aChildCount) const;
  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 bool aNotify);
  virtual void RemoveChildAt(PRUint32 aIndex, bool aNotify);
  NS_IMETHOD GetTextContent(nsAString &aTextContent);
  NS_IMETHOD SetTextContent(const nsAString& aTextContent);

  
  virtual already_AddRefed<nsINodeList> GetChildren(PRUint32 aFilter);
  virtual const nsTextFragment *GetText();
  virtual PRUint32 TextLength() const;
  virtual nsresult SetText(const PRUnichar* aBuffer, PRUint32 aLength,
                           bool aNotify);
  
  nsresult SetText(const nsAString& aStr, bool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }
  virtual nsresult AppendText(const PRUnichar* aBuffer, PRUint32 aLength,
                              bool aNotify);
  virtual bool TextIsOnlyWhitespace();
  virtual void AppendTextTo(nsAString& aResult);
  virtual nsIContent *GetBindingParent() const;
  virtual bool IsLink(nsIURI** aURI) const;

  virtual void DestroyContent();
  virtual void SaveSubtreeState();

  virtual const nsAttrValue* DoGetClasses() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);

public:
  
  NS_IMETHOD GetNodeName(nsAString& aNodeName);
  NS_IMETHOD GetLocalName(nsAString& aLocalName);
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue);
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue);
  NS_IMETHOD GetNodeType(PRUint16* aNodeType);
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes);
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI);
  NS_IMETHOD GetPrefix(nsAString& aPrefix);
  NS_IMETHOD IsSupported(const nsAString& aFeature,
                         const nsAString& aVersion, bool* aReturn);
  NS_IMETHOD HasAttributes(bool* aHasAttributes);
  NS_IMETHOD HasChildNodes(bool* aHasChildNodes);
  nsresult InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                        nsIDOMNode** aReturn)
  {
    return ReplaceOrInsertBefore(false, aNewChild, aRefChild, aReturn);
  }
  nsresult ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                        nsIDOMNode** aReturn)
  {
    return ReplaceOrInsertBefore(true, aNewChild, aOldChild, aReturn);
  }
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
  {
    return nsINode::RemoveChild(aOldChild, aReturn);
  }
  nsresult AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  {
    return InsertBefore(aNewChild, nullptr, aReturn);
  }

  nsresult CloneNode(bool aDeep, PRUint8 aOptionalArgc, nsIDOMNode **aResult)
  {
    if (!aOptionalArgc) {
      aDeep = true;
    }
    
    return nsNodeUtils::CloneNodeImpl(this, aDeep, true, aResult);
  }

  

  







  static nsresult InternalIsSupported(nsISupports* aObject,
                                      const nsAString& aFeature,
                                      const nsAString& aVersion,
                                      bool* aReturn);

  



  static void FireNodeInserted(nsIDocument* aDoc,
                               nsINode* aParent,
                               nsTArray<nsCOMPtr<nsIContent> >& aNodes);

  


  static nsIContent* doQuerySelector(nsINode* aRoot, const nsAString& aSelector,
                                     nsresult *aResult);
  static nsresult doQuerySelectorAll(nsINode* aRoot,
                                     const nsAString& aSelector,
                                     nsIDOMNodeList **aReturn);

  NS_DECL_CYCLE_COLLECTION_SKIPPABLE_SCRIPT_HOLDER_CLASS(FragmentOrElement)

  


  void FireNodeRemovedForChildren();

  virtual bool OwnedOnlyByTheDOMTree()
  {
    PRUint32 rc = mRefCnt.get();
    if (GetParent()) {
      --rc;
    }
    rc -= mAttrsAndChildren.ChildCount();
    return rc == 0;
  }

  virtual bool IsPurple()
  {
    return mRefCnt.IsPurple();
  }

  virtual void RemovePurple()
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

    




    nsCOMPtr<nsICSSDeclaration> mStyle;

    



    nsIDOMDOMStringMap* mDataset; 

    



    nsCOMPtr<nsICSSDeclaration> mSMILOverrideStyle;

    


    nsRefPtr<mozilla::css::StyleRule> mSMILOverrideStyleRule;

    



    nsRefPtr<nsDOMAttributeMap> mAttributeMap;

    union {
      



      nsIContent* mBindingParent;  

      


      nsIControllers* mControllers; 
    };

    


    nsRefPtr<nsContentList> mChildrenList;

    


    nsRefPtr<nsDOMTokenList> mClassList;
  };

protected:
  
  virtual nsINode::nsSlots* CreateSlots();

  nsDOMSlots *DOMSlots()
  {
    return static_cast<nsDOMSlots*>(GetSlots());
  }

  nsDOMSlots *GetExistingDOMSlots() const
  {
    return static_cast<nsDOMSlots*>(GetExistingSlots());
  }

  friend class ::ContentUnbinder;
  


  nsAttrAndChildArray mAttrsAndChildren;

  nsContentList* GetChildrenList();
};

} 
} 




class nsTouchEventReceiverTearoff MOZ_FINAL : public nsITouchEventReceiver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_FORWARD_NSITOUCHEVENTRECEIVER(mElement->)

  NS_DECL_CYCLE_COLLECTION_CLASS(nsTouchEventReceiverTearoff)

  nsTouchEventReceiverTearoff(mozilla::dom::FragmentOrElement *aElement) : mElement(aElement)
  {
  }

private:
  nsRefPtr<mozilla::dom::FragmentOrElement> mElement;
};




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
    rv = FragmentOrElement::QueryInterface(aIID, aInstancePtr);                \
    if (NS_SUCCEEDED(rv))                                                     \
      return rv;                                                              \
                                                                              \
    NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

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
