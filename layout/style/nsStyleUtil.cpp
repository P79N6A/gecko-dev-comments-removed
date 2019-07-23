




































#include <math.h>
#include "nsStyleUtil.h"
#include "nsCRT.h"
#include "nsStyleConsts.h"

#include "nsGkAtoms.h"
#include "nsILinkHandler.h"
#include "nsILink.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsContentUtils.h"
#include "nsTextFormatter.h"



#include "nsRuleNode.h"

nsCachedStyleData::StyleStructInfo
nsCachedStyleData::gInfo[] = {

#define STYLE_STRUCT_INHERITED(name, checkdata_cb, ctor_args) \
  { offsetof(nsCachedStyleData, mInheritedData), \
    offsetof(nsInheritedStyleData, m##name##Data), \
    PR_FALSE },
#define STYLE_STRUCT_RESET(name, checkdata_cb, ctor_args) \
  { offsetof(nsCachedStyleData, mResetData), \
    offsetof(nsResetStyleData, m##name##Data), \
    PR_TRUE },

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET

  { 0, 0, 0 }
};

#define POSITIVE_SCALE_FACTOR 1.10 /* 10% */
#define NEGATIVE_SCALE_FACTOR .90  /* 10% */










float nsStyleUtil::GetScalingFactor(PRInt32 aScaler)
{
  double scale = 1.0;
  double mult;
  PRInt32 count;

  if(aScaler < 0)   {
    count = -aScaler;
    mult = NEGATIVE_SCALE_FACTOR;
  }
  else {
    count = aScaler;
    mult = POSITIVE_SCALE_FACTOR;
  }

  
  while(count--) {
    scale *= mult;
  }

  return (float)scale;
}






nscoord
nsStyleUtil::CalcFontPointSize(PRInt32 aHTMLSize, PRInt32 aBasePointSize,
                               float aScalingFactor, nsPresContext* aPresContext,
                               nsFontSizeType aFontSizeType)
{
#define sFontSizeTableMin  9 
#define sFontSizeTableMax 16 







  static PRInt32 sStrictFontSizeTable[sFontSizeTableMax - sFontSizeTableMin + 1][8] =
  {
      { 9,    9,     9,     9,    11,    14,    18,    27},
      { 9,    9,     9,    10,    12,    15,    20,    30},
      { 9,    9,    10,    11,    13,    17,    22,    33},
      { 9,    9,    10,    12,    14,    18,    24,    36},
      { 9,   10,    12,    13,    16,    20,    26,    39},
      { 9,   10,    12,    14,    17,    21,    28,    42},
      { 9,   10,    13,    15,    18,    23,    30,    45},
      { 9,   10,    13,    16,    18,    24,    32,    48}
  };





















  static PRInt32 sQuirksFontSizeTable[sFontSizeTableMax - sFontSizeTableMin + 1][8] =
  {
      { 9,    9,     9,     9,    11,    14,    18,    28 },
      { 9,    9,     9,    10,    12,    15,    20,    31 },
      { 9,    9,     9,    11,    13,    17,    22,    34 },
      { 9,    9,    10,    12,    14,    18,    24,    37 },
      { 9,    9,    10,    13,    16,    20,    26,    40 }, 
      { 9,    9,    11,    14,    17,    21,    28,    42 },
      { 9,   10,    12,    15,    17,    23,    30,    45 },
      { 9,   10,    13,    16,    18,    24,    32,    48 }  
  };





#if 0



      { ?,    8,    11,    12,    13,    16,    21,    32 }, 
      { ?,    9,    12,    13,    16,    21,    27,    40 }, 
      { ?,   10,    13,    16,    18,    24,    32,    48 }, 
      { ?,   13,    16,    19,    21,    27,    37,    ?? }, 
      { ?,   16,    19,    21,    24,    32,    43,    ?? }  






#endif

  static PRInt32 sFontSizeFactors[8] = { 60,75,89,100,120,150,200,300 };

  static PRInt32 sCSSColumns[7]  = {0, 1, 2, 3, 4, 5, 6}; 
  static PRInt32 sHTMLColumns[7] = {1, 2, 3, 4, 5, 6, 7}; 

  double dFontSize;

  if (aFontSizeType == eFontSize_HTML) {
    aHTMLSize--;    
  }

  if (aHTMLSize < 0)
    aHTMLSize = 0;
  else if (aHTMLSize > 6)
    aHTMLSize = 6;

  PRInt32* column;
  switch (aFontSizeType)
  {
    case eFontSize_HTML: column = sHTMLColumns; break;
    case eFontSize_CSS:  column = sCSSColumns;  break;
  }

  
  PRInt32 fontSize = nsPresContext::AppUnitsToIntCSSPixels(aBasePointSize);

  if ((fontSize >= sFontSizeTableMin) && (fontSize <= sFontSizeTableMax))
  {
    PRInt32 row = fontSize - sFontSizeTableMin;

	  if (aPresContext->CompatibilityMode() == eCompatibility_NavQuirks) {
	    dFontSize = nsPresContext::CSSPixelsToAppUnits(sQuirksFontSizeTable[row][column[aHTMLSize]]);
	  } else {
	    dFontSize = nsPresContext::CSSPixelsToAppUnits(sStrictFontSizeTable[row][column[aHTMLSize]]);
	  }
  }
  else
  {
    PRInt32 factor = sFontSizeFactors[column[aHTMLSize]];
    dFontSize = (factor * aBasePointSize) / 100;
  }

  dFontSize *= aScalingFactor;

  if (1.0 < dFontSize) {
    return (nscoord)dFontSize;
  }
  return (nscoord)1;
}






nscoord nsStyleUtil::FindNextSmallerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                             float aScalingFactor, nsPresContext* aPresContext,
                                             nsFontSizeType aFontSizeType)
{
  PRInt32 index;
  PRInt32 indexMin;
  PRInt32 indexMax;
  float relativePosition;
  nscoord smallerSize;
  nscoord indexFontSize = aFontSize; 
  nscoord smallestIndexFontSize;
  nscoord largestIndexFontSize;
  nscoord smallerIndexFontSize;
  nscoord largerIndexFontSize;

  nscoord onePx = nsPresContext::CSSPixelsToAppUnits(1);

	if (aFontSizeType == eFontSize_HTML) {
		indexMin = 1;
		indexMax = 7;
	} else {
		indexMin = 0;
		indexMax = 6;
	}
  
  smallestIndexFontSize = CalcFontPointSize(indexMin, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
  largestIndexFontSize = CalcFontPointSize(indexMax, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType); 
  if (aFontSize > smallestIndexFontSize) {
    if (aFontSize < NSToCoordRound(float(largestIndexFontSize) * 1.5)) { 
      
      for (index = indexMax; index >= indexMin; index--) {
        indexFontSize = CalcFontPointSize(index, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
        if (indexFontSize < aFontSize)
          break;
      } 
      
      if (indexFontSize == smallestIndexFontSize) {
        smallerIndexFontSize = indexFontSize - onePx;
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
      } else if (indexFontSize == largestIndexFontSize) {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
        largerIndexFontSize = NSToCoordRound(float(largestIndexFontSize) * 1.5);
      } else {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
      }
      
      relativePosition = float(aFontSize - indexFontSize) / float(largerIndexFontSize - indexFontSize);            
      
      smallerSize = smallerIndexFontSize + NSToCoordRound(relativePosition * (indexFontSize - smallerIndexFontSize));      
    }
    else {  
      smallerSize = NSToCoordRound(float(aFontSize) / 1.5);
    }
  }
  else { 
    smallerSize = PR_MAX(aFontSize - onePx, onePx);
  }
  return smallerSize;
}





nscoord nsStyleUtil::FindNextLargerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                            float aScalingFactor, nsPresContext* aPresContext,
                                            nsFontSizeType aFontSizeType)
{
  PRInt32 index;
  PRInt32 indexMin;
  PRInt32 indexMax;
  float relativePosition;
  nscoord largerSize;
  nscoord indexFontSize = aFontSize; 
  nscoord smallestIndexFontSize;
  nscoord largestIndexFontSize;
  nscoord smallerIndexFontSize;
  nscoord largerIndexFontSize;

  nscoord onePx = nsPresContext::CSSPixelsToAppUnits(1);

	if (aFontSizeType == eFontSize_HTML) {
		indexMin = 1;
		indexMax = 7;
	} else {
		indexMin = 0;
		indexMax = 6;
	}
  
  smallestIndexFontSize = CalcFontPointSize(indexMin, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
  largestIndexFontSize = CalcFontPointSize(indexMax, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType); 
  if (aFontSize > (smallestIndexFontSize - onePx)) {
    if (aFontSize < largestIndexFontSize) { 
      
      for (index = indexMin; index <= indexMax; index++) { 
        indexFontSize = CalcFontPointSize(index, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
        if (indexFontSize > aFontSize)
          break;
      }
      
      if (indexFontSize == smallestIndexFontSize) {
        smallerIndexFontSize = indexFontSize - onePx;
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
      } else if (indexFontSize == largestIndexFontSize) {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
        largerIndexFontSize = NSToCoordRound(float(largestIndexFontSize) * 1.5);
      } else {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aScalingFactor, aPresContext, aFontSizeType);
      }
      
      relativePosition = float(aFontSize - smallerIndexFontSize) / float(indexFontSize - smallerIndexFontSize);
      
      largerSize = indexFontSize + NSToCoordRound(relativePosition * (largerIndexFontSize - indexFontSize));      
    }
    else {  
      largerSize = NSToCoordRound(float(aFontSize) * 1.5);
    }
  }
  else { 
    largerSize = aFontSize + onePx; 
  }
  return largerSize;
}





PRInt32 
nsStyleUtil::ConstrainFontWeight(PRInt32 aWeight)
{
  aWeight = ((aWeight < 100) ? 100 : ((aWeight > 900) ? 900 : aWeight));
  PRInt32 base = ((aWeight / 100) * 100);
  PRInt32 step = (aWeight % 100);
  PRBool  negativeStep = PRBool(50 < step);
  PRInt32 maxStep;
  if (negativeStep) {
    step = 100 - step;
    maxStep = (base / 100);
    base += 100;
  }
  else {
    maxStep = ((900 - base) / 100);
  }
  if (maxStep < step) {
    step = maxStep;
  }
  return (base + ((negativeStep) ? -step : step));
}

static nsLinkState
GetLinkStateFromURI(nsIURI* aURI, nsIContent* aContent,
                    nsILinkHandler* aLinkHandler)
{
  NS_PRECONDITION(aURI, "Must have URI");
  nsLinkState state;
  if (NS_LIKELY(aLinkHandler)) {
    aLinkHandler->GetLinkState(aURI, state);
  }
  else {
    
    NS_ASSERTION(aContent->GetOwnerDoc(), "Shouldn't happen");
    nsCOMPtr<nsISupports> supp =
      aContent->GetOwnerDoc()->GetContainer();
    nsCOMPtr<nsILinkHandler> handler = do_QueryInterface(supp);
    if (handler) {
      handler->GetLinkState(aURI, state);
    } else {
      
      state = eLinkState_Unvisited;
    }
  }

  return state;  
}


PRBool nsStyleUtil::IsHTMLLink(nsIContent *aContent, nsIAtom *aTag,
                               nsILinkHandler *aLinkHandler,
                               PRBool aForStyling,
                               nsLinkState *aState)
{
  NS_ASSERTION(aContent && aState, "null arg in IsHTMLLink");

  
  
  
  

  PRBool result = PR_FALSE;

  if ((aTag == nsGkAtoms::a) ||
      (aTag == nsGkAtoms::link) ||
      (aTag == nsGkAtoms::area)) {

    nsCOMPtr<nsILink> link( do_QueryInterface(aContent) );
    
    if (link) {
      nsLinkState linkState;
      link->GetLinkState(linkState);
      if (linkState == eLinkState_Unknown) {
        
        
        
        

        nsCOMPtr<nsIURI> hrefURI;
        link->GetHrefURI(getter_AddRefs(hrefURI));

        if (hrefURI) {
          linkState = GetLinkStateFromURI(hrefURI, aContent, aLinkHandler);
        } else {
          linkState = eLinkState_NotLink;
        }
        if (linkState != eLinkState_NotLink && aForStyling) {
          NS_ASSERTION(aContent->GetCurrentDoc(), "Must have document!");
          aContent->GetCurrentDoc()->AddStyleRelevantLink(aContent, hrefURI);
        }
        link->SetLinkState(linkState);
      }
      if (linkState != eLinkState_NotLink) {
        *aState = linkState;
        result = PR_TRUE;
      }
    }
  }

  return result;
}


PRBool nsStyleUtil::IsLink(nsIContent     *aContent,
                           nsILinkHandler *aLinkHandler,
                           PRBool          aForStyling,
                           nsLinkState    *aState)
{
  
  
  

  NS_ASSERTION(aContent && aState, "invalid call to IsLink with null content");

  PRBool rv = PR_FALSE;

  if (aContent && aState) {
    nsCOMPtr<nsIURI> absURI;
    if (aContent->IsLink(getter_AddRefs(absURI))) {
      *aState = GetLinkStateFromURI(absURI, aContent, aLinkHandler);
      if (aForStyling) {
        NS_ASSERTION(aContent->GetCurrentDoc(), "Must have document!");
        aContent->GetCurrentDoc()->AddStyleRelevantLink(aContent, absURI);
      }

      rv = PR_TRUE;
    }
  }
  return rv;
}


PRBool nsStyleUtil::DashMatchCompare(const nsAString& aAttributeValue,
                                     const nsAString& aSelectorValue,
                                     const nsStringComparator& aComparator)
{
  PRBool result;
  PRUint32 selectorLen = aSelectorValue.Length();
  PRUint32 attributeLen = aAttributeValue.Length();
  if (selectorLen > attributeLen) {
    result = PR_FALSE;
  }
  else {
    nsAString::const_iterator iter;
    if (selectorLen != attributeLen &&
        *aAttributeValue.BeginReading(iter).advance(selectorLen) !=
            PRUnichar('-')) {
      
      
      
      result = PR_FALSE;
    }
    else {
      result = StringBeginsWith(aAttributeValue, aSelectorValue, aComparator);
    }
  }
  return result;
}

void nsStyleUtil::EscapeCSSString(const nsString& aString, nsAString& aReturn)
{
  aReturn.Truncate();

  const nsString::char_type* in = aString.get();
  const nsString::char_type* const end = in + aString.Length();
  for (; in != end; in++)
  {
    if (*in < 0x20)
    {
     
   
     




     PRUnichar buf[5];
     nsTextFormatter::snprintf(buf, NS_ARRAY_LENGTH(buf), NS_LITERAL_STRING("\\%hX ").get(), *in);
     aReturn.Append(buf);
   
    } else switch (*in) {
      
      case '\\':
      case '\"':
      case '\'':
       aReturn.Append(PRUnichar('\\'));
      
      default:
       aReturn.Append(PRUnichar(*in));
    }
  }
}

 float
nsStyleUtil::ColorComponentToFloat(PRUint8 aAlpha)
{
  
  
  
  
  float rounded = NS_roundf(float(aAlpha) * 100.0f / 255.0f) / 100.0f;
  if (FloatToColorComponent(rounded) != aAlpha) {
    
    rounded = NS_roundf(float(aAlpha) * 1000.0f / 255.0f) / 1000.0f;
  }
  return rounded;
}

 PRBool
nsStyleUtil::IsSignificantChild(nsIContent* aChild, PRBool aTextIsSignificant,
                                PRBool aWhitespaceIsSignificant)
{
  NS_ASSERTION(!aWhitespaceIsSignificant || aTextIsSignificant,
               "Nonsensical arguments");

  PRBool isText = aChild->IsNodeOfType(nsINode::eTEXT);

  if (!isText && !aChild->IsNodeOfType(nsINode::eCOMMENT) &&
      !aChild->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
    return PR_TRUE;
  }

  return aTextIsSignificant && isText && aChild->TextLength() != 0 &&
         (aWhitespaceIsSignificant ||
          !aChild->TextIsOnlyWhitespace());
}

