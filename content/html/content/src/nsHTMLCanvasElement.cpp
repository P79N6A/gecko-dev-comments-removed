




































#include "nsHTMLCanvasElement.h"

#include "mozilla/Base64.h"
#include "nsNetUtil.h"
#include "prmem.h"
#include "nsDOMFile.h"
#include "CheckedInt.h"

#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"
#include "nsJSUtils.h"
#include "nsMathUtils.h"

#include "nsFrameManager.h"
#include "nsDisplayList.h"
#include "ImageLayers.h"
#include "BasicLayers.h"
#include "imgIEncoder.h"

#include "nsIWritablePropertyBag2.h"

#define DEFAULT_CANVAS_WIDTH 300
#define DEFAULT_CANVAS_HEIGHT 150

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::layers;

nsGenericHTMLElement*
NS_NewHTMLCanvasElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                        FromParser aFromParser)
{
  return new nsHTMLCanvasElement(aNodeInfo);
}

nsHTMLCanvasElement::nsHTMLCanvasElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo), mWriteOnly(PR_FALSE)
{
}

nsHTMLCanvasElement::~nsHTMLCanvasElement()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLCanvasElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLCanvasElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCurrentContext)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLCanvasElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLCanvasElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLCanvasElement, nsHTMLCanvasElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLCanvasElement)
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
  nsIntSize size(DEFAULT_CANVAS_WIDTH, DEFAULT_CANVAS_HEIGHT);
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

  return size;
}

NS_IMPL_UINT_ATTR_DEFAULT_VALUE(nsHTMLCanvasElement, Width, width, DEFAULT_CANVAS_WIDTH)
NS_IMPL_UINT_ATTR_DEFAULT_VALUE(nsHTMLCanvasElement, Height, height, DEFAULT_CANVAS_HEIGHT)
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
    dest->GetContext(NS_LITERAL_STRING("2d"), JSVAL_VOID, getter_AddRefs(cxt));
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
    return aResult.ParseNonNegativeIntValue(aValue);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}




NS_IMETHODIMP
nsHTMLCanvasElement::ToDataURL(const nsAString& aType, nsIVariant* aParams,
                               PRUint8 optional_argc, nsAString& aDataURL)
{
  
  if (mWriteOnly && !nsContentUtils::IsCallerTrustedForRead()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  return ToDataURLImpl(aType, aParams, aDataURL);
}

nsresult
nsHTMLCanvasElement::ExtractData(const nsAString& aType,
                                 const nsAString& aOptions,
                                 nsIInputStream** aStream,
                                 bool& aFellBackToPNG)
{
  
  
  
  nsRefPtr<gfxImageSurface> emptyCanvas;
  nsIntSize size = GetWidthHeight();
  if (!mCurrentContext) {
    emptyCanvas = new gfxImageSurface(gfxIntSize(size.width, size.height), gfxASurface::ImageFormatARGB32);
  }

  nsresult rv;

  
  nsCOMPtr<nsIInputStream> imgStream;
  NS_ConvertUTF16toUTF8 encoderType(aType);

 try_again:
  if (mCurrentContext) {
    rv = mCurrentContext->GetInputStream(nsPromiseFlatCString(encoderType).get(),
                                         nsPromiseFlatString(aOptions).get(),
                                         getter_AddRefs(imgStream));
  } else {
    
    nsCString enccid("@mozilla.org/image/encoder;2?type=");
    enccid += encoderType;

    nsCOMPtr<imgIEncoder> encoder = do_CreateInstance(nsPromiseFlatCString(enccid).get(), &rv);
    if (NS_SUCCEEDED(rv) && encoder) {
      rv = encoder->InitFromData(emptyCanvas->Data(),
                                 size.width * size.height * 4,
                                 size.width,
                                 size.height,
                                 size.width * 4,
                                 imgIEncoder::INPUT_FORMAT_HOSTARGB,
                                 aOptions);
      if (NS_SUCCEEDED(rv)) {
        imgStream = do_QueryInterface(encoder);
      }
    } else {
      rv = NS_ERROR_FAILURE;
    }
  }

  if (NS_FAILED(rv) && !aFellBackToPNG) {
    
    
    aFellBackToPNG = true;
    encoderType.AssignLiteral("image/png");
    goto try_again;
  }

  NS_ENSURE_SUCCESS(rv, rv);

  return CallQueryInterface(imgStream, aStream);
  return NS_OK;
}

nsresult
nsHTMLCanvasElement::ToDataURLImpl(const nsAString& aMimeType,
                                   nsIVariant* aEncoderOptions,
                                   nsAString& aDataURL)
{
  bool fallbackToPNG = false;

  nsIntSize size = GetWidthHeight();
  if (size.height == 0 || size.width == 0) {
    aDataURL = NS_LITERAL_STRING("data:,");
    return NS_OK;
  }

  nsAutoString type;
  nsContentUtils::ASCIIToLower(aMimeType, type);

  nsAutoString params;

  
  if (type.EqualsLiteral("image/jpeg")) {
    PRUint16 vartype;

    if (aEncoderOptions &&
        NS_SUCCEEDED(aEncoderOptions->GetDataType(&vartype)) &&
        vartype <= nsIDataType::VTYPE_DOUBLE) {

      double quality;
      
      if (NS_SUCCEEDED(aEncoderOptions->GetAsDouble(&quality)) &&
          quality >= 0.0 && quality <= 1.0) {
        params.AppendLiteral("quality=");
        params.AppendInt(NS_lround(quality * 100.0));
      }
    }
  }

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = ExtractData(type, params, getter_AddRefs(stream),
                            fallbackToPNG);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (fallbackToPNG)
    aDataURL = NS_LITERAL_STRING("data:image/png;base64,");
  else
    aDataURL = NS_LITERAL_STRING("data:") + type +
      NS_LITERAL_STRING(";base64,");

  PRUint32 count;
  rv = stream->Available(&count);
  NS_ENSURE_SUCCESS(rv, rv);

  return Base64EncodeInputStream(stream, aDataURL, count, aDataURL.Length());
}

NS_IMETHODIMP
nsHTMLCanvasElement::MozGetAsFile(const nsAString& aName,
                                  const nsAString& aType,
                                  PRUint8 optional_argc,
                                  nsIDOMFile** aResult)
{
  
  if ((mWriteOnly) &&
      !nsContentUtils::IsCallerTrustedForRead()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  return MozGetAsFileImpl(aName, aType, aResult);
}

nsresult
nsHTMLCanvasElement::MozGetAsFileImpl(const nsAString& aName,
                                      const nsAString& aType,
                                      nsIDOMFile** aResult)
{
  bool fallbackToPNG = false;

  nsCOMPtr<nsIInputStream> stream;
  nsresult rv = ExtractData(aType, EmptyString(), getter_AddRefs(stream),
                            fallbackToPNG);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString type(aType);
  if (fallbackToPNG) {
    type.AssignLiteral("image/png");
  }

  PRUint32 imgSize;
  rv = stream->Available(&imgSize);
  NS_ENSURE_SUCCESS(rv, rv);

  void* imgData = nsnull;
  rv = NS_ReadInputStreamToBuffer(stream, &imgData, imgSize);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsRefPtr<nsDOMMemoryFile> file =
    new nsDOMMemoryFile(imgData, imgSize, aName, type);

  return CallQueryInterface(file, aResult);
}

nsresult
nsHTMLCanvasElement::GetContextHelper(const nsAString& aContextId,
                                      nsICanvasRenderingContextInternal **aContext)
{
  NS_ENSURE_ARG(aContext);

  NS_LossyConvertUTF16toASCII ctxId(aContextId);

  
  for (PRUint32 i = 0; i < ctxId.Length(); i++) {
    if ((ctxId[i] < 'A' || ctxId[i] > 'Z') &&
        (ctxId[i] < 'a' || ctxId[i] > 'z') &&
        (ctxId[i] < '0' || ctxId[i] > '9') &&
        (ctxId[i] != '-') &&
        (ctxId[i] != '_'))
    {
      
      return NS_OK;
    }
  }

  nsCString ctxString("@mozilla.org/content/canvas-rendering-context;1?id=");
  ctxString.Append(ctxId);

  nsresult rv;
  nsCOMPtr<nsICanvasRenderingContextInternal> ctx =
    do_CreateInstance(nsPromiseFlatCString(ctxString).get(), &rv);
  if (rv == NS_ERROR_OUT_OF_MEMORY) {
    *aContext = nsnull;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (NS_FAILED(rv)) {
    *aContext = nsnull;
    
    return NS_OK;
  }

  rv = ctx->SetCanvasElement(this);
  if (NS_FAILED(rv)) {
    *aContext = nsnull;
    return rv;
  }

  *aContext = ctx.forget().get();

  return rv;
}

NS_IMETHODIMP
nsHTMLCanvasElement::GetContext(const nsAString& aContextId,
                                const jsval& aContextOptions,
                                nsISupports **aContext)
{
  nsresult rv;

  if (mCurrentContextId.IsEmpty()) {
    rv = GetContextHelper(aContextId, getter_AddRefs(mCurrentContext));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!mCurrentContext) {
      return NS_OK;
    }

    
    
    nsXPCOMCycleCollectionParticipant *cp = nsnull;
    CallQueryInterface(mCurrentContext, &cp);
    if (!cp) {
      mCurrentContext = nsnull;
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIPropertyBag> contextProps;
    if (!JSVAL_IS_NULL(aContextOptions) &&
        !JSVAL_IS_VOID(aContextOptions))
    {
      JSContext *cx = nsContentUtils::GetCurrentJSContext();

      nsCOMPtr<nsIWritablePropertyBag2> newProps;

      
      
      
      if (JSVAL_IS_OBJECT(aContextOptions)) {
        newProps = do_CreateInstance("@mozilla.org/hash-property-bag;1");

        JSObject *opts = JSVAL_TO_OBJECT(aContextOptions);
        JSIdArray *props = JS_Enumerate(cx, opts);
        for (int i = 0; props && i < props->length; ++i) {
          jsid propid = props->vector[i];
          jsval propname, propval;
          if (!JS_IdToValue(cx, propid, &propname) ||
              !JS_GetPropertyById(cx, opts, propid, &propval))
          {
            continue;
          }

          JSString *propnameString = JS_ValueToString(cx, propname);
          nsDependentJSString pstr;
          if (!propnameString || !pstr.init(cx, propnameString)) {
            mCurrentContext = nsnull;
            return NS_ERROR_FAILURE;
          }

          if (JSVAL_IS_BOOLEAN(propval)) {
            newProps->SetPropertyAsBool(pstr, propval == JSVAL_TRUE ? PR_TRUE : PR_FALSE);
          } else if (JSVAL_IS_INT(propval)) {
            newProps->SetPropertyAsInt32(pstr, JSVAL_TO_INT(propval));
          } else if (JSVAL_IS_DOUBLE(propval)) {
            newProps->SetPropertyAsDouble(pstr, JSVAL_TO_DOUBLE(propval));
          } else if (JSVAL_IS_STRING(propval)) {
            JSString *propvalString = JS_ValueToString(cx, propval);
            nsDependentJSString vstr;
            if (!propvalString || !vstr.init(cx, propvalString)) {
              mCurrentContext = nsnull;
              return NS_ERROR_FAILURE;
            }

            newProps->SetPropertyAsAString(pstr, vstr);
          }
        }
      }

      contextProps = newProps;
    }

    rv = UpdateContext(contextProps);
    if (NS_FAILED(rv)) {
      mCurrentContext = nsnull;
      return rv;
    }

    mCurrentContextId.Assign(aContextId);
  } else if (!mCurrentContextId.Equals(aContextId)) {
    
    return NS_OK;
  }

  NS_ADDREF (*aContext = mCurrentContext);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCanvasElement::MozGetIPCContext(const nsAString& aContextId,
                                      nsISupports **aContext)
{
  if(!nsContentUtils::IsCallerTrustedForRead()) {
    
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  
  if (!aContextId.Equals(NS_LITERAL_STRING("2d")))
    return NS_ERROR_INVALID_ARG;

  nsresult rv;

  if (mCurrentContextId.IsEmpty()) {
    rv = GetContextHelper(aContextId, getter_AddRefs(mCurrentContext));
    NS_ENSURE_SUCCESS(rv, rv);
    if (!mCurrentContext) {
      return NS_OK;
    }

    mCurrentContext->SetIsIPC(PR_TRUE);

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
nsHTMLCanvasElement::UpdateContext(nsIPropertyBag *aNewContextOptions)
{
  if (!mCurrentContext)
    return NS_OK;

  nsresult rv = NS_OK;
  nsIntSize sz = GetWidthHeight();

  rv = mCurrentContext->SetIsOpaque(GetIsOpaque());
  if (NS_FAILED(rv)) {
    mCurrentContext = nsnull;
    return rv;
  }

  rv = mCurrentContext->SetContextOptions(aNewContextOptions);
  if (NS_FAILED(rv)) {
    mCurrentContext = nsnull;
    return rv;
  }

  rv = mCurrentContext->SetDimensions(sz.width, sz.height);
  if (NS_FAILED(rv)) {
    mCurrentContext = nsnull;
    return rv;
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
nsHTMLCanvasElement::InvalidateCanvasContent(const gfxRect* damageRect)
{
  
  
  nsIFrame *frame = GetPrimaryFrame();
  if (!frame)
    return;

  frame->MarkLayersActive();

  nsRect invalRect;
  nsRect contentArea = frame->GetContentRect();
  if (damageRect) {
    nsIntSize size = GetWidthHeight();
    if (size.width != 0 && size.height != 0) {

      
      
      
      gfxRect realRect(*damageRect);
      realRect.Scale(contentArea.width / gfxFloat(size.width),
                     contentArea.height / gfxFloat(size.height));
      realRect.RoundOut();

      
      invalRect = nsRect(realRect.X(), realRect.Y(),
                         realRect.Width(), realRect.Height());
    }
  } else {
    invalRect = nsRect(nsPoint(0, 0), contentArea.Size());
  }
  invalRect.MoveBy(contentArea.TopLeft() - frame->GetPosition());

  Layer* layer = frame->InvalidateLayer(invalRect, nsDisplayItem::TYPE_CANVAS);
  if (layer) {
    static_cast<CanvasLayer*>(layer)->Updated();
  }
}

void
nsHTMLCanvasElement::InvalidateCanvas()
{
  
  
  nsIFrame *frame = GetPrimaryFrame();
  if (!frame)
    return;

  frame->Invalidate(frame->GetContentRect() - frame->GetPosition());
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
nsHTMLCanvasElement::GetCanvasLayer(nsDisplayListBuilder* aBuilder,
                                    CanvasLayer *aOldLayer,
                                    LayerManager *aManager)
{
  if (!mCurrentContext)
    return nsnull;

  return mCurrentContext->GetCanvasLayer(aBuilder, aOldLayer, aManager);
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
