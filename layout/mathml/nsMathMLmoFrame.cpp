




#include "nsMathMLmoFrame.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsContentUtils.h"
#include "nsFrameSelection.h"
#include "nsMathMLElement.h"
#include <algorithm>






#define NS_MATHML_CHAR_STYLE_CONTEXT_INDEX   0

nsIFrame*
NS_NewMathMLmoFrame(nsIPresShell* aPresShell, nsStyleContext *aContext)
{
  return new (aPresShell) nsMathMLmoFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMathMLmoFrame)

nsMathMLmoFrame::~nsMathMLmoFrame()
{
}

static const char16_t kApplyFunction  = char16_t(0x2061);
static const char16_t kInvisibleTimes = char16_t(0x2062);
static const char16_t kInvisibleSeparator = char16_t(0x2063);
static const char16_t kInvisiblePlus = char16_t(0x2064);

eMathMLFrameType
nsMathMLmoFrame::GetMathMLFrameType()
{
  return NS_MATHML_OPERATOR_IS_INVISIBLE(mFlags)
    ? eMathMLFrameType_OperatorInvisible
    : eMathMLFrameType_OperatorOrdinary;
}




bool
nsMathMLmoFrame::IsFrameInSelection(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "null arg");
  if (!aFrame || !aFrame->IsSelected())
    return false;

  const nsFrameSelection* frameSelection = aFrame->GetConstFrameSelection();
  SelectionDetails* details =
    frameSelection->LookUpSelection(aFrame->GetContent(), 0, 1, true);

  if (!details)
    return false;

  while (details) {
    SelectionDetails* next = details->mNext;
    delete details;
    details = next;
  }
  return true;
}

bool
nsMathMLmoFrame::UseMathMLChar()
{
  return (NS_MATHML_OPERATOR_GET_FORM(mFlags) &&
          NS_MATHML_OPERATOR_IS_MUTABLE(mFlags)) ||
    NS_MATHML_OPERATOR_IS_CENTERED(mFlags);
}

void
nsMathMLmoFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  bool useMathMLChar = UseMathMLChar();

  if (!useMathMLChar) {
    
    nsMathMLTokenFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  } else {
    DisplayBorderBackgroundOutline(aBuilder, aLists);
    
    
    bool isSelected = false;
    nsRect selectedRect;
    nsIFrame* firstChild = mFrames.FirstChild();
    if (IsFrameInSelection(firstChild)) {
      mMathMLChar.GetRect(selectedRect);
      
      selectedRect.Inflate(nsPresContext::CSSPixelsToAppUnits(1));
      isSelected = true;
    }
    mMathMLChar.Display(aBuilder, this, aLists, 0, isSelected ? &selectedRect : nullptr);
  
#if defined(DEBUG) && defined(SHOW_BOUNDING_BOX)
    
    DisplayBoundingMetrics(aBuilder, this, mReference, mBoundingMetrics, aLists);
#endif
  }
}


void
nsMathMLmoFrame::ProcessTextData()
{
  mFlags = 0;

  nsAutoString data;
  if (!nsContentUtils::GetNodeTextContent(mContent, false, data)) {
    NS_RUNTIMEABORT("OOM");
  }

  data.CompressWhitespace();
  int32_t length = data.Length();
  char16_t ch = (length == 0) ? char16_t('\0') : data[0];

  if ((length == 1) && 
      (ch == kApplyFunction  ||
       ch == kInvisibleSeparator ||
       ch == kInvisiblePlus ||
       ch == kInvisibleTimes)) {
    mFlags |= NS_MATHML_OPERATOR_INVISIBLE;
  }

  
  nsPresContext* presContext = PresContext();
  if (mFrames.GetLength() != 1) {
    data.Truncate(); 
    mMathMLChar.SetData(presContext, data);
    ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mMathMLChar);
    return;
  }

  
  
  
  
  if (1 == length && ch == '-') {
    ch = 0x2212;
    data = ch;
  }

  
  
  

  
  
  nsOperatorFlags flags[4];
  float lspace[4], rspace[4];
  nsMathMLOperators::LookupOperators(data, flags, lspace, rspace);
  nsOperatorFlags allFlags =
    flags[NS_MATHML_OPERATOR_FORM_INFIX] |
    flags[NS_MATHML_OPERATOR_FORM_POSTFIX] |
    flags[NS_MATHML_OPERATOR_FORM_PREFIX];

  mFlags |= allFlags & NS_MATHML_OPERATOR_ACCENT;
  mFlags |= allFlags & NS_MATHML_OPERATOR_MOVABLELIMITS;

  
  
  if (1 == length) {
    if ((ch == '+') || (ch == '=') || (ch == '*') ||
        (ch == 0x2212) || 
        (ch == 0x2264) || 
        (ch == 0x2265) || 
        (ch == 0x00D7)) { 
      mFlags |= NS_MATHML_OPERATOR_CENTERED;
    }
  }

  
  mMathMLChar.SetData(presContext, data);

  
  
  
  mEmbellishData.direction = mMathMLChar.GetStretchDirection();

  bool isMutable =
    NS_MATHML_OPERATOR_IS_LARGEOP(allFlags) ||
    (mEmbellishData.direction != NS_STRETCH_DIRECTION_UNSUPPORTED);
  if (isMutable)
    mFlags |= NS_MATHML_OPERATOR_MUTABLE;

  ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mMathMLChar);
}






void
nsMathMLmoFrame::ProcessOperatorData()
{
  
  nsOperatorFlags form = NS_MATHML_OPERATOR_GET_FORM(mFlags);
  nsAutoString value;
  float fontSizeInflation = nsLayoutUtils::FontSizeInflationFor(this);

  
  
  
  
  
  
  
  mFlags &= NS_MATHML_OPERATOR_MUTABLE |
            NS_MATHML_OPERATOR_ACCENT | 
            NS_MATHML_OPERATOR_MOVABLELIMITS |
            NS_MATHML_OPERATOR_CENTERED |
            NS_MATHML_OPERATOR_INVISIBLE;

  if (!mEmbellishData.coreFrame) {
    
    form = NS_MATHML_OPERATOR_FORM_INFIX;

    
    
    mEmbellishData.flags = 0;
    mEmbellishData.coreFrame = nullptr;
    mEmbellishData.leadingSpace = 0;
    mEmbellishData.trailingSpace = 0;
    if (mMathMLChar.Length() != 1)
      mEmbellishData.direction = NS_STRETCH_DIRECTION_UNSUPPORTED;  
    

    if (!mFrames.FirstChild()) {
      return;
    }

    mEmbellishData.flags |= NS_MATHML_EMBELLISH_OPERATOR;
    mEmbellishData.coreFrame = this;

    
    
    
    

    
    
    

    
    
    if (NS_MATHML_OPERATOR_IS_ACCENT(mFlags))
      mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENT;
    if (NS_MATHML_OPERATOR_IS_MOVABLELIMITS(mFlags))
      mEmbellishData.flags |= NS_MATHML_EMBELLISH_MOVABLELIMITS;

    
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::accent_, value);
    if (value.EqualsLiteral("true"))
      mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENT;
    else if (value.EqualsLiteral("false"))
      mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENT;

    
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::movablelimits_, value);
    if (value.EqualsLiteral("true"))
      mEmbellishData.flags |= NS_MATHML_EMBELLISH_MOVABLELIMITS;
    else if (value.EqualsLiteral("false"))
      mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_MOVABLELIMITS;

     
     
     
     mFlags |= form;
     return;
  }

  nsPresContext* presContext = PresContext();

  
  
  
  
  
  
  if (form) {
    
    
    nsIFrame* embellishAncestor = this;
    nsEmbellishData embellishData;
    nsIFrame* parentAncestor = this;
    do {
      embellishAncestor = parentAncestor;
      parentAncestor = embellishAncestor->GetParent();
      GetEmbellishDataFrom(parentAncestor, embellishData);
    } while (embellishData.coreFrame == this);

    
    if (embellishAncestor != this)
      mFlags |= NS_MATHML_OPERATOR_EMBELLISH_ANCESTOR;
    else
      mFlags &= ~NS_MATHML_OPERATOR_EMBELLISH_ANCESTOR;

    
    

    nsIFrame* nextSibling = embellishAncestor->GetNextSibling();
    nsIFrame* prevSibling = embellishAncestor->GetPrevSibling();

    
    
    nsIMathMLFrame* mathAncestor = do_QueryFrame(parentAncestor);
    bool zeroSpacing = false;
    if (mathAncestor) {
      zeroSpacing =  !mathAncestor->IsMrowLike();
    } else {
      nsMathMLmathBlockFrame* blockFrame = do_QueryFrame(parentAncestor);
      if (blockFrame) {
        zeroSpacing = !blockFrame->IsMrowLike();
      }
    }
    if (zeroSpacing) {
      mFlags |= NS_MATHML_OPERATOR_EMBELLISH_ISOLATED;
    } else {
      mFlags &= ~NS_MATHML_OPERATOR_EMBELLISH_ISOLATED;
    }

    
    form = NS_MATHML_OPERATOR_FORM_INFIX;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::form, value);
    if (!value.IsEmpty()) {
      if (value.EqualsLiteral("prefix"))
        form = NS_MATHML_OPERATOR_FORM_PREFIX;
      else if (value.EqualsLiteral("postfix"))
        form = NS_MATHML_OPERATOR_FORM_POSTFIX;
    }
    else {
      
      if (!prevSibling && nextSibling)
        form = NS_MATHML_OPERATOR_FORM_PREFIX;
      else if (prevSibling && !nextSibling)
        form = NS_MATHML_OPERATOR_FORM_POSTFIX;
    }
    mFlags &= ~NS_MATHML_OPERATOR_FORM; 
    mFlags |= form;

    
    
    
    float lspace = 5.0f/18.0f;
    float rspace = 5.0f/18.0f;
    
    nsAutoString data;
    mMathMLChar.GetData(data);
    nsMathMLOperators::LookupOperator(data, form, &mFlags, &lspace, &rspace);
    
    
    if (!NS_MATHML_OPERATOR_EMBELLISH_IS_ISOLATED(mFlags) &&
        (lspace || rspace)) {
      
      
      nscoord em;
      nsRefPtr<nsFontMetrics> fm;
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                            fontSizeInflation);
      GetEmHeight(fm, em);

      mEmbellishData.leadingSpace = NSToCoordRound(lspace * em);
      mEmbellishData.trailingSpace = NSToCoordRound(rspace * em);

      
      
      
      if (StyleFont()->mScriptLevel > 0 &&
          !NS_MATHML_OPERATOR_HAS_EMBELLISH_ANCESTOR(mFlags)) {
        mEmbellishData.leadingSpace /= 2;
        mEmbellishData.trailingSpace /= 2;
      }
    }
  }

  
  

  
  
  
  
  
  
  
  
  
  
  
  
  nscoord leadingSpace = mEmbellishData.leadingSpace;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::lspace_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (nsMathMLElement::ParseNumericValue(value, cssValue, 0,
                                           mContent->OwnerDoc())) {
      if ((eCSSUnit_Number == cssValue.GetUnit()) && !cssValue.GetFloatValue())
        leadingSpace = 0;
      else if (cssValue.IsLengthUnit())
        leadingSpace = CalcLength(presContext, mStyleContext, cssValue,
                                  fontSizeInflation);
      mFlags |= NS_MATHML_OPERATOR_LSPACE_ATTR;
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  nscoord trailingSpace = mEmbellishData.trailingSpace;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rspace_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (nsMathMLElement::ParseNumericValue(value, cssValue, 0,
                                           mContent->OwnerDoc())) {
      if ((eCSSUnit_Number == cssValue.GetUnit()) && !cssValue.GetFloatValue())
        trailingSpace = 0;
      else if (cssValue.IsLengthUnit())
        trailingSpace = CalcLength(presContext, mStyleContext, cssValue,
                                   fontSizeInflation);
      mFlags |= NS_MATHML_OPERATOR_RSPACE_ATTR;
    }
  }

  
  
  if (leadingSpace || trailingSpace) {
    nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
    if (leadingSpace && leadingSpace < onePixel)
      leadingSpace = onePixel;
    if (trailingSpace && trailingSpace < onePixel)
      trailingSpace = onePixel;
  }

  
  mEmbellishData.leadingSpace = leadingSpace;
  mEmbellishData.trailingSpace = trailingSpace;

  
  
  

  
  
  
  

  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::stretchy_, value);
  if (value.EqualsLiteral("false")) {
    mFlags &= ~NS_MATHML_OPERATOR_STRETCHY;
  } else if (value.EqualsLiteral("true")) {
    mFlags |= NS_MATHML_OPERATOR_STRETCHY;
  }
  if (NS_MATHML_OPERATOR_IS_FENCE(mFlags)) {
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::fence_, value);
    if (value.EqualsLiteral("false"))
      mFlags &= ~NS_MATHML_OPERATOR_FENCE;
  }
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::largeop_, value);
  if (value.EqualsLiteral("false")) {
    mFlags &= ~NS_MATHML_OPERATOR_LARGEOP;
  } else if (value.EqualsLiteral("true")) {
    mFlags |= NS_MATHML_OPERATOR_LARGEOP;
  }
  if (NS_MATHML_OPERATOR_IS_SEPARATOR(mFlags)) {
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::separator_, value);
    if (value.EqualsLiteral("false"))
      mFlags &= ~NS_MATHML_OPERATOR_SEPARATOR;
  }
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::symmetric_, value);
  if (value.EqualsLiteral("false"))
    mFlags &= ~NS_MATHML_OPERATOR_SYMMETRIC;
  else if (value.EqualsLiteral("true"))
    mFlags |= NS_MATHML_OPERATOR_SYMMETRIC;


  
  
  
  
  
  
  
  
  
  
  
  
  mMinSize = 0;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::minsize_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (nsMathMLElement::ParseNumericValue(value, cssValue,
                                           nsMathMLElement::
                                           PARSE_ALLOW_UNITLESS,
                                           mContent->OwnerDoc())) {
      nsCSSUnit unit = cssValue.GetUnit();
      if (eCSSUnit_Number == unit)
        mMinSize = cssValue.GetFloatValue();
      else if (eCSSUnit_Percent == unit)
        mMinSize = cssValue.GetPercentValue();
      else if (eCSSUnit_Null != unit) {
        mMinSize = float(CalcLength(presContext, mStyleContext, cssValue,
                                    fontSizeInflation));
        mFlags |= NS_MATHML_OPERATOR_MINSIZE_ABSOLUTE;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  mMaxSize = NS_MATHML_OPERATOR_SIZE_INFINITY;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::maxsize_, value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (nsMathMLElement::ParseNumericValue(value, cssValue,
                                           nsMathMLElement::
                                           PARSE_ALLOW_UNITLESS,
                                           mContent->OwnerDoc())) {
      nsCSSUnit unit = cssValue.GetUnit();
      if (eCSSUnit_Number == unit)
        mMaxSize = cssValue.GetFloatValue();
      else if (eCSSUnit_Percent == unit)
        mMaxSize = cssValue.GetPercentValue();
      else if (eCSSUnit_Null != unit) {
        mMaxSize = float(CalcLength(presContext, mStyleContext, cssValue,
                                    fontSizeInflation));
        mFlags |= NS_MATHML_OPERATOR_MAXSIZE_ABSOLUTE;
      }
    }
  }
}

static uint32_t
GetStretchHint(nsOperatorFlags aFlags, nsPresentationData aPresentationData,
               bool aIsVertical, const nsStyleFont* aStyleFont)
{
  uint32_t stretchHint = NS_STRETCH_NONE;
  
  
  if (NS_MATHML_OPERATOR_IS_MUTABLE(aFlags)) {
    
    
    
    
    
    if (aStyleFont->mMathDisplay == NS_MATHML_DISPLAYSTYLE_BLOCK &&
        NS_MATHML_OPERATOR_IS_LARGEOP(aFlags)) {
      stretchHint = NS_STRETCH_LARGEOP; 
      if (NS_MATHML_OPERATOR_IS_INTEGRAL(aFlags)) {
        stretchHint |= NS_STRETCH_INTEGRAL;
      }
      if (NS_MATHML_OPERATOR_IS_STRETCHY(aFlags)) {
        stretchHint |= NS_STRETCH_NEARER | NS_STRETCH_LARGER;
      }
    }
    else if(NS_MATHML_OPERATOR_IS_STRETCHY(aFlags)) {
      if (aIsVertical) {
        
        stretchHint = NS_STRETCH_NEARER;
      }
      else {
        stretchHint = NS_STRETCH_NORMAL;
      }
    }
    
    
  }
  return stretchHint;
}




NS_IMETHODIMP
nsMathMLmoFrame::Stretch(nsRenderingContext& aRenderingContext,
                         nsStretchDirection   aStretchDirection,
                         nsBoundingMetrics&   aContainerSize,
                         nsHTMLReflowMetrics& aDesiredStretchSize)
{
  if (NS_MATHML_STRETCH_WAS_DONE(mPresentationData.flags)) {
    NS_WARNING("it is wrong to fire stretch more than once on a frame");
    return NS_OK;
  }
  mPresentationData.flags |= NS_MATHML_STRETCH_DONE;

  nsIFrame* firstChild = mFrames.FirstChild();

  
  float fontSizeInflation = nsLayoutUtils::FontSizeInflationFor(this);
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                        fontSizeInflation);
  nscoord axisHeight, height;
  GetAxisHeight(aRenderingContext, fm, axisHeight);

  
  
  nscoord em;
  GetEmHeight(fm, em);
  nscoord leading = NSToCoordRound(0.2f * em);

  
  
  
  bool useMathMLChar = UseMathMLChar();

  nsBoundingMetrics charSize;
  nsBoundingMetrics container = aDesiredStretchSize.mBoundingMetrics;
  bool isVertical = false;

  if (((aStretchDirection == NS_STRETCH_DIRECTION_VERTICAL) ||
       (aStretchDirection == NS_STRETCH_DIRECTION_DEFAULT))  &&
      (mEmbellishData.direction == NS_STRETCH_DIRECTION_VERTICAL)) {
    isVertical = true;
  }

  uint32_t stretchHint =
    GetStretchHint(mFlags, mPresentationData, isVertical, StyleFont());

  if (useMathMLChar) {
    nsBoundingMetrics initialSize = aDesiredStretchSize.mBoundingMetrics;

    if (stretchHint != NS_STRETCH_NONE) {

      container = aContainerSize;

      

      if (isVertical && NS_MATHML_OPERATOR_IS_SYMMETRIC(mFlags)) {
        
        nscoord delta = std::max(container.ascent - axisHeight,
                               container.descent + axisHeight);
        container.ascent = delta + axisHeight;
        container.descent = delta - axisHeight;

        
        delta = std::max(initialSize.ascent - axisHeight,
                       initialSize.descent + axisHeight);
        initialSize.ascent = delta + axisHeight;
        initialSize.descent = delta - axisHeight;
      }

      

      if (mMaxSize != NS_MATHML_OPERATOR_SIZE_INFINITY && mMaxSize > 0.0f) {
        
        
        if (NS_MATHML_OPERATOR_MAXSIZE_IS_ABSOLUTE(mFlags)) {
          
          
          float aspect = mMaxSize / float(initialSize.ascent + initialSize.descent);
          container.ascent =
            std::min(container.ascent, nscoord(initialSize.ascent * aspect));
          container.descent =
            std::min(container.descent, nscoord(initialSize.descent * aspect));
          
          
          container.width =
            std::min(container.width, (nscoord)mMaxSize);
        }
        else { 
          container.ascent =
            std::min(container.ascent, nscoord(initialSize.ascent * mMaxSize));
          container.descent =
            std::min(container.descent, nscoord(initialSize.descent * mMaxSize));
          container.width =
            std::min(container.width, nscoord(initialSize.width * mMaxSize));
        }

        if (isVertical && !NS_MATHML_OPERATOR_IS_SYMMETRIC(mFlags)) {
          
          height = container.ascent + container.descent;
          container.descent = aContainerSize.descent;
          container.ascent = height - container.descent;
        }
      }

      if (mMinSize > 0.0f) {
        
        
        
        if (aStretchDirection != NS_STRETCH_DIRECTION_DEFAULT &&
            aStretchDirection != mEmbellishData.direction) {
          aStretchDirection = NS_STRETCH_DIRECTION_DEFAULT;
          
          
          container = initialSize;
        }
        if (NS_MATHML_OPERATOR_MINSIZE_IS_ABSOLUTE(mFlags)) {
          
          
          float aspect = mMinSize / float(initialSize.ascent + initialSize.descent);
          container.ascent =
            std::max(container.ascent, nscoord(initialSize.ascent * aspect));
          container.descent =
            std::max(container.descent, nscoord(initialSize.descent * aspect));
          container.width =
            std::max(container.width, (nscoord)mMinSize);
        }
        else { 
          container.ascent =
            std::max(container.ascent, nscoord(initialSize.ascent * mMinSize));
          container.descent =
            std::max(container.descent, nscoord(initialSize.descent * mMinSize));
          container.width =
            std::max(container.width, nscoord(initialSize.width * mMinSize));
        }

        if (isVertical && !NS_MATHML_OPERATOR_IS_SYMMETRIC(mFlags)) {
          
          height = container.ascent + container.descent;
          container.descent = aContainerSize.descent;
          container.ascent = height - container.descent;
        }
      }
    }

    
    nsresult res = mMathMLChar.Stretch(PresContext(), aRenderingContext,
                                       fontSizeInflation,
                                       aStretchDirection, container, charSize,
                                       stretchHint,
                                       StyleVisibility()->mDirection);
    if (NS_FAILED(res)) {
      
      
      mFlags &= ~NS_MATHML_OPERATOR_FORM;
      useMathMLChar = false;
    }
  }

  
  
  nsresult rv = Place(aRenderingContext, true, aDesiredStretchSize);
  if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
    
    DidReflowChildren(mFrames.FirstChild());
  }

  if (useMathMLChar) {
    
    mBoundingMetrics = charSize;

    
    
    if (mMathMLChar.GetStretchDirection() != NS_STRETCH_DIRECTION_UNSUPPORTED ||
        NS_MATHML_OPERATOR_IS_CENTERED(mFlags)) {

      bool largeopOnly =
        (NS_STRETCH_LARGEOP & stretchHint) != 0 &&
        (NS_STRETCH_VARIABLE_MASK & stretchHint) == 0;

      if (isVertical || NS_MATHML_OPERATOR_IS_CENTERED(mFlags)) {
        
        
        

        height = mBoundingMetrics.ascent + mBoundingMetrics.descent;
        if (NS_MATHML_OPERATOR_IS_SYMMETRIC(mFlags) ||
            NS_MATHML_OPERATOR_IS_CENTERED(mFlags)) {
          
          
          mBoundingMetrics.descent = height/2 - axisHeight;
        } else if (!largeopOnly) {
          
          mBoundingMetrics.descent = height/2 +
            (container.ascent + container.descent)/2 - container.ascent;
        } 
        mBoundingMetrics.ascent = height - mBoundingMetrics.descent;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  
  

  
  
  bool isAccent =
    NS_MATHML_EMBELLISH_IS_ACCENT(mEmbellishData.flags);
  if (isAccent) {
    nsEmbellishData parentData;
    GetEmbellishDataFrom(GetParent(), parentData);
    isAccent =
       (NS_MATHML_EMBELLISH_IS_ACCENTOVER(parentData.flags) ||
        NS_MATHML_EMBELLISH_IS_ACCENTUNDER(parentData.flags)) &&
       parentData.coreFrame != this;
  }
  if (isAccent && firstChild) {
    
    nscoord dy = aDesiredStretchSize.BlockStartAscent() -
      (mBoundingMetrics.ascent + leading);
    aDesiredStretchSize.SetBlockStartAscent(mBoundingMetrics.ascent + leading);
    aDesiredStretchSize.Height() = aDesiredStretchSize.BlockStartAscent() +
                                   mBoundingMetrics.descent;

    firstChild->SetPosition(firstChild->GetPosition() - nsPoint(0, dy));
  }
  else if (useMathMLChar) {
    nscoord ascent = fm->MaxAscent();
    nscoord descent = fm->MaxDescent();
    aDesiredStretchSize.SetBlockStartAscent(std::max(mBoundingMetrics.ascent + leading, ascent));
    aDesiredStretchSize.Height() = aDesiredStretchSize.BlockStartAscent() +
                                 std::max(mBoundingMetrics.descent + leading, descent);
  }
  aDesiredStretchSize.Width() = mBoundingMetrics.width;
  aDesiredStretchSize.mBoundingMetrics = mBoundingMetrics;
  mReference.x = 0;
  mReference.y = aDesiredStretchSize.BlockStartAscent();
  
  if (useMathMLChar) {
    nscoord dy = aDesiredStretchSize.BlockStartAscent() - mBoundingMetrics.ascent;
    mMathMLChar.SetRect(nsRect(0, dy, charSize.width, charSize.ascent + charSize.descent));
  }

  
  
  
  

  if (!NS_MATHML_OPERATOR_HAS_EMBELLISH_ANCESTOR(mFlags)) {

    
    nscoord leadingSpace = mEmbellishData.leadingSpace;
    if (isAccent && !NS_MATHML_OPERATOR_HAS_LSPACE_ATTR(mFlags)) {
      leadingSpace = 0;
    }
    nscoord trailingSpace = mEmbellishData.trailingSpace;
    if (isAccent && !NS_MATHML_OPERATOR_HAS_RSPACE_ATTR(mFlags)) {
      trailingSpace = 0;
    }

    mBoundingMetrics.width += leadingSpace + trailingSpace;
    aDesiredStretchSize.Width() = mBoundingMetrics.width;
    aDesiredStretchSize.mBoundingMetrics.width = mBoundingMetrics.width;

    nscoord dx = (StyleVisibility()->mDirection ?
                  trailingSpace : leadingSpace);
    if (dx) {
      
      mBoundingMetrics.leftBearing += dx;
      mBoundingMetrics.rightBearing += dx;
      aDesiredStretchSize.mBoundingMetrics.leftBearing += dx;
      aDesiredStretchSize.mBoundingMetrics.rightBearing += dx;

      if (useMathMLChar) {
        nsRect rect;
        mMathMLChar.GetRect(rect);
        mMathMLChar.SetRect(nsRect(rect.x + dx, rect.y,
                                   rect.width, rect.height));
      }
      else {
        nsIFrame* childFrame = firstChild;
        while (childFrame) {
          childFrame->SetPosition(childFrame->GetPosition() +
                                  nsPoint(dx, 0));
          childFrame = childFrame->GetNextSibling();
        }
      }
    }
  }

  
  ClearSavedChildMetrics();
  
  GatherAndStoreOverflow(&aDesiredStretchSize);

  
  
  

  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmoFrame::InheritAutomaticData(nsIFrame* aParent)
{
  
  nsStretchDirection direction = mEmbellishData.direction;
  nsMathMLTokenFrame::InheritAutomaticData(aParent);
  ProcessTextData();
  mEmbellishData.direction = direction;
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmoFrame::TransmitAutomaticData()
{
  
  
  
  mEmbellishData.coreFrame = nullptr;
  ProcessOperatorData();
  return NS_OK;
}

void
nsMathMLmoFrame::SetInitialChildList(ChildListID     aListID,
                                     nsFrameList&    aChildList)
{
  
  nsMathMLTokenFrame::SetInitialChildList(aListID, aChildList);
  ProcessTextData();
}

void
nsMathMLmoFrame::Reflow(nsPresContext*          aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  
  
  ProcessOperatorData();

  nsMathMLTokenFrame::Reflow(aPresContext, aDesiredSize,
                             aReflowState, aStatus);
}

nsresult
nsMathMLmoFrame::Place(nsRenderingContext&  aRenderingContext,
                       bool                 aPlaceOrigin,
                       nsHTMLReflowMetrics& aDesiredSize)
{
  nsresult rv = nsMathMLTokenFrame::Place(aRenderingContext, aPlaceOrigin,
                                          aDesiredSize);

  if (NS_FAILED(rv)) {
    return rv;
  }

  










  if (!aPlaceOrigin &&
      StyleFont()->mMathDisplay == NS_MATHML_DISPLAYSTYLE_BLOCK &&
      NS_MATHML_OPERATOR_IS_LARGEOP(mFlags) && UseMathMLChar()) {
    nsBoundingMetrics newMetrics;
    rv = mMathMLChar.Stretch(PresContext(), aRenderingContext,
                                      nsLayoutUtils::FontSizeInflationFor(this),
                                      NS_STRETCH_DIRECTION_VERTICAL,
                                      aDesiredSize.mBoundingMetrics, newMetrics,
                                      NS_STRETCH_LARGEOP, StyleVisibility()->mDirection);

    if (NS_FAILED(rv)) {
      
      return NS_OK;
    }

    aDesiredSize.mBoundingMetrics = newMetrics;
     



    aDesiredSize.SetBlockStartAscent(std::max(mBoundingMetrics.ascent,
                                              newMetrics.ascent));
    aDesiredSize.Height() = aDesiredSize.BlockStartAscent() +
                            std::max(mBoundingMetrics.descent,
                                     newMetrics.descent);
    aDesiredSize.Width() = newMetrics.width;
    mBoundingMetrics = newMetrics;
  }
  return NS_OK;
}

 void
nsMathMLmoFrame::MarkIntrinsicISizesDirty()
{
  
  
  
  

  ProcessTextData();

  nsIFrame* target = this;
  nsEmbellishData embellishData;
  do {
    target = target->GetParent();
    GetEmbellishDataFrom(target, embellishData);
  } while (embellishData.coreFrame == this);

  
  
  
  RebuildAutomaticDataForChildren(target);

  nsMathMLContainerFrame::MarkIntrinsicISizesDirty();
}

 void
nsMathMLmoFrame::GetIntrinsicISizeMetrics(nsRenderingContext *aRenderingContext, nsHTMLReflowMetrics& aDesiredSize)
{
  ProcessOperatorData();
  if (UseMathMLChar()) {
    uint32_t stretchHint = GetStretchHint(mFlags, mPresentationData, true,
                                          StyleFont());
    aDesiredSize.Width() = mMathMLChar.
      GetMaxWidth(PresContext(), *aRenderingContext,
                  nsLayoutUtils::FontSizeInflationFor(this),
                  stretchHint, mMaxSize,
                  NS_MATHML_OPERATOR_MAXSIZE_IS_ABSOLUTE(mFlags));
  }
  else {
    nsMathMLTokenFrame::GetIntrinsicISizeMetrics(aRenderingContext,
                                                 aDesiredSize);
  }

  
  
  
  bool isRTL = StyleVisibility()->mDirection;
  aDesiredSize.Width() +=
    mEmbellishData.leadingSpace + mEmbellishData.trailingSpace;
  aDesiredSize.mBoundingMetrics.width = aDesiredSize.Width();
  if (isRTL) {
    aDesiredSize.mBoundingMetrics.leftBearing += mEmbellishData.trailingSpace;
    aDesiredSize.mBoundingMetrics.rightBearing += mEmbellishData.trailingSpace;
  } else {
    aDesiredSize.mBoundingMetrics.leftBearing += mEmbellishData.leadingSpace;
    aDesiredSize.mBoundingMetrics.rightBearing += mEmbellishData.leadingSpace;
  }
}

nsresult
nsMathMLmoFrame::AttributeChanged(int32_t         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  int32_t         aModType)
{
  
  
  if (nsGkAtoms::accent_ == aAttribute ||
      nsGkAtoms::movablelimits_ == aAttribute) {

    
    
    nsIFrame* target = this;
    nsEmbellishData embellishData;
    do {
      target = target->GetParent();
      GetEmbellishDataFrom(target, embellishData);
    } while (embellishData.coreFrame == this);

    
    return ReLayoutChildren(target);
  }

  return nsMathMLTokenFrame::
         AttributeChanged(aNameSpaceID, aAttribute, aModType);
}




nsStyleContext*
nsMathMLmoFrame::GetAdditionalStyleContext(int32_t aIndex) const
{
  switch (aIndex) {
  case NS_MATHML_CHAR_STYLE_CONTEXT_INDEX:
    return mMathMLChar.GetStyleContext();
  default:
    return nullptr;
  }
}

void
nsMathMLmoFrame::SetAdditionalStyleContext(int32_t          aIndex,
                                           nsStyleContext*  aStyleContext)
{
  switch (aIndex) {
  case NS_MATHML_CHAR_STYLE_CONTEXT_INDEX:
    mMathMLChar.SetStyleContext(aStyleContext);
    break;
  }
}
