










#ifndef nsGenericElement_h___
#define nsGenericElement_h___

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "mozilla/dom/FragmentOrElement.h"
#include "mozilla/dom/Element.h"
#include "nsIDOMElement.h"
#include "nsIDOMDocumentFragment.h"
#include "nsILinkHandler.h"
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
#include "nsContentList.h"
#include "nsDOMClassInfoID.h" 
#include "nsIDOMTouchEvent.h"
#include "nsIInlineEventHandlers.h"
#include "mozilla/CORSMode.h"
#include "mozilla/Attributes.h"
#include "nsContentUtils.h"
#include "nsISMILAttr.h"

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIFrame;
class nsIDOMNamedNodeMap;
class nsICSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsIURI;
class nsINodeInfo;
class nsIControllers;
class nsEventChainVisitor;
class nsEventListenerManager;
class nsIScrollableFrame;
class nsAttrValueOrString;
class nsContentList;
class nsDOMTokenList;
class ContentUnbinder;
struct nsRect;

typedef PRUptrdiff PtrBits;





class nsGenericElement : public mozilla::dom::Element
{
public:
  nsGenericElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual void UpdateEditableState(bool aNotify);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual nsIAtom *GetClassAttributeName() const;
  virtual already_AddRefed<nsINodeInfo> GetExistingAttrNameFromQName(const nsAString& aStr) const;
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nullptr, aValue, aNotify);
  }
  















  bool MaybeCheckSameAttrVal(PRInt32 aNamespaceID, nsIAtom* aName,
                             nsIAtom* aPrefix,
                             const nsAttrValueOrString& aValue,
                             bool aNotify, nsAttrValue& aOldValue,
                             PRUint8* aModType, bool* aHasListeners);

  bool OnlyNotifySameValueSet(PRInt32 aNamespaceID, nsIAtom* aName,
                              nsIAtom* aPrefix,
                              const nsAttrValueOrString& aValue,
                              bool aNotify, nsAttrValue& aOldValue,
                              PRUint8* aModType, bool* aHasListeners)
  {
    if (MaybeCheckSameAttrVal(aNamespaceID, aName, aPrefix, aValue, aNotify,
                              aOldValue, aModType, aHasListeners)) {
      nsAutoScriptBlocker scriptBlocker;
      nsNodeUtils::AttributeSetToCurrentValue(this, aNamespaceID, aName);
      return true;
    }
    return false;
  }

  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName, nsIAtom* aPrefix,
                           const nsAString& aValue, bool aNotify);
  virtual nsresult SetParsedAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 nsIAtom* aPrefix, nsAttrValue& aParsedValue,
                                 bool aNotify);
  virtual bool GetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                         nsAString& aResult) const;
  virtual bool HasAttr(PRInt32 aNameSpaceID, nsIAtom* aName) const;
  virtual bool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             const nsAString& aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual bool AttrValueIs(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aValue,
                             nsCaseTreatment aCaseSensitive) const;
  virtual PRInt32 FindAttrValueIn(PRInt32 aNameSpaceID,
                                  nsIAtom* aName,
                                  AttrValuesArray* aValues,
                                  nsCaseTreatment aCaseSensitive) const;
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                             bool aNotify);
  virtual const nsAttrName* GetAttrNameAt(PRUint32 aIndex) const;
  virtual PRUint32 GetAttrCount() const;
  virtual bool IsNodeOfType(PRUint32 aFlags) const;

  virtual nsISMILAttr* GetAnimatedAttr(PRInt32 , nsIAtom* )
  {
    return nullptr;
  }
  virtual nsICSSDeclaration* GetSMILOverrideStyle();
  virtual mozilla::css::StyleRule* GetSMILOverrideStyleRule();
  virtual nsresult SetSMILOverrideStyleRule(mozilla::css::StyleRule* aStyleRule,
                                            bool aNotify);
  virtual bool IsLabelable() const;

#ifdef DEBUG
  virtual void List(FILE* out, PRInt32 aIndent) const
  {
    List(out, aIndent, EmptyCString());
  }
  virtual void DumpContent(FILE* out, PRInt32 aIndent, bool aDumpAll) const;
  void List(FILE* out, PRInt32 aIndent, const nsCString& aPrefix) const;
  void ListAttributes(FILE* out) const;
#endif

  virtual mozilla::css::StyleRule* GetInlineStyleRule();
  virtual nsresult SetInlineStyleRule(mozilla::css::StyleRule* aStyleRule,
                                      const nsAString* aSerialized,
                                      bool aNotify);
  NS_IMETHOD_(bool)
    IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  


  struct MappedAttributeEntry {
    nsIAtom** attribute;
  };

  





  template<size_t N>
  static bool
  FindAttributeDependence(const nsIAtom* aAttribute,
                          const MappedAttributeEntry* const (&aMaps)[N])
  {
    return FindAttributeDependence(aAttribute, aMaps, N);
  }

private:
  static bool
  FindAttributeDependence(const nsIAtom* aAttribute,
                          const MappedAttributeEntry* const aMaps[],
                          PRUint32 aMapCount);

public:
  
  NS_DECL_NSIDOMELEMENT

  

  






  nsresult AddScriptEventListener(nsIAtom* aEventName,
                                  const nsAString& aValue,
                                  bool aDefer = true);

  


  nsresult LeaveLink(nsPresContext* aPresContext);

  static bool ShouldBlur(nsIContent *aContent);

  






  static nsresult DispatchClickEvent(nsPresContext* aPresContext,
                                     nsInputEvent* aSourceEvent,
                                     nsIContent* aTarget,
                                     bool aFullDispatch,
                                     PRUint32 aFlags,
                                     nsEventStatus* aStatus);

  






  using nsIContent::DispatchEvent;
  static nsresult DispatchEvent(nsPresContext* aPresContext,
                                nsEvent* aEvent,
                                nsIContent* aTarget,
                                bool aFullDispatch,
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

    return slots ? slots->mAttributeMap.get() : nullptr;
  }

  virtual void RecompileScriptEventListeners()
  {
  }

  PRInt32 GetScrollTop();
  PRInt32 GetScrollLeft();
  PRInt32 GetScrollHeight();
  PRInt32 GetScrollWidth();
  PRInt32 GetScrollLeftMax();
  PRInt32 GetScrollTopMax();
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
  nsDOMTokenList* GetClassList(nsresult *aResult);
  bool MozMatchesSelector(const nsAString& aSelector, nsresult* aResult);

  







  virtual nsAttrInfo GetAttrInfo(PRInt32 aNamespaceID, nsIAtom* aName) const;

  virtual void NodeInfoChanged(nsINodeInfo* aOldNodeInfo)
  {
  }

  




  static void ParseCORSValue(const nsAString& aValue, nsAttrValue& aResult);

  


  static mozilla::CORSMode StringToCORSMode(const nsAString& aValue);
  
  



  static mozilla::CORSMode AttrValueToCORSMode(const nsAttrValue* aValue);

protected:
  



  static const bool kFireMutationEvent           = true;
  static const bool kDontFireMutationEvent       = false;
  static const bool kNotifyDocumentObservers     = true;
  static const bool kDontNotifyDocumentObservers = false;
  static const bool kCallAfterSetAttr            = true;
  static const bool kDontCallAfterSetAttr        = false;

  





















  nsresult SetAttrAndNotify(PRInt32 aNamespaceID,
                            nsIAtom* aName,
                            nsIAtom* aPrefix,
                            const nsAttrValue& aOldValue,
                            nsAttrValue& aParsedValue,
                            PRUint8 aModType,
                            bool aFireMutation,
                            bool aNotify,
                            bool aCallAfterSetAttr);

  










  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  












  virtual bool SetMappedAttribute(nsIDocument* aDocument,
                                    nsIAtom* aName,
                                    nsAttrValue& aValue,
                                    nsresult* aRetval);

  













  
  
  virtual nsresult BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                 const nsAttrValueOrString* aValue,
                                 bool aNotify)
  {
    return NS_OK;
  }

  











  
  
  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAttrValue* aValue, bool aNotify)
  {
    return NS_OK;
  }

  



  virtual nsEventListenerManager*
    GetEventListenerManagerForAttr(nsIAtom* aAttrName, bool* aDefer);

  


  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  






  virtual void GetOffsetRect(nsRect& aRect, nsIContent** aOffsetParent);

  




  nsIntSize GetPaddingRectSize();

  nsIFrame* GetStyledFrame();

  virtual mozilla::dom::Element* GetNameSpaceElement()
  {
    return this;
  }

  nsresult GetAttributeNodeNSInternal(const nsAString& aNamespaceURI,
                                      const nsAString& aLocalName,
                                      nsIDOMAttr** aReturn);

  void RegisterFreezableElement() {
    OwnerDoc()->RegisterFreezableElement(this);
  }
  void UnregisterFreezableElement() {
    OwnerDoc()->UnregisterFreezableElement(this);
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

  




  







  bool CheckHandleEventForLinksPrecondition(nsEventChainVisitor& aVisitor,
                                              nsIURI** aURI) const;

  


  nsresult PreHandleEventForLinks(nsEventChainPreVisitor& aVisitor);

  


  nsresult PostHandleEventForLinks(nsEventChainPostVisitor& aVisitor);

  









  virtual void GetLinkTarget(nsAString& aTarget);

private:
  



  nsRect GetClientAreaRect();

  nsIScrollableFrame* GetScrollFrame(nsIFrame **aStyledFrame = nullptr);
};





#define NS_IMPL_ELEMENT_CLONE(_elementName)                                 \
nsresult                                                                    \
_elementName::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const        \
{                                                                           \
  *aResult = nullptr;                                                        \
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;                                     \
  _elementName *it = new _elementName(ni.forget());                         \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsINode> kungFuDeathGrip = it;                                   \
  nsresult rv = const_cast<_elementName*>(this)->CopyInnerTo(it);           \
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
  *aResult = nullptr;                                                        \
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;                                     \
  _elementName *it = new _elementName(ni.forget());                         \
  if (!it) {                                                                \
    return NS_ERROR_OUT_OF_MEMORY;                                          \
  }                                                                         \
                                                                            \
  nsCOMPtr<nsINode> kungFuDeathGrip = it;                                   \
  nsresult rv = it->Init();                                                 \
  nsresult rv2 = const_cast<_elementName*>(this)->CopyInnerTo(it);          \
  if (NS_FAILED(rv2)) {                                                     \
    rv = rv2;                                                               \
  }                                                                         \
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








#define NS_IMPL_STRING_ATTR(_class, _method, _atom)                     \
  NS_IMETHODIMP                                                         \
  _class::Get##_method(nsAString& aValue)                               \
  {                                                                     \
    GetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aValue);               \
    return NS_OK;                                                       \
  }                                                                     \
  NS_IMETHODIMP                                                         \
  _class::Set##_method(const nsAString& aValue)                         \
  {                                                                     \
    return SetAttr(kNameSpaceID_None, nsGkAtoms::_atom, nullptr, aValue, true); \
  }

#endif 
