






































#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMNSHTMLOptionElement.h"
#include "nsIOptionElement.h"
#include "nsIDOMHTMLOptGroupElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIFormControl.h"
#include "nsIForm.h"
#include "nsIDOMText.h"
#include "nsIDOMNode.h"
#include "nsGenericElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIJSNativeInitializer.h"
#include "nsISelectElement.h"
#include "nsISelectControlFrame.h"


#include "nsIFormControlFrame.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsNodeInfoManager.h"
#include "nsCOMPtr.h"
#include "nsIEventStateManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsContentCreatorFunctions.h"
#include "mozAutoDocUpdate.h"




class nsHTMLOptionElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLOptionElement,
                            public nsIDOMNSHTMLOptionElement,
                            public nsIJSNativeInitializer,
                            public nsIOptionElement
{
public:
  using nsGenericElement::GetText;
  using nsGenericElement::SetText;

  nsHTMLOptionElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLOptionElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLOPTIONELEMENT

  
  NS_IMETHOD SetText(const nsAString & aText); 

  
  NS_IMETHOD Initialize(nsISupports* aOwner, JSContext* aContext,
                        JSObject *aObj, PRUint32 argc, jsval *argv);

  virtual nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute,
                                              PRInt32 aModType) const;

  virtual nsresult BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                 const nsAString* aValue, PRBool aNotify);
  
  
  NS_IMETHOD SetSelectedInternal(PRBool aValue, PRBool aNotify);

  
  virtual PRInt32 IntrinsicState() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  





  nsIContent* GetSelect();

  PRPackedBool mSelectedChanged;
  PRPackedBool mIsSelected;

  
  
  PRPackedBool mIsInSetDefaultSelected;
};

nsGenericHTMLElement*
NS_NewHTMLOptionElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  




  nsCOMPtr<nsINodeInfo> nodeInfo(aNodeInfo);
  if (!nodeInfo) {
    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(nsContentUtils::GetDocumentFromCaller());
    NS_ENSURE_TRUE(doc, nsnull);

    nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::option, nsnull,
                                                   kNameSpaceID_XHTML);
    NS_ENSURE_TRUE(nodeInfo, nsnull);
  }

  return new nsHTMLOptionElement(nodeInfo);
}

nsHTMLOptionElement::nsHTMLOptionElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mSelectedChanged(PR_FALSE),
    mIsSelected(PR_FALSE),
    mIsInSetDefaultSelected(PR_FALSE)
{
}

nsHTMLOptionElement::~nsHTMLOptionElement()
{
}




NS_IMPL_ADDREF_INHERITED(nsHTMLOptionElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLOptionElement, nsGenericElement)



NS_INTERFACE_TABLE_HEAD(nsHTMLOptionElement)
  NS_HTML_CONTENT_INTERFACE_TABLE4(nsHTMLOptionElement,
                                   nsIDOMHTMLOptionElement,
                                   nsIDOMNSHTMLOptionElement,
                                   nsIJSNativeInitializer,
                                   nsIOptionElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLOptionElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLOptionElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLOptionElement)


NS_IMETHODIMP
nsHTMLOptionElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  NS_ENSURE_ARG_POINTER(aForm);
  *aForm = nsnull;

  nsCOMPtr<nsIFormControl> selectControl = do_QueryInterface(GetSelect());

  if (selectControl) {
    selectControl->GetForm(aForm);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionElement::SetSelectedInternal(PRBool aValue, PRBool aNotify)
{
  mSelectedChanged = PR_TRUE;
  mIsSelected = aValue;

  
  
  if (aNotify && !mIsInSetDefaultSelected) {
    nsIDocument* document = GetCurrentDoc();
    if (document) {
      mozAutoDocUpdate upd(document, UPDATE_CONTENT_STATE, aNotify);
      document->ContentStatesChanged(this, nsnull, NS_EVENT_STATE_CHECKED);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionElement::SetValue(const nsAString& aValue)
{
  SetAttr(kNameSpaceID_None, nsGkAtoms::value, aValue, PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionElement::GetValue(nsAString& aValue)
{
  
  
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::value, aValue)) {
    GetText(aValue);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLOptionElement::GetSelected(PRBool* aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  *aValue = PR_FALSE;

  
  if (!mSelectedChanged) {
    return GetDefaultSelected(aValue);
  }

  *aValue = mIsSelected;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionElement::SetSelected(PRBool aValue)
{
  
  
  nsCOMPtr<nsISelectElement> selectInt = do_QueryInterface(GetSelect());
  if (selectInt) {
    PRInt32 index;
    GetIndex(&index);
    
    return selectInt->SetOptionsSelectedByIndex(index, index, aValue,
                                                PR_FALSE, PR_TRUE, PR_TRUE,
                                                nsnull);
  } else {
    return SetSelectedInternal(aValue, PR_TRUE);
  }

  return NS_OK;
}

NS_IMPL_BOOL_ATTR(nsHTMLOptionElement, DefaultSelected, selected)
NS_IMPL_STRING_ATTR(nsHTMLOptionElement, Label, label)

NS_IMPL_BOOL_ATTR(nsHTMLOptionElement, Disabled, disabled)

NS_IMETHODIMP 
nsHTMLOptionElement::GetIndex(PRInt32* aIndex)
{
  NS_ENSURE_ARG_POINTER(aIndex);

  *aIndex = -1; 

  
  nsCOMPtr<nsIDOMHTMLSelectElement> selectElement =
    do_QueryInterface(GetSelect());

  if (selectElement) {
    
    nsCOMPtr<nsIDOMHTMLOptionsCollection> options;
    selectElement->GetOptions(getter_AddRefs(options));

    if (options) {
      
      PRUint32 length = 0;
      options->GetLength(&length);

      nsCOMPtr<nsIDOMNode> thisOption;

      for (PRUint32 i = 0; i < length; i++) {
        options->Item(i, getter_AddRefs(thisOption));

        if (thisOption.get() == static_cast<nsIDOMNode *>(this)) {
          *aIndex = i;

          break;
        }
      }
    }
  }

  return NS_OK;
}

nsChangeHint
nsHTMLOptionElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            PRInt32 aModType) const
{
  nsChangeHint retval =
      nsGenericHTMLElement::GetAttributeChangeHint(aAttribute, aModType);

  if (aAttribute == nsGkAtoms::label ||
      aAttribute == nsGkAtoms::text) {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  }
  return retval;
}

nsresult
nsHTMLOptionElement::BeforeSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                   const nsAString* aValue, PRBool aNotify)
{
  nsresult rv = nsGenericHTMLElement::BeforeSetAttr(aNamespaceID, aName,
                                                    aValue, aNotify);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aNamespaceID != kNameSpaceID_None || aName != nsGkAtoms::selected ||
      mSelectedChanged) {
    return NS_OK;
  }
  
  
  
  
  nsCOMPtr<nsISelectElement> selectInt = do_QueryInterface(GetSelect());
  if (!selectInt) {
    return NS_OK;
  }

  
  
  NS_ASSERTION(!mSelectedChanged, "Shouldn't be here");
  
  PRBool newSelected = (aValue != nsnull);
  PRBool inSetDefaultSelected = mIsInSetDefaultSelected;
  mIsInSetDefaultSelected = PR_TRUE;
  
  PRInt32 index;
  GetIndex(&index);
  
  
  
  rv = selectInt->SetOptionsSelectedByIndex(index, index, newSelected,
                                            PR_FALSE, PR_TRUE, aNotify,
                                            nsnull);

  
  
  mIsInSetDefaultSelected = inSetDefaultSelected;
  mSelectedChanged = PR_FALSE;
  

  return rv;
}

NS_IMETHODIMP
nsHTMLOptionElement::GetText(nsAString& aText)
{
  nsAutoString text;
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, text);

  
  text.CompressWhitespace(PR_TRUE, PR_TRUE);
  aText = text;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOptionElement::SetText(const nsAString& aText)
{
  return nsContentUtils::SetNodeTextContent(this, aText, PR_TRUE);
}

PRInt32
nsHTMLOptionElement::IntrinsicState() const
{
  PRInt32 state = nsGenericHTMLElement::IntrinsicState();
  
  
  
  PRBool selected;
  const_cast<nsHTMLOptionElement*>(this)->GetSelected(&selected);
  if (selected) {
    state |= NS_EVENT_STATE_CHECKED;
  }

  
  const_cast<nsHTMLOptionElement*>(this)->GetDefaultSelected(&selected);
  if (selected) {
    state |= NS_EVENT_STATE_DEFAULT;
  }

  PRBool disabled;
  GetBoolAttr(nsGkAtoms::disabled, &disabled);
  if (disabled) {
    state |= NS_EVENT_STATE_DISABLED;
    state &= ~NS_EVENT_STATE_ENABLED;
  } else {
    state &= ~NS_EVENT_STATE_DISABLED;
    state |= NS_EVENT_STATE_ENABLED;
  }

  return state;
}


nsIContent*
nsHTMLOptionElement::GetSelect()
{
  nsIContent* parent = this;
  while ((parent = parent->GetParent()) &&
         parent->IsHTML()) {
    if (parent->Tag() == nsGkAtoms::select) {
      return parent;
    }
    if (parent->Tag() != nsGkAtoms::optgroup) {
      break;
    }
  }
  
  return nsnull;
}

NS_IMETHODIMP    
nsHTMLOptionElement::Initialize(nsISupports* aOwner,
                                JSContext* aContext,
                                JSObject *aObj,
                                PRUint32 argc, 
                                jsval *argv)
{
  nsresult result = NS_OK;

  if (argc > 0) {
    
    JSString* jsstr = JS_ValueToString(aContext, argv[0]);
    if (!jsstr) {
      return NS_ERROR_FAILURE;
    }

    
    nsCOMPtr<nsIContent> textContent;
    result = NS_NewTextNode(getter_AddRefs(textContent),
                            mNodeInfo->NodeInfoManager());
    if (NS_FAILED(result)) {
      return result;
    }

    textContent->SetText(reinterpret_cast<const PRUnichar*>
                                         (JS_GetStringChars(jsstr)),
                         JS_GetStringLength(jsstr),
                         PR_FALSE);
    
    result = AppendChildTo(textContent, PR_FALSE);
    if (NS_FAILED(result)) {
      return result;
    }

    if (argc > 1) {
      
      jsstr = JS_ValueToString(aContext, argv[1]);
      if (!jsstr) {
        return NS_ERROR_FAILURE;
      }

      
      nsAutoString value(reinterpret_cast<const PRUnichar*>
                                         (JS_GetStringChars(jsstr)));

      result = SetAttr(kNameSpaceID_None, nsGkAtoms::value, value,
                       PR_FALSE);
      if (NS_FAILED(result)) {
        return result;
      }

      if (argc > 2) {
        
        JSBool defaultSelected;
        JS_ValueToBoolean(aContext, argv[2], &defaultSelected);
        if (defaultSelected) {
          result = SetAttr(kNameSpaceID_None, nsGkAtoms::selected,
                           EmptyString(), PR_FALSE);
          NS_ENSURE_SUCCESS(result, result);
        }

        
        if (argc > 3) {
          JSBool selected;
          JS_ValueToBoolean(aContext, argv[3], &selected);

          return SetSelected(selected);
        }
      }
    }
  }

  return result;
}
