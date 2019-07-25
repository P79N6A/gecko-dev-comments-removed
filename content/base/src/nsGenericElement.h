










































#ifndef nsGenericElement_h___
#define nsGenericElement_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/Element.h"
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
#include "nsPresContext.h"
#include "nsIDOMDOMStringMap.h"

#ifdef MOZ_SMIL
#include "nsISMILAttr.h"
#endif 

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIFrame;
class nsIDOMNamedNodeMap;
class nsICSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsIURI;
class nsINodeInfo;
class nsIControllers;
class nsIDOMNSFeatureFactory;
class nsIEventListenerManager;
class nsIScrollableFrame;
class nsContentList;
class nsDOMTokenList;
struct nsRect;

typedef PRUptrdiff PtrBits;







class nsChildContentList : public nsINodeList
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

  virtual nsINode* GetParentObject()
  {
    return mNode;
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

  nsNode3Tearoff(nsINode *aNode) : mNode(aNode)
  {
  }

protected:
  virtual ~nsNode3Tearoff() {}

private:
  nsCOMPtr<nsINode> mNode;
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

  nsNodeSelectorTearoff(nsINode *aNode) : mNode(aNode)
  {
  }

private:
  ~nsNodeSelectorTearoff() {}

private:
  nsCOMPtr<nsINode> mNode;
};


class nsNSElementTearoff;





class nsGenericElement : public mozilla::dom::Element
{
public:
  nsGenericElement(already_AddRefed<nsINodeInfo> aNodeInfo);
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
  virtual nsresult RemoveChildAt(PRUint32 aIndex, PRBool aNotify);
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
  virtual void GetTextContent(nsAString &aTextContent)
  {
    nsContentUtils::GetNodeTextContent(this, PR_TRUE, aTextContent);
  }
  virtual nsresult SetTextContent(const nsAString& aTextContent)
  {
    return nsContentUtils::SetNodeTextContent(this, aTextContent, PR_FALSE);
  }

  
  virtual void UpdateEditableState(PRBool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual already_AddRefed<nsINodeList> GetChildren(PRUint32 aFilter);
  virtual nsIAtom *GetClassAttributeName() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  














  PRBool MaybeCheckSameAttrVal(PRInt32 aNamespaceID, nsIAtom* aName,
                               nsIAtom* aPrefix, const nsAString& aValue,
                               PRBool aNotify, nsAutoString* aOldValue,
                               PRUint8* aModType, PRBool* aHasListeners);
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                           const nsAString& aValue, PRBool aNotify);
  virtual nsresult SetParsedAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 nsIAtom* aPrefix, nsAttrValue& aParsedValue,
                                 PRBool aNotify);
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
  virtual PRBool IsLink(nsIURI** aURI) const;

  virtual PRUint32 GetScriptTypeID() const;
  NS_IMETHOD SetScriptTypeID(PRUint32 aLang);

  virtual void DestroyContent();
  virtual void SaveSubtreeState();

#ifdef MOZ_SMIL
  virtual nsISMILAttr* GetAnimatedAttr(PRInt32 , nsIAtom* )
  {
    return nsnull;
  }
  virtual nsIDOMCSSStyleDeclaration* GetSMILOverrideStyle();
  virtual mozilla::css::StyleRule* GetSMILOverrideStyleRule();
  virtual nsresult SetSMILOverrideStyleRule(mozilla::css::StyleRule* aStyleRule,
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

  virtual const nsAttrValue* DoGetClasses() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  virtual mozilla::css::StyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(mozilla::css::StyleRule* aStyleRule, PRBool aNotify);
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
  NS_IMETHOD Normalize();
  NS_IMETHOD IsSupported(const nsAString& aFeature,
                         const nsAString& aVersion, PRBool* aReturn);
  NS_IMETHOD HasAttributes(PRBool* aHasAttributes);
  NS_IMETHOD HasChildNodes(PRBool* aHasChildNodes);
  nsresult InsertBefore(nsIDOMNode* aNewChild, nsIDOMNode* aRefChild,
                        nsIDOMNode** aReturn)
  {
    return ReplaceOrInsertBefore(PR_FALSE, aNewChild, aRefChild, aReturn);
  }
  nsresult ReplaceChild(nsIDOMNode* aNewChild, nsIDOMNode* aOldChild,
                        nsIDOMNode** aReturn)
  {
    return ReplaceOrInsertBefore(PR_TRUE, aNewChild, aOldChild, aReturn);
  }
  nsresult RemoveChild(nsIDOMNode* aOldChild, nsIDOMNode** aReturn)
  {
    return nsINode::RemoveChild(aOldChild, aReturn);
  }
  nsresult AppendChild(nsIDOMNode* aNewChild, nsIDOMNode** aReturn)
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
    return nsNodeUtils::CloneNodeImpl(this, aDeep, PR_TRUE, aResult);
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

  static PRBool ShouldBlur(nsIContent *aContent);

  



  static void FireNodeInserted(nsIDocument* aDoc,
                               nsINode* aParent,
                               nsTArray<nsCOMPtr<nsIContent> >& aNodes);

  


  static nsIContent* doQuerySelector(nsINode* aRoot, const nsAString& aSelector,
                                     nsresult *aResult NS_OUTPARAM);
  static nsresult doQuerySelectorAll(nsINode* aRoot,
                                     const nsAString& aSelector,
                                     nsIDOMNodeList **aReturn);

  


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

  






  nsIFrame* GetPrimaryFrame(mozFlushType aType);
  
  nsIFrame* GetPrimaryFrame() const { return nsIContent::GetPrimaryFrame(); }

  



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

  
  nsresult GetElementsByClassName(const nsAString& aClasses,
                                  nsIDOMNodeList** aReturn)
  {
    return nsContentUtils::GetElementsByClassName(this, aClasses, aReturn);
  }
  nsresult GetClientRects(nsIDOMClientRectList** aResult);
  nsresult GetBoundingClientRect(nsIDOMClientRect** aResult);
  PRInt32 GetScrollTop();
  void SetScrollTop(PRInt32 aScrollTop);
  PRInt32 GetScrollLeft();
  void SetScrollLeft(PRInt32 aScrollLeft);
  PRInt32 GetScrollHeight();
  PRInt32 GetScrollWidth();
  PRInt32 GetClientTop()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().y);
  }
  PRInt32 GetClientLeft()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().x);
  }
  PRInt32 GetClientHeight()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().height);
  }
  PRInt32 GetClientWidth()
  {
    return nsPresContext::AppUnitsToIntCSSPixels(GetClientAreaRect().width);
  }
  nsIContent* GetFirstElementChild();
  nsIContent* GetLastElementChild();
  nsIContent* GetPreviousElementSibling();
  nsIContent* GetNextElementSibling();
  nsresult GetChildElementCount(PRUint32* aResult)
  {
    nsContentList* list = GetChildrenList();
    if (!list) {
      *aResult = 0;

      return NS_ERROR_OUT_OF_MEMORY;
    }

    *aResult = list->Length(PR_TRUE);

    return NS_OK;
  }
  nsresult GetChildren(nsIDOMNodeList** aResult)
  {
    nsContentList* list = GetChildrenList();
    if (!list) {
      *aResult = nsnull;

      return NS_ERROR_OUT_OF_MEMORY;
    }

    NS_ADDREF(*aResult = list);

    return NS_OK;
  }
  nsIDOMDOMTokenList* GetClassList(nsresult *aResult);
  void SetCapture(PRBool aRetargetToElement);
  void ReleaseCapture();
  PRBool MozMatchesSelector(const nsAString& aSelector, nsresult* aResult);

  







  virtual nsAttrInfo GetAttrInfo(PRInt32 aNamespaceID, nsIAtom* aName) const;

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(nsGenericElement)

  virtual void NodeInfoChanged(nsINodeInfo* aOldNodeInfo)
  {
  }

  


  void FireNodeRemovedForChildren();

protected:
  



















  nsresult SetAttrAndNotify(PRInt32 aNamespaceID,
                            nsIAtom* aName,
                            nsIAtom* aPrefix,
                            const nsAString& aOldValue,
                            nsAttrValue& aParsedValue,
                            PRUint8 aModType,
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

  



  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  


  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  






  virtual void GetOffsetRect(nsRect& aRect, nsIContent** aOffsetParent);

  nsIFrame* GetStyledFrame();

  virtual mozilla::dom::Element* GetNameSpaceElement()
  {
    return this;
  }

public:
  
  
  







  class nsDOMSlots : public nsINode::nsSlots
  {
  public:
    nsDOMSlots();
    virtual ~nsDOMSlots();

    




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

  


  void AddToIdTable(nsIAtom* aId) {
    NS_ASSERTION(HasID(), "Node doesn't have an ID?");
    nsIDocument* doc = GetCurrentDoc();
    if (doc && (!IsInAnonymousSubtree() || doc->IsXUL())) {
      doc->AddToIdTable(this, aId);
    }
  }
  void RemoveFromIdTable() {
    if (HasID()) {
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        nsIAtom* id = DoGetID();
        
        
        
        if (id) {
          doc->RemoveFromIdTable(this, DoGetID());
        }
      }
    }
  }

  




  







  PRBool CheckHandleEventForLinksPrecondition(nsEventChainVisitor& aVisitor,
                                              nsIURI** aURI) const;

  


  nsresult PreHandleEventForLinks(nsEventChainPreVisitor& aVisitor);

  


  nsresult PostHandleEventForLinks(nsEventChainPostVisitor& aVisitor);

  









  virtual void GetLinkTarget(nsAString& aTarget);

  


  nsAttrAndChildArray mAttrsAndChildren;

private:
  



  nsRect GetClientAreaRect();

  nsIScrollableFrame* GetScrollFrame(nsIFrame **aStyledFrame = nsnull);

  nsContentList* GetChildrenList();
};





#define NS_IMPL_ELEMENT_CLONE(_elementName)                                 \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const        \
{                                                                           \
  *aResult = nsnull;                                                        \
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;                                     \
  _elementName *it = new _elementName(ni.forget());                         \
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
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;                                     \
  _elementName *it = new _elementName(ni.forget());                         \
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

#define DOMCI_NODE_DATA(_interface, _class)                             \
  DOMCI_DATA(_interface, _class)                                        \
  nsXPCClassInfo* _class::GetClassInfo()                                \
  {                                                                     \
    return static_cast<nsXPCClassInfo*>(                                \
      NS_GetDOMClassInfoInstance(eDOMClassInfo_##_interface##_id));     \
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
  nsRefPtr<nsGenericElement> mContent;
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
