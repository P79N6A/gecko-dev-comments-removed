




































#ifndef nsGenericHTMLElement_h___
#define nsGenericHTMLElement_h___

#include "nsGenericElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsINameSpaceManager.h"  
#include "nsIFormControl.h"
#include "nsIDOMNSHTMLFrameElement.h"
#include "nsFrameLoader.h"
#include "nsGkAtoms.h"

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIDOMNodeList;
class nsIFrame;
class nsMappedAttributes;
class nsIStyleRule;
class nsISupportsArray;
class nsChildContentList;
class nsDOMCSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsIURI;
class nsIFormControlFrame;
class nsIForm;
class nsPresState;
class nsIScrollableView;
class nsILayoutHistoryState;
class nsIEditor;
struct nsRect;
struct nsSize;
struct nsRuleData;

typedef void (*nsMapRuleToAttributesFunc)(const nsMappedAttributes* aAttributes, 
                                          nsRuleData* aData);





class nsGenericHTMLElement : public nsGenericElement
{
public:
  nsGenericHTMLElement(nsINodeInfo *aNodeInfo)
    : nsGenericElement(aNodeInfo)
  {
  }

  
  static nsGenericHTMLElement* FromContent(nsIContent *aContent)
  {
    if (aContent->IsNodeOfType(eHTML))
      return static_cast<nsGenericHTMLElement*>(aContent);
    return nsnull;
  }

  
  static void Shutdown();

  







  nsresult DOMQueryInterface(nsIDOMHTMLElement *aElement, REFNSIID aIID,
                             void **aInstancePtr);

  
  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  
  NS_METHOD GetNodeName(nsAString& aNodeName);
  NS_METHOD GetLocalName(nsAString& aLocalName);

  
  NS_METHOD SetAttribute(const nsAString& aName,
                         const nsAString& aValue);
  NS_METHOD GetTagName(nsAString& aTagName);
  NS_METHOD GetElementsByTagName(const nsAString& aTagname,
                                 nsIDOMNodeList** aReturn);
  NS_METHOD GetElementsByTagNameNS(const nsAString& aNamespaceURI,
                                   const nsAString& aLocalName,
                                   nsIDOMNodeList** aReturn);

  
  
  
  nsresult GetId(nsAString& aId);
  nsresult SetId(const nsAString& aId);
  nsresult GetTitle(nsAString& aTitle);
  nsresult SetTitle(const nsAString& aTitle);
  nsresult GetLang(nsAString& aLang);
  nsresult SetLang(const nsAString& aLang);
  nsresult GetDir(nsAString& aDir);
  nsresult SetDir(const nsAString& aDir);
  nsresult GetClassName(nsAString& aClassName);
  nsresult SetClassName(const nsAString& aClassName);

  
  
  
  nsresult GetStyle(nsIDOMCSSStyleDeclaration** aStyle);
  nsresult GetOffsetTop(PRInt32* aOffsetTop);
  nsresult GetOffsetLeft(PRInt32* aOffsetLeft);
  nsresult GetOffsetWidth(PRInt32* aOffsetWidth);
  nsresult GetOffsetHeight(PRInt32* aOffsetHeight);
  nsresult GetOffsetParent(nsIDOMElement** aOffsetParent);
  virtual nsresult GetInnerHTML(nsAString& aInnerHTML);
  virtual nsresult SetInnerHTML(const nsAString& aInnerHTML);
  nsresult GetScrollTop(PRInt32* aScrollTop);
  nsresult SetScrollTop(PRInt32 aScrollTop);
  nsresult GetScrollLeft(PRInt32* aScrollLeft);
  nsresult SetScrollLeft(PRInt32 aScrollLeft);
  nsresult GetScrollHeight(PRInt32* aScrollHeight);
  nsresult GetScrollWidth(PRInt32* aScrollWidth);
  nsresult GetClientTop(PRInt32* aLength);
  nsresult GetClientLeft(PRInt32* aLength);
  nsresult GetClientHeight(PRInt32* aClientHeight);
  nsresult GetClientWidth(PRInt32* aClientWidth);
  nsresult ScrollIntoView(PRBool aTop);
  
  
  
  NS_IMETHOD Focus();
  NS_IMETHOD Blur();
  NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);
  NS_IMETHOD GetSpellcheck(PRBool* aSpellcheck);
  NS_IMETHOD SetSpellcheck(PRBool aSpellcheck);
  nsresult GetContentEditable(nsAString &aContentEditable);
  nsresult SetContentEditable(const nsAString &aContentEditable);

  






  void GetOffsetRect(nsRect& aRect, nsIContent** aOffsetParent);
  






  void GetScrollInfo(nsIScrollableView **aScrollableView,
                     nsIFrame **aFrame = nsnull);

  





  nsRect GetClientAreaRect();

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             PRBool aNotify);
  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;
  virtual void RemoveFocus(nsPresContext *aPresContext);
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);
  virtual void PerformAccesskey(PRBool aKeyCausesActivation,
                                PRBool aIsTrustedEvent);

  



  PRBool CheckHandleEventForAnchorsPreconditions(nsEventChainVisitor& aVisitor);
  nsresult PreHandleEventForAnchors(nsEventChainPreVisitor& aVisitor);
  nsresult PostHandleEventForAnchors(nsEventChainPostVisitor& aVisitor);
  PRBool IsHTMLLink(nsIURI** aURI) const;

  
  
  nsresult GetHrefURIForAnchors(nsIURI** aURI) const;

  
  void Compact() { mAttrsAndChildren.Compact(); }
  const nsAttrValue* GetParsedAttr(nsIAtom* aAttr) const
  {
    return mAttrsAndChildren.GetAttr(aAttr);
  }
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual void UpdateEditableState();

  virtual const nsAttrValue* GetClasses() const;
  virtual nsIAtom *GetIDAttributeName() const;
  virtual nsIAtom *GetClassAttributeName() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  virtual nsICSSStyleRule* GetInlineStyleRule();
  NS_IMETHOD SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify);
  already_AddRefed<nsIURI> GetBaseURI() const;

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual PRBool SetMappedAttribute(nsIDocument* aDocument,
                                    nsIAtom* aName,
                                    nsAttrValue& aValue,
                                    nsresult* aRetval);

  







  void GetBaseTarget(nsAString& aBaseTarget) const;

  






  nsIFormControlFrame* GetFormControlFrame(PRBool aFlushContent)
  {
    nsIDocument* doc = GetCurrentDoc();
    if (!doc) {
      return nsnull;
    }

    return GetFormControlFrameFor(this, doc, aFlushContent);
  }

  

  






  static PRBool ParseAlignValue(const nsAString& aString,
                                nsAttrValue& aResult);

  






  PRBool ParseDivAlignValue(const nsAString& aString,
                            nsAttrValue& aResult) const;

  






  PRBool ParseTableHAlignValue(const nsAString& aString,
                               nsAttrValue& aResult) const;

  






  PRBool ParseTableCellHAlignValue(const nsAString& aString,
                                   nsAttrValue& aResult) const;

  







  static PRBool ParseTableVAlignValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  







  static PRBool ParseImageAttribute(nsIAtom* aAttribute,
                                    const nsAString& aString,
                                    nsAttrValue& aResult);
  






  static PRBool ParseFrameborderValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  






  static PRBool ParseScrollingValue(const nsAString& aString,
                                    nsAttrValue& aResult);

  



  nsresult  ReparseStyleAttribute(void);
  







  static void ParseStyleAttribute(nsIContent* aContent,
                                  PRBool aCaseSensitive,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult);

  



  








  static void MapCommonAttributesInto(const nsMappedAttributes* aAttributes, 
                                      nsRuleData* aRuleData);
  static const MappedAttributeEntry sCommonAttributeMap[];
  static const MappedAttributeEntry sImageMarginSizeAttributeMap[];
  static const MappedAttributeEntry sImageBorderAttributeMap[];
  static const MappedAttributeEntry sImageAlignAttributeMap[];
  static const MappedAttributeEntry sDivAlignAttributeMap[];
  static const MappedAttributeEntry sBackgroundAttributeMap[];
  static const MappedAttributeEntry sBackgroundColorAttributeMap[];
  static const MappedAttributeEntry sScrollingAttributeMap[];
  
  






  static void MapImageAlignAttributeInto(const nsMappedAttributes* aAttributes,
                                         nsRuleData* aData);

  







  static void MapDivAlignAttributeInto(const nsMappedAttributes* aAttributes,
                                       nsRuleData* aData);

  






  static void MapImageBorderAttributeInto(const nsMappedAttributes* aAttributes,
                                          nsRuleData* aData);
  






  static void MapImageMarginAttributeInto(const nsMappedAttributes* aAttributes,
                                          nsRuleData* aData);
  






  static void MapImageSizeAttributesInto(const nsMappedAttributes* aAttributes,
                                         nsRuleData* aData);
  







  static void MapBackgroundInto(const nsMappedAttributes* aAttributes,
                                nsRuleData* aData);
  







  static void MapBGColorInto(const nsMappedAttributes* aAttributes,
                             nsRuleData* aData);
  







  static void MapBackgroundAttributesInto(const nsMappedAttributes* aAttributes,
                                          nsRuleData* aData);
  







  static void MapScrollingAttributeInto(const nsMappedAttributes* aAttributes,
                                        nsRuleData* aData);
  









  static nsIFormControlFrame* GetFormControlFrameFor(nsIContent* aContent,
                                                     nsIDocument* aDocument,
                                                     PRBool aFlushContent);
  






  static nsresult GetPrimaryPresState(nsGenericHTMLElement* aContent,
                                      nsPresState** aPresState);
  









  static nsresult GetLayoutHistoryAndKey(nsGenericHTMLElement* aContent,
                                         PRBool aRead,
                                         nsILayoutHistoryState** aState,
                                         nsACString& aKey);
  








  static PRBool RestoreFormControlState(nsGenericHTMLElement* aContent,
                                        nsIFormControl* aControl);

  



  NS_HIDDEN_(nsPresContext*) GetPresContext();

  
  







  already_AddRefed<nsIDOMHTMLFormElement> FindForm(nsIForm* aCurrentForm = nsnull);

  virtual void RecompileScriptEventListeners();

  



  static PRBool InNavQuirksMode(nsIDocument* aDoc);

  
  static nsresult SetProtocolInHrefString(const nsAString &aHref,
                                          const nsAString &aProtocol,
                                          nsAString &aResult);

  static nsresult SetHostInHrefString(const nsAString &aHref,
                                      const nsAString &aHost,
                                      nsAString &aResult);

  static nsresult SetHostnameInHrefString(const nsAString &aHref,
                                          const nsAString &aHostname,
                                          nsAString &aResult);

  static nsresult SetPathnameInHrefString(const nsAString &aHref,
                                          const nsAString &aHostname,
                                          nsAString &aResult);

  static nsresult SetSearchInHrefString(const nsAString &aHref,
                                        const nsAString &aSearch,
                                        nsAString &aResult);
  
  static nsresult SetHashInHrefString(const nsAString &aHref,
                                      const nsAString &aHash,
                                      nsAString &aResult);

  static nsresult SetPortInHrefString(const nsAString &aHref,
                                      const nsAString &aPort,
                                      nsAString &aResult);

  static nsresult GetProtocolFromHrefString(const nsAString &aHref,
                                            nsAString& aProtocol,
                                            nsIDocument *aDocument);

  static nsresult GetHostFromHrefString(const nsAString &aHref,
                                        nsAString& aHost);

  static nsresult GetHostnameFromHrefString(const nsAString &aHref,
                                            nsAString& aHostname);

  static nsresult GetPathnameFromHrefString(const nsAString &aHref,
                                            nsAString& aPathname);

  static nsresult GetSearchFromHrefString(const nsAString &aHref,
                                          nsAString& aSearch);

  static nsresult GetPortFromHrefString(const nsAString &aHref,
                                        nsAString& aPort);

  static nsresult GetHashFromHrefString(const nsAString &aHref,
                                        nsAString& aHash);
protected:
  





  void SetElementFocus(PRBool aDoFocus);
  




  void RegUnRegAccessKey(PRBool aDoReg);

  




  PRBool IsEventName(nsIAtom* aName);

  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);

  virtual nsresult
    GetEventListenerManagerForAttr(nsIEventListenerManager** aManager,
                                   nsISupports** aTarget,
                                   PRBool* aDefer);

  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  









  NS_HIDDEN_(nsresult) GetAttrHelper(nsIAtom* aAttr, nsAString& aValue);

  








  NS_HIDDEN_(nsresult) SetAttrHelper(nsIAtom* aAttr, const nsAString& aValue);

  








  NS_HIDDEN_(nsresult) GetStringAttrWithDefault(nsIAtom* aAttr,
                                                const char* aDefault,
                                                nsAString& aResult);

  







  NS_HIDDEN_(nsresult) GetBoolAttr(nsIAtom* aAttr, PRBool* aValue) const;

  







  NS_HIDDEN_(nsresult) SetBoolAttr(nsIAtom* aAttr, PRBool aValue);

  









  NS_HIDDEN_(nsresult) GetIntAttr(nsIAtom* aAttr, PRInt32 aDefault, PRInt32* aValue);

  







  NS_HIDDEN_(nsresult) SetIntAttr(nsIAtom* aAttr, PRInt32 aValue);

  










  NS_HIDDEN_(nsresult) GetURIAttr(nsIAtom* aAttr, nsIAtom* aBaseAttr, nsAString& aResult);

  











  NS_HIDDEN_(nsresult) GetURIListAttr(nsIAtom* aAttr, nsAString& aResult);

  


  NS_HIDDEN_(nsresult) GetEditor(nsIEditor** aEditor);
  NS_HIDDEN_(nsresult) GetEditorInternal(nsIEditor** aEditor);

  







  virtual already_AddRefed<nsIEditor> GetAssociatedEditor();

  


  PRBool IsCurrentBodyElement();

  



  static void SyncEditorsOnSubtree(nsIContent* content);

  enum ContentEditableTristate {
    eInherit = -1,
    eFalse = 0,
    eTrue = 1
  };

  





  NS_HIDDEN_(ContentEditableTristate) GetContentEditableValue() const
  {
    static const nsIContent::AttrValuesArray values[] =
      { &nsGkAtoms::_false, &nsGkAtoms::_true, &nsGkAtoms::_empty, nsnull };

    PRInt32 value = FindAttrValueIn(kNameSpaceID_None,
                                    nsGkAtoms::contenteditable, values,
                                    eIgnoreCase);

    return value > 0 ? eTrue : (value == 0 ? eFalse : eInherit);
  }

private:
  









  PRBool IsEditableRoot() const;

  void ChangeEditableState(PRInt32 aChange);
};







class nsGenericHTMLFormElement : public nsGenericHTMLElement,
                                 public nsIFormControl
{
public:
  nsGenericHTMLFormElement(nsINodeInfo *aNodeInfo);
  virtual ~nsGenericHTMLFormElement();

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  virtual PRBool IsNodeOfType(PRUint32 aFlags) const;

  
  NS_IMETHOD GetForm(nsIDOMHTMLFormElement** aForm);
  NS_IMETHOD SetForm(nsIDOMHTMLFormElement* aForm,
                     PRBool aRemoveFromForm,
                     PRBool aNotify);

  NS_IMETHOD SaveState()
  {
    return NS_OK;
  }
  virtual PRBool RestoreState(nsPresState* aState)
  {
    return PR_FALSE;
  }
  virtual PRBool AllowDrop()
  {
    return PR_TRUE;
  }
  
  virtual PRBool IsSubmitControl() const;

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             PRBool aNotify);
  virtual PRUint32 GetDesiredIMEState();
  virtual PRInt32 IntrinsicState() const;

protected:
  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);

  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);

  


  PRBool CanBeDisabled() const;

  void UpdateEditableFormControlState();

  void SetFocusAndScrollIntoView(nsPresContext* aPresContext);

  
  nsIForm* mForm;
};







class nsGenericHTMLFrameElement : public nsGenericHTMLElement,
                                  public nsIDOMNSHTMLFrameElement,
                                  public nsIFrameLoaderOwner
{
public:
  nsGenericHTMLFrameElement(nsINodeInfo *aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }
  virtual ~nsGenericHTMLFrameElement();

  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  
  NS_DECL_NSIDOMNSHTMLFRAMEELEMENT

  
  NS_DECL_NSIFRAMELOADEROWNER

  
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);

  
  NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsGenericHTMLFrameElement,
                                                     nsGenericHTMLElement)

protected:
  
  
  nsresult EnsureFrameLoader();
  nsresult LoadSrc();
  nsresult GetContentDocument(nsIDOMDocument** aContentDocument);

  nsCOMPtr<nsIFrameLoader> mFrameLoader;
};






#define NS_IMPL_NS_NEW_HTML_ELEMENT(_elementName)                            \
nsGenericHTMLElement*                                                        \
NS_NewHTML##_elementName##Element(nsINodeInfo *aNodeInfo, PRBool aFromParser)\
{                                                                            \
  return new nsHTML##_elementName##Element(aNodeInfo);                       \
}

#define NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(_elementName)               \
nsGenericHTMLElement*                                                        \
NS_NewHTML##_elementName##Element(nsINodeInfo *aNodeInfo, PRBool aFromParser)\
{                                                                            \
  return new nsHTML##_elementName##Element(aNodeInfo, aFromParser);          \
}







#define NS_IMPL_STRING_ATTR(_class, _method, _atom)                  \
  NS_IMETHODIMP                                                      \
  _class::Get##_method(nsAString& aValue)                            \
  {                                                                  \
    return GetAttrHelper(nsGkAtoms::_atom, aValue);                \
  }                                                                  \
  NS_IMETHODIMP                                                      \
  _class::Set##_method(const nsAString& aValue)                      \
  {                                                                  \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                \
  }






#define NS_IMPL_STRING_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                      \
  _class::Get##_method(nsAString& aValue)                            \
  {                                                                  \
    return GetStringAttrWithDefault(nsGkAtoms::_atom, _default, aValue);\
  }                                                                  \
  NS_IMETHODIMP                                                      \
  _class::Set##_method(const nsAString& aValue)                      \
  {                                                                  \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                \
  }






#define NS_IMPL_BOOL_ATTR(_class, _method, _atom)                     \
  NS_IMETHODIMP                                                       \
  _class::Get##_method(PRBool* aValue)                                \
  {                                                                   \
    return GetBoolAttr(nsGkAtoms::_atom, aValue);                   \
  }                                                                   \
  NS_IMETHODIMP                                                       \
  _class::Set##_method(PRBool aValue)                                 \
  {                                                                   \
    return SetBoolAttr(nsGkAtoms::_atom, aValue);                   \
  }






#define NS_IMPL_INT_ATTR(_class, _method, _atom)                    \
  NS_IMPL_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, -1)

#define NS_IMPL_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default)  \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(PRInt32* aValue)                                   \
  {                                                                       \
    return GetIntAttr(nsGkAtoms::_atom, _default, aValue);              \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(PRInt32 aValue)                                    \
  {                                                                       \
    return SetIntAttr(nsGkAtoms::_atom, aValue);                        \
  }








#define NS_IMPL_URI_ATTR(_class, _method, _atom)                    \
  NS_IMETHODIMP                                                     \
  _class::Get##_method(nsAString& aValue)                           \
  {                                                                 \
    return GetURIAttr(nsGkAtoms::_atom, nsnull, aValue);          \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  _class::Set##_method(const nsAString& aValue)                     \
  {                                                                 \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);               \
  }

#define NS_IMPL_URI_ATTR_WITH_BASE(_class, _method, _atom, _base_atom)       \
  NS_IMETHODIMP                                                              \
  _class::Get##_method(nsAString& aValue)                                    \
  {                                                                          \
    return GetURIAttr(nsGkAtoms::_atom, nsGkAtoms::_base_atom, aValue);  \
  }                                                                          \
  NS_IMETHODIMP                                                              \
  _class::Set##_method(const nsAString& aValue)                              \
  {                                                                          \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                        \
  }





#define NS_HTML_CONTENT_INTERFACE_MAP_AMBIGOUS_BEGIN(_class, _base, _base_if) \
  NS_IMETHODIMP _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)    \
  {                                                                           \
    NS_PRECONDITION(aInstancePtr, "null out param");                          \
                                                                              \
    nsresult rv;                                                              \
                                                                              \
    rv = _base::QueryInterface(aIID, aInstancePtr);                           \
                                                                              \
    if (NS_SUCCEEDED(rv))                                                     \
      return rv;                                                              \
                                                                              \
    rv = DOMQueryInterface(static_cast<_base_if *>(this), aIID,               \
                           aInstancePtr);                                     \
                                                                              \
    if (NS_SUCCEEDED(rv))                                                     \
      return rv;                                                              \
                                                                              \
    nsISupports *foundInterface = nsnull;


#define NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(_class, _base)                    \
  NS_HTML_CONTENT_INTERFACE_MAP_AMBIGOUS_BEGIN(_class, _base,                 \
                                               nsIDOMHTMLElement)

#define NS_HTML_CONTENT_CC_INTERFACE_MAP_AMBIGUOUS_BEGIN(_class, _base,       \
                                                        _base_if)             \
  NS_IMETHODIMP _class::QueryInterface(REFNSIID aIID, void** aInstancePtr)    \
  {                                                                           \
    NS_PRECONDITION(aInstancePtr, "null out param");                          \
                                                                              \
    if ( aIID.Equals(NS_GET_IID(nsXPCOMCycleCollectionParticipant)) ) {       \
      *aInstancePtr = &NS_CYCLE_COLLECTION_NAME(_class);                      \
      return NS_OK;                                                           \
    }                                                                         \
                                                                              \
    nsresult rv;                                                              \
                                                                              \
    rv = _base::QueryInterface(aIID, aInstancePtr);                           \
                                                                              \
    if (NS_SUCCEEDED(rv))                                                     \
      return rv;                                                              \
                                                                              \
    rv = DOMQueryInterface(static_cast<_base_if *>(this), aIID,               \
                           aInstancePtr);                                     \
                                                                              \
    if (NS_SUCCEEDED(rv))                                                     \
      return rv;                                                              \
                                                                              \
    nsISupports *foundInterface = nsnull;

#define NS_HTML_CONTENT_CC_INTERFACE_MAP_BEGIN(_class, _base)                 \
  NS_HTML_CONTENT_CC_INTERFACE_MAP_AMBIGUOUS_BEGIN(_class, _base,             \
                                                   nsIDOMHTMLElement)

#define NS_HTML_CONTENT_INTERFACE_MAP_END                                     \
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


#define NS_INTERFACE_MAP_ENTRY_IF_TAG(_interface, _tag)                       \
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(_interface,                              \
                                     mNodeInfo->Equals(nsGkAtoms::_tag))


#define NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(_class, _tag)         \
  if (mNodeInfo->Equals(nsGkAtoms::_tag) &&                                   \
      aIID.Equals(NS_GET_IID(nsIClassInfo))) {                                \
    foundInterface = NS_GetDOMClassInfoInstance(eDOMClassInfo_##_class##_id); \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else





#define NS_DECLARE_NS_NEW_HTML_ELEMENT(_elementName)              \
nsGenericHTMLElement*                                             \
NS_NewHTML##_elementName##Element(nsINodeInfo *aNodeInfo,         \
                                  PRBool aFromParser = PR_FALSE);

NS_DECLARE_NS_NEW_HTML_ELEMENT(Shared)
NS_DECLARE_NS_NEW_HTML_ELEMENT(SharedList)
NS_DECLARE_NS_NEW_HTML_ELEMENT(SharedObject)

NS_DECLARE_NS_NEW_HTML_ELEMENT(Anchor)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Area)
NS_DECLARE_NS_NEW_HTML_ELEMENT(BR)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Body)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Button)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Canvas)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Mod)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Div)
NS_DECLARE_NS_NEW_HTML_ELEMENT(FieldSet)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Font)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Form)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Frame)
NS_DECLARE_NS_NEW_HTML_ELEMENT(FrameSet)
NS_DECLARE_NS_NEW_HTML_ELEMENT(HR)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Head)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Heading)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Html)
NS_DECLARE_NS_NEW_HTML_ELEMENT(IFrame)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Image)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Input)
NS_DECLARE_NS_NEW_HTML_ELEMENT(LI)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Label)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Legend)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Link)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Map)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Meta)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Object)
NS_DECLARE_NS_NEW_HTML_ELEMENT(OptGroup)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Option)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Paragraph)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Pre)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Script)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Select)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Span)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Style)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableCaption)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableCell)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableCol)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Table)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableRow)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TableSection)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Tbody)
NS_DECLARE_NS_NEW_HTML_ELEMENT(TextArea)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Tfoot)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Thead)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Title)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Unknown)

#endif 
