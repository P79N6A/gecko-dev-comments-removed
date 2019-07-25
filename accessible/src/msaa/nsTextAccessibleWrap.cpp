






































#include "nsTextAccessibleWrap.h"
#include "ISimpleDOMText_i.c"

#include "nsCoreUtils.h"
#include "nsDocAccessible.h"

#include "nsIFontMetrics.h"
#include "nsIFrame.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIRenderingContext.h"
#include "nsIComponentManager.h"





nsTextAccessibleWrap::
  nsTextAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
  nsTextAccessible(aContent, aShell)
{
}

STDMETHODIMP_(ULONG) nsTextAccessibleWrap::AddRef()
{
  return nsAccessNode::AddRef();
}

STDMETHODIMP_(ULONG) nsTextAccessibleWrap::Release()
{
  return nsAccessNode::Release();
}

STDMETHODIMP nsTextAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = nsnull;

  if (IID_IUnknown == iid || IID_ISimpleDOMText == iid)
    *ppv = static_cast<ISimpleDOMText*>(this);

  if (nsnull == *ppv)
    return nsAccessibleWrap::QueryInterface(iid, ppv);
   
  (reinterpret_cast<IUnknown*>(*ppv))->AddRef(); 
  return S_OK;
}

STDMETHODIMP nsTextAccessibleWrap::get_domText( 
     BSTR __RPC_FAR *aDomText)
{
__try {
  *aDomText = NULL;

  if (IsDefunct())
    return E_FAIL;

  nsAutoString nodeValue;

  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  DOMNode->GetNodeValue(nodeValue);
  if (nodeValue.IsEmpty())
    return S_FALSE;

  *aDomText = ::SysAllocStringLen(nodeValue.get(), nodeValue.Length());
  if (!*aDomText)
    return E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP nsTextAccessibleWrap::get_clippedSubstringBounds( 
     unsigned int aStartIndex,
     unsigned int aEndIndex,
     int __RPC_FAR *aX,
     int __RPC_FAR *aY,
     int __RPC_FAR *aWidth,
     int __RPC_FAR *aHeight)
{
__try {
  *aX = *aY = *aWidth = *aHeight = 0;
  nscoord x, y, width, height, docX, docY, docWidth, docHeight;
  HRESULT rv = get_unclippedSubstringBounds(aStartIndex, aEndIndex, &x, &y, &width, &height);
  if (FAILED(rv)) {
    return rv;
  }

  nsDocAccessible *docAccessible = GetDocAccessible();
  NS_ASSERTION(docAccessible,
               "There must always be a doc accessible, but there isn't. Crash!");

  docAccessible->GetBounds(&docX, &docY, &docWidth, &docHeight);

  nsIntRect unclippedRect(x, y, width, height);
  nsIntRect docRect(docX, docY, docWidth, docHeight);
  nsIntRect clippedRect;

  clippedRect.IntersectRect(unclippedRect, docRect);

  *aX = clippedRect.x;
  *aY = clippedRect.y;
  *aWidth = clippedRect.width;
  *aHeight = clippedRect.height;
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}

STDMETHODIMP nsTextAccessibleWrap::get_unclippedSubstringBounds( 
     unsigned int aStartIndex,
     unsigned int aEndIndex,
     int __RPC_FAR *aX,
     int __RPC_FAR *aY,
     int __RPC_FAR *aWidth,
     int __RPC_FAR *aHeight)
{
__try {
  *aX = *aY = *aWidth = *aHeight = 0;

  if (IsDefunct())
    return E_FAIL;

  if (NS_FAILED(GetCharacterExtents(aStartIndex, aEndIndex, 
                                    aX, aY, aWidth, aHeight))) {
    return NS_ERROR_FAILURE;
  }
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}


STDMETHODIMP nsTextAccessibleWrap::scrollToSubstring(
     unsigned int aStartIndex,
     unsigned int aEndIndex)
{
__try {
  if (IsDefunct())
    return E_FAIL;

  nsCOMPtr<nsIDOMNode> DOMNode(do_QueryInterface(mContent));
  nsresult rv =
    nsCoreUtils::ScrollSubstringTo(GetFrame(), DOMNode, aStartIndex,
                                   DOMNode, aEndIndex,
                                   nsIAccessibleScrollType::SCROLL_TYPE_ANYWHERE);
  if (NS_FAILED(rv))
    return E_FAIL;
} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }
  return S_OK;
}

nsIFrame* nsTextAccessibleWrap::GetPointFromOffset(nsIFrame *aContainingFrame, 
                                                   PRInt32 aOffset, 
                                                   PRBool aPreferNext, 
                                                   nsPoint& aOutPoint)
{
  nsIFrame *textFrame = nsnull;
  PRInt32 outOffset;
  aContainingFrame->GetChildFrameContainingOffset(aOffset, aPreferNext, &outOffset, &textFrame);
  if (!textFrame) {
    return nsnull;
  }

  textFrame->GetPointFromOffset(aOffset, &aOutPoint);
  return textFrame;
}




nsresult nsTextAccessibleWrap::GetCharacterExtents(PRInt32 aStartOffset, PRInt32 aEndOffset,
                                                   PRInt32* aX, PRInt32* aY, 
                                                   PRInt32* aWidth, PRInt32* aHeight) 
{
  *aX = *aY = *aWidth = *aHeight = 0;
  nsPresContext *presContext = GetPresContext();
  NS_ENSURE_TRUE(presContext, NS_ERROR_FAILURE);

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  nsPoint startPoint, endPoint;
  nsIFrame *startFrame = GetPointFromOffset(frame, aStartOffset, PR_TRUE, startPoint);
  nsIFrame *endFrame = GetPointFromOffset(frame, aEndOffset, PR_FALSE, endPoint);
  if (!startFrame || !endFrame) {
    return E_FAIL;
  }
  
  nsIntRect sum(0, 0, 0, 0);
  nsIFrame *iter = startFrame;
  nsIFrame *stopLoopFrame = endFrame->GetNextContinuation();
  for (; iter != stopLoopFrame; iter = iter->GetNextContinuation()) {
    nsIntRect rect = iter->GetScreenRectExternal();
    nscoord start = (iter == startFrame) ? presContext->AppUnitsToDevPixels(startPoint.x) : 0;
    nscoord end = (iter == endFrame) ? presContext->AppUnitsToDevPixels(endPoint.x) :
                                       rect.width;
    rect.x += start;
    rect.width = end - start;
    sum.UnionRect(sum, rect);
  }

  *aX      = sum.x;
  *aY      = sum.y;
  *aWidth  = sum.width;
  *aHeight = sum.height;

  return NS_OK;
}

STDMETHODIMP nsTextAccessibleWrap::get_fontFamily(
     BSTR __RPC_FAR *aFontFamily)
{
__try {
  *aFontFamily = NULL;

  nsIFrame *frame = GetFrame();
  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (!frame || !presShell || !presShell->GetPresContext()) {
    return E_FAIL;
  }

  nsCOMPtr<nsIRenderingContext> rc = presShell->GetReferenceRenderingContext();
  if (!rc) {
    return E_FAIL;
  }

  const nsStyleFont *font = frame->GetStyleFont();

  const nsStyleVisibility *visibility = frame->GetStyleVisibility();

  if (NS_FAILED(rc->SetFont(font->mFont, visibility->mLanguage,
                            presShell->GetPresContext()->GetUserFontSet()))) {
    return E_FAIL;
  }

  nsCOMPtr<nsIDeviceContext> deviceContext;
  rc->GetDeviceContext(*getter_AddRefs(deviceContext));
  if (!deviceContext) {
    return E_FAIL;
  }

  nsCOMPtr<nsIFontMetrics> fm;
  rc->GetFontMetrics(*getter_AddRefs(fm));
  if (!fm) {
    return E_FAIL;
  }

  nsAutoString fontFamily;
  deviceContext->FirstExistingFont(fm->Font(), fontFamily);
  if (fontFamily.IsEmpty())
    return S_FALSE;

  *aFontFamily = ::SysAllocStringLen(fontFamily.get(), fontFamily.Length());
  if (!*aFontFamily)
    return E_OUTOFMEMORY;

} __except(FilterA11yExceptions(::GetExceptionCode(), GetExceptionInformation())) { }

  return S_OK;
}
