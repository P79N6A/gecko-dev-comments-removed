














































#include "nsReadableUtils.h"
#include "nsTextBoxFrame.h"
#include "nsCOMPtr.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
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

#ifdef IBMBIDI
#include "nsBidiUtils.h"
#include "nsBidiPresUtils.h"
#endif 


#define ELLIPSIS PRUnichar(0x2026)

#define CROP_LEFT   "left"
#define CROP_RIGHT  "right"
#define CROP_CENTER "center"
#define CROP_START  "start"
#define CROP_END    "end"

#define NS_STATE_NEED_LAYOUT 0x01000000

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


NS_IMETHODIMP
nsTextBoxFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 PRInt32         aModType)
{
    mState |= NS_STATE_NEED_LAYOUT;
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
  nsLeafBoxFrame(aShell, aContext), mCropType(CropRight),mAccessKeyInfo(nsnull)
{
    mState |= NS_STATE_NEED_LAYOUT;
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

    mState |= NS_STATE_NEED_LAYOUT;
    PRBool aResize;
    PRBool aRedraw;
    UpdateAttributes(nsnull, aResize, aRedraw); 

    
    RegUnregAccessKey(PR_TRUE);

    return NS_OK;
}

void
nsTextBoxFrame::Destroy()
{
    
    RegUnregAccessKey(PR_FALSE);
    nsTextBoxFrameSuper::Destroy();
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
        nsAutoString accesskey;
        nsCOMPtr<nsIDOMXULLabelElement> labelElement = do_QueryInterface(mContent);
        if (labelElement) {
          labelElement->GetAccessKey(accesskey);  
        }
        else {
          mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accesskey);
        }
        if (!accesskey.Equals(mAccessKey)) {
            if (!doUpdateTitle) {
                
                mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, mTitle);
                doUpdateTitle = PR_TRUE;
            }
            mAccessKey = accesskey;
        }
    }

    if (doUpdateTitle) {
        UpdateAccessTitle();
        aResize = PR_TRUE;
    }

}

class nsDisplayXULTextBox : public nsDisplayItem {
public:
  nsDisplayXULTextBox(nsTextBoxFrame* aFrame) : nsDisplayItem(aFrame) {
      MOZ_COUNT_CTOR(nsDisplayXULTextBox);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULTextBox() {
      MOZ_COUNT_DTOR(nsDisplayXULTextBox);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("XULTextBox")
};

void nsDisplayXULTextBox::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  static_cast<nsTextBoxFrame*>(mFrame)->
    PaintTitle(*aCtx, aDirtyRect, aBuilder->ToReferenceFrame(mFrame));
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
        nsDisplayXULTextBox(this));
}

void
nsTextBoxFrame::PaintTitle(nsIRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nsPoint              aPt)
{
    if (mTitle.IsEmpty())
        return;

    nsRect textRect(aPt, GetSize());
    textRect.Deflate(GetUsedBorderAndPadding());

    
    nsPresContext* presContext = PresContext();
    LayoutTitle(presContext, aRenderingContext, textRect);

    
    nscoord outerWidth = textRect.width;
    textRect.width = mTitleWidth;

    
    const nsStyleVisibility* vis = GetStyleVisibility();
    const nsStyleText* textStyle = GetStyleText();

    if (textStyle->mTextAlign == NS_STYLE_TEXT_ALIGN_CENTER)
      textRect.x += (outerWidth - textRect.width)/2;
    else if (textStyle->mTextAlign == NS_STYLE_TEXT_ALIGN_RIGHT) {
      if (vis->mDirection == NS_STYLE_DIRECTION_LTR)
        textRect.x += (outerWidth - textRect.width);
    }
    else {
      if (vis->mDirection == NS_STYLE_DIRECTION_RTL)
        textRect.x += (outerWidth - textRect.width);
    }

    
    if (PR_FALSE == aDirtyRect.Intersects(textRect))
        return;

    
    nscolor overColor;
    nscolor underColor;
    nscolor strikeColor;
    nsStyleContext* context = mStyleContext;
  
    PRUint8 decorations = NS_STYLE_TEXT_DECORATION_NONE; 
    PRUint8 decorMask = NS_STYLE_TEXT_DECORATION_UNDERLINE | NS_STYLE_TEXT_DECORATION_OVERLINE |
                        NS_STYLE_TEXT_DECORATION_LINE_THROUGH; 
    PRBool hasDecorations = context->HasTextDecorations();

    do {  
      const nsStyleTextReset* styleText = context->GetStyleTextReset();
      
      if (decorMask & styleText->mTextDecoration) {  
        nscolor color = context->GetStyleColor()->mColor;
    
        if (NS_STYLE_TEXT_DECORATION_UNDERLINE & decorMask & styleText->mTextDecoration) {
          underColor = color;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
          decorations |= NS_STYLE_TEXT_DECORATION_UNDERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_OVERLINE & decorMask & styleText->mTextDecoration) {
          overColor = color;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_OVERLINE;
          decorations |= NS_STYLE_TEXT_DECORATION_OVERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_LINE_THROUGH & decorMask & styleText->mTextDecoration) {
          strikeColor = color;
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

    const nsStyleFont* fontStyle = GetStyleFont();
    
    nscoord offset;
    nscoord size;
    nscoord baseline;
    nsCOMPtr<nsIFontMetrics> fontMet;
    presContext->DeviceContext()->GetMetricsFor(fontStyle->mFont,
                                                *getter_AddRefs(fontMet));
    fontMet->GetMaxAscent(baseline);
    PRBool isRTL = vis->mDirection == NS_STYLE_DIRECTION_RTL;

    nsRefPtr<gfxContext> ctx = (gfxContext*)
      aRenderingContext.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT);
    gfxFloat a2p = 1.0 / presContext->AppUnitsPerDevPixel();
    gfxPoint pt(textRect.x * a2p, textRect.y * a2p);
    gfxFloat width = textRect.width * a2p;
    gfxFloat baselinePixel = baseline * a2p;
    if (decorations & (NS_FONT_DECORATION_OVERLINE | NS_FONT_DECORATION_UNDERLINE)) {
      fontMet->GetUnderline(offset, size);
      gfxFloat offsetPixel = offset * a2p;
      gfxFloat sizePixel = size * a2p;
      if (decorations & NS_FONT_DECORATION_OVERLINE) {
        nsCSSRendering::PaintDecorationLine(ctx, overColor,
                                            pt, gfxSize(width, sizePixel),
                                            baselinePixel, baselinePixel,
                                            sizePixel,
                                            NS_STYLE_TEXT_DECORATION_OVERLINE,
                                            NS_STYLE_BORDER_STYLE_SOLID,
                                            isRTL);
      }
      if (decorations & NS_FONT_DECORATION_UNDERLINE) {
        nsCSSRendering::PaintDecorationLine(ctx, underColor,
                                            pt, gfxSize(width, sizePixel),
                                            baselinePixel, offsetPixel,
                                            sizePixel,
                                            NS_STYLE_TEXT_DECORATION_UNDERLINE,
                                            NS_STYLE_BORDER_STYLE_SOLID,
                                            isRTL);
      }
    }
    if (decorations & NS_FONT_DECORATION_LINE_THROUGH) {
      fontMet->GetStrikeout(offset, size);
      gfxFloat offsetPixel = offset * a2p;
      gfxFloat sizePixel = size * a2p;
      nsCSSRendering::PaintDecorationLine(ctx, underColor,
                                          pt, gfxSize(width, sizePixel),
                                          baselinePixel, offsetPixel,
                                          sizePixel,
                                          NS_STYLE_TEXT_DECORATION_LINE_THROUGH,
                                          NS_STYLE_BORDER_STYLE_SOLID,
                                          isRTL);
    }
 
    aRenderingContext.SetFont(fontStyle->mFont, nsnull);

    CalculateUnderline(aRenderingContext);

    aRenderingContext.SetColor(GetStyleColor()->mColor);

#ifdef IBMBIDI
    nsresult rv = NS_ERROR_FAILURE;

    if (mState & NS_FRAME_IS_BIDI) {
      presContext->SetBidiEnabled(PR_TRUE);
      nsBidiPresUtils* bidiUtils = presContext->GetBidiUtils();

      if (bidiUtils) {
        const nsStyleVisibility* vis = GetStyleVisibility();
        nsBidiDirection direction = (NS_STYLE_DIRECTION_RTL == vis->mDirection) ? NSBIDI_RTL : NSBIDI_LTR;
        if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
           
           
           nsBidiPositionResolve posResolve;
           posResolve.logicalIndex = mAccessKeyInfo->mAccesskeyIndex;
           rv = bidiUtils->RenderText(mCroppedTitle.get(), mCroppedTitle.Length(), direction,
                                      presContext, aRenderingContext,
                                      textRect.x, textRect.y + baseline,
                                      &posResolve,
                                      1);
           mAccessKeyInfo->mBeforeWidth = posResolve.visualLeftTwips;
        }
        else
        {
           rv = bidiUtils->RenderText(mCroppedTitle.get(), mCroppedTitle.Length(), direction,
                                      presContext, aRenderingContext,
                                      textRect.x, textRect.y + baseline);
        }
      }
    }
    if (NS_FAILED(rv) )
#endif 
    {
       aRenderingContext.SetTextRunRTL(PR_FALSE);

       if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
           
           
           
           if (mAccessKeyInfo->mAccesskeyIndex > 0)
               aRenderingContext.GetWidth(mCroppedTitle.get(), mAccessKeyInfo->mAccesskeyIndex,
                                          mAccessKeyInfo->mBeforeWidth);
           else
               mAccessKeyInfo->mBeforeWidth = 0;
       }

       aRenderingContext.DrawString(mCroppedTitle, textRect.x, textRect.y + baseline);
    }

    if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
        aRenderingContext.FillRect(textRect.x + mAccessKeyInfo->mBeforeWidth,
                                   textRect.y + mAccessKeyInfo->mAccessOffset,
                                   mAccessKeyInfo->mAccessWidth,
                                   mAccessKeyInfo->mAccessUnderlineSize);
    }
}

void
nsTextBoxFrame::LayoutTitle(nsPresContext*      aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            const nsRect&        aRect)
{
    
    if ((mState & NS_STATE_NEED_LAYOUT)) {

        
        CalculateTitleForWidth(aPresContext, aRenderingContext, aRect.width);

        
        UpdateAccessIndex();

        
        mState &= ~NS_STATE_NEED_LAYOUT;
    }
}

void
nsTextBoxFrame::CalculateUnderline(nsIRenderingContext& aRenderingContext)
{
    if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
         
         
         const PRUnichar *titleString = mCroppedTitle.get();
         aRenderingContext.SetTextRunRTL(PR_FALSE);
         aRenderingContext.GetWidth(titleString[mAccessKeyInfo->mAccesskeyIndex],
                                    mAccessKeyInfo->mAccessWidth);

         nscoord offset, baseline;
         nsIFontMetrics *metrics;
         aRenderingContext.GetFontMetrics(metrics);
         metrics->GetUnderline(offset, mAccessKeyInfo->mAccessUnderlineSize);
         metrics->GetMaxAscent(baseline);
         NS_RELEASE(metrics);
         mAccessKeyInfo->mAccessOffset = baseline - offset;
    }
}

void
nsTextBoxFrame::CalculateTitleForWidth(nsPresContext*      aPresContext,
                                       nsIRenderingContext& aRenderingContext,
                                       nscoord              aWidth)
{
    if (mTitle.IsEmpty())
        return;

    nsCOMPtr<nsIFontMetrics> fontMet;
    aPresContext->DeviceContext()->GetMetricsFor(GetStyleFont()->mFont,
                                                 *getter_AddRefs(fontMet));
    aRenderingContext.SetFont(fontMet);

    
    mTitleWidth = nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                                mTitle.get(), mTitle.Length());

    if (mTitleWidth <= aWidth) {
        mCroppedTitle = mTitle;
#ifdef IBMBIDI
        PRInt32 length = mTitle.Length();
        for (PRInt32 i = 0; i < length; i++) {
          if ((UCS2_CHAR_IS_BIDI(mTitle.CharAt(i)) ) ||
              ((NS_IS_HIGH_SURROGATE(mTitle.CharAt(i))) &&
               (++i < length) &&
               (NS_IS_LOW_SURROGATE(mTitle.CharAt(i))) &&
               (UTF32_CHAR_IS_BIDI(SURROGATE_TO_UCS4(mTitle.CharAt(i-1),
                                                     mTitle.CharAt(i)))))) {
            mState |= NS_FRAME_IS_BIDI;
            break;
          }
        }
#endif 
        return;  
    }

    
    mCroppedTitle.Assign(ELLIPSIS);

    
    
    nscoord ellipsisWidth;
    aRenderingContext.SetTextRunRTL(PR_FALSE);
    aRenderingContext.GetWidth(ELLIPSIS, ellipsisWidth);

    if (ellipsisWidth > aWidth) {
        mCroppedTitle.SetLength(0);
        mTitleWidth = aWidth;
        return;
    }

    
    if (ellipsisWidth == aWidth) {
        mTitleWidth = aWidth;
        return;
    }

    aWidth -= ellipsisWidth;

    
    
    
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
                return;

            
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
                break;

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

            
            nsAutoString ellipsisString;
            ellipsisString.Assign(ELLIPSIS);

            mCroppedTitle = leftString + ellipsisString + rightString;
        }
        break;
    }

    mTitleWidth = nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                                mCroppedTitle.get(), mCroppedTitle.Length());
}



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

    PRInt32 offset = mTitle.RFind("...");
    if (offset == kNotFound) {
        offset = (PRInt32)mTitle.Length();
        if (mTitle.Last() == PRUnichar(':'))
            offset--;
    }

    if (InsertSeparatorBeforeAccessKey() && offset > 0 &&
        !NS_IS_SPACE(mTitle[offset - 1])) {
        mTitle.Insert(' ', (PRUint32)offset);
        offset++;
    }

    mTitle.Insert(accessKeyLabel, (PRUint32)offset);
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
    mState |= NS_STATE_NEED_LAYOUT;

    return nsLeafBoxFrame::DoLayout(aBoxLayoutState);
}

 void
nsTextBoxFrame::MarkIntrinsicWidthsDirty()
{
    mNeedsRecalc = PR_TRUE;
    nsTextBoxFrameSuper::MarkIntrinsicWidthsDirty();
}

void
nsTextBoxFrame::GetTextSize(nsPresContext* aPresContext, nsIRenderingContext& aRenderingContext,
                                const nsString& aString, nsSize& aSize, nscoord& aAscent)
{
    nsCOMPtr<nsIFontMetrics> fontMet;
    aPresContext->DeviceContext()->GetMetricsFor(GetStyleFont()->mFont,
                                                 *getter_AddRefs(fontMet));
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
        nsIRenderingContext* rendContext = aBoxLayoutState.GetRenderingContext();
        if (rendContext) {
            GetTextSize(presContext, *rendContext,
                        mTitle, size, mAscent);
            mTextSize = size;
            mNeedsRecalc = PR_FALSE;
        }
    }
}




nsSize
nsTextBoxFrame::GetPrefSize(nsBoxLayoutState& aBoxLayoutState)
{
    CalcTextSize(aBoxLayoutState);

    nsSize size = mTextSize;
    DISPLAY_PREF_SIZE(this, size);

    AddBorderAndPadding(size);
    nsIBox::AddCSSPrefSize(aBoxLayoutState, this, size);

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
    nsIBox::AddCSSMinSize(aBoxLayoutState, this, size);

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
