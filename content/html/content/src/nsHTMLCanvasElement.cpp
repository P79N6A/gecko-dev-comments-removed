




































#include "nsHTMLCanvasElement.h"

#include "plbase64.h"
#include "nsNetUtil.h"
#include "prmem.h"

#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

#include "nsFrameManager.h"
#include "ImageLayers.h"
#include "BasicLayers.h"

#define DEFAULT_CANVAS_WIDTH 300
#define DEFAULT_CANVAS_HEIGHT 150

using namespace mozilla;
using namespace mozilla::layers;

nsGenericHTMLElement*
NS_NewHTMLCanvasElement(nsINodeInfo *aNodeInfo, PRUint32 aFromParser)
{
  return new nsHTMLCanvasElement(aNodeInfo);
}

nsHTMLCanvasElement::nsHTMLCanvasElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo), mWriteOnly(PR_FALSE)
{
}

nsHTMLCanvasElement::~nsHTMLCanvasElement()
{
  if (mCurrentContext) {
    nsCOMPtr<nsICanvasRenderingContextInternal> internalctx(do_QueryInterface(mCurrentContext));
    internalctx->SetCanvasElement(nsnull);
    mCurrentContext = nsnull;
  }
}

NS_IMPL_ADDREF_INHERITED(nsHTMLCanvasElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLCanvasElement, nsGenericElement)

DOMCI_DATA(HTMLCanvasElement, nsHTMLCanvasElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLCanvasElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLCanvasElement,
                                   nsIDOMHTMLCanvasElement,
                                   nsICanvasElementExternal)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLCanvasElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLCanvasElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLCanvasElement)

nsIntSize
nsHTMLCanvasElement::GetWidthHeight()
{
  nsIntSize size(0,0);
  const nsAttrValue* value;

  if ((value = GetParsedAttr(nsGkAtoms::width)) &&
      value->Type() == nsAttrValue::eInteger)
  {
      size.width = value->GetIntegerValue();
  }

  if ((value = GetParsedAttr(nsGkAtoms::height)) &&
      value->Type() == nsAttrValue::eInteger)
  {
      size.height = value->GetIntegerValue();
  }

  if (size.width <= 0)
    size.width = DEFAULT_CANVAS_WIDTH;
  if (size.height <= 0)
    size.height = DEFAULT_CANVAS_HEIGHT;

  return size;
}

NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLCanvasElement, Width, width, DEFAULT_CANVAS_WIDTH)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLCanvasElement, Height, height, DEFAULT_CANVAS_HEIGHT)
NS_IMPL_BOOL_ATTR(nsHTMLCanvasElement, MozOpaque, moz_opaque)

nsresult
nsHTMLCanvasElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aPrefix, const nsAString& aValue,
                             PRBool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                              aNotify);
  if (NS_SUCCEEDED(rv) && mCurrentContext &&
      (aName == nsGkAtoms::width || aName == nsGkAtoms::height || aName == nsGkAtoms::moz_opaque))
  {
    rv = UpdateContext();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

nsresult
nsHTMLCanvasElement::CopyInnerTo(nsGenericElement* aDest) const
{
  nsresult rv = nsGenericHTMLElement::CopyInnerTo(aDest);
  NS_ENSURE_SUCCESS(rv, rv);
  if (aDest->GetOwnerDoc()->IsStaticDocument()) {
    nsHTMLCanvasElement* dest = static_cast<nsHTMLCanvasElement*>(aDest);
    nsCOMPtr<nsISupports> cxt;
    dest->GetContext(NS_LITERAL_STRING("2d"), getter_AddRefs(cxt));
    nsCOMPtr<nsIDOMCanvasRenderingContext2D> context2d = do_QueryInterface(cxt);
    if (context2d) {
      context2d->DrawImage(const_cast<nsHTMLCanvasElement*>(this),
                           0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0);
    }
  }
  return rv;
}

nsChangeHint
nsHTMLCanvasElement::GetAttributeChangeHint(const nsIAtom* aAttribute,
                                            PRInt32 aModType) const
{
  nsChangeHint retval =
    nsGenericHTMLElement::GetAttributeChangeHint(aAttribute, aModType);
  if (aAttribute == nsGkAtoms::width ||
      aAttribute == nsGkAtoms::height)
  {
    NS_UpdateHint(retval, NS_STYLE_HINT_REFLOW);
  } else if (aAttribute == nsGkAtoms::moz_opaque)
  {
    NS_UpdateHint(retval, NS_STYLE_HINT_VISUAL);
  }
  return retval;
}

PRBool
nsHTMLCanvasElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::width || aAttribute == nsGkAtoms::height)) {
    return aResult.ParseIntWithBounds(aValue, 0);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}




NS_IMETHODIMP
nsHTMLCanvasElement::ToDataURL(const nsAString& aType, const nsAString& aParams,
                               PRUint8 optional_argc, nsAString& aDataURL)
{
  
  
  if ((mWriteOnly || optional_argc >= 2) &&
      !nsContentUtils::IsCallerTrustedForRead()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsAutoString type(aType);

  if (type.IsEmpty()) {
    type.AssignLiteral("image/png");
  }

  return ToDataURLImpl(type, aParams, aDataURL);
}






NS_IMETHODIMP
nsHTMLCanvasElement::ToDataURLAs(const nsAString& aMimeType,
                                 const nsAString& aEncoderOptions,
                                 nsAString& aDataURL)
{
  return ToDataURLImpl(aMimeType, aEncoderOptions, aDataURL);
}

nsresult
nsHTMLCanvasElement::ToDataURLImpl(const nsAString& aMimeType,
                                   const nsAString& aEncoderOptions,
                                   nsAString& aDataURL)
{
  nsresult rv;
  
  
  
  
  nsCOMPtr<nsICanvasRenderingContextInternal> context;
  rv = GetContext(NS_LITERAL_STRING("2d"), getter_AddRefs(context));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIInputStream> imgStream;
  NS_ConvertUTF16toUTF8 aMimeType8(aMimeType);
  rv = context->GetInputStream(nsPromiseFlatCString(aMimeType8).get(),
                               nsPromiseFlatString(aEncoderOptions).get(),
                               getter_AddRefs(imgStream));
  
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  PRUint32 bufSize;
  rv = imgStream->Available(&bufSize);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  bufSize += 16;
  PRUint32 imgSize = 0;
  char* imgData = (char*)PR_Malloc(bufSize);
  if (! imgData)
    return NS_ERROR_OUT_OF_MEMORY;
  PRUint32 numReadThisTime = 0;
  while ((rv = imgStream->Read(&imgData[imgSize], bufSize - imgSize,
                         &numReadThisTime)) == NS_OK && numReadThisTime > 0) {
    imgSize += numReadThisTime;
    if (imgSize == bufSize) {
      
      bufSize *= 2;
      char* newImgData = (char*)PR_Realloc(imgData, bufSize);
      if (! newImgData) {
        PR_Free(imgData);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      imgData = newImgData;
    }
  }

  
  char* encodedImg = PL_Base64Encode(imgData, imgSize, nsnull);
  PR_Free(imgData);
  if (!encodedImg) 
    return NS_ERROR_OUT_OF_MEMORY;

  
  aDataURL = NS_LITERAL_STRING("data:") + aMimeType +
    NS_LITERAL_STRING(";base64,") + NS_ConvertUTF8toUTF16(encodedImg);

  PR_Free(encodedImg);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCanvasElement::GetContext(const nsAString& aContextId,
                                nsISupports **aContext)
{
  nsresult rv;

  if (mCurrentContextId.IsEmpty()) {
    nsCString ctxId;
    ctxId.Assign(NS_LossyConvertUTF16toASCII(aContextId));

    
    for (PRUint32 i = 0; i < ctxId.Length(); i++) {
      if ((ctxId[i] < 'A' || ctxId[i] > 'Z') &&
          (ctxId[i] < 'a' || ctxId[i] > 'z') &&
          (ctxId[i] < '0' || ctxId[i] > '9') &&
          (ctxId[i] != '-') &&
          (ctxId[i] != '_'))
      {
        
        return NS_ERROR_INVALID_ARG;
      }
    }

    nsCString ctxString("@mozilla.org/content/canvas-rendering-context;1?id=");
    ctxString.Append(ctxId);

    mCurrentContext = do_CreateInstance(nsPromiseFlatCString(ctxString).get(), &rv);
    if (rv == NS_ERROR_OUT_OF_MEMORY)
      return NS_ERROR_OUT_OF_MEMORY;
    if (NS_FAILED(rv))
      
      return NS_ERROR_INVALID_ARG;

    rv = mCurrentContext->SetCanvasElement(this);
    if (NS_FAILED(rv)) {
      mCurrentContext = nsnull;
      return rv;
    }

    rv = UpdateContext();
    if (NS_FAILED(rv)) {
      mCurrentContext = nsnull;
      return rv;
    }

    mCurrentContextId.Assign(aContextId);
  } else if (!mCurrentContextId.Equals(aContextId)) {
    
    return NS_ERROR_INVALID_ARG;
  }

  NS_ADDREF (*aContext = mCurrentContext);
  return NS_OK;
}

nsresult
nsHTMLCanvasElement::UpdateContext()
{
  nsresult rv = NS_OK;
  if (mCurrentContext) {
    nsIntSize sz = GetWidthHeight();
    rv = mCurrentContext->SetIsOpaque(GetIsOpaque());
    rv = mCurrentContext->SetDimensions(sz.width, sz.height);
  }

  return rv;
}

nsIFrame *
nsHTMLCanvasElement::GetPrimaryCanvasFrame()
{
  return GetPrimaryFrame(Flush_Frames);
}

nsIntSize
nsHTMLCanvasElement::GetSize()
{
  return GetWidthHeight();
}

PRBool
nsHTMLCanvasElement::IsWriteOnly()
{
  return mWriteOnly;
}

void
nsHTMLCanvasElement::SetWriteOnly()
{
  mWriteOnly = PR_TRUE;
}

void
nsHTMLCanvasElement::InvalidateFrame(const gfxRect* damageRect)
{
  
  
  nsIFrame *frame = GetPrimaryFrame();
  if (!frame)
    return;

  if (damageRect) {
    nsRect contentArea(frame->GetContentRect());
    nsIntSize size = GetWidthHeight();

    
    
    
    gfxRect realRect(*damageRect);
    realRect.Scale(contentArea.width / gfxFloat(size.width),
                   contentArea.height / gfxFloat(size.height));
    realRect.RoundOut();

    
    nsRect invalRect(realRect.X(), realRect.Y(),
                     realRect.Width(), realRect.Height());

    
    invalRect.MoveBy(contentArea.TopLeft() - frame->GetPosition());

    frame->Invalidate(invalRect);
  } else {
    nsRect r(frame->GetContentRect() - frame->GetPosition());
    frame->Invalidate(r);
  }
}

PRInt32
nsHTMLCanvasElement::CountContexts()
{
  if (mCurrentContext)
    return 1;

  return 0;
}

nsICanvasRenderingContextInternal *
nsHTMLCanvasElement::GetContextAtIndex (PRInt32 index)
{
  if (mCurrentContext && index == 0)
    return mCurrentContext.get();

  return NULL;
}

PRBool
nsHTMLCanvasElement::GetIsOpaque()
{
  return HasAttr(kNameSpaceID_None, nsGkAtoms::moz_opaque);
}

already_AddRefed<CanvasLayer>
nsHTMLCanvasElement::GetCanvasLayer(LayerManager *aManager)
{
  if (!mCurrentContext)
    return nsnull;

  return mCurrentContext->GetCanvasLayer(aManager);
}

void
nsHTMLCanvasElement::MarkContextClean()
{
  if (!mCurrentContext)
    return;

  mCurrentContext->MarkContextClean();
}

NS_IMETHODIMP_(nsIntSize)
nsHTMLCanvasElement::GetSizeExternal()
{
  return GetWidthHeight();
}

NS_IMETHODIMP
nsHTMLCanvasElement::RenderContextsExternal(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter)
{
  if (!mCurrentContext)
    return NS_OK;

  return mCurrentContext->Render(aContext, aFilter);
}
