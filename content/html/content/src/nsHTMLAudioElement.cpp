




































#include "nsIDOMHTMLAudioElement.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsHTMLAudioElement.h"
#include "nsGenericHTMLElement.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsGkAtoms.h"
#include "nsSize.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsDOMError.h"
#include "nsNodeInfoManager.h"
#include "plbase64.h"
#include "nsNetUtil.h"
#include "prmem.h"
#include "nsNetUtil.h"
#include "nsXPCOMStrings.h"
#include "prlock.h"
#include "nsThreadUtils.h"
#include "nsJSUtils.h"

#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

#include "nsIRenderingContext.h"
#include "nsITimer.h"

#include "nsEventDispatcher.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMProgressEvent.h"
#include "nsHTMLMediaError.h"

nsGenericHTMLElement*
NS_NewHTMLAudioElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  




  nsCOMPtr<nsINodeInfo> nodeInfo(aNodeInfo);
  if (!nodeInfo) {
    nsCOMPtr<nsIDocument> doc =
      do_QueryInterface(nsContentUtils::GetDocumentFromCaller());
    NS_ENSURE_TRUE(doc, nsnull);

    nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::audio, nsnull,
                                                   kNameSpaceID_None);
    NS_ENSURE_TRUE(nodeInfo, nsnull);
  }

  return new nsHTMLAudioElement(nodeInfo);
}

NS_IMPL_ADDREF_INHERITED(nsHTMLAudioElement, nsHTMLMediaElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLAudioElement, nsHTMLMediaElement)

NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLAudioElement, nsHTMLMediaElement)
NS_INTERFACE_TABLE_INHERITED2(nsHTMLAudioElement, nsIDOMHTMLAudioElement, nsIJSNativeInitializer)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLAudioElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLAudioElement)


nsHTMLAudioElement::nsHTMLAudioElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
  : nsHTMLMediaElement(aNodeInfo, aFromParser)
{
}

nsHTMLAudioElement::~nsHTMLAudioElement()
{
}

nsresult nsHTMLAudioElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                                        nsIContent* aBindingParent,
                                        PRBool aCompileEventHandlers)
{
  if (mDecoder)
    mDecoder->ElementAvailable(this);

  return nsHTMLMediaElement::BindToTree(aDocument, 
                                        aParent, 
                                        aBindingParent, 
                                        aCompileEventHandlers);
}

void nsHTMLAudioElement::UnbindFromTree(PRBool aDeep,
                                        PRBool aNullParent)
{
  if (mDecoder) 
    mDecoder->ElementUnavailable();

  nsHTMLMediaElement::UnbindFromTree(aDeep, aNullParent);
}

nsresult nsHTMLAudioElement::InitializeDecoder(nsAString& aChosenMediaResource)
{
  if (mDecoder) 
    mDecoder->ElementAvailable(this);

  return nsHTMLMediaElement::InitializeDecoder(aChosenMediaResource);
}

NS_IMETHODIMP
nsHTMLAudioElement::Initialize(nsISupports* aOwner, JSContext* aContext,
                               JSObject *aObj, PRUint32 argc, jsval *argv)
{
  if (argc <= 0) {
    
    return NS_OK;
  }

  
  JSString* jsstr = JS_ValueToString(aContext, argv[0]);
  if (!jsstr)
    return NS_ERROR_FAILURE;

  nsDependentJSString str(jsstr);
  return SetAttr(kNameSpaceID_None, nsGkAtoms::src, str, PR_TRUE);
}
