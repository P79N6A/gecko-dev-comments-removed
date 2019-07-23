




































#include "nsIDOMHTMLCanvasElement.h"
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

#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "jsapi.h"

#include "nsICanvasElement.h"
#include "nsIRenderingContext.h"

#include "nsICanvasRenderingContextInternal.h"

#define DEFAULT_CANVAS_WIDTH 300
#define DEFAULT_CANVAS_HEIGHT 150

class nsHTMLCanvasElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLCanvasElement,
                            public nsICanvasElement
{
public:
  nsHTMLCanvasElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLCanvasElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLCANVASELEMENT

  
  NS_IMETHOD GetPrimaryCanvasFrame(nsIFrame **aFrame);
  NS_IMETHOD GetSize(PRUint32 *width, PRUint32 *height);
  NS_IMETHOD RenderContexts(gfxContext *ctx);
  virtual PRBool IsWriteOnly();
  virtual void SetWriteOnly();
  NS_IMETHOD InvalidateFrame ();
  NS_IMETHOD InvalidateFrameSubrect (const nsRect& damageRect);

  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  nsChangeHint GetAttributeChangeHint(const nsIAtom* aAttribute, PRInt32 aModType) const;

  
  
  nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                   const nsAString& aValue, PRBool aNotify)
  {
    return SetAttr(aNameSpaceID, aName, nsnull, aValue, aNotify);
  }
  virtual nsresult SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                           nsIAtom* aPrefix, const nsAString& aValue,
                           PRBool aNotify);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:
  nsIntSize GetWidthHeight();
  nsresult UpdateContext();
  nsresult ToDataURLImpl(const nsAString& aMimeType,
                         const nsAString& aEncoderOptions,
                         nsAString& aDataURL);

  nsString mCurrentContextId;
  nsCOMPtr<nsICanvasRenderingContextInternal> mCurrentContext;
  
public:
  
  
  
  
  PRPackedBool             mWriteOnly;
};

nsGenericHTMLElement*
NS_NewHTMLCanvasElement(nsINodeInfo *aNodeInfo, PRBool aFromParser)
{
  return new (aNodeInfo) nsHTMLCanvasElement(aNodeInfo);
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

NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLCanvasElement, nsGenericHTMLElement)
  NS_INTERFACE_TABLE_INHERITED2(nsHTMLCanvasElement,
                                nsIDOMHTMLCanvasElement,
                                nsICanvasElement)
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

nsresult
nsHTMLCanvasElement::SetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                             nsIAtom* aPrefix, const nsAString& aValue,
                             PRBool aNotify)
{
  nsresult rv = nsGenericHTMLElement::SetAttr(aNameSpaceID, aName, aPrefix, aValue,
                                              aNotify);
  if (NS_SUCCEEDED(rv) && mCurrentContext &&
      (aName == nsGkAtoms::width || aName == nsGkAtoms::height))
  {
    rv = UpdateContext();
    NS_ENSURE_SUCCESS(rv, rv);
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
  }
  return retval;
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  nsGenericHTMLElement::MapImageMarginAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

nsMapRuleToAttributesFunc
nsHTMLCanvasElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

static const nsGenericElement::MappedAttributeEntry
sImageMarginAttributeMap[] = {
  { &nsGkAtoms::hspace },
  { &nsGkAtoms::vspace },
  { nsnull }
};

NS_IMETHODIMP_(PRBool)
nsHTMLCanvasElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sCommonAttributeMap,
    sImageMarginAttributeMap
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

PRBool
nsHTMLCanvasElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None)
  {
    if ((aAttribute == nsGkAtoms::width) ||
        (aAttribute == nsGkAtoms::height))
    {
      return aResult.ParseIntWithBounds(aValue, 0);
    }

    if (ParseImageAttribute(aAttribute, aValue, aResult))
    {
      return PR_TRUE;
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}




NS_IMETHODIMP
nsHTMLCanvasElement::ToDataURL(nsAString& aDataURL)
{
  nsresult rv;

  nsAXPCNativeCallContext *ncc = nsnull;
  rv = nsContentUtils::XPConnect()->
    GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!ncc)
    return NS_ERROR_FAILURE;

  JSContext *ctx = nsnull;

  rv = ncc->GetJSContext(&ctx);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 argc;
  jsval *argv = nsnull;

  ncc->GetArgc(&argc);
  ncc->GetArgvPtr(&argv);

  
  
  if ((mWriteOnly || argc >= 2) && !nsContentUtils::IsCallerTrustedForRead()) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  
  if (argc == 0) {
    return ToDataURLImpl(NS_LITERAL_STRING("image/png"), EmptyString(), aDataURL);
  }

  JSAutoRequest ar(ctx);

  
  if (argc == 1) {
    if (!JSVAL_IS_STRING(argv[0]))
      return NS_ERROR_DOM_SYNTAX_ERR;
    JSString *type = JS_ValueToString(ctx, argv[0]);
    return ToDataURLImpl (nsDependentString(reinterpret_cast<PRUnichar*>((JS_GetStringChars(type)))),
                          EmptyString(), aDataURL);
  }

  
  if (argc == 2) {
    if (!JSVAL_IS_STRING(argv[0]) && !JSVAL_IS_STRING(argv[1]))
      return NS_ERROR_DOM_SYNTAX_ERR;

    JSString *type, *params;
    type = JS_ValueToString(ctx, argv[0]);
    params = JS_ValueToString(ctx, argv[1]);

    return ToDataURLImpl (nsDependentString(reinterpret_cast<PRUnichar*>(JS_GetStringChars(type))),
                          nsDependentString(reinterpret_cast<PRUnichar*>(JS_GetStringChars(params))),
                          aDataURL);
  }

  return NS_ERROR_DOM_SYNTAX_ERR;
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
    rv = mCurrentContext->SetDimensions(sz.width, sz.height);
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLCanvasElement::GetPrimaryCanvasFrame(nsIFrame **aFrame)
{
  *aFrame = GetPrimaryFrame(Flush_Frames);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCanvasElement::GetSize(PRUint32 *width, PRUint32 *height)
{
  nsIntSize sz = GetWidthHeight();
  *width = sz.width;
  *height = sz.height;

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCanvasElement::RenderContexts(gfxContext *ctx)
{
  if (!mCurrentContext)
    return NS_OK;

  return mCurrentContext->Render(ctx);
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

NS_IMETHODIMP
nsHTMLCanvasElement::InvalidateFrame()
{
  nsIFrame *frame = GetPrimaryFrame(Flush_Frames);
  if (frame) {
    nsRect r = frame->GetRect();
    r.x = r.y = 0;
    frame->Invalidate(r, PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCanvasElement::InvalidateFrameSubrect(const nsRect& damageRect)
{
  nsIFrame *frame = GetPrimaryFrame(Flush_Frames);
  if (frame) {
    frame->Invalidate(damageRect, PR_FALSE);
  }

  return NS_OK;
}
