










































#ifndef nsGenericElement_h___
#define nsGenericElement_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocumentFragment.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOM3EventTarget.h"
#include "nsIDOM3Node.h"
#include "nsIDOMNSEventTarget.h"
#include "nsIDOMNSElement.h"
#include "nsILinkHandler.h"
#include "nsContentUtils.h"
#include "nsNodeUtils.h"
#include "nsAttrAndChildArray.h"
#include "mozFlushType.h"
#include "nsDOMAttributeMap.h"
#include "nsIWeakReference.h"
#include "nsCycleCollectionParticipant.h"
#include "nsIDocument.h"
#include "nsIDOMNodeSelector.h"
#include "nsIDOMXPathNSResolver.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
#endif 

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIFrame;
class nsIDOMNamedNodeMap;
class nsDOMCSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsIURI;
class nsINodeInfo;
class nsIControllers;
class nsIDOMNSFeatureFactory;
class nsIEventListenerManager;
class nsIScrollableView;
class nsContentList;
class nsDOMTokenList;
struct nsRect;

typedef PRUptrdiff PtrBits;







class nsChildContentList : public nsINodeList,
                           public nsWrapperCache
{
public:
  nsChildContentList(nsINode* aNode)
    : mNode(aNode)
  {
  }

  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMNODELIST

  
  virtual nsIContent* GetNodeAt(PRUint32 aIndex);
  virtual PRInt32 IndexOf(nsIContent* aContent);

  void DropReference()
  {
    mNode = nsnull;
  }

  nsISupports* GetParentObject()
  {
    return mNode;
  }

  static nsChildContentList* FromSupports(nsISupports* aSupports)
  {
    nsINodeList* list = static_cast<nsINodeList*>(aSupports);
#ifdef DEBUG
    {
      nsCOMPtr<nsINodeList> list_qi = do_QueryInterface(aSupports);

      
      
      
      NS_ASSERTION(list_qi == list, "Uh, fix QI!");
    }
#endif
    return static_cast<nsChildContentList*>(list);
  }

private:
  
  nsINode* mNode;
};




class nsNode3Tearoff : public nsIDOM3Node, public nsIDOMXPathNSResolver
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOM3NODE

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsNode3Tearoff, nsIDOM3Node)

  nsNode3Tearoff(nsIContent *aContent) : mContent(aContent)
  {
  }

  







  static PRBool AreNodesEqual(nsIContent* aContent1,
                              nsIContent* aContent2);

protected:
  virtual ~nsNode3Tearoff() {}

private:
  nsCOMPtr<nsIContent> mContent;
};





class nsNodeWeakReference : public nsIWeakReference
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
    mNode = nsnull;
  }

private:
  nsINode* mNode;
};




class nsNodeSupportsWeakRefTearoff : public nsISupportsWeakReference
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

#define NS_EVENT_TEAROFF_CACHE_SIZE 4










class nsDOMEventRTTearoff : public nsIDOMEventTarget,
                            public nsIDOM3EventTarget,
                            public nsIDOMNSEventTarget
{
private:
  
  
  
  
  

  nsDOMEventRTTearoff(nsINode *aNode);

  static nsDOMEventRTTearoff *mCachedEventTearoff[NS_EVENT_TEAROFF_CACHE_SIZE];
  static PRUint32 mCachedEventTearoffCount;

  




  void LastRelease();

  nsresult GetDOM3EventTarget(nsIDOM3EventTarget **aTarget);

public:
  virtual ~nsDOMEventRTTearoff();

  



  static nsDOMEventRTTearoff *Create(nsINode *aNode);

  


  static void Shutdown();

  
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  
  NS_DECL_NSIDOMEVENTTARGET

  
  NS_DECL_NSIDOM3EVENTTARGET

  
  NS_DECL_NSIDOMNSEVENTTARGET

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMEventRTTearoff,
                                           nsIDOMEventTarget)

private:
  



  nsCOMPtr<nsINode> mNode;
};




class nsNodeSelectorTearoff : public nsIDOMNodeSelector
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMNODESELECTOR

  NS_DECL_CYCLE_COLLECTION_CLASS(nsNodeSelectorTearoff)

  nsNodeSelectorTearoff(nsIContent *aContent) : mContent(aContent)
  {
  }

private:
  ~nsNodeSelectorTearoff() {}

private:
  nsCOMPtr<nsIContent> mContent;
};


class nsNSElementTearoff;





class nsGenericElement : public nsIContent
{
public:
  nsGenericElement(nsINodeInfo *aNodeInfo);
  virtual ~nsGenericElement();

  friend class nsNSElementTearoff;

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  



  nsresult PostQueryInterface(REFNSIID aIID, void** aInstancePtr);

  
  virtual PRUint32 GetChildCount() const;
  virtual nsIContent *GetChildAt(PRUint32 aIndex) const;
  virtual nsIContent * const * GetChildArray(PRUint32* aChildCount) const;
  virtual PRInt32 IndexOf(nsINode* aPossibleChild) const;
  virtual nsresult InsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                 PRBool aNotify);
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify, PRBool aMutationEvent = PR_TRUE);
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult DispatchDOMEvent(nsEvent* aEvent, nsIDOMEvent* aDOMEvent,
                                    nsPresContext* aPresContext,
                                    nsEventStatus* aEventStatus);
  virtual nsIEventListenerManager* GetListenerManager(PRBool aCreateIfNotFound);
  virtual nsresult AddEventListenerByIID(nsIDOMEventListener *aListener,
                                         const nsIID& aIID);
  virtual nsresult RemoveEventListenerByIID(nsIDOMEventListener *aListener,
                                            const nsIID& aIID);
  virtual nsresult GetSystemEventGroup(nsIDOMEventGroup** aGroup);
  virtual nsIScriptContext* GetContextForEventHandlers(nsresult* aRv)
  {
    return nsContentUtils::GetContextForEventHandlers(this, aRv);
  }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsIAtom *GetIDAttributeName() const;
  virtual nsIAtom *GetClassAttributeName() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                           const nsAString& aValue, PRBool aNotify);
  virtual PRBool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                         nsAString& aResult) const;
  virtual PRBool HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const;
  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual PRBool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual PRInt32 FindAttrValueIn(PRInt32 aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const;
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             PRBool aNotify);
  virtual const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const;
  virtual PRUint32 GetAttrCount() const;
  virtual const nsTextFragment *GetText();
  virtual PRUint32 TextLength();
  virtual nsresult SetText(const PRUnichar* aBuffer, PRUint32 aLength,
                           PRBool aNotify);
  
  nsresult SetText(const nsAString& aStr, PRBool aNotify)
  {
    return SetText(aStr.BeginReading(), aStr.Length(), aNotify);
  }
  virtual nsresult AppendText(const PRUnichar* aBuffer, PRUint32 aLength,
                              PRBool aNotify);
  virtual PRBool TextIsOnlyWhitespace();
  virtual void AppendTextTo(nsAString& aResult);
  virtual nsIContent *GetBindingParent() const;
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual already_AddRefed<nsIURI> GetBaseURI() const;
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void SetMayHaveFrame(PRBool aMayHaveFrame);
  virtual PRBool MayHaveFrame() const;

  virtual PRUint32 GetScriptTypeID() const;
  NS_IMETHOD SetScriptTypeID(PRUint32 aLang);

  virtual void DestroyContent();
  virtual void SaveSubtreeState();

#ifdef MOZ_SMIL
  virtual nsISMILAttr* GetAnimatedAttr(const nsIAtom* )
  {
    return nsnull;
  }
  virtual nsresult GetSMILOverrideStyle(nsIDOMCSSStyleDeclaration** aStyle);
  virtual nsICSSStyleRule* GetSMILOverrideStyleRule();
  virtual nsresult SetSMILOverrideStyleRule(nsICSSStyleRule* aStyleRule,
                                            PRBool aNotify);
#endif 

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const
  {
    List(out, aIndent, EmptyCString());
  }
  virtual void DumpContent(FILE* out, PRInt32 aIndent, PRBool aDumpAll) const;
  void List(FILE* out, PRInt32 aIndent, const nsCString& aPrefix) const;
  void ListAttributes(FILE* out) const;
#endif

  virtual nsIAtom* GetID() const;
  virtual const nsAttrValue* DoGetClasses() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  virtual nsICSSStyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify);
  NS_IMETHOD_(PRBool)
    IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  


  struct MappedAttributeEntry {
    nsIAtom** attribute;
  };

  





  static PRBool
  FindAttributeDependence(const nsIAtom* aAttribute,
                          const MappedAttributeEntry* const aMaps[],
                          PRUint32 aMapCount);

  
  NS_IMETHOD GetNodeName(nsAString& aNodeName);
  NS_IMETHOD GetLocalName(nsAString& aLocalName);
  NS_IMETHOD GetNodeValue(nsAString& aNodeValue);
  NS_IMETHOD SetNodeValue(const nsAString& aNodeValue);
  NS_IMETHOD GetNodeType(PRUint16* aNodeType);
  NS_IMETHOD GetAttributes(nsIDOMNamedNodeMap** aAttributes);
  NS_IMETHOD GetNamespaceURI(nsAString& aNamespaceURI);
  NS_IMETHOD GetPrefix(nsAString& aPrefix);
  NS_IMETHOD SetPrefix(const nsAString& aPrefix);
  NS_IMETHOD Normalize();
  NS_IMETHOD IsSupported(const nsAString& aFeature,
                         const nsAString& aVersion, PRBool* aReturn);
  NS_IMETHOD HasAttributes(PRBool* aHasAttributes);
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes);
  NS_IMETHOD InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                          nsIDOMNode** aReturn);
  NS_IMETHOD ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                          nsIDOMNode** aReturn);
  NS_IMETHOD RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn);
  NS_IMETHOD AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
  {
    return InsertBefore(aNewChild, nsnull, aReturn);
  }

  
  NS_IMETHOD GetTagName(nsAString& aTagName);
  NS_IMETHOD GetAttribute(const nsAString& aName,
                          nsAString& aReturn);
  NS_IMETHOD SetAttribute(const nsAString& aName,
                          const nsAString& aValue);
  NS_IMETHOD RemoveAttribute(const nsAString& aName);
  NS_IMETHOD GetAttributeNode(const nsAString& aName,
                              nsIDOMAttr** aReturn);
  NS_IMETHOD SetAttributeNode(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD RemoveAttributeNode(nsIDOMAttr* aOldAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD GetElementsByTagName(const nsAString& aTagname,
                                  nsIDOMNodeList** aReturn);
  NS_IMETHOD GetAttributeNS(const nsAString& aNamespaceURI,
                            const nsAString& aLocalName,
                            nsAString& aReturn);
  NS_IMETHOD SetAttributeNS(const nsAString& aNamespaceURI,
                            const nsAString& aQualifiedName,
                            const nsAString& aValue);
  NS_IMETHOD RemoveAttributeNS(const nsAString& aNamespaceURI,
                               const nsAString& aLocalName);
  NS_IMETHOD GetAttributeNodeNS(const nsAString& aNamespaceURI,
                                const nsAString& aLocalName,
                                nsIDOMAttr** aReturn);
  NS_IMETHOD SetAttributeNodeNS(nsIDOMAttr* aNewAttr, nsIDOMAttr** aReturn);
  NS_IMETHOD GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                    const nsAString& aLocalName,
                                    nsIDOMNodeList** aReturn);
  NS_IMETHOD HasAttribute(const nsAString& aName, PRBool* aReturn);
  NS_IMETHOD HasAttributeNS(const nsAString& aNamespaceURI,
                            const nsAString& aLocalName,
                            PRBool* aReturn);
  nsresult CloneNode(PRBool aDeep, nsIDOMNode **aResult)
  {
    return nsNodeUtils::CloneNodeImpl(this, aDeep, aResult);
  }

  

  






  nsresult AddScriptEventListener(nsIAtom* aEventName,
                                  const nsAString& aValue,
                                  PRBool aDefer = PR_TRUE);

  


  nsresult LeaveLink(nsPresContext* aPresContext);

  




  nsresult JoinTextNodes(nsIContent* aFirst,
                         nsIContent* aSecond);

  







  static nsresult InternalIsSupported(nsISupports* aObject,
                                      const nsAString& aFeature,
                                      const nsAString& aVersion,
                                      PRBool* aReturn);

  static nsresult InternalGetFeature(nsISupports* aObject,
                                     const nsAString& aFeature,
                                     const nsAString& aVersion,
                                     nsISupports** aReturn);

  static already_AddRefed<nsIDOMNSFeatureFactory>
    GetDOMFeatureFactory(const nsAString& aFeature, const nsAString& aVersion);

  static PRBool ShouldBlur(nsIContent *aContent);

  













  static nsresult doReplaceOrInsertBefore(PRBool aReplace, nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                                          nsIContent* aParent, nsIDocument* aDocument,
                                          nsIDOMNode** aReturn);

  










  static nsresult doRemoveChild(nsIDOMNode* aOldChild,
                                nsIContent* aParent, nsIDocument* aDocument,
                                nsIDOMNode** aReturn);

  













  static nsresult doInsertChildAt(nsIContent* aKid, PRUint32 aIndex,
                                  PRBool aNotify, nsIContent* aParent,
                                  nsIDocument* aDocument,
                                  nsAttrAndChildArray& aChildArray);

  













  static nsresult doRemoveChildAt(PRUint32 aIndex, PRBool aNotify,
                                  nsIContent* aKid, nsIContent* aParent,
                                  nsIDocument* aDocument,
                                  nsAttrAndChildArray& aChildArray,
                                  PRBool aMutationEvent);

  


  static nsresult doQuerySelector(nsINode* aRoot, const nsAString& aSelector,
                                  nsIDOMElement **aReturn);
  static nsresult doQuerySelectorAll(nsINode* aRoot,
                                     const nsAString& aSelector,
                                     nsIDOMNodeList **aReturn);
  static PRBool doMatchesSelector(nsIContent* aNode, const nsAString& aSelector);

  


  static nsresult doPreHandleEvent(nsIContent* aContent,
                                   nsEventChainPreVisitor& aVisitor);

  






  static nsresult DispatchClickEvent(nsPresContext* aPresContext,
                                     nsInputEvent* aSourceEvent,
                                     nsIContent* aTarget,
                                     PRBool aFullDispatch,
                                     nsEventStatus* aStatus);

  






  static nsresult DispatchEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent* aTarget,
                                PRBool aFullDispatch,
                                nsEventStatus* aStatus);

  





  nsIFrame* GetPrimaryFrame();

  







  nsIFrame* GetPrimaryFrame(mozFlushType aType);

  






  static nsIFrame* GetPrimaryFrameFor(nsIContent* aContent,
                                      nsIDocument* aDocument);

  



  struct nsAttrInfo {
    nsAttrInfo(const nsAttrName* aName, const nsAttrValue* aValue) :
      mName(aName), mValue(aValue) {}
    nsAttrInfo(const nsAttrInfo& aOther) :
      mName(aOther.mName), mValue(aOther.mValue) {}

    const nsAttrName* mName;
    const nsAttrValue* mValue;
  };

  const nsAttrValue* GetParsedAttr(nsIAtom* aAttr) const
  {
    return mAttrsAndChildren.GetAttr(aAttr);
  }

  




  nsDOMAttributeMap *GetAttributeMap()
  {
    nsDOMSlots *slots = GetExistingDOMSlots();

    return slots ? slots->mAttributeMap.get() : nsnull;
  }

  virtual void RecompileScriptEventListeners()
  {
  }

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsGenericElement)

protected:
  
















  nsresult SetAttrAndNotify(PRInt32 aNamespaceID,
                            nsIAtom* aName,
                            nsIAtom* aPrefix,
                            const nsAString& aOldValue,
                            nsAttrValue& aParsedValue,
                            PRBool aModification,
                            PRBool aFireMutation,
                            PRBool aNotify,
                            const nsAString* aValueForAfterSetAttr);

  










  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  












  virtual PRBool SetMappedAttribute(nsIDocument* aDocument,
                                    nsIAtom* aName,
                                    nsAttrValue& aValue,
                                    nsresult* aRetval);

  











  
  
  virtual nsresult BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify)
  {
    return NS_OK;
  }

  










  
  
  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify)
  {
    return NS_OK;
  }

  



  virtual nsresult
    GetEventListenerManagerForAttr(nsIEventListenerManager** aManager,
                                   nsISupports** aTarget,
                                   PRBool* aDefer);

  







  virtual nsAttrInfo GetAttrInfo(PRInt32 aNamespaceID, nsIAtom* aName) const;

  



  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  


  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  






  virtual void GetOffsetRect(nsRect& aRect, nsIContent** aOffsetParent);

  nsIFrame* GetStyledFrame();

public:
  
  
  







  class nsDOMSlots : public nsINode::nsSlots
  {
  public:
    nsDOMSlots(PtrBits aFlags);
    virtual ~nsDOMSlots();

    



    nsRefPtr<nsDOMCSSDeclaration> mStyle;

    



    nsRefPtr<nsDOMCSSDeclaration> mSMILOverrideStyle;

    


    nsCOMPtr<nsICSSStyleRule> mSMILOverrideStyleRule;

    



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

  nsDOMSlots *GetDOMSlots()
  {
    return static_cast<nsDOMSlots*>(GetSlots());
  }

  nsDOMSlots *GetExistingDOMSlots() const
  {
    return static_cast<nsDOMSlots*>(GetExistingSlots());
  }

  void RegisterFreezableElement() {
    nsIDocument* doc = GetOwnerDoc();
    if (doc) {
      doc->RegisterFreezableElement(this);
    }
  }
  void UnregisterFreezableElement() {
    nsIDocument* doc = GetOwnerDoc();
    if (doc) {
      doc->UnregisterFreezableElement(this);
    }
  }

  








  void GetContentsAsText(nsAString& aText);

  




  







  PRBool CheckHandleEventForLinksPrecondition(nsEventChainVisitor& aVisitor,
                                              nsIURI** aURI) const;

  


  nsresult PreHandleEventForLinks(nsEventChainPreVisitor& aVisitor);

  


  nsresult PostHandleEventForLinks(nsEventChainPostVisitor& aVisitor);

  









  virtual void GetLinkTarget(nsAString& aTarget);

  


  nsAttrAndChildArray mAttrsAndChildren;
};





#define NS_IMPL_ELEMENT_CLONE(_elementName)                                 \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const        \
{                                                                           \
  *aResult = nsnull;                                                        \
                                                                            \
  _elementName *it = new _elementName(aNodeInfo);                           \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsINode> kungFuDeathGrip = it;                                   \
  nsresult rv = CopyInnerTo(it);                                            \
  if (NS_SUCCEEDED(rv)) {                                                   \
    kungFuDeathGrip.swap(*aResult);                                         \
  }                                                                         \
                                                                            \
  return rv;                                                                \
}

#define NS_IMPL_ELEMENT_CLONE_WITH_INIT(_elementName)                       \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const        \
{                                                                           \
  *aResult = nsnull;                                                        \
                                                                            \
  _elementName *it = new _elementName(aNodeInfo);                           \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsINode> kungFuDeathGrip = it;                                   \
  nsresult rv = it->Init();                                                 \
  rv |= CopyInnerTo(it);                                                    \
  if (NS_SUCCEEDED(rv)) {                                                   \
    kungFuDeathGrip.swap(*aResult);                                         \
  }                                                                         \
                                                                            \
  return rv;                                                                \
}





class nsNSElementTearoff : public nsIDOMNSElement
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS

  NS_DECL_NSIDOMNSELEMENT

  NS_DECL_CYCLE_COLLECTION_CLASS(nsNSElementTearoff)

  nsNSElementTearoff(nsGenericElement *aContent) : mContent(aContent)
  {
  }

private:
  nsContentList* GetChildrenList();

  nsRefPtr<nsGenericElement> mContent;

  



  nsRect GetClientAreaRect();

private:

  






  void GetScrollInfo(nsIScrollableView **aScrollableView,
                     nsIFrame **aFrame = nsnull);
};

#define NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE                               \
    rv = nsGenericElement::QueryInterface(aIID, aInstancePtr);                \
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
