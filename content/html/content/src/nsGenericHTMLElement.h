




































#ifndef nsGenericHTMLElement_h___
#define nsGenericHTMLElement_h___

#include "nsMappedAttributeElement.h"
#include "nsIDOMHTMLElement.h"
#include "nsINameSpaceManager.h"  
#include "nsIFormControl.h"
#include "nsIDOMHTMLFrameElement.h"
#include "nsFrameLoader.h"
#include "nsGkAtoms.h"
#include "nsContentCreatorFunctions.h"
#include "nsDOMMemoryReporter.h"

class nsIDOMAttr;
class nsIDOMEventListener;
class nsIDOMNodeList;
class nsIFrame;
class nsIStyleRule;
class nsChildContentList;
class nsDOMCSSDeclaration;
class nsIDOMCSSStyleDeclaration;
class nsIURI;
class nsIFormControlFrame;
class nsIForm;
class nsPresState;
class nsILayoutHistoryState;
class nsIEditor;
struct nsRect;
struct nsSize;
class nsHTMLFormElement;
class nsIDOMDOMStringMap;
class nsIDOMHTMLMenuElement;

typedef nsMappedAttributeElement nsGenericHTMLElementBase;




class nsGenericHTMLElement : public nsGenericHTMLElementBase
{
public:
  nsGenericHTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElementBase(aNodeInfo)
  {
    NS_ASSERTION(mNodeInfo->NamespaceID() == kNameSpaceID_XHTML,
                 "Unexpected namespace");
  }

  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(nsGenericHTMLElement,
                                              nsGenericHTMLElementBase)

  
  static nsGenericHTMLElement* FromContent(nsIContent *aContent)
  {
    if (aContent->IsHTML())
      return static_cast<nsGenericHTMLElement*>(aContent);
    return nsnull;
  }

  







  nsresult DOMQueryInterface(nsIDOMHTMLElement *aElement, REFNSIID aIID,
                             void **aInstancePtr);

  
  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  
  NS_METHOD SetAttribute(const nsAString& aName,
                         const nsAString& aValue);

  
  
  
  nsresult GetId(nsAString& aId);
  nsresult SetId(const nsAString& aId);
  nsresult GetTitle(nsAString& aTitle);
  nsresult SetTitle(const nsAString& aTitle);
  nsresult GetLang(nsAString& aLang);
  nsresult SetLang(const nsAString& aLang);
  NS_IMETHOD GetDir(nsAString& aDir);
  NS_IMETHOD SetDir(const nsAString& aDir);
  nsresult GetClassName(nsAString& aClassName);
  nsresult SetClassName(const nsAString& aClassName);
  nsresult GetOffsetTop(PRInt32* aOffsetTop);
  nsresult GetOffsetLeft(PRInt32* aOffsetLeft);
  nsresult GetOffsetWidth(PRInt32* aOffsetWidth);
  nsresult GetOffsetHeight(PRInt32* aOffsetHeight);
  nsresult GetOffsetParent(nsIDOMElement** aOffsetParent);
  NS_IMETHOD GetInnerHTML(nsAString& aInnerHTML);
  NS_IMETHOD SetInnerHTML(const nsAString& aInnerHTML);
  NS_IMETHOD GetOuterHTML(nsAString& aOuterHTML);
  NS_IMETHOD SetOuterHTML(const nsAString& aOuterHTML);
  NS_IMETHOD InsertAdjacentHTML(const nsAString& aPosition,
                                const nsAString& aText);
  nsresult ScrollIntoView(bool aTop, PRUint8 optional_argc);
  nsresult MozRequestFullScreen();
  
  
  
  NS_IMETHOD Focus();
  NS_IMETHOD Blur();
  NS_IMETHOD Click();
  NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);
  NS_IMETHOD GetHidden(bool* aHidden);
  NS_IMETHOD SetHidden(bool aHidden);
  NS_IMETHOD GetSpellcheck(bool* aSpellcheck);
  NS_IMETHOD SetSpellcheck(bool aSpellcheck);
  NS_IMETHOD GetDraggable(bool* aDraggable);
  NS_IMETHOD SetDraggable(bool aDraggable);
  NS_IMETHOD GetAccessKey(nsAString &aAccessKey);
  NS_IMETHOD SetAccessKey(const nsAString& aAccessKey);
  NS_IMETHOD GetAccessKeyLabel(nsAString& aLabel);
  nsresult GetContentEditable(nsAString& aContentEditable);
  nsresult GetIsContentEditable(bool* aContentEditable);
  nsresult SetContentEditable(const nsAString &aContentEditable);
  nsresult GetDataset(nsIDOMDOMStringMap** aDataset);
  
  nsresult ClearDataset();
  nsresult GetContextMenu(nsIDOMHTMLMenuElement** aContextMenu);

protected:
  nsresult GetMarkup(bool aIncludeSelf, nsAString& aMarkup);

public:
  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual nsresult UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             bool aNotify);
  virtual bool IsFocusable(PRInt32 *aTabIndex = nsnull, bool aWithMouse = false)
  {
    bool isFocusable = false;
    IsHTMLFocusable(aWithMouse, &isFocusable, aTabIndex);
    return isFocusable;
  }
  



  virtual bool IsHTMLFocusable(bool aWithMouse,
                                 bool *aIsFocusable,
                                 PRInt32 *aTabIndex);
  virtual void PerformAccesskey(bool aKeyCausesActivation,
                                bool aIsTrustedEvent);

  



  bool CheckHandleEventForAnchorsPreconditions(nsEventChainVisitor& aVisitor);
  nsresult PreHandleEventForAnchors(nsEventChainPreVisitor& aVisitor);
  nsresult PostHandleEventForAnchors(nsEventChainPostVisitor& aVisitor);
  bool IsHTMLLink(nsIURI** aURI) const;

  
  void Compact() { mAttrsAndChildren.Compact(); }

  virtual void UpdateEditableState(bool aNotify);

  
  void DoSetEditableFlag(bool aEditable, bool aNotify) {
    SetEditableFlag(aEditable);
    UpdateState(aNotify);
  }

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);

  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  







  void GetBaseTarget(nsAString& aBaseTarget) const;

  






  nsIFormControlFrame* GetFormControlFrame(bool aFlushFrames);

  

  






  static bool ParseAlignValue(const nsAString& aString,
                                nsAttrValue& aResult);

  






  static bool ParseDivAlignValue(const nsAString& aString,
                                   nsAttrValue& aResult);

  






  static bool ParseTableHAlignValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  






  static bool ParseTableCellHAlignValue(const nsAString& aString,
                                          nsAttrValue& aResult);

  







  static bool ParseTableVAlignValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  







  static bool ParseImageAttribute(nsIAtom* aAttribute,
                                    const nsAString& aString,
                                    nsAttrValue& aResult);
  






  static bool ParseFrameborderValue(const nsAString& aString,
                                      nsAttrValue& aResult);

  






  static bool ParseScrollingValue(const nsAString& aString,
                                    nsAttrValue& aResult);

  



  








  static void MapCommonAttributesInto(const nsMappedAttributes* aAttributes, 
                                      nsRuleData* aRuleData);

  




  static void MapCommonAttributesExceptHiddenInto(const nsMappedAttributes* aAttributes,
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
  






  static nsresult GetPrimaryPresState(nsGenericHTMLElement* aContent,
                                      nsPresState** aPresState);
  









  static nsresult GetLayoutHistoryAndKey(nsGenericHTMLElement* aContent,
                                         bool aRead,
                                         nsILayoutHistoryState** aState,
                                         nsACString& aKey);
  








  static bool RestoreFormControlState(nsGenericHTMLElement* aContent,
                                        nsIFormControl* aControl);

  



  NS_HIDDEN_(nsPresContext*) GetPresContext();

  
  








  nsHTMLFormElement* FindAncestorForm(nsHTMLFormElement* aCurrentForm = nsnull);

  virtual void RecompileScriptEventListeners();

  



  static bool InNavQuirksMode(nsIDocument* aDoc);

  


  NS_HIDDEN_(nsresult) GetEditor(nsIEditor** aEditor);
  NS_HIDDEN_(nsresult) GetEditorInternal(nsIEditor** aEditor);

  










  NS_HIDDEN_(nsresult) GetURIAttr(nsIAtom* aAttr, nsIAtom* aBaseAttr, nsAString& aResult);

  


  virtual bool IsDisabled() const {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);
  }

  bool IsHidden() const
  {
    return HasAttr(kNameSpaceID_None, nsGkAtoms::hidden);
  }

protected:
  


  void AddToNameTable(nsIAtom* aName) {
    NS_ASSERTION(HasName(), "Node doesn't have name?");
    nsIDocument* doc = GetCurrentDoc();
    if (doc && !IsInAnonymousSubtree()) {
      doc->AddToNameTable(this, aName);
    }
  }
  void RemoveFromNameTable() {
    if (HasName()) {
      nsIDocument* doc = GetCurrentDoc();
      if (doc) {
        doc->RemoveFromNameTable(this, GetParsedAttr(nsGkAtoms::name)->
                                         GetAtomValue());
      }
    }
  }

  



  void RegAccessKey()
  {
    if (HasFlag(NODE_HAS_ACCESSKEY)) {
      RegUnRegAccessKey(true);
    }
  }

  void UnregAccessKey()
  {
    if (HasFlag(NODE_HAS_ACCESSKEY)) {
      RegUnRegAccessKey(false);
    }
  }

private:
  







  void FireMutationEventsForDirectParsing(nsIDocument* aDoc,
                                          nsIContent* aDest,
                                          PRInt32 aOldChildCount);

  void RegUnRegAccessKey(bool aDoReg);

protected:
  




  bool IsEventName(nsIAtom* aName);

  virtual nsresult AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, bool aNotify);

  virtual nsEventListenerManager*
    GetEventListenerManagerForAttr(nsIAtom* aAttrName, bool* aDefer);

  virtual const nsAttrName* InternalGetExistingAttrNameFromQName(const nsAString& aStr) const;

  









  NS_HIDDEN_(nsresult) GetAttrHelper(nsIAtom* aAttr, nsAString& aValue);

  








  NS_HIDDEN_(nsresult) SetAttrHelper(nsIAtom* aAttr, const nsAString& aValue);

  







  NS_HIDDEN_(nsresult) GetBoolAttr(nsIAtom* aAttr, bool* aValue) const;

  







  NS_HIDDEN_(nsresult) SetBoolAttr(nsIAtom* aAttr, bool aValue);

  









  NS_HIDDEN_(nsresult) GetIntAttr(nsIAtom* aAttr, PRInt32 aDefault, PRInt32* aValue);

  







  NS_HIDDEN_(nsresult) SetIntAttr(nsIAtom* aAttr, PRInt32 aValue);

  









  NS_HIDDEN_(nsresult) GetUnsignedIntAttr(nsIAtom* aAttr, PRUint32 aDefault,
                                          PRUint32* aValue);

  







  NS_HIDDEN_(nsresult) SetUnsignedIntAttr(nsIAtom* aAttr, PRUint32 aValue);

  









  NS_HIDDEN_(nsresult) GetDoubleAttr(nsIAtom* aAttr, double aDefault, double* aValue);

  







  NS_HIDDEN_(nsresult) SetDoubleAttr(nsIAtom* aAttr, double aValue);

  





  NS_HIDDEN_(bool) GetURIAttr(nsIAtom* aAttr, nsIAtom* aBaseAttr, nsIURI** aURI) const;

  











  NS_HIDDEN_(nsresult) GetURIListAttr(nsIAtom* aAttr, nsAString& aResult);

  








  NS_HIDDEN_(nsresult) GetEnumAttr(nsIAtom* aAttr,
                                   const char* aDefault,
                                   nsAString& aResult);

  







  virtual already_AddRefed<nsIEditor> GetAssociatedEditor();

  






  virtual void GetOffsetRect(nsRect& aRect, nsIContent** aOffsetParent);

  


  bool IsCurrentBodyElement();

  



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

    if (!MayHaveContentEditableAttr())
      return eInherit;

    PRInt32 value = FindAttrValueIn(kNameSpaceID_None,
                                    nsGkAtoms::contenteditable, values,
                                    eIgnoreCase);

    return value > 0 ? eTrue : (value == 0 ? eFalse : eInherit);
  }

  
  already_AddRefed<nsIURI> GetHrefURIForAnchors() const;

  









  bool IsEditableRoot() const;

private:
  void ChangeEditableState(PRInt32 aChange);
};




class nsHTMLFieldSetElement;




class nsGenericHTMLFormElement : public nsGenericHTMLElement,
                                 public nsIFormControl
{
public:
  nsGenericHTMLFormElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsGenericHTMLFormElement();

  NS_DECL_AND_IMPL_DOM_MEMORY_REPORTER_SIZEOF(nsGenericHTMLFormElement,
                                              nsGenericHTMLElement)

  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  virtual bool IsNodeOfType(PRUint32 aFlags) const;
  virtual void SaveSubtreeState();

  
  virtual mozilla::dom::Element* GetFormElement();
  virtual void SetForm(nsIDOMHTMLFormElement* aForm);
  virtual void ClearForm(bool aRemoveFromForm);

  nsresult GetForm(nsIDOMHTMLFormElement** aForm);

  NS_IMETHOD SaveState()
  {
    return NS_OK;
  }
  
  virtual bool RestoreState(nsPresState* aState)
  {
    return false;
  }
  virtual bool AllowDrop()
  {
    return true;
  }

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  virtual PRUint32 GetDesiredIMEState();
  virtual nsEventStates IntrinsicState() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);

  virtual bool IsDisabled() const;

  








  virtual void FieldSetDisabledChanged(bool aNotify);

  void FieldSetFirstLegendChanged(bool aNotify) {
    UpdateFieldSet(aNotify);
  }

  






  void ForgetFieldSet(nsIContent* aFieldset);

  


  bool CanBeDisabled() const;

  virtual bool IsHTMLFocusable(bool aWithMouse, bool* aIsFocusable,
                                 PRInt32* aTabIndex);

protected:
  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, bool aNotify);

  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, bool aNotify);

  void UpdateEditableFormControlState(bool aNotify);

  










  void UpdateFormOwner(bool aBindToTree, Element* aFormIdElement);

  


  void UpdateFieldSet(bool aNotify);

  





  Element* AddFormIdObserver();

  


  void RemoveFormIdObserver();

  




  static bool FormIdUpdated(Element* aOldElement, Element* aNewElement,
                              void* aData);

  
  virtual bool IsElementDisabledForEvents(PRUint32 aMessage, nsIFrame* aFrame);

  
  
  
  enum FocusTristate {
    eUnfocusable,
    eInactiveWindow,
    eActiveWindow
  };

  
  
  FocusTristate FocusState();

  
  nsHTMLFormElement* mForm;

  
  nsHTMLFieldSetElement* mFieldSet;
};





#define ADDED_TO_FORM (1 << ELEMENT_TYPE_SPECIFIC_BITS_OFFSET)




#define MAYBE_ORPHAN_FORM_ELEMENT (1 << (ELEMENT_TYPE_SPECIFIC_BITS_OFFSET+1))






PR_STATIC_ASSERT(ELEMENT_TYPE_SPECIFIC_BITS_OFFSET + 1 < 32);







class nsGenericHTMLFrameElement : public nsGenericHTMLElement,
                                  public nsIFrameLoaderOwner
{
public:
  nsGenericHTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                            mozilla::dom::FromParser aFromParser)
    : nsGenericHTMLElement(aNodeInfo)
  {
    mNetworkCreated = aFromParser == mozilla::dom::FROM_PARSER_NETWORK;
  }
  virtual ~nsGenericHTMLFrameElement();

  NS_DECL_DOM_MEMORY_REPORTER_SIZEOF

  
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);

  
  NS_DECL_NSIFRAMELOADEROWNER

  
  virtual bool IsHTMLFocusable(bool aWithMouse, bool *aIsFocusable, PRInt32 *aTabIndex);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);
  virtual void UnbindFromTree(bool aDeep = true,
                              bool aNullParent = true);
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, bool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           bool aNotify);
  virtual void DestroyContent();

  nsresult CopyInnerTo(nsGenericElement* aDest) const;

  
  NS_IMETHOD GetTabIndex(PRInt32 *aTabIndex);
  NS_IMETHOD SetTabIndex(PRInt32 aTabIndex);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsGenericHTMLFrameElement,
                                                     nsGenericHTMLElement)

protected:
  
  
  nsresult EnsureFrameLoader();
  nsresult LoadSrc();
  nsresult GetContentDocument(nsIDOMDocument** aContentDocument);
  nsresult GetContentWindow(nsIDOMWindow** aContentWindow);

  nsRefPtr<nsFrameLoader> mFrameLoader;
  
  
  
  bool                    mNetworkCreated;
};








#define NS_IMPL_STRING_ATTR(_class, _method, _atom)                  \
  NS_IMETHODIMP                                                      \
  _class::Get##_method(nsAString& aValue)                            \
  {                                                                  \
    return GetAttrHelper(nsGkAtoms::_atom, aValue);                  \
  }                                                                  \
  NS_IMETHODIMP                                                      \
  _class::Set##_method(const nsAString& aValue)                      \
  {                                                                  \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                  \
  }





#define NS_IMPL_STRING_ATTR_WITH_FALLBACK(_class, _method, _atom, _fallback) \
  NS_IMETHODIMP                                                              \
  _class::Get##_method(nsAString& aValue)                                    \
  {                                                                          \
    if (!GetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aValue)) {             \
      _fallback(aValue);                                                     \
    }                                                                        \
    return NS_OK;                                                            \
  }                                                                          \
  NS_IMETHODIMP                                                              \
  _class::Set##_method(const nsAString& aValue)                              \
  {                                                                          \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                          \
  }






#define NS_IMPL_BOOL_ATTR(_class, _method, _atom)                     \
  NS_IMETHODIMP                                                       \
  _class::Get##_method(bool* aValue)                                \
  {                                                                   \
    return GetBoolAttr(nsGkAtoms::_atom, aValue);                   \
  }                                                                   \
  NS_IMETHODIMP                                                       \
  _class::Set##_method(bool aValue)                                 \
  {                                                                   \
    return SetBoolAttr(nsGkAtoms::_atom, aValue);                   \
  }






#define NS_IMPL_INT_ATTR(_class, _method, _atom)                    \
  NS_IMPL_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, 0)

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






#define NS_IMPL_UINT_ATTR(_class, _method, _atom)                         \
  NS_IMPL_UINT_ATTR_DEFAULT_VALUE(_class, _method, _atom, 0)

#define NS_IMPL_UINT_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(PRUint32* aValue)                                  \
  {                                                                       \
    return GetUnsignedIntAttr(nsGkAtoms::_atom, _default, aValue);        \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(PRUint32 aValue)                                   \
  {                                                                       \
    return SetUnsignedIntAttr(nsGkAtoms::_atom, aValue);                  \
  }







#define NS_IMPL_UINT_ATTR_NON_ZERO(_class, _method, _atom)                \
  NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(_class, _method, _atom, 1)

#define NS_IMPL_UINT_ATTR_NON_ZERO_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(PRUint32* aValue)                                  \
  {                                                                       \
    return GetUnsignedIntAttr(nsGkAtoms::_atom, _default, aValue);        \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(PRUint32 aValue)                                   \
  {                                                                       \
    if (aValue == 0) {                                                    \
      return NS_ERROR_DOM_INDEX_SIZE_ERR;                                 \
    }                                                                     \
    return SetUnsignedIntAttr(nsGkAtoms::_atom, aValue);                  \
  }






#define NS_IMPL_DOUBLE_ATTR(_class, _method, _atom)                    \
  NS_IMPL_DOUBLE_ATTR_DEFAULT_VALUE(_class, _method, _atom, 0.0)

#define NS_IMPL_DOUBLE_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                             \
  _class::Get##_method(double* aValue)                                      \
  {                                                                         \
    return GetDoubleAttr(nsGkAtoms::_atom, _default, aValue);               \
  }                                                                         \
  NS_IMETHODIMP                                                             \
  _class::Set##_method(double aValue)                                       \
  {                                                                         \
    return SetDoubleAttr(nsGkAtoms::_atom, aValue);                         \
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






#define NS_IMPL_ACTION_ATTR(_class, _method, _atom)                 \
  NS_IMETHODIMP                                                     \
  _class::Get##_method(nsAString& aValue)                           \
  {                                                                 \
    GetAttr(kNameSpaceID_None, nsGkAtoms::_atom, aValue);           \
    if (aValue.IsEmpty()) {                                         \
      return NS_OK;                                                 \
    }                                                               \
    return GetURIAttr(nsGkAtoms::_atom, nsnull, aValue);            \
  }                                                                 \
  NS_IMETHODIMP                                                     \
  _class::Set##_method(const nsAString& aValue)                     \
  {                                                                 \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                 \
  }








#define NS_IMPL_NON_NEGATIVE_INT_ATTR(_class, _method, _atom)             \
  NS_IMPL_NON_NEGATIVE_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, -1)

#define NS_IMPL_NON_NEGATIVE_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default)  \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(PRInt32* aValue)                                   \
  {                                                                       \
    return GetIntAttr(nsGkAtoms::_atom, _default, aValue);                \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(PRInt32 aValue)                                    \
  {                                                                       \
    if (aValue < 0) {                                                     \
      return NS_ERROR_DOM_INDEX_SIZE_ERR;                                 \
    }                                                                     \
    return SetIntAttr(nsGkAtoms::_atom, aValue);                          \
  }






#define NS_IMPL_ENUM_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default) \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(nsAString& aValue)                                 \
  {                                                                       \
    return GetEnumAttr(nsGkAtoms::_atom, _default, aValue);               \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(const nsAString& aValue)                           \
  {                                                                       \
    return SetAttrHelper(nsGkAtoms::_atom, aValue);                       \
  }








#define NS_IMPL_POSITIVE_INT_ATTR(_class, _method, _atom)                 \
  NS_IMPL_POSITIVE_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, 1)

#define NS_IMPL_POSITIVE_INT_ATTR_DEFAULT_VALUE(_class, _method, _atom, _default)  \
  NS_IMETHODIMP                                                           \
  _class::Get##_method(PRInt32* aValue)                                   \
  {                                                                       \
    return GetIntAttr(nsGkAtoms::_atom, _default, aValue);                \
  }                                                                       \
  NS_IMETHODIMP                                                           \
  _class::Set##_method(PRInt32 aValue)                                    \
  {                                                                       \
    if (aValue <= 0) {                                                    \
      return NS_ERROR_DOM_INDEX_SIZE_ERR;                                 \
    }                                                                     \
    return SetIntAttr(nsGkAtoms::_atom, aValue);                          \
  }





#define NS_HTML_CONTENT_INTERFACE_TABLE_AMBIGUOUS_BEGIN(_class, _base)        \
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(_class)                            \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMNode, _base)             \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMElement, _base)          \
    NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(_class, nsIDOMHTMLElement, _base)

#define NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                         \
  NS_HTML_CONTENT_INTERFACE_TABLE_AMBIGUOUS_BEGIN(_class, nsIDOMHTMLElement)

#define NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE_AMBIGUOUS(_class, _base, \
                                                               _base_if)      \
  rv = _base::QueryInterface(aIID, aInstancePtr);                             \
  if (NS_SUCCEEDED(rv))                                                       \
    return rv;                                                                \
                                                                              \
  rv = DOMQueryInterface(static_cast<_base_if *>(this), aIID, aInstancePtr);  \
  if (NS_SUCCEEDED(rv))                                                       \
    return rv;                                                                \
                                                                              \
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE

#define NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(_class, _base)           \
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE_AMBIGUOUS(_class, _base,       \
                                                         nsIDOMHTMLElement)

#define NS_HTML_CONTENT_INTERFACE_MAP_END                                     \
  NS_ELEMENT_INTERFACE_MAP_END

#define NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(_class)                \
    NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(_class)                              \
  NS_HTML_CONTENT_INTERFACE_MAP_END

#define NS_INTERFACE_MAP_ENTRY_IF_TAG(_interface, _tag)                       \
  NS_INTERFACE_MAP_ENTRY_CONDITIONAL(_interface,                              \
                                     mNodeInfo->Equals(nsGkAtoms::_tag))


#define NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO_GETTER(_getter) \
  if (aIID.Equals(NS_GET_IID(nsIClassInfo)) ||               \
      aIID.Equals(NS_GET_IID(nsXPCClassInfo))) {             \
    foundInterface = _getter ();                             \
    if (!foundInterface) {                                   \
      *aInstancePtr = nsnull;                                \
      return NS_ERROR_OUT_OF_MEMORY;                         \
    }                                                        \
  } else

#define NS_HTML_CONTENT_INTERFACE_TABLE0(_class)                              \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE1(_class, _i1)                         \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE2(_class, _i1, _i2)                    \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE3(_class, _i1, _i2, _i3)          \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE4(_class, _i1, _i2, _i3, _i4)          \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE5(_class, _i1, _i2, _i3, _i4, _i5)     \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE6(_class, _i1, _i2, _i3, _i4, _i5,     \
                                         _i6)                                 \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE7(_class, _i1, _i2, _i3, _i4, _i5,     \
                                         _i6, _i7)                            \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE8(_class, _i1, _i2, _i3, _i4, _i5,     \
                                         _i6, _i7, _i8)                       \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE9(_class, _i1, _i2, _i3, _i4, _i5,     \
                                         _i6, _i7, _i8, _i9)                  \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i9)                                     \
  NS_OFFSET_AND_INTERFACE_TABLE_END

#define NS_HTML_CONTENT_INTERFACE_TABLE10(_class, _i1, _i2, _i3, _i4, _i5,    \
                                          _i6, _i7, _i8, _i9, _i10)           \
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(_class)                               \
    NS_INTERFACE_TABLE_ENTRY(_class, _i1)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i2)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i3)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i4)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i5)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i6)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i7)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i8)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i9)                                     \
    NS_INTERFACE_TABLE_ENTRY(_class, _i10)                                    \
  NS_OFFSET_AND_INTERFACE_TABLE_END













#define NS_FORWARD_NSIDOMHTMLELEMENT_BASIC(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetId(nsAString& aId) { \
    return _to GetId(aId); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetId(const nsAString& aId) { \
    return _to SetId(aId); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetTitle(nsAString& aTitle) { \
    return _to GetTitle(aTitle); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetTitle(const nsAString& aTitle) { \
    return _to SetTitle(aTitle); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetLang(nsAString& aLang) { \
    return _to GetLang(aLang); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetLang(const nsAString& aLang) { \
    return _to SetLang(aLang); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetDir(nsAString& aDir) { \
    return _to GetDir(aDir); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetDir(const nsAString& aDir) { \
    return _to SetDir(aDir); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetClassName(nsAString& aClassName) { \
    return _to GetClassName(aClassName); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetClassName(const nsAString& aClassName) { \
    return _to SetClassName(aClassName); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetDataset(nsIDOMDOMStringMap** aDataset) { \
    return _to GetDataset(aDataset); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetHidden(bool* aHidden) { \
    return _to GetHidden(aHidden); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetHidden(bool aHidden) { \
    return _to SetHidden(aHidden); \
  } \
  NS_SCRIPTABLE NS_IMETHOD Blur() { \
    return _to Blur(); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetAccessKey(nsAString& aAccessKey) { \
    return _to GetAccessKey(aAccessKey); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetAccessKey(const nsAString& aAccessKey) { \
    return _to SetAccessKey(aAccessKey); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetAccessKeyLabel(nsAString& aAccessKeyLabel) { \
    return _to GetAccessKeyLabel(aAccessKeyLabel); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetDraggable(bool aDraggable) { \
    return _to SetDraggable(aDraggable); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetContentEditable(nsAString& aContentEditable) { \
    return _to GetContentEditable(aContentEditable); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetContentEditable(const nsAString& aContentEditable) { \
    return _to SetContentEditable(aContentEditable); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetIsContentEditable(bool* aIsContentEditable) { \
    return _to GetIsContentEditable(aIsContentEditable); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetContextMenu(nsIDOMHTMLMenuElement** aContextMenu) { \
    return _to GetContextMenu(aContextMenu); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetSpellcheck(bool* aSpellcheck) { \
    return _to GetSpellcheck(aSpellcheck); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetSpellcheck(bool aSpellcheck) { \
    return _to SetSpellcheck(aSpellcheck); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetOuterHTML(nsAString& aOuterHTML) { \
    return _to GetOuterHTML(aOuterHTML); \
  } \
  NS_SCRIPTABLE NS_IMETHOD SetOuterHTML(const nsAString& aOuterHTML) { \
    return _to SetOuterHTML(aOuterHTML); \
  } \
  NS_SCRIPTABLE NS_IMETHOD InsertAdjacentHTML(const nsAString& position, const nsAString& text) { \
    return _to InsertAdjacentHTML(position, text); \
  } \
  NS_SCRIPTABLE NS_IMETHOD ScrollIntoView(bool top, PRUint8 _argc) { \
    return _to ScrollIntoView(top, _argc); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetParent(nsIDOMElement** aOffsetParent) { \
    return _to GetOffsetParent(aOffsetParent); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetTop(PRInt32* aOffsetTop) { \
    return _to GetOffsetTop(aOffsetTop); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetLeft(PRInt32* aOffsetLeft) { \
    return _to GetOffsetLeft(aOffsetLeft); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetWidth(PRInt32* aOffsetWidth) { \
    return _to GetOffsetWidth(aOffsetWidth); \
  } \
  NS_SCRIPTABLE NS_IMETHOD GetOffsetHeight(PRInt32* aOffsetHeight) { \
    return _to GetOffsetHeight(aOffsetHeight); \
  } \
  NS_SCRIPTABLE NS_IMETHOD MozRequestFullScreen() { \
    return _to MozRequestFullScreen(); \
  }




#define NS_DECLARE_NS_NEW_HTML_ELEMENT(_elementName)                       \
nsGenericHTMLElement*                                                      \
NS_NewHTML##_elementName##Element(already_AddRefed<nsINodeInfo> aNodeInfo, \
                                  mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);

#define NS_DECLARE_NS_NEW_HTML_ELEMENT_AS_SHARED(_elementName)             \
inline nsGenericHTMLElement*                                               \
NS_NewHTML##_elementName##Element(already_AddRefed<nsINodeInfo> aNodeInfo, \
                                  mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER) \
{                                                                          \
  return NS_NewHTMLSharedElement(aNodeInfo, aFromParser);                  \
}




#define NS_IMPL_NS_NEW_HTML_ELEMENT(_elementName)                            \
nsGenericHTMLElement*                                                        \
NS_NewHTML##_elementName##Element(already_AddRefed<nsINodeInfo> aNodeInfo,   \
                                  mozilla::dom::FromParser aFromParser)      \
{                                                                            \
  return new nsHTML##_elementName##Element(aNodeInfo);                       \
}

#define NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(_elementName)               \
nsGenericHTMLElement*                                                        \
NS_NewHTML##_elementName##Element(already_AddRefed<nsINodeInfo> aNodeInfo,   \
                                  mozilla::dom::FromParser aFromParser)      \
{                                                                            \
  return new nsHTML##_elementName##Element(aNodeInfo, aFromParser);          \
}



nsGenericHTMLElement*
NS_NewHTMLElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                  mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);

NS_DECLARE_NS_NEW_HTML_ELEMENT(Shared)
NS_DECLARE_NS_NEW_HTML_ELEMENT(SharedList)
NS_DECLARE_NS_NEW_HTML_ELEMENT(SharedObject)

NS_DECLARE_NS_NEW_HTML_ELEMENT(Anchor)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Area)
#if defined(MOZ_MEDIA)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Audio)
#endif
NS_DECLARE_NS_NEW_HTML_ELEMENT(BR)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Body)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Button)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Canvas)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Mod)
NS_DECLARE_NS_NEW_HTML_ELEMENT(DataList)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Div)
NS_DECLARE_NS_NEW_HTML_ELEMENT(FieldSet)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Font)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Form)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Frame)
NS_DECLARE_NS_NEW_HTML_ELEMENT(FrameSet)
NS_DECLARE_NS_NEW_HTML_ELEMENT(HR)
NS_DECLARE_NS_NEW_HTML_ELEMENT_AS_SHARED(Head)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Heading)
NS_DECLARE_NS_NEW_HTML_ELEMENT_AS_SHARED(Html)
NS_DECLARE_NS_NEW_HTML_ELEMENT(IFrame)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Image)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Input)
NS_DECLARE_NS_NEW_HTML_ELEMENT(LI)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Label)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Legend)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Link)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Map)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Menu)
NS_DECLARE_NS_NEW_HTML_ELEMENT(MenuItem)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Meta)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Object)
NS_DECLARE_NS_NEW_HTML_ELEMENT(OptGroup)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Option)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Output)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Paragraph)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Pre)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Progress)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Script)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Select)
#if defined(MOZ_MEDIA)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Source)
#endif
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
#if defined(MOZ_MEDIA)
NS_DECLARE_NS_NEW_HTML_ELEMENT(Video)
#endif

#endif 
