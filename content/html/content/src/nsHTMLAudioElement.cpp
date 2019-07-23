




































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

#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

#include "nsIRenderingContext.h"
#include "nsITimer.h"

#include "nsEventDispatcher.h"
#include "nsIDOMDocumentEvent.h"
#include "nsIDOMProgressEvent.h"
#include "nsHTMLMediaError.h"

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Audio)

NS_IMPL_ADDREF_INHERITED(nsHTMLAudioElement, nsHTMLMediaElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLAudioElement, nsHTMLMediaElement)

NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLAudioElement, nsHTMLMediaElement)
  NS_INTERFACE_TABLE_INHERITED1(nsHTMLAudioElement, nsIDOMHTMLAudioElement)
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
