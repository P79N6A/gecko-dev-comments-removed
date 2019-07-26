




#include "TextLeafAccessibleWrap.h"
#include "ISimpleDOMText_i.c"

#include "nsCoreUtils.h"
#include "DocAccessible.h"
#include "Statistics.h"
#include "nsIFrame.h"
#include "nsFontMetrics.h"
#include "nsPresContext.h"
#include "nsLayoutUtils.h"

#include "gfxFont.h"

using namespace mozilla::a11y;





TextLeafAccessibleWrap::
  TextLeafAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
  TextLeafAccessible(aContent, aDoc)
{
}

STDMETHODIMP_(ULONG)
TextLeafAccessibleWrap::AddRef()
{
  return nsAccessNode::AddRef();
}

STDMETHODIMP_(ULONG)
TextLeafAccessibleWrap::Release()
{
  return nsAccessNode::Release();
}

STDMETHODIMP
TextLeafAccessibleWrap::QueryInterface(REFIID iid, void** ppv)
{
  *ppv = nullptr;

  if (IID_IUnknown == iid) {
    *ppv = static_cast<ISimpleDOMText*>(this);
  } else if (IID_ISimpleDOMText == iid) {
    statistics::ISimpleDOMUsed();
    *ppv = static_cast<ISimpleDOMText*>(this);
  } else {
    return AccessibleWrap::QueryInterface(iid, ppv);
  }
   
  (reinterpret_cast<IUnknown*>(*ppv))->AddRef(); 
  return S_OK;
}

STDMETHODIMP
TextLeafAccessibleWrap::get_domText( 
     BSTR __RPC_FAR *aDomText)
{
  A11Y_TRYBLOCK_BEGIN

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
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
TextLeafAccessibleWrap::get_clippedSubstringBounds( 
     unsigned int aStartIndex,
     unsigned int aEndIndex,
     int __RPC_FAR *aX,
     int __RPC_FAR *aY,
     int __RPC_FAR *aWidth,
     int __RPC_FAR *aHeight)
{
  A11Y_TRYBLOCK_BEGIN

  *aX = *aY = *aWidth = *aHeight = 0;
  nscoord x, y, width, height, docX, docY, docWidth, docHeight;
  HRESULT rv = get_unclippedSubstringBounds(aStartIndex, aEndIndex, &x, &y, &width, &height);
  if (FAILED(rv)) {
    return rv;
  }

  DocAccessible* docAccessible = Document();
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
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
TextLeafAccessibleWrap::get_unclippedSubstringBounds( 
     unsigned int aStartIndex,
     unsigned int aEndIndex,
     int __RPC_FAR *aX,
     int __RPC_FAR *aY,
     int __RPC_FAR *aWidth,
     int __RPC_FAR *aHeight)
{
  A11Y_TRYBLOCK_BEGIN

  *aX = *aY = *aWidth = *aHeight = 0;

  if (IsDefunct())
    return E_FAIL;

  if (FAILED(GetCharacterExtents(aStartIndex, aEndIndex,
                                 aX, aY, aWidth, aHeight))) {
    return E_FAIL;
  }
  return S_OK;

  A11Y_TRYBLOCK_END
}

STDMETHODIMP
TextLeafAccessibleWrap::scrollToSubstring(
     unsigned int aStartIndex,
     unsigned int aEndIndex)
{
  A11Y_TRYBLOCK_BEGIN

  if (IsDefunct())
    return E_FAIL;

  nsRefPtr<nsRange> range = new nsRange();
  if (NS_FAILED(range->SetStart(mContent, aStartIndex)))
      return E_FAIL;

  if (NS_FAILED(range->SetEnd(mContent, aEndIndex)))
  return E_FAIL;

  nsresult rv =
    nsCoreUtils::ScrollSubstringTo(GetFrame(), range,
                                   nsIAccessibleScrollType::SCROLL_TYPE_ANYWHERE);
  return NS_SUCCEEDED(rv) ? S_OK : E_FAIL;

  A11Y_TRYBLOCK_END
}

nsIFrame*
TextLeafAccessibleWrap::GetPointFromOffset(nsIFrame* aContainingFrame, 
                                           int32_t aOffset, 
                                           bool aPreferNext, 
                                           nsPoint& aOutPoint)
{
  nsIFrame *textFrame = nullptr;
  int32_t outOffset;
  aContainingFrame->GetChildFrameContainingOffset(aOffset, aPreferNext, &outOffset, &textFrame);
  if (!textFrame) {
    return nullptr;
  }

  textFrame->GetPointFromOffset(aOffset, &aOutPoint);
  return textFrame;
}




HRESULT
TextLeafAccessibleWrap::GetCharacterExtents(int32_t aStartOffset,
                                            int32_t aEndOffset,
                                            int32_t* aX,
                                            int32_t* aY,
                                            int32_t* aWidth,
                                            int32_t* aHeight)
{
  if (!aX || !aY || !aWidth || !aHeight)
    return E_INVALIDARG;

  *aX = *aY = *aWidth = *aHeight = 0;

  if (IsDefunct())
    return CO_E_OBJNOTCONNECTED;

  nsIFrame *frame = GetFrame();
  NS_ENSURE_TRUE(frame, E_FAIL);

  nsPoint startPoint, endPoint;
  nsIFrame *startFrame = GetPointFromOffset(frame, aStartOffset, true, startPoint);
  nsIFrame *endFrame = GetPointFromOffset(frame, aEndOffset, false, endPoint);
  if (!startFrame || !endFrame) {
    return E_FAIL;
  }

  nsRect sum;
  nsIFrame *iter = startFrame;
  nsIFrame *stopLoopFrame = endFrame->GetNextContinuation();
  for (; iter != stopLoopFrame; iter = iter->GetNextContinuation()) {
    nsRect rect = iter->GetScreenRectInAppUnits();
    nscoord start = (iter == startFrame) ? startPoint.x : 0;
    nscoord end = (iter == endFrame) ? endPoint.x : rect.width;
    rect.x += start;
    rect.width = end - start;
    sum.UnionRect(sum, rect);
  }

  nsPresContext* presContext = mDoc->PresContext();
  *aX = presContext->AppUnitsToDevPixels(sum.x);
  *aY = presContext->AppUnitsToDevPixels(sum.y);
  *aWidth = presContext->AppUnitsToDevPixels(sum.width);
  *aHeight = presContext->AppUnitsToDevPixels(sum.height);

  return S_OK;
}

STDMETHODIMP
TextLeafAccessibleWrap::get_fontFamily(
     BSTR __RPC_FAR *aFontFamily)
{
  A11Y_TRYBLOCK_BEGIN

  *aFontFamily = NULL;

  nsIFrame* frame = GetFrame();
  if (!frame) {
    return E_FAIL;
  }

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(frame, getter_AddRefs(fm));

  const nsString& name = fm->GetThebesFontGroup()->GetFontAt(0)->GetName();
  if (name.IsEmpty())
    return S_FALSE;

  *aFontFamily = ::SysAllocStringLen(name.get(), name.Length());
  if (!*aFontFamily)
    return E_OUTOFMEMORY;
  return S_OK;

  A11Y_TRYBLOCK_END
}
