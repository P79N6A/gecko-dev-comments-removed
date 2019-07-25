














































#include "nsReadableUtils.h"
#include "nsTextBoxFrame.h"
#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsBoxLayoutState.h"
#include "nsMenuBarListener.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULLabelElement.h"
#include "nsIEventStateManager.h"
#include "nsITheme.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsCSSRendering.h"
#include "nsIReflowCallback.h"
#include "nsBoxFrame.h"
#include "nsIThebesFontMetrics.h"

#ifdef IBMBIDI
#include "nsBidiUtils.h"
#include "nsBidiPresUtils.h"
#endif 

#define CROP_LEFT   "left"
#define CROP_RIGHT  "right"
#define CROP_CENTER "center"
#define CROP_START  "start"
#define CROP_END    "end"

class nsAccessKeyInfo
{
public:
    PRInt32 mAccesskeyIndex;
    nscoord mBeforeWidth, mAccessWidth, mAccessUnderlineSize, mAccessOffset;
};


PRBool nsTextBoxFrame::gAlwaysAppendAccessKey          = PR_FALSE;
PRBool nsTextBoxFrame::gAccessKeyPrefInitialized       = PR_FALSE;
PRBool nsTextBoxFrame::gInsertSeparatorBeforeAccessKey = PR_FALSE;
PRBool nsTextBoxFrame::gInsertSeparatorPrefInitialized = PR_FALSE;






nsIFrame*
NS_NewTextBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
    return new (aPresShell) nsTextBoxFrame (aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTextBoxFrame)


NS_IMETHODIMP
nsTextBoxFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 PRInt32         aModType)
{
    PRBool aResize;
    PRBool aRedraw;

    UpdateAttributes(aAttribute, aResize, aRedraw);

    if (aResize) {
        PresContext()->PresShell()->
            FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                             NS_FRAME_IS_DIRTY);
    } else if (aRedraw) {
        nsBoxLayoutState state(PresContext());
        Redraw(state);
    }

    
    
    if (aAttribute == nsGkAtoms::accesskey || aAttribute == nsGkAtoms::control)
        RegUnregAccessKey(PR_TRUE);

    return NS_OK;
}

nsTextBoxFrame::nsTextBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsLeafBoxFrame(aShell, aContext), mAccessKeyInfo(nsnull), mCropType(CropRight),
  mNeedsReflowCallback(PR_FALSE)
{
    MarkIntrinsicWidthsDirty();
}

nsTextBoxFrame::~nsTextBoxFrame()
{
    delete mAccessKeyInfo;
}


NS_IMETHODIMP
nsTextBoxFrame::Init(nsIContent*      aContent,
                     nsIFrame*        aParent,
                     nsIFrame*        aPrevInFlow)
{
    nsTextBoxFrameSuper::Init(aContent, aParent, aPrevInFlow);

    PRBool aResize;
    PRBool aRedraw;
    UpdateAttributes(nsnull, aResize, aRedraw); 

    
    RegUnregAccessKey(PR_TRUE);

    return NS_OK;
}

void
nsTextBoxFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
    
    RegUnregAccessKey(PR_FALSE);
    nsTextBoxFrameSuper::DestroyFrom(aDestructRoot);
}

PRBool
nsTextBoxFrame::AlwaysAppendAccessKey()
{
  if (!gAccessKeyPrefInitialized) 
  {
    gAccessKeyPrefInitialized = PR_TRUE;

    const char* prefName = "intl.menuitems.alwaysappendaccesskeys";
    nsAdoptingString val = nsContentUtils::GetLocalizedStringPref(prefName);
    gAlwaysAppendAccessKey = val.Equals(NS_LITERAL_STRING("true"));
  }
  return gAlwaysAppendAccessKey;
}

PRBool
nsTextBoxFrame::InsertSeparatorBeforeAccessKey()
{
  if (!gInsertSeparatorPrefInitialized)
  {
    gInsertSeparatorPrefInitialized = PR_TRUE;

    const char* prefName = "intl.menuitems.insertseparatorbeforeaccesskeys";
    nsAdoptingString val = nsContentUtils::GetLocalizedStringPref(prefName);
    gInsertSeparatorBeforeAccessKey = val.EqualsLiteral("true");
  }
  return gInsertSeparatorBeforeAccessKey;
}

class nsAsyncAccesskeyUpdate : public nsIReflowCallback
{
public:
    nsAsyncAccesskeyUpdate(nsIFrame* aFrame) : mWeakFrame(aFrame)
    {
    }

    virtual PRBool ReflowFinished()
    {
        PRBool shouldFlush = PR_FALSE;
        nsTextBoxFrame* frame =
            static_cast<nsTextBoxFrame*>(mWeakFrame.GetFrame());
        if (frame) {
            shouldFlush = frame->UpdateAccesskey(mWeakFrame);
        }
        delete this;
        return shouldFlush;
    }

    virtual void ReflowCallbackCanceled()
    {
        delete this;
    }

    nsWeakFrame mWeakFrame;
};

PRBool
nsTextBoxFrame::UpdateAccesskey(nsWeakFrame& aWeakThis)
{
    nsAutoString accesskey;
    nsCOMPtr<nsIDOMXULLabelElement> labelElement = do_QueryInterface(mContent);
    NS_ENSURE_TRUE(aWeakThis.IsAlive(), PR_FALSE);
    if (labelElement) {
        
        
        
        
        
        nsCxPusher cx;
        if (cx.Push(mContent)) {
          labelElement->GetAccessKey(accesskey);
          NS_ENSURE_TRUE(aWeakThis.IsAlive(), PR_FALSE);
        }
    }
    else {
        mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accesskey);
    }

    if (!accesskey.Equals(mAccessKey)) {
        
        mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, mTitle);
        mAccessKey = accesskey;
        UpdateAccessTitle();
        PresContext()->PresShell()->
            FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                             NS_FRAME_IS_DIRTY);
        return PR_TRUE;
    }
    return PR_FALSE;
}

void
nsTextBoxFrame::UpdateAttributes(nsIAtom*         aAttribute,
                                 PRBool&          aResize,
                                 PRBool&          aRedraw)
{
    PRBool doUpdateTitle = PR_FALSE;
    aResize = PR_FALSE;
    aRedraw = PR_FALSE;

    if (aAttribute == nsnull || aAttribute == nsGkAtoms::crop) {
        static nsIContent::AttrValuesArray strings[] =
          {&nsGkAtoms::left, &nsGkAtoms::start, &nsGkAtoms::center,
           &nsGkAtoms::right, &nsGkAtoms::end, nsnull};
        CroppingStyle cropType;
        switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::crop,
                                          strings, eCaseMatters)) {
          case 0:
          case 1:
            cropType = CropLeft;
            break;
          case 2:
            cropType = CropCenter;
            break;
          case 3:
          case 4:
            cropType = CropRight;
            break;
          default:
            cropType = CropNone;
            break;
        }

        if (cropType != mCropType) {
            aResize = PR_TRUE;
            mCropType = cropType;
        }
    }

    if (aAttribute == nsnull || aAttribute == nsGkAtoms::value) {
        mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, mTitle);
        doUpdateTitle = PR_TRUE;
    }

    if (aAttribute == nsnull || aAttribute == nsGkAtoms::accesskey) {
        mNeedsReflowCallback = PR_TRUE;
        
        aResize = PR_TRUE;
    }

    if (doUpdateTitle) {
        UpdateAccessTitle();
        aResize = PR_TRUE;
    }

}

class nsDisplayXULTextBox : public nsDisplayItem {
public:
  nsDisplayXULTextBox(nsDisplayListBuilder* aBuilder,
                      nsTextBoxFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame),
    mDisableSubpixelAA(PR_FALSE)
  {
    MOZ_COUNT_CTOR(nsDisplayXULTextBox);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULTextBox() {
    MOZ_COUNT_DTOR(nsDisplayXULTextBox);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder);
  NS_DISPLAY_DECL_NAME("XULTextBox", TYPE_XUL_TEXT_BOX)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder);

  virtual void DisableComponentAlpha() { mDisableSubpixelAA = PR_TRUE; }

  PRPackedBool mDisableSubpixelAA;
};

void
nsDisplayXULTextBox::Paint(nsDisplayListBuilder* aBuilder,
                           nsRenderingContext* aCtx)
{
  gfxContextAutoDisableSubpixelAntialiasing disable(aCtx->ThebesContext(),
                                                    mDisableSubpixelAA);
  static_cast<nsTextBoxFrame*>(mFrame)->
    PaintTitle(*aCtx, mVisibleRect, ToReferenceFrame());
}

nsRect
nsDisplayXULTextBox::GetBounds(nsDisplayListBuilder* aBuilder) {
  return mFrame->GetVisualOverflowRect() + ToReferenceFrame();
}

nsRect
nsDisplayXULTextBox::GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
{
  return static_cast<nsTextBoxFrame*>(mFrame)->GetComponentAlphaBounds() +
      ToReferenceFrame();
}

NS_IMETHODIMP
nsTextBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists)
{
    if (!IsVisibleForPainting(aBuilder))
      return NS_OK;

    nsresult rv = nsLeafBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
    
    return aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayXULTextBox(aBuilder, this));
}

void
nsTextBoxFrame::PaintTitle(nsRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nsPoint              aPt)
{
    if (mTitle.IsEmpty())
        return;

    nsRect textRect = mTextDrawRect + aPt;

    
    const nsStyleText* textStyle = GetStyleText();
    if (textStyle->mTextShadow) {
      
      
      for (PRUint32 i = textStyle->mTextShadow->Length(); i > 0; --i) {
        PaintOneShadow(aRenderingContext.ThebesContext(),
                       textRect,
                       textStyle->mTextShadow->ShadowAt(i - 1),
                       GetStyleColor()->mColor,
                       aDirtyRect);
      }
    }

    DrawText(aRenderingContext, textRect, nsnull);
}

void
nsTextBoxFrame::DrawText(nsRenderingContext& aRenderingContext,
                         const nsRect&        aTextRect,
                         const nscolor*       aOverrideColor)
{
    nsPresContext* presContext = PresContext();

    
    nscolor overColor;
    nscolor underColor;
    nscolor strikeColor;
    PRUint8 overStyle;
    PRUint8 underStyle;
    PRUint8 strikeStyle;
    nsStyleContext* context = mStyleContext;
  
    PRUint8 decorations = NS_STYLE_TEXT_DECORATION_NONE; 
    PRUint8 decorMask = NS_STYLE_TEXT_DECORATION_UNDERLINE | NS_STYLE_TEXT_DECORATION_OVERLINE |
                        NS_STYLE_TEXT_DECORATION_LINE_THROUGH; 
    PRBool hasDecorations = context->HasTextDecorations();

    do {  
      const nsStyleTextReset* styleText = context->GetStyleTextReset();
      
      if (decorMask & styleText->mTextDecoration) {  
        nscolor color;
        if (aOverrideColor) {
          color = *aOverrideColor;
        } else {
          PRBool isForeground;
          styleText->GetDecorationColor(color, isForeground);
          if (isForeground) {
            color = context->GetVisitedDependentColor(eCSSProperty_color);
          }
        }
        PRUint8 style = styleText->GetDecorationStyle();

        if (NS_STYLE_TEXT_DECORATION_UNDERLINE & decorMask & styleText->mTextDecoration) {
          underColor = color;
          underStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
          decorations |= NS_STYLE_TEXT_DECORATION_UNDERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_OVERLINE & decorMask & styleText->mTextDecoration) {
          overColor = color;
          overStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_OVERLINE;
          decorations |= NS_STYLE_TEXT_DECORATION_OVERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_LINE_THROUGH & decorMask & styleText->mTextDecoration) {
          strikeColor = color;
          strikeStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
          decorations |= NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
        }
      }
      if (0 != decorMask) {
        context = context->GetParent();
        if (context) {
          hasDecorations = context->HasTextDecorations();
        }
      }
    } while (context && hasDecorations && (0 != decorMask));

    nsCOMPtr<nsIFontMetrics> fontMet;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));

    nscoord offset;
    nscoord size;
    nscoord ascent;
    fontMet->GetMaxAscent(ascent);

    nscoord baseline =
      presContext->RoundAppUnitsToNearestDevPixels(aTextRect.y + ascent);
    nsRefPtr<gfxContext> ctx = aRenderingContext.ThebesContext();
    gfxPoint pt(presContext->AppUnitsToGfxUnits(aTextRect.x),
                presContext->AppUnitsToGfxUnits(aTextRect.y));
    gfxFloat width = presContext->AppUnitsToGfxUnits(aTextRect.width);
    gfxFloat ascentPixel = presContext->AppUnitsToGfxUnits(ascent);

    
    
    
    
    
    if (decorations & (NS_FONT_DECORATION_OVERLINE |
                       NS_FONT_DECORATION_UNDERLINE)) {
      fontMet->GetUnderline(offset, size);
      gfxFloat offsetPixel = presContext->AppUnitsToGfxUnits(offset);
      gfxFloat sizePixel = presContext->AppUnitsToGfxUnits(size);
      if (decorations & NS_FONT_DECORATION_UNDERLINE) {
        nsCSSRendering::PaintDecorationLine(ctx, underColor,
                          pt, gfxSize(width, sizePixel),
                          ascentPixel, offsetPixel,
                          NS_STYLE_TEXT_DECORATION_UNDERLINE, underStyle);
      }
      if (decorations & NS_FONT_DECORATION_OVERLINE) {
        nsCSSRendering::PaintDecorationLine(ctx, overColor,
                          pt, gfxSize(width, sizePixel),
                          ascentPixel, ascentPixel,
                          NS_STYLE_TEXT_DECORATION_OVERLINE, overStyle);
      }
    }

    nsRefPtr<nsRenderingContext> refContext =
        PresContext()->PresShell()->GetReferenceRenderingContext();

    aRenderingContext.SetFont(fontMet);
    refContext->SetFont(fontMet);

    CalculateUnderline(*refContext);

    aRenderingContext.SetColor(aOverrideColor ? *aOverrideColor : GetStyleColor()->mColor);

#ifdef IBMBIDI
    nsresult rv = NS_ERROR_FAILURE;

    if (mState & NS_FRAME_IS_BIDI) {
      presContext->SetBidiEnabled();
      const nsStyleVisibility* vis = GetStyleVisibility();
      nsBidiDirection direction = (NS_STYLE_DIRECTION_RTL == vis->mDirection) ? NSBIDI_RTL : NSBIDI_LTR;
      if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
          
          
          nsBidiPositionResolve posResolve;
          posResolve.logicalIndex = mAccessKeyInfo->mAccesskeyIndex;
          rv = nsBidiPresUtils::RenderText(mCroppedTitle.get(), mCroppedTitle.Length(), direction,
                                           presContext, aRenderingContext,
                                           *refContext,
                                           aTextRect.x, baseline,
                                           &posResolve,
                                           1);
          mAccessKeyInfo->mBeforeWidth = posResolve.visualLeftTwips;
          mAccessKeyInfo->mAccessWidth = posResolve.visualWidth;
      }
      else
      {
          rv = nsBidiPresUtils::RenderText(mCroppedTitle.get(), mCroppedTitle.Length(), direction,
                                           presContext, aRenderingContext,
                                           *refContext,
                                           aTextRect.x, baseline);
      }
    }
    if (NS_FAILED(rv) )
#endif 
    {
       aRenderingContext.SetTextRunRTL(PR_FALSE);

       if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
           
           
           
           if (mAccessKeyInfo->mAccesskeyIndex > 0)
               refContext->GetWidth(mCroppedTitle.get(), mAccessKeyInfo->mAccesskeyIndex,
                                    mAccessKeyInfo->mBeforeWidth);
           else
               mAccessKeyInfo->mBeforeWidth = 0;
       }

       nsIThebesFontMetrics* fm = static_cast<nsIThebesFontMetrics*>(fontMet.get());
       fm->DrawString(mCroppedTitle.get(), mCroppedTitle.Length(),
                      aTextRect.x, baseline, &aRenderingContext, refContext.get());
    }

    if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
        aRenderingContext.FillRect(aTextRect.x + mAccessKeyInfo->mBeforeWidth,
                                   aTextRect.y + mAccessKeyInfo->mAccessOffset,
                                   mAccessKeyInfo->mAccessWidth,
                                   mAccessKeyInfo->mAccessUnderlineSize);
    }

    
    
    if (decorations & NS_FONT_DECORATION_LINE_THROUGH) {
      fontMet->GetStrikeout(offset, size);
      gfxFloat offsetPixel = presContext->AppUnitsToGfxUnits(offset);
      gfxFloat sizePixel = presContext->AppUnitsToGfxUnits(size);
      nsCSSRendering::PaintDecorationLine(ctx, strikeColor,
                        pt, gfxSize(width, sizePixel), ascentPixel, offsetPixel,
                        NS_STYLE_TEXT_DECORATION_LINE_THROUGH, strikeStyle);
    }
}

void nsTextBoxFrame::PaintOneShadow(gfxContext*      aCtx,
                                    const nsRect&    aTextRect,
                                    nsCSSShadowItem* aShadowDetails,
                                    const nscolor&   aForegroundColor,
                                    const nsRect&    aDirtyRect) {
  nsPoint shadowOffset(aShadowDetails->mXOffset,
                       aShadowDetails->mYOffset);
  nscoord blurRadius = NS_MAX(aShadowDetails->mRadius, 0);

  nsRect shadowRect(aTextRect);
  shadowRect.MoveBy(shadowOffset);

  nsContextBoxBlur contextBoxBlur;
  gfxContext* shadowContext = contextBoxBlur.Init(shadowRect, 0, blurRadius,
                                                  PresContext()->AppUnitsPerDevPixel(),
                                                  aCtx, aDirtyRect, nsnull);

  if (!shadowContext)
    return;

  nscolor shadowColor;
  if (aShadowDetails->mHasColor)
    shadowColor = aShadowDetails->mColor;
  else
    shadowColor = aForegroundColor;

  
  nsRefPtr<nsRenderingContext> renderingContext = nsnull;
  nsIDeviceContext* devCtx = PresContext()->DeviceContext();
  devCtx->CreateRenderingContextInstance(*getter_AddRefs(renderingContext));
  if (!renderingContext)
    return;
  renderingContext->Init(devCtx, shadowContext);

  aCtx->Save();
  aCtx->NewPath();
  aCtx->SetColor(gfxRGBA(shadowColor));

  
  
  
  DrawText(*renderingContext, shadowRect, &shadowColor);
  contextBoxBlur.DoPaint();
  aCtx->Restore();
}

void
nsTextBoxFrame::CalculateUnderline(nsRenderingContext& aRenderingContext)
{
    if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
         
         
         const PRUnichar *titleString = mCroppedTitle.get();
         aRenderingContext.SetTextRunRTL(PR_FALSE);
         aRenderingContext.GetWidth(titleString[mAccessKeyInfo->mAccesskeyIndex],
                                    mAccessKeyInfo->mAccessWidth);

         nscoord offset, baseline;
         nsCOMPtr<nsIFontMetrics> metrics
             = aRenderingContext.GetFontMetrics();
         metrics->GetUnderline(offset, mAccessKeyInfo->mAccessUnderlineSize);
         metrics->GetMaxAscent(baseline);
         mAccessKeyInfo->mAccessOffset = baseline - offset;
    }
}

nscoord
nsTextBoxFrame::CalculateTitleForWidth(nsPresContext*      aPresContext,
                                       nsRenderingContext& aRenderingContext,
                                       nscoord              aWidth)
{
    if (mTitle.IsEmpty())
        return 0;

    nsLayoutUtils::SetFontFromStyle(&aRenderingContext, GetStyleContext());

    
    nscoord titleWidth = nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                                       mTitle.get(), mTitle.Length());

    if (titleWidth <= aWidth) {
        mCroppedTitle = mTitle;
#ifdef IBMBIDI
        if (HasRTLChars(mTitle)) {
            mState |= NS_FRAME_IS_BIDI;
        }
#endif 
        return titleWidth;  
    }

    const nsDependentString& kEllipsis = nsContentUtils::GetLocalizedEllipsis();
    
    mCroppedTitle.Assign(kEllipsis);

    
    
    aRenderingContext.SetTextRunRTL(PR_FALSE);
    aRenderingContext.GetWidth(kEllipsis, titleWidth);

    if (titleWidth > aWidth) {
        mCroppedTitle.SetLength(0);
        return 0;
    }

    
    if (titleWidth == aWidth)
        return titleWidth;

    aWidth -= titleWidth;

    
    
    
    switch (mCropType)
    {
        case CropNone:
        case CropRight:
        {
            nscoord cwidth;
            nscoord twidth = 0;
            int length = mTitle.Length();
            int i;
            for (i = 0; i < length; ++i) {
                PRUnichar ch = mTitle.CharAt(i);
                
                aRenderingContext.GetWidth(ch,cwidth);
                if (twidth + cwidth > aWidth)
                    break;

                twidth += cwidth;
#ifdef IBMBIDI
                if (UCS2_CHAR_IS_BIDI(ch) ) {
                  mState |= NS_FRAME_IS_BIDI;
                }
#endif 
            }

            if (i == 0)
                return titleWidth;

            
            nsAutoString title( mTitle );
            title.Truncate(i);
            mCroppedTitle.Insert(title, 0);
        }
        break;

        case CropLeft:
        {
            nscoord cwidth;
            nscoord twidth = 0;
            int length = mTitle.Length();
            int i;
            for (i=length-1; i >= 0; --i) {
                PRUnichar ch = mTitle.CharAt(i);
                aRenderingContext.GetWidth(ch,cwidth);
                if (twidth + cwidth > aWidth)
                    break;

                twidth += cwidth;
#ifdef IBMBIDI
                if (UCS2_CHAR_IS_BIDI(ch) ) {
                  mState |= NS_FRAME_IS_BIDI;
                }
#endif 
            }

            if (i == length-1)
                return titleWidth;

            nsAutoString copy;
            mTitle.Right(copy, length-1-i);
            mCroppedTitle += copy;
        }
        break;

        case CropCenter:
        {
            nscoord stringWidth =
                nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                              mTitle.get(), mTitle.Length());
            if (stringWidth <= aWidth) {
                
                mCroppedTitle.Insert(mTitle, 0);
                break;
            }

            
            nscoord charWidth = 0;
            nscoord totalWidth = 0;
            PRUnichar ch;
            int leftPos, rightPos;
            nsAutoString leftString, rightString;

            rightPos = mTitle.Length() - 1;
            aRenderingContext.SetTextRunRTL(PR_FALSE);
            for (leftPos = 0; leftPos <= rightPos;) {
                
                ch = mTitle.CharAt(leftPos);
                aRenderingContext.GetWidth(ch, charWidth);
                totalWidth += charWidth;
                if (totalWidth > aWidth)
                    
                    break;
                leftString.Insert(ch, leftString.Length());

#ifdef IBMBIDI
                if (UCS2_CHAR_IS_BIDI(ch))
                    mState |= NS_FRAME_IS_BIDI;
#endif

                
                if (rightPos > leftPos) {
                    
                    ch = mTitle.CharAt(rightPos);
                    aRenderingContext.GetWidth(ch, charWidth);
                    totalWidth += charWidth;
                    if (totalWidth > aWidth)
                        
                        break;
                    rightString.Insert(ch, 0);

#ifdef IBMBIDI
                    if (UCS2_CHAR_IS_BIDI(ch))
                        mState |= NS_FRAME_IS_BIDI;
#endif
                }

                
                leftPos++;
                rightPos--;
            }

            mCroppedTitle = leftString + kEllipsis + rightString;
        }
        break;
    }

    return nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                         mCroppedTitle.get(), mCroppedTitle.Length());
}

#define OLD_ELLIPSIS NS_LITERAL_STRING("...")



void
nsTextBoxFrame::UpdateAccessTitle()
{
    





    PRInt32 menuAccessKey;
    nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
    if (!menuAccessKey || mAccessKey.IsEmpty())
        return;

    if (!AlwaysAppendAccessKey() &&
        FindInReadable(mAccessKey, mTitle, nsCaseInsensitiveStringComparator()))
        return;

    nsAutoString accessKeyLabel;
    accessKeyLabel += '(';
    accessKeyLabel += mAccessKey;
    ToUpperCase(accessKeyLabel);
    accessKeyLabel += ')';

    if (mTitle.IsEmpty()) {
        mTitle = accessKeyLabel;
        return;
    }

    const nsDependentString& kEllipsis = nsContentUtils::GetLocalizedEllipsis();
    PRUint32 offset = mTitle.Length();
    if (StringEndsWith(mTitle, kEllipsis)) {
        offset -= kEllipsis.Length();
    } else if (StringEndsWith(mTitle, OLD_ELLIPSIS)) {
        
        offset -= OLD_ELLIPSIS.Length();
    } else {
        
        
        const PRUnichar kLastChar = mTitle.Last();
        if (kLastChar == PRUnichar(0x2026) || kLastChar == PRUnichar(':'))
            offset--;
    }

    if (InsertSeparatorBeforeAccessKey() &&
        offset > 0 && !NS_IS_SPACE(mTitle[offset - 1])) {
        mTitle.Insert(' ', offset);
        offset++;
    }

    mTitle.Insert(accessKeyLabel, offset);
}

void
nsTextBoxFrame::UpdateAccessIndex()
{
    PRInt32 menuAccessKey;
    nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
    if (menuAccessKey) {
        if (mAccessKey.IsEmpty()) {
            if (mAccessKeyInfo) {
                delete mAccessKeyInfo;
                mAccessKeyInfo = nsnull;
            }
        } else {
            if (!mAccessKeyInfo) {
                mAccessKeyInfo = new nsAccessKeyInfo();
                if (!mAccessKeyInfo)
                    return;
            }

            nsAString::const_iterator start, end;
                
            mCroppedTitle.BeginReading(start);
            mCroppedTitle.EndReading(end);
            
            
            nsAString::const_iterator originalStart = start;

            PRBool found;
            if (!AlwaysAppendAccessKey()) {
                
                
                found = FindInReadable(mAccessKey, start, end);
                if (!found) {
                    
                    start = originalStart;
                    found = FindInReadable(mAccessKey, start, end,
                                           nsCaseInsensitiveStringComparator());
                }
            } else {
                found = RFindInReadable(mAccessKey, start, end,
                                        nsCaseInsensitiveStringComparator());
            }
            
            if (found)
                mAccessKeyInfo->mAccesskeyIndex = Distance(originalStart, start);
            else
                mAccessKeyInfo->mAccesskeyIndex = kNotFound;
        }
    }
}

NS_IMETHODIMP
nsTextBoxFrame::DoLayout(nsBoxLayoutState& aBoxLayoutState)
{
    if (mNeedsReflowCallback) {
        nsIReflowCallback* cb = new nsAsyncAccesskeyUpdate(this);
        if (cb) {
            PresContext()->PresShell()->PostReflowCallback(cb);
        }
        mNeedsReflowCallback = PR_FALSE;
    }

    nsresult rv = nsLeafBoxFrame::DoLayout(aBoxLayoutState);

    CalcDrawRect(*aBoxLayoutState.GetRenderingContext());

    const nsStyleText* textStyle = GetStyleText();
    if (textStyle->mTextShadow) {
      nsRect bounds(nsPoint(0, 0), GetSize());
      nsOverflowAreas overflow(bounds, bounds);
      
      
      nsPoint origin(0,0);
      nsRect &vis = overflow.VisualOverflow();
      vis.UnionRect(vis, nsLayoutUtils::GetTextShadowRectsUnion(mTextDrawRect, this));
      FinishAndStoreOverflow(overflow, GetSize());
    }

    return rv;
}

nsRect
nsTextBoxFrame::GetComponentAlphaBounds()
{
  return nsLayoutUtils::GetTextShadowRectsUnion(mTextDrawRect, this,
                                                nsLayoutUtils::EXCLUDE_BLUR_SHADOWS);
}

PRBool
nsTextBoxFrame::ComputesOwnOverflowArea()
{
    return PR_TRUE;
}

 void
nsTextBoxFrame::MarkIntrinsicWidthsDirty()
{
    mNeedsRecalc = PR_TRUE;
    nsTextBoxFrameSuper::MarkIntrinsicWidthsDirty();
}

void
nsTextBoxFrame::GetTextSize(nsPresContext* aPresContext, nsRenderingContext& aRenderingContext,
                                const nsString& aString, nsSize& aSize, nscoord& aAscent)
{
    nsCOMPtr<nsIFontMetrics> fontMet;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));
    fontMet->GetHeight(aSize.height);
    aRenderingContext.SetFont(fontMet);
    aSize.width =
      nsLayoutUtils::GetStringWidth(this, &aRenderingContext, aString.get(), aString.Length());
    fontMet->GetMaxAscent(aAscent);
}

void
nsTextBoxFrame::CalcTextSize(nsBoxLayoutState& aBoxLayoutState)
{
    if (mNeedsRecalc)
    {
        nsSize size;
        nsPresContext* presContext = aBoxLayoutState.PresContext();
        nsRenderingContext* rendContext = aBoxLayoutState.GetRenderingContext();
        if (rendContext) {
            GetTextSize(presContext, *rendContext,
                        mTitle, size, mAscent);
            mTextSize = size;
            mNeedsRecalc = PR_FALSE;
        }
    }
}

void
nsTextBoxFrame::CalcDrawRect(nsRenderingContext &aRenderingContext)
{
    nsRect textRect(nsPoint(0, 0), GetSize());
    nsMargin borderPadding;
    GetBorderAndPadding(borderPadding);
    textRect.Deflate(borderPadding);

    
    nsPresContext* presContext = PresContext();
    
    nscoord titleWidth =
        CalculateTitleForWidth(presContext, aRenderingContext, textRect.width);
    
    UpdateAccessIndex();

    
    nscoord outerWidth = textRect.width;
    textRect.width = titleWidth;

    
    const nsStyleVisibility* vis = GetStyleVisibility();
    const nsStyleText* textStyle = GetStyleText();

    if (textStyle->mTextAlign == NS_STYLE_TEXT_ALIGN_CENTER)
      textRect.x += (outerWidth - textRect.width)/2;
    else if (textStyle->mTextAlign == NS_STYLE_TEXT_ALIGN_RIGHT ||
             (textStyle->mTextAlign == NS_STYLE_TEXT_ALIGN_DEFAULT &&
              vis->mDirection == NS_STYLE_DIRECTION_RTL) ||
             (textStyle->mTextAlign == NS_STYLE_TEXT_ALIGN_END &&
              vis->mDirection == NS_STYLE_DIRECTION_LTR)) {
      textRect.x += (outerWidth - textRect.width);
    }

    mTextDrawRect = textRect;
}




nsSize
nsTextBoxFrame::GetPrefSize(nsBoxLayoutState& aBoxLayoutState)
{
    CalcTextSize(aBoxLayoutState);

    nsSize size = mTextSize;
    DISPLAY_PREF_SIZE(this, size);

    AddBorderAndPadding(size);
    PRBool widthSet, heightSet;
    nsIBox::AddCSSPrefSize(this, size, widthSet, heightSet);

    return size;
}




nsSize
nsTextBoxFrame::GetMinSize(nsBoxLayoutState& aBoxLayoutState)
{
    CalcTextSize(aBoxLayoutState);

    nsSize size = mTextSize;
    DISPLAY_MIN_SIZE(this, size);

    
    if (mCropType != CropNone)
        size.width = 0;

    AddBorderAndPadding(size);
    PRBool widthSet, heightSet;
    nsIBox::AddCSSMinSize(aBoxLayoutState, this, size, widthSet, heightSet);

    return size;
}

nscoord
nsTextBoxFrame::GetBoxAscent(nsBoxLayoutState& aBoxLayoutState)
{
    CalcTextSize(aBoxLayoutState);

    nscoord ascent = mAscent;

    nsMargin m(0,0,0,0);
    GetBorderAndPadding(m);
    ascent += m.top;

    return ascent;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTextBoxFrame::GetFrameName(nsAString& aResult) const
{
    MakeFrameName(NS_LITERAL_STRING("TextBox"), aResult);
    aResult += NS_LITERAL_STRING("[value=") + mTitle + NS_LITERAL_STRING("]");
    return NS_OK;
}
#endif



nsresult
nsTextBoxFrame::RegUnregAccessKey(PRBool aDoReg)
{
    
    if (!mContent)
        return NS_ERROR_FAILURE;

    
    
    

    
    
    
    
    if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::control))
        return NS_OK;

    
    nsAutoString accessKey;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);

    if (accessKey.IsEmpty())
        return NS_OK;

    nsresult rv;

    
    
    nsIEventStateManager *esm = PresContext()->EventStateManager();

    PRUint32 key = accessKey.First();
    if (aDoReg)
        rv = esm->RegisterAccessKey(mContent, key);
    else
        rv = esm->UnregisterAccessKey(mContent, key);

    return rv;
}
