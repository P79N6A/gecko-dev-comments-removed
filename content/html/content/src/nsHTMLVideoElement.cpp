




































#include "nsIDOMHTMLVideoElement.h"
#include "nsIDOMHTMLSourceElement.h"
#include "nsHTMLVideoElement.h"
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

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Video)

NS_IMPL_ADDREF_INHERITED(nsHTMLVideoElement, nsHTMLMediaElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLVideoElement, nsHTMLMediaElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLVideoElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLVideoElement, nsIDOMHTMLMediaElement, nsIDOMHTMLVideoElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLVideoElement,
                                               nsHTMLMediaElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLVideoElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLVideoElement)


NS_IMPL_INT_ATTR(nsHTMLVideoElement, Width, width)
NS_IMPL_INT_ATTR(nsHTMLVideoElement, Height, height)



NS_IMETHODIMP nsHTMLVideoElement::GetVideoWidth(PRUint32 *aVideoWidth)
{
  *aVideoWidth = mMediaSize.width == -1 ? 0 : mMediaSize.width;
  return NS_OK;
}


NS_IMETHODIMP nsHTMLVideoElement::GetVideoHeight(PRUint32 *aVideoHeight)
{
  *aVideoHeight = mMediaSize.height == -1 ? 0 : mMediaSize.height;
  return NS_OK;
}

nsHTMLVideoElement::nsHTMLVideoElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
  : nsHTMLMediaElement(aNodeInfo, aFromParser)
{
}

nsHTMLVideoElement::~nsHTMLVideoElement()
{
}

nsIntSize nsHTMLVideoElement::GetVideoSize(nsIntSize aDefaultSize)
{
  return mMediaSize.width == -1 && mMediaSize.height == -1 ? aDefaultSize : mMediaSize;
}

PRBool
nsHTMLVideoElement::ParseAttribute(PRInt32 aNamespaceID,
                                   nsIAtom* aAttribute,
                                   const nsAString& aValue,
                                   nsAttrValue& aResult)
{
   if (aAttribute == nsGkAtoms::width || aAttribute == nsGkAtoms::height) {
     return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
   }

   return nsHTMLMediaElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                             aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  nsGenericHTMLElement::MapImageSizeAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLVideoElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::width },
    { &nsGkAtoms::height },
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

nsMapRuleToAttributesFunc
nsHTMLVideoElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

NS_IMPL_URI_ATTR(nsHTMLVideoElement, Poster, poster)
