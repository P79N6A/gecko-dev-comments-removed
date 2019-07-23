




































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

#include "nsLayoutUtils.h"

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
  NS_IMETHOD RenderContexts(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter);
  virtual PRBool IsWriteOnly();
  virtual void SetWriteOnly();
  NS_IMETHOD InvalidateFrame ();
  NS_IMETHOD InvalidateFrameSubrect (const gfxRect& damageRect);
  virtual PRInt32 CountContexts();
  virtual nsICanvasRenderingContextInternal *GetContextAtIndex (PRInt32 index);
  virtual PRBool GetIsOpaque();

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

NS_INTERFACE_TABLE_HEAD(nsHTMLCanvasElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLCanvasElement,
                                   nsIDOMHTMLCanvasElement,
                                   nsICanvasElement)
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
nsHTMLCanvasElement::RenderContexts(gfxContext *ctx, gfxPattern::GraphicsFilter aFilter)
{
  if (!mCurrentContext)
    return NS_OK;

  return mCurrentContext->Render(ctx, aFilter);
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
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return NS_OK;
  }

  
  
  nsIFrame *frame = GetPrimaryFrameFor(this, doc);
  if (frame) {
    nsRect r = frame->GetRect();
    r.x = r.y = 0;
    frame->Invalidate(r);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLCanvasElement::InvalidateFrameSubrect(const gfxRect& damageRect)
{
  nsIDocument* doc = GetCurrentDoc();
  if (!doc) {
    return NS_OK;
  }

  
  
  nsIFrame *frame = GetPrimaryFrameFor(this, doc);
  if (frame) {
    
    
    
    nsAutoDisableGetUsedXAssertions noAssert;
    
    nsRect contentArea(frame->GetContentRect());
    nsIntSize size = GetWidthHeight();

    
    
    
    gfxRect realRect(damageRect);
    realRect.Scale(contentArea.width / gfxFloat(size.width),
                   contentArea.height / gfxFloat(size.height));
    realRect.RoundOut();

    
    nsRect invalRect(realRect.X(), realRect.Y(),
                     realRect.Width(), realRect.Height());

    
    invalRect.MoveBy(contentArea.TopLeft() - frame->GetPosition());

    frame->Invalidate(invalRect);
  }

  return NS_OK;
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
