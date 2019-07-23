








































#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsContentUtils.h"

#include "nsIDOMText.h"

#include "nsMathMLmoFrame.h"






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

static const PRUnichar kInvisibleComma = PRUnichar(0x200B); 
static const PRUnichar kApplyFunction  = PRUnichar(0x2061);
static const PRUnichar kInvisibleTimes = PRUnichar(0x2062);
static const PRUnichar kNullCh         = PRUnichar('\0');

eMathMLFrameType
nsMathMLmoFrame::GetMathMLFrameType()
{
  return NS_MATHML_OPERATOR_IS_INVISIBLE(mFlags)
    ? eMathMLFrameType_OperatorInvisible
    : eMathMLFrameType_OperatorOrdinary;
}




PRBool
nsMathMLmoFrame::IsFrameInSelection(nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "null arg");
  if (!aFrame)
    return PR_FALSE;

  PRBool isSelected = PR_FALSE;
  aFrame->GetSelected(&isSelected);
  if (!isSelected)
    return PR_FALSE;

  const nsFrameSelection* frameSelection = aFrame->GetConstFrameSelection();
  SelectionDetails* details =
    frameSelection->LookUpSelection(aFrame->GetContent(), 0, 1, PR_TRUE);

  if (!details)
    return PR_FALSE;

  while (details) {
    SelectionDetails* next = details->mNext;
    delete details;
    details = next;
  }
  return PR_TRUE;
}

PRBool
nsMathMLmoFrame::UseMathMLChar()
{
  return (NS_MATHML_OPERATOR_GET_FORM(mFlags) &&
          NS_MATHML_OPERATOR_IS_MUTABLE(mFlags)) ||
    NS_MATHML_OPERATOR_IS_CENTERED(mFlags) ||
    NS_MATHML_OPERATOR_IS_INVISIBLE(mFlags);
}

NS_IMETHODIMP
nsMathMLmoFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  nsresult rv = NS_OK;
  PRBool useMathMLChar = UseMathMLChar();

  if (!useMathMLChar) {
    
    rv = nsMathMLTokenFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    PRBool isSelected = PR_FALSE;
    nsRect selectedRect;
    nsIFrame* firstChild = mFrames.FirstChild();
    if (IsFrameInSelection(firstChild)) {
      selectedRect = firstChild->GetRect();
      isSelected = PR_TRUE;
    }
    rv = mMathMLChar.Display(aBuilder, this, aLists, isSelected ? &selectedRect : nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  
#if defined(NS_DEBUG) && defined(SHOW_BOUNDING_BOX)
    
    rv = DisplayBoundingMetrics(aBuilder, this, mReference, mBoundingMetrics, aLists);
#endif
  }
  return rv;
}


void
nsMathMLmoFrame::ProcessTextData()
{
  mFlags = 0;

  nsAutoString data;
  nsContentUtils::GetNodeTextContent(mContent, PR_FALSE, data);
  PRInt32 length = data.Length();
  PRUnichar ch = (length == 0) ? kNullCh : data[0];

  if ((length == 1) && 
      (ch == kInvisibleComma || 
       ch == kApplyFunction  || 
       ch == kInvisibleTimes)) {
    mFlags |= NS_MATHML_OPERATOR_INVISIBLE;
  }

  
  
  nsPresContext* presContext = PresContext();
  if (NS_MATHML_OPERATOR_IS_INVISIBLE(mFlags) || mFrames.GetLength() != 1) {
    data.Truncate(); 
    mMathMLChar.SetData(presContext, data);
    ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mMathMLChar, PR_FALSE);
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

  PRBool isMutable =
    NS_MATHML_OPERATOR_IS_STRETCHY(allFlags) ||
    NS_MATHML_OPERATOR_IS_LARGEOP(allFlags);
  if (isMutable)
    mFlags |= NS_MATHML_OPERATOR_MUTABLE;

  
  
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
  ResolveMathMLCharStyle(presContext, mContent, mStyleContext, &mMathMLChar, isMutable);

  
  
  
  mEmbellishData.direction = mMathMLChar.GetStretchDirection();
}






void
nsMathMLmoFrame::ProcessOperatorData()
{
  
  nsOperatorFlags form = NS_MATHML_OPERATOR_GET_FORM(mFlags);
  nsAutoString value;

  
  
  
  
  
  
  
  mFlags &= NS_MATHML_OPERATOR_MUTABLE |
            NS_MATHML_OPERATOR_ACCENT | 
            NS_MATHML_OPERATOR_MOVABLELIMITS |
            NS_MATHML_OPERATOR_CENTERED |
            NS_MATHML_OPERATOR_INVISIBLE;

  if (!mEmbellishData.coreFrame) {
    
    form = NS_MATHML_OPERATOR_FORM_INFIX;

    
    
    mEmbellishData.flags = 0;
    mEmbellishData.coreFrame = nsnull;
    mEmbellishData.leftSpace = 0;
    mEmbellishData.rightSpace = 0;
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

    
    GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::accent_,
                 value);
    if (value.EqualsLiteral("true"))
      mEmbellishData.flags |= NS_MATHML_EMBELLISH_ACCENT;
    else if (value.EqualsLiteral("false"))
      mEmbellishData.flags &= ~NS_MATHML_EMBELLISH_ACCENT;

    
    GetAttribute(mContent, mPresentationData.mstyle,
                 nsGkAtoms::movablelimits_, value);
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

    
    
    nsFrameList frameList(parentAncestor->GetFirstChild(nsnull));

    nsIFrame* nextSibling = embellishAncestor->GetNextSibling();
    nsIFrame* prevSibling = frameList.GetPrevSiblingFor(embellishAncestor);

    
    if (!prevSibling && !nextSibling)
      mFlags |= NS_MATHML_OPERATOR_EMBELLISH_ISOLATED;
    else
      mFlags &= ~NS_MATHML_OPERATOR_EMBELLISH_ISOLATED;

    
    form = NS_MATHML_OPERATOR_FORM_INFIX;
    GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::form,
                 value);
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

    
    float lspace = 0.0f;
    float rspace = 0.0f;
    nsAutoString data;
    mMathMLChar.GetData(data);
    PRBool found = nsMathMLOperators::LookupOperator(data, form, &mFlags, &lspace, &rspace);
    if (found && (lspace || rspace)) {
      
      
      nscoord em;
      nsCOMPtr<nsIFontMetrics> fm =
	presContext->GetMetricsFor(GetStyleFont()->mFont);
      GetEmHeight(fm, em);

      mEmbellishData.leftSpace = NSToCoordRound(lspace * em);
      mEmbellishData.rightSpace = NSToCoordRound(rspace * em);

      
      
      
      if (GetStyleFont()->mScriptLevel > 0) {
        if (NS_MATHML_OPERATOR_EMBELLISH_IS_ISOLATED(mFlags)) {
          
          mEmbellishData.leftSpace = 0;
          mEmbellishData.rightSpace  = 0;
        }
        else if (!NS_MATHML_OPERATOR_HAS_EMBELLISH_ANCESTOR(mFlags)) {
          mEmbellishData.leftSpace /= 2;
          mEmbellishData.rightSpace  /= 2;
        }
      }
    }
  }

  
  

  
  nscoord leftSpace = mEmbellishData.leftSpace;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::lspace_,
               value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) ||
        ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue))
    {
      if ((eCSSUnit_Number == cssValue.GetUnit()) && !cssValue.GetFloatValue())
        leftSpace = 0;
      else if (cssValue.IsLengthUnit())
        leftSpace = CalcLength(presContext, mStyleContext, cssValue);
      mFlags |= NS_MATHML_OPERATOR_LEFTSPACE_ATTR;
    }
  }

  
  nscoord rightSpace = mEmbellishData.rightSpace;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::rspace_,
               value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) ||
        ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue))
    {
      if ((eCSSUnit_Number == cssValue.GetUnit()) && !cssValue.GetFloatValue())
        rightSpace = 0;
      else if (cssValue.IsLengthUnit())
        rightSpace = CalcLength(presContext, mStyleContext, cssValue);
      mFlags |= NS_MATHML_OPERATOR_RIGHTSPACE_ATTR;
    }
  }

  
  
  if (leftSpace || rightSpace) {
    nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
    if (leftSpace && leftSpace < onePixel)
      leftSpace = onePixel;
    if (rightSpace && rightSpace < onePixel)
      rightSpace = onePixel;
  }

  
  mEmbellishData.leftSpace = leftSpace;
  mEmbellishData.rightSpace = rightSpace;

  
  
  

  
  
  
  

  if (NS_MATHML_OPERATOR_IS_STRETCHY(mFlags)) {
    GetAttribute(mContent, mPresentationData.mstyle,
                 nsGkAtoms::stretchy_, value);
    if (value.EqualsLiteral("false"))
      mFlags &= ~NS_MATHML_OPERATOR_STRETCHY;
  }
  if (NS_MATHML_OPERATOR_IS_FENCE(mFlags)) {
    GetAttribute(mContent, mPresentationData.mstyle,
                 nsGkAtoms::fence_, value);
    if (value.EqualsLiteral("false"))
      mFlags &= ~NS_MATHML_OPERATOR_FENCE;
  }
  if (NS_MATHML_OPERATOR_IS_LARGEOP(mFlags)) {
    GetAttribute(mContent, mPresentationData.mstyle,
                 nsGkAtoms::largeop_, value);
    if (value.EqualsLiteral("false"))
      mFlags &= ~NS_MATHML_OPERATOR_LARGEOP;
  }
  if (NS_MATHML_OPERATOR_IS_SEPARATOR(mFlags)) {
    GetAttribute(mContent, mPresentationData.mstyle,
                 nsGkAtoms::separator_, value);
    if (value.EqualsLiteral("false"))
      mFlags &= ~NS_MATHML_OPERATOR_SEPARATOR;
  }
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::symmetric_,
               value);
  if (value.EqualsLiteral("false"))
    mFlags &= ~NS_MATHML_OPERATOR_SYMMETRIC;
  else if (value.EqualsLiteral("true"))
    mFlags |= NS_MATHML_OPERATOR_SYMMETRIC;

  
  mMinSize = 0.0;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::minsize_,
               value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) ||
        ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue))
    {
      nsCSSUnit unit = cssValue.GetUnit();
      if (eCSSUnit_Number == unit)
        mMinSize = cssValue.GetFloatValue();
      else if (eCSSUnit_Percent == unit)
        mMinSize = cssValue.GetPercentValue();
      else if (eCSSUnit_Null != unit) {
        mMinSize = float(CalcLength(presContext, mStyleContext, cssValue));
        mFlags |= NS_MATHML_OPERATOR_MINSIZE_ABSOLUTE;
      }

      if ((eCSSUnit_Number == unit) || (eCSSUnit_Percent == unit)) {
        
        GetAttribute(nsnull, mPresentationData.mstyle,
                     nsGkAtoms::minsize_, value);
        if (!value.IsEmpty()) {
          if (ParseNumericValue(value, cssValue)) {
            if (cssValue.IsLengthUnit()) {
              mMinSize *= float(CalcLength(presContext, mStyleContext, cssValue));
              mFlags |= NS_MATHML_OPERATOR_MINSIZE_ABSOLUTE;
            }
          }
        }
      }
    }
  }

  
  mMaxSize = NS_MATHML_OPERATOR_SIZE_INFINITY;
  GetAttribute(mContent, mPresentationData.mstyle, nsGkAtoms::maxsize_,
               value);
  if (!value.IsEmpty()) {
    nsCSSValue cssValue;
    if (ParseNumericValue(value, cssValue) ||
        ParseNamedSpaceValue(mPresentationData.mstyle, value, cssValue))
    {
      nsCSSUnit unit = cssValue.GetUnit();
      if (eCSSUnit_Number == unit)
        mMaxSize = cssValue.GetFloatValue();
      else if (eCSSUnit_Percent == unit)
        mMaxSize = cssValue.GetPercentValue();
      else if (eCSSUnit_Null != unit) {
        mMaxSize = float(CalcLength(presContext, mStyleContext, cssValue));
        mFlags |= NS_MATHML_OPERATOR_MAXSIZE_ABSOLUTE;
      }

      if ((eCSSUnit_Number == unit) || (eCSSUnit_Percent == unit)) {
        
        GetAttribute(nsnull, mPresentationData.mstyle,
                     nsGkAtoms::maxsize_, value);
        if (!value.IsEmpty()) {
          if (ParseNumericValue(value, cssValue)) {
            if (cssValue.IsLengthUnit()) {
              mMaxSize *= float(CalcLength(presContext, mStyleContext, cssValue));
              mFlags |= NS_MATHML_OPERATOR_MAXSIZE_ABSOLUTE;
            }
          }
        }
      }
    }
  }
}

static PRUint32
GetStretchHint(nsOperatorFlags aFlags, nsPresentationData aPresentationData,
               PRBool aIsVertical)
{
  PRUint32 stretchHint = NS_STRETCH_NONE;
  
  
  if (NS_MATHML_OPERATOR_IS_MUTABLE(aFlags)) {
    
    
    
    
    
    if (NS_MATHML_IS_DISPLAYSTYLE(aPresentationData.flags) &&
        NS_MATHML_OPERATOR_IS_LARGEOP(aFlags)) {
      stretchHint = NS_STRETCH_LARGEOP; 
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
nsMathMLmoFrame::Stretch(nsIRenderingContext& aRenderingContext,
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

  
  nsCOMPtr<nsIFontMetrics> fm;
  aRenderingContext.SetFont(GetStyleFont()->mFont, nsnull,
                            PresContext()->GetUserFontSet());
  aRenderingContext.GetFontMetrics(*getter_AddRefs(fm));
  nscoord axisHeight, height;
  GetAxisHeight(aRenderingContext, fm, axisHeight);

  
  
  nscoord em;
  GetEmHeight(fm, em);
  nscoord leading = NSToCoordRound(0.2f * em);

  
  
  
  PRBool useMathMLChar = UseMathMLChar();

  nsBoundingMetrics charSize;
  nsBoundingMetrics container = aDesiredStretchSize.mBoundingMetrics;
  PRBool isVertical = PR_FALSE;
  if (useMathMLChar) {
    nsBoundingMetrics initialSize = aDesiredStretchSize.mBoundingMetrics;

    if (((aStretchDirection == NS_STRETCH_DIRECTION_VERTICAL) ||
         (aStretchDirection == NS_STRETCH_DIRECTION_DEFAULT))  &&
        (mEmbellishData.direction == NS_STRETCH_DIRECTION_VERTICAL)) {
      isVertical = PR_TRUE;
    }

    PRUint32 stretchHint =
      GetStretchHint(mFlags, mPresentationData, isVertical);

    if (stretchHint != NS_STRETCH_NONE) {

      container = aContainerSize;

      

      if (isVertical && NS_MATHML_OPERATOR_IS_SYMMETRIC(mFlags)) {
        
        nscoord delta = NS_MAX(container.ascent - axisHeight,
                               container.descent + axisHeight);
        container.ascent = delta + axisHeight;
        container.descent = delta - axisHeight;

        
        delta = NS_MAX(initialSize.ascent - axisHeight,
                       initialSize.descent + axisHeight);
        initialSize.ascent = delta + axisHeight;
        initialSize.descent = delta - axisHeight;
      }

      

      if (mMaxSize != NS_MATHML_OPERATOR_SIZE_INFINITY && mMaxSize > 0.0f) {
        
        
        if (NS_MATHML_OPERATOR_MAXSIZE_IS_ABSOLUTE(mFlags)) {
          
          
          float aspect = mMaxSize / float(initialSize.ascent + initialSize.descent);
          container.ascent =
            NS_MIN(container.ascent, nscoord(initialSize.ascent * aspect));
          container.descent =
            NS_MIN(container.descent, nscoord(initialSize.descent * aspect));
          
          
          container.width =
            NS_MIN(container.width, (nscoord)mMaxSize);
        }
        else { 
          container.ascent =
            NS_MIN(container.ascent, nscoord(initialSize.ascent * mMaxSize));
          container.descent =
            NS_MIN(container.descent, nscoord(initialSize.descent * mMaxSize));
          container.width =
            NS_MIN(container.width, nscoord(initialSize.width * mMaxSize));
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
            NS_MAX(container.ascent, nscoord(initialSize.ascent * aspect));
          container.descent =
            NS_MAX(container.descent, nscoord(initialSize.descent * aspect));
          container.width =
            NS_MAX(container.width, (nscoord)mMinSize);
        }
        else { 
          container.ascent =
            NS_MAX(container.ascent, nscoord(initialSize.ascent * mMinSize));
          container.descent =
            NS_MAX(container.descent, nscoord(initialSize.descent * mMinSize));
          container.width =
            NS_MAX(container.width, nscoord(initialSize.width * mMinSize));
        }

        if (isVertical && !NS_MATHML_OPERATOR_IS_SYMMETRIC(mFlags)) {
          
          height = container.ascent + container.descent;
          container.descent = aContainerSize.descent;
          container.ascent = height - container.descent;
        }
      }
    }

    
    nsresult res = mMathMLChar.Stretch(PresContext(), aRenderingContext,
                                       aStretchDirection, container, charSize, stretchHint);
    if (NS_FAILED(res)) {
      
      
      mFlags &= ~NS_MATHML_OPERATOR_FORM;
      useMathMLChar = PR_FALSE;
    }
  }

  
  if (!NS_MATHML_OPERATOR_IS_INVISIBLE(mFlags)) {
    
    
    nsresult rv = Place(aRenderingContext, PR_TRUE, aDesiredStretchSize);
    if (NS_MATHML_HAS_ERROR(mPresentationData.flags) || NS_FAILED(rv)) {
      
      DidReflowChildren(mFrames.FirstChild());
    }
  }

  if (useMathMLChar) {
    
    mBoundingMetrics = charSize;

    
    
    if (mMathMLChar.GetStretchDirection() != NS_STRETCH_DIRECTION_UNSUPPORTED ||
        NS_MATHML_OPERATOR_IS_CENTERED(mFlags)) {

      if (isVertical || NS_MATHML_OPERATOR_IS_CENTERED(mFlags)) {
        
        
        

        height = mBoundingMetrics.ascent + mBoundingMetrics.descent;
        if (NS_MATHML_OPERATOR_IS_SYMMETRIC(mFlags) ||
            NS_MATHML_OPERATOR_IS_CENTERED(mFlags)) {
          
          
          mBoundingMetrics.descent = height/2 - axisHeight;
        }
        else {
          
          mBoundingMetrics.descent = container.descent;
        }
        mBoundingMetrics.ascent = height - mBoundingMetrics.descent;
      }
    }
  }

  
  
  
  
  
  
  
  
  
  
  

  
  
  PRBool isAccent =
    NS_MATHML_EMBELLISH_IS_ACCENT(mEmbellishData.flags);
  if (isAccent) {
    nsEmbellishData parentData;
    GetEmbellishDataFrom(mParent, parentData);
    isAccent =
       (NS_MATHML_EMBELLISH_IS_ACCENTOVER(parentData.flags) ||
        NS_MATHML_EMBELLISH_IS_ACCENTUNDER(parentData.flags)) &&
       parentData.coreFrame != this;
  }
  if (isAccent && firstChild) {
    
    nscoord dy = aDesiredStretchSize.ascent - (mBoundingMetrics.ascent + leading);
    aDesiredStretchSize.ascent = mBoundingMetrics.ascent + leading;
    aDesiredStretchSize.height = aDesiredStretchSize.ascent + mBoundingMetrics.descent;

    firstChild->SetPosition(firstChild->GetPosition() - nsPoint(0, dy));
  }
  else if (useMathMLChar) {
    nscoord ascent, descent;
    fm->GetMaxAscent(ascent);
    fm->GetMaxDescent(descent);
    aDesiredStretchSize.ascent = NS_MAX(mBoundingMetrics.ascent + leading, ascent);
    aDesiredStretchSize.height = aDesiredStretchSize.ascent +
                                 NS_MAX(mBoundingMetrics.descent + leading, descent);
  }
  aDesiredStretchSize.width = mBoundingMetrics.width;
  aDesiredStretchSize.mBoundingMetrics = mBoundingMetrics;
  mReference.x = 0;
  mReference.y = aDesiredStretchSize.ascent;
  
  if (useMathMLChar) {
    nscoord dy = aDesiredStretchSize.ascent - mBoundingMetrics.ascent;
    mMathMLChar.SetRect(nsRect(0, dy, charSize.width, charSize.ascent + charSize.descent));
  }

  
  
  
  

  if (!NS_MATHML_OPERATOR_HAS_EMBELLISH_ANCESTOR(mFlags)) {

    
    nscoord leftSpace = mEmbellishData.leftSpace;
    if (isAccent && !NS_MATHML_OPERATOR_HAS_LEFTSPACE_ATTR(mFlags)) {
      leftSpace = 0;
    }
    nscoord rightSpace = mEmbellishData.rightSpace;
    if (isAccent && !NS_MATHML_OPERATOR_HAS_RIGHTSPACE_ATTR(mFlags)) {
      rightSpace = 0;
    }

    mBoundingMetrics.width += leftSpace + rightSpace;
    aDesiredStretchSize.width = mBoundingMetrics.width;
    aDesiredStretchSize.mBoundingMetrics.width = mBoundingMetrics.width;

    if (leftSpace) {
      
      mBoundingMetrics.leftBearing += leftSpace;
      mBoundingMetrics.rightBearing += leftSpace;
      aDesiredStretchSize.mBoundingMetrics.leftBearing += leftSpace;
      aDesiredStretchSize.mBoundingMetrics.rightBearing += leftSpace;

      if (useMathMLChar) {
        nsRect rect;
        mMathMLChar.GetRect(rect);
        mMathMLChar.SetRect(nsRect(rect.x + leftSpace, rect.y, rect.width, rect.height));
      }
      else {
        nsIFrame* childFrame = firstChild;
        while (childFrame) {
          childFrame->SetPosition(childFrame->GetPosition()
				  + nsPoint(leftSpace, 0));
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
  mEmbellishData.direction = direction;
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmoFrame::TransmitAutomaticData()
{
  
  
  
  mEmbellishData.coreFrame = nsnull;
  ProcessOperatorData();
  return NS_OK;
}

NS_IMETHODIMP
nsMathMLmoFrame::Reflow(nsPresContext*          aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  
  
  ProcessOperatorData();

  
  
  if (NS_MATHML_OPERATOR_IS_INVISIBLE(mFlags)) {
    
    
    
    aDesiredSize.width = 0;
    aDesiredSize.height = 0;
    aDesiredSize.ascent = 0;
    aDesiredSize.mBoundingMetrics.Clear();
    aStatus = NS_FRAME_COMPLETE;

    NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
    return NS_OK;
  }

  return nsMathMLTokenFrame::Reflow(aPresContext, aDesiredSize,
                                    aReflowState, aStatus);
}

 void
nsMathMLmoFrame::MarkIntrinsicWidthsDirty()
{
  
  
  
  

  ProcessTextData();

  nsIFrame* target = this;
  nsEmbellishData embellishData;
  do {
    target = target->GetParent();
    GetEmbellishDataFrom(target, embellishData);
  } while (embellishData.coreFrame == this);

  
  
  
  RebuildAutomaticDataForChildren(target);

  nsMathMLContainerFrame::MarkIntrinsicWidthsDirty();
}

 nscoord
nsMathMLmoFrame::GetIntrinsicWidth(nsIRenderingContext *aRenderingContext)
{
  ProcessOperatorData();
  nscoord width;
  if (UseMathMLChar()) {
    PRUint32 stretchHint = GetStretchHint(mFlags, mPresentationData, PR_TRUE);
    width = mMathMLChar.
      GetMaxWidth(PresContext(), *aRenderingContext,
                  stretchHint, mMaxSize,
                  NS_MATHML_OPERATOR_MAXSIZE_IS_ABSOLUTE(mFlags));
  }
  else {
    width = nsMathMLTokenFrame::GetIntrinsicWidth(aRenderingContext);
  }

  
  
  
  width += mEmbellishData.leftSpace + mEmbellishData.rightSpace;

  return width;
}

NS_IMETHODIMP
nsMathMLmoFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  PRInt32         aModType)
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
nsMathMLmoFrame::GetAdditionalStyleContext(PRInt32 aIndex) const
{
  switch (aIndex) {
  case NS_MATHML_CHAR_STYLE_CONTEXT_INDEX:
    return mMathMLChar.GetStyleContext();
  default:
    return nsnull;
  }
}

void
nsMathMLmoFrame::SetAdditionalStyleContext(PRInt32          aIndex,
                                           nsStyleContext*  aStyleContext)
{
  switch (aIndex) {
  case NS_MATHML_CHAR_STYLE_CONTEXT_INDEX:
    mMathMLChar.SetStyleContext(aStyleContext);
    break;
  }
}
