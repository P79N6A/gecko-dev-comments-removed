




#include "nsTextBoxFrame.h"

#include "nsReadableUtils.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsNameSpaceManager.h"
#include "nsBoxLayoutState.h"
#include "nsMenuBarListener.h"
#include "nsXPIDLString.h"
#include "nsIServiceManager.h"
#include "nsIDOMElement.h"
#include "nsIDOMXULLabelElement.h"
#include "mozilla/EventStateManager.h"
#include "nsITheme.h"
#include "nsUnicharUtils.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsCSSRendering.h"
#include "nsIReflowCallback.h"
#include "nsBoxFrame.h"
#include "mozilla/Preferences.h"
#include "nsLayoutUtils.h"
#include "mozilla/Attributes.h"

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#include "nsBidiUtils.h"
#include "nsBidiPresUtils.h"

using namespace mozilla;

class nsAccessKeyInfo
{
public:
    int32_t mAccesskeyIndex;
    nscoord mBeforeWidth, mAccessWidth, mAccessUnderlineSize, mAccessOffset;
};


bool nsTextBoxFrame::gAlwaysAppendAccessKey          = false;
bool nsTextBoxFrame::gAccessKeyPrefInitialized       = false;
bool nsTextBoxFrame::gInsertSeparatorBeforeAccessKey = false;
bool nsTextBoxFrame::gInsertSeparatorPrefInitialized = false;

nsIFrame*
NS_NewTextBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
    return new (aPresShell) nsTextBoxFrame (aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTextBoxFrame)

NS_QUERYFRAME_HEAD(nsTextBoxFrame)
  NS_QUERYFRAME_ENTRY(nsTextBoxFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsTextBoxFrameSuper)

nsresult
nsTextBoxFrame::AttributeChanged(int32_t         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 int32_t         aModType)
{
    bool aResize;
    bool aRedraw;

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
        RegUnregAccessKey(true);

    return NS_OK;
}

nsTextBoxFrame::nsTextBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsLeafBoxFrame(aShell, aContext), mAccessKeyInfo(nullptr), mCropType(CropRight),
  mNeedsReflowCallback(false)
{
    MarkIntrinsicISizesDirty();
}

nsTextBoxFrame::~nsTextBoxFrame()
{
    delete mAccessKeyInfo;
}


void
nsTextBoxFrame::Init(nsIContent*       aContent,
                     nsContainerFrame* aParent,
                     nsIFrame*         aPrevInFlow)
{
    nsTextBoxFrameSuper::Init(aContent, aParent, aPrevInFlow);

    bool aResize;
    bool aRedraw;
    UpdateAttributes(nullptr, aResize, aRedraw); 

    
    RegUnregAccessKey(true);
}

void
nsTextBoxFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
    
    RegUnregAccessKey(false);
    nsTextBoxFrameSuper::DestroyFrom(aDestructRoot);
}

bool
nsTextBoxFrame::AlwaysAppendAccessKey()
{
  if (!gAccessKeyPrefInitialized) 
  {
    gAccessKeyPrefInitialized = true;

    const char* prefName = "intl.menuitems.alwaysappendaccesskeys";
    nsAdoptingString val = Preferences::GetLocalizedString(prefName);
    gAlwaysAppendAccessKey = val.EqualsLiteral("true");
  }
  return gAlwaysAppendAccessKey;
}

bool
nsTextBoxFrame::InsertSeparatorBeforeAccessKey()
{
  if (!gInsertSeparatorPrefInitialized)
  {
    gInsertSeparatorPrefInitialized = true;

    const char* prefName = "intl.menuitems.insertseparatorbeforeaccesskeys";
    nsAdoptingString val = Preferences::GetLocalizedString(prefName);
    gInsertSeparatorBeforeAccessKey = val.EqualsLiteral("true");
  }
  return gInsertSeparatorBeforeAccessKey;
}

class nsAsyncAccesskeyUpdate MOZ_FINAL : public nsIReflowCallback
{
public:
    explicit nsAsyncAccesskeyUpdate(nsIFrame* aFrame) : mWeakFrame(aFrame)
    {
    }

    virtual bool ReflowFinished() MOZ_OVERRIDE
    {
        bool shouldFlush = false;
        nsTextBoxFrame* frame =
            static_cast<nsTextBoxFrame*>(mWeakFrame.GetFrame());
        if (frame) {
            shouldFlush = frame->UpdateAccesskey(mWeakFrame);
        }
        delete this;
        return shouldFlush;
    }

    virtual void ReflowCallbackCanceled() MOZ_OVERRIDE
    {
        delete this;
    }

    nsWeakFrame mWeakFrame;
};

bool
nsTextBoxFrame::UpdateAccesskey(nsWeakFrame& aWeakThis)
{
    nsAutoString accesskey;
    nsCOMPtr<nsIDOMXULLabelElement> labelElement = do_QueryInterface(mContent);
    NS_ENSURE_TRUE(aWeakThis.IsAlive(), false);
    if (labelElement) {
        
        labelElement->GetAccessKey(accesskey);
        NS_ENSURE_TRUE(aWeakThis.IsAlive(), false);
    }
    else {
        mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accesskey);
    }

    if (!accesskey.Equals(mAccessKey)) {
        
        RecomputeTitle();
        mAccessKey = accesskey;
        UpdateAccessTitle();
        PresContext()->PresShell()->
            FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                             NS_FRAME_IS_DIRTY);
        return true;
    }
    return false;
}

void
nsTextBoxFrame::UpdateAttributes(nsIAtom*         aAttribute,
                                 bool&          aResize,
                                 bool&          aRedraw)
{
    bool doUpdateTitle = false;
    aResize = false;
    aRedraw = false;

    if (aAttribute == nullptr || aAttribute == nsGkAtoms::crop) {
        static nsIContent::AttrValuesArray strings[] =
          {&nsGkAtoms::left, &nsGkAtoms::start, &nsGkAtoms::center,
           &nsGkAtoms::right, &nsGkAtoms::end, nullptr};
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
            aResize = true;
            mCropType = cropType;
        }
    }

    if (aAttribute == nullptr || aAttribute == nsGkAtoms::value) {
        RecomputeTitle();
        doUpdateTitle = true;
    }

    if (aAttribute == nullptr || aAttribute == nsGkAtoms::accesskey) {
        mNeedsReflowCallback = true;
        
        aResize = true;
    }

    if (doUpdateTitle) {
        UpdateAccessTitle();
        aResize = true;
    }

}

class nsDisplayXULTextBox : public nsDisplayItem {
public:
  nsDisplayXULTextBox(nsDisplayListBuilder* aBuilder,
                      nsTextBoxFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame),
    mDisableSubpixelAA(false)
  {
    MOZ_COUNT_CTOR(nsDisplayXULTextBox);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULTextBox() {
    MOZ_COUNT_DTOR(nsDisplayXULTextBox);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("XULTextBox", TYPE_XUL_TEXT_BOX)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  virtual void DisableComponentAlpha() MOZ_OVERRIDE {
    mDisableSubpixelAA = true;
  }

  void PaintTextToContext(nsRenderingContext* aCtx,
                          nsPoint aOffset,
                          const nscolor* aColor);

  bool mDisableSubpixelAA;
};

static void
PaintTextShadowCallback(nsRenderingContext* aCtx,
                        nsPoint aShadowOffset,
                        const nscolor& aShadowColor,
                        void* aData)
{
  reinterpret_cast<nsDisplayXULTextBox*>(aData)->
           PaintTextToContext(aCtx, aShadowOffset, &aShadowColor);
}

void
nsDisplayXULTextBox::Paint(nsDisplayListBuilder* aBuilder,
                           nsRenderingContext* aCtx)
{
  gfxContextAutoDisableSubpixelAntialiasing disable(aCtx->ThebesContext(),
                                                    mDisableSubpixelAA);

  
  nsRect drawRect = static_cast<nsTextBoxFrame*>(mFrame)->mTextDrawRect +
                    ToReferenceFrame();
  nsLayoutUtils::PaintTextShadow(mFrame, aCtx,
                                 drawRect, mVisibleRect,
                                 mFrame->StyleColor()->mColor,
                                 PaintTextShadowCallback,
                                 (void*)this);

  PaintTextToContext(aCtx, nsPoint(0, 0), nullptr);
}

void
nsDisplayXULTextBox::PaintTextToContext(nsRenderingContext* aCtx,
                                        nsPoint aOffset,
                                        const nscolor* aColor)
{
  static_cast<nsTextBoxFrame*>(mFrame)->
    PaintTitle(*aCtx, mVisibleRect, ToReferenceFrame() + aOffset, aColor);
}

nsRect
nsDisplayXULTextBox::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
  *aSnap = false;
  return mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
}

nsRect
nsDisplayXULTextBox::GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
{
  return static_cast<nsTextBoxFrame*>(mFrame)->GetComponentAlphaBounds() +
      ToReferenceFrame();
}

void
nsTextBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists)
{
    if (!IsVisibleForPainting(aBuilder))
        return;

    nsLeafBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
    
    aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayXULTextBox(aBuilder, this));
}

void
nsTextBoxFrame::PaintTitle(nsRenderingContext& aRenderingContext,
                           const nsRect&        aDirtyRect,
                           nsPoint              aPt,
                           const nscolor*       aOverrideColor)
{
    if (mTitle.IsEmpty())
        return;

    DrawText(aRenderingContext, aDirtyRect, mTextDrawRect + aPt, aOverrideColor);
}

void
nsTextBoxFrame::DrawText(nsRenderingContext& aRenderingContext,
                         const nsRect&       aDirtyRect,
                         const nsRect&       aTextRect,
                         const nscolor*      aOverrideColor)
{
    nsPresContext* presContext = PresContext();

    
    nscolor overColor;
    nscolor underColor;
    nscolor strikeColor;
    uint8_t overStyle;
    uint8_t underStyle;
    uint8_t strikeStyle;

    
    uint8_t decorations = NS_STYLE_TEXT_DECORATION_LINE_NONE;
    
    uint8_t decorMask = NS_STYLE_TEXT_DECORATION_LINE_LINES_MASK;

    nsIFrame* f = this;
    do {  
      nsStyleContext* context = f->StyleContext();
      if (!context->HasTextDecorationLines()) {
        break;
      }
      const nsStyleTextReset* styleText = context->StyleTextReset();
      
      if (decorMask & styleText->mTextDecorationLine) {  
        nscolor color;
        if (aOverrideColor) {
          color = *aOverrideColor;
        } else {
          bool isForeground;
          styleText->GetDecorationColor(color, isForeground);
          if (isForeground) {
            color = nsLayoutUtils::GetColor(f, eCSSProperty_color);
          }
        }
        uint8_t style = styleText->GetDecorationStyle();

        if (NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE & decorMask &
              styleText->mTextDecorationLine) {
          underColor = color;
          underStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE;
          decorations |= NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_LINE_OVERLINE & decorMask &
              styleText->mTextDecorationLine) {
          overColor = color;
          overStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_OVERLINE;
          decorations |= NS_STYLE_TEXT_DECORATION_LINE_OVERLINE;
        }
        if (NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH & decorMask &
              styleText->mTextDecorationLine) {
          strikeColor = color;
          strikeStyle = style;
          decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH;
          decorations |= NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH;
        }
      }
    } while (0 != decorMask &&
             (f = nsLayoutUtils::GetParentOrPlaceholderFor(f)));

    nsRefPtr<nsFontMetrics> fontMet;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));

    nscoord offset;
    nscoord size;
    nscoord ascent = fontMet->MaxAscent();

    nscoord baseline =
      presContext->RoundAppUnitsToNearestDevPixels(aTextRect.y + ascent);
    nsRefPtr<gfxContext> ctx = aRenderingContext.ThebesContext();
    gfxPoint pt(presContext->AppUnitsToGfxUnits(aTextRect.x),
                presContext->AppUnitsToGfxUnits(aTextRect.y));
    gfxFloat width = presContext->AppUnitsToGfxUnits(aTextRect.width);
    gfxFloat ascentPixel = presContext->AppUnitsToGfxUnits(ascent);
    gfxFloat xInFrame = PresContext()->AppUnitsToGfxUnits(mTextDrawRect.x);
    gfxRect dirtyRect(presContext->AppUnitsToGfxUnits(aDirtyRect));

    
    
    
    
    
    if (decorations & (NS_FONT_DECORATION_OVERLINE |
                       NS_FONT_DECORATION_UNDERLINE)) {
      fontMet->GetUnderline(offset, size);
      gfxFloat offsetPixel = presContext->AppUnitsToGfxUnits(offset);
      gfxFloat sizePixel = presContext->AppUnitsToGfxUnits(size);
      if ((decorations & NS_FONT_DECORATION_UNDERLINE) &&
          underStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
        nsCSSRendering::PaintDecorationLine(this, ctx, dirtyRect, underColor,
                          pt, xInFrame, gfxSize(width, sizePixel),
                          ascentPixel, offsetPixel,
                          NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE, underStyle);
      }
      if ((decorations & NS_FONT_DECORATION_OVERLINE) &&
          overStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
        nsCSSRendering::PaintDecorationLine(this, ctx, dirtyRect, overColor,
                          pt, xInFrame, gfxSize(width, sizePixel),
                          ascentPixel, ascentPixel,
                          NS_STYLE_TEXT_DECORATION_LINE_OVERLINE, overStyle);
      }
    }

    nsRefPtr<nsRenderingContext> refContext =
        PresContext()->PresShell()->CreateReferenceRenderingContext();

    aRenderingContext.SetFont(fontMet);
    refContext->SetFont(fontMet);

    CalculateUnderline(*refContext);

    aRenderingContext.SetColor(aOverrideColor ? *aOverrideColor : StyleColor()->mColor);

    nsresult rv = NS_ERROR_FAILURE;

    if (mState & NS_FRAME_IS_BIDI) {
      presContext->SetBidiEnabled();
      nsBidiLevel level = nsBidiPresUtils::BidiLevelFromStyle(StyleContext());
      if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
          
          
          nsBidiPositionResolve posResolve;
          posResolve.logicalIndex = mAccessKeyInfo->mAccesskeyIndex;
          rv = nsBidiPresUtils::RenderText(mCroppedTitle.get(), mCroppedTitle.Length(), level,
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
          rv = nsBidiPresUtils::RenderText(mCroppedTitle.get(), mCroppedTitle.Length(), level,
                                           presContext, aRenderingContext,
                                           *refContext,
                                           aTextRect.x, baseline);
      }
    }
    if (NS_FAILED(rv)) {
       aRenderingContext.SetTextRunRTL(false);

       if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
           
           
           
           if (mAccessKeyInfo->mAccesskeyIndex > 0)
               mAccessKeyInfo->mBeforeWidth =
                   refContext->GetWidth(mCroppedTitle.get(),
                                        mAccessKeyInfo->mAccesskeyIndex);
           else
               mAccessKeyInfo->mBeforeWidth = 0;
       }

       fontMet->DrawString(mCroppedTitle.get(), mCroppedTitle.Length(),
                           aTextRect.x, baseline, &aRenderingContext,
                           refContext.get());
    }

    if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
        aRenderingContext.FillRect(aTextRect.x + mAccessKeyInfo->mBeforeWidth,
                                   aTextRect.y + mAccessKeyInfo->mAccessOffset,
                                   mAccessKeyInfo->mAccessWidth,
                                   mAccessKeyInfo->mAccessUnderlineSize);
    }

    
    
    if ((decorations & NS_FONT_DECORATION_LINE_THROUGH) &&
        strikeStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
      fontMet->GetStrikeout(offset, size);
      gfxFloat offsetPixel = presContext->AppUnitsToGfxUnits(offset);
      gfxFloat sizePixel = presContext->AppUnitsToGfxUnits(size);
      nsCSSRendering::PaintDecorationLine(this, ctx, dirtyRect, strikeColor,
                        pt, xInFrame, gfxSize(width, sizePixel), ascentPixel,
                        offsetPixel, NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH,
                        strikeStyle);
    }
}

void
nsTextBoxFrame::CalculateUnderline(nsRenderingContext& aRenderingContext)
{
    if (mAccessKeyInfo && mAccessKeyInfo->mAccesskeyIndex != kNotFound) {
         
         
         const char16_t *titleString = mCroppedTitle.get();
         aRenderingContext.SetTextRunRTL(false);
         mAccessKeyInfo->mAccessWidth =
             aRenderingContext.GetWidth(titleString[mAccessKeyInfo->
                                                    mAccesskeyIndex]);

         nscoord offset, baseline;
         nsFontMetrics* metrics = aRenderingContext.FontMetrics();
         metrics->GetUnderline(offset, mAccessKeyInfo->mAccessUnderlineSize);
         baseline = metrics->MaxAscent();
         mAccessKeyInfo->mAccessOffset = baseline - offset;
    }
}

nscoord
nsTextBoxFrame::CalculateTitleForWidth(nsPresContext*      aPresContext,
                                       nsRenderingContext& aRenderingContext,
                                       nscoord              aWidth)
{
    if (mTitle.IsEmpty()) {
        mCroppedTitle.Truncate();
        return 0;
    }

    nsRefPtr<nsFontMetrics> fm;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
    aRenderingContext.SetFont(fm);

    
    nscoord titleWidth = nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                                       mTitle.get(), mTitle.Length());

    if (titleWidth <= aWidth) {
        mCroppedTitle = mTitle;
        if (HasRTLChars(mTitle)) {
            mState |= NS_FRAME_IS_BIDI;
        }
        return titleWidth;  
    }

    const nsDependentString& kEllipsis = nsContentUtils::GetLocalizedEllipsis();
    
    mCroppedTitle.Assign(kEllipsis);

    
    
    aRenderingContext.SetTextRunRTL(false);
    titleWidth = aRenderingContext.GetWidth(kEllipsis);

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
                char16_t ch = mTitle.CharAt(i);
                
                cwidth = aRenderingContext.GetWidth(ch);
                if (twidth + cwidth > aWidth)
                    break;

                twidth += cwidth;
                if (UCS2_CHAR_IS_BIDI(ch) ) {
                  mState |= NS_FRAME_IS_BIDI;
                }
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
                char16_t ch = mTitle.CharAt(i);
                cwidth = aRenderingContext.GetWidth(ch);
                if (twidth + cwidth > aWidth)
                    break;

                twidth += cwidth;
                if (UCS2_CHAR_IS_BIDI(ch) ) {
                  mState |= NS_FRAME_IS_BIDI;
                }
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
            char16_t ch;
            int leftPos, rightPos;
            nsAutoString leftString, rightString;

            rightPos = mTitle.Length() - 1;
            aRenderingContext.SetTextRunRTL(false);
            for (leftPos = 0; leftPos <= rightPos;) {
                
                ch = mTitle.CharAt(leftPos);
                charWidth = aRenderingContext.GetWidth(ch);
                totalWidth += charWidth;
                if (totalWidth > aWidth)
                    
                    break;
                leftString.Insert(ch, leftString.Length());

                if (UCS2_CHAR_IS_BIDI(ch))
                    mState |= NS_FRAME_IS_BIDI;

                
                if (rightPos > leftPos) {
                    
                    ch = mTitle.CharAt(rightPos);
                    charWidth = aRenderingContext.GetWidth(ch);
                    totalWidth += charWidth;
                    if (totalWidth > aWidth)
                        
                        break;
                    rightString.Insert(ch, 0);

                    if (UCS2_CHAR_IS_BIDI(ch))
                        mState |= NS_FRAME_IS_BIDI;
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
    





    int32_t menuAccessKey;
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
    uint32_t offset = mTitle.Length();
    if (StringEndsWith(mTitle, kEllipsis)) {
        offset -= kEllipsis.Length();
    } else if (StringEndsWith(mTitle, OLD_ELLIPSIS)) {
        
        offset -= OLD_ELLIPSIS.Length();
    } else {
        
        
        const char16_t kLastChar = mTitle.Last();
        if (kLastChar == char16_t(0x2026) || kLastChar == char16_t(':'))
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
    int32_t menuAccessKey;
    nsMenuBarListener::GetMenuAccessKey(&menuAccessKey);
    if (menuAccessKey) {
        if (mAccessKey.IsEmpty()) {
            if (mAccessKeyInfo) {
                delete mAccessKeyInfo;
                mAccessKeyInfo = nullptr;
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

            bool found;
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

void
nsTextBoxFrame::RecomputeTitle()
{
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, mTitle);

  
  
  uint8_t textTransform = StyleText()->mTextTransform;
  if (textTransform == NS_STYLE_TEXT_TRANSFORM_UPPERCASE) {
    ToUpperCase(mTitle);
  } else if (textTransform == NS_STYLE_TEXT_TRANSFORM_LOWERCASE) {
    ToLowerCase(mTitle);
  }
  
  
  
}

void
nsTextBoxFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (!aOldStyleContext) {
    
    return;
  }

  const nsStyleText* oldTextStyle = aOldStyleContext->PeekStyleText();
  
  
  
  if (!oldTextStyle ||
      oldTextStyle->mTextTransform != StyleText()->mTextTransform) {
    RecomputeTitle();
    UpdateAccessTitle();
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
        mNeedsReflowCallback = false;
    }

    nsresult rv = nsLeafBoxFrame::DoLayout(aBoxLayoutState);

    CalcDrawRect(*aBoxLayoutState.GetRenderingContext());

    const nsStyleText* textStyle = StyleText();
    
    nsRect scrollBounds(nsPoint(0, 0), GetSize());
    nsRect textRect = mTextDrawRect;
    
    nsRefPtr<nsFontMetrics> fontMet;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));
    nsBoundingMetrics metrics = 
      fontMet->GetInkBoundsForVisualOverflow(mCroppedTitle.get(),
                                             mCroppedTitle.Length(),
                                             aBoxLayoutState.GetRenderingContext());

    textRect.x -= metrics.leftBearing;
    textRect.width = metrics.width;
    
    textRect.y += fontMet->MaxAscent() - metrics.ascent;
    textRect.height = metrics.ascent + metrics.descent;

    
    
    nsRect visualBounds;
    visualBounds.UnionRect(scrollBounds, textRect);
    nsOverflowAreas overflow(visualBounds, scrollBounds);

    if (textStyle->mTextShadow) {
      
      nsRect &vis = overflow.VisualOverflow();
      vis.UnionRect(vis, nsLayoutUtils::GetTextShadowRectsUnion(mTextDrawRect, this));
    }
    FinishAndStoreOverflow(overflow, GetSize());

    return rv;
}

nsRect
nsTextBoxFrame::GetComponentAlphaBounds()
{
  if (StyleText()->mTextShadow) {
    return GetVisualOverflowRectRelativeToSelf();
  }
  return mTextDrawRect;
}

bool
nsTextBoxFrame::ComputesOwnOverflowArea()
{
    return true;
}

 void
nsTextBoxFrame::MarkIntrinsicISizesDirty()
{
    mNeedsRecalc = true;
    nsTextBoxFrameSuper::MarkIntrinsicISizesDirty();
}

void
nsTextBoxFrame::GetTextSize(nsPresContext* aPresContext,
                            nsRenderingContext& aRenderingContext,
                            const nsString& aString,
                            nsSize& aSize, nscoord& aAscent)
{
    nsRefPtr<nsFontMetrics> fontMet;
    nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet));
    aSize.height = fontMet->MaxHeight();
    aRenderingContext.SetFont(fontMet);
    aSize.width =
      nsLayoutUtils::GetStringWidth(this, &aRenderingContext,
                                    aString.get(), aString.Length());
    aAscent = fontMet->MaxAscent();
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
            mNeedsRecalc = false;
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

#ifdef ACCESSIBILITY
    
    
    nsAccessibilityService* accService = GetAccService();
    if (accService) {
        accService->UpdateLabelValue(PresContext()->PresShell(), mContent,
                                     mCroppedTitle);
    }
#endif

    
    UpdateAccessIndex();

    
    nscoord outerWidth = textRect.width;
    textRect.width = titleWidth;

    
    const nsStyleVisibility* vis = StyleVisibility();
    const nsStyleText* textStyle = StyleText();

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
    bool widthSet, heightSet;
    nsIFrame::AddCSSPrefSize(this, size, widthSet, heightSet);

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
    bool widthSet, heightSet;
    nsIFrame::AddCSSMinSize(aBoxLayoutState, this, size, widthSet, heightSet);

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

#ifdef DEBUG_FRAME_DUMP
nsresult
nsTextBoxFrame::GetFrameName(nsAString& aResult) const
{
    MakeFrameName(NS_LITERAL_STRING("TextBox"), aResult);
    aResult += NS_LITERAL_STRING("[value=") + mTitle + NS_LITERAL_STRING("]");
    return NS_OK;
}
#endif



nsresult
nsTextBoxFrame::RegUnregAccessKey(bool aDoReg)
{
    
    if (!mContent)
        return NS_ERROR_FAILURE;

    
    
    

    
    
    
    
    if (!mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::control))
        return NS_OK;

    
    nsAutoString accessKey;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accesskey, accessKey);

    if (accessKey.IsEmpty())
        return NS_OK;

    
    
    EventStateManager* esm = PresContext()->EventStateManager();

    uint32_t key = accessKey.First();
    if (aDoReg)
        esm->RegisterAccessKey(mContent, key);
    else
        esm->UnregisterAccessKey(mContent, key);

    return NS_OK;
}
