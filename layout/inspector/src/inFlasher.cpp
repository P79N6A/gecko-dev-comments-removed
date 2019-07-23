





































#include "inFlasher.h"
#include "inLayoutUtils.h"

#include "nsIServiceManager.h"
#include "nsIPresShell.h"
#include "nsIFrame.h"
#include "nsReadableUtils.h"

#include "prprf.h"



inFlasher::inFlasher() :
  mColor(NS_RGB(0,0,0)),
  mThickness(0),
  mInvert(PR_FALSE)
{
}

inFlasher::~inFlasher()
{
}

NS_IMPL_ISUPPORTS1(inFlasher, inIFlasher)




NS_IMETHODIMP
inFlasher::GetColor(nsAString& aColor)
{
  
  char buf[10];
  PR_snprintf(buf, sizeof(buf), "#%02x%02x%02x",
              NS_GET_R(mColor), NS_GET_G(mColor), NS_GET_B(mColor));
  CopyASCIItoUTF16(buf, aColor);

  return NS_OK;
}

NS_IMETHODIMP
inFlasher::SetColor(const nsAString& aColor)
{
  NS_ENSURE_FALSE(aColor.IsEmpty(), NS_ERROR_ILLEGAL_VALUE);

  nsAutoString colorStr;
  colorStr.Assign(aColor);

  if (colorStr.CharAt(0) != '#') {
    if (NS_ColorNameToRGB(colorStr, &mColor)) {
      return NS_OK;
    }
  }
  else {
    colorStr.Cut(0, 1);
    if (NS_HexToRGB(colorStr, &mColor)) {
      return NS_OK;
    }
  }

  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
inFlasher::GetThickness(PRUint16 *aThickness)
{
  NS_PRECONDITION(aThickness, "Null pointer");
  *aThickness = mThickness;
  return NS_OK;
}

NS_IMETHODIMP
inFlasher::SetThickness(PRUint16 aThickness)
{
  mThickness = aThickness;
  return NS_OK;
}

NS_IMETHODIMP
inFlasher::GetInvert(PRBool *aInvert)
{
  NS_PRECONDITION(aInvert, "Null pointer");
  *aInvert = mInvert;
  return NS_OK;
}

NS_IMETHODIMP
inFlasher::SetInvert(PRBool aInvert)
{
  mInvert = aInvert;
  return NS_OK;
}

NS_IMETHODIMP
inFlasher::RepaintElement(nsIDOMElement* aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  nsCOMPtr<nsIDOMWindowInternal> window = inLayoutUtils::GetWindowFor(aElement);
  if (!window) return NS_OK;
  nsCOMPtr<nsIPresShell> presShell = inLayoutUtils::GetPresShellFor(window);
  if (!presShell) return NS_OK;
  nsIFrame* frame = inLayoutUtils::GetFrameFor(aElement, presShell);
  if (!frame) return NS_OK;

  frame->Invalidate(frame->GetRect());

  return NS_OK;
}

NS_IMETHODIMP 
inFlasher::DrawElementOutline(nsIDOMElement* aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  nsCOMPtr<nsIDOMWindowInternal> window = inLayoutUtils::GetWindowFor(aElement);
  if (!window) return NS_OK;
  nsCOMPtr<nsIPresShell> presShell = inLayoutUtils::GetPresShellFor(window);
  if (!presShell) return NS_OK;

  nsIFrame* frame = inLayoutUtils::GetFrameFor(aElement, presShell);

  PRBool isFirstFrame = PR_TRUE;

  while (frame) {
    nsCOMPtr<nsIRenderingContext> rcontext;
    nsresult rv =
      presShell->CreateRenderingContext(frame, getter_AddRefs(rcontext));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRect rect(nsPoint(0,0), frame->GetSize());
    if (mInvert) {
      rcontext->InvertRect(rect);
    }

    frame = frame->GetNextContinuation();

    PRBool isLastFrame = (frame == nsnull);
    DrawOutline(rect.x, rect.y, rect.width, rect.height, rcontext,
                isFirstFrame, isLastFrame);
    isFirstFrame = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
inFlasher::ScrollElementIntoView(nsIDOMElement *aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  nsCOMPtr<nsIDOMWindowInternal> window = inLayoutUtils::GetWindowFor(aElement);
  if (!window) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> presShell = inLayoutUtils::GetPresShellFor(window);
  if (!presShell) {
    return NS_OK;
  }

  nsCOMPtr<nsIContent> content = do_QueryInterface(aElement);
  presShell->ScrollContentIntoView(content,
                                   NS_PRESSHELL_SCROLL_ANYWHERE ,
                                   NS_PRESSHELL_SCROLL_ANYWHERE );

  return NS_OK;
}




void
inFlasher::DrawOutline(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                       nsIRenderingContext* aRenderContext,
                       PRBool aDrawBegin, PRBool aDrawEnd)
{
  aRenderContext->SetColor(mColor);

  DrawLine(aX, aY, aWidth, DIR_HORIZONTAL, BOUND_OUTER, aRenderContext);
  if (aDrawBegin) {
    DrawLine(aX, aY, aHeight, DIR_VERTICAL, BOUND_OUTER, aRenderContext);
  }
  DrawLine(aX, aY+aHeight, aWidth, DIR_HORIZONTAL, BOUND_INNER, aRenderContext);
  if (aDrawEnd) {
    DrawLine(aX+aWidth, aY, aHeight, DIR_VERTICAL, BOUND_INNER, aRenderContext);
  }
}

void
inFlasher::DrawLine(nscoord aX, nscoord aY, nscoord aLength,
                    PRBool aDir, PRBool aBounds,
                    nsIRenderingContext* aRenderContext)
{
  nscoord thickTwips = nsPresContext::CSSPixelsToAppUnits(mThickness);
  if (aDir) { 
    aRenderContext->FillRect(aX, aY+(aBounds?0:-thickTwips), aLength, thickTwips);
  } else { 
    aRenderContext->FillRect(aX+(aBounds?0:-thickTwips), aY, thickTwips, aLength);
  }
}
