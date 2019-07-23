




































#include "nsCOMPtr.h"
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMNSHTMLInputElement.h"
#include "nsITextControlElement.h"
#include "nsIFileControlElement.h"
#include "nsIDOMNSEditableElement.h"
#include "nsIRadioControlElement.h"
#include "nsIRadioVisitor.h"
#include "nsIPhonetic.h"

#include "nsIControllers.h"
#include "nsIFocusController.h"
#include "nsPIDOMWindow.h"
#include "nsContentCID.h"
#include "nsIComponentManager.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIFormSubmission.h"
#include "nsITextControlFrame.h"
#include "nsIRadioControlFrame.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIFormControlFrame.h"
#include "nsIFrame.h"
#include "nsIEventStateManager.h"
#include "nsIServiceManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsDOMError.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIEditor.h"
#include "nsGUIEvent.h"

#include "nsPresState.h"
#include "nsLayoutErrors.h"
#include "nsIDOMEvent.h"
#include "nsIDOMNSEvent.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLCollection.h"
#include "nsICheckboxControlFrame.h"
#include "nsLinebreakConverter.h" 
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsEventDispatcher.h"
#include "nsLayoutUtils.h"

#include "nsIDOMMutationEvent.h"
#include "nsIDOMEventTarget.h"
#include "nsMutationEvent.h"
#include "nsIEventListenerManager.h"

#include "nsRuleData.h"


#include "nsIRadioControlFrame.h"
#include "nsIRadioGroupContainer.h"


#include "nsIMIMEService.h"
#include "nsCExternalHandlerService.h"
#include "nsIFile.h"
#include "nsILocalFile.h"
#include "nsIFileStreams.h"
#include "nsNetUtil.h"


#include "nsImageLoadingContent.h"
#include "nsIDOMWindowInternal.h"



static NS_DEFINE_CID(kXULControllersCID,  NS_XULCONTROLLERS_CID);



#define BF_DISABLED_CHANGED 0
#define BF_HANDLING_CLICK 1
#define BF_VALUE_CHANGED 2
#define BF_CHECKED_CHANGED 3
#define BF_CHECKED 4
#define BF_HANDLING_SELECT_EVENT 5
#define BF_SHOULD_INIT_CHECKED 6
#define BF_PARSER_CREATING 7
#define BF_IN_INTERNAL_ACTIVATE 8
#define BF_CHECKED_IS_TOGGLED 9

#define GET_BOOLBIT(bitfield, field) (((bitfield) & (0x01 << (field))) \
                                        ? PR_TRUE : PR_FALSE)
#define SET_BOOLBIT(bitfield, field, b) ((b) \
                                        ? ((bitfield) |=  (0x01 << (field))) \
                                        : ((bitfield) &= ~(0x01 << (field))))


#define NS_OUTER_ACTIVATE_EVENT   (1 << 9)
#define NS_ORIGINAL_CHECKED_VALUE (1 << 10)
#define NS_NO_CONTENT_DISPATCH    (1 << 11)
#define NS_CONTROL_TYPE(bits)  ((bits) & ~( \
  NS_OUTER_ACTIVATE_EVENT | NS_ORIGINAL_CHECKED_VALUE | NS_NO_CONTENT_DISPATCH))

static const char kWhitespace[] = "\n\r\t\b";

class nsHTMLInputElement : public nsGenericHTMLFormElement,
                           public nsImageLoadingContent,
                           public nsIDOMHTMLInputElement,
                           public nsIDOMNSHTMLInputElement,
                           public nsITextControlElement,
                           public nsIRadioControlElement,
                           public nsIPhonetic,
                           public nsIDOMNSEditableElement,
                           public nsIFileControlElement
{
public:
  nsHTMLInputElement(nsINodeInfo *aNodeInfo, PRBool aFromParser);
  virtual ~nsHTMLInputElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLINPUTELEMENT

  
  NS_DECL_NSIDOMNSHTMLINPUTELEMENT

  
  NS_DECL_NSIPHONETIC

  
  NS_FORWARD_NSIDOMNSEDITABLEELEMENT(nsGenericHTMLElement::)

  
  NS_IMETHOD_(PRInt32) GetType() const { return mType; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);
  NS_IMETHOD SaveState();
  virtual PRBool RestoreState(nsPresState* aState);
  virtual PRBool AllowDrop();

  
  virtual void SetFocus(nsPresContext* aPresContext);
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);

  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);
  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);
  
  virtual void DoneCreatingElement();

  virtual PRInt32 IntrinsicState() const;

  
  NS_IMETHOD TakeTextFrameValue(const nsAString& aValue);
  NS_IMETHOD SetValueChanged(PRBool aValueChanged);
  
  
  virtual void GetFileName(nsAString& aFileName);
  virtual void SetFileName(const nsAString& aFileName);

  
  NS_IMETHOD RadioSetChecked(PRBool aNotify);
  NS_IMETHOD SetCheckedChanged(PRBool aCheckedChanged);
  NS_IMETHOD SetCheckedChangedInternal(PRBool aCheckedChanged);
  NS_IMETHOD GetCheckedChanged(PRBool* aCheckedChanged);
  NS_IMETHOD AddedToRadioGroup(PRBool aNotify = PR_TRUE);
  NS_IMETHOD WillRemoveFromRadioGroup();
  



  virtual already_AddRefed<nsIRadioGroupContainer> GetRadioGroupContainer();

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual void UpdateEditableState()
  {
    return UpdateEditableFormControlState();
  }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLInputElement,
                                                     nsGenericHTMLFormElement)

protected:
  
  nsresult SetValueInternal(const nsAString& aValue,
                            nsITextControlFrame* aFrame);

  nsresult GetSelectionRange(PRInt32* aSelectionStart, PRInt32* aSelectionEnd);

  




  PRBool GetNameIfExists(nsAString& aName) {
    return GetAttr(kNameSpaceID_None, nsGkAtoms::name, aName);
  }

  


  virtual nsresult BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);
  


  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);

  void SelectAll(nsPresContext* aPresContext);
  PRBool IsImage() const
  {
    return AttrValueIs(kNameSpaceID_None, nsGkAtoms::type,
                       nsGkAtoms::image, eIgnoreCase);
  }

  


  void FireOnChange();

  



  nsresult VisitGroup(nsIRadioVisitor* aVisitor, PRBool aFlushContent);

  



  nsresult DoSetChecked(PRBool aValue, PRBool aNotify = PR_TRUE);

  




  nsresult DoSetCheckedChanged(PRBool aCheckedChanged, PRBool aNotify);

  



  nsresult SetCheckedInternal(PRBool aValue, PRBool aNotify);

  



  nsresult MaybeSubmitForm(nsPresContext* aPresContext);
  
  nsCOMPtr<nsIControllers> mControllers;

  



  PRInt8                   mType;
  



  PRInt16                  mBitField;
  


  char*                    mValue;
  









  nsAutoPtr<nsString>      mFileName;
};

#ifdef ACCESSIBILITY

static nsresult FireEventForAccessibility(nsIDOMHTMLInputElement* aTarget,
                                          nsPresContext* aPresContext,
                                          const nsAString& aEventType);
#endif





NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Input)

nsHTMLInputElement::nsHTMLInputElement(nsINodeInfo *aNodeInfo,
                                       PRBool aFromParser)
  : nsGenericHTMLFormElement(aNodeInfo),
    mType(NS_FORM_INPUT_TEXT), 
    mBitField(0),
    mValue(nsnull)
{
  SET_BOOLBIT(mBitField, BF_PARSER_CREATING, aFromParser);
}

nsHTMLInputElement::~nsHTMLInputElement()
{
  DestroyImageLoadingContent();
  if (mValue) {
    nsMemory::Free(mValue);
  }
}




NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLInputElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLInputElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mControllers)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLInputElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLInputElement, nsGenericElement) 



NS_HTML_CONTENT_CC_INTERFACE_MAP_BEGIN(nsHTMLInputElement,
                                       nsGenericHTMLFormElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLInputElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLInputElement)
  NS_INTERFACE_MAP_ENTRY(nsITextControlElement)
  NS_INTERFACE_MAP_ENTRY(nsIFileControlElement)
  NS_INTERFACE_MAP_ENTRY(nsIRadioControlElement)
  NS_INTERFACE_MAP_ENTRY(nsIPhonetic)
  NS_INTERFACE_MAP_ENTRY(imgIDecoderObserver)
  NS_INTERFACE_MAP_ENTRY(nsIImageLoadingContent)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSEditableElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLInputElement)
NS_HTML_CONTENT_INTERFACE_MAP_END




nsresult
nsHTMLInputElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsHTMLInputElement *it = new nsHTMLInputElement(aNodeInfo, PR_FALSE);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (mType) {
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_PASSWORD:
      if (GET_BOOLBIT(mBitField, BF_VALUE_CHANGED)) {
        
        
        
        nsAutoString value;
        NS_CONST_CAST(nsHTMLInputElement*, this)->GetValue(value);
        
        it->SetValueInternal(value, nsnull);
      }
      break;
    case NS_FORM_INPUT_FILE:
      if (mFileName) {
        it->mFileName = new nsString(*mFileName);
      }
      break;
    case NS_FORM_INPUT_RADIO:
    case NS_FORM_INPUT_CHECKBOX:
      if (GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED)) {
        
        
        
        PRBool checked;
        NS_CONST_CAST(nsHTMLInputElement*, this)->GetChecked(&checked);
        it->DoSetChecked(checked, PR_FALSE);
      }
      break;
    default:
      break;
  }

  kungFuDeathGrip.swap(*aResult);

  return NS_OK;
}

nsresult
nsHTMLInputElement::BeforeSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                  const nsAString* aValue,
                                  PRBool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    
    
    
    
    
    if ((aName == nsGkAtoms::name ||
         (aName == nsGkAtoms::type && !mForm)) &&
        mType == NS_FORM_INPUT_RADIO &&
        (mForm || !(GET_BOOLBIT(mBitField, BF_PARSER_CREATING)))) {
      WillRemoveFromRadioGroup();
    } else if (aNotify && aName == nsGkAtoms::src &&
               aValue && mType == NS_FORM_INPUT_IMAGE) {
      
      LoadImage(*aValue, PR_TRUE, aNotify);
    } else if (aNotify && aName == nsGkAtoms::disabled) {
      SET_BOOLBIT(mBitField, BF_DISABLED_CHANGED, PR_TRUE);
    }
  }

  return nsGenericHTMLFormElement::BeforeSetAttr(aNameSpaceID, aName,
                                                 aValue, aNotify);
}

nsresult
nsHTMLInputElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                 const nsAString* aValue,
                                 PRBool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    
    
    
    
    
    if ((aName == nsGkAtoms::name ||
         (aName == nsGkAtoms::type && !mForm)) &&
        mType == NS_FORM_INPUT_RADIO &&
        (mForm || !(GET_BOOLBIT(mBitField, BF_PARSER_CREATING)))) {
      AddedToRadioGroup();
    }

    
    
    
    
    
    
    
    
    
    if (aName == nsGkAtoms::value &&
        !GET_BOOLBIT(mBitField, BF_VALUE_CHANGED) &&
        (mType == NS_FORM_INPUT_TEXT ||
         mType == NS_FORM_INPUT_PASSWORD ||
         mType == NS_FORM_INPUT_FILE)) {
      Reset();
    }
    
    
    
    if (aName == nsGkAtoms::checked) {
      if (aNotify &&
          (mType == NS_FORM_INPUT_RADIO || mType == NS_FORM_INPUT_CHECKBOX)) {
        
        
        nsIDocument* document = GetCurrentDoc();
        if (document) {
          MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, aNotify);
          document->ContentStatesChanged(this, nsnull, NS_EVENT_STATE_DEFAULT);
        }
      }
      if (!GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED)) {
        
        
        if (GET_BOOLBIT(mBitField, BF_PARSER_CREATING)) {
          SET_BOOLBIT(mBitField, BF_SHOULD_INIT_CHECKED, PR_TRUE);
        } else {
          PRBool defaultChecked;
          GetDefaultChecked(&defaultChecked);
          DoSetChecked(defaultChecked);
          SetCheckedChanged(PR_FALSE);
        }
      }
    }

    if (aName == nsGkAtoms::type) {
      
      
      nsIDocument* document = GetCurrentDoc();
      MOZ_AUTO_DOC_UPDATE(document, UPDATE_CONTENT_STATE, aNotify);
      
      if (!aValue) {
        
        
        
        mType = NS_FORM_INPUT_TEXT;
      }
    
      
      
      if (mValue &&
          mType != NS_FORM_INPUT_TEXT &&
          mType != NS_FORM_INPUT_PASSWORD &&
          mType != NS_FORM_INPUT_FILE) {
        SetAttr(kNameSpaceID_None, nsGkAtoms::value,
                NS_ConvertUTF8toUTF16(mValue), PR_FALSE);
        if (mValue) {
          nsMemory::Free(mValue);
          mValue = nsnull;
        }
      }

      if (mType != NS_FORM_INPUT_IMAGE) {
        
        
        
        CancelImageRequests(aNotify);
      } else if (aNotify) {
        
        
        nsAutoString src;
        if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, src)) {
          LoadImage(src, PR_FALSE, aNotify);
        }
      }

      if (aNotify && document) {
        
        
        
        
        
        document->ContentStatesChanged(this, nsnull,
                                       NS_EVENT_STATE_CHECKED |
                                       NS_EVENT_STATE_DEFAULT |
                                       NS_EVENT_STATE_BROKEN |
                                       NS_EVENT_STATE_USERDISABLED |
                                       NS_EVENT_STATE_SUPPRESSED |
                                       NS_EVENT_STATE_LOADING);
      }
    }

    
    
    if (aNotify && aName == nsGkAtoms::readonly &&
        (mType == NS_FORM_INPUT_TEXT || mType == NS_FORM_INPUT_PASSWORD)) {
      UpdateEditableState();

      nsIDocument* document = GetCurrentDoc();
      if (document) {
        mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, PR_TRUE);
        document->ContentStatesChanged(this, nsnull,
                                       NS_EVENT_STATE_MOZ_READONLY |
                                       NS_EVENT_STATE_MOZ_READWRITE);
      }
    }
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
                                                aValue, aNotify);
}



NS_IMETHODIMP
nsHTMLInputElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}


NS_IMPL_BOOL_ATTR(nsHTMLInputElement, DefaultChecked, checked)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Accept, accept)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, AccessKey, accesskey)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Alt, alt)

NS_IMPL_BOOL_ATTR(nsHTMLInputElement, Disabled, disabled)
NS_IMPL_INT_ATTR(nsHTMLInputElement, MaxLength, maxlength)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLInputElement, ReadOnly, readonly)
NS_IMPL_URI_ATTR(nsHTMLInputElement, Src, src)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLInputElement, TabIndex, tabindex, 0)
NS_IMPL_STRING_ATTR(nsHTMLInputElement, UseMap, usemap)




NS_IMETHODIMP
nsHTMLInputElement::GetDefaultValue(nsAString& aValue)
{
  GetAttrHelper(nsGkAtoms::value, aValue);

  if (mType != NS_FORM_INPUT_HIDDEN) {
    
    aValue = nsContentUtils::TrimCharsInSet(kWhitespace, aValue);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetDefaultValue(const nsAString& aValue)
{
  return SetAttrHelper(nsGkAtoms::value, aValue);
}

NS_IMETHODIMP
nsHTMLInputElement::GetSize(PRUint32* aValue)
{
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(nsGkAtoms::size);
  if (attrVal && attrVal->Type() == nsAttrValue::eInteger) {
    *aValue = attrVal->GetIntegerValue();
  }
  else {
    *aValue = 0;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSize(PRUint32 aValue)
{
  nsAutoString val;
  val.AppendInt(aValue);

  return SetAttr(kNameSpaceID_None, nsGkAtoms::size, val, PR_TRUE);
}

NS_IMETHODIMP 
nsHTMLInputElement::GetValue(nsAString& aValue)
{
  if (mType == NS_FORM_INPUT_TEXT || mType == NS_FORM_INPUT_PASSWORD) {
    
    
    
    nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);

    PRBool frameOwnsValue = PR_FALSE;
    if (formControlFrame) {
      nsITextControlFrame* textControlFrame = nsnull;
      CallQueryInterface(formControlFrame, &textControlFrame);

      if (textControlFrame) {
        textControlFrame->OwnsValue(&frameOwnsValue);
      } else {
        
        frameOwnsValue = PR_TRUE;
      }
    }

    if (frameOwnsValue) {
      formControlFrame->GetFormProperty(nsGkAtoms::value, aValue);
    } else {
      if (!GET_BOOLBIT(mBitField, BF_VALUE_CHANGED) || !mValue) {
        GetDefaultValue(aValue);
      } else {
        CopyUTF8toUTF16(mValue, aValue);
      }
    }

    return NS_OK;
  }

  if (mType == NS_FORM_INPUT_FILE) {
    if (mFileName) {
      aValue = *mFileName;
    }
    else {
      aValue.Truncate();
    }
    
    return NS_OK;
  }

  
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::value, aValue) &&
      (mType == NS_FORM_INPUT_RADIO || mType == NS_FORM_INPUT_CHECKBOX)) {
    
    aValue.AssignLiteral("on");
  }

  if (mType != NS_FORM_INPUT_HIDDEN) {
    aValue = nsContentUtils::TrimCharsInSet(kWhitespace, aValue);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::SetValue(const nsAString& aValue)
{
  
  
  if (mType == NS_FORM_INPUT_FILE) {
    if (!aValue.IsEmpty()) {
      nsIScriptSecurityManager *securityManager =
        nsContentUtils::GetSecurityManager();

      PRBool enabled;
      nsresult rv =
        securityManager->IsCapabilityEnabled("UniversalFileRead", &enabled);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!enabled) {
        
        
        return NS_ERROR_DOM_SECURITY_ERR;
      }
    }
    SetFileName(aValue);
  }
  else {
    SetValueInternal(aValue, nsnull);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::TakeTextFrameValue(const nsAString& aValue)
{
  if (mValue) {
    nsMemory::Free(mValue);
  }
  mValue = ToNewUTF8String(aValue);
  SetValueChanged(PR_TRUE);
  return NS_OK;
}

void
nsHTMLInputElement::GetFileName(nsAString& aValue)
{
  if (mFileName) {
    aValue = *mFileName;
  }
  else {
    aValue.Truncate();
  }
}

void
nsHTMLInputElement::SetFileName(const nsAString& aValue)
{
  
  mFileName = aValue.IsEmpty() ? nsnull : new nsString(aValue);

  SetValueChanged(PR_TRUE);
}

nsresult
nsHTMLInputElement::SetValueInternal(const nsAString& aValue,
                                     nsITextControlFrame* aFrame)
{
  NS_PRECONDITION(mType != NS_FORM_INPUT_FILE,
                  "Don't call SetValueInternal for file inputs");

  if (mType == NS_FORM_INPUT_TEXT || mType == NS_FORM_INPUT_PASSWORD) {

    nsITextControlFrame* textControlFrame = aFrame;
    nsIFormControlFrame* formControlFrame = textControlFrame;
    if (!textControlFrame) {
      
      
      
      formControlFrame = GetFormControlFrame(PR_FALSE);

      if (formControlFrame) {
        CallQueryInterface(formControlFrame, &textControlFrame);
      }
    }

    
    
    PRBool frameOwnsValue = PR_FALSE;
    if (textControlFrame) {
      textControlFrame->OwnsValue(&frameOwnsValue);
    }
    
    if (frameOwnsValue) {
      formControlFrame->SetFormProperty(nsGkAtoms::value, aValue);
      return NS_OK;
    }

    
    if (mValue) {
      nsMemory::Free(mValue);
    }

    mValue = ToNewUTF8String(aValue);

    SetValueChanged(PR_TRUE);
    return mValue ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  }

  if (mType == NS_FORM_INPUT_FILE) {
    return NS_ERROR_UNEXPECTED;
  }

  
  
  
  
  
  if (mType == NS_FORM_INPUT_HIDDEN) {
    SetValueChanged(PR_TRUE);
  }

  
  return nsGenericHTMLFormElement::SetAttr(kNameSpaceID_None,
                                           nsGkAtoms::value, aValue,
                                           PR_TRUE);
}

NS_IMETHODIMP
nsHTMLInputElement::SetValueChanged(PRBool aValueChanged)
{
  SET_BOOLBIT(mBitField, BF_VALUE_CHANGED, aValueChanged);
  if (!aValueChanged) {
    if (mValue) {
      nsMemory::Free(mValue);
      mValue = nsnull;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLInputElement::GetChecked(PRBool* aChecked)
{
  *aChecked = GET_BOOLBIT(mBitField, BF_CHECKED);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetCheckedChanged(PRBool aCheckedChanged)
{
  return DoSetCheckedChanged(aCheckedChanged, PR_TRUE);
}

nsresult
nsHTMLInputElement::DoSetCheckedChanged(PRBool aCheckedChanged,
                                        PRBool aNotify)
{
  if (mType == NS_FORM_INPUT_RADIO) {
    if (GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED) != aCheckedChanged) {
      nsCOMPtr<nsIRadioVisitor> visitor;
      NS_GetRadioSetCheckedChangedVisitor(aCheckedChanged,
                                          getter_AddRefs(visitor));
      VisitGroup(visitor, aNotify);
    }
  } else {
    SetCheckedChangedInternal(aCheckedChanged);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetCheckedChangedInternal(PRBool aCheckedChanged)
{
  SET_BOOLBIT(mBitField, BF_CHECKED_CHANGED, aCheckedChanged);
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLInputElement::GetCheckedChanged(PRBool* aCheckedChanged)
{
  *aCheckedChanged = GET_BOOLBIT(mBitField, BF_CHECKED_CHANGED);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetChecked(PRBool aChecked)
{
  return DoSetChecked(aChecked);
}

nsresult
nsHTMLInputElement::DoSetChecked(PRBool aChecked, PRBool aNotify)
{
  nsresult rv = NS_OK;

  
  
  
  
  
  DoSetCheckedChanged(PR_TRUE, aNotify);

  
  
  
  
  
  PRBool checked = PR_FALSE;
  GetChecked(&checked);
  if (checked == aChecked) {
    return NS_OK;
  }

  
  
  
  if (mType == NS_FORM_INPUT_RADIO) {
    
    
    
    if (aChecked) {
      rv = RadioSetChecked(aNotify);
    } else {
      rv = SetCheckedInternal(PR_FALSE, aNotify);
      nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
      if (container) {
        nsAutoString name;
        if (GetNameIfExists(name)) {
          container->SetCurrentRadioButton(name, nsnull);
        }
      }
    }
  } else {
    rv = SetCheckedInternal(aChecked, aNotify);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::RadioSetChecked(PRBool aNotify)
{
  nsresult rv = NS_OK;

  
  
  
  nsCOMPtr<nsIDOMHTMLInputElement> currentlySelected;
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  
  nsAutoString name;
  PRBool nameExists = PR_FALSE;
  if (container) {
    nameExists = GetNameIfExists(name);
    if (nameExists) {
      container->GetCurrentRadioButton(name, getter_AddRefs(currentlySelected));
    }
  }

  
  
  
  if (currentlySelected) {
    
    
    rv = NS_STATIC_CAST(nsHTMLInputElement*,
                        NS_STATIC_CAST(nsIDOMHTMLInputElement*, currentlySelected)
         )->SetCheckedInternal(PR_FALSE, PR_TRUE);
  }

  
  
  
  if (NS_SUCCEEDED(rv)) {
    rv = SetCheckedInternal(PR_TRUE, aNotify);
  }

  
  
  
  NS_ENSURE_SUCCESS(rv, rv);
  if (container && nameExists) {
    rv = container->SetCurrentRadioButton(name, this);
  }

  return rv;
}

 already_AddRefed<nsIRadioGroupContainer>
nsHTMLInputElement::GetRadioGroupContainer()
{
  nsIRadioGroupContainer* retval = nsnull;
  if (mForm) {
    CallQueryInterface(mForm, &retval);
  } else {
    nsIDocument* currentDoc = GetCurrentDoc();
    if (currentDoc) {
      CallQueryInterface(currentDoc, &retval);
    }
  }
  return retval;
}

nsresult
nsHTMLInputElement::MaybeSubmitForm(nsPresContext* aPresContext)
{
  if (!mForm) {
    
    return NS_OK;
  }
  
  nsCOMPtr<nsIPresShell> shell = aPresContext->GetPresShell();
  if (!shell) {
    return NS_OK;
  }

  
  nsIFormControl* submitControl = mForm->GetDefaultSubmitElement();
  if (submitControl) {
    nsCOMPtr<nsIContent> submitContent(do_QueryInterface(submitControl));
    NS_ASSERTION(submitContent, "Form control not implementing nsIContent?!");
    
    
    nsMouseEvent event(PR_TRUE, NS_MOUSE_CLICK, nsnull, nsMouseEvent::eReal);
    nsEventStatus status = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(submitContent, &event, &status);
  } else if (mForm->HasSingleTextControl()) {
    
    nsCOMPtr<nsIContent> form = do_QueryInterface(mForm);
    nsFormEvent event(PR_TRUE, NS_FORM_SUBMIT);
    nsEventStatus status  = nsEventStatus_eIgnore;
    shell->HandleDOMEventWithTarget(form, &event, &status);
  }

  return NS_OK;
}

nsresult
nsHTMLInputElement::SetCheckedInternal(PRBool aChecked, PRBool aNotify)
{
  
  
  
  SET_BOOLBIT(mBitField, BF_CHECKED, aChecked);

  
  
  
  nsIFrame* frame = GetPrimaryFrame();
  if (frame) {
    nsPresContext *presContext = GetPresContext();

    if (mType == NS_FORM_INPUT_CHECKBOX) {
      nsICheckboxControlFrame* checkboxFrame = nsnull;
      CallQueryInterface(frame, &checkboxFrame);
      if (checkboxFrame) {
        checkboxFrame->OnChecked(presContext, aChecked);
      }
    } else if (mType == NS_FORM_INPUT_RADIO) {
      nsIRadioControlFrame* radioFrame = nsnull;
      CallQueryInterface(frame, &radioFrame);
      if (radioFrame) {
        radioFrame->OnChecked(presContext, aChecked);
      }
    }
  }

  
  
  if (aNotify) {
    nsIDocument* document = GetCurrentDoc();
    if (document) {
      mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, aNotify);
      document->ContentStatesChanged(this, nsnull, NS_EVENT_STATE_CHECKED);
    }
  }

  return NS_OK;
}


void
nsHTMLInputElement::FireOnChange()
{
  
  
  
  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event(PR_TRUE, NS_FORM_CHANGE);
  nsCOMPtr<nsPresContext> presContext = GetPresContext();
  nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this), presContext,
                              &event, nsnull, &status);
}

NS_IMETHODIMP
nsHTMLInputElement::Blur()
{
  if (ShouldFocus(this)) {
    SetElementFocus(PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::Focus()
{
  if (ShouldFocus(this)) {
    SetElementFocus(PR_TRUE);
  }

  return NS_OK;
}

void
nsHTMLInputElement::SetFocus(nsPresContext* aPresContext)
{
  if (!aPresContext)
    return;

  
  nsIDocument* doc = GetCurrentDoc();
  if (!doc)
    return;

  
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return;
  }
 
  
  
  
  nsCOMPtr<nsPIDOMWindow> win = doc->GetWindow();
  if (win) {
    nsIFocusController *focusController = win->GetRootFocusController();
    if (focusController) {
      PRBool isActive = PR_FALSE;
      focusController->GetActive(&isActive);
      if (!isActive) {
        focusController->SetFocusedWindow(win);
        focusController->SetFocusedElement(this);

        return;
      }
    }
  }

  SetFocusAndScrollIntoView(aPresContext);
}

NS_IMETHODIMP
nsHTMLInputElement::Select()
{
  nsresult rv = NS_OK;

  nsIDocument* doc = GetCurrentDoc();
  if (!doc)
    return NS_OK;

  
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return NS_OK;
  }

  if (mType == NS_FORM_INPUT_PASSWORD || mType == NS_FORM_INPUT_TEXT) {
    
    

    nsCOMPtr<nsPresContext> presContext = GetPresContext();

    
    
    
    nsPIDOMWindow *win = doc->GetWindow();
    if (win) {
      nsIFocusController *focusController = win->GetRootFocusController();
      if (focusController) {
        PRBool isActive = PR_FALSE;
        focusController->GetActive(&isActive);
        if (!isActive) {
          focusController->SetFocusedWindow(win);
          focusController->SetFocusedElement(this);
          SelectAll(presContext);
          return NS_OK;
        }
      }
    }

    
    nsEventStatus status = nsEventStatus_eIgnore;
    
    
    if (!GET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT)) {
      nsEvent event(nsContentUtils::IsCallerChrome(), NS_FORM_SELECTED);

      SET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT, PR_TRUE);
      nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                  presContext, &event, nsnull, &status);
      SET_BOOLBIT(mBitField, BF_HANDLING_SELECT_EVENT, PR_FALSE);
    }

    
    
    if (status == nsEventStatus_eIgnore) {
      PRBool shouldFocus = ShouldFocus(this);

      if (presContext && shouldFocus) {
        nsIEventStateManager *esm = presContext->EventStateManager();
        
        
        
        
        PRInt32 currentState;
        esm->GetContentState(this, currentState);
        if (!(currentState & NS_EVENT_STATE_FOCUS) &&
            !esm->SetContentState(this, NS_EVENT_STATE_FOCUS)) {
          return rv; 
        }
      }

      nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

      if (formControlFrame) {
        if (shouldFocus) {
          formControlFrame->SetFocus(PR_TRUE, PR_TRUE);
        }

        
        SelectAll(presContext);
      }
    }
  }

  return rv;
}

void
nsHTMLInputElement::SelectAll(nsPresContext* aPresContext)
{
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    formControlFrame->SetFormProperty(nsGkAtoms::select, EmptyString());
  }
}

NS_IMETHODIMP
nsHTMLInputElement::Click()
{
  nsresult rv = NS_OK;

  if (GET_BOOLBIT(mBitField, BF_HANDLING_CLICK)) 
      return rv;                      

  
  nsAutoString disabled;
  if (HasAttr(kNameSpaceID_None, nsGkAtoms::disabled)) {
    return NS_OK;
  }

  
  
  if (mType == NS_FORM_INPUT_BUTTON   ||
      mType == NS_FORM_INPUT_CHECKBOX ||
      mType == NS_FORM_INPUT_RADIO    ||
      mType == NS_FORM_INPUT_RESET    ||
      mType == NS_FORM_INPUT_SUBMIT   ||
      mType == NS_FORM_INPUT_IMAGE) {

    
    nsCOMPtr<nsIDocument> doc = GetCurrentDoc();
    if (!doc) {
      return rv;
    }
    
    nsIPresShell *shell = doc->GetPrimaryShell();

    if (shell) {
      nsCOMPtr<nsPresContext> context = shell->GetPresContext();

      if (context) {
        
        
        
        nsMouseEvent event(nsContentUtils::IsCallerChrome(),
                           NS_MOUSE_CLICK, nsnull, nsMouseEvent::eReal);
        nsEventStatus status = nsEventStatus_eIgnore;

        SET_BOOLBIT(mBitField, BF_HANDLING_CLICK, PR_TRUE);

        nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this), context,
                                    &event, nsnull, &status);

        SET_BOOLBIT(mBitField, BF_HANDLING_CLICK, PR_FALSE);
      }
    }
  }

  return NS_OK;
}

nsresult
nsHTMLInputElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_FALSE;
  PRBool disabled;
  nsresult rv = GetDisabled(&disabled);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (disabled || !aVisitor.mPresContext) {
    return NS_OK;
  }
  
  
  
  {
    nsIFrame* frame = GetPrimaryFrame();
    if (frame) {
      const nsStyleUserInterface* uiStyle = frame->GetStyleUserInterface();

      if (uiStyle->mUserInput == NS_STYLE_USER_INPUT_NONE ||
          uiStyle->mUserInput == NS_STYLE_USER_INPUT_DISABLED) {
        return NS_OK;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  
  
  PRBool outerActivateEvent =
    (NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent) ||
     (aVisitor.mEvent->message == NS_UI_ACTIVATE &&
      !GET_BOOLBIT(mBitField, BF_IN_INTERNAL_ACTIVATE)));

  if (outerActivateEvent) {
    aVisitor.mItemFlags |= NS_OUTER_ACTIVATE_EVENT;
  }

  PRBool originalCheckedValue = PR_FALSE;

  if (outerActivateEvent) {
    SET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED, PR_FALSE);

    switch(mType) {
      case NS_FORM_INPUT_CHECKBOX:
        {
          GetChecked(&originalCheckedValue);
          DoSetChecked(!originalCheckedValue);
          SET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED, PR_TRUE);
        }
        break;

      case NS_FORM_INPUT_RADIO:
        {
          nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
          if (container) {
            nsAutoString name;
            if (GetNameIfExists(name)) {
              nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton;
              container->GetCurrentRadioButton(name,
                                               getter_AddRefs(selectedRadioButton));
              aVisitor.mItemData = selectedRadioButton;
            }
          }

          GetChecked(&originalCheckedValue);
          if (!originalCheckedValue) {
            DoSetChecked(PR_TRUE);
            SET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED, PR_TRUE);
          }
        }
        break;

      case NS_FORM_INPUT_SUBMIT:
      case NS_FORM_INPUT_IMAGE:
        if(mForm) {
          
          
          
          mForm->OnSubmitClickBegin();
        }
        break;

      default:
        break;
    } 
  }

  if (originalCheckedValue) {
    aVisitor.mItemFlags |= NS_ORIGINAL_CHECKED_VALUE;
  }

  
  
  
  if (aVisitor.mEvent->flags & NS_EVENT_FLAG_NO_CONTENT_DISPATCH) {
    aVisitor.mItemFlags |= NS_NO_CONTENT_DISPATCH;
  }
  if ((mType == NS_FORM_INPUT_TEXT || mType == NS_FORM_INPUT_PASSWORD) &&
      aVisitor.mEvent->message == NS_MOUSE_CLICK &&
      aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
      NS_STATIC_CAST(nsMouseEvent*, aVisitor.mEvent)->button ==
        nsMouseEvent::eMiddleButton) {
    aVisitor.mEvent->flags &= ~NS_EVENT_FLAG_NO_CONTENT_DISPATCH;
  }

  
  aVisitor.mItemFlags |= NS_STATIC_CAST(PRUint8, mType);

  
  if (aVisitor.mEvent->message == NS_BLUR_CONTENT) {
    nsIFrame* primaryFrame = GetPrimaryFrame();
    if (primaryFrame) {
      nsITextControlFrame* textFrame = nsnull;
      CallQueryInterface(primaryFrame, &textFrame);
      if (textFrame) {
        textFrame->CheckFireOnChange();
      }
    }
  }

  return nsGenericHTMLElement::PreHandleEvent(aVisitor);
}

nsresult
nsHTMLInputElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  if (!aVisitor.mPresContext) {
    return NS_OK;
  }
  nsresult rv = NS_OK;
  PRBool outerActivateEvent = aVisitor.mItemFlags & NS_OUTER_ACTIVATE_EVENT;
  PRBool originalCheckedValue =
    aVisitor.mItemFlags & NS_ORIGINAL_CHECKED_VALUE;
  PRBool noContentDispatch = aVisitor.mItemFlags & NS_NO_CONTENT_DISPATCH;
  PRInt8 oldType = NS_CONTROL_TYPE(aVisitor.mItemFlags);
  
  
  
  
  
  
  
  if (aVisitor.mEventStatus != nsEventStatus_eConsumeNoDefault &&
      mType != NS_FORM_INPUT_TEXT &&
      NS_IS_MOUSE_LEFT_CLICK(aVisitor.mEvent)) {
    nsUIEvent actEvent(NS_IS_TRUSTED_EVENT(aVisitor.mEvent), NS_UI_ACTIVATE, 1);

    nsCOMPtr<nsIPresShell> shell = aVisitor.mPresContext->GetPresShell();
    if (shell) {
      nsEventStatus status = nsEventStatus_eIgnore;
      SET_BOOLBIT(mBitField, BF_IN_INTERNAL_ACTIVATE, PR_TRUE);
      rv = shell->HandleDOMEventWithTarget(this, &actEvent, &status);
      SET_BOOLBIT(mBitField, BF_IN_INTERNAL_ACTIVATE, PR_FALSE);

      
      
      if (status == nsEventStatus_eConsumeNoDefault)
        aVisitor.mEventStatus = status;
    }
  }

  if (outerActivateEvent) {
    switch(oldType) {
      case NS_FORM_INPUT_SUBMIT:
      case NS_FORM_INPUT_IMAGE:
        if(mForm) {
          
          
          
          
          mForm->OnSubmitClickEnd();
        }
        break;
    } 
  }

  
  aVisitor.mEvent->flags |=
    noContentDispatch ? NS_EVENT_FLAG_NO_CONTENT_DISPATCH : NS_EVENT_FLAG_NONE;

  
  if (GET_BOOLBIT(mBitField, BF_CHECKED_IS_TOGGLED) && outerActivateEvent) {
    if (aVisitor.mEventStatus == nsEventStatus_eConsumeNoDefault) {
      
      
      
      nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton =
        do_QueryInterface(aVisitor.mItemData);
      if (selectedRadioButton) {
        selectedRadioButton->SetChecked(PR_TRUE);
        
        
        if (mType != NS_FORM_INPUT_RADIO) {
          DoSetChecked(PR_FALSE);
        }
      } else if (oldType == NS_FORM_INPUT_CHECKBOX) {
        DoSetChecked(originalCheckedValue);
      }
    } else {
      FireOnChange();
#ifdef ACCESSIBILITY
      
      if (mType == NS_FORM_INPUT_CHECKBOX) {
        FireEventForAccessibility(this, aVisitor.mPresContext,
                                  NS_LITERAL_STRING("CheckboxStateChange"));
      } else {
        FireEventForAccessibility(this, aVisitor.mPresContext,
                                  NS_LITERAL_STRING("RadioStateChange"));
        
        nsCOMPtr<nsIDOMHTMLInputElement> previous =
          do_QueryInterface(aVisitor.mItemData);
        if(previous) {
          FireEventForAccessibility(previous, aVisitor.mPresContext,
                                    NS_LITERAL_STRING("RadioStateChange"));
        }
      }
#endif
    }
  }

  if (NS_SUCCEEDED(rv)) {
    if (nsEventStatus_eIgnore == aVisitor.mEventStatus) {
      switch (aVisitor.mEvent->message) {

        case NS_FOCUS_CONTENT:
        {
          
          
          
          nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);
          if (formControlFrame && ShouldFocus(this) &&
              aVisitor.mEvent->originalTarget == NS_STATIC_CAST(nsINode*, this))
            formControlFrame->SetFocus(PR_TRUE, PR_TRUE);
        }
        break; 

        case NS_KEY_PRESS:
        case NS_KEY_UP:
        {
          
          
          nsKeyEvent * keyEvent = (nsKeyEvent *)aVisitor.mEvent;

          if ((aVisitor.mEvent->message == NS_KEY_PRESS &&
               keyEvent->keyCode == NS_VK_RETURN) ||
              (aVisitor.mEvent->message == NS_KEY_UP &&
               keyEvent->keyCode == NS_VK_SPACE)) {
            switch(mType) {
              case NS_FORM_INPUT_CHECKBOX:
              case NS_FORM_INPUT_RADIO:
              {
                
                if (keyEvent->keyCode != NS_VK_SPACE) {
                  MaybeSubmitForm(aVisitor.mPresContext);

                  break;  
                }
                
              }
              case NS_FORM_INPUT_BUTTON:
              case NS_FORM_INPUT_RESET:
              case NS_FORM_INPUT_SUBMIT:
              case NS_FORM_INPUT_IMAGE: 
              {
                nsMouseEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                                   NS_MOUSE_CLICK, nsnull, nsMouseEvent::eReal);
                nsEventStatus status = nsEventStatus_eIgnore;

                nsEventDispatcher::Dispatch(NS_STATIC_CAST(nsIContent*, this),
                                            aVisitor.mPresContext, &event,
                                            nsnull, &status);
              } 
            } 
          }
          if (aVisitor.mEvent->message == NS_KEY_PRESS &&
              mType == NS_FORM_INPUT_RADIO && !keyEvent->isAlt &&
              !keyEvent->isControl && !keyEvent->isMeta) {
            PRBool isMovingBack = PR_FALSE;
            switch (keyEvent->keyCode) {
              case NS_VK_UP: 
              case NS_VK_LEFT:
                isMovingBack = PR_TRUE;
              case NS_VK_DOWN:
              case NS_VK_RIGHT:
              
              nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
              if (container) {
                nsAutoString name;
                if (GetNameIfExists(name)) {
                  nsCOMPtr<nsIDOMHTMLInputElement> selectedRadioButton;
                  container->GetNextRadioButton(name, isMovingBack, this,
                                                getter_AddRefs(selectedRadioButton));
                  nsCOMPtr<nsIContent> radioContent =
                    do_QueryInterface(selectedRadioButton);
                  if (radioContent) {
                    rv = selectedRadioButton->Focus();
                    if (NS_SUCCEEDED(rv)) {
                      nsEventStatus status = nsEventStatus_eIgnore;
                      nsMouseEvent event(NS_IS_TRUSTED_EVENT(aVisitor.mEvent),
                                         NS_MOUSE_CLICK, nsnull,
                                         nsMouseEvent::eReal);
                      rv = nsEventDispatcher::Dispatch(radioContent,
                                                       aVisitor.mPresContext,
                                                       &event, nsnull, &status);
                      if (NS_SUCCEEDED(rv)) {
                        aVisitor.mEventStatus = nsEventStatus_eConsumeNoDefault;
                      }
                    }
                  }
                }
              }
            }
          }

          












          if (aVisitor.mEvent->message == NS_KEY_PRESS &&
              (keyEvent->keyCode == NS_VK_RETURN ||
               keyEvent->keyCode == NS_VK_ENTER) &&
              (mType == NS_FORM_INPUT_TEXT ||
               mType == NS_FORM_INPUT_PASSWORD ||
               mType == NS_FORM_INPUT_FILE)) {

            PRBool isButton = PR_FALSE;
            
            
            if (mType == NS_FORM_INPUT_FILE) {
              nsCOMPtr<nsIContent> maybeButton =
                do_QueryInterface(aVisitor.mEvent->originalTarget);
              if (maybeButton) {
                isButton = maybeButton->AttrValueIs(kNameSpaceID_None,
                                                    nsGkAtoms::type,
                                                    nsGkAtoms::button,
                                                    eCaseMatters);
              }
            }

            if (!isButton) {
              nsIFrame* primaryFrame = GetPrimaryFrame();
              if (primaryFrame) {
                nsITextControlFrame* textFrame = nsnull;
                CallQueryInterface(primaryFrame, &textFrame);
              
                
                if (textFrame) {
                  textFrame->CheckFireOnChange();
                }
              }

              rv = MaybeSubmitForm(aVisitor.mPresContext);
              NS_ENSURE_SUCCESS(rv, rv);
            }
          }

        } break; 

        case NS_MOUSE_BUTTON_DOWN:
        case NS_MOUSE_BUTTON_UP:
        case NS_MOUSE_DOUBLECLICK:
        {
          
          
          if (aVisitor.mEvent->eventStructType == NS_MOUSE_EVENT &&
              (NS_STATIC_CAST(nsMouseEvent*, aVisitor.mEvent)->button ==
                 nsMouseEvent::eMiddleButton ||
               NS_STATIC_CAST(nsMouseEvent*, aVisitor.mEvent)->button ==
                 nsMouseEvent::eRightButton)) {
            if (mType == NS_FORM_INPUT_BUTTON ||
                mType == NS_FORM_INPUT_RESET ||
                mType == NS_FORM_INPUT_SUBMIT) {
              if (aVisitor.mDOMEvent) {
                aVisitor.mDOMEvent->StopPropagation();
              } else {
                rv = NS_ERROR_FAILURE;
              }
            }

          }
          break;
        }
        default:
          break;
      }

      if (outerActivateEvent) {
        if (mForm && (oldType == NS_FORM_INPUT_SUBMIT ||
                      oldType == NS_FORM_INPUT_IMAGE)) {
          if (mType != NS_FORM_INPUT_SUBMIT && mType != NS_FORM_INPUT_IMAGE) {
            
            
            
            mForm->FlushPendingSubmission();
          }
        }
        switch(mType) {
        case NS_FORM_INPUT_RESET:
        case NS_FORM_INPUT_SUBMIT:
        case NS_FORM_INPUT_IMAGE:
          if (mForm) {
            nsFormEvent event(PR_TRUE, (mType == NS_FORM_INPUT_RESET) ?
                              NS_FORM_RESET : NS_FORM_SUBMIT);
            event.originator      = this;
            nsEventStatus status  = nsEventStatus_eIgnore;

            nsCOMPtr<nsIPresShell> presShell =
              aVisitor.mPresContext->GetPresShell();

            
            
            
            if (presShell) {
              nsCOMPtr<nsIContent> form(do_QueryInterface(mForm));
              presShell->HandleDOMEventWithTarget(form, &event, &status);
            }
          }
          break;

        default:
          break;
        } 
      } 
    } else if (outerActivateEvent &&
               (oldType == NS_FORM_INPUT_SUBMIT ||
                oldType == NS_FORM_INPUT_IMAGE) &&
               mForm) {
      
      
      
      
      mForm->FlushPendingSubmission();
    }
  } 

  return rv;
}


nsresult
nsHTMLInputElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               PRBool aCompileEventHandlers)
{
  nsresult rv = nsGenericHTMLFormElement::BindToTree(aDocument, aParent,
                                                     aBindingParent,
                                                     aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mType == NS_FORM_INPUT_IMAGE) {
    
    
    nsAutoString uri;
    if (GetAttr(kNameSpaceID_None, nsGkAtoms::src, uri)) {
      
      
      LoadImage(uri, PR_FALSE, PR_FALSE);
    }
  }

  
  
  if (aDocument && !mForm && mType == NS_FORM_INPUT_RADIO) {
    AddedToRadioGroup();
  }

  return rv;
}

void
nsHTMLInputElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  
  
  
  
  
  if (!mForm && mType == NS_FORM_INPUT_RADIO) {
    WillRemoveFromRadioGroup();
  }

  nsGenericHTMLFormElement::UnbindFromTree(aDeep, aNullParent);
}

static const nsAttrValue::EnumTable kInputTypeTable[] = {
  { "button", NS_FORM_INPUT_BUTTON },
  { "checkbox", NS_FORM_INPUT_CHECKBOX },
  { "file", NS_FORM_INPUT_FILE },
  { "hidden", NS_FORM_INPUT_HIDDEN },
  { "reset", NS_FORM_INPUT_RESET },
  { "image", NS_FORM_INPUT_IMAGE },
  { "password", NS_FORM_INPUT_PASSWORD },
  { "radio", NS_FORM_INPUT_RADIO },
  { "submit", NS_FORM_INPUT_SUBMIT },
  { "text", NS_FORM_INPUT_TEXT },
  { 0 }
};

PRBool
nsHTMLInputElement::ParseAttribute(PRInt32 aNamespaceID,
                                   nsIAtom* aAttribute,
                                   const nsAString& aValue,
                                   nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::type) {
      
      
      if (!aResult.ParseEnumValue(aValue, kInputTypeTable)) {
        mType = NS_FORM_INPUT_TEXT;
        return PR_FALSE;
      }

      
      
      
      
      
      
      PRInt32 newType = aResult.GetEnumValue();
      if (newType == NS_FORM_INPUT_FILE) {
        
        
        
        SetFileName(EmptyString());
        SetValueInternal(EmptyString(), nsnull);
      }

      mType = newType;

      return PR_TRUE;
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::maxlength) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::size) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::border) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseAlignValue(aValue, aResult);
    }
    if (ParseImageAttribute(aAttribute, aValue, aResult)) {
      
      
      
      
      return PR_TRUE;
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

NS_IMETHODIMP
nsHTMLInputElement::GetType(nsAString& aValue)
{
  const nsAttrValue::EnumTable *table = kInputTypeTable;

  while (table->tag) {
    if (mType == table->value) {
      CopyUTF8toUTF16(table->tag, aValue);

      return NS_OK;
    }

    ++table;
  }

  NS_ERROR("Shouldn't get here!");

  aValue.Truncate();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::SetType(const nsAString& aValue)
{
  return SetAttrHelper(nsGkAtoms::type, aValue);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::type);
  if (value && value->Type() == nsAttrValue::eEnum &&
      value->GetEnumValue() == NS_FORM_INPUT_IMAGE) {
    nsGenericHTMLFormElement::MapImageBorderAttributeInto(aAttributes, aData);
    nsGenericHTMLFormElement::MapImageMarginAttributeInto(aAttributes, aData);
    nsGenericHTMLFormElement::MapImageSizeAttributesInto(aAttributes, aData);
    
    nsGenericHTMLFormElement::MapImageAlignAttributeInto(aAttributes, aData);
  } 

  nsGenericHTMLFormElement::MapCommonAttributesInto(aAttributes, aData);
}

nsChangeHint
nsHTMLInputElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                           PRInt32 aModType) const
{
  nsChangeHint retval =
    nsGenericHTMLFormElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::type) {
    NS_UpdateHint(retval, NS_STYLE_HINT_FRAMECHANGE);
  } else if (aAttribute == nsGkAtoms::value) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  } else if (aAttribute == nsGkAtoms::size &&
             (mType == NS_FORM_INPUT_TEXT ||
              mType == NS_FORM_INPUT_PASSWORD)) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  }
  return retval;
}

NS_IMETHODIMP_(PRBool)
nsHTMLInputElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align },
    { &nsGkAtoms::type },
    { nsnull },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sImageMarginSizeAttributeMap,
    sImageBorderAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

nsMapRuleToAttributesFunc
nsHTMLInputElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}




NS_IMETHODIMP
nsHTMLInputElement::GetControllers(nsIControllers** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  
  if (mType == NS_FORM_INPUT_TEXT || mType == NS_FORM_INPUT_PASSWORD)
  {
    if (!mControllers)
    {
      nsresult rv;
      mControllers = do_CreateInstance(kXULControllersCID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIController>
        controller(do_CreateInstance("@mozilla.org/editor/editorcontroller;1",
                                     &rv));
      NS_ENSURE_SUCCESS(rv, rv);

      mControllers->AppendController(controller);
    }
  }

  *aResult = mControllers;
  NS_IF_ADDREF(*aResult);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::GetTextLength(PRInt32* aTextLength)
{
  nsAutoString val;

  nsresult rv = GetValue(val);

  *aTextLength = val.Length();

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionRange(PRInt32 aSelectionStart,
                                      PRInt32 aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = nsnull;
    CallQueryInterface(formControlFrame, &textControlFrame);

    if (textControlFrame)
      rv = textControlFrame->SetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionStart(PRInt32* aSelectionStart)
{
  NS_ENSURE_ARG_POINTER(aSelectionStart);
  
  PRInt32 selEnd;
  return GetSelectionRange(aSelectionStart, &selEnd);
}

NS_IMETHODIMP
nsHTMLInputElement::SetSelectionStart(PRInt32 aSelectionStart)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = nsnull;
    CallQueryInterface(formControlFrame, &textControlFrame);

    if (textControlFrame)
      rv = textControlFrame->SetSelectionStart(aSelectionStart);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::GetSelectionEnd(PRInt32* aSelectionEnd)
{
  NS_ENSURE_ARG_POINTER(aSelectionEnd);
  
  PRInt32 selStart;
  return GetSelectionRange(&selStart, aSelectionEnd);
}


NS_IMETHODIMP
nsHTMLInputElement::SetSelectionEnd(PRInt32 aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = nsnull;
    CallQueryInterface(formControlFrame, &textControlFrame);

    if (textControlFrame)
      rv = textControlFrame->SetSelectionEnd(aSelectionEnd);
  }

  return rv;
}

nsresult
nsHTMLInputElement::GetSelectionRange(PRInt32* aSelectionStart,
                                      PRInt32* aSelectionEnd)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsITextControlFrame* textControlFrame = nsnull;
    CallQueryInterface(formControlFrame, &textControlFrame);

    if (textControlFrame)
      rv = textControlFrame->GetSelectionRange(aSelectionStart, aSelectionEnd);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::GetPhonetic(nsAString& aPhonetic)
{
  aPhonetic.Truncate(0);
  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_TRUE);

  if (formControlFrame) {
    nsCOMPtr<nsIPhonetic>
      phonetic(do_QueryInterface(formControlFrame));

    if (phonetic)
      phonetic->GetPhonetic(aPhonetic);
  }

  return NS_OK;
}

#ifdef ACCESSIBILITY
 nsresult
FireEventForAccessibility(nsIDOMHTMLInputElement* aTarget,
                          nsPresContext* aPresContext,
                          const nsAString& aEventType)
{
  nsCOMPtr<nsIDOMEvent> event;
  if (NS_SUCCEEDED(nsEventDispatcher::CreateEvent(aPresContext, nsnull,
                                                  NS_LITERAL_STRING("Events"),
                                                  getter_AddRefs(event)))) {
    event->InitEvent(aEventType, PR_TRUE, PR_TRUE);

    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
    if (privateEvent) {
      privateEvent->SetTrusted(PR_TRUE);
    }

    nsEventDispatcher::DispatchDOMEvent(aTarget, nsnull, event, aPresContext, nsnull);
  }

  return NS_OK;
}
#endif

nsresult
nsHTMLInputElement::Reset()
{
  nsresult rv = NS_OK;

  nsIFormControlFrame* formControlFrame = GetFormControlFrame(PR_FALSE);

  switch (mType) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
    {
      PRBool resetVal;
      GetDefaultChecked(&resetVal);
      rv = DoSetChecked(resetVal);
      SetCheckedChanged(PR_FALSE);
      break;
    }
    case NS_FORM_INPUT_PASSWORD:
    case NS_FORM_INPUT_TEXT:
    {
      
      
      if (formControlFrame) {
        nsAutoString resetVal;
        GetDefaultValue(resetVal);
        rv = SetValue(resetVal);
      }
      SetValueChanged(PR_FALSE);
      break;
    }
    case NS_FORM_INPUT_FILE:
    {
      
      SetFileName(EmptyString());
      break;
    }
    
    case NS_FORM_INPUT_HIDDEN:
    default:
      break;
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLInputElement::SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                                      nsIContent* aSubmitElement)
{
  nsresult rv = NS_OK;

  
  
  
  PRBool disabled;
  rv = GetDisabled(&disabled);
  if (NS_FAILED(rv) || disabled) {
    return rv;
  }

  
  
  
  if (mType == NS_FORM_INPUT_RESET || mType == NS_FORM_INPUT_BUTTON) {
    return rv;
  }

  
  
  
  
  if ((mType == NS_FORM_INPUT_SUBMIT || mType == NS_FORM_INPUT_IMAGE)
      && aSubmitElement != this) {
    return rv;
  }

  
  
  
  if (mType == NS_FORM_INPUT_RADIO || mType == NS_FORM_INPUT_CHECKBOX) {
    PRBool checked;
    rv = GetChecked(&checked);
    if (NS_FAILED(rv) || !checked) {
      return rv;
    }
  }

  
  
  
  nsAutoString name;
  PRBool nameThere = GetNameIfExists(name);

  
  
  
  if (mType == NS_FORM_INPUT_IMAGE) {
    
    nsAutoString xVal;
    nsAutoString yVal;

    nsIntPoint* lastClickedPoint =
      NS_STATIC_CAST(nsIntPoint*, GetProperty(nsGkAtoms::imageClickedPoint));
    if (lastClickedPoint) {
      
      xVal.AppendInt(lastClickedPoint->x);
      yVal.AppendInt(lastClickedPoint->y);
    }

    if (!name.IsEmpty()) {
      aFormSubmission->AddNameValuePair(this,
                                        name + NS_LITERAL_STRING(".x"), xVal);
      aFormSubmission->AddNameValuePair(this,
                                        name + NS_LITERAL_STRING(".y"), yVal);
    } else {
      
      
      aFormSubmission->AddNameValuePair(this, NS_LITERAL_STRING("x"), xVal);
      aFormSubmission->AddNameValuePair(this, NS_LITERAL_STRING("y"), yVal);
    }
  }

  
  
  

  
  if (!nameThere) {
    return rv;
  }

  
  nsAutoString value;
  rv = GetValue(value);
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (mType == NS_FORM_INPUT_SUBMIT && value.IsEmpty() &&
      !HasAttr(kNameSpaceID_None, nsGkAtoms::value)) {
    
    nsXPIDLString defaultValue;
    nsContentUtils::GetLocalizedString(nsContentUtils::eFORMS_PROPERTIES,
                                       "Submit", defaultValue);
    value = defaultValue;
  }
      
  
  
  
  if (mType == NS_FORM_INPUT_FILE) {
    
    
    
    nsCOMPtr<nsIFile> file;
 
    if (mFileName) {
      if (StringBeginsWith(*mFileName, NS_LITERAL_STRING("file:"),
                           nsCaseInsensitiveStringComparator())) {
        
        
        rv = NS_GetFileFromURLSpec(NS_ConvertUTF16toUTF8(*mFileName),
                                   getter_AddRefs(file));
      }
      if (!file) {
        
        nsCOMPtr<nsILocalFile> localFile;
        rv = NS_NewLocalFile(*mFileName, PR_FALSE, getter_AddRefs(localFile));
        file = localFile;
      }
    }

    if (file) {

      
      
      
      nsAutoString filename;
      rv = file->GetLeafName(filename);

      if (NS_SUCCEEDED(rv) && !filename.IsEmpty()) {
        PRBool acceptsFiles = aFormSubmission->AcceptsFiles();

        if (acceptsFiles) {
          
          
          
          nsCOMPtr<nsIMIMEService> MIMEService =
            do_GetService(NS_MIMESERVICE_CONTRACTID, &rv);
          NS_ENSURE_SUCCESS(rv, rv);

          nsCAutoString contentType;
          rv = MIMEService->GetTypeFromFile(file, contentType);
          if (NS_FAILED(rv)) {
            contentType.AssignLiteral("application/octet-stream");
          }

          
          
          
          nsCOMPtr<nsIInputStream> fileStream;
          rv = NS_NewLocalFileInputStream(getter_AddRefs(fileStream),
                                          file, -1, -1,
                                          nsIFileInputStream::CLOSE_ON_EOF |
                                          nsIFileInputStream::REOPEN_ON_REWIND);
          if (fileStream) {
            
            
            
            nsCOMPtr<nsIInputStream> bufferedStream;
            rv = NS_NewBufferedInputStream(getter_AddRefs(bufferedStream),
                                           fileStream, 8192);
            NS_ENSURE_SUCCESS(rv, rv);
            if (bufferedStream) {
              
              
              
              aFormSubmission->AddNameFilePair(this, name, filename,
                                               bufferedStream, contentType,
                                               PR_FALSE);
              return rv;
            }
          }
        }

        
        
        
        aFormSubmission->AddNameFilePair(this, name, filename,
            nsnull, NS_LITERAL_CSTRING("application/octet-stream"),
            PR_FALSE);
        return rv;
      } else {
        
        rv = NS_OK;
      }
    }
    
    
    
    
    
    aFormSubmission->AddNameFilePair(this, name, EmptyString(),
        nsnull, NS_LITERAL_CSTRING("application/octet-stream"),
        PR_FALSE);
    return rv;
  }

  
  
  if (mType != NS_FORM_INPUT_IMAGE || !value.IsEmpty()) {
    rv = aFormSubmission->AddNameValuePair(this, name, value);
  }

  return rv;
}


NS_IMETHODIMP
nsHTMLInputElement::SaveState()
{
  nsresult rv = NS_OK;

  nsPresState *state = nsnull;
  switch (mType) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      {
        PRBool checked = PR_FALSE;
        GetChecked(&checked);
        PRBool defaultChecked = PR_FALSE;
        GetDefaultChecked(&defaultChecked);
        
        
        
        if (mType == NS_FORM_INPUT_RADIO || checked != defaultChecked) {
          rv = GetPrimaryPresState(this, &state);
          if (state) {
            if (checked) {
              rv = state->SetStateProperty(NS_LITERAL_STRING("checked"),
                                           NS_LITERAL_STRING("t"));
            } else {
              rv = state->SetStateProperty(NS_LITERAL_STRING("checked"),
                                           NS_LITERAL_STRING("f"));
            }
            NS_ASSERTION(NS_SUCCEEDED(rv), "checked save failed!");
          }
        }
        break;
      }

    
    case NS_FORM_INPUT_PASSWORD:
      break;
    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_HIDDEN:
      {
        if (GET_BOOLBIT(mBitField, BF_VALUE_CHANGED)) {
          rv = GetPrimaryPresState(this, &state);
          if (state) {
            nsAutoString value;
            GetValue(value);
            rv = nsLinebreakConverter::ConvertStringLineBreaks(
                     value,
                     nsLinebreakConverter::eLinebreakPlatform,
                     nsLinebreakConverter::eLinebreakContent);
            NS_ASSERTION(NS_SUCCEEDED(rv), "Converting linebreaks failed!");
            rv = state->SetStateProperty(NS_LITERAL_STRING("v"), value);
            NS_ASSERTION(NS_SUCCEEDED(rv), "value save failed!");
          }
        }
        break;
      }
    case NS_FORM_INPUT_FILE:
      {
        if (mFileName) {
          rv = GetPrimaryPresState(this, &state);
          if (state) {
            rv = state->SetStateProperty(NS_LITERAL_STRING("f"), *mFileName);
            NS_ASSERTION(NS_SUCCEEDED(rv), "value save failed!");
          }
        }
        break;
      }
  }
  
  if (GET_BOOLBIT(mBitField, BF_DISABLED_CHANGED)) {
    rv |= GetPrimaryPresState(this, &state);
    if (state) {
      PRBool disabled;
      GetDisabled(&disabled);
      if (disabled) {
        rv |= state->SetStateProperty(NS_LITERAL_STRING("disabled"),
                                     NS_LITERAL_STRING("t"));
      } else {
        rv |= state->SetStateProperty(NS_LITERAL_STRING("disabled"),
                                     NS_LITERAL_STRING("f"));
      }
      NS_ASSERTION(NS_SUCCEEDED(rv), "disabled save failed!");
    }
  }

  return rv;
}

void
nsHTMLInputElement::DoneCreatingElement()
{
  SET_BOOLBIT(mBitField, BF_PARSER_CREATING, PR_FALSE);

  
  
  
  
  PRBool restoredCheckedState = RestoreFormControlState(this, this);

  
  
  
  
  if (!restoredCheckedState &&
      GET_BOOLBIT(mBitField, BF_SHOULD_INIT_CHECKED)) {
    PRBool resetVal;
    GetDefaultChecked(&resetVal);
    DoSetChecked(resetVal, PR_FALSE);
    DoSetCheckedChanged(PR_FALSE, PR_FALSE);
  }

  SET_BOOLBIT(mBitField, BF_SHOULD_INIT_CHECKED, PR_FALSE);
}

PRInt32
nsHTMLInputElement::IntrinsicState() const
{
  
  
  
  PRInt32 state = nsGenericHTMLFormElement::IntrinsicState();
  if (mType == NS_FORM_INPUT_CHECKBOX || mType == NS_FORM_INPUT_RADIO) {
    
    if (GET_BOOLBIT(mBitField, BF_CHECKED)) {
      state |= NS_EVENT_STATE_CHECKED;
    }

    
    
    
    PRBool defaultState = PR_FALSE;
    NS_CONST_CAST(nsHTMLInputElement*, this)->GetDefaultChecked(&defaultState);
    if (defaultState) {
      state |= NS_EVENT_STATE_DEFAULT;
    }
  } else if (mType == NS_FORM_INPUT_IMAGE) {
    state |= nsImageLoadingContent::ImageState();
  }

  return state;
}

PRBool
nsHTMLInputElement::RestoreState(nsPresState* aState)
{
  PRBool restoredCheckedState = PR_FALSE;

  nsresult rv;
  
  switch (mType) {
    case NS_FORM_INPUT_CHECKBOX:
    case NS_FORM_INPUT_RADIO:
      {
        nsAutoString checked;
        rv = aState->GetStateProperty(NS_LITERAL_STRING("checked"), checked);
        NS_ASSERTION(NS_SUCCEEDED(rv), "checked restore failed!");
        if (rv == NS_STATE_PROPERTY_EXISTS) {
          restoredCheckedState = PR_TRUE;
          DoSetChecked(checked.EqualsLiteral("t"), PR_FALSE);
        }
        break;
      }

    case NS_FORM_INPUT_TEXT:
    case NS_FORM_INPUT_HIDDEN:
      {
        nsAutoString value;
        rv = aState->GetStateProperty(NS_LITERAL_STRING("v"), value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "value restore failed!");
        if (rv == NS_STATE_PROPERTY_EXISTS) {
          SetValueInternal(value, nsnull);
        }
        break;
      }
    case NS_FORM_INPUT_FILE:
      {
        nsAutoString value;
        rv = aState->GetStateProperty(NS_LITERAL_STRING("f"), value);
        NS_ASSERTION(NS_SUCCEEDED(rv), "value restore failed!");
        if (rv == NS_STATE_PROPERTY_EXISTS) {
          SetFileName(value);
        }
        break;
      }
  }
  
  nsAutoString disabled;
  rv = aState->GetStateProperty(NS_LITERAL_STRING("disabled"), disabled);
  NS_ASSERTION(NS_SUCCEEDED(rv), "disabled restore failed!");
  if (rv == NS_STATE_PROPERTY_EXISTS) {
    SetDisabled(disabled.EqualsLiteral("t"));
  }

  return restoredCheckedState;
}

PRBool
nsHTMLInputElement::AllowDrop()
{
  

  return mType != NS_FORM_INPUT_FILE;
}





NS_IMETHODIMP
nsHTMLInputElement::AddedToRadioGroup(PRBool aNotify)
{
  
  aNotify = aNotify && !GET_BOOLBIT(mBitField, BF_PARSER_CREATING);

  
  
  
  
  if (!mForm && !(IsInDoc() && GetParent())) {
    return NS_OK;
  }

  
  
  
  
  PRBool checked;
  GetChecked(&checked);
  if (checked) {
    
    
    
    
    
    
    
    RadioSetChecked(aNotify);
  }

  
  
  
  
  PRBool checkedChanged = PR_FALSE;
  nsCOMPtr<nsIRadioVisitor> visitor;
  nsresult rv = NS_GetRadioGetCheckedChangedVisitor(&checkedChanged, this,
                                           getter_AddRefs(visitor));
  NS_ENSURE_SUCCESS(rv, rv);
  
  VisitGroup(visitor, aNotify);
  SetCheckedChangedInternal(checkedChanged);
  
  
  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    if (GetNameIfExists(name)) {
      container->AddToRadioGroup(name, NS_STATIC_CAST(nsIFormControl*, this));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLInputElement::WillRemoveFromRadioGroup()
{
  
  
  
  
  if (!mForm && !(IsInDoc() && GetParent())) {
    return NS_OK;
  }

  
  
  
  
  PRBool checked = PR_FALSE;
  GetChecked(&checked);

  nsAutoString name;
  PRBool gotName = PR_FALSE;
  if (checked) {
    if (!gotName) {
      if (!GetNameIfExists(name)) {
        
        return NS_OK;
      }
      gotName = PR_TRUE;
    }

    nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
    if (container) {
      container->SetCurrentRadioButton(name, nsnull);
    }
  }
  
  
  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    if (!gotName) {
      if (!GetNameIfExists(name)) {
        
        return NS_OK;
      }
      gotName = PR_TRUE;
    }
    container->RemoveFromRadioGroup(name,
                                    NS_STATIC_CAST(nsIFormControl*, this));
  }

  return NS_OK;
}

PRBool
nsHTMLInputElement::IsFocusable(PRInt32 *aTabIndex)
{
  if (!nsGenericHTMLElement::IsFocusable(aTabIndex)) {
    return PR_FALSE;
  }

  if (mType == NS_FORM_INPUT_TEXT || mType == NS_FORM_INPUT_PASSWORD) {
    return PR_TRUE;
  }

  if (mType == NS_FORM_INPUT_HIDDEN || mType == NS_FORM_INPUT_FILE) {
    
    if (aTabIndex) {
      *aTabIndex = -1;
    }
    return PR_FALSE;
  }

  if (!aTabIndex) {
    
    return PR_TRUE;
  }

  
  if (mType != NS_FORM_INPUT_TEXT && mType != NS_FORM_INPUT_PASSWORD &&
      !(sTabFocusModel & eTabFocus_formElementsMask)) {
    *aTabIndex = -1;
  }

  if (mType != NS_FORM_INPUT_RADIO) {
    return PR_TRUE;
  }

  PRBool checked;
  GetChecked(&checked);
  if (checked) {
    
    return PR_TRUE;
  }

  
  
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  nsAutoString name;
  if (!container || !GetNameIfExists(name)) {
    return PR_TRUE;
  }

  nsCOMPtr<nsIDOMHTMLInputElement> currentRadio;
  container->GetCurrentRadioButton(name, getter_AddRefs(currentRadio));
  if (currentRadio) {
    *aTabIndex = -1;
  }
  return PR_TRUE;
}

nsresult
nsHTMLInputElement::VisitGroup(nsIRadioVisitor* aVisitor, PRBool aFlushContent)
{
  nsresult rv = NS_OK;
  nsCOMPtr<nsIRadioGroupContainer> container = GetRadioGroupContainer();
  if (container) {
    nsAutoString name;
    if (GetNameIfExists(name)) {
      rv = container->WalkRadioGroup(name, aVisitor, aFlushContent);
    } else {
      PRBool stop;
      aVisitor->Visit(this, &stop);
    }
  } else {
    PRBool stop;
    aVisitor->Visit(this, &stop);
  }
  return rv;
}










class nsRadioVisitor : public nsIRadioVisitor {
public:
  nsRadioVisitor() { }
  virtual ~nsRadioVisitor() { }

  NS_DECL_ISUPPORTS

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop) = 0;
};

NS_IMPL_ADDREF(nsRadioVisitor)
NS_IMPL_RELEASE(nsRadioVisitor)

NS_INTERFACE_MAP_BEGIN(nsRadioVisitor)
  NS_INTERFACE_MAP_ENTRY(nsIRadioVisitor)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END





class nsRadioSetCheckedChangedVisitor : public nsRadioVisitor {
public:
  nsRadioSetCheckedChangedVisitor(PRBool aCheckedChanged) :
    nsRadioVisitor(), mCheckedChanged(aCheckedChanged)
    { }

  virtual ~nsRadioSetCheckedChangedVisitor() { }

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop)
  {
    nsCOMPtr<nsIRadioControlElement> radio(do_QueryInterface(aRadio));
    NS_ASSERTION(radio, "Visit() passed a null button (or non-radio)!");
    radio->SetCheckedChangedInternal(mCheckedChanged);
    return NS_OK;
  }

protected:
  PRPackedBool mCheckedChanged;
};




class nsRadioGetCheckedChangedVisitor : public nsRadioVisitor {
public:
  nsRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                  nsIFormControl* aExcludeElement) :
    nsRadioVisitor(),
    mCheckedChanged(aCheckedChanged),
    mExcludeElement(aExcludeElement)
    { }

  virtual ~nsRadioGetCheckedChangedVisitor() { }

  NS_IMETHOD Visit(nsIFormControl* aRadio, PRBool* aStop)
  {
    if (aRadio == mExcludeElement) {
      return NS_OK;
    }
    nsCOMPtr<nsIRadioControlElement> radio(do_QueryInterface(aRadio));
    NS_ASSERTION(radio, "Visit() passed a null button (or non-radio)!");
    radio->GetCheckedChanged(mCheckedChanged);
    *aStop = PR_TRUE;
    return NS_OK;
  }

protected:
  PRBool* mCheckedChanged;
  nsIFormControl* mExcludeElement;
};

nsresult
NS_GetRadioSetCheckedChangedVisitor(PRBool aCheckedChanged,
                                    nsIRadioVisitor** aVisitor)
{
  
  
  
  
  
  static nsIRadioVisitor* sVisitorTrue = nsnull;
  static nsIRadioVisitor* sVisitorFalse = nsnull;

  
  
  
  if (aCheckedChanged) {
    if (!sVisitorTrue) {
      sVisitorTrue = new nsRadioSetCheckedChangedVisitor(PR_TRUE);
      if (!sVisitorTrue) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      NS_ADDREF(sVisitorTrue);
      nsresult rv =
        nsContentUtils::ReleasePtrOnShutdown((nsISupports**)&sVisitorTrue);
      if (NS_FAILED(rv)) {
        NS_RELEASE(sVisitorTrue);
        return rv;
      }
    }
    *aVisitor = sVisitorTrue;
  }
  
  
  
  else {
    if (!sVisitorFalse) {
      sVisitorFalse = new nsRadioSetCheckedChangedVisitor(PR_FALSE);
      if (!sVisitorFalse) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      NS_ADDREF(sVisitorFalse);
      nsresult rv =
        nsContentUtils::ReleasePtrOnShutdown((nsISupports**)&sVisitorFalse);
      if (NS_FAILED(rv)) {
        NS_RELEASE(sVisitorFalse);
        return rv;
      }
    }
    *aVisitor = sVisitorFalse;
  }

  NS_ADDREF(*aVisitor);
  return NS_OK;
}

nsresult
NS_GetRadioGetCheckedChangedVisitor(PRBool* aCheckedChanged,
                                    nsIFormControl* aExcludeElement,
                                    nsIRadioVisitor** aVisitor)
{
  *aVisitor = new nsRadioGetCheckedChangedVisitor(aCheckedChanged,
                                                  aExcludeElement);
  if (!*aVisitor) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aVisitor);

  return NS_OK;
}
