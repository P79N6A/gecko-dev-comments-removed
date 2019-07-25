


















































#include "nsRuleNode.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIWidget.h"
#include "nsILookAndFeel.h"
#include "nsIPresShell.h"
#include "nsFontMetrics.h"
#include "gfxFont.h"
#include "nsStyleUtil.h"
#include "nsCSSPseudoElements.h"
#include "nsThemeConstants.h"
#include "nsITheme.h"
#include "pldhash.h"
#include "nsStyleContext.h"
#include "nsStyleSet.h"
#include "nsSize.h"
#include "imgIRequest.h"
#include "nsRuleData.h"
#include "nsILanguageAtomService.h"
#include "nsIStyleRule.h"
#include "nsBidiUtils.h"
#include "nsUnicharUtils.h"
#include "nsStyleStructInlines.h"
#include "nsStyleTransformMatrix.h"
#include "nsCSSKeywords.h"
#include "nsCSSProps.h"
#include "nsTArray.h"
#include "nsContentUtils.h"
#include "mozilla/dom/Element.h"
#include "CSSCalc.h"
#include "nsPrintfCString.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#ifdef _MSC_VER
#define alloca _alloca
#endif
#endif
#ifdef SOLARIS
#include <alloca.h>
#endif

using namespace mozilla::dom;
namespace css = mozilla::css;

#define NS_SET_IMAGE_REQUEST(method_, context_, request_)                   \
  if ((context_)->PresContext()->IsDynamic()) {                               \
    method_(request_);                                                      \
  } else {                                                                  \
    nsCOMPtr<imgIRequest> req = nsContentUtils::GetStaticRequest(request_); \
    method_(req);                                                           \
  }





struct ChildrenHashEntry : public PLDHashEntryHdr {
  
  nsRuleNode *mRuleNode;
};

 PLDHashNumber
nsRuleNode::ChildrenHashHashKey(PLDHashTable *aTable, const void *aKey)
{
  const nsRuleNode::Key *key =
    static_cast<const nsRuleNode::Key*>(aKey);
  
  
  return PL_DHashVoidPtrKeyStub(aTable, key->mRule);
}

 PRBool
nsRuleNode::ChildrenHashMatchEntry(PLDHashTable *aTable,
                                   const PLDHashEntryHdr *aHdr,
                                   const void *aKey)
{
  const ChildrenHashEntry *entry =
    static_cast<const ChildrenHashEntry*>(aHdr);
  const nsRuleNode::Key *key =
    static_cast<const nsRuleNode::Key*>(aKey);
  return entry->mRuleNode->GetKey() == *key;
}

 PLDHashTableOps
nsRuleNode::ChildrenHashOps = {
  
  
  
  
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  ChildrenHashHashKey,
  ChildrenHashMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  NULL
};






static void EnsureBlockDisplay(PRUint8& display)
{
  
  switch (display) {
  case NS_STYLE_DISPLAY_NONE :
    
  case NS_STYLE_DISPLAY_TABLE :
  case NS_STYLE_DISPLAY_BLOCK :
  case NS_STYLE_DISPLAY_LIST_ITEM :
    
    
    
    
    
    break;

  case NS_STYLE_DISPLAY_INLINE_TABLE :
    
    display = NS_STYLE_DISPLAY_TABLE;
    break;

  default :
    
    display = NS_STYLE_DISPLAY_BLOCK;
  }
}

static nscoord CalcLengthWith(const nsCSSValue& aValue,
                              nscoord aFontSize,
                              const nsStyleFont* aStyleFont,
                              nsStyleContext* aStyleContext,
                              nsPresContext* aPresContext,
                              PRBool aUseProvidedRootEmSize,
                              PRBool aUseUserFontSet,
                              PRBool& aCanStoreInRuleTree);

struct CalcLengthCalcOps : public css::BasicCoordCalcOps,
                           public css::NumbersAlreadyNormalizedOps
{
  
  const nscoord mFontSize;
  const nsStyleFont* const mStyleFont;
  nsStyleContext* const mStyleContext;
  nsPresContext* const mPresContext;
  const PRBool mUseProvidedRootEmSize;
  const PRBool mUseUserFontSet;
  PRBool& mCanStoreInRuleTree;

  CalcLengthCalcOps(nscoord aFontSize, const nsStyleFont* aStyleFont,
                    nsStyleContext* aStyleContext, nsPresContext* aPresContext,
                    PRBool aUseProvidedRootEmSize, PRBool aUseUserFontSet,
                    PRBool& aCanStoreInRuleTree)
    : mFontSize(aFontSize),
      mStyleFont(aStyleFont),
      mStyleContext(aStyleContext),
      mPresContext(aPresContext),
      mUseProvidedRootEmSize(aUseProvidedRootEmSize),
      mUseUserFontSet(aUseUserFontSet),
      mCanStoreInRuleTree(aCanStoreInRuleTree)
  {
  }

  result_type ComputeLeafValue(const nsCSSValue& aValue)
  {
    return CalcLengthWith(aValue, mFontSize, mStyleFont, mStyleContext,
                          mPresContext, mUseProvidedRootEmSize,
                          mUseUserFontSet, mCanStoreInRuleTree);
  }
};

static inline nscoord ScaleCoord(const nsCSSValue &aValue, float factor)
{
  return NSToCoordRoundWithClamp(aValue.GetFloatValue() * factor);
}

static nscoord CalcLengthWith(const nsCSSValue& aValue,
                              nscoord aFontSize,
                              const nsStyleFont* aStyleFont,
                              nsStyleContext* aStyleContext,
                              nsPresContext* aPresContext,
                              PRBool aUseProvidedRootEmSize,
                              
                              
                              
                              PRBool aUseUserFontSet,
                              PRBool& aCanStoreInRuleTree)
{
  NS_ASSERTION(aValue.IsLengthUnit() || aValue.IsCalcUnit(),
               "not a length or calc unit");
  NS_ASSERTION(aStyleFont || aStyleContext, "Must have style data");
  NS_ASSERTION(!aStyleFont || !aStyleContext, "Duplicate sources of data");
  NS_ASSERTION(aPresContext, "Must have prescontext");

  if (aValue.IsFixedLengthUnit()) {
    return aValue.GetFixedLength(aPresContext);
  }
  if (aValue.IsPixelLengthUnit()) {
    return aValue.GetPixelLength();
  }
  
  
  aCanStoreInRuleTree = PR_FALSE;
  const nsStyleFont *styleFont =
    aStyleFont ? aStyleFont : aStyleContext->GetStyleFont();
  if (aFontSize == -1) {
    
    
    aFontSize = styleFont->mFont.size;
  }
  switch (aValue.GetUnit()) {
    case eCSSUnit_RootEM: {
      nscoord rootFontSize;

      if (aUseProvidedRootEmSize) {
        
        
        
        
        
        rootFontSize = aFontSize;
      } else if (aStyleContext && !aStyleContext->GetParent()) {
        
        
        
        
        rootFontSize = styleFont->mFont.size;
      } else {
        
        
        nsRefPtr<nsStyleContext> rootStyle;
        const nsStyleFont *rootStyleFont = styleFont;
        Element* docElement = aPresContext->Document()->GetRootElement();

        if (docElement) {
          rootStyle = aPresContext->StyleSet()->ResolveStyleFor(docElement,
                                                                nsnull);
          if (rootStyle) {
            rootStyleFont = rootStyle->GetStyleFont();
          }
        }

        rootFontSize = rootStyleFont->mFont.size;
      }

      return ScaleCoord(aValue, float(rootFontSize));
    }
    case eCSSUnit_EM: {
      return ScaleCoord(aValue, float(aFontSize));
      
    }
    case eCSSUnit_XHeight: {
      nsFont font = styleFont->mFont;
      font.size = aFontSize;
      nsRefPtr<nsFontMetrics> fm =
        aPresContext->GetMetricsFor(font, aUseUserFontSet);
      return ScaleCoord(aValue, float(fm->XHeight()));
    }
    case eCSSUnit_Char: {
      nsFont font = styleFont->mFont;
      font.size = aFontSize;
      nsRefPtr<nsFontMetrics> fm =
        aPresContext->GetMetricsFor(font, aUseUserFontSet);
      gfxFloat zeroWidth = (fm->GetThebesFontGroup()->GetFontAt(0)
                            ->GetMetrics().zeroOrAveCharWidth);

      return ScaleCoord(aValue, NS_ceil(aPresContext->AppUnitsPerDevPixel() *
                                        zeroWidth));
    }
    
    
    
    
    
    case eCSSUnit_Calc:
    case eCSSUnit_Calc_Plus:
    case eCSSUnit_Calc_Minus:
    case eCSSUnit_Calc_Times_L:
    case eCSSUnit_Calc_Times_R:
    case eCSSUnit_Calc_Divided: {
      CalcLengthCalcOps ops(aFontSize, aStyleFont, aStyleContext, aPresContext,
                            aUseProvidedRootEmSize, aUseUserFontSet,
                            aCanStoreInRuleTree);
      return css::ComputeCalc(aValue, ops);
    }
    default:
      NS_NOTREACHED("unexpected unit");
      break;
  }
  return 0;
}

 nscoord
nsRuleNode::CalcLength(const nsCSSValue& aValue,
                       nsStyleContext* aStyleContext,
                       nsPresContext* aPresContext,
                       PRBool& aCanStoreInRuleTree)
{
  NS_ASSERTION(aStyleContext, "Must have style data");

  return CalcLengthWith(aValue, -1, nsnull, aStyleContext, aPresContext,
                        PR_FALSE, PR_TRUE, aCanStoreInRuleTree);
}


static inline nscoord CalcLength(const nsCSSValue& aValue,
                                 nsStyleContext* aStyleContext,
                                 nsPresContext* aPresContext,
                                 PRBool& aCanStoreInRuleTree)
{
  return nsRuleNode::CalcLength(aValue, aStyleContext,
                                aPresContext, aCanStoreInRuleTree);
}

 nscoord
nsRuleNode::CalcLengthWithInitialFont(nsPresContext* aPresContext,
                                      const nsCSSValue& aValue)
{
  nsStyleFont defaultFont(aPresContext);
  PRBool canStoreInRuleTree;
  return CalcLengthWith(aValue, -1, &defaultFont, nsnull, aPresContext,
                        PR_TRUE, PR_FALSE, canStoreInRuleTree);
}

struct LengthPercentPairCalcOps : public css::NumbersAlreadyNormalizedOps
{
  typedef nsRuleNode::ComputedCalc result_type;

  LengthPercentPairCalcOps(nsStyleContext* aContext,
                           nsPresContext* aPresContext,
                           PRBool& aCanStoreInRuleTree)
    : mContext(aContext),
      mPresContext(aPresContext),
      mCanStoreInRuleTree(aCanStoreInRuleTree),
      mHasPercent(PR_FALSE) {}

  nsStyleContext* mContext;
  nsPresContext* mPresContext;
  PRBool& mCanStoreInRuleTree;
  PRBool mHasPercent;

  result_type ComputeLeafValue(const nsCSSValue& aValue)
  {
    if (aValue.GetUnit() == eCSSUnit_Percent) {
      mHasPercent = PR_TRUE;
      return result_type(0, aValue.GetPercentValue());
    }
    return result_type(CalcLength(aValue, mContext, mPresContext,
                                  mCanStoreInRuleTree),
                       0.0f);
  }

  result_type
  MergeAdditive(nsCSSUnit aCalcFunction,
                result_type aValue1, result_type aValue2)
  {
    if (aCalcFunction == eCSSUnit_Calc_Plus) {
      return result_type(NSCoordSaturatingAdd(aValue1.mLength,
                                              aValue2.mLength),
                         aValue1.mPercent + aValue2.mPercent);
    }
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Minus,
                      "min() and max() are not allowed in calc() on "
                      "transform");
    return result_type(NSCoordSaturatingSubtract(aValue1.mLength,
                                                 aValue2.mLength, 0),
                       aValue1.mPercent - aValue2.mPercent);
  }

  result_type
  MergeMultiplicativeL(nsCSSUnit aCalcFunction,
                       float aValue1, result_type aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_L,
                      "unexpected unit");
    return result_type(NSCoordSaturatingMultiply(aValue2.mLength, aValue1),
                       aValue1 * aValue2.mPercent);
  }

  result_type
  MergeMultiplicativeR(nsCSSUnit aCalcFunction,
                       result_type aValue1, float aValue2)
  {
    NS_ABORT_IF_FALSE(aCalcFunction == eCSSUnit_Calc_Times_R ||
                      aCalcFunction == eCSSUnit_Calc_Divided,
                      "unexpected unit");
    if (aCalcFunction == eCSSUnit_Calc_Divided) {
      aValue2 = 1.0f / aValue2;
    }
    return result_type(NSCoordSaturatingMultiply(aValue1.mLength, aValue2),
                       aValue1.mPercent * aValue2);
  }

};

static void
SpecifiedCalcToComputedCalc(const nsCSSValue& aValue, nsStyleCoord& aCoord, 
                            nsStyleContext* aStyleContext,
                            PRBool& aCanStoreInRuleTree)
{
  LengthPercentPairCalcOps ops(aStyleContext, aStyleContext->PresContext(),
                               aCanStoreInRuleTree);
  nsRuleNode::ComputedCalc vals = ComputeCalc(aValue, ops);

  nsStyleCoord::Calc *calcObj =
    new (aStyleContext->Alloc(sizeof(nsStyleCoord::Calc))) nsStyleCoord::Calc;
  
  
  aCanStoreInRuleTree = PR_FALSE;

  calcObj->mLength = vals.mLength;
  calcObj->mPercent = vals.mPercent;
  calcObj->mHasPercent = ops.mHasPercent;

  aCoord.SetCalcValue(calcObj);
}

 nsRuleNode::ComputedCalc
nsRuleNode::SpecifiedCalcToComputedCalc(const nsCSSValue& aValue,
                                        nsStyleContext* aStyleContext,
                                        nsPresContext* aPresContext,
                                        PRBool& aCanStoreInRuleTree)
{
  LengthPercentPairCalcOps ops(aStyleContext, aPresContext,
                               aCanStoreInRuleTree);
  return ComputeCalc(aValue, ops);
}



 nscoord
nsRuleNode::ComputeComputedCalc(const nsStyleCoord& aValue,
                                nscoord aPercentageBasis)
{
  nsStyleCoord::Calc *calc = aValue.GetCalcValue();
  return calc->mLength +
         NSToCoordFloorClamped(aPercentageBasis * calc->mPercent);
}

 nscoord
nsRuleNode::ComputeCoordPercentCalc(const nsStyleCoord& aCoord,
                                    nscoord aPercentageBasis)
{
  switch (aCoord.GetUnit()) {
    case eStyleUnit_Coord:
      return aCoord.GetCoordValue();
    case eStyleUnit_Percent:
      return NSToCoordFloorClamped(aPercentageBasis * aCoord.GetPercentValue());
    case eStyleUnit_Calc:
      return ComputeComputedCalc(aCoord, aPercentageBasis);
    default:
      NS_ABORT_IF_FALSE(PR_FALSE, "unexpected unit");
      return 0;
  }
}








static float
GetFloatFromBoxPosition(PRInt32 aEnumValue)
{
  switch (aEnumValue) {
  case NS_STYLE_BG_POSITION_LEFT:
  case NS_STYLE_BG_POSITION_TOP:
    return 0.0f;
  case NS_STYLE_BG_POSITION_RIGHT:
  case NS_STYLE_BG_POSITION_BOTTOM:
    return 1.0f;
  default:
    NS_NOTREACHED("unexpected value");
    
  case NS_STYLE_BG_POSITION_CENTER:
    return 0.5f;
  }
}

#define SETCOORD_NORMAL                 0x01   // N
#define SETCOORD_AUTO                   0x02   // A
#define SETCOORD_INHERIT                0x04   // H
#define SETCOORD_PERCENT                0x08   // P
#define SETCOORD_FACTOR                 0x10   // F
#define SETCOORD_LENGTH                 0x20   // L
#define SETCOORD_INTEGER                0x40   // I
#define SETCOORD_ENUMERATED             0x80   // E
#define SETCOORD_NONE                   0x100  // O
#define SETCOORD_INITIAL_ZERO           0x200
#define SETCOORD_INITIAL_AUTO           0x400
#define SETCOORD_INITIAL_NONE           0x800
#define SETCOORD_INITIAL_NORMAL         0x1000
#define SETCOORD_INITIAL_HALF           0x2000
#define SETCOORD_CALC_LENGTH_ONLY       0x4000
#define SETCOORD_CALC_CLAMP_NONNEGATIVE 0x8000 // modifier for CALC_LENGTH_ONLY
#define SETCOORD_STORE_CALC             0x00010000
#define SETCOORD_BOX_POSITION           0x00020000 // exclusive with _ENUMERATED

#define SETCOORD_LP     (SETCOORD_LENGTH | SETCOORD_PERCENT)
#define SETCOORD_LH     (SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_AH     (SETCOORD_AUTO | SETCOORD_INHERIT)
#define SETCOORD_LAH    (SETCOORD_AUTO | SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_LPH    (SETCOORD_LP | SETCOORD_INHERIT)
#define SETCOORD_LPAH   (SETCOORD_LP | SETCOORD_AH)
#define SETCOORD_LPEH   (SETCOORD_LP | SETCOORD_ENUMERATED | SETCOORD_INHERIT)
#define SETCOORD_LPAEH  (SETCOORD_LPAH | SETCOORD_ENUMERATED)
#define SETCOORD_LPO    (SETCOORD_LP | SETCOORD_NONE)
#define SETCOORD_LPOH   (SETCOORD_LPH | SETCOORD_NONE)
#define SETCOORD_LPOEH  (SETCOORD_LPOH | SETCOORD_ENUMERATED)
#define SETCOORD_LE     (SETCOORD_LENGTH | SETCOORD_ENUMERATED)
#define SETCOORD_LEH    (SETCOORD_LE | SETCOORD_INHERIT)
#define SETCOORD_IA     (SETCOORD_INTEGER | SETCOORD_AUTO)
#define SETCOORD_LAE    (SETCOORD_LENGTH | SETCOORD_AUTO | SETCOORD_ENUMERATED)


static PRBool SetCoord(const nsCSSValue& aValue, nsStyleCoord& aCoord,
                       const nsStyleCoord& aParentCoord,
                       PRInt32 aMask, nsStyleContext* aStyleContext,
                       nsPresContext* aPresContext,
                       PRBool& aCanStoreInRuleTree)
{
  PRBool  result = PR_TRUE;
  if (aValue.GetUnit() == eCSSUnit_Null) {
    result = PR_FALSE;
  }
  else if ((((aMask & SETCOORD_LENGTH) != 0) &&
            aValue.IsLengthUnit()) ||
           (((aMask & SETCOORD_CALC_LENGTH_ONLY) != 0) &&
            aValue.IsCalcUnit())) {
    nscoord len = CalcLength(aValue, aStyleContext, aPresContext,
                             aCanStoreInRuleTree);
    if ((aMask & SETCOORD_CALC_CLAMP_NONNEGATIVE) && len < 0) {
      NS_ASSERTION(aValue.IsCalcUnit(),
                   "parser should have ensured no nonnegative lengths");
      len = 0;
    }
    aCoord.SetCoordValue(len);
  }
  else if (((aMask & SETCOORD_PERCENT) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Percent)) {
    aCoord.SetPercentValue(aValue.GetPercentValue());
  }
  else if (((aMask & SETCOORD_INTEGER) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Integer)) {
    aCoord.SetIntValue(aValue.GetIntValue(), eStyleUnit_Integer);
  }
  else if (((aMask & SETCOORD_ENUMERATED) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Enumerated)) {
    aCoord.SetIntValue(aValue.GetIntValue(), eStyleUnit_Enumerated);
  }
  else if (((aMask & SETCOORD_BOX_POSITION) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Enumerated)) {
    aCoord.SetPercentValue(GetFloatFromBoxPosition(aValue.GetIntValue()));
  }
  else if (((aMask & SETCOORD_AUTO) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Auto)) {
    aCoord.SetAutoValue();
  }
  else if (((aMask & SETCOORD_INHERIT) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Inherit)) {
    aCoord = aParentCoord;  
    aCanStoreInRuleTree = PR_FALSE;
  }
  else if (((aMask & SETCOORD_NORMAL) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Normal)) {
    aCoord.SetNormalValue();
  }
  else if (((aMask & SETCOORD_NONE) != 0) &&
           (aValue.GetUnit() == eCSSUnit_None)) {
    aCoord.SetNoneValue();
  }
  else if (((aMask & SETCOORD_FACTOR) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Number)) {
    aCoord.SetFactorValue(aValue.GetFloatValue());
  }
  else if (((aMask & SETCOORD_INITIAL_AUTO) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Initial)) {
    aCoord.SetAutoValue();
  }
  else if (((aMask & SETCOORD_INITIAL_ZERO) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Initial)) {
    aCoord.SetCoordValue(0);
  }
  else if (((aMask & SETCOORD_INITIAL_NONE) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Initial)) {
    aCoord.SetNoneValue();
  }
  else if (((aMask & SETCOORD_INITIAL_NORMAL) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Initial)) {
    aCoord.SetNormalValue();
  }
  else if (((aMask & SETCOORD_INITIAL_HALF) != 0) &&
           (aValue.GetUnit() == eCSSUnit_Initial)) {
    aCoord.SetPercentValue(0.5f);
  }
  else if (((aMask & SETCOORD_STORE_CALC) != 0) &&
           (aValue.IsCalcUnit())) {
    SpecifiedCalcToComputedCalc(aValue, aCoord, aStyleContext,
                                aCanStoreInRuleTree);
  }
  else {
    result = PR_FALSE;  
  }
  return result;
}



static inline PRBool SetAbsCoord(const nsCSSValue& aValue,
                                 nsStyleCoord& aCoord,
                                 PRInt32 aMask)
{
  NS_ABORT_IF_FALSE((aMask & SETCOORD_LH) == 0,
                    "does not handle SETCOORD_LENGTH and SETCOORD_INHERIT");

  
  
  const nsStyleCoord dummyParentCoord;
  nsStyleContext* dummyStyleContext = nsnull;
  nsPresContext* dummyPresContext = nsnull;
  PRBool dummyCanStoreInRuleTree = PR_TRUE;

  PRBool rv = SetCoord(aValue, aCoord, dummyParentCoord, aMask,
                       dummyStyleContext, dummyPresContext,
                       dummyCanStoreInRuleTree);
  NS_ABORT_IF_FALSE(dummyCanStoreInRuleTree,
                    "SetCoord() should not modify dummyCanStoreInRuleTree.");

  return rv;
}




static PRBool
SetPairCoords(const nsCSSValue& aValue,
              nsStyleCoord& aCoordX, nsStyleCoord& aCoordY,
              const nsStyleCoord& aParentX, const nsStyleCoord& aParentY,
              PRInt32 aMask, nsStyleContext* aStyleContext,
              nsPresContext* aPresContext, PRBool& aCanStoreInRuleTree)
{
  const nsCSSValue& valX =
    aValue.GetUnit() == eCSSUnit_Pair ? aValue.GetPairValue().mXValue : aValue;
  const nsCSSValue& valY =
    aValue.GetUnit() == eCSSUnit_Pair ? aValue.GetPairValue().mYValue : aValue;

  PRBool cX = SetCoord(valX, aCoordX, aParentX, aMask, aStyleContext,
                       aPresContext, aCanStoreInRuleTree);
  PRBool cY = SetCoord(valY, aCoordY, aParentY, aMask, aStyleContext,
                       aPresContext, aCanStoreInRuleTree);
  NS_ABORT_IF_FALSE(cX == cY, "changed one but not the other");
  return cX;
}

static PRBool SetColor(const nsCSSValue& aValue, const nscolor aParentColor,
                       nsPresContext* aPresContext, nsStyleContext *aContext,
                       nscolor& aResult, PRBool& aCanStoreInRuleTree)
{
  PRBool  result = PR_FALSE;
  nsCSSUnit unit = aValue.GetUnit();

  if (eCSSUnit_Color == unit) {
    aResult = aValue.GetColorValue();
    result = PR_TRUE;
  }
  else if (eCSSUnit_Ident == unit) {
    nsAutoString  value;
    aValue.GetStringValue(value);
    nscolor rgba;
    if (NS_ColorNameToRGB(value, &rgba)) {
      aResult = rgba;
      result = PR_TRUE;
    }
  }
  else if (eCSSUnit_EnumColor == unit) {
    PRInt32 intValue = aValue.GetIntValue();
    if (0 <= intValue) {
      nsILookAndFeel* look = aPresContext->LookAndFeel();
      nsILookAndFeel::nsColorID colorID = (nsILookAndFeel::nsColorID) intValue;
      if (NS_SUCCEEDED(look->GetColor(colorID, aResult))) {
        result = PR_TRUE;
      }
    }
    else {
      switch (intValue) {
        case NS_COLOR_MOZ_HYPERLINKTEXT:
          aResult = aPresContext->DefaultLinkColor();
          break;
        case NS_COLOR_MOZ_VISITEDHYPERLINKTEXT:
          aResult = aPresContext->DefaultVisitedLinkColor();
          break;
        case NS_COLOR_MOZ_ACTIVEHYPERLINKTEXT:
          aResult = aPresContext->DefaultActiveLinkColor();
          break;
        case NS_COLOR_CURRENTCOLOR:
          
          
          aCanStoreInRuleTree = PR_FALSE;
          aResult = aContext->GetStyleColor()->mColor;
          break;
        case NS_COLOR_MOZ_DEFAULT_COLOR:
          aResult = aPresContext->DefaultColor();
          break;
        case NS_COLOR_MOZ_DEFAULT_BACKGROUND_COLOR:
          aResult = aPresContext->DefaultBackgroundColor();
          break;
        default:
          NS_NOTREACHED("Should never have an unknown negative colorID.");
          break;
      }
      result = PR_TRUE;
    }
  }
  else if (eCSSUnit_Inherit == unit) {
    aResult = aParentColor;
    result = PR_TRUE;
    aCanStoreInRuleTree = PR_FALSE;
  }
  return result;
}

static void SetGradientCoord(const nsCSSValue& aValue, nsPresContext* aPresContext,
                             nsStyleContext* aContext, nsStyleCoord& aResult,
                             PRBool& aCanStoreInRuleTree)
{
  
  if (!SetCoord(aValue, aResult, nsStyleCoord(),
                SETCOORD_LPO | SETCOORD_BOX_POSITION | SETCOORD_STORE_CALC,
                aContext, aPresContext, aCanStoreInRuleTree)) {
    NS_NOTREACHED("unexpected unit for gradient anchor point");
    aResult.SetNoneValue();
  }
}

static void SetGradient(const nsCSSValue& aValue, nsPresContext* aPresContext,
                        nsStyleContext* aContext, nsStyleGradient& aResult,
                        PRBool& aCanStoreInRuleTree)
{
  NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Gradient,
                    "The given data is not a gradient");

  nsCSSValueGradient* gradient = aValue.GetGradientValue();

  if (gradient->mIsRadial) {
    if (gradient->mRadialShape.GetUnit() == eCSSUnit_Enumerated) {
      aResult.mShape = gradient->mRadialShape.GetIntValue();
    } else {
      NS_ASSERTION(gradient->mRadialShape.GetUnit() == eCSSUnit_None,
                   "bad unit for radial shape");
      aResult.mShape = NS_STYLE_GRADIENT_SHAPE_ELLIPTICAL;
    }
    if (gradient->mRadialSize.GetUnit() == eCSSUnit_Enumerated) {
      aResult.mSize = gradient->mRadialSize.GetIntValue();
    } else {
      NS_ASSERTION(gradient->mRadialSize.GetUnit() == eCSSUnit_None,
                   "bad unit for radial shape");
      aResult.mSize = NS_STYLE_GRADIENT_SIZE_FARTHEST_CORNER;
    }
  } else {
    NS_ASSERTION(gradient->mRadialShape.GetUnit() == eCSSUnit_None,
                 "bad unit for linear shape");
    NS_ASSERTION(gradient->mRadialSize.GetUnit() == eCSSUnit_None,
                 "bad unit for linear size");
    aResult.mShape = NS_STYLE_GRADIENT_SHAPE_LINEAR;
    aResult.mSize = NS_STYLE_GRADIENT_SIZE_FARTHEST_CORNER;
  }

  
  SetGradientCoord(gradient->mBgPos.mXValue, aPresContext, aContext,
                   aResult.mBgPosX, aCanStoreInRuleTree);

  SetGradientCoord(gradient->mBgPos.mYValue, aPresContext, aContext,
                   aResult.mBgPosY, aCanStoreInRuleTree);

  aResult.mRepeating = gradient->mIsRepeating;

  
  if (gradient->mAngle.IsAngularUnit()) {
    nsStyleUnit unit;
    switch (gradient->mAngle.GetUnit()) {
    case eCSSUnit_Degree: unit = eStyleUnit_Degree; break;
    case eCSSUnit_Grad:   unit = eStyleUnit_Grad; break;
    case eCSSUnit_Radian: unit = eStyleUnit_Radian; break;
    default: NS_NOTREACHED("unrecognized angular unit");
      unit = eStyleUnit_Degree;
    }
    aResult.mAngle.SetAngleValue(gradient->mAngle.GetAngleValue(), unit);
  } else {
    NS_ASSERTION(gradient->mAngle.GetUnit() == eCSSUnit_None,
                 "bad unit for gradient angle");
    aResult.mAngle.SetNoneValue();
  }

  
  for (PRUint32 i = 0; i < gradient->mStops.Length(); i++) {
    nsStyleGradientStop stop;
    nsCSSValueGradientStop &valueStop = gradient->mStops[i];

    if (!SetCoord(valueStop.mLocation, stop.mLocation,
                  nsStyleCoord(), SETCOORD_LPO,
                  aContext, aPresContext, aCanStoreInRuleTree)) {
      NS_NOTREACHED("unexpected unit for gradient stop location");
    }

    
    
    NS_ASSERTION(valueStop.mColor.GetUnit() != eCSSUnit_Inherit,
                 "inherit is not a valid color for gradient stops");
    SetColor(valueStop.mColor, NS_RGB(0, 0, 0), aPresContext,
             aContext, stop.mColor, aCanStoreInRuleTree);

    aResult.mStops.AppendElement(stop);
  }
}


static void SetStyleImageToImageRect(nsStyleContext* aStyleContext,
                                     const nsCSSValue& aValue,
                                     nsStyleImage& aResult)
{
  NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Function &&
                    aValue.EqualsFunction(eCSSKeyword__moz_image_rect),
                    "the value is not valid -moz-image-rect()");

  nsCSSValue::Array* arr = aValue.GetArrayValue();
  NS_ABORT_IF_FALSE(arr && arr->Count() == 6, "invalid number of arguments");

  
  if (arr->Item(1).GetUnit() == eCSSUnit_Image) {
    NS_SET_IMAGE_REQUEST(aResult.SetImageData,
                         aStyleContext,
                         arr->Item(1).GetImageValue())
  } else {
    NS_WARNING("nsCSSValue::Image::Image() failed?");
  }

  
  nsStyleSides cropRect;
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord coord;
    const nsCSSValue& val = arr->Item(2 + side);

#ifdef DEBUG
    PRBool unitOk =
#endif
      SetAbsCoord(val, coord, SETCOORD_FACTOR | SETCOORD_PERCENT);
    NS_ABORT_IF_FALSE(unitOk, "Incorrect data structure created by CSS parser");
    cropRect.Set(side, coord);
  }
  aResult.SetCropRect(&cropRect);
}

static void SetStyleImage(nsStyleContext* aStyleContext,
                          const nsCSSValue& aValue,
                          nsStyleImage& aResult,
                          PRBool& aCanStoreInRuleTree)
{
  aResult.SetNull();

  switch (aValue.GetUnit()) {
    case eCSSUnit_Image:
      NS_SET_IMAGE_REQUEST(aResult.SetImageData,
                           aStyleContext,
                           aValue.GetImageValue())
      break;
    case eCSSUnit_Function:
      if (aValue.EqualsFunction(eCSSKeyword__moz_image_rect)) {
        SetStyleImageToImageRect(aStyleContext, aValue, aResult);
      } else {
        NS_NOTREACHED("-moz-image-rect() is the only expected function");
      }
      break;
    case eCSSUnit_Gradient:
    {
      nsStyleGradient* gradient = new nsStyleGradient();
      if (gradient) {
        SetGradient(aValue, aStyleContext->PresContext(), aStyleContext,
                    *gradient, aCanStoreInRuleTree);
        aResult.SetGradientData(gradient);
      }
      break;
    }
    case eCSSUnit_Element:
      aResult.SetElementId(aValue.GetStringBufferValue());
      break;
    case eCSSUnit_None:
      break;
    default:
      
      
      
      NS_ASSERTION(aStyleContext->IsStyleIfVisited() &&
                   aValue.GetUnit() == eCSSUnit_URL,
                   "unexpected unit; maybe nsCSSValue::Image::Image() failed?");
      break;
  }
}




#define SETDSC_NORMAL                 0x01   // N
#define SETDSC_AUTO                   0x02   // A
#define SETDSC_INTEGER                0x40   // I
#define SETDSC_ENUMERATED             0x80   // E
#define SETDSC_NONE                   0x100  // O
#define SETDSC_SYSTEM_FONT            0x2000


template <typename FieldT,
          typename T1, typename T2, typename T3, typename T4, typename T5>
static void
SetDiscrete(const nsCSSValue& aValue, FieldT & aField,
            PRBool& aCanStoreInRuleTree, PRUint32 aMask,
            FieldT aParentValue,
            T1 aInitialValue,
            T2 aAutoValue,
            T3 aNoneValue,
            T4 aNormalValue,
            T5 aSystemFontValue)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    return;

    
    
  case eCSSUnit_Inherit:
    aCanStoreInRuleTree = PR_FALSE;
    aField = aParentValue;
    return;

  case eCSSUnit_Initial:
    aField = aInitialValue;
    return;

    
    
  case eCSSUnit_Enumerated:
    if (aMask & SETDSC_ENUMERATED) {
      aField = aValue.GetIntValue();
      return;
    }
    break;

  case eCSSUnit_Integer:
    if (aMask & SETDSC_INTEGER) {
      aField = aValue.GetIntValue();
      return;
    }
    break;

    
  case eCSSUnit_Auto:
    if (aMask & SETDSC_AUTO) {
      aField = aAutoValue;
      return;
    }
    break;

  case eCSSUnit_None:
    if (aMask & SETDSC_NONE) {
      aField = aNoneValue;
      return;
    }
    break;

  case eCSSUnit_Normal:
    if (aMask & SETDSC_NORMAL) {
      aField = aNormalValue;
      return;
    }
    break;

  case eCSSUnit_System_Font:
    if (aMask & SETDSC_SYSTEM_FONT) {
      aField = aSystemFontValue;
      return;
    }
    break;

  default:
    break;
  }

  NS_NOTREACHED("SetDiscrete: inappropriate unit");
}


#define SETFCT_POSITIVE 0x01        // assert value is >= 0.0f
#define SETFCT_OPACITY  0x02        // clamp value to [0.0f .. 1.0f]
#define SETFCT_NONE     0x04        // allow _None (uses aInitialValue).

static void
SetFactor(const nsCSSValue& aValue, float& aField, PRBool& aCanStoreInRuleTree,
          float aParentValue, float aInitialValue, PRUint32 aFlags = 0)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    return;

  case eCSSUnit_Number:
    aField = aValue.GetFloatValue();
    if (aFlags & SETFCT_POSITIVE) {
      NS_ASSERTION(aField >= 0.0f, "negative value for positive-only property");
      if (aField < 0.0f)
        aField = 0.0f;
    }
    if (aFlags & SETFCT_OPACITY) {
      if (aField < 0.0f)
        aField = 0.0f;
      if (aField > 1.0f)
        aField = 1.0f;
    }
    return;

  case eCSSUnit_Inherit:
    aCanStoreInRuleTree = PR_FALSE;
    aField = aParentValue;
    return;

  case eCSSUnit_Initial:
    aField = aInitialValue;
    return;

  case eCSSUnit_None:
    if (aFlags & SETFCT_NONE) {
      aField = aInitialValue;
      return;
    }
    break;

  default:
    break;
  }

  NS_NOTREACHED("SetFactor: inappropriate unit");
}



void*
nsRuleNode::operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW
{
  
  return aPresContext->AllocateFromShell(sz);
}

 PLDHashOperator
nsRuleNode::EnqueueRuleNodeChildren(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                    PRUint32 number, void *arg)
{
  ChildrenHashEntry *entry = static_cast<ChildrenHashEntry*>(hdr);
  nsRuleNode ***destroyQueueTail = static_cast<nsRuleNode***>(arg);
  **destroyQueueTail = entry->mRuleNode;
  *destroyQueueTail = &entry->mRuleNode->mNextSibling;
  return PL_DHASH_NEXT;
}



void
nsRuleNode::DestroyInternal(nsRuleNode ***aDestroyQueueTail)
{
  nsRuleNode *destroyQueue, **destroyQueueTail;
  if (aDestroyQueueTail) {
    destroyQueueTail = *aDestroyQueueTail;
  } else {
    destroyQueue = nsnull;
    destroyQueueTail = &destroyQueue;
  }

  if (ChildrenAreHashed()) {
    PLDHashTable *children = ChildrenHash();
    PL_DHashTableEnumerate(children, EnqueueRuleNodeChildren,
                           &destroyQueueTail);
    *destroyQueueTail = nsnull; 
    PL_DHashTableDestroy(children);
  } else if (HaveChildren()) {
    *destroyQueueTail = ChildrenList();
    do {
      destroyQueueTail = &(*destroyQueueTail)->mNextSibling;
    } while (*destroyQueueTail);
  }
  mChildren.asVoid = nsnull;

  if (aDestroyQueueTail) {
    
    *aDestroyQueueTail = destroyQueueTail;
  } else {
    
    
    while (destroyQueue) {
      nsRuleNode *cur = destroyQueue;
      destroyQueue = destroyQueue->mNextSibling;
      if (!destroyQueue) {
        NS_ASSERTION(destroyQueueTail == &cur->mNextSibling, "mangled list");
        destroyQueueTail = &destroyQueue;
      }
      cur->DestroyInternal(&destroyQueueTail);
    }
  }

  
  this->~nsRuleNode();

  
  
  mPresContext->FreeToShell(sizeof(nsRuleNode), this);
}

nsRuleNode* nsRuleNode::CreateRootNode(nsPresContext* aPresContext)
{
  return new (aPresContext)
    nsRuleNode(aPresContext, nsnull, nsnull, 0xff, PR_FALSE);
}

nsILanguageAtomService* nsRuleNode::gLangService = nsnull;

nsRuleNode::nsRuleNode(nsPresContext* aContext, nsRuleNode* aParent,
                       nsIStyleRule* aRule, PRUint8 aLevel,
                       PRBool aIsImportant)
  : mPresContext(aContext),
    mParent(aParent),
    mRule(aRule),
    mDependentBits((PRUint32(aLevel) << NS_RULE_NODE_LEVEL_SHIFT) |
                   (aIsImportant ? NS_RULE_NODE_IS_IMPORTANT : 0)),
    mNoneBits(0),
    mRefCnt(0)
{
  mChildren.asVoid = nsnull;
  MOZ_COUNT_CTOR(nsRuleNode);
  NS_IF_ADDREF(mRule);

  NS_ASSERTION(IsRoot() || GetLevel() == aLevel, "not enough bits");
  NS_ASSERTION(IsRoot() || IsImportantRule() == aIsImportant, "yikes");
  


  if (!IsRoot()) {
    mParent->AddRef();
    aContext->StyleSet()->RuleNodeUnused();
  }

  
  
  NS_ABORT_IF_FALSE(IsRoot() || GetLevel() != nsStyleSet::eAnimationSheet ||
                    mParent->IsRoot() ||
                    mParent->GetLevel() != nsStyleSet::eAnimationSheet,
                    "must be only one rule at animation level");
}

nsRuleNode::~nsRuleNode()
{
  MOZ_COUNT_DTOR(nsRuleNode);
  if (mStyleData.mResetData || mStyleData.mInheritedData)
    mStyleData.Destroy(0, mPresContext);
  NS_IF_RELEASE(mRule);
}

nsRuleNode*
nsRuleNode::Transition(nsIStyleRule* aRule, PRUint8 aLevel,
                       PRPackedBool aIsImportantRule)
{
  nsRuleNode* next = nsnull;
  nsRuleNode::Key key(aRule, aLevel, aIsImportantRule);

  if (HaveChildren() && !ChildrenAreHashed()) {
    PRInt32 numKids = 0;
    nsRuleNode* curr = ChildrenList();
    while (curr && curr->GetKey() != key) {
      curr = curr->mNextSibling;
      ++numKids;
    }
    if (curr)
      next = curr;
    else if (numKids >= kMaxChildrenInList)
      ConvertChildrenToHash();
  }

  if (ChildrenAreHashed()) {
    ChildrenHashEntry *entry = static_cast<ChildrenHashEntry*>
                                          (PL_DHashTableOperate(ChildrenHash(), &key, PL_DHASH_ADD));
    if (!entry) {
      NS_WARNING("out of memory");
      return this;
    }
    if (entry->mRuleNode)
      next = entry->mRuleNode;
    else {
      next = entry->mRuleNode = new (mPresContext)
        nsRuleNode(mPresContext, this, aRule, aLevel, aIsImportantRule);
      if (!next) {
        PL_DHashTableRawRemove(ChildrenHash(), entry);
        NS_WARNING("out of memory");
        return this;
      }
    }
  } else if (!next) {
    
    next = new (mPresContext)
      nsRuleNode(mPresContext, this, aRule, aLevel, aIsImportantRule);
    if (!next) {
      NS_WARNING("out of memory");
      return this;
    }
    next->mNextSibling = ChildrenList();
    SetChildrenList(next);
  }

  return next;
}

void
nsRuleNode::ConvertChildrenToHash()
{
  NS_ASSERTION(!ChildrenAreHashed() && HaveChildren(),
               "must have a non-empty list of children");
  PLDHashTable *hash = PL_NewDHashTable(&ChildrenHashOps, nsnull,
                                        sizeof(ChildrenHashEntry),
                                        kMaxChildrenInList * 4);
  if (!hash)
    return;
  for (nsRuleNode* curr = ChildrenList(); curr; curr = curr->mNextSibling) {
    
    ChildrenHashEntry *entry = static_cast<ChildrenHashEntry*>(
      PL_DHashTableOperate(hash, curr->mRule, PL_DHASH_ADD));
    NS_ASSERTION(!entry->mRuleNode, "duplicate entries in list");
    entry->mRuleNode = curr;
  }
  SetChildrenHash(hash);
}

inline void
nsRuleNode::PropagateNoneBit(PRUint32 aBit, nsRuleNode* aHighestNode)
{
  nsRuleNode* curr = this;
  for (;;) {
    NS_ASSERTION(!(curr->mNoneBits & aBit), "propagating too far");
    curr->mNoneBits |= aBit;
    if (curr == aHighestNode)
      break;
    curr = curr->mParent;
  }
}

inline void
nsRuleNode::PropagateDependentBit(PRUint32 aBit, nsRuleNode* aHighestNode)
{
  for (nsRuleNode* curr = this; curr != aHighestNode; curr = curr->mParent) {
    if (curr->mDependentBits & aBit) {
#ifdef DEBUG
      while (curr != aHighestNode) {
        NS_ASSERTION(curr->mDependentBits & aBit, "bit not set");
        curr = curr->mParent;
      }
#endif
      break;
    }

    curr->mDependentBits |= aBit;
  }
}











typedef nsRuleNode::RuleDetail
  (* CheckCallbackFn)(const nsRuleData* aRuleData,
                      nsRuleNode::RuleDetail aResult);






inline void
ExamineCSSValue(const nsCSSValue& aValue,
                PRUint32& aSpecifiedCount, PRUint32& aInheritedCount)
{
  if (aValue.GetUnit() != eCSSUnit_Null) {
    ++aSpecifiedCount;
    if (aValue.GetUnit() == eCSSUnit_Inherit) {
      ++aInheritedCount;
    }
  }
}

static nsRuleNode::RuleDetail
CheckFontCallback(const nsRuleData* aRuleData,
                  nsRuleNode::RuleDetail aResult)
{
  
  
  
  
  const nsCSSValue& size = *aRuleData->ValueForFontSize();
  const nsCSSValue& weight = *aRuleData->ValueForFontWeight();
  if (size.IsRelativeLengthUnit() ||
      size.GetUnit() == eCSSUnit_Percent ||
      (size.GetUnit() == eCSSUnit_Enumerated &&
       (size.GetIntValue() == NS_STYLE_FONT_SIZE_SMALLER ||
        size.GetIntValue() == NS_STYLE_FONT_SIZE_LARGER)) ||
#ifdef MOZ_MATHML
      aRuleData->ValueForScriptLevel()->GetUnit() == eCSSUnit_Integer ||
#endif
      (weight.GetUnit() == eCSSUnit_Enumerated &&
       (weight.GetIntValue() == NS_STYLE_FONT_WEIGHT_BOLDER ||
        weight.GetIntValue() == NS_STYLE_FONT_WEIGHT_LIGHTER))) {
    NS_ASSERTION(aResult == nsRuleNode::eRulePartialReset ||
                 aResult == nsRuleNode::eRuleFullReset ||
                 aResult == nsRuleNode::eRulePartialMixed ||
                 aResult == nsRuleNode::eRuleFullMixed,
                 "we know we already have a reset-counted property");
    
    
    
    if (aResult == nsRuleNode::eRulePartialReset)
      aResult = nsRuleNode::eRulePartialMixed;
    else if (aResult == nsRuleNode::eRuleFullReset)
      aResult = nsRuleNode::eRuleFullMixed;
  }

  return aResult;
}

static nsRuleNode::RuleDetail
CheckColorCallback(const nsRuleData* aRuleData,
                   nsRuleNode::RuleDetail aResult)
{
  
  const nsCSSValue* colorValue = aRuleData->ValueForColor();
  if (colorValue->GetUnit() == eCSSUnit_EnumColor &&
      colorValue->GetIntValue() == NS_COLOR_CURRENTCOLOR) {
    NS_ASSERTION(aResult == nsRuleNode::eRuleFullReset,
                 "we should already be counted as full-reset");
    aResult = nsRuleNode::eRuleFullInherited;
  }

  return aResult;
}

static nsRuleNode::RuleDetail
CheckTextCallback(const nsRuleData* aRuleData,
                  nsRuleNode::RuleDetail aResult)
{
  const nsCSSValue* textAlignValue = aRuleData->ValueForTextAlign();
  if (textAlignValue->GetUnit() == eCSSUnit_Enumerated &&
      textAlignValue->GetIntValue() ==
        NS_STYLE_TEXT_ALIGN_MOZ_CENTER_OR_INHERIT) {
    
    
    if (aResult == nsRuleNode::eRulePartialReset)
      aResult = nsRuleNode::eRulePartialMixed;
    else if (aResult == nsRuleNode::eRuleFullReset)
      aResult = nsRuleNode::eRuleFullMixed;
  }

  return aResult;
}

#define FLAG_DATA_FOR_PROPERTY(name_, id_, method_, flags_, parsevariant_,   \
                               kwtable_, stylestructoffset_, animtype_)      \
  flags_,



static const PRUint32 gFontFlags[] = {
#define CSS_PROP_FONT FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_FONT
};

static const PRUint32 gDisplayFlags[] = {
#define CSS_PROP_DISPLAY FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_DISPLAY
};

static const PRUint32 gVisibilityFlags[] = {
#define CSS_PROP_VISIBILITY FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_VISIBILITY
};

static const PRUint32 gMarginFlags[] = {
#define CSS_PROP_MARGIN FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_MARGIN
};

static const PRUint32 gBorderFlags[] = {
#define CSS_PROP_BORDER FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_BORDER
};

static const PRUint32 gPaddingFlags[] = {
#define CSS_PROP_PADDING FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_PADDING
};

static const PRUint32 gOutlineFlags[] = {
#define CSS_PROP_OUTLINE FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_OUTLINE
};

static const PRUint32 gListFlags[] = {
#define CSS_PROP_LIST FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_LIST
};

static const PRUint32 gColorFlags[] = {
#define CSS_PROP_COLOR FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_COLOR
};

static const PRUint32 gBackgroundFlags[] = {
#define CSS_PROP_BACKGROUND FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_BACKGROUND
};

static const PRUint32 gPositionFlags[] = {
#define CSS_PROP_POSITION FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_POSITION
};

static const PRUint32 gTableFlags[] = {
#define CSS_PROP_TABLE FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TABLE
};

static const PRUint32 gTableBorderFlags[] = {
#define CSS_PROP_TABLEBORDER FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TABLEBORDER
};

static const PRUint32 gContentFlags[] = {
#define CSS_PROP_CONTENT FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_CONTENT
};

static const PRUint32 gQuotesFlags[] = {
#define CSS_PROP_QUOTES FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_QUOTES
};

static const PRUint32 gTextFlags[] = {
#define CSS_PROP_TEXT FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TEXT
};

static const PRUint32 gTextResetFlags[] = {
#define CSS_PROP_TEXTRESET FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TEXTRESET
};

static const PRUint32 gUserInterfaceFlags[] = {
#define CSS_PROP_USERINTERFACE FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_USERINTERFACE
};

static const PRUint32 gUIResetFlags[] = {
#define CSS_PROP_UIRESET FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_UIRESET
};

static const PRUint32 gXULFlags[] = {
#define CSS_PROP_XUL FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_XUL
};

static const PRUint32 gSVGFlags[] = {
#define CSS_PROP_SVG FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_SVG
};

static const PRUint32 gSVGResetFlags[] = {
#define CSS_PROP_SVGRESET FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_SVGRESET
};

static const PRUint32 gColumnFlags[] = {
#define CSS_PROP_COLUMN FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_COLUMN
};

#undef FLAG_DATA_FOR_PROPERTY

static const PRUint32* gFlagsByStruct[] = {

#define STYLE_STRUCT(name, checkdata_cb, ctor_args) \
  g##name##Flags,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

};

static const CheckCallbackFn gCheckCallbacks[] = {

#define STYLE_STRUCT(name, checkdata_cb, ctor_args) \
  checkdata_cb,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

};

#if defined(MOZ_MATHML) && defined(DEBUG)
static PRBool
AreAllMathMLPropertiesUndefined(const nsRuleData* aRuleData)
{
  return
    aRuleData->ValueForScriptLevel()->GetUnit() == eCSSUnit_Null &&
    aRuleData->ValueForScriptSizeMultiplier()->GetUnit() == eCSSUnit_Null &&
    aRuleData->ValueForScriptMinSize()->GetUnit() == eCSSUnit_Null;
}
#endif

inline nsRuleNode::RuleDetail
nsRuleNode::CheckSpecifiedProperties(const nsStyleStructID aSID,
                                     const nsRuleData* aRuleData)
{
  
  PRUint32 total = 0,      
           specified = 0,  
           inherited = 0;  
                           

  
  NS_ABORT_IF_FALSE(aRuleData->mValueOffsets[aSID] == 0,
                    "we assume the value offset is zero instead of adding it");
  for (nsCSSValue *values = aRuleData->mValueStorage,
              *values_end = values + nsCSSProps::PropertyCountInStruct(aSID);
       values != values_end; ++values) {
    ++total;
    ExamineCSSValue(*values, specified, inherited);
  }

#if 0
  printf("CheckSpecifiedProperties: SID=%d total=%d spec=%d inh=%d.\n",
         aSID, total, specified, inherited);
#endif

#ifdef MOZ_MATHML
  NS_ASSERTION(aSID != eStyleStruct_Font ||
               mPresContext->Document()->GetMathMLEnabled() ||
               AreAllMathMLPropertiesUndefined(aRuleData),
               "MathML style property was defined even though MathML is disabled");
#endif

  




  nsRuleNode::RuleDetail result;
  if (inherited == total)
    result = eRuleFullInherited;
  else if (specified == total
#ifdef MOZ_MATHML
           
           
           
           
           
           
           || (aSID == eStyleStruct_Font && specified + 3 == total &&
               !mPresContext->Document()->GetMathMLEnabled())
#endif
          ) {
    if (inherited == 0)
      result = eRuleFullReset;
    else
      result = eRuleFullMixed;
  } else if (specified == 0)
    result = eRuleNone;
  else if (specified == inherited)
    result = eRulePartialInherited;
  else if (inherited == 0)
    result = eRulePartialReset;
  else
    result = eRulePartialMixed;

  CheckCallbackFn cb = gCheckCallbacks[aSID];
  if (cb) {
    result = (*cb)(aRuleData, result);
  }

  return result;
}




inline PRUint32
GetPseudoRestriction(nsStyleContext *aContext)
{
  
  PRUint32 pseudoRestriction = 0;
  nsIAtom *pseudoType = aContext->GetPseudo();
  if (pseudoType) {
    if (pseudoType == nsCSSPseudoElements::firstLetter) {
      pseudoRestriction = CSS_PROPERTY_APPLIES_TO_FIRST_LETTER;
    } else if (pseudoType == nsCSSPseudoElements::firstLine) {
      pseudoRestriction = CSS_PROPERTY_APPLIES_TO_FIRST_LINE;
    }
  }
  return pseudoRestriction;
}

static void
UnsetPropertiesWithoutFlags(const nsStyleStructID aSID,
                            nsRuleData* aRuleData,
                            PRUint32 aFlags)
{
  NS_ASSERTION(aFlags != 0, "aFlags must be nonzero");

  const PRUint32 *flagData = gFlagsByStruct[aSID];

  
  NS_ABORT_IF_FALSE(aRuleData->mValueOffsets[aSID] == 0,
                    "we assume the value offset is zero instead of adding it");
  nsCSSValue *values = aRuleData->mValueStorage;

  for (size_t i = 0, i_end = nsCSSProps::PropertyCountInStruct(aSID);
       i != i_end; ++i) {
    if ((flagData[i] & aFlags) != aFlags)
      values[i].Reset();
  }
}











struct AutoCSSValueArray {
  


  AutoCSSValueArray(void* aStorage, size_t aCount) {
    NS_ABORT_IF_FALSE(size_t(aStorage) % NS_ALIGNMENT_OF(nsCSSValue) == 0,
                      "bad alignment from alloca");
    mCount = aCount;
    
    
    mArray = static_cast<nsCSSValue*>(aStorage);
    for (size_t i = 0; i < mCount; ++i) {
      new (mArray + i) nsCSSValue();
    }
  }

  ~AutoCSSValueArray() {
    for (size_t i = 0; i < mCount; ++i) {
      mArray[i].~nsCSSValue();
    }
  }

  nsCSSValue* get() { return mArray; }

private:
  nsCSSValue *mArray;
  size_t mCount;
};

const void*
nsRuleNode::WalkRuleTree(const nsStyleStructID aSID,
                         nsStyleContext* aContext)
{
  
  
  
  size_t nprops = nsCSSProps::PropertyCountInStruct(aSID);
  void* dataStorage = alloca(nprops * sizeof(nsCSSValue));
  AutoCSSValueArray dataArray(dataStorage, nprops);

  nsRuleData ruleData(nsCachedStyleData::GetBitForSID(aSID),
                      dataArray.get(), mPresContext, aContext);
  ruleData.mValueOffsets[aSID] = 0;

  
  void* startStruct = nsnull;

  nsRuleNode* ruleNode = this;
  nsRuleNode* highestNode = nsnull; 
                                    
                                    
                                    
  nsRuleNode* rootNode = this; 
                               
                               
                               
                               
  RuleDetail detail = eRuleNone;
  PRUint32 bit = nsCachedStyleData::GetBitForSID(aSID);

  while (ruleNode) {
    
    
    if (ruleNode->mNoneBits & bit)
      break;

    
    
    while (ruleNode->mDependentBits & bit) {
      NS_ASSERTION(ruleNode->mStyleData.GetStyleData(aSID) == nsnull,
                   "dependent bit with cached data makes no sense");
      
      rootNode = ruleNode;
      ruleNode = ruleNode->mParent;
      NS_ASSERTION(!(ruleNode->mNoneBits & bit), "can't have both bits set");
    }

    
    
    startStruct = ruleNode->mStyleData.GetStyleData(aSID);
    if (startStruct)
      break; 
             
             

    
    nsIStyleRule *rule = ruleNode->mRule;
    if (rule) {
      ruleData.mLevel = ruleNode->GetLevel();
      ruleData.mIsImportantRule = ruleNode->IsImportantRule();
      rule->MapRuleInfoInto(&ruleData);
    }

    
    
    RuleDetail oldDetail = detail;
    detail = CheckSpecifiedProperties(aSID, &ruleData);

    if (oldDetail == eRuleNone && detail != eRuleNone)
      highestNode = ruleNode;

    if (detail == eRuleFullReset ||
        detail == eRuleFullMixed ||
        detail == eRuleFullInherited)
      break; 
             

    
    rootNode = ruleNode;
    ruleNode = ruleNode->mParent;
  }

  
  
  
  PRUint32 pseudoRestriction = GetPseudoRestriction(aContext);
  if (pseudoRestriction) {
    UnsetPropertiesWithoutFlags(aSID, &ruleData, pseudoRestriction);

    
    
    
    detail = CheckSpecifiedProperties(aSID, &ruleData);
  }

  NS_ASSERTION(!startStruct || (detail != eRuleFullReset &&
                                detail != eRuleFullMixed &&
                                detail != eRuleFullInherited),
               "can't have start struct and be fully specified");

  PRBool isReset = nsCachedStyleData::IsReset(aSID);
  if (!highestNode)
    highestNode = rootNode;

  if (!ruleData.mCanStoreInRuleTree)
    detail = eRulePartialMixed; 
                                

  if (detail == eRuleNone && startStruct && !ruleData.mPostResolveCallback) {
    
    
    
    
    
    PropagateDependentBit(bit, ruleNode);
    return startStruct;
  }
  
  if ((!startStruct && !isReset &&
       (detail == eRuleNone || detail == eRulePartialInherited)) ||
      detail == eRuleFullInherited) {
    
    

    
    
    
    
    
    
    
    if (highestNode != this && !isReset)
      PropagateNoneBit(bit, highestNode);

    
    
    
    nsStyleContext* parentContext = aContext->GetParent();
    if (isReset) {
      
      
      while (parentContext &&
             parentContext->GetPseudo() == nsCSSPseudoElements::firstLine) {
        parentContext = parentContext->GetParent();
      }
    }
    if (parentContext) {
      
      
      
      
      const void* parentStruct = parentContext->GetStyleData(aSID);
      aContext->AddStyleBit(bit); 
      aContext->SetStyle(aSID, const_cast<void*>(parentStruct));
      return parentStruct;
    }
    else
      
      
      return SetDefaultOnRoot(aSID, aContext);
  }

  
  const void* res;
#define STYLE_STRUCT_TEST aSID
#define STYLE_STRUCT(name, checkdata_cb, ctor_args)                           \
  res = Compute##name##Data(startStruct, &ruleData, aContext,                 \
                            highestNode, detail, ruleData.mCanStoreInRuleTree);
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
#undef STYLE_STRUCT_TEST

  
  if (ruleData.mPostResolveCallback && (NS_LIKELY(res != nsnull)))
    (*ruleData.mPostResolveCallback)(const_cast<void*>(res), &ruleData);

  
  return res;
}

const void*
nsRuleNode::SetDefaultOnRoot(const nsStyleStructID aSID, nsStyleContext* aContext)
{
  switch (aSID) {
    case eStyleStruct_Font:
    {
      nsStyleFont* fontData = new (mPresContext) nsStyleFont(mPresContext);
      if (NS_LIKELY(fontData != nsnull)) {
        nscoord minimumFontSize = mPresContext->MinFontSize();

        if (minimumFontSize > 0 && !mPresContext->IsChrome()) {
          fontData->mFont.size = NS_MAX(fontData->mSize, minimumFontSize);
        }
        else {
          fontData->mFont.size = fontData->mSize;
        }
        aContext->SetStyle(eStyleStruct_Font, fontData);
      }
      return fontData;
    }
    case eStyleStruct_Display:
    {
      nsStyleDisplay* disp = new (mPresContext) nsStyleDisplay();
      if (NS_LIKELY(disp != nsnull)) {
        aContext->SetStyle(eStyleStruct_Display, disp);
      }
      return disp;
    }
    case eStyleStruct_Visibility:
    {
      nsStyleVisibility* vis = new (mPresContext) nsStyleVisibility(mPresContext);
      if (NS_LIKELY(vis != nsnull)) {
        aContext->SetStyle(eStyleStruct_Visibility, vis);
      }
      return vis;
    }
    case eStyleStruct_Text:
    {
      nsStyleText* text = new (mPresContext) nsStyleText();
      if (NS_LIKELY(text != nsnull)) {
        aContext->SetStyle(eStyleStruct_Text, text);
      }
      return text;
    }
    case eStyleStruct_TextReset:
    {
      nsStyleTextReset* text = new (mPresContext) nsStyleTextReset();
      if (NS_LIKELY(text != nsnull)) {
        aContext->SetStyle(eStyleStruct_TextReset, text);
      }
      return text;
    }
    case eStyleStruct_Color:
    {
      nsStyleColor* color = new (mPresContext) nsStyleColor(mPresContext);
      if (NS_LIKELY(color != nsnull)) {
        aContext->SetStyle(eStyleStruct_Color, color);
      }
      return color;
    }
    case eStyleStruct_Background:
    {
      nsStyleBackground* bg = new (mPresContext) nsStyleBackground();
      if (NS_LIKELY(bg != nsnull)) {
        aContext->SetStyle(eStyleStruct_Background, bg);
      }
      return bg;
    }
    case eStyleStruct_Margin:
    {
      nsStyleMargin* margin = new (mPresContext) nsStyleMargin();
      if (NS_LIKELY(margin != nsnull)) {
        aContext->SetStyle(eStyleStruct_Margin, margin);
      }
      return margin;
    }
    case eStyleStruct_Border:
    {
      nsStyleBorder* border = new (mPresContext) nsStyleBorder(mPresContext);
      if (NS_LIKELY(border != nsnull)) {
        aContext->SetStyle(eStyleStruct_Border, border);
      }
      return border;
    }
    case eStyleStruct_Padding:
    {
      nsStylePadding* padding = new (mPresContext) nsStylePadding();
      if (NS_LIKELY(padding != nsnull)) {
        aContext->SetStyle(eStyleStruct_Padding, padding);
      }
      return padding;
    }
    case eStyleStruct_Outline:
    {
      nsStyleOutline* outline = new (mPresContext) nsStyleOutline(mPresContext);
      if (NS_LIKELY(outline != nsnull)) {
        aContext->SetStyle(eStyleStruct_Outline, outline);
      }
      return outline;
    }
    case eStyleStruct_List:
    {
      nsStyleList* list = new (mPresContext) nsStyleList();
      if (NS_LIKELY(list != nsnull)) {
        aContext->SetStyle(eStyleStruct_List, list);
      }
      return list;
    }
    case eStyleStruct_Position:
    {
      nsStylePosition* pos = new (mPresContext) nsStylePosition();
      if (NS_LIKELY(pos != nsnull)) {
        aContext->SetStyle(eStyleStruct_Position, pos);
      }
      return pos;
    }
    case eStyleStruct_Table:
    {
      nsStyleTable* table = new (mPresContext) nsStyleTable();
      if (NS_LIKELY(table != nsnull)) {
        aContext->SetStyle(eStyleStruct_Table, table);
      }
      return table;
    }
    case eStyleStruct_TableBorder:
    {
      nsStyleTableBorder* table = new (mPresContext) nsStyleTableBorder(mPresContext);
      if (NS_LIKELY(table != nsnull)) {
        aContext->SetStyle(eStyleStruct_TableBorder, table);
      }
      return table;
    }
    case eStyleStruct_Content:
    {
      nsStyleContent* content = new (mPresContext) nsStyleContent();
      if (NS_LIKELY(content != nsnull)) {
        aContext->SetStyle(eStyleStruct_Content, content);
      }
      return content;
    }
    case eStyleStruct_Quotes:
    {
      nsStyleQuotes* quotes = new (mPresContext) nsStyleQuotes();
      if (NS_LIKELY(quotes != nsnull)) {
        aContext->SetStyle(eStyleStruct_Quotes, quotes);
      }
      return quotes;
    }
    case eStyleStruct_UserInterface:
    {
      nsStyleUserInterface* ui = new (mPresContext) nsStyleUserInterface();
      if (NS_LIKELY(ui != nsnull)) {
        aContext->SetStyle(eStyleStruct_UserInterface, ui);
      }
      return ui;
    }
    case eStyleStruct_UIReset:
    {
      nsStyleUIReset* ui = new (mPresContext) nsStyleUIReset();
      if (NS_LIKELY(ui != nsnull)) {
        aContext->SetStyle(eStyleStruct_UIReset, ui);
      }
      return ui;
    }

    case eStyleStruct_XUL:
    {
      nsStyleXUL* xul = new (mPresContext) nsStyleXUL();
      if (NS_LIKELY(xul != nsnull)) {
        aContext->SetStyle(eStyleStruct_XUL, xul);
      }
      return xul;
    }

    case eStyleStruct_Column:
    {
      nsStyleColumn* column = new (mPresContext) nsStyleColumn(mPresContext);
      if (NS_LIKELY(column != nsnull)) {
        aContext->SetStyle(eStyleStruct_Column, column);
      }
      return column;
    }

    case eStyleStruct_SVG:
    {
      nsStyleSVG* svg = new (mPresContext) nsStyleSVG();
      if (NS_LIKELY(svg != nsnull)) {
        aContext->SetStyle(eStyleStruct_SVG, svg);
      }
      return svg;
    }

    case eStyleStruct_SVGReset:
    {
      nsStyleSVGReset* svgReset = new (mPresContext) nsStyleSVGReset();
      if (NS_LIKELY(svgReset != nsnull)) {
        aContext->SetStyle(eStyleStruct_SVGReset, svgReset);
      }
      return svgReset;
    }
    default:
      



      return nsnull;
  }
  return nsnull;
}























void
nsRuleNode::AdjustLogicalBoxProp(nsStyleContext* aContext,
                                 const nsCSSValue& aLTRSource,
                                 const nsCSSValue& aRTLSource,
                                 const nsCSSValue& aLTRLogicalValue,
                                 const nsCSSValue& aRTLLogicalValue,
                                 mozilla::css::Side aSide,
                                 nsCSSRect& aValueRect,
                                 PRBool& aCanStoreInRuleTree)
{
  PRBool LTRlogical = aLTRSource.GetUnit() == eCSSUnit_Enumerated &&
                      aLTRSource.GetIntValue() == NS_BOXPROP_SOURCE_LOGICAL;
  PRBool RTLlogical = aRTLSource.GetUnit() == eCSSUnit_Enumerated &&
                      aRTLSource.GetIntValue() == NS_BOXPROP_SOURCE_LOGICAL;
  if (LTRlogical || RTLlogical) {
    
    
    
    aCanStoreInRuleTree = PR_FALSE;
    PRUint8 dir = aContext->GetStyleVisibility()->mDirection;

    if (dir == NS_STYLE_DIRECTION_LTR) {
      if (LTRlogical)
        aValueRect.*(nsCSSRect::sides[aSide]) = aLTRLogicalValue;
    } else {
      if (RTLlogical)
        aValueRect.*(nsCSSRect::sides[aSide]) = aRTLLogicalValue;
    }
  } else if (aLTRLogicalValue.GetUnit() == eCSSUnit_Inherit ||
             aRTLLogicalValue.GetUnit() == eCSSUnit_Inherit) {
    
    
    
    aCanStoreInRuleTree = PR_FALSE;
  }
}











#define COMPUTE_START_INHERITED(type_, ctorargs_, data_, parentdata_)         \
  NS_ASSERTION(aRuleDetail != eRuleFullInherited,                             \
               "should not have bothered calling Compute*Data");              \
                                                                              \
  nsStyleContext* parentContext = aContext->GetParent();                      \
                                                                              \
  nsStyle##type_* data_ = nsnull;                                             \
  const nsStyle##type_* parentdata_ = nsnull;                                 \
  PRBool canStoreInRuleTree = aCanStoreInRuleTree;                            \
                                                                              \
  /* If |canStoreInRuleTree| might be true by the time we're done, we */      \
  /* can't call parentContext->GetStyle##type_() since it could recur into */ \
  /* setting the same struct on the same rule node, causing a leak. */        \
  if (parentContext && aRuleDetail != eRuleFullReset &&                       \
      (!aStartStruct || (aRuleDetail != eRulePartialReset &&                  \
                         aRuleDetail != eRuleNone)))                          \
    parentdata_ = parentContext->GetStyle##type_();                           \
  if (aStartStruct)                                                           \
    /* We only need to compute the delta between this computed data and */    \
    /* our computed data. */                                                  \
    data_ = new (mPresContext)                                                \
            nsStyle##type_(*static_cast<nsStyle##type_*>(aStartStruct));      \
  else {                                                                      \
    if (aRuleDetail != eRuleFullMixed && aRuleDetail != eRuleFullReset) {     \
      /* No question. We will have to inherit. Go ahead and init */           \
      /* with inherited vals from parent. */                                  \
      canStoreInRuleTree = PR_FALSE;                                          \
      if (parentdata_)                                                        \
        data_ = new (mPresContext) nsStyle##type_(*parentdata_);              \
      else                                                                    \
        data_ = new (mPresContext) nsStyle##type_ ctorargs_;                  \
    }                                                                         \
    else                                                                      \
      data_ = new (mPresContext) nsStyle##type_ ctorargs_;                    \
  }                                                                           \
                                                                              \
  if (NS_UNLIKELY(!data_))                                                    \
    return nsnull;  /* Out Of Memory */                                       \
  if (!parentdata_)                                                           \
    parentdata_ = data_;











#define COMPUTE_START_RESET(type_, ctorargs_, data_, parentdata_)             \
  NS_ASSERTION(aRuleDetail != eRuleFullInherited,                             \
               "should not have bothered calling Compute*Data");              \
                                                                              \
  nsStyleContext* parentContext = aContext->GetParent();                      \
  /* Reset structs don't inherit from first-line */                           \
  /* See similar code in WalkRuleTree */                                      \
  while (parentContext &&                                                     \
         parentContext->GetPseudo() == nsCSSPseudoElements::firstLine) {      \
    parentContext = parentContext->GetParent();                               \
  }                                                                           \
                                                                              \
  nsStyle##type_* data_;                                                      \
  if (aStartStruct)                                                           \
    /* We only need to compute the delta between this computed data and */    \
    /* our computed data. */                                                  \
    data_ = new (mPresContext)                                                \
            nsStyle##type_(*static_cast<nsStyle##type_*>(aStartStruct));      \
  else                                                                        \
    data_ = new (mPresContext) nsStyle##type_ ctorargs_;                      \
                                                                              \
  if (NS_UNLIKELY(!data_))                                                    \
    return nsnull;  /* Out Of Memory */                                       \
                                                                              \
  /* If |canStoreInRuleTree| might be true by the time we're done, we */      \
  /* can't call parentContext->GetStyle##type_() since it could recur into */ \
  /* setting the same struct on the same rule node, causing a leak. */        \
  const nsStyle##type_* parentdata_ = data_;                                  \
  if (parentContext &&                                                        \
      aRuleDetail != eRuleFullReset &&                                        \
      aRuleDetail != eRulePartialReset &&                                     \
      aRuleDetail != eRuleNone)                                               \
    parentdata_ = parentContext->GetStyle##type_();                           \
  PRBool canStoreInRuleTree = aCanStoreInRuleTree;







#define COMPUTE_END_INHERITED(type_, data_)                                   \
  NS_POSTCONDITION(!canStoreInRuleTree || aRuleDetail == eRuleFullReset ||    \
                   (aStartStruct && aRuleDetail == eRulePartialReset),        \
                   "canStoreInRuleTree must be false for inherited structs "  \
                   "unless all properties have been specified with values "   \
                   "other than inherit");                                     \
  if (canStoreInRuleTree) {                                                   \
    /* We were fully specified and can therefore be cached right on the */    \
    /* rule node. */                                                          \
    if (!aHighestNode->mStyleData.mInheritedData) {                           \
      aHighestNode->mStyleData.mInheritedData =                               \
        new (mPresContext) nsInheritedStyleData;                              \
      if (NS_UNLIKELY(!aHighestNode->mStyleData.mInheritedData)) {            \
        data_->Destroy(mPresContext);                                         \
        return nsnull;                                                        \
      }                                                                       \
    }                                                                         \
    NS_ASSERTION(!aHighestNode->mStyleData.mInheritedData->                   \
                   mStyleStructs[eStyleStruct_##type_],                       \
                 "Going to leak style data");                                 \
    aHighestNode->mStyleData.mInheritedData->                                 \
      mStyleStructs[eStyleStruct_##type_] = data_;                            \
    /* Propagate the bit down. */                                             \
    PropagateDependentBit(NS_STYLE_INHERIT_BIT(type_), aHighestNode);         \
    /* Tell the style context that it doesn't own the data */                 \
    aContext->                                                                \
      AddStyleBit(nsCachedStyleData::GetBitForSID(eStyleStruct_##type_));     \
  }                                                                           \
  /* Always cache inherited data on the style context */                      \
  aContext->SetStyle##type_(data_);                                           \
                                                                              \
  return data_;







#define COMPUTE_END_RESET(type_, data_)                                       \
  NS_POSTCONDITION(!canStoreInRuleTree ||                                     \
                   aRuleDetail == eRuleNone ||                                \
                   aRuleDetail == eRulePartialReset ||                        \
                   aRuleDetail == eRuleFullReset,                             \
                   "canStoreInRuleTree must be false for reset structs "      \
                   "if any properties were specified as inherit");            \
  if (!canStoreInRuleTree)                                                    \
    /* We can't be cached in the rule node.  We have to be put right */       \
    /* on the style context. */                                               \
    aContext->SetStyle(eStyleStruct_##type_, data_);                          \
  else {                                                                      \
    /* We were fully specified and can therefore be cached right on the */    \
    /* rule node. */                                                          \
    if (!aHighestNode->mStyleData.mResetData) {                               \
      aHighestNode->mStyleData.mResetData =                                   \
        new (mPresContext) nsResetStyleData;                                  \
      if (NS_UNLIKELY(!aHighestNode->mStyleData.mResetData)) {                \
        data_->Destroy(mPresContext);                                         \
        return nsnull;                                                        \
      }                                                                       \
    }                                                                         \
    NS_ASSERTION(!aHighestNode->mStyleData.mResetData->                       \
                   mStyleStructs[eStyleStruct_##type_],                       \
                 "Going to leak style data");                                 \
    aHighestNode->mStyleData.mResetData->                                     \
      mStyleStructs[eStyleStruct_##type_] = data_;                            \
    /* Propagate the bit down. */                                             \
    PropagateDependentBit(NS_STYLE_INHERIT_BIT(type_), aHighestNode);         \
  }                                                                           \
                                                                              \
  return data_;

#ifdef MOZ_MATHML













static nscoord
ComputeScriptLevelSize(const nsStyleFont* aFont, const nsStyleFont* aParentFont,
                       nsPresContext* aPresContext, nscoord* aUnconstrainedSize)
{
  PRInt32 scriptLevelChange =
    aFont->mScriptLevel - aParentFont->mScriptLevel;
  if (scriptLevelChange == 0) {
    *aUnconstrainedSize = aParentFont->mScriptUnconstrainedSize;
    
    
    
    return aParentFont->mSize;
  }

  
  nscoord minScriptSize =
    nsStyleFont::ZoomText(aPresContext, aParentFont->mScriptMinSize);

  double scriptLevelScale =
    pow(aParentFont->mScriptSizeMultiplier, scriptLevelChange);
  
  
  *aUnconstrainedSize =
    NSToCoordRound(NS_MIN(aParentFont->mScriptUnconstrainedSize*scriptLevelScale,
                          double(nscoord_MAX)));
  
  nscoord scriptLevelSize =
    NSToCoordRound(NS_MIN(aParentFont->mSize*scriptLevelScale,
                          double(nscoord_MAX)));
  if (scriptLevelScale <= 1.0) {
    if (aParentFont->mSize <= minScriptSize) {
      
      
      
      return aParentFont->mSize;
    }
    
    return NS_MAX(minScriptSize, scriptLevelSize);
  } else {
    
    NS_ASSERTION(*aUnconstrainedSize <= scriptLevelSize, "How can this ever happen?");
    
    return NS_MIN(scriptLevelSize, NS_MAX(*aUnconstrainedSize, minScriptSize));
  }
}
#endif

struct SetFontSizeCalcOps : public css::BasicCoordCalcOps,
                            public css::NumbersAlreadyNormalizedOps
{
  
  const nscoord mParentSize;
  const nsStyleFont* const mParentFont;
  nsPresContext* const mPresContext;
  const PRBool mAtRoot;
  PRBool& mCanStoreInRuleTree;

  SetFontSizeCalcOps(nscoord aParentSize, const nsStyleFont* aParentFont,
                     nsPresContext* aPresContext, PRBool aAtRoot,
                     PRBool& aCanStoreInRuleTree)
    : mParentSize(aParentSize),
      mParentFont(aParentFont),
      mPresContext(aPresContext),
      mAtRoot(aAtRoot),
      mCanStoreInRuleTree(aCanStoreInRuleTree)
  {
  }

  result_type ComputeLeafValue(const nsCSSValue& aValue)
  {
    nscoord size;
    if (aValue.IsLengthUnit()) {
      
      
      
      size = CalcLengthWith(aValue, mParentSize, mParentFont,
                            nsnull, mPresContext, mAtRoot,
                            PR_TRUE, mCanStoreInRuleTree);
      if (!aValue.IsRelativeLengthUnit()) {
        size = nsStyleFont::ZoomText(mPresContext, size);
      }
    }
    else if (eCSSUnit_Percent == aValue.GetUnit()) {
      mCanStoreInRuleTree = PR_FALSE;
      
      
      
      
      size = NSCoordSaturatingMultiply(mParentSize, aValue.GetPercentValue());
    } else {
      NS_ABORT_IF_FALSE(PR_FALSE, "unexpected value");
      size = mParentSize;
    }

    return size;
  }
};

 void
nsRuleNode::SetFontSize(nsPresContext* aPresContext,
                        const nsRuleData* aRuleData,
                        const nsStyleFont* aFont,
                        const nsStyleFont* aParentFont,
                        nscoord* aSize,
                        const nsFont& aSystemFont,
                        nscoord aParentSize,
                        nscoord aScriptLevelAdjustedParentSize,
                        PRBool aUsedStartStruct,
                        PRBool aAtRoot,
                        PRBool& aCanStoreInRuleTree)
{
  PRBool zoom = PR_FALSE;
  PRInt32 baseSize = (PRInt32) aPresContext->
    GetDefaultFont(aFont->mGenericID)->size;
  const nsCSSValue* sizeValue = aRuleData->ValueForFontSize();
  if (eCSSUnit_Enumerated == sizeValue->GetUnit()) {
    PRInt32 value = sizeValue->GetIntValue();
    PRInt32 scaler = aPresContext->FontScaler();
    float scaleFactor = nsStyleUtil::GetScalingFactor(scaler);

    zoom = PR_TRUE;
    if ((NS_STYLE_FONT_SIZE_XXSMALL <= value) &&
        (value <= NS_STYLE_FONT_SIZE_XXLARGE)) {
      *aSize = nsStyleUtil::CalcFontPointSize(value, baseSize,
                       scaleFactor, aPresContext, eFontSize_CSS);
    }
    else if (NS_STYLE_FONT_SIZE_XXXLARGE == value) {
      
      *aSize = nsStyleUtil::CalcFontPointSize(value, baseSize,
                       scaleFactor, aPresContext);
    }
    else if (NS_STYLE_FONT_SIZE_LARGER  == value ||
             NS_STYLE_FONT_SIZE_SMALLER == value) {
      aCanStoreInRuleTree = PR_FALSE;

      
      
      
      
      
      nscoord parentSize =
        nsStyleFont::UnZoomText(aPresContext, aParentSize);

      if (NS_STYLE_FONT_SIZE_LARGER == value) {
        *aSize = nsStyleUtil::FindNextLargerFontSize(parentSize,
                         baseSize, scaleFactor, aPresContext, eFontSize_CSS);
        NS_ASSERTION(*aSize > parentSize,
                     "FindNextLargerFontSize failed");
      }
      else {
        *aSize = nsStyleUtil::FindNextSmallerFontSize(parentSize,
                         baseSize, scaleFactor, aPresContext, eFontSize_CSS);
        NS_ASSERTION(*aSize < parentSize ||
                     parentSize <= nsPresContext::CSSPixelsToAppUnits(1),
                     "FindNextSmallerFontSize failed");
      }
    } else {
      NS_NOTREACHED("unexpected value");
    }
  }
  else if (sizeValue->IsLengthUnit() ||
           sizeValue->GetUnit() == eCSSUnit_Percent ||
           sizeValue->IsCalcUnit()) {
    SetFontSizeCalcOps ops(aParentSize, aParentFont, aPresContext, aAtRoot,
                           aCanStoreInRuleTree);
    *aSize = css::ComputeCalc(*sizeValue, ops);
    if (*aSize < 0) {
      NS_ABORT_IF_FALSE(sizeValue->IsCalcUnit(),
                        "negative lengths and percents should be rejected "
                        "by parser");
      *aSize = 0;
    }
    
    zoom = PR_FALSE;
  }
  else if (eCSSUnit_System_Font == sizeValue->GetUnit()) {
    
    *aSize = aSystemFont.size;
    zoom = PR_TRUE;
  }
  else if (eCSSUnit_Inherit == sizeValue->GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    
    
    
    *aSize = aScriptLevelAdjustedParentSize;
    zoom = PR_FALSE;
  }
  else if (eCSSUnit_Initial == sizeValue->GetUnit()) {
    
    
    *aSize = baseSize;
    zoom = PR_TRUE;
  } else {
    NS_ASSERTION(eCSSUnit_Null == sizeValue->GetUnit(),
                 "What kind of font-size value is this?");
#ifdef MOZ_MATHML
    
    
    
    
    if (!aUsedStartStruct && aParentSize != aScriptLevelAdjustedParentSize) {
      
      
      
      aCanStoreInRuleTree = PR_FALSE;
      *aSize = aScriptLevelAdjustedParentSize;
    }
#endif
  }

  
  
  if (zoom) {
    *aSize = nsStyleFont::ZoomText(aPresContext, *aSize);
  }
}

static PRInt8 ClampTo8Bit(PRInt32 aValue) {
  if (aValue < -128)
    return -128;
  if (aValue > 127)
    return 127;
  return PRInt8(aValue);
}

 void
nsRuleNode::SetFont(nsPresContext* aPresContext, nsStyleContext* aContext,
                    nscoord aMinFontSize,
                    PRUint8 aGenericFontID, const nsRuleData* aRuleData,
                    const nsStyleFont* aParentFont,
                    nsStyleFont* aFont, PRBool aUsedStartStruct,
                    PRBool& aCanStoreInRuleTree)
{
  const nsFont* defaultVariableFont =
    aPresContext->GetDefaultFont(kPresContext_DefaultVariableFont_ID);
  PRBool atRoot = !aContext->GetParent();

  
  nsFont systemFont;
  const nsCSSValue* systemFontValue = aRuleData->ValueForSystemFont();
  if (eCSSUnit_Enumerated == systemFontValue->GetUnit()) {
    nsSystemFontID sysID;
    switch (systemFontValue->GetIntValue()) {
      case NS_STYLE_FONT_CAPTION:       sysID = eSystemFont_Caption;      break;    
      case NS_STYLE_FONT_ICON:          sysID = eSystemFont_Icon;         break;
      case NS_STYLE_FONT_MENU:          sysID = eSystemFont_Menu;         break;
      case NS_STYLE_FONT_MESSAGE_BOX:   sysID = eSystemFont_MessageBox;   break;
      case NS_STYLE_FONT_SMALL_CAPTION: sysID = eSystemFont_SmallCaption; break;
      case NS_STYLE_FONT_STATUS_BAR:    sysID = eSystemFont_StatusBar;    break;
      case NS_STYLE_FONT_WINDOW:        sysID = eSystemFont_Window;       break;    
      case NS_STYLE_FONT_DOCUMENT:      sysID = eSystemFont_Document;     break;
      case NS_STYLE_FONT_WORKSPACE:     sysID = eSystemFont_Workspace;    break;
      case NS_STYLE_FONT_DESKTOP:       sysID = eSystemFont_Desktop;      break;
      case NS_STYLE_FONT_INFO:          sysID = eSystemFont_Info;         break;
      case NS_STYLE_FONT_DIALOG:        sysID = eSystemFont_Dialog;       break;
      case NS_STYLE_FONT_BUTTON:        sysID = eSystemFont_Button;       break;
      case NS_STYLE_FONT_PULL_DOWN_MENU:sysID = eSystemFont_PullDownMenu; break;
      case NS_STYLE_FONT_LIST:          sysID = eSystemFont_List;         break;
      case NS_STYLE_FONT_FIELD:         sysID = eSystemFont_Field;        break;
    }

    
    
    
    systemFont.size = defaultVariableFont->size;

    if (NS_FAILED(aPresContext->DeviceContext()->GetSystemFont(sysID,
                                                               &systemFont))) {
        systemFont.name = defaultVariableFont->name;
    }

    
    

#ifdef XP_WIN
    
    
    
    
    
    
    switch (sysID) {
      
      
      
      
      
      
      case eSystemFont_Field:
      case eSystemFont_Button:
      case eSystemFont_List:
        
        systemFont.size =
          NS_MAX(defaultVariableFont->size - nsPresContext::CSSPointsToAppUnits(2), 0);
        break;
    }
#endif
  } else {
    
    systemFont = *defaultVariableFont;
  }


  
  const nsCSSValue* familyValue = aRuleData->ValueForFontFamily();
  NS_ASSERTION(eCSSUnit_Enumerated != familyValue->GetUnit(),
               "system fonts should not be in mFamily anymore");
  if (eCSSUnit_Families == familyValue->GetUnit()) {
    
    
    if (aGenericFontID == kGenericFont_NONE) {
      
      if (!aFont->mFont.name.IsEmpty())
        aFont->mFont.name.Append((PRUnichar)',');
      
      aFont->mFont.name.Append(defaultVariableFont->name);
    }
    aFont->mFont.systemFont = PR_FALSE;
    
    
    
    aFont->mGenericID = aGenericFontID;
  }
  else if (eCSSUnit_System_Font == familyValue->GetUnit()) {
    aFont->mFont.name = systemFont.name;
    aFont->mFont.systemFont = PR_TRUE;
    aFont->mGenericID = kGenericFont_NONE;
  }
  else if (eCSSUnit_Inherit == familyValue->GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    aFont->mFont.name = aParentFont->mFont.name;
    aFont->mFont.systemFont = aParentFont->mFont.systemFont;
    aFont->mGenericID = aParentFont->mGenericID;
  }
  else if (eCSSUnit_Initial == familyValue->GetUnit()) {
    aFont->mFont.name = defaultVariableFont->name;
    aFont->mFont.systemFont = defaultVariableFont->systemFont;
    aFont->mGenericID = kGenericFont_NONE;
  }

  
  
  
  
  if (aGenericFontID != kGenericFont_NONE) {
    aFont->mGenericID = aGenericFontID;
  }

  
  SetDiscrete(*aRuleData->ValueForFontStyle(),
              aFont->mFont.style, aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT,
              aParentFont->mFont.style,
              defaultVariableFont->style,
              0, 0, 0, systemFont.style);

  
  SetDiscrete(*aRuleData->ValueForFontVariant(),
              aFont->mFont.variant, aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT,
              aParentFont->mFont.variant,
              defaultVariableFont->variant,
              0, 0, 0, systemFont.variant);

  
  
  const nsCSSValue* weightValue = aRuleData->ValueForFontWeight();
  if (eCSSUnit_Enumerated == weightValue->GetUnit()) {
    PRInt32 value = weightValue->GetIntValue();
    switch (value) {
      case NS_STYLE_FONT_WEIGHT_NORMAL:
      case NS_STYLE_FONT_WEIGHT_BOLD:
        aFont->mFont.weight = value;
        break;
      case NS_STYLE_FONT_WEIGHT_BOLDER: {
        aCanStoreInRuleTree = PR_FALSE;
        PRInt32 inheritedValue = aParentFont->mFont.weight;
        if (inheritedValue <= 300) {
          aFont->mFont.weight = 400;
        } else if (inheritedValue <= 500) {
          aFont->mFont.weight = 700;
        } else {
          aFont->mFont.weight = 900;
        }
        break;
      }
      case NS_STYLE_FONT_WEIGHT_LIGHTER: {
        aCanStoreInRuleTree = PR_FALSE;
        PRInt32 inheritedValue = aParentFont->mFont.weight;
        if (inheritedValue < 600) {
          aFont->mFont.weight = 100;
        } else if (inheritedValue < 800) {
          aFont->mFont.weight = 400;
        } else {
          aFont->mFont.weight = 700;
        }
        break;
      }
    }
  } else
    SetDiscrete(*weightValue, aFont->mFont.weight, aCanStoreInRuleTree,
                SETDSC_INTEGER | SETDSC_SYSTEM_FONT,
                aParentFont->mFont.weight,
                defaultVariableFont->weight,
                0, 0, 0, systemFont.weight);

  
  SetDiscrete(*aRuleData->ValueForFontStretch(),
              aFont->mFont.stretch, aCanStoreInRuleTree,
              SETDSC_SYSTEM_FONT | SETDSC_ENUMERATED,
              aParentFont->mFont.stretch,
              defaultVariableFont->stretch,
              0, 0, 0, systemFont.stretch);

#ifdef MOZ_MATHML
  
  

  
  const nsCSSValue* scriptMinSizeValue = aRuleData->ValueForScriptMinSize();
  if (scriptMinSizeValue->IsLengthUnit()) {
    
    
    
    aFont->mScriptMinSize =
      CalcLengthWith(*scriptMinSizeValue, aParentFont->mSize, aParentFont,
                     nsnull, aPresContext, atRoot, PR_TRUE,
                     aCanStoreInRuleTree);
  }

  
  SetFactor(*aRuleData->ValueForScriptSizeMultiplier(),
            aFont->mScriptSizeMultiplier,
            aCanStoreInRuleTree, aParentFont->mScriptSizeMultiplier,
            NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER,
            SETFCT_POSITIVE);

  
  const nsCSSValue* scriptLevelValue = aRuleData->ValueForScriptLevel();
  if (eCSSUnit_Integer == scriptLevelValue->GetUnit()) {
    
    aFont->mScriptLevel = ClampTo8Bit(aParentFont->mScriptLevel + scriptLevelValue->GetIntValue());
  }
  else if (eCSSUnit_Number == scriptLevelValue->GetUnit()) {
    
    aFont->mScriptLevel = ClampTo8Bit(PRInt32(scriptLevelValue->GetFloatValue()));
  }
  else if (eCSSUnit_Inherit == scriptLevelValue->GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    aFont->mScriptLevel = aParentFont->mScriptLevel;
  }
  else if (eCSSUnit_Initial == scriptLevelValue->GetUnit()) {
    aFont->mScriptLevel = 0;
  }
#endif

  
  const nsCSSValue* featureSettingsValue =
    aRuleData->ValueForFontFeatureSettings();
  if (eCSSUnit_Inherit == featureSettingsValue->GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    aFont->mFont.featureSettings = aParentFont->mFont.featureSettings;
  } else if (eCSSUnit_Normal == featureSettingsValue->GetUnit() ||
             eCSSUnit_Initial == featureSettingsValue->GetUnit()) {
    aFont->mFont.featureSettings.Truncate();
  } else if (eCSSUnit_System_Font == featureSettingsValue->GetUnit()) {
    aFont->mFont.featureSettings = systemFont.featureSettings;
  } else if (eCSSUnit_String == featureSettingsValue->GetUnit()) {
    featureSettingsValue->GetStringValue(aFont->mFont.featureSettings);
  }

  
  const nsCSSValue* languageOverrideValue =
    aRuleData->ValueForFontLanguageOverride();
  if (eCSSUnit_Inherit == languageOverrideValue->GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    aFont->mFont.languageOverride = aParentFont->mFont.languageOverride;
  } else if (eCSSUnit_Normal == languageOverrideValue->GetUnit() ||
             eCSSUnit_Initial == languageOverrideValue->GetUnit()) {
    aFont->mFont.languageOverride.Truncate();
  } else if (eCSSUnit_System_Font == languageOverrideValue->GetUnit()) {
    aFont->mFont.languageOverride = systemFont.languageOverride;
  } else if (eCSSUnit_String == languageOverrideValue->GetUnit()) {
    languageOverrideValue->GetStringValue(aFont->mFont.languageOverride);
  }

  
  nscoord scriptLevelAdjustedParentSize = aParentFont->mSize;
#ifdef MOZ_MATHML
  nscoord scriptLevelAdjustedUnconstrainedParentSize;
  scriptLevelAdjustedParentSize =
    ComputeScriptLevelSize(aFont, aParentFont, aPresContext,
                           &scriptLevelAdjustedUnconstrainedParentSize);
  NS_ASSERTION(!aUsedStartStruct || aFont->mScriptUnconstrainedSize == aFont->mSize,
               "If we have a start struct, we should have reset everything coming in here");
#endif
  SetFontSize(aPresContext, aRuleData, aFont, aParentFont, &aFont->mSize,
              systemFont, aParentFont->mSize, scriptLevelAdjustedParentSize,
              aUsedStartStruct, atRoot, aCanStoreInRuleTree);
#ifdef MOZ_MATHML
  if (aParentFont->mSize == aParentFont->mScriptUnconstrainedSize &&
      scriptLevelAdjustedParentSize == scriptLevelAdjustedUnconstrainedParentSize) {
    
    
    
    
    
    aFont->mScriptUnconstrainedSize = aFont->mSize;
  } else {
    SetFontSize(aPresContext, aRuleData, aFont, aParentFont,
                &aFont->mScriptUnconstrainedSize, systemFont,
                aParentFont->mScriptUnconstrainedSize,
                scriptLevelAdjustedUnconstrainedParentSize,
                aUsedStartStruct, atRoot, aCanStoreInRuleTree);
  }
  NS_ASSERTION(aFont->mScriptUnconstrainedSize <= aFont->mSize,
               "scriptminsize should never be making things bigger");
#endif

  
  
  if (0 < aFont->mSize && aFont->mSize < aMinFontSize)
    aFont->mFont.size = aMinFontSize;
  else
    aFont->mFont.size = aFont->mSize;

  
  const nsCSSValue* sizeAdjustValue = aRuleData->ValueForFontSizeAdjust();
  if (eCSSUnit_System_Font == sizeAdjustValue->GetUnit()) {
    aFont->mFont.sizeAdjust = systemFont.sizeAdjust;
  } else
    SetFactor(*sizeAdjustValue, aFont->mFont.sizeAdjust,
              aCanStoreInRuleTree, aParentFont->mFont.sizeAdjust, 0.0f,
              SETFCT_NONE);
}





 void
nsRuleNode::SetGenericFont(nsPresContext* aPresContext,
                           nsStyleContext* aContext,
                           PRUint8 aGenericFontID, nscoord aMinFontSize,
                           nsStyleFont* aFont)
{
  
  nsAutoTArray<nsStyleContext*, 8> contextPath;
  contextPath.AppendElement(aContext);
  nsStyleContext* higherContext = aContext->GetParent();
  while (higherContext) {
    if (higherContext->GetStyleFont()->mGenericID == aGenericFontID) {
      
      break;
    }
    contextPath.AppendElement(higherContext);
    higherContext = higherContext->GetParent();
  }

  

  
  
  
  const nsFont* defaultFont = aPresContext->GetDefaultFont(aGenericFontID);
  nsStyleFont parentFont(*defaultFont, aPresContext);
  if (higherContext) {
    const nsStyleFont* tmpFont = higherContext->GetStyleFont();
    parentFont = *tmpFont;
  }
  *aFont = parentFont;

  PRBool dummy;
  PRUint32 fontBit = nsCachedStyleData::GetBitForSID(eStyleStruct_Font);

  
  
  
  size_t nprops = nsCSSProps::PropertyCountInStruct(eStyleStruct_Font);
  void* dataStorage = alloca(nprops * sizeof(nsCSSValue));

  for (PRInt32 i = contextPath.Length() - 1; i >= 0; --i) {
    nsStyleContext* context = contextPath[i];
    AutoCSSValueArray dataArray(dataStorage, nprops);

    nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Font), dataArray.get(),
                        aPresContext, context);
    ruleData.mValueOffsets[eStyleStruct_Font] = 0;

    
    
    
    
    for (nsRuleNode* ruleNode = context->GetRuleNode(); ruleNode;
         ruleNode = ruleNode->GetParent()) {
      if (ruleNode->mNoneBits & fontBit)
        
        break;

      nsIStyleRule *rule = ruleNode->GetRule();
      if (rule) {
        ruleData.mLevel = ruleNode->GetLevel();
        ruleData.mIsImportantRule = ruleNode->IsImportantRule();
        rule->MapRuleInfoInto(&ruleData);
      }
    }

    

    
    
    if (i != 0)
      ruleData.ValueForFontFamily()->Reset();

    nsRuleNode::SetFont(aPresContext, context, aMinFontSize,
                        aGenericFontID, &ruleData, &parentFont, aFont,
                        PR_FALSE, dummy);

    
    
    if (ruleData.mPostResolveCallback)
      (ruleData.mPostResolveCallback)(aFont, &ruleData);

    parentFont = *aFont;
  }
}

static PRBool ExtractGeneric(const nsString& aFamily, PRBool aGeneric,
                             void *aData)
{
  nsAutoString *data = static_cast<nsAutoString*>(aData);

  if (aGeneric) {
    *data = aFamily;
    return PR_FALSE; 
  }
  return PR_TRUE;
}

const void*
nsRuleNode::ComputeFontData(void* aStartStruct,
                            const nsRuleData* aRuleData,
                            nsStyleContext* aContext,
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Font, (mPresContext), font, parentFont)

  
  
  
  
  
  
  
  
  
  

  
  nscoord minimumFontSize = mPresContext->MinFontSize();

  if (minimumFontSize < 0)
    minimumFontSize = 0;

  PRBool useDocumentFonts =
    mPresContext->GetCachedBoolPref(kPresContext_UseDocumentFonts);

  
  
  
  
  if ((!useDocumentFonts || minimumFontSize > 0) && mPresContext->IsChrome()) {
    
    
    useDocumentFonts = PR_TRUE;
    minimumFontSize = 0;
  }

  
  PRUint8 generic = kGenericFont_NONE;
  
  
  const nsCSSValue* familyValue = aRuleData->ValueForFontFamily();
  if (eCSSUnit_Families == familyValue->GetUnit()) {
    familyValue->GetStringValue(font->mFont.name);
    
    
    nsFont::GetGenericID(font->mFont.name, &generic);

    
    
    if (!useDocumentFonts) {
      
      nsAutoString genericName;
      if (!font->mFont.EnumerateFamilies(ExtractGeneric, &genericName)) {
        
        font->mFont.name = genericName;
        nsFont::GetGenericID(genericName, &generic);

        
        if (generic != kGenericFont_moz_fixed &&
            generic != kGenericFont_monospace) {
          font->mFont.name.Truncate();
          generic = kGenericFont_NONE;
        }
      } else {
        
        font->mFont.name.Truncate();
        generic = kGenericFont_NONE;
      }
    }
  }

  
  if (generic == kGenericFont_NONE) {
    
    nsRuleNode::SetFont(mPresContext, aContext, minimumFontSize, generic,
                        aRuleData, parentFont, font,
                        aStartStruct != nsnull, canStoreInRuleTree);
  }
  else {
    
    canStoreInRuleTree = PR_FALSE;
    nsRuleNode::SetGenericFont(mPresContext, aContext, generic,
                               minimumFontSize, font);
  }

  COMPUTE_END_INHERITED(Font, font)
}

template <typename T>
inline PRUint32 ListLength(const T* aList)
{
  PRUint32 len = 0;
  while (aList) {
    len++;
    aList = aList->mNext;
  }
  return len;
}



already_AddRefed<nsCSSShadowArray>
nsRuleNode::GetShadowData(const nsCSSValueList* aList,
                          nsStyleContext* aContext,
                          PRBool aIsBoxShadow,
                          PRBool& canStoreInRuleTree)
{
  PRUint32 arrayLength = ListLength(aList);

  NS_ABORT_IF_FALSE(arrayLength > 0,
                    "Non-null text-shadow list, yet we counted 0 items.");
  nsCSSShadowArray* shadowList = new(arrayLength) nsCSSShadowArray(arrayLength);

  if (!shadowList)
    return nsnull;

  nsStyleCoord tempCoord;
  PRBool unitOK;
  for (nsCSSShadowItem* item = shadowList->ShadowAt(0);
       aList;
       aList = aList->mNext, ++item) {
    NS_ABORT_IF_FALSE(aList->mValue.GetUnit() == eCSSUnit_Array,
                      "expecting a plain array value");
    nsCSSValue::Array *arr = aList->mValue.GetArrayValue();
    
    unitOK = SetCoord(arr->Item(0), tempCoord, nsStyleCoord(),
                      SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY,
                      aContext, mPresContext, canStoreInRuleTree);
    NS_ASSERTION(unitOK, "unexpected unit");
    item->mXOffset = tempCoord.GetCoordValue();

    unitOK = SetCoord(arr->Item(1), tempCoord, nsStyleCoord(),
                      SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY,
                      aContext, mPresContext, canStoreInRuleTree);
    NS_ASSERTION(unitOK, "unexpected unit");
    item->mYOffset = tempCoord.GetCoordValue();

    
    if (arr->Item(2).GetUnit() != eCSSUnit_Null) {
      unitOK = SetCoord(arr->Item(2), tempCoord, nsStyleCoord(),
                        SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY |
                          SETCOORD_CALC_CLAMP_NONNEGATIVE,
                        aContext, mPresContext, canStoreInRuleTree);
      NS_ASSERTION(unitOK, "unexpected unit");
      item->mRadius = tempCoord.GetCoordValue();
    } else {
      item->mRadius = 0;
    }

    
    if (aIsBoxShadow && arr->Item(3).GetUnit() != eCSSUnit_Null) {
      unitOK = SetCoord(arr->Item(3), tempCoord, nsStyleCoord(),
                        SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY,
                        aContext, mPresContext, canStoreInRuleTree);
      NS_ASSERTION(unitOK, "unexpected unit");
      item->mSpread = tempCoord.GetCoordValue();
    } else {
      item->mSpread = 0;
    }

    if (arr->Item(4).GetUnit() != eCSSUnit_Null) {
      item->mHasColor = PR_TRUE;
      
      unitOK = SetColor(arr->Item(4), 0, mPresContext, aContext, item->mColor,
                        canStoreInRuleTree);
      NS_ASSERTION(unitOK, "unexpected unit");
    }

    if (aIsBoxShadow && arr->Item(5).GetUnit() == eCSSUnit_Enumerated) {
      NS_ASSERTION(arr->Item(5).GetIntValue() == NS_STYLE_BOX_SHADOW_INSET,
                   "invalid keyword type for box shadow");
      item->mInset = PR_TRUE;
    } else {
      item->mInset = PR_FALSE;
    }
  }

  NS_ADDREF(shadowList);
  return shadowList;
}

const void*
nsRuleNode::ComputeTextData(void* aStartStruct,
                            const nsRuleData* aRuleData,
                            nsStyleContext* aContext,
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Text, (), text, parentText)

  
  SetDiscrete(*aRuleData->ValueForTabSize(),
              text->mTabSize, canStoreInRuleTree,
              SETDSC_INTEGER, parentText->mTabSize,
              NS_STYLE_TABSIZE_INITIAL, 0, 0, 0, 0);

  
  SetCoord(*aRuleData->ValueForLetterSpacing(),
           text->mLetterSpacing, parentText->mLetterSpacing,
           SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL |
             SETCOORD_CALC_LENGTH_ONLY,
           aContext, mPresContext, canStoreInRuleTree);

  
  const nsCSSValue* textShadowValue = aRuleData->ValueForTextShadow();
  if (textShadowValue->GetUnit() != eCSSUnit_Null) {
    text->mTextShadow = nsnull;

    
    
    if (textShadowValue->GetUnit() == eCSSUnit_Inherit) {
      canStoreInRuleTree = PR_FALSE;
      text->mTextShadow = parentText->mTextShadow;
    } else if (textShadowValue->GetUnit() == eCSSUnit_List ||
               textShadowValue->GetUnit() == eCSSUnit_ListDep) {
      
      text->mTextShadow = GetShadowData(textShadowValue->GetListValue(),
                                        aContext, PR_FALSE, canStoreInRuleTree);
    }
  }

  
  const nsCSSValue* lineHeightValue = aRuleData->ValueForLineHeight();
  if (eCSSUnit_Percent == lineHeightValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    
    text->mLineHeight.SetCoordValue(
        nscoord(float(aContext->GetStyleFont()->mFont.size) *
                lineHeightValue->GetPercentValue()));
  }
  else if (eCSSUnit_Initial == lineHeightValue->GetUnit() ||
           eCSSUnit_System_Font == lineHeightValue->GetUnit()) {
    text->mLineHeight.SetNormalValue();
  }
  else {
    SetCoord(*lineHeightValue, text->mLineHeight, parentText->mLineHeight,
             SETCOORD_LEH | SETCOORD_FACTOR | SETCOORD_NORMAL,
             aContext, mPresContext, canStoreInRuleTree);
    if (lineHeightValue->IsLengthUnit() &&
        !lineHeightValue->IsRelativeLengthUnit()) {
      nscoord lh = nsStyleFont::ZoomText(mPresContext,
                                         text->mLineHeight.GetCoordValue());
      nscoord minimumFontSize = mPresContext->MinFontSize();

      if (minimumFontSize > 0 && !mPresContext->IsChrome()) {
        
        
        
        canStoreInRuleTree = PR_FALSE;
        const nsStyleFont *font = aContext->GetStyleFont();
        if (font->mSize != 0) {
          lh = nscoord(float(lh) * float(font->mFont.size) / float(font->mSize));
        } else {
          lh = minimumFontSize;
        }
      }
      text->mLineHeight.SetCoordValue(lh);
    }
  }


  
  const nsCSSValue* textAlignValue = aRuleData->ValueForTextAlign();
  if (eCSSUnit_String == textAlignValue->GetUnit()) {
    NS_NOTYETIMPLEMENTED("align string");
  } else if (eCSSUnit_Enumerated == textAlignValue->GetUnit() &&
             NS_STYLE_TEXT_ALIGN_MOZ_CENTER_OR_INHERIT ==
               textAlignValue->GetIntValue()) {
    canStoreInRuleTree = PR_FALSE;
    PRUint8 parentAlign = parentText->mTextAlign;
    text->mTextAlign = (NS_STYLE_TEXT_ALIGN_DEFAULT == parentAlign) ?
      NS_STYLE_TEXT_ALIGN_CENTER : parentAlign;
  } else
    SetDiscrete(*textAlignValue, text->mTextAlign, canStoreInRuleTree,
                SETDSC_ENUMERATED, parentText->mTextAlign,
                NS_STYLE_TEXT_ALIGN_DEFAULT,
                0, 0, 0, 0);

  
  SetCoord(*aRuleData->ValueForTextIndent(), text->mTextIndent, parentText->mTextIndent,
           SETCOORD_LPH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetDiscrete(*aRuleData->ValueForTextTransform(), text->mTextTransform, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentText->mTextTransform,
              NS_STYLE_TEXT_TRANSFORM_NONE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForWhiteSpace(), text->mWhiteSpace, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentText->mWhiteSpace,
              NS_STYLE_WHITESPACE_NORMAL, 0, 0, 0, 0);

  
  nsStyleCoord tempCoord;
  const nsCSSValue* wordSpacingValue = aRuleData->ValueForWordSpacing();
  if (SetCoord(*wordSpacingValue, tempCoord,
               nsStyleCoord(parentText->mWordSpacing,
                            nsStyleCoord::CoordConstructor),
               SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL |
                 SETCOORD_CALC_LENGTH_ONLY,
               aContext, mPresContext, canStoreInRuleTree)) {
    if (tempCoord.GetUnit() == eStyleUnit_Coord) {
      text->mWordSpacing = tempCoord.GetCoordValue();
    } else if (tempCoord.GetUnit() == eStyleUnit_Normal) {
      text->mWordSpacing = 0;
    } else {
      NS_NOTREACHED("unexpected unit");
    }
  } else {
    NS_ASSERTION(wordSpacingValue->GetUnit() == eCSSUnit_Null,
                 "unexpected unit");
  }

  
  SetDiscrete(*aRuleData->ValueForWordWrap(), text->mWordWrap, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentText->mWordWrap,
              NS_STYLE_WORDWRAP_NORMAL, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForHyphens(), text->mHyphens, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentText->mHyphens,
              NS_STYLE_HYPHENS_MANUAL, 0, 0, 0, 0);

  COMPUTE_END_INHERITED(Text, text)
}

const void*
nsRuleNode::ComputeTextResetData(void* aStartStruct,
                                 const nsRuleData* aRuleData,
                                 nsStyleContext* aContext,
                                 nsRuleNode* aHighestNode,
                                 const RuleDetail aRuleDetail,
                                 const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(TextReset, (), text, parentText)

  
  const nsCSSValue* verticalAlignValue = aRuleData->ValueForVerticalAlign();
  if (!SetCoord(*verticalAlignValue, text->mVerticalAlign,
                parentText->mVerticalAlign,
                SETCOORD_LPH | SETCOORD_ENUMERATED | SETCOORD_STORE_CALC,
                aContext, mPresContext, canStoreInRuleTree)) {
    if (eCSSUnit_Initial == verticalAlignValue->GetUnit()) {
      text->mVerticalAlign.SetIntValue(NS_STYLE_VERTICAL_ALIGN_BASELINE,
                                       eStyleUnit_Enumerated);
    }
  }

  
  SetDiscrete(*aRuleData->ValueForTextBlink(), text->mTextBlink,
              canStoreInRuleTree, SETDSC_ENUMERATED, parentText->mTextBlink,
              NS_STYLE_TEXT_BLINK_NONE, 0, 0, 0, 0);

  
  const nsCSSValue* decorationLineValue =
    aRuleData->ValueForTextDecorationLine();
  if (eCSSUnit_Enumerated == decorationLineValue->GetUnit()) {
    PRInt32 td = decorationLineValue->GetIntValue();
    text->mTextDecorationLine = td;
    if (td & NS_STYLE_TEXT_DECORATION_LINE_PREF_ANCHORS) {
      PRBool underlineLinks =
        mPresContext->GetCachedBoolPref(kPresContext_UnderlineLinks);
      if (underlineLinks) {
        text->mTextDecorationLine |= NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE;
      }
      else {
        text->mTextDecorationLine &= ~NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE;
      }
    }
  } else if (eCSSUnit_Inherit == decorationLineValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    text->mTextDecorationLine = parentText->mTextDecorationLine;
  } else if (eCSSUnit_Initial == decorationLineValue->GetUnit()) {
    text->mTextDecorationLine = NS_STYLE_TEXT_DECORATION_LINE_NONE;
  }

  
  const nsCSSValue* decorationColorValue =
    aRuleData->ValueForTextDecorationColor();
  nscolor decorationColor;
  if (eCSSUnit_Inherit == decorationColorValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    if (parentContext) {
      PRBool isForeground;
      parentText->GetDecorationColor(decorationColor, isForeground);
      if (isForeground) {
        text->SetDecorationColor(parentContext->GetStyleColor()->mColor);
      } else {
        text->SetDecorationColor(decorationColor);
      }
    } else {
      text->SetDecorationColorToForeground();
    }
  }
  else if (SetColor(*decorationColorValue, 0, mPresContext, aContext,
                    decorationColor, canStoreInRuleTree)) {
    text->SetDecorationColor(decorationColor);
  }
  else if (eCSSUnit_Initial == decorationColorValue->GetUnit() ||
           eCSSUnit_Enumerated == decorationColorValue->GetUnit()) {
    NS_ABORT_IF_FALSE(eCSSUnit_Enumerated != decorationColorValue->GetUnit() ||
                      decorationColorValue->GetIntValue() ==
                        NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR,
                      "unexpected enumerated value");
    text->SetDecorationColorToForeground();
  }
  else if (eCSSUnit_Initial == decorationColorValue->GetUnit()) {
    text->SetDecorationColorToForeground();
  }

  
  const nsCSSValue* decorationStyleValue =
    aRuleData->ValueForTextDecorationStyle();
  if (eCSSUnit_Enumerated == decorationStyleValue->GetUnit()) {
    text->SetDecorationStyle(decorationStyleValue->GetIntValue());
  } else if (eCSSUnit_Inherit == decorationStyleValue->GetUnit()) {
    text->SetDecorationStyle(parentText->GetDecorationStyle());
    canStoreInRuleTree = PR_FALSE;
  } else if (eCSSUnit_Initial == decorationStyleValue->GetUnit()) {
    text->SetDecorationStyle(NS_STYLE_TEXT_DECORATION_STYLE_SOLID);
  }

  
  SetDiscrete(*aRuleData->ValueForUnicodeBidi(), text->mUnicodeBidi, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentText->mUnicodeBidi,
              NS_STYLE_UNICODE_BIDI_NORMAL, 0, 0, 0, 0);

  COMPUTE_END_RESET(TextReset, text)
}

const void*
nsRuleNode::ComputeUserInterfaceData(void* aStartStruct,
                                     const nsRuleData* aRuleData,
                                     nsStyleContext* aContext,
                                     nsRuleNode* aHighestNode,
                                     const RuleDetail aRuleDetail,
                                     const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(UserInterface, (), ui, parentUI)

  
  const nsCSSValue* cursorValue = aRuleData->ValueForCursor();
  nsCSSUnit cursorUnit = cursorValue->GetUnit();
  if (cursorUnit != eCSSUnit_Null) {
    delete [] ui->mCursorArray;
    ui->mCursorArray = nsnull;
    ui->mCursorArrayLength = 0;

    if (cursorUnit == eCSSUnit_Inherit) {
      canStoreInRuleTree = PR_FALSE;
      ui->mCursor = parentUI->mCursor;
      ui->CopyCursorArrayFrom(*parentUI);
    }
    else if (cursorUnit == eCSSUnit_Initial) {
      ui->mCursor = NS_STYLE_CURSOR_AUTO;
    }
    else {
      
      
      NS_ABORT_IF_FALSE(cursorUnit == eCSSUnit_List ||
                        cursorUnit == eCSSUnit_ListDep,
                        nsPrintfCString(64, "unrecognized cursor unit %d",
                                        cursorUnit).get());
      const nsCSSValueList* list = cursorValue->GetListValue();
      const nsCSSValueList* list2 = list;
      PRUint32 arrayLength = 0;
      for ( ; list->mValue.GetUnit() == eCSSUnit_Array; list = list->mNext)
        if (list->mValue.GetArrayValue()->Item(0).GetImageValue())
          ++arrayLength;

      if (arrayLength != 0) {
        ui->mCursorArray = new nsCursorImage[arrayLength];
        if (ui->mCursorArray) {
          ui->mCursorArrayLength = arrayLength;

          for (nsCursorImage *item = ui->mCursorArray;
               list2->mValue.GetUnit() == eCSSUnit_Array;
               list2 = list2->mNext) {
            nsCSSValue::Array *arr = list2->mValue.GetArrayValue();
            imgIRequest *req = arr->Item(0).GetImageValue();
            if (req) {
              item->SetImage(req);
              if (arr->Item(1).GetUnit() != eCSSUnit_Null) {
                item->mHaveHotspot = PR_TRUE;
                item->mHotspotX = arr->Item(1).GetFloatValue(),
                item->mHotspotY = arr->Item(2).GetFloatValue();
              }
              ++item;
            }
          }
        }
      }

      NS_ASSERTION(list, "Must have non-array value at the end");
      NS_ASSERTION(list->mValue.GetUnit() == eCSSUnit_Enumerated,
                   "Unexpected fallback value at end of cursor list");
      ui->mCursor = list->mValue.GetIntValue();
    }
  }

  
  SetDiscrete(*aRuleData->ValueForUserInput(),
              ui->mUserInput, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentUI->mUserInput,
              NS_STYLE_USER_INPUT_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForUserModify(),
              ui->mUserModify, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentUI->mUserModify,
              NS_STYLE_USER_MODIFY_READ_ONLY,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForUserFocus(),
              ui->mUserFocus, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentUI->mUserFocus,
              NS_STYLE_USER_FOCUS_NONE, 0, 0, 0, 0);

  COMPUTE_END_INHERITED(UserInterface, ui)
}

const void*
nsRuleNode::ComputeUIResetData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(UIReset, (), ui, parentUI)

  
  SetDiscrete(*aRuleData->ValueForUserSelect(),
              ui->mUserSelect, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentUI->mUserSelect,
              NS_STYLE_USER_SELECT_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForImeMode(),
              ui->mIMEMode, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentUI->mIMEMode,
              NS_STYLE_IME_MODE_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForForceBrokenImageIcon(),
              ui->mForceBrokenImageIcon,
              canStoreInRuleTree,
              SETDSC_INTEGER,
              parentUI->mForceBrokenImageIcon,
              0, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForWindowShadow(),
              ui->mWindowShadow, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentUI->mWindowShadow,
              NS_STYLE_WINDOW_SHADOW_DEFAULT, 0, 0, 0, 0);

  COMPUTE_END_RESET(UIReset, ui)
}



struct TransitionPropInfo {
  nsCSSProperty property;
  
  PRUint32 nsStyleDisplay::* sdCount;
};



static const TransitionPropInfo transitionPropInfo[4] = {
  { eCSSProperty_transition_delay,
    &nsStyleDisplay::mTransitionDelayCount },
  { eCSSProperty_transition_duration,
    &nsStyleDisplay::mTransitionDurationCount },
  { eCSSProperty_transition_property,
    &nsStyleDisplay::mTransitionPropertyCount },
  { eCSSProperty_transition_timing_function,
    &nsStyleDisplay::mTransitionTimingFunctionCount },
};

#ifdef MOZ_CSS_ANIMATIONS


static const TransitionPropInfo animationPropInfo[8] = {
  { eCSSProperty_animation_delay,
    &nsStyleDisplay::mAnimationDelayCount },
  { eCSSProperty_animation_duration,
    &nsStyleDisplay::mAnimationDurationCount },
  { eCSSProperty_animation_name,
    &nsStyleDisplay::mAnimationNameCount },
  { eCSSProperty_animation_timing_function,
    &nsStyleDisplay::mAnimationTimingFunctionCount },
  { eCSSProperty_animation_direction,
    &nsStyleDisplay::mAnimationDirectionCount },
  { eCSSProperty_animation_fill_mode,
    &nsStyleDisplay::mAnimationFillModeCount },
  { eCSSProperty_animation_play_state,
    &nsStyleDisplay::mAnimationPlayStateCount },
  { eCSSProperty_animation_iteration_count,
    &nsStyleDisplay::mAnimationIterationCountCount },
};
#endif



struct TransitionPropData {
  const nsCSSValueList *list;
  nsCSSUnit unit;
  PRUint32 num;
};

static PRUint32
CountTransitionProps(const TransitionPropInfo* aInfo,
                     TransitionPropData* aData,
                     size_t aLength,
                     nsStyleDisplay* aDisplay,
                     const nsStyleDisplay* aParentDisplay,
                     const nsRuleData* aRuleData,
                     PRBool& aCanStoreInRuleTree)
{
  
  
  
  
  
  
  
  

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  

  PRUint32 numTransitions = 0;
  for (size_t i = 0; i < aLength; ++i) {
    const TransitionPropInfo& info = aInfo[i];
    TransitionPropData& data = aData[i];

    
    

    const nsCSSValue& value = *aRuleData->ValueFor(info.property);
    data.unit = value.GetUnit();
    data.list = (value.GetUnit() == eCSSUnit_List ||
                 value.GetUnit() == eCSSUnit_ListDep)
                  ? value.GetListValue() : nsnull;

    
    
    
    
    
    
    
    
    
    


    
    if (data.unit == eCSSUnit_Inherit) {
      data.num = aParentDisplay->*(info.sdCount);
      aCanStoreInRuleTree = PR_FALSE;
    } else if (data.list) {
      data.num = ListLength(data.list);
    } else {
      data.num = aDisplay->*(info.sdCount);
    }
    if (data.num > numTransitions)
      numTransitions = data.num;
  }

  return numTransitions;
}

static void
ComputeTimingFunction(const nsCSSValue& aValue, nsTimingFunction& aResult)
{
  switch (aValue.GetUnit()) {
    case eCSSUnit_Enumerated:
      aResult = nsTimingFunction(aValue.GetIntValue());
      break;
    case eCSSUnit_Cubic_Bezier:
      {
        nsCSSValue::Array* array = aValue.GetArrayValue();
        NS_ASSERTION(array && array->Count() == 4,
                     "Need 4 control points");
        aResult = nsTimingFunction(array->Item(0).GetFloatValue(),
                                   array->Item(1).GetFloatValue(),
                                   array->Item(2).GetFloatValue(),
                                   array->Item(3).GetFloatValue());
      }
      break;
    case eCSSUnit_Steps:
      {
        nsCSSValue::Array* array = aValue.GetArrayValue();
        NS_ASSERTION(array && array->Count() == 2,
                     "Need 2 items");
        NS_ASSERTION(array->Item(0).GetUnit() == eCSSUnit_Integer,
                     "unexpected first value");
        NS_ASSERTION(array->Item(1).GetUnit() == eCSSUnit_Enumerated &&
                     (array->Item(1).GetIntValue() ==
                       NS_STYLE_TRANSITION_TIMING_FUNCTION_STEP_START ||
                      array->Item(1).GetIntValue() ==
                       NS_STYLE_TRANSITION_TIMING_FUNCTION_STEP_END),
                     "unexpected second value");
        nsTimingFunction::Type type =
          (array->Item(1).GetIntValue() ==
            NS_STYLE_TRANSITION_TIMING_FUNCTION_STEP_END)
            ? nsTimingFunction::StepEnd : nsTimingFunction::StepStart;
        aResult = nsTimingFunction(type, array->Item(0).GetIntValue());
      }
      break;
    default:
      NS_NOTREACHED("Invalid transition property unit");
  }
}

const void*
nsRuleNode::ComputeDisplayData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Display, (), display, parentDisplay)

  
  
  TransitionPropData transitionPropData[4];
  TransitionPropData& delay = transitionPropData[0];
  TransitionPropData& duration = transitionPropData[1];
  TransitionPropData& property = transitionPropData[2];
  TransitionPropData& timingFunction = transitionPropData[3];

#define FOR_ALL_TRANSITION_PROPS(var_) \
                                      for (PRUint32 var_ = 0; var_ < 4; ++var_)

  
  PRUint32 numTransitions =
    CountTransitionProps(transitionPropInfo, transitionPropData,
                         NS_ARRAY_LENGTH(transitionPropData),
                         display, parentDisplay, aRuleData,
                         canStoreInRuleTree);

  if (!display->mTransitions.SetLength(numTransitions)) {
    NS_WARNING("failed to allocate transitions array");
    display->mTransitions.SetLength(1);
    NS_ABORT_IF_FALSE(display->mTransitions.Length() == 1,
                      "could not allocate using auto array buffer");
    numTransitions = 1;
    FOR_ALL_TRANSITION_PROPS(p) {
      TransitionPropData& d = transitionPropData[p];

      d.num = 1;
    }
  }

  FOR_ALL_TRANSITION_PROPS(p) {
    const TransitionPropInfo& i = transitionPropInfo[p];
    TransitionPropData& d = transitionPropData[p];

    display->*(i.sdCount) = d.num;
  }

  
  for (PRUint32 i = 0; i < numTransitions; ++i) {
    nsTransition *transition = &display->mTransitions[i];

    if (i >= delay.num) {
      transition->SetDelay(display->mTransitions[i % delay.num].GetDelay());
    } else if (delay.unit == eCSSUnit_Inherit) {
      
      
      
      NS_ABORT_IF_FALSE(i < parentDisplay->mTransitionDelayCount,
                        "delay.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      transition->SetDelay(parentDisplay->mTransitions[i].GetDelay());
    } else if (delay.unit == eCSSUnit_Initial) {
      transition->SetDelay(0.0);
    } else if (delay.list) {
      switch (delay.list->mValue.GetUnit()) {
        case eCSSUnit_Seconds:
          transition->SetDelay(PR_MSEC_PER_SEC *
                               delay.list->mValue.GetFloatValue());
          break;
        case eCSSUnit_Milliseconds:
          transition->SetDelay(delay.list->mValue.GetFloatValue());
          break;
        default:
          NS_NOTREACHED("Invalid delay unit");
      }
    }

    if (i >= duration.num) {
      transition->SetDuration(
        display->mTransitions[i % duration.num].GetDuration());
    } else if (duration.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mTransitionDurationCount,
                        "duration.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      transition->SetDuration(parentDisplay->mTransitions[i].GetDuration());
    } else if (duration.unit == eCSSUnit_Initial) {
      transition->SetDuration(0.0);
    } else if (duration.list) {
      switch (duration.list->mValue.GetUnit()) {
        case eCSSUnit_Seconds:
          transition->SetDuration(PR_MSEC_PER_SEC *
                                  duration.list->mValue.GetFloatValue());
          break;
        case eCSSUnit_Milliseconds:
          transition->SetDuration(duration.list->mValue.GetFloatValue());
          break;
        default:
          NS_NOTREACHED("Invalid duration unit");
      }
    }

    if (i >= property.num) {
      transition->CopyPropertyFrom(display->mTransitions[i % property.num]);
    } else if (property.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mTransitionPropertyCount,
                        "property.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      transition->CopyPropertyFrom(parentDisplay->mTransitions[i]);
    } else if (property.unit == eCSSUnit_Initial ||
               property.unit == eCSSUnit_All) {
      transition->SetProperty(eCSSPropertyExtra_all_properties);
    } else if (property.unit == eCSSUnit_None) {
      transition->SetProperty(eCSSPropertyExtra_no_properties);
    } else if (property.list) {
      NS_ABORT_IF_FALSE(property.list->mValue.GetUnit() == eCSSUnit_Ident,
                        nsPrintfCString(64,
                                        "Invalid transition property unit %d",
                                        property.list->mValue.GetUnit()).get());

      nsDependentString
        propertyStr(property.list->mValue.GetStringBufferValue());
      nsCSSProperty prop = nsCSSProps::LookupProperty(propertyStr);
      if (prop == eCSSProperty_UNKNOWN) {
        transition->SetUnknownProperty(propertyStr);
      } else {
        transition->SetProperty(prop);
      }
    }

    if (i >= timingFunction.num) {
      transition->SetTimingFunction(
        display->mTransitions[i % timingFunction.num].GetTimingFunction());
    } else if (timingFunction.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mTransitionTimingFunctionCount,
                        "timingFunction.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      transition->SetTimingFunction(
        parentDisplay->mTransitions[i].GetTimingFunction());
    } else if (timingFunction.unit == eCSSUnit_Initial) {
      transition->SetTimingFunction(
        nsTimingFunction(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE));
    } else if (timingFunction.list) {
      ComputeTimingFunction(timingFunction.list->mValue,
                            transition->TimingFunctionSlot());
    }

    FOR_ALL_TRANSITION_PROPS(p) {
      const TransitionPropInfo& info = transitionPropInfo[p];
      TransitionPropData& d = transitionPropData[p];

      
      
      if (d.list) {
        d.list = d.list->mNext ? d.list->mNext :
          aRuleData->ValueFor(info.property)->GetListValue();
      }
    }
  }

#ifdef MOZ_CSS_ANIMATIONS
  
  
  TransitionPropData animationPropData[8];
  TransitionPropData& animDelay = animationPropData[0];
  TransitionPropData& animDuration = animationPropData[1];
  TransitionPropData& animName = animationPropData[2];
  TransitionPropData& animTimingFunction = animationPropData[3];
  TransitionPropData& animDirection = animationPropData[4];
  TransitionPropData& animFillMode = animationPropData[5];
  TransitionPropData& animPlayState = animationPropData[6];
  TransitionPropData& animIterationCount = animationPropData[7];

#define FOR_ALL_ANIMATION_PROPS(var_) \
    for (PRUint32 var_ = 0; var_ < 8; ++var_)

  

  PRUint32 numAnimations =
    CountTransitionProps(animationPropInfo, animationPropData,
                         NS_ARRAY_LENGTH(animationPropData),
                         display, parentDisplay, aRuleData,
                         canStoreInRuleTree);

  if (!display->mAnimations.SetLength(numAnimations)) {
    NS_WARNING("failed to allocate animations array");
    display->mAnimations.SetLength(1);
    NS_ABORT_IF_FALSE(display->mAnimations.Length() == 1,
                      "could not allocate using auto array buffer");
    numAnimations = 1;
    FOR_ALL_ANIMATION_PROPS(p) {
      TransitionPropData& d = animationPropData[p];

      d.num = 1;
    }
  }

  FOR_ALL_ANIMATION_PROPS(p) {
    const TransitionPropInfo& i = animationPropInfo[p];
    TransitionPropData& d = animationPropData[p];

    display->*(i.sdCount) = d.num;
  }

  
  for (PRUint32 i = 0; i < numAnimations; ++i) {
    nsAnimation *animation = &display->mAnimations[i];

    if (i >= animDelay.num) {
      animation->SetDelay(display->mAnimations[i % animDelay.num].GetDelay());
    } else if (animDelay.unit == eCSSUnit_Inherit) {
      
      
      
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationDelayCount,
                        "animDelay.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetDelay(parentDisplay->mAnimations[i].GetDelay());
    } else if (animDelay.unit == eCSSUnit_Initial) {
      animation->SetDelay(0.0);
    } else if (animDelay.list) {
      switch (animDelay.list->mValue.GetUnit()) {
        case eCSSUnit_Seconds:
          animation->SetDelay(PR_MSEC_PER_SEC *
                              animDelay.list->mValue.GetFloatValue());
          break;
        case eCSSUnit_Milliseconds:
          animation->SetDelay(animDelay.list->mValue.GetFloatValue());
          break;
        default:
          NS_NOTREACHED("Invalid delay unit");
      }
    }

    if (i >= animDuration.num) {
      animation->SetDuration(
        display->mAnimations[i % animDuration.num].GetDuration());
    } else if (animDuration.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationDurationCount,
                        "animDuration.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetDuration(parentDisplay->mAnimations[i].GetDuration());
    } else if (animDuration.unit == eCSSUnit_Initial) {
      animation->SetDuration(0.0);
    } else if (animDuration.list) {
      switch (animDuration.list->mValue.GetUnit()) {
        case eCSSUnit_Seconds:
          animation->SetDuration(PR_MSEC_PER_SEC *
                                 animDuration.list->mValue.GetFloatValue());
          break;
        case eCSSUnit_Milliseconds:
          animation->SetDuration(animDuration.list->mValue.GetFloatValue());
          break;
        default:
          NS_NOTREACHED("Invalid duration unit");
      }
    }

    if (i >= animName.num) {
      animation->SetName(display->mAnimations[i % animName.num].GetName());
    } else if (animName.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationNameCount,
                        "animName.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetName(parentDisplay->mAnimations[i].GetName());
    } else if (animName.unit == eCSSUnit_Initial) {
      animation->SetName(EmptyString());
    } else if (animName.list) {
      switch (animName.list->mValue.GetUnit()) {
        case eCSSUnit_Ident: {
          nsDependentString
            nameStr(animName.list->mValue.GetStringBufferValue());
          animation->SetName(nameStr);
          break;
        }
        case eCSSUnit_None: {
          animation->SetName(EmptyString());
          break;
        }
        default:
          NS_ABORT_IF_FALSE(PR_FALSE,
            nsPrintfCString(64, "Invalid animation-name unit %d",
                                animName.list->mValue.GetUnit()).get());
      }
    }

    if (i >= animTimingFunction.num) {
      animation->SetTimingFunction(
        display->mAnimations[i % animTimingFunction.num].GetTimingFunction());
    } else if (animTimingFunction.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationTimingFunctionCount,
                        "animTimingFunction.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetTimingFunction(
        parentDisplay->mAnimations[i].GetTimingFunction());
    } else if (animTimingFunction.unit == eCSSUnit_Initial) {
      animation->SetTimingFunction(
        nsTimingFunction(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE));
    } else if (animTimingFunction.list) {
      ComputeTimingFunction(animTimingFunction.list->mValue,
                            animation->TimingFunctionSlot());
    }

    if (i >= animDirection.num) {
      animation->SetDirection(display->mAnimations[i % animDirection.num].GetDirection());
    } else if (animDirection.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationDirectionCount,
                        "animDirection.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetDirection(parentDisplay->mAnimations[i].GetDirection());
    } else if (animDirection.unit == eCSSUnit_Initial) {
      animation->SetDirection(NS_STYLE_ANIMATION_DIRECTION_NORMAL);
    } else if (animDirection.list) {
      NS_ABORT_IF_FALSE(animDirection.list->mValue.GetUnit() == eCSSUnit_Enumerated,
                        nsPrintfCString(64,
                                        "Invalid animation-direction unit %d",
                                        animDirection.list->mValue.GetUnit()).get());

      animation->SetDirection(animDirection.list->mValue.GetIntValue());
    }

    if (i >= animFillMode.num) {
      animation->SetFillMode(display->mAnimations[i % animFillMode.num].GetFillMode());
    } else if (animFillMode.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationFillModeCount,
                        "animFillMode.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetFillMode(parentDisplay->mAnimations[i].GetFillMode());
    } else if (animFillMode.unit == eCSSUnit_Initial) {
      animation->SetFillMode(NS_STYLE_ANIMATION_FILL_MODE_NONE);
    } else if (animFillMode.list) {
      NS_ABORT_IF_FALSE(animFillMode.list->mValue.GetUnit() == eCSSUnit_Enumerated,
                        nsPrintfCString(64,
                                        "Invalid animation-fill-mode unit %d",
                                        animFillMode.list->mValue.GetUnit()).get());

      animation->SetFillMode(animFillMode.list->mValue.GetIntValue());
    }

    if (i >= animPlayState.num) {
      animation->SetPlayState(display->mAnimations[i % animPlayState.num].GetPlayState());
    } else if (animPlayState.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationPlayStateCount,
                        "animPlayState.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetPlayState(parentDisplay->mAnimations[i].GetPlayState());
    } else if (animPlayState.unit == eCSSUnit_Initial) {
      animation->SetPlayState(NS_STYLE_ANIMATION_PLAY_STATE_RUNNING);
    } else if (animPlayState.list) {
      NS_ABORT_IF_FALSE(animPlayState.list->mValue.GetUnit() == eCSSUnit_Enumerated,
                        nsPrintfCString(64,
                                        "Invalid animation-play-state unit %d",
                                        animPlayState.list->mValue.GetUnit()).get());

      animation->SetPlayState(animPlayState.list->mValue.GetIntValue());
    }

    if (i >= animIterationCount.num) {
      animation->SetIterationCount(display->mAnimations[i % animIterationCount.num].GetIterationCount());
    } else if (animIterationCount.unit == eCSSUnit_Inherit) {
      NS_ABORT_IF_FALSE(i < parentDisplay->mAnimationIterationCountCount,
                        "animIterationCount.num computed incorrectly");
      NS_ABORT_IF_FALSE(!canStoreInRuleTree,
                        "should have made canStoreInRuleTree false above");
      animation->SetIterationCount(parentDisplay->mAnimations[i].GetIterationCount());
    } else if (animIterationCount.unit == eCSSUnit_Initial) {
      animation->SetIterationCount(1.0f);
    } else if (animIterationCount.list) {
      switch(animIterationCount.list->mValue.GetUnit()) {
        case eCSSUnit_Enumerated:
          NS_ABORT_IF_FALSE(animIterationCount.list->mValue.GetIntValue() ==
                              NS_STYLE_ANIMATION_ITERATION_COUNT_INFINITE,
                            "unexpected value");
          animation->SetIterationCount(NS_IEEEPositiveInfinity());
          break;
        case eCSSUnit_Number:
          animation->SetIterationCount(
            animIterationCount.list->mValue.GetFloatValue());
          break;
        default:
          NS_ABORT_IF_FALSE(PR_FALSE,
                            "unexpected animation-iteration-count unit");
      }
    }

    FOR_ALL_ANIMATION_PROPS(p) {
      const TransitionPropInfo& info = animationPropInfo[p];
      TransitionPropData& d = animationPropData[p];

      
      
      if (d.list) {
        d.list = d.list->mNext ? d.list->mNext :
          aRuleData->ValueFor(info.property)->GetListValue();
      }
    }
  }
#endif

  
  SetFactor(*aRuleData->ValueForOpacity(), display->mOpacity, canStoreInRuleTree,
            parentDisplay->mOpacity, 1.0f, SETFCT_OPACITY);

  
  SetDiscrete(*aRuleData->ValueForDisplay(), display->mDisplay, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mDisplay,
              NS_STYLE_DISPLAY_INLINE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForAppearance(),
              display->mAppearance, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mAppearance,
              NS_THEME_NONE, 0, 0, 0, 0);

  
  const nsCSSValue* bindingValue = aRuleData->ValueForBinding();
  if (eCSSUnit_URL == bindingValue->GetUnit()) {
    nsCSSValue::URL* url = bindingValue->GetURLStructValue();
    NS_ASSERTION(url, "What's going on here?");

    if (NS_LIKELY(url->mURI)) {
      display->mBinding = url;
    } else {
      display->mBinding = nsnull;
    }
  }
  else if (eCSSUnit_None == bindingValue->GetUnit() ||
           eCSSUnit_Initial == bindingValue->GetUnit()) {
    display->mBinding = nsnull;
  }
  else if (eCSSUnit_Inherit == bindingValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    display->mBinding = parentDisplay->mBinding;
  }

  
  SetDiscrete(*aRuleData->ValueForPosition(), display->mPosition, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mPosition,
              NS_STYLE_POSITION_STATIC, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForClear(), display->mBreakType, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mBreakType,
              NS_STYLE_CLEAR_NONE, 0, 0, 0, 0);

  
  
  
  
  
  const nsCSSValue* breakBeforeValue = aRuleData->ValueForPageBreakBefore();
  if (eCSSUnit_Enumerated == breakBeforeValue->GetUnit()) {
    display->mBreakBefore =
      (NS_STYLE_PAGE_BREAK_AVOID != breakBeforeValue->GetIntValue() &&
       NS_STYLE_PAGE_BREAK_AUTO  != breakBeforeValue->GetIntValue());
  }
  else if (eCSSUnit_Initial == breakBeforeValue->GetUnit()) {
    display->mBreakBefore = PR_FALSE;
  }
  else if (eCSSUnit_Inherit == breakBeforeValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    display->mBreakBefore = parentDisplay->mBreakBefore;
  }

  const nsCSSValue* breakAfterValue = aRuleData->ValueForPageBreakAfter();
  if (eCSSUnit_Enumerated == breakAfterValue->GetUnit()) {
    display->mBreakAfter =
      (NS_STYLE_PAGE_BREAK_AVOID != breakAfterValue->GetIntValue() &&
       NS_STYLE_PAGE_BREAK_AUTO  != breakAfterValue->GetIntValue());
  }
  else if (eCSSUnit_Initial == breakAfterValue->GetUnit()) {
    display->mBreakAfter = PR_FALSE;
  }
  else if (eCSSUnit_Inherit == breakAfterValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    display->mBreakAfter = parentDisplay->mBreakAfter;
  }
  

  
  SetDiscrete(*aRuleData->ValueForCssFloat(),
              display->mFloats, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mFloats,
              NS_STYLE_FLOAT_NONE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForOverflowX(),
              display->mOverflowX, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mOverflowX,
              NS_STYLE_OVERFLOW_VISIBLE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForOverflowY(),
              display->mOverflowY, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mOverflowY,
              NS_STYLE_OVERFLOW_VISIBLE, 0, 0, 0, 0);

  
  
  
  if (display->mOverflowX != display->mOverflowY &&
      (display->mOverflowX == NS_STYLE_OVERFLOW_VISIBLE ||
       display->mOverflowX == NS_STYLE_OVERFLOW_CLIP ||
       display->mOverflowY == NS_STYLE_OVERFLOW_VISIBLE ||
       display->mOverflowY == NS_STYLE_OVERFLOW_CLIP)) {
    
    
    canStoreInRuleTree = PR_FALSE;

    
    
    if (display->mOverflowX == NS_STYLE_OVERFLOW_CLIP)
      display->mOverflowX = NS_STYLE_OVERFLOW_HIDDEN;
    if (display->mOverflowY == NS_STYLE_OVERFLOW_CLIP)
      display->mOverflowY = NS_STYLE_OVERFLOW_HIDDEN;

    
    
    if (display->mOverflowX == NS_STYLE_OVERFLOW_VISIBLE)
      display->mOverflowX = NS_STYLE_OVERFLOW_AUTO;
    if (display->mOverflowY == NS_STYLE_OVERFLOW_VISIBLE)
      display->mOverflowY = NS_STYLE_OVERFLOW_AUTO;
  }

  SetDiscrete(*aRuleData->ValueForResize(), display->mResize, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mResize,
              NS_STYLE_RESIZE_NONE, 0, 0, 0, 0);

  
  const nsCSSValue* clipValue = aRuleData->ValueForClip();
  switch (clipValue->GetUnit()) {
  case eCSSUnit_Inherit:
    canStoreInRuleTree = PR_FALSE;
    display->mClipFlags = parentDisplay->mClipFlags;
    display->mClip = parentDisplay->mClip;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Auto:
    display->mClipFlags = NS_STYLE_CLIP_AUTO;
    display->mClip.SetRect(0,0,0,0);
    break;

  case eCSSUnit_Null:
    break;

  case eCSSUnit_Rect: {
    const nsCSSRect& clipRect = clipValue->GetRectValue();

    display->mClipFlags = NS_STYLE_CLIP_RECT;

    if (clipRect.mTop.GetUnit() == eCSSUnit_Auto) {
      display->mClip.y = 0;
      display->mClipFlags |= NS_STYLE_CLIP_TOP_AUTO;
    }
    else if (clipRect.mTop.IsLengthUnit()) {
      display->mClip.y = CalcLength(clipRect.mTop, aContext,
                                    mPresContext, canStoreInRuleTree);
    }

    if (clipRect.mBottom.GetUnit() == eCSSUnit_Auto) {
      
      
      
      display->mClip.height = NS_MAXSIZE;
      display->mClipFlags |= NS_STYLE_CLIP_BOTTOM_AUTO;
    }
    else if (clipRect.mBottom.IsLengthUnit()) {
      display->mClip.height = CalcLength(clipRect.mBottom, aContext,
                                         mPresContext, canStoreInRuleTree) -
                              display->mClip.y;
    }

    if (clipRect.mLeft.GetUnit() == eCSSUnit_Auto) {
      display->mClip.x = 0;
      display->mClipFlags |= NS_STYLE_CLIP_LEFT_AUTO;
    }
    else if (clipRect.mLeft.IsLengthUnit()) {
      display->mClip.x = CalcLength(clipRect.mLeft, aContext,
                                    mPresContext, canStoreInRuleTree);
    }

    if (clipRect.mRight.GetUnit() == eCSSUnit_Auto) {
      
      
      
      display->mClip.width = NS_MAXSIZE;
      display->mClipFlags |= NS_STYLE_CLIP_RIGHT_AUTO;
    }
    else if (clipRect.mRight.IsLengthUnit()) {
      display->mClip.width = CalcLength(clipRect.mRight, aContext,
                                        mPresContext, canStoreInRuleTree) -
                             display->mClip.x;
    }
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false, "unrecognized clip unit");
  }

  if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
    
    
    

    if (nsCSSPseudoElements::firstLetter == aContext->GetPseudo()) {
      
      
      display->mDisplay = NS_STYLE_DISPLAY_INLINE;

      
      
      
      canStoreInRuleTree = PR_FALSE;
    }

    if (display->IsAbsolutelyPositioned()) {
      
      

      
      
      
      display->mOriginalDisplay = display->mDisplay;
      EnsureBlockDisplay(display->mDisplay);
      display->mFloats = NS_STYLE_FLOAT_NONE;

      
      
      
      canStoreInRuleTree = PR_FALSE;
    } else if (display->mFloats != NS_STYLE_FLOAT_NONE) {
      
      

      EnsureBlockDisplay(display->mDisplay);

      
      
      
      canStoreInRuleTree = PR_FALSE;
    }

  }

  
  const nsCSSValue* transformValue = aRuleData->ValueForTransform();
  switch (transformValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_None:
    display->mSpecifiedTransform = nsnull;
    break;

  case eCSSUnit_Inherit:
    display->mSpecifiedTransform = parentDisplay->mSpecifiedTransform;
    if (parentDisplay->mSpecifiedTransform)
      display->mTransform = parentDisplay->mTransform;
    canStoreInRuleTree = PR_FALSE;
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep: {
    const nsCSSValueList* head = transformValue->GetListValue();
    
    if (head->mValue.GetUnit() == eCSSUnit_None) {
      NS_ABORT_IF_FALSE(head->mNext == nsnull, "none must be alone");
      display->mSpecifiedTransform = nsnull;
    } else {
      display->mSpecifiedTransform = head; 
      display->mTransform = nsStyleTransformMatrix::ReadTransforms(head,
                              aContext, mPresContext, canStoreInRuleTree);
    }
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false, "unrecognized transform unit");
  }

  
  const nsCSSValue* transformOriginValue =
    aRuleData->ValueForTransformOrigin();
  if (transformOriginValue->GetUnit() != eCSSUnit_Null) {
#ifdef DEBUG
    PRBool result =
#endif
      SetPairCoords(*transformOriginValue,
                    display->mTransformOrigin[0],
                    display->mTransformOrigin[1],
                    parentDisplay->mTransformOrigin[0],
                    parentDisplay->mTransformOrigin[1],
                    SETCOORD_LPH | SETCOORD_INITIAL_HALF |
                    SETCOORD_BOX_POSITION | SETCOORD_STORE_CALC,
                    aContext, mPresContext, canStoreInRuleTree);
    NS_ASSERTION(result, "Malformed -moz-transform-origin parse!");
  }

  COMPUTE_END_RESET(Display, display)
}

const void*
nsRuleNode::ComputeVisibilityData(void* aStartStruct,
                                  const nsRuleData* aRuleData,
                                  nsStyleContext* aContext,
                                  nsRuleNode* aHighestNode,
                                  const RuleDetail aRuleDetail,
                                  const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Visibility, (mPresContext),
                          visibility, parentVisibility)

  
  SetDiscrete(*aRuleData->ValueForDirection(), visibility->mDirection,
              canStoreInRuleTree,
              SETDSC_ENUMERATED, parentVisibility->mDirection,
              (GET_BIDI_OPTION_DIRECTION(mPresContext->GetBidi())
               == IBMBIDI_TEXTDIRECTION_RTL)
              ? NS_STYLE_DIRECTION_RTL : NS_STYLE_DIRECTION_LTR,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForVisibility(), visibility->mVisible,
              canStoreInRuleTree,
              SETDSC_ENUMERATED, parentVisibility->mVisible,
              NS_STYLE_VISIBILITY_VISIBLE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForPointerEvents(), visibility->mPointerEvents,
              canStoreInRuleTree,
              SETDSC_ENUMERATED, parentVisibility->mPointerEvents,
              NS_STYLE_POINTER_EVENTS_AUTO, 0, 0, 0, 0);

  
  
  const nsCSSValue* langValue = aRuleData->ValueForLang();
  if (eCSSUnit_Ident == langValue->GetUnit()) {
    if (!gLangService) {
      CallGetService(NS_LANGUAGEATOMSERVICE_CONTRACTID, &gLangService);
    }

    if (gLangService) {
      nsAutoString lang;
      langValue->GetStringValue(lang);

      nsContentUtils::ASCIIToLower(lang);
      visibility->mLanguage = do_GetAtom(lang);
    }
  }

  COMPUTE_END_INHERITED(Visibility, visibility)
}

const void*
nsRuleNode::ComputeColorData(void* aStartStruct,
                             const nsRuleData* aRuleData,
                             nsStyleContext* aContext,
                             nsRuleNode* aHighestNode,
                             const RuleDetail aRuleDetail,
                             const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Color, (mPresContext), color, parentColor)

  
  
  
  const nsCSSValue* colorValue = aRuleData->ValueForColor();
  if (colorValue->GetUnit() == eCSSUnit_EnumColor &&
      colorValue->GetIntValue() == NS_COLOR_CURRENTCOLOR) {
    color->mColor = parentColor->mColor;
    canStoreInRuleTree = PR_FALSE;
  }
  else if (colorValue->GetUnit() == eCSSUnit_Initial) {
    color->mColor = mPresContext->DefaultColor();
  }
  else {
    SetColor(*colorValue, parentColor->mColor, mPresContext, aContext,
             color->mColor, canStoreInRuleTree);
  }

  COMPUTE_END_INHERITED(Color, color)
}


template <class SpecifiedValueItem>
struct InitialInheritLocationFor {
};

NS_SPECIALIZE_TEMPLATE
struct InitialInheritLocationFor<nsCSSValueList> {
  static nsCSSValue nsCSSValueList::* Location() {
    return &nsCSSValueList::mValue;
  }
};

NS_SPECIALIZE_TEMPLATE
struct InitialInheritLocationFor<nsCSSValuePairList> {
  static nsCSSValue nsCSSValuePairList::* Location() {
    return &nsCSSValuePairList::mXValue;
  }
};

template <class SpecifiedValueItem, class ComputedValueItem>
struct BackgroundItemComputer {
};

NS_SPECIALIZE_TEMPLATE
struct BackgroundItemComputer<nsCSSValueList, PRUint8>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValueList* aSpecifiedValue,
                           PRUint8& aComputedValue,
                           PRBool& aCanStoreInRuleTree)
  {
    SetDiscrete(aSpecifiedValue->mValue, aComputedValue, aCanStoreInRuleTree,
                SETDSC_ENUMERATED, PRUint8(0), 0, 0, 0, 0, 0);
  }
};

NS_SPECIALIZE_TEMPLATE
struct BackgroundItemComputer<nsCSSValueList, nsStyleImage>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValueList* aSpecifiedValue,
                           nsStyleImage& aComputedValue,
                           PRBool& aCanStoreInRuleTree)
  {
    SetStyleImage(aStyleContext, aSpecifiedValue->mValue, aComputedValue,
                  aCanStoreInRuleTree);
  }
};

struct BackgroundPositionAxis {
  nsCSSValue nsCSSValuePairList::*specified;
  nsStyleBackground::Position::PositionCoord
    nsStyleBackground::Position::*result;
};

static const BackgroundPositionAxis gBGPosAxes[] = {
  { &nsCSSValuePairList::mXValue,
    &nsStyleBackground::Position::mXPosition },
  { &nsCSSValuePairList::mYValue,
    &nsStyleBackground::Position::mYPosition }
};

NS_SPECIALIZE_TEMPLATE
struct BackgroundItemComputer<nsCSSValuePairList, nsStyleBackground::Position>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValuePairList* aSpecifiedValue,
                           nsStyleBackground::Position& aComputedValue,
                           PRBool& aCanStoreInRuleTree)
  {
    nsStyleBackground::Position &position = aComputedValue;
    for (const BackgroundPositionAxis *axis = gBGPosAxes,
                        *axis_end = gBGPosAxes + NS_ARRAY_LENGTH(gBGPosAxes);
         axis != axis_end; ++axis) {
      const nsCSSValue &specified = aSpecifiedValue->*(axis->specified);
      if (eCSSUnit_Percent == specified.GetUnit()) {
        (position.*(axis->result)).mLength = 0;
        (position.*(axis->result)).mPercent = specified.GetPercentValue();
      }
      else if (specified.IsLengthUnit()) {
        (position.*(axis->result)).mLength =
          CalcLength(specified, aStyleContext, aStyleContext->PresContext(),
                     aCanStoreInRuleTree);
        (position.*(axis->result)).mPercent = 0.0f;
      }
      else if (specified.IsCalcUnit()) {
        LengthPercentPairCalcOps ops(aStyleContext,
                                     aStyleContext->PresContext(),
                                     aCanStoreInRuleTree);
        nsRuleNode::ComputedCalc vals = ComputeCalc(specified, ops);
        (position.*(axis->result)).mLength = vals.mLength;
        (position.*(axis->result)).mPercent = vals.mPercent;
      }
      else if (eCSSUnit_Enumerated == specified.GetUnit()) {
        (position.*(axis->result)).mLength = 0;
        (position.*(axis->result)).mPercent =
          GetFloatFromBoxPosition(specified.GetIntValue());
      } else {
        NS_NOTREACHED("unexpected unit");
      }
    }
  }
};


struct BackgroundSizeAxis {
  nsCSSValue nsCSSValuePairList::* specified;
  nsStyleBackground::Size::Dimension nsStyleBackground::Size::* result;
  PRUint8 nsStyleBackground::Size::* type;
};

static const BackgroundSizeAxis gBGSizeAxes[] = {
  { &nsCSSValuePairList::mXValue,
    &nsStyleBackground::Size::mWidth,
    &nsStyleBackground::Size::mWidthType },
  { &nsCSSValuePairList::mYValue,
    &nsStyleBackground::Size::mHeight,
    &nsStyleBackground::Size::mHeightType }
};

NS_SPECIALIZE_TEMPLATE
struct BackgroundItemComputer<nsCSSValuePairList, nsStyleBackground::Size>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValuePairList* aSpecifiedValue,
                           nsStyleBackground::Size& aComputedValue,
                           PRBool& aCanStoreInRuleTree)
  {
    nsStyleBackground::Size &size = aComputedValue;
    for (const BackgroundSizeAxis *axis = gBGSizeAxes,
                        *axis_end = gBGSizeAxes + NS_ARRAY_LENGTH(gBGSizeAxes);
         axis != axis_end; ++axis) {
      const nsCSSValue &specified = aSpecifiedValue->*(axis->specified);
      if (eCSSUnit_Auto == specified.GetUnit()) {
        size.*(axis->type) = nsStyleBackground::Size::eAuto;
      }
      else if (eCSSUnit_Enumerated == specified.GetUnit()) {
        PR_STATIC_ASSERT(nsStyleBackground::Size::eContain ==
                         NS_STYLE_BG_SIZE_CONTAIN);
        PR_STATIC_ASSERT(nsStyleBackground::Size::eCover ==
                         NS_STYLE_BG_SIZE_COVER);
        NS_ABORT_IF_FALSE(specified.GetIntValue() == NS_STYLE_BG_SIZE_CONTAIN ||
                          specified.GetIntValue() == NS_STYLE_BG_SIZE_COVER,
                          "invalid enumerated value for size coordinate");
        size.*(axis->type) = specified.GetIntValue();
      }
      else if (eCSSUnit_Null == specified.GetUnit()) {
        NS_ABORT_IF_FALSE(axis == gBGSizeAxes + 1,
                          "null allowed only as height value, and only "
                          "for contain/cover/initial/inherit");
#ifdef DEBUG
        {
          const nsCSSValue &widthValue = aSpecifiedValue->mXValue;
          NS_ABORT_IF_FALSE(widthValue.GetUnit() != eCSSUnit_Inherit &&
                            widthValue.GetUnit() != eCSSUnit_Initial,
                            "initial/inherit should already have been handled");
          NS_ABORT_IF_FALSE(widthValue.GetUnit() == eCSSUnit_Enumerated &&
                            (widthValue.GetIntValue() == NS_STYLE_BG_SIZE_CONTAIN ||
                             widthValue.GetIntValue() == NS_STYLE_BG_SIZE_COVER),
                            "null height value not corresponding to allowable "
                            "non-null width value");
        }
#endif
        size.*(axis->type) = size.mWidthType;
      }
      else if (eCSSUnit_Percent == specified.GetUnit()) {
        (size.*(axis->result)).mLength = 0;
        (size.*(axis->result)).mPercent = specified.GetPercentValue();
        size.*(axis->type) = nsStyleBackground::Size::eLengthPercentage;
      }
      else if (specified.IsLengthUnit()) {
        (size.*(axis->result)).mLength =
          CalcLength(specified, aStyleContext, aStyleContext->PresContext(),
                     aCanStoreInRuleTree);
        (size.*(axis->result)).mPercent = 0.0f;
        size.*(axis->type) = nsStyleBackground::Size::eLengthPercentage;
      } else {
        NS_ABORT_IF_FALSE(specified.IsCalcUnit(), "unexpected unit");
        LengthPercentPairCalcOps ops(aStyleContext,
                                     aStyleContext->PresContext(),
                                     aCanStoreInRuleTree);
        nsRuleNode::ComputedCalc vals = ComputeCalc(specified, ops);
        (size.*(axis->result)).mLength = vals.mLength;
        (size.*(axis->result)).mPercent = vals.mPercent;
        size.*(axis->type) = nsStyleBackground::Size::eLengthPercentage;
      }
    }

    NS_ABORT_IF_FALSE(size.mWidthType < nsStyleBackground::Size::eDimensionType_COUNT,
                      "bad width type");
    NS_ABORT_IF_FALSE(size.mHeightType < nsStyleBackground::Size::eDimensionType_COUNT,
                      "bad height type");
    NS_ABORT_IF_FALSE((size.mWidthType != nsStyleBackground::Size::eContain &&
                       size.mWidthType != nsStyleBackground::Size::eCover) ||
                      size.mWidthType == size.mHeightType,
                      "contain/cover apply to both dimensions or to neither");
  }
};

template <class ComputedValueItem>
static void
SetBackgroundList(nsStyleContext* aStyleContext,
                  const nsCSSValue& aValue,
                  nsAutoTArray< nsStyleBackground::Layer, 1> &aLayers,
                  const nsAutoTArray<nsStyleBackground::Layer, 1> &aParentLayers,
                  ComputedValueItem nsStyleBackground::Layer::* aResultLocation,
                  ComputedValueItem aInitialValue,
                  PRUint32 aParentItemCount,
                  PRUint32& aItemCount,
                  PRUint32& aMaxItemCount,
                  PRBool& aRebuild,
                  PRBool& aCanStoreInRuleTree)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aRebuild = PR_TRUE;
    aCanStoreInRuleTree = PR_FALSE;
    if (!aLayers.EnsureLengthAtLeast(aParentItemCount)) {
      NS_WARNING("out of memory");
      aParentItemCount = aLayers.Length();
    }
    aItemCount = aParentItemCount;
    for (PRUint32 i = 0; i < aParentItemCount; ++i) {
      aLayers[i].*aResultLocation = aParentLayers[i].*aResultLocation;
    }
    break;

  case eCSSUnit_Initial:
    aRebuild = PR_TRUE;
    aItemCount = 1;
    aLayers[0].*aResultLocation = aInitialValue;
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep: {
    aRebuild = PR_TRUE;
    aItemCount = 0;
    const nsCSSValueList* item = aValue.GetListValue();
    do {
      NS_ASSERTION(item->mValue.GetUnit() != eCSSUnit_Null &&
                   item->mValue.GetUnit() != eCSSUnit_Inherit &&
                   item->mValue.GetUnit() != eCSSUnit_Initial,
                   "unexpected unit");
      ++aItemCount;
      if (!aLayers.EnsureLengthAtLeast(aItemCount)) {
        NS_WARNING("out of memory");
        --aItemCount;
        break;
      }
      BackgroundItemComputer<nsCSSValueList, ComputedValueItem>
        ::ComputeValue(aStyleContext, item,
                       aLayers[aItemCount-1].*aResultLocation,
                       aCanStoreInRuleTree);
      item = item->mNext;
    } while (item);
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false,
                      nsPrintfCString(32, "unexpected unit %d",
                                      aValue.GetUnit()).get());
  }

  if (aItemCount > aMaxItemCount)
    aMaxItemCount = aItemCount;
}

template <class ComputedValueItem>
static void
SetBackgroundPairList(nsStyleContext* aStyleContext,
                      const nsCSSValue& aValue,
                      nsAutoTArray< nsStyleBackground::Layer, 1> &aLayers,
                      const nsAutoTArray<nsStyleBackground::Layer, 1>
                                                                 &aParentLayers,
                      ComputedValueItem nsStyleBackground::Layer::*
                                                                aResultLocation,
                      ComputedValueItem aInitialValue,
                      PRUint32 aParentItemCount,
                      PRUint32& aItemCount,
                      PRUint32& aMaxItemCount,
                      PRBool& aRebuild,
                      PRBool& aCanStoreInRuleTree)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aRebuild = PR_TRUE;
    aCanStoreInRuleTree = PR_FALSE;
    if (!aLayers.EnsureLengthAtLeast(aParentItemCount)) {
      NS_WARNING("out of memory");
      aParentItemCount = aLayers.Length();
    }
    aItemCount = aParentItemCount;
    for (PRUint32 i = 0; i < aParentItemCount; ++i) {
      aLayers[i].*aResultLocation = aParentLayers[i].*aResultLocation;
    }
    break;

  case eCSSUnit_Initial:
    aRebuild = PR_TRUE;
    aItemCount = 1;
    aLayers[0].*aResultLocation = aInitialValue;
    break;

  case eCSSUnit_PairList:
  case eCSSUnit_PairListDep: {
    aRebuild = PR_TRUE;
    aItemCount = 0;
    const nsCSSValuePairList* item = aValue.GetPairListValue();
    do {
      NS_ASSERTION(item->mXValue.GetUnit() != eCSSUnit_Inherit &&
                   item->mXValue.GetUnit() != eCSSUnit_Initial &&
                   item->mYValue.GetUnit() != eCSSUnit_Inherit &&
                   item->mYValue.GetUnit() != eCSSUnit_Initial,
                   "unexpected unit");
      ++aItemCount;
      if (!aLayers.EnsureLengthAtLeast(aItemCount)) {
        NS_WARNING("out of memory");
        --aItemCount;
        break;
      }
      BackgroundItemComputer<nsCSSValuePairList, ComputedValueItem>
        ::ComputeValue(aStyleContext, item,
                       aLayers[aItemCount-1].*aResultLocation,
                       aCanStoreInRuleTree);
      item = item->mNext;
    } while (item);
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false,
                      nsPrintfCString(32, "unexpected unit %d",
                                      aValue.GetUnit()).get());
  }

  if (aItemCount > aMaxItemCount)
    aMaxItemCount = aItemCount;
}

template <class ComputedValueItem>
static void
FillBackgroundList(nsAutoTArray< nsStyleBackground::Layer, 1> &aLayers,
    ComputedValueItem nsStyleBackground::Layer::* aResultLocation,
    PRUint32 aItemCount, PRUint32 aFillCount)
{
  NS_PRECONDITION(aFillCount <= aLayers.Length(), "unexpected array length");
  for (PRUint32 sourceLayer = 0, destLayer = aItemCount;
       destLayer < aFillCount;
       ++sourceLayer, ++destLayer) {
    aLayers[destLayer].*aResultLocation =
      aLayers[sourceLayer].*aResultLocation;
  }
}

const void*
nsRuleNode::ComputeBackgroundData(void* aStartStruct,
                                  const nsRuleData* aRuleData,
                                  nsStyleContext* aContext,
                                  nsRuleNode* aHighestNode,
                                  const RuleDetail aRuleDetail,
                                  const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Background, (), bg, parentBG)

  
  const nsCSSValue* backColorValue = aRuleData->ValueForBackgroundColor();
  if (eCSSUnit_Initial == backColorValue->GetUnit()) {
    bg->mBackgroundColor = NS_RGBA(0, 0, 0, 0);
  } else if (!SetColor(*backColorValue, parentBG->mBackgroundColor,
                       mPresContext, aContext, bg->mBackgroundColor,
                       canStoreInRuleTree)) {
    NS_ASSERTION(eCSSUnit_Null == backColorValue->GetUnit(),
                 "unexpected color unit");
  }

  PRUint32 maxItemCount = 1;
  PRBool rebuild = PR_FALSE;

  
  nsStyleImage initialImage;
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundImage(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mImage,
                    initialImage, parentBG->mImageCount, bg->mImageCount,
                    maxItemCount, rebuild, canStoreInRuleTree);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundRepeat(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mRepeat,
                    PRUint8(NS_STYLE_BG_REPEAT_XY), parentBG->mRepeatCount,
                    bg->mRepeatCount, maxItemCount, rebuild, canStoreInRuleTree);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundAttachment(),
                    bg->mLayers, parentBG->mLayers,
                    &nsStyleBackground::Layer::mAttachment,
                    PRUint8(NS_STYLE_BG_ATTACHMENT_SCROLL),
                    parentBG->mAttachmentCount,
                    bg->mAttachmentCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundClip(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mClip,
                    PRUint8(NS_STYLE_BG_CLIP_BORDER), parentBG->mClipCount,
                    bg->mClipCount, maxItemCount, rebuild, canStoreInRuleTree);

  
  SetDiscrete(*aRuleData->ValueForBackgroundInlinePolicy(),
              bg->mBackgroundInlinePolicy,
              canStoreInRuleTree, SETDSC_ENUMERATED,
              parentBG->mBackgroundInlinePolicy,
              NS_STYLE_BG_INLINE_POLICY_CONTINUOUS, 0, 0, 0, 0);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundOrigin(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mOrigin,
                    PRUint8(NS_STYLE_BG_ORIGIN_PADDING), parentBG->mOriginCount,
                    bg->mOriginCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  nsStyleBackground::Position initialPosition;
  initialPosition.SetInitialValues();
  SetBackgroundPairList(aContext, *aRuleData->ValueForBackgroundPosition(),
                        bg->mLayers,
                        parentBG->mLayers, &nsStyleBackground::Layer::mPosition,
                        initialPosition, parentBG->mPositionCount,
                        bg->mPositionCount, maxItemCount, rebuild,
                        canStoreInRuleTree);

  
  nsStyleBackground::Size initialSize;
  initialSize.SetInitialValues();
  SetBackgroundPairList(aContext, *aRuleData->ValueForBackgroundSize(),
                        bg->mLayers,
                        parentBG->mLayers, &nsStyleBackground::Layer::mSize,
                        initialSize, parentBG->mSizeCount,
                        bg->mSizeCount, maxItemCount, rebuild,
                        canStoreInRuleTree);

  if (rebuild) {
    
    
    bg->mLayers.TruncateLength(maxItemCount);

    PRUint32 fillCount = bg->mImageCount;
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mImage,
                       bg->mImageCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mRepeat,
                       bg->mRepeatCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mAttachment,
                       bg->mAttachmentCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mClip,
                       bg->mClipCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mOrigin,
                       bg->mOriginCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mPosition,
                       bg->mPositionCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mSize,
                       bg->mSizeCount, fillCount);
  }

  
  for (PRUint32 i = 0; i < bg->mImageCount; ++i)
    bg->mLayers[i].TrackImages(aContext->PresContext());

  COMPUTE_END_RESET(Background, bg)
}

const void*
nsRuleNode::ComputeMarginData(void* aStartStruct,
                              const nsRuleData* aRuleData,
                              nsStyleContext* aContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Margin, (), margin, parentMargin)

  
  nsStyleCoord  coord;
  nsCSSRect ourMargin;
  ourMargin.mTop = *aRuleData->ValueForMarginTop();
  ourMargin.mRight = *aRuleData->ValueForMarginRightValue();
  ourMargin.mBottom = *aRuleData->ValueForMarginBottom();
  ourMargin.mLeft = *aRuleData->ValueForMarginLeftValue();
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForMarginLeftLTRSource(),
                       *aRuleData->ValueForMarginLeftRTLSource(),
                       *aRuleData->ValueForMarginStartValue(),
                       *aRuleData->ValueForMarginEndValue(),
                       NS_SIDE_LEFT, ourMargin, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForMarginRightLTRSource(),
                       *aRuleData->ValueForMarginRightRTLSource(),
                       *aRuleData->ValueForMarginEndValue(),
                       *aRuleData->ValueForMarginStartValue(),
                       NS_SIDE_RIGHT, ourMargin, canStoreInRuleTree);
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentMargin->mMargin.Get(side);
    if (SetCoord(ourMargin.*(nsCSSRect::sides[side]),
                 coord, parentCoord,
                 SETCOORD_LPAH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC,
                 aContext, mPresContext, canStoreInRuleTree)) {
      margin->mMargin.Set(side, coord);
    }
  }

  margin->RecalcData();
  COMPUTE_END_RESET(Margin, margin)
}

const void*
nsRuleNode::ComputeBorderData(void* aStartStruct,
                              const nsRuleData* aRuleData,
                              nsStyleContext* aContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Border, (mPresContext), border, parentBorder)

  
  const nsCSSValue* boxShadowValue = aRuleData->ValueForBoxShadow();
  switch (boxShadowValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_None:
    border->mBoxShadow = nsnull;
    break;

  case eCSSUnit_Inherit:
    border->mBoxShadow = parentBorder->mBoxShadow;
    canStoreInRuleTree = PR_FALSE;
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep:
    border->mBoxShadow = GetShadowData(boxShadowValue->GetListValue(),
                                       aContext, PR_TRUE, canStoreInRuleTree);
    break;

  default:
    NS_ABORT_IF_FALSE(false,
                      nsPrintfCString(64, "unrecognized shadow unit %d",
                                      boxShadowValue->GetUnit()).get());
  }

  
  nsStyleCoord  coord;
  nsCSSRect ourBorderWidth;
  ourBorderWidth.mTop = *aRuleData->ValueForBorderTopWidth();
  ourBorderWidth.mRight = *aRuleData->ValueForBorderRightWidthValue();
  ourBorderWidth.mBottom = *aRuleData->ValueForBorderBottomWidth();
  ourBorderWidth.mLeft = *aRuleData->ValueForBorderLeftWidthValue();
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForBorderLeftWidthLTRSource(),
                       *aRuleData->ValueForBorderLeftWidthRTLSource(),
                       *aRuleData->ValueForBorderStartWidthValue(),
                       *aRuleData->ValueForBorderEndWidthValue(),
                       NS_SIDE_LEFT, ourBorderWidth, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForBorderRightWidthLTRSource(),
                       *aRuleData->ValueForBorderRightWidthRTLSource(),
                       *aRuleData->ValueForBorderEndWidthValue(),
                       *aRuleData->ValueForBorderStartWidthValue(),
                       NS_SIDE_RIGHT, ourBorderWidth, canStoreInRuleTree);
  { 
    NS_FOR_CSS_SIDES(side) {
      const nsCSSValue &value = ourBorderWidth.*(nsCSSRect::sides[side]);
      NS_ASSERTION(eCSSUnit_Percent != value.GetUnit(),
                   "Percentage borders not implemented yet "
                   "If implementing, make sure to fix all consumers of "
                   "nsStyleBorder, the IsPercentageAwareChild method, "
                   "the nsAbsoluteContainingBlock::FrameDependsOnContainer "
                   "method, the "
                   "nsLineLayout::IsPercentageAwareReplacedElement method "
                   "and probably some other places");
      if (eCSSUnit_Enumerated == value.GetUnit()) {
        NS_ASSERTION(value.GetIntValue() == NS_STYLE_BORDER_WIDTH_THIN ||
                     value.GetIntValue() == NS_STYLE_BORDER_WIDTH_MEDIUM ||
                     value.GetIntValue() == NS_STYLE_BORDER_WIDTH_THICK,
                     "Unexpected enum value");
        border->SetBorderWidth(side,
                               (mPresContext->GetBorderWidthTable())[value.GetIntValue()]);
      }
      
      else if (SetCoord(value, coord, nsStyleCoord(),
                        SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY,
                        aContext, mPresContext, canStoreInRuleTree)) {
        NS_ASSERTION(coord.GetUnit() == eStyleUnit_Coord, "unexpected unit");
        
        border->SetBorderWidth(side, NS_MAX(coord.GetCoordValue(), 0));
      }
      else if (eCSSUnit_Inherit == value.GetUnit()) {
        canStoreInRuleTree = PR_FALSE;
        border->SetBorderWidth(side,
                               parentBorder->GetComputedBorder().Side(side));
      }
      else if (eCSSUnit_Initial == value.GetUnit()) {
        border->SetBorderWidth(side,
          (mPresContext->GetBorderWidthTable())[NS_STYLE_BORDER_WIDTH_MEDIUM]);
      }
      else {
        NS_ASSERTION(eCSSUnit_Null == value.GetUnit(),
                     "missing case handling border width");
      }
    }
  }

  
  nsCSSRect ourBorderStyle;
  ourBorderStyle.mTop = *aRuleData->ValueForBorderTopStyle();
  ourBorderStyle.mRight = *aRuleData->ValueForBorderRightStyleValue();
  ourBorderStyle.mBottom = *aRuleData->ValueForBorderBottomStyle();
  ourBorderStyle.mLeft = *aRuleData->ValueForBorderLeftStyleValue();
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForBorderLeftStyleLTRSource(),
                       *aRuleData->ValueForBorderLeftStyleRTLSource(),
                       *aRuleData->ValueForBorderStartStyleValue(),
                       *aRuleData->ValueForBorderEndStyleValue(),
                       NS_SIDE_LEFT, ourBorderStyle, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForBorderRightStyleLTRSource(),
                       *aRuleData->ValueForBorderRightStyleRTLSource(),
                       *aRuleData->ValueForBorderEndStyleValue(),
                       *aRuleData->ValueForBorderStartStyleValue(),
                       NS_SIDE_RIGHT, ourBorderStyle, canStoreInRuleTree);
  { 
    NS_FOR_CSS_SIDES(side) {
      const nsCSSValue &value = ourBorderStyle.*(nsCSSRect::sides[side]);
      nsCSSUnit unit = value.GetUnit();
      NS_ABORT_IF_FALSE(eCSSUnit_None != unit,
                        "'none' should be handled as enumerated value");
      if (eCSSUnit_Enumerated == unit) {
        border->SetBorderStyle(side, value.GetIntValue());
      }
      else if (eCSSUnit_Initial == unit) {
        border->SetBorderStyle(side, NS_STYLE_BORDER_STYLE_NONE);
      }
      else if (eCSSUnit_Inherit == unit) {
        canStoreInRuleTree = PR_FALSE;
        border->SetBorderStyle(side, parentBorder->GetBorderStyle(side));
      }
    }
  }

  
  nscolor borderColor;
  nscolor unused = NS_RGB(0,0,0);

  static const nsCSSProperty borderColorsProps[] = {
    eCSSProperty_border_top_colors,
    eCSSProperty_border_right_colors,
    eCSSProperty_border_bottom_colors,
    eCSSProperty_border_left_colors
  };

  NS_FOR_CSS_SIDES(side) {
    const nsCSSValue& value = *aRuleData->ValueFor(borderColorsProps[side]);
    switch (value.GetUnit()) {
    case eCSSUnit_Null:
      break;

    case eCSSUnit_Initial:
    case eCSSUnit_None:
      border->ClearBorderColors(side);
      break;

    case eCSSUnit_Inherit: {
      canStoreInRuleTree = PR_FALSE;
      nsBorderColors *parentColors;
      parentBorder->GetCompositeColors(side, &parentColors);
      if (parentColors) {
        border->EnsureBorderColors();
        border->ClearBorderColors(side);
        border->mBorderColors[side] = parentColors->Clone();
      } else {
        border->ClearBorderColors(side);
      }
      break;
    }

    case eCSSUnit_List:
    case eCSSUnit_ListDep: {
      
      
      border->EnsureBorderColors();
      border->ClearBorderColors(side);
      const nsCSSValueList* list = value.GetListValue();
      while (list) {
        if (SetColor(list->mValue, unused, mPresContext,
                     aContext, borderColor, canStoreInRuleTree))
          border->AppendBorderColor(side, borderColor);
        else {
          NS_NOTREACHED("unexpected item in -moz-border-*-colors list");
        }
        list = list->mNext;
      }
      break;
    }

    default:
      NS_ABORT_IF_FALSE(false, "unrecognized border color unit");
    }
  }

  
  PRBool foreground;
  nsCSSRect ourBorderColor;
  ourBorderColor.mTop = *aRuleData->ValueForBorderTopColor();
  ourBorderColor.mRight = *aRuleData->ValueForBorderRightColorValue();
  ourBorderColor.mBottom = *aRuleData->ValueForBorderBottomColor();
  ourBorderColor.mLeft = *aRuleData->ValueForBorderLeftColorValue();
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForBorderLeftColorLTRSource(),
                       *aRuleData->ValueForBorderLeftColorRTLSource(),
                       *aRuleData->ValueForBorderStartColorValue(),
                       *aRuleData->ValueForBorderEndColorValue(),
                       NS_SIDE_LEFT, ourBorderColor, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForBorderRightColorLTRSource(),
                       *aRuleData->ValueForBorderRightColorRTLSource(),
                       *aRuleData->ValueForBorderEndColorValue(),
                       *aRuleData->ValueForBorderStartColorValue(),
                       NS_SIDE_RIGHT, ourBorderColor, canStoreInRuleTree);
  { 
    NS_FOR_CSS_SIDES(side) {
      const nsCSSValue &value = ourBorderColor.*(nsCSSRect::sides[side]);
      if (eCSSUnit_Inherit == value.GetUnit()) {
        canStoreInRuleTree = PR_FALSE;
        if (parentContext) {
          parentBorder->GetBorderColor(side, borderColor, foreground);
          if (foreground) {
            
            
            
            
            
            border->SetBorderColor(side, parentContext->GetStyleColor()->mColor);
          } else
            border->SetBorderColor(side, borderColor);
        } else {
          
          border->SetBorderToForeground(side);
        }
      }
      else if (SetColor(value, unused, mPresContext, aContext, borderColor,
                        canStoreInRuleTree)) {
        border->SetBorderColor(side, borderColor);
      }
      else if (eCSSUnit_Enumerated == value.GetUnit()) {
        switch (value.GetIntValue()) {
          case NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR:
            border->SetBorderToForeground(side);
            break;
        }
      }
      else if (eCSSUnit_Initial == value.GetUnit()) {
        border->SetBorderToForeground(side);
      }
    }
  }

  
  {
    const nsCSSProperty* subprops =
      nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_radius);
    NS_FOR_CSS_FULL_CORNERS(corner) {
      int cx = NS_FULL_TO_HALF_CORNER(corner, false);
      int cy = NS_FULL_TO_HALF_CORNER(corner, true);
      const nsCSSValue& radius = *aRuleData->ValueFor(subprops[corner]);
      nsStyleCoord parentX = parentBorder->mBorderRadius.Get(cx);
      nsStyleCoord parentY = parentBorder->mBorderRadius.Get(cy);
      nsStyleCoord coordX, coordY;

      if (SetPairCoords(radius, coordX, coordY, parentX, parentY,
                        SETCOORD_LPH | SETCOORD_INITIAL_ZERO |
                          SETCOORD_STORE_CALC,
                        aContext, mPresContext, canStoreInRuleTree)) {
        border->mBorderRadius.Set(cx, coordX);
        border->mBorderRadius.Set(cy, coordY);
      }
    }
  }

  
  SetDiscrete(*aRuleData->ValueForFloatEdge(),
              border->mFloatEdge, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentBorder->mFloatEdge,
              NS_STYLE_FLOAT_EDGE_CONTENT, 0, 0, 0, 0);

  
  const nsCSSValue* borderImageValue = aRuleData->ValueForBorderImage();
  if (eCSSUnit_Array == borderImageValue->GetUnit()) {
    nsCSSValue::Array *arr = borderImageValue->GetArrayValue();

    
    if (eCSSUnit_Image == arr->Item(0).GetUnit()) {
      NS_SET_IMAGE_REQUEST(border->SetBorderImage,
                           aContext,
                           arr->Item(0).GetImageValue())
    }

    
    NS_FOR_CSS_SIDES(side) {
      if (SetAbsCoord(arr->Item(1 + side), coord,
                      SETCOORD_FACTOR | SETCOORD_PERCENT)) {
        border->mBorderImageSplit.Set(side, coord);
      }
    }

    
    
    if (eCSSUnit_Null != arr->Item(5).GetUnit()) {
      NS_FOR_CSS_SIDES(side) {
        
        if (!SetCoord(arr->Item(5 + side), coord, nsStyleCoord(),
                      SETCOORD_LENGTH, aContext, mPresContext,
                      canStoreInRuleTree)) {
          NS_NOTREACHED("SetCoord for border-width replacement from border-image failed");
        }
        if (coord.GetUnit() == eStyleUnit_Coord) {
          border->SetBorderImageWidthOverride(side, coord.GetCoordValue());
        } else {
          NS_WARNING("a border-width replacement from border-image "
                     "has a unit that's not eStyleUnit_Coord");
          border->SetBorderImageWidthOverride(side, 0);
        }
      }
      border->mHaveBorderImageWidth = PR_TRUE;
    } else {
      border->mHaveBorderImageWidth = PR_FALSE;
    }

    
    if (eCSSUnit_Null == arr->Item(9).GetUnit()) {
      
      border->mBorderImageHFill = NS_STYLE_BORDER_IMAGE_STRETCH;
      border->mBorderImageVFill = NS_STYLE_BORDER_IMAGE_STRETCH;
    } else {
      
      border->mBorderImageHFill = arr->Item(9).GetIntValue();
      if (eCSSUnit_Null == arr->Item(10).GetUnit()) {
        
        border->mBorderImageVFill = border->mBorderImageHFill;
      } else {
        
        border->mBorderImageVFill = arr->Item(10).GetIntValue();
      }
    }
  } else if (eCSSUnit_None == borderImageValue->GetUnit() ||
             eCSSUnit_Initial == borderImageValue->GetUnit()) {
    border->mHaveBorderImageWidth = PR_FALSE;
    border->SetBorderImage(nsnull);
  } else if (eCSSUnit_Inherit == borderImageValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    NS_FOR_CSS_SIDES(side) {
      border->SetBorderImageWidthOverride(side, parentBorder->mBorderImageWidth.Side(side));
    }
    border->mBorderImageSplit = parentBorder->mBorderImageSplit;
    border->mBorderImageHFill = parentBorder->mBorderImageHFill;
    border->mBorderImageVFill = parentBorder->mBorderImageVFill;
    border->mHaveBorderImageWidth = parentBorder->mHaveBorderImageWidth;
    NS_SET_IMAGE_REQUEST(border->SetBorderImage, aContext,
                         parentBorder->GetBorderImage())
  }

  if (border->HasBorderImage())
    border->TrackImage(aContext->PresContext());

  COMPUTE_END_RESET(Border, border)
}

const void*
nsRuleNode::ComputePaddingData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Padding, (), padding, parentPadding)

  
  nsStyleCoord  coord;
  nsCSSRect ourPadding;
  ourPadding.mTop = *aRuleData->ValueForPaddingTop();
  ourPadding.mRight = *aRuleData->ValueForPaddingRightValue();
  ourPadding.mBottom = *aRuleData->ValueForPaddingBottom();
  ourPadding.mLeft = *aRuleData->ValueForPaddingLeftValue();
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForPaddingLeftLTRSource(),
                       *aRuleData->ValueForPaddingLeftRTLSource(),
                       *aRuleData->ValueForPaddingStartValue(),
                       *aRuleData->ValueForPaddingEndValue(),
                       NS_SIDE_LEFT, ourPadding, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       *aRuleData->ValueForPaddingRightLTRSource(),
                       *aRuleData->ValueForPaddingRightRTLSource(),
                       *aRuleData->ValueForPaddingEndValue(),
                       *aRuleData->ValueForPaddingStartValue(),
                       NS_SIDE_RIGHT, ourPadding, canStoreInRuleTree);
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentPadding->mPadding.Get(side);
    if (SetCoord(ourPadding.*(nsCSSRect::sides[side]),
                 coord, parentCoord,
                 SETCOORD_LPH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC,
                 aContext, mPresContext, canStoreInRuleTree)) {
      padding->mPadding.Set(side, coord);
    }
  }

  padding->RecalcData();
  COMPUTE_END_RESET(Padding, padding)
}

const void*
nsRuleNode::ComputeOutlineData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Outline, (mPresContext), outline, parentOutline)

  
  const nsCSSValue* outlineWidthValue = aRuleData->ValueForOutlineWidth();
  if (eCSSUnit_Initial == outlineWidthValue->GetUnit()) {
    outline->mOutlineWidth =
      nsStyleCoord(NS_STYLE_BORDER_WIDTH_MEDIUM, eStyleUnit_Enumerated);
  }
  else {
    SetCoord(*outlineWidthValue, outline->mOutlineWidth,
             parentOutline->mOutlineWidth,
             SETCOORD_LEH | SETCOORD_CALC_LENGTH_ONLY, aContext,
             mPresContext, canStoreInRuleTree);
  }

  
  nsStyleCoord tempCoord;
  const nsCSSValue* outlineOffsetValue = aRuleData->ValueForOutlineOffset();
  if (SetCoord(*outlineOffsetValue, tempCoord,
               nsStyleCoord(parentOutline->mOutlineOffset,
                            nsStyleCoord::CoordConstructor),
               SETCOORD_LH | SETCOORD_INITIAL_ZERO | SETCOORD_CALC_LENGTH_ONLY,
               aContext, mPresContext, canStoreInRuleTree)) {
    outline->mOutlineOffset = tempCoord.GetCoordValue();
  } else {
    NS_ASSERTION(outlineOffsetValue->GetUnit() == eCSSUnit_Null,
                 "unexpected unit");
  }

  
  nscolor outlineColor;
  nscolor unused = NS_RGB(0,0,0);
  const nsCSSValue* outlineColorValue = aRuleData->ValueForOutlineColor();
  if (eCSSUnit_Inherit == outlineColorValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    if (parentContext) {
      if (parentOutline->GetOutlineColor(outlineColor))
        outline->SetOutlineColor(outlineColor);
      else {
#ifdef GFX_HAS_INVERT
        outline->SetOutlineInitialColor();
#else
        
        
        
        
        
        outline->SetOutlineColor(parentContext->GetStyleColor()->mColor);
#endif
      }
    } else {
      outline->SetOutlineInitialColor();
    }
  }
  else if (SetColor(*outlineColorValue, unused, mPresContext,
                    aContext, outlineColor, canStoreInRuleTree))
    outline->SetOutlineColor(outlineColor);
  else if (eCSSUnit_Enumerated == outlineColorValue->GetUnit() ||
           eCSSUnit_Initial == outlineColorValue->GetUnit()) {
    outline->SetOutlineInitialColor();
  }

  
  {
    const nsCSSProperty* subprops =
      nsCSSProps::SubpropertyEntryFor(eCSSProperty__moz_outline_radius);
    NS_FOR_CSS_FULL_CORNERS(corner) {
      int cx = NS_FULL_TO_HALF_CORNER(corner, false);
      int cy = NS_FULL_TO_HALF_CORNER(corner, true);
      const nsCSSValue& radius = *aRuleData->ValueFor(subprops[corner]);
      nsStyleCoord parentX = parentOutline->mOutlineRadius.Get(cx);
      nsStyleCoord parentY = parentOutline->mOutlineRadius.Get(cy);
      nsStyleCoord coordX, coordY;

      if (SetPairCoords(radius, coordX, coordY, parentX, parentY,
                        SETCOORD_LPH | SETCOORD_INITIAL_ZERO |
                          SETCOORD_STORE_CALC,
                        aContext, mPresContext, canStoreInRuleTree)) {
        outline->mOutlineRadius.Set(cx, coordX);
        outline->mOutlineRadius.Set(cy, coordY);
      }
    }
  }

  
  
  const nsCSSValue* outlineStyleValue = aRuleData->ValueForOutlineStyle();
  nsCSSUnit unit = outlineStyleValue->GetUnit();
  NS_ABORT_IF_FALSE(eCSSUnit_None != unit && eCSSUnit_Auto != unit,
                    "'none' and 'auto' should be handled as enumerated values");
  if (eCSSUnit_Enumerated == unit) {
    outline->SetOutlineStyle(outlineStyleValue->GetIntValue());
  } else if (eCSSUnit_Initial == unit) {
    outline->SetOutlineStyle(NS_STYLE_BORDER_STYLE_NONE);
  } else if (eCSSUnit_Inherit == unit) {
    canStoreInRuleTree = PR_FALSE;
    outline->SetOutlineStyle(parentOutline->GetOutlineStyle());
  }

  outline->RecalcData(mPresContext);
  COMPUTE_END_RESET(Outline, outline)
}

const void*
nsRuleNode::ComputeListData(void* aStartStruct,
                            const nsRuleData* aRuleData,
                            nsStyleContext* aContext,
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(List, (), list, parentList)

  
  SetDiscrete(*aRuleData->ValueForListStyleType(),
              list->mListStyleType, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentList->mListStyleType,
              NS_STYLE_LIST_STYLE_DISC, 0, 0, 0, 0);

  
  const nsCSSValue* imageValue = aRuleData->ValueForListStyleImage();
  if (eCSSUnit_Image == imageValue->GetUnit()) {
    NS_SET_IMAGE_REQUEST(list->SetListStyleImage,
                         aContext,
                         imageValue->GetImageValue())
  }
  else if (eCSSUnit_None == imageValue->GetUnit() ||
           eCSSUnit_Initial == imageValue->GetUnit()) {
    list->SetListStyleImage(nsnull);
  }
  else if (eCSSUnit_Inherit == imageValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    NS_SET_IMAGE_REQUEST(list->SetListStyleImage,
                         aContext,
                         parentList->GetListStyleImage())
  }

  
  SetDiscrete(*aRuleData->ValueForListStylePosition(),
              list->mListStylePosition, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentList->mListStylePosition,
              NS_STYLE_LIST_STYLE_POSITION_OUTSIDE, 0, 0, 0, 0);

  
  const nsCSSValue* imageRegionValue = aRuleData->ValueForImageRegion();
  switch (imageRegionValue->GetUnit()) {
  case eCSSUnit_Inherit:
    canStoreInRuleTree = PR_FALSE;
    list->mImageRegion = parentList->mImageRegion;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Auto:
    list->mImageRegion.SetRect(0,0,0,0);
    break;

  case eCSSUnit_Null:
    break;

  case eCSSUnit_Rect: {
    const nsCSSRect& rgnRect = imageRegionValue->GetRectValue();

    if (rgnRect.mTop.GetUnit() == eCSSUnit_Auto)
      list->mImageRegion.y = 0;
    else if (rgnRect.mTop.IsLengthUnit())
      list->mImageRegion.y =
        CalcLength(rgnRect.mTop, aContext, mPresContext, canStoreInRuleTree);

    if (rgnRect.mBottom.GetUnit() == eCSSUnit_Auto)
      list->mImageRegion.height = 0;
    else if (rgnRect.mBottom.IsLengthUnit())
      list->mImageRegion.height =
        CalcLength(rgnRect.mBottom, aContext, mPresContext,
                   canStoreInRuleTree) - list->mImageRegion.y;

    if (rgnRect.mLeft.GetUnit() == eCSSUnit_Auto)
      list->mImageRegion.x = 0;
    else if (rgnRect.mLeft.IsLengthUnit())
      list->mImageRegion.x =
        CalcLength(rgnRect.mLeft, aContext, mPresContext, canStoreInRuleTree);

    if (rgnRect.mRight.GetUnit() == eCSSUnit_Auto)
      list->mImageRegion.width = 0;
    else if (rgnRect.mRight.IsLengthUnit())
      list->mImageRegion.width =
        CalcLength(rgnRect.mRight, aContext, mPresContext,
                   canStoreInRuleTree) - list->mImageRegion.x;
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false, "unrecognized image-region unit");
  }

  COMPUTE_END_INHERITED(List, list)
}

const void*
nsRuleNode::ComputePositionData(void* aStartStruct,
                                const nsRuleData* aRuleData,
                                nsStyleContext* aContext,
                                nsRuleNode* aHighestNode,
                                const RuleDetail aRuleDetail,
                                const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Position, (), pos, parentPos)

  
  static const nsCSSProperty offsetProps[] = {
    eCSSProperty_top,
    eCSSProperty_right,
    eCSSProperty_bottom,
    eCSSProperty_left
  };
  nsStyleCoord  coord;
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentPos->mOffset.Get(side);
    if (SetCoord(*aRuleData->ValueFor(offsetProps[side]),
                 coord, parentCoord,
                 SETCOORD_LPAH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC,
                 aContext, mPresContext, canStoreInRuleTree)) {
      pos->mOffset.Set(side, coord);
    }
  }

  SetCoord(*aRuleData->ValueForWidth(), pos->mWidth, parentPos->mWidth,
           SETCOORD_LPAEH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC,
           aContext, mPresContext, canStoreInRuleTree);
  SetCoord(*aRuleData->ValueForMinWidth(), pos->mMinWidth, parentPos->mMinWidth,
           SETCOORD_LPEH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC,
           aContext, mPresContext, canStoreInRuleTree);
  SetCoord(*aRuleData->ValueForMaxWidth(), pos->mMaxWidth, parentPos->mMaxWidth,
           SETCOORD_LPOEH | SETCOORD_INITIAL_NONE | SETCOORD_STORE_CALC,
           aContext, mPresContext, canStoreInRuleTree);

  SetCoord(*aRuleData->ValueForHeight(), pos->mHeight, parentPos->mHeight,
           SETCOORD_LPAH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC,
           aContext, mPresContext, canStoreInRuleTree);
  SetCoord(*aRuleData->ValueForMinHeight(), pos->mMinHeight, parentPos->mMinHeight,
           SETCOORD_LPH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC,
           aContext, mPresContext, canStoreInRuleTree);
  SetCoord(*aRuleData->ValueForMaxHeight(), pos->mMaxHeight, parentPos->mMaxHeight,
           SETCOORD_LPOH | SETCOORD_INITIAL_NONE | SETCOORD_STORE_CALC,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetDiscrete(*aRuleData->ValueForBoxSizing(),
              pos->mBoxSizing, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentPos->mBoxSizing,
              NS_STYLE_BOX_SIZING_CONTENT, 0, 0, 0, 0);

  
  const nsCSSValue* zIndexValue = aRuleData->ValueForZIndex();
  if (! SetCoord(*zIndexValue, pos->mZIndex, parentPos->mZIndex,
                 SETCOORD_IA | SETCOORD_INITIAL_AUTO, aContext,
                 nsnull, canStoreInRuleTree)) {
    if (eCSSUnit_Inherit == zIndexValue->GetUnit()) {
      
      canStoreInRuleTree = PR_FALSE;
      pos->mZIndex = parentPos->mZIndex;
    }
  }

  COMPUTE_END_RESET(Position, pos)
}

const void*
nsRuleNode::ComputeTableData(void* aStartStruct,
                             const nsRuleData* aRuleData,
                             nsStyleContext* aContext,
                             nsRuleNode* aHighestNode,
                             const RuleDetail aRuleDetail,
                             const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Table, (), table, parentTable)

  
  SetDiscrete(*aRuleData->ValueForTableLayout(),
              table->mLayoutStrategy, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentTable->mLayoutStrategy,
              NS_STYLE_TABLE_LAYOUT_AUTO, 0, 0, 0, 0);

  
  const nsCSSValue* colsValue = aRuleData->ValueForCols();
  if (eCSSUnit_Enumerated == colsValue->GetUnit() ||
      eCSSUnit_Integer == colsValue->GetUnit())
    table->mCols = colsValue->GetIntValue();

  
  const nsCSSValue* spanValue = aRuleData->ValueForSpan();
  if (eCSSUnit_Enumerated == spanValue->GetUnit() ||
      eCSSUnit_Integer == spanValue->GetUnit())
    table->mSpan = spanValue->GetIntValue();

  COMPUTE_END_RESET(Table, table)
}

const void*
nsRuleNode::ComputeTableBorderData(void* aStartStruct,
                                   const nsRuleData* aRuleData,
                                   nsStyleContext* aContext,
                                   nsRuleNode* aHighestNode,
                                   const RuleDetail aRuleDetail,
                                   const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(TableBorder, (mPresContext), table, parentTable)

  
  SetDiscrete(*aRuleData->ValueForBorderCollapse(), table->mBorderCollapse,
              canStoreInRuleTree,
              SETDSC_ENUMERATED, parentTable->mBorderCollapse,
              NS_STYLE_BORDER_SEPARATE, 0, 0, 0, 0);

  const nsCSSValue* borderSpacingValue = aRuleData->ValueForBorderSpacing();
  if (borderSpacingValue->GetUnit() != eCSSUnit_Null) {
    
    nsStyleCoord parentX(parentTable->mBorderSpacingX,
                         nsStyleCoord::CoordConstructor);
    nsStyleCoord parentY(parentTable->mBorderSpacingY,
                         nsStyleCoord::CoordConstructor);
    nsStyleCoord coordX, coordY;

#ifdef DEBUG
    PRBool result =
#endif
      SetPairCoords(*borderSpacingValue,
                    coordX, coordY, parentX, parentY,
                    SETCOORD_LH | SETCOORD_INITIAL_ZERO |
                    SETCOORD_CALC_LENGTH_ONLY |
                    SETCOORD_CALC_CLAMP_NONNEGATIVE,
                    aContext, mPresContext, canStoreInRuleTree);
    NS_ASSERTION(result, "malformed table border value");
    table->mBorderSpacingX = coordX.GetCoordValue();
    table->mBorderSpacingY = coordY.GetCoordValue();
  }

  
  SetDiscrete(*aRuleData->ValueForCaptionSide(),
              table->mCaptionSide, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentTable->mCaptionSide,
              NS_STYLE_CAPTION_SIDE_TOP, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForEmptyCells(),
              table->mEmptyCells, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentTable->mEmptyCells,
              (mPresContext->CompatibilityMode() == eCompatibility_NavQuirks)
              ? NS_STYLE_TABLE_EMPTY_CELLS_SHOW_BACKGROUND
              : NS_STYLE_TABLE_EMPTY_CELLS_SHOW,
              0, 0, 0, 0);

  COMPUTE_END_INHERITED(TableBorder, table)
}

const void*
nsRuleNode::ComputeContentData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  PRUint32 count;
  nsAutoString buffer;

  COMPUTE_START_RESET(Content, (), content, parentContent)

  
  const nsCSSValue* contentValue = aRuleData->ValueForContent();
  switch (contentValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Normal:
  case eCSSUnit_None:
  case eCSSUnit_Initial:
    
    content->AllocateContents(0);
    break;

  case eCSSUnit_Inherit:
    canStoreInRuleTree = PR_FALSE;
    count = parentContent->ContentCount();
    if (NS_SUCCEEDED(content->AllocateContents(count))) {
      while (0 < count--) {
        content->ContentAt(count) = parentContent->ContentAt(count);
      }
    }
    break;

  case eCSSUnit_Enumerated: {
    NS_ABORT_IF_FALSE(contentValue->GetIntValue() ==
                      NS_STYLE_CONTENT_ALT_CONTENT,
                      "unrecognized solitary content keyword");
    content->AllocateContents(1);
    nsStyleContentData& data = content->ContentAt(0);
    data.mType = eStyleContentType_AltContent;
    data.mContent.mString = nsnull;
    break;
  }

  case eCSSUnit_List:
  case eCSSUnit_ListDep: {
    const nsCSSValueList* contentValueList = contentValue->GetListValue();
      count = 0;
      while (contentValueList) {
        count++;
        contentValueList = contentValueList->mNext;
      }
      if (NS_SUCCEEDED(content->AllocateContents(count))) {
        const nsAutoString  nullStr;
        count = 0;
        contentValueList = contentValue->GetListValue();
        while (contentValueList) {
          const nsCSSValue& value = contentValueList->mValue;
          nsCSSUnit unit = value.GetUnit();
          nsStyleContentType type;
          nsStyleContentData &data = content->ContentAt(count++);
          switch (unit) {
          case eCSSUnit_String:   type = eStyleContentType_String;    break;
          case eCSSUnit_Image:    type = eStyleContentType_Image;     break;
          case eCSSUnit_Attr:     type = eStyleContentType_Attr;      break;
          case eCSSUnit_Counter:  type = eStyleContentType_Counter;   break;
          case eCSSUnit_Counters: type = eStyleContentType_Counters;  break;
          case eCSSUnit_Enumerated:
            switch (value.GetIntValue()) {
            case NS_STYLE_CONTENT_OPEN_QUOTE:
              type = eStyleContentType_OpenQuote;     break;
            case NS_STYLE_CONTENT_CLOSE_QUOTE:
              type = eStyleContentType_CloseQuote;    break;
            case NS_STYLE_CONTENT_NO_OPEN_QUOTE:
              type = eStyleContentType_NoOpenQuote;   break;
            case NS_STYLE_CONTENT_NO_CLOSE_QUOTE:
              type = eStyleContentType_NoCloseQuote;  break;
            default:
              NS_ERROR("bad content value");
            }
            break;
          default:
            NS_ERROR("bad content type");
          }
          data.mType = type;
          if (type == eStyleContentType_Image) {
            NS_SET_IMAGE_REQUEST(data.SetImage, aContext, value.GetImageValue());
          }
          else if (type <= eStyleContentType_Attr) {
            value.GetStringValue(buffer);
            data.mContent.mString = NS_strdup(buffer.get());
          }
          else if (type <= eStyleContentType_Counters) {
            data.mContent.mCounters = value.GetArrayValue();
            data.mContent.mCounters->AddRef();
          }
          else {
            data.mContent.mString = nsnull;
          }
          contentValueList = contentValueList->mNext;
        }
      }
      break;
  }

  default:
    NS_ABORT_IF_FALSE(false,
                      nsPrintfCString(64, "unrecognized content unit %d",
                                      contentValue->GetUnit()).get());
  }

  
  const nsCSSValue* counterIncrementValue =
    aRuleData->ValueForCounterIncrement();
  switch (counterIncrementValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_None:
  case eCSSUnit_Initial:
    content->AllocateCounterIncrements(0);
    break;

  case eCSSUnit_Inherit:
    canStoreInRuleTree = PR_FALSE;
    count = parentContent->CounterIncrementCount();
    if (NS_SUCCEEDED(content->AllocateCounterIncrements(count))) {
      while (0 < count--) {
        const nsStyleCounterData *data =
          parentContent->GetCounterIncrementAt(count);
        content->SetCounterIncrementAt(count, data->mCounter, data->mValue);
      }
    }
    break;

  case eCSSUnit_PairList:
  case eCSSUnit_PairListDep: {
    const nsCSSValuePairList* ourIncrement =
      counterIncrementValue->GetPairListValue();
    NS_ABORT_IF_FALSE(ourIncrement->mXValue.GetUnit() == eCSSUnit_Ident,
                      "unexpected value unit");
    count = ListLength(ourIncrement);
    if (NS_FAILED(content->AllocateCounterIncrements(count))) {
      break;
    }

    count = 0;
    for (const nsCSSValuePairList* p = ourIncrement; p; p = p->mNext, count++) {
      PRInt32 increment;
      if (p->mYValue.GetUnit() == eCSSUnit_Integer) {
        increment = p->mYValue.GetIntValue();
      } else {
        increment = 1;
      }
      p->mXValue.GetStringValue(buffer);
      content->SetCounterIncrementAt(count, buffer, increment);
    }
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false, "unexpected value unit");
  }

  
  const nsCSSValue* counterResetValue = aRuleData->ValueForCounterReset();
  switch (counterResetValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_None:
  case eCSSUnit_Initial:
    content->AllocateCounterResets(0);
    break;

  case eCSSUnit_Inherit:
    canStoreInRuleTree = PR_FALSE;
    count = parentContent->CounterResetCount();
    if (NS_SUCCEEDED(content->AllocateCounterResets(count))) {
      while (0 < count--) {
        const nsStyleCounterData *data =
          parentContent->GetCounterResetAt(count);
        content->SetCounterResetAt(count, data->mCounter, data->mValue);
      }
    }
    break;

  case eCSSUnit_PairList:
  case eCSSUnit_PairListDep: {
    const nsCSSValuePairList* ourReset =
      counterResetValue->GetPairListValue();
    NS_ABORT_IF_FALSE(ourReset->mXValue.GetUnit() == eCSSUnit_Ident,
                      "unexpected value unit");
    count = ListLength(ourReset);
    if (NS_FAILED(content->AllocateCounterResets(count))) {
      break;
    }

    count = 0;
    for (const nsCSSValuePairList* p = ourReset; p; p = p->mNext, count++) {
      PRInt32 reset;
      if (p->mYValue.GetUnit() == eCSSUnit_Integer) {
        reset = p->mYValue.GetIntValue();
      } else {
        reset = 0;
      }
      p->mXValue.GetStringValue(buffer);
      content->SetCounterResetAt(count, buffer, reset);
    }
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false, "unexpected value unit");
  }

  
  SetCoord(*aRuleData->ValueForMarkerOffset(), content->mMarkerOffset, parentContent->mMarkerOffset,
           SETCOORD_LH | SETCOORD_AUTO | SETCOORD_INITIAL_AUTO |
             SETCOORD_CALC_LENGTH_ONLY,
           aContext, mPresContext, canStoreInRuleTree);

  
  for (PRUint32 i = 0; i < content->ContentCount(); ++i) {
    if ((content->ContentAt(i).mType == eStyleContentType_Image) &&
        content->ContentAt(i).mContent.mImage) {
      content->ContentAt(i).TrackImage(aContext->PresContext());
    }
  }

  COMPUTE_END_RESET(Content, content)
}

const void*
nsRuleNode::ComputeQuotesData(void* aStartStruct,
                              const nsRuleData* aRuleData,
                              nsStyleContext* aContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Quotes, (), quotes, parentQuotes)

  
  const nsCSSValue* quotesValue = aRuleData->ValueForQuotes();
  switch (quotesValue->GetUnit()) {
  case eCSSUnit_Null:
    break;
  case eCSSUnit_Inherit:
    canStoreInRuleTree = PR_FALSE;
    quotes->CopyFrom(*parentQuotes);
    break;
  case eCSSUnit_Initial:
    quotes->SetInitial();
    break;
  case eCSSUnit_None:
    quotes->AllocateQuotes(0);
    break;
  case eCSSUnit_PairList:
  case eCSSUnit_PairListDep: {
    const nsCSSValuePairList* ourQuotes
      = quotesValue->GetPairListValue();
    nsAutoString buffer;
    nsAutoString closeBuffer;
    PRUint32 count = ListLength(ourQuotes);
    if (NS_FAILED(quotes->AllocateQuotes(count))) {
      break;
    }
    count = 0;
    while (ourQuotes) {
      NS_ABORT_IF_FALSE(ourQuotes->mXValue.GetUnit() == eCSSUnit_String &&
                        ourQuotes->mYValue.GetUnit() == eCSSUnit_String,
                        "improper list contents for quotes");
      ourQuotes->mXValue.GetStringValue(buffer);
      ourQuotes->mYValue.GetStringValue(closeBuffer);
      quotes->SetQuotesAt(count++, buffer, closeBuffer);
      ourQuotes = ourQuotes->mNext;
    }
    break;
  }
  default:
    NS_ABORT_IF_FALSE(false, "unexpected value unit");
  }

  COMPUTE_END_INHERITED(Quotes, quotes)
}

const void*
nsRuleNode::ComputeXULData(void* aStartStruct,
                           const nsRuleData* aRuleData,
                           nsStyleContext* aContext,
                           nsRuleNode* aHighestNode,
                           const RuleDetail aRuleDetail,
                           const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(XUL, (), xul, parentXUL)

  
  SetDiscrete(*aRuleData->ValueForBoxAlign(),
              xul->mBoxAlign, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxAlign,
              NS_STYLE_BOX_ALIGN_STRETCH, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForBoxDirection(),
              xul->mBoxDirection, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxDirection,
              NS_STYLE_BOX_DIRECTION_NORMAL, 0, 0, 0, 0);

  
  SetFactor(*aRuleData->ValueForBoxFlex(),
            xul->mBoxFlex, canStoreInRuleTree,
            parentXUL->mBoxFlex, 0.0f);

  
  SetDiscrete(*aRuleData->ValueForBoxOrient(),
              xul->mBoxOrient, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxOrient,
              NS_STYLE_BOX_ORIENT_HORIZONTAL, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForBoxPack(),
              xul->mBoxPack, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxPack,
              NS_STYLE_BOX_PACK_START, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForBoxOrdinalGroup(),
              xul->mBoxOrdinal, canStoreInRuleTree,
              SETDSC_INTEGER, parentXUL->mBoxOrdinal, 1,
              0, 0, 0, 0);

  const nsCSSValue* stackSizingValue = aRuleData->ValueForStackSizing();
  if (eCSSUnit_Inherit == stackSizingValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    xul->mStretchStack = parentXUL->mStretchStack;
  } else if (eCSSUnit_Initial == stackSizingValue->GetUnit()) {
    xul->mStretchStack = PR_TRUE;
  } else if (eCSSUnit_Enumerated == stackSizingValue->GetUnit()) {
    xul->mStretchStack = stackSizingValue->GetIntValue() ==
      NS_STYLE_STACK_SIZING_STRETCH_TO_FIT;
  }

  COMPUTE_END_RESET(XUL, xul)
}

const void*
nsRuleNode::ComputeColumnData(void* aStartStruct,
                              const nsRuleData* aRuleData,
                              nsStyleContext* aContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Column, (mPresContext), column, parent)

  
  SetCoord(*aRuleData->ValueForColumnWidth(),
           column->mColumnWidth, parent->mColumnWidth,
           SETCOORD_LAH | SETCOORD_INITIAL_AUTO |
           SETCOORD_CALC_LENGTH_ONLY | SETCOORD_CALC_CLAMP_NONNEGATIVE,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetCoord(*aRuleData->ValueForColumnGap(),
           column->mColumnGap, parent->mColumnGap,
           SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL |
           SETCOORD_CALC_LENGTH_ONLY,
           aContext, mPresContext, canStoreInRuleTree);
  
  if (column->mColumnGap.GetUnit() == eStyleUnit_Coord) {
    column->mColumnGap.SetCoordValue(
      NS_MAX(column->mColumnGap.GetCoordValue(), 0));
  }

  
  const nsCSSValue* columnCountValue = aRuleData->ValueForColumnCount();
  if (eCSSUnit_Auto == columnCountValue->GetUnit() ||
      eCSSUnit_Initial == columnCountValue->GetUnit()) {
    column->mColumnCount = NS_STYLE_COLUMN_COUNT_AUTO;
  } else if (eCSSUnit_Integer == columnCountValue->GetUnit()) {
    column->mColumnCount = columnCountValue->GetIntValue();
    
    column->mColumnCount = NS_MIN(column->mColumnCount, 1000U);
  } else if (eCSSUnit_Inherit == columnCountValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    column->mColumnCount = parent->mColumnCount;
  }

  
  const nsCSSValue& widthValue = *aRuleData->ValueForColumnRuleWidth();
  if (eCSSUnit_Initial == widthValue.GetUnit()) {
    column->SetColumnRuleWidth(
        (mPresContext->GetBorderWidthTable())[NS_STYLE_BORDER_WIDTH_MEDIUM]);
  }
  else if (eCSSUnit_Enumerated == widthValue.GetUnit()) {
    NS_ASSERTION(widthValue.GetIntValue() == NS_STYLE_BORDER_WIDTH_THIN ||
                 widthValue.GetIntValue() == NS_STYLE_BORDER_WIDTH_MEDIUM ||
                 widthValue.GetIntValue() == NS_STYLE_BORDER_WIDTH_THICK,
                 "Unexpected enum value");
    column->SetColumnRuleWidth(
        (mPresContext->GetBorderWidthTable())[widthValue.GetIntValue()]);
  }
  else if (eCSSUnit_Inherit == widthValue.GetUnit()) {
    column->SetColumnRuleWidth(parent->GetComputedColumnRuleWidth());
    canStoreInRuleTree = PR_FALSE;
  }
  else if (widthValue.IsLengthUnit() || widthValue.IsCalcUnit()) {
    nscoord len =
      CalcLength(widthValue, aContext, mPresContext, canStoreInRuleTree);
    if (len < 0) {
      
      
      
      NS_ASSERTION(widthValue.IsCalcUnit(),
                   "parser should have rejected negative length");
      len = 0;
    }
    column->SetColumnRuleWidth(len);
  }

  
  const nsCSSValue& styleValue = *aRuleData->ValueForColumnRuleStyle();
  NS_ABORT_IF_FALSE(eCSSUnit_None != styleValue.GetUnit(),
                    "'none' should be handled as enumerated value");
  if (eCSSUnit_Enumerated == styleValue.GetUnit()) {
    column->mColumnRuleStyle = styleValue.GetIntValue();
  }
  else if (eCSSUnit_Initial == styleValue.GetUnit()) {
    column->mColumnRuleStyle = NS_STYLE_BORDER_STYLE_NONE;
  }
  else if (eCSSUnit_Inherit == styleValue.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    column->mColumnRuleStyle = parent->mColumnRuleStyle;
  }

  
  const nsCSSValue& colorValue = *aRuleData->ValueForColumnRuleColor();
  if (eCSSUnit_Inherit == colorValue.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    column->mColumnRuleColorIsForeground = PR_FALSE;
    if (parent->mColumnRuleColorIsForeground) {
      column->mColumnRuleColor = parentContext->GetStyleColor()->mColor;
    } else {
      column->mColumnRuleColor = parent->mColumnRuleColor;
    }
  }
  else if (eCSSUnit_Initial == colorValue.GetUnit() ||
           eCSSUnit_Enumerated == colorValue.GetUnit()) {
    column->mColumnRuleColorIsForeground = PR_TRUE;
  }
  else if (SetColor(colorValue, 0, mPresContext, aContext,
                    column->mColumnRuleColor, canStoreInRuleTree)) {
    column->mColumnRuleColorIsForeground = PR_FALSE;
  }

  COMPUTE_END_RESET(Column, column)
}

static void
SetSVGPaint(const nsCSSValue& aValue, const nsStyleSVGPaint& parentPaint,
            nsPresContext* aPresContext, nsStyleContext *aContext,
            nsStyleSVGPaint& aResult, nsStyleSVGPaintType aInitialPaintType,
            PRBool& aCanStoreInRuleTree)
{
  nscolor color;

  if (aValue.GetUnit() == eCSSUnit_Inherit) {
    aResult = parentPaint;
    aCanStoreInRuleTree = PR_FALSE;
  } else if (aValue.GetUnit() == eCSSUnit_None) {
    aResult.SetType(eStyleSVGPaintType_None);
  } else if (aValue.GetUnit() == eCSSUnit_Initial) {
    aResult.SetType(aInitialPaintType);
    aResult.mPaint.mColor = NS_RGB(0, 0, 0);
    aResult.mFallbackColor = NS_RGB(0, 0, 0);
  } else if (SetColor(aValue, NS_RGB(0, 0, 0), aPresContext, aContext,
                      color, aCanStoreInRuleTree)) {
    aResult.SetType(eStyleSVGPaintType_Color);
    aResult.mPaint.mColor = color;
  } else if (aValue.GetUnit() == eCSSUnit_Pair) {
    const nsCSSValuePair& pair = aValue.GetPairValue();
    NS_ABORT_IF_FALSE(pair.mXValue.GetUnit() == eCSSUnit_URL,
                      "malformed paint server value");

    aResult.SetType(eStyleSVGPaintType_Server);
    aResult.mPaint.mPaintServer = pair.mXValue.GetURLValue();
    NS_IF_ADDREF(aResult.mPaint.mPaintServer);

    if (pair.mYValue.GetUnit() == eCSSUnit_None) {
      aResult.mFallbackColor = NS_RGBA(0, 0, 0, 0);
    } else {
      NS_ABORT_IF_FALSE(pair.mYValue.GetUnit() != eCSSUnit_Inherit,
                        "cannot inherit fallback colour");
      SetColor(pair.mYValue, NS_RGB(0, 0, 0), aPresContext, aContext,
               aResult.mFallbackColor, aCanStoreInRuleTree);
    }
  } else {
    NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Null,
                      "malformed paint server value");
  }
}

const void*
nsRuleNode::ComputeSVGData(void* aStartStruct,
                           const nsRuleData* aRuleData,
                           nsStyleContext* aContext,
                           nsRuleNode* aHighestNode,
                           const RuleDetail aRuleDetail,
                           const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(SVG, (), svg, parentSVG)

  
  SetDiscrete(*aRuleData->ValueForClipRule(),
              svg->mClipRule, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mClipRule,
              NS_STYLE_FILL_RULE_NONZERO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForColorInterpolation(),
              svg->mColorInterpolation, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mColorInterpolation,
              NS_STYLE_COLOR_INTERPOLATION_SRGB, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForColorInterpolationFilters(),
              svg->mColorInterpolationFilters, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mColorInterpolationFilters,
              NS_STYLE_COLOR_INTERPOLATION_LINEARRGB, 0, 0, 0, 0);

  
  SetSVGPaint(*aRuleData->ValueForFill(),
              parentSVG->mFill, mPresContext, aContext,
              svg->mFill, eStyleSVGPaintType_Color, canStoreInRuleTree);

  
  SetFactor(*aRuleData->ValueForFillOpacity(),
            svg->mFillOpacity, canStoreInRuleTree,
            parentSVG->mFillOpacity, 1.0f, SETFCT_OPACITY);

  
  SetDiscrete(*aRuleData->ValueForFillRule(),
              svg->mFillRule, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mFillRule,
              NS_STYLE_FILL_RULE_NONZERO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForImageRendering(),
              svg->mImageRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mImageRendering,
              NS_STYLE_IMAGE_RENDERING_AUTO, 0, 0, 0, 0);

  
  const nsCSSValue* markerEndValue = aRuleData->ValueForMarkerEnd();
  if (eCSSUnit_URL == markerEndValue->GetUnit()) {
    svg->mMarkerEnd = markerEndValue->GetURLValue();
  } else if (eCSSUnit_None == markerEndValue->GetUnit() ||
             eCSSUnit_Initial == markerEndValue->GetUnit()) {
    svg->mMarkerEnd = nsnull;
  } else if (eCSSUnit_Inherit == markerEndValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svg->mMarkerEnd = parentSVG->mMarkerEnd;
  }

  
  const nsCSSValue* markerMidValue = aRuleData->ValueForMarkerMid();
  if (eCSSUnit_URL == markerMidValue->GetUnit()) {
    svg->mMarkerMid = markerMidValue->GetURLValue();
  } else if (eCSSUnit_None == markerMidValue->GetUnit() ||
             eCSSUnit_Initial == markerMidValue->GetUnit()) {
    svg->mMarkerMid = nsnull;
  } else if (eCSSUnit_Inherit == markerMidValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svg->mMarkerMid = parentSVG->mMarkerMid;
  }

  
  const nsCSSValue* markerStartValue = aRuleData->ValueForMarkerStart();
  if (eCSSUnit_URL == markerStartValue->GetUnit()) {
    svg->mMarkerStart = markerStartValue->GetURLValue();
  } else if (eCSSUnit_None == markerStartValue->GetUnit() ||
             eCSSUnit_Initial == markerStartValue->GetUnit()) {
    svg->mMarkerStart = nsnull;
  } else if (eCSSUnit_Inherit == markerStartValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svg->mMarkerStart = parentSVG->mMarkerStart;
  }

  
  SetDiscrete(*aRuleData->ValueForShapeRendering(),
              svg->mShapeRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mShapeRendering,
              NS_STYLE_SHAPE_RENDERING_AUTO, 0, 0, 0, 0);

  
  SetSVGPaint(*aRuleData->ValueForStroke(),
              parentSVG->mStroke, mPresContext, aContext,
              svg->mStroke, eStyleSVGPaintType_None, canStoreInRuleTree);

  
  const nsCSSValue* strokeDasharrayValue = aRuleData->ValueForStrokeDasharray();
  switch (strokeDasharrayValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    canStoreInRuleTree = PR_FALSE;
    
    
    if (!svg->mStrokeDasharray) {
      svg->mStrokeDasharrayLength = parentSVG->mStrokeDasharrayLength;
      if (svg->mStrokeDasharrayLength) {
        svg->mStrokeDasharray = new nsStyleCoord[svg->mStrokeDasharrayLength];
        if (svg->mStrokeDasharray)
          memcpy(svg->mStrokeDasharray,
                 parentSVG->mStrokeDasharray,
                 svg->mStrokeDasharrayLength * sizeof(nsStyleCoord));
        else
          svg->mStrokeDasharrayLength = 0;
      }
    }
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_None:
    delete [] svg->mStrokeDasharray;
    svg->mStrokeDasharray = nsnull;
    svg->mStrokeDasharrayLength = 0;
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep: {
    delete [] svg->mStrokeDasharray;
    svg->mStrokeDasharray = nsnull;
    svg->mStrokeDasharrayLength = 0;

    
    const nsCSSValueList *value = strokeDasharrayValue->GetListValue();
    svg->mStrokeDasharrayLength = ListLength(value);

    NS_ASSERTION(svg->mStrokeDasharrayLength != 0, "no dasharray items");

    svg->mStrokeDasharray = new nsStyleCoord[svg->mStrokeDasharrayLength];

    if (svg->mStrokeDasharray) {
      PRUint32 i = 0;
      while (nsnull != value) {
        SetCoord(value->mValue,
                 svg->mStrokeDasharray[i++], nsStyleCoord(),
                 SETCOORD_LP | SETCOORD_FACTOR,
                 aContext, mPresContext, canStoreInRuleTree);
        value = value->mNext;
      }
    } else {
      svg->mStrokeDasharrayLength = 0;
    }
    break;
  }

  default:
    NS_ABORT_IF_FALSE(false, "unrecognized dasharray unit");
  }

  
  SetCoord(*aRuleData->ValueForStrokeDashoffset(),
           svg->mStrokeDashoffset, parentSVG->mStrokeDashoffset,
           SETCOORD_LPH | SETCOORD_FACTOR | SETCOORD_INITIAL_ZERO,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetDiscrete(*aRuleData->ValueForStrokeLinecap(),
              svg->mStrokeLinecap, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mStrokeLinecap,
              NS_STYLE_STROKE_LINECAP_BUTT, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForStrokeLinejoin(),
              svg->mStrokeLinejoin, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mStrokeLinejoin,
              NS_STYLE_STROKE_LINEJOIN_MITER, 0, 0, 0, 0);

  
  SetFactor(*aRuleData->ValueForStrokeMiterlimit(),
            svg->mStrokeMiterlimit,
            canStoreInRuleTree,
            parentSVG->mStrokeMiterlimit, 4.0f);

  
  SetFactor(*aRuleData->ValueForStrokeOpacity(),
            svg->mStrokeOpacity, canStoreInRuleTree,
            parentSVG->mStrokeOpacity, 1.0f, SETFCT_OPACITY);

  
  const nsCSSValue* strokeWidthValue = aRuleData->ValueForStrokeWidth();
  if (eCSSUnit_Initial == strokeWidthValue->GetUnit()) {
    svg->mStrokeWidth.SetCoordValue(nsPresContext::CSSPixelsToAppUnits(1));
  } else {
    SetCoord(*strokeWidthValue,
             svg->mStrokeWidth, parentSVG->mStrokeWidth,
             SETCOORD_LPH | SETCOORD_FACTOR,
             aContext, mPresContext, canStoreInRuleTree);
  }

  
  SetDiscrete(*aRuleData->ValueForTextAnchor(),
              svg->mTextAnchor, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mTextAnchor,
              NS_STYLE_TEXT_ANCHOR_START, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForTextRendering(),
              svg->mTextRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mTextRendering,
              NS_STYLE_TEXT_RENDERING_AUTO, 0, 0, 0, 0);

  COMPUTE_END_INHERITED(SVG, svg)
}

const void*
nsRuleNode::ComputeSVGResetData(void* aStartStruct,
                                const nsRuleData* aRuleData,
                                nsStyleContext* aContext,
                                nsRuleNode* aHighestNode,
                                const RuleDetail aRuleDetail,
                                const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(SVGReset, (), svgReset, parentSVGReset)

  
  const nsCSSValue* stopColorValue = aRuleData->ValueForStopColor();
  if (eCSSUnit_Initial == stopColorValue->GetUnit()) {
    svgReset->mStopColor = NS_RGB(0, 0, 0);
  } else {
    SetColor(*stopColorValue, parentSVGReset->mStopColor,
             mPresContext, aContext, svgReset->mStopColor, canStoreInRuleTree);
  }

  
  const nsCSSValue* floodColorValue = aRuleData->ValueForFloodColor();
  if (eCSSUnit_Initial == floodColorValue->GetUnit()) {
    svgReset->mFloodColor = NS_RGB(0, 0, 0);
  } else {
    SetColor(*floodColorValue, parentSVGReset->mFloodColor,
             mPresContext, aContext, svgReset->mFloodColor, canStoreInRuleTree);
  }

  
  const nsCSSValue* lightingColorValue = aRuleData->ValueForLightingColor();
  if (eCSSUnit_Initial == lightingColorValue->GetUnit()) {
    svgReset->mLightingColor = NS_RGB(255, 255, 255);
  } else {
    SetColor(*lightingColorValue, parentSVGReset->mLightingColor,
             mPresContext, aContext, svgReset->mLightingColor,
             canStoreInRuleTree);
  }

  
  const nsCSSValue* clipPathValue = aRuleData->ValueForClipPath();
  if (eCSSUnit_URL == clipPathValue->GetUnit()) {
    svgReset->mClipPath = clipPathValue->GetURLValue();
  } else if (eCSSUnit_None == clipPathValue->GetUnit() ||
             eCSSUnit_Initial == clipPathValue->GetUnit()) {
    svgReset->mClipPath = nsnull;
  } else if (eCSSUnit_Inherit == clipPathValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svgReset->mClipPath = parentSVGReset->mClipPath;
  }

  
  SetFactor(*aRuleData->ValueForStopOpacity(),
            svgReset->mStopOpacity, canStoreInRuleTree,
            parentSVGReset->mStopOpacity, 1.0f, SETFCT_OPACITY);

  
  SetFactor(*aRuleData->ValueForFloodOpacity(),
            svgReset->mFloodOpacity, canStoreInRuleTree,
            parentSVGReset->mFloodOpacity, 1.0f, SETFCT_OPACITY);

  
  SetDiscrete(*aRuleData->ValueForDominantBaseline(),
              svgReset->mDominantBaseline,
              canStoreInRuleTree, SETDSC_ENUMERATED,
              parentSVGReset->mDominantBaseline,
              NS_STYLE_DOMINANT_BASELINE_AUTO, 0, 0, 0, 0);

  
  const nsCSSValue* filterValue = aRuleData->ValueForFilter();
  if (eCSSUnit_URL == filterValue->GetUnit()) {
    svgReset->mFilter = filterValue->GetURLValue();
  } else if (eCSSUnit_None == filterValue->GetUnit() ||
             eCSSUnit_Initial == filterValue->GetUnit()) {
    svgReset->mFilter = nsnull;
  } else if (eCSSUnit_Inherit == filterValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svgReset->mFilter = parentSVGReset->mFilter;
  }

  
  const nsCSSValue* maskValue = aRuleData->ValueForMask();
  if (eCSSUnit_URL == maskValue->GetUnit()) {
    svgReset->mMask = maskValue->GetURLValue();
  } else if (eCSSUnit_None == maskValue->GetUnit() ||
             eCSSUnit_Initial == maskValue->GetUnit()) {
    svgReset->mMask = nsnull;
  } else if (eCSSUnit_Inherit == maskValue->GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svgReset->mMask = parentSVGReset->mMask;
  }

  COMPUTE_END_RESET(SVGReset, svgReset)
}

inline const void*
nsRuleNode::GetParentData(const nsStyleStructID aSID)
{
  NS_PRECONDITION(mDependentBits & nsCachedStyleData::GetBitForSID(aSID),
                  "should be called when node depends on parent data");
  NS_ASSERTION(mStyleData.GetStyleData(aSID) == nsnull,
               "both struct and dependent bits present");
  
  
  PRUint32 bit = nsCachedStyleData::GetBitForSID(aSID);
  nsRuleNode *ruleNode = mParent;
  while (ruleNode->mDependentBits & bit) {
    NS_ASSERTION(ruleNode->mStyleData.GetStyleData(aSID) == nsnull,
                 "both struct and dependent bits present");
    ruleNode = ruleNode->mParent;
  }

  return ruleNode->mStyleData.GetStyleData(aSID);
}

#define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
inline const nsStyle##name_ *                                               \
nsRuleNode::GetParent##name_()                                              \
{                                                                           \
  NS_PRECONDITION(mDependentBits &                                          \
                  nsCachedStyleData::GetBitForSID(eStyleStruct_##name_),    \
                  "should be called when node depends on parent data");     \
  NS_ASSERTION(mStyleData.GetStyle##name_() == nsnull,                      \
               "both struct and dependent bits present");                   \
  /* Walk up the rule tree from this rule node (towards less specific */    \
  /* rules). */                                                             \
  PRUint32 bit = nsCachedStyleData::GetBitForSID(eStyleStruct_##name_);     \
  nsRuleNode *ruleNode = mParent;                                           \
  while (ruleNode->mDependentBits & bit) {                                  \
    NS_ASSERTION(ruleNode->mStyleData.GetStyle##name_() == nsnull,          \
                 "both struct and dependent bits present");                 \
    ruleNode = ruleNode->mParent;                                           \
  }                                                                         \
                                                                            \
  return ruleNode->mStyleData.GetStyle##name_();                            \
}
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

const void*
nsRuleNode::GetStyleData(nsStyleStructID aSID,
                         nsStyleContext* aContext,
                         PRBool aComputeData)
{
  const void *data;
  if (mDependentBits & nsCachedStyleData::GetBitForSID(aSID)) {
    
    
    
    data = GetParentData(aSID);
    NS_ASSERTION(data, "dependent bits set but no cached struct present");
    return data;
  }

  data = mStyleData.GetStyleData(aSID);
  if (NS_LIKELY(data != nsnull))
    return data; 

  if (NS_UNLIKELY(!aComputeData))
    return nsnull;

  
  data = WalkRuleTree(aSID, aContext);

  if (NS_LIKELY(data != nsnull))
    return data;

  NS_NOTREACHED("could not create style struct");
  
  
  
  
  
  
  return mPresContext->PresShell()->StyleSet()->
    DefaultStyleData()->GetStyleData(aSID);
}



#define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                        \
const nsStyle##name_*                                                         \
nsRuleNode::GetStyle##name_(nsStyleContext* aContext, PRBool aComputeData)    \
{                                                                             \
  const nsStyle##name_ *data;                                                 \
  if (mDependentBits &                                                        \
      nsCachedStyleData::GetBitForSID(eStyleStruct_##name_)) {                \
    data = GetParent##name_();                                                \
    NS_ASSERTION(data, "dependent bits set but no cached struct present");    \
    return data;                                                              \
  }                                                                           \
                                                                              \
  data = mStyleData.GetStyle##name_();                                        \
  if (NS_LIKELY(data != nsnull))                                              \
    return data;                                                              \
                                                                              \
  if (NS_UNLIKELY(!aComputeData))                                             \
    return nsnull;                                                            \
                                                                              \
  data = static_cast<const nsStyle##name_ *>                                  \
           (WalkRuleTree(eStyleStruct_##name_, aContext));                    \
                                                                              \
  if (NS_LIKELY(data != nsnull))                                              \
    return data;                                                              \
                                                                              \
  NS_NOTREACHED("could not create style struct");                             \
  return                                                                      \
    static_cast<const nsStyle##name_ *>(                                      \
                   mPresContext->PresShell()->StyleSet()->                    \
                     DefaultStyleData()->GetStyleData(eStyleStruct_##name_)); \
}
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

void
nsRuleNode::Mark()
{
  for (nsRuleNode *node = this;
       node && !(node->mDependentBits & NS_RULE_NODE_GC_MARK);
       node = node->mParent)
    node->mDependentBits |= NS_RULE_NODE_GC_MARK;
}

static PLDHashOperator
SweepRuleNodeChildren(PLDHashTable *table, PLDHashEntryHdr *hdr,
                      PRUint32 number, void *arg)
{
  ChildrenHashEntry *entry = static_cast<ChildrenHashEntry*>(hdr);
  if (entry->mRuleNode->Sweep())
    return PL_DHASH_REMOVE; 
  return PL_DHASH_NEXT;
}

PRBool
nsRuleNode::Sweep()
{
  
  
  
  
  if (!(mDependentBits & NS_RULE_NODE_GC_MARK) &&
      
      !(IsRoot() && mPresContext->StyleSet()->GetRuleTree() == this)) {
    Destroy();
    return PR_TRUE;
  }

  
  mDependentBits &= ~NS_RULE_NODE_GC_MARK;

  
  
  if (HaveChildren()) {
    PRUint32 childrenDestroyed;
    if (ChildrenAreHashed()) {
      PLDHashTable *children = ChildrenHash();
      PRUint32 oldChildCount = children->entryCount;
      PL_DHashTableEnumerate(children, SweepRuleNodeChildren, nsnull);
      childrenDestroyed = children->entryCount - oldChildCount;
    } else {
      childrenDestroyed = 0;
      for (nsRuleNode **children = ChildrenListPtr(); *children; ) {
        nsRuleNode *next = (*children)->mNextSibling;
        if ((*children)->Sweep()) {
          
          
          *children = next;
          ++childrenDestroyed;
        } else {
          
          children = &(*children)->mNextSibling;
        }
      }
    }
    mRefCnt -= childrenDestroyed;
    NS_POSTCONDITION(IsRoot() || mRefCnt > 0,
                     "We didn't get swept, so we'd better have style contexts "
                     "pointing to us or to one of our descendants, which means "
                     "we'd better have a nonzero mRefCnt here!");
  }
  return PR_FALSE;
}

 PRBool
nsRuleNode::HasAuthorSpecifiedRules(nsStyleContext* aStyleContext,
                                    PRUint32 ruleTypeMask,
                                    PRBool aAuthorColorsAllowed)
{
  PRUint32 inheritBits = 0;
  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND)
    inheritBits |= NS_STYLE_INHERIT_BIT(Background);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER)
    inheritBits |= NS_STYLE_INHERIT_BIT(Border);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING)
    inheritBits |= NS_STYLE_INHERIT_BIT(Padding);

  
  size_t nprops = 0, backgroundOffset, borderOffset, paddingOffset;

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND) {
    backgroundOffset = nprops;
    nprops += nsCSSProps::PropertyCountInStruct(eStyleStruct_Background);
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER) {
    borderOffset = nprops;
    nprops += nsCSSProps::PropertyCountInStruct(eStyleStruct_Border);
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING) {
    paddingOffset = nprops;
    nprops += nsCSSProps::PropertyCountInStruct(eStyleStruct_Padding);
  }

  void* dataStorage = alloca(nprops * sizeof(nsCSSValue));
  AutoCSSValueArray dataArray(dataStorage, nprops);

  
  nsRuleData ruleData(inheritBits, dataArray.get(),
                      aStyleContext->PresContext(), aStyleContext);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND) {
    ruleData.mValueOffsets[eStyleStruct_Background] = backgroundOffset;
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER) {
    ruleData.mValueOffsets[eStyleStruct_Border] = borderOffset;
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING) {
    ruleData.mValueOffsets[eStyleStruct_Padding] = paddingOffset;
  }

  static const nsCSSProperty backgroundValues[] = {
    eCSSProperty_background_color,
    eCSSProperty_background_image,
  };

  static const nsCSSProperty borderValues[] = {
    eCSSProperty_border_top_color,
    eCSSProperty_border_top_style,
    eCSSProperty_border_top_width,
    eCSSProperty_border_right_color_value,
    eCSSProperty_border_right_style_value,
    eCSSProperty_border_right_width_value,
    eCSSProperty_border_bottom_color,
    eCSSProperty_border_bottom_style,
    eCSSProperty_border_bottom_width,
    eCSSProperty_border_left_color_value,
    eCSSProperty_border_left_style_value,
    eCSSProperty_border_left_width_value,
    eCSSProperty_border_start_color_value,
    eCSSProperty_border_start_style_value,
    eCSSProperty_border_start_width_value,
    eCSSProperty_border_end_color_value,
    eCSSProperty_border_end_style_value,
    eCSSProperty_border_end_width_value,
    eCSSProperty_border_top_left_radius,
    eCSSProperty_border_top_right_radius,
    eCSSProperty_border_bottom_right_radius,
    eCSSProperty_border_bottom_left_radius,
  };

  static const nsCSSProperty paddingValues[] = {
    eCSSProperty_padding_top,
    eCSSProperty_padding_right_value,
    eCSSProperty_padding_bottom,
    eCSSProperty_padding_left_value,
    eCSSProperty_padding_start_value,
    eCSSProperty_padding_end_value,
  };

  
  size_t nValues = 0;

  size_t backColorIndex = size_t(-1);

  nsCSSValue* values[NS_ARRAY_LENGTH(backgroundValues) +
                     NS_ARRAY_LENGTH(borderValues) +
                     NS_ARRAY_LENGTH(paddingValues)];

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND) {
    for (PRUint32 i = 0, i_end = NS_ARRAY_LENGTH(backgroundValues);
         i < i_end; ++i) {
      if (backgroundValues[i] == eCSSProperty_background_color) {
        backColorIndex = nValues;
      }
      values[nValues++] = ruleData.ValueFor(backgroundValues[i]);
    }
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER) {
    for (PRUint32 i = 0, i_end = NS_ARRAY_LENGTH(borderValues);
         i < i_end; ++i) {
      values[nValues++] = ruleData.ValueFor(borderValues[i]);
    }
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING) {
    for (PRUint32 i = 0, i_end = NS_ARRAY_LENGTH(paddingValues);
         i < i_end; ++i) {
      values[nValues++] = ruleData.ValueFor(paddingValues[i]);
    }
  }

  nsStyleContext* styleContext = aStyleContext;

  
  
  
  
  
  
  PRBool haveExplicitUAInherit;
  do {
    haveExplicitUAInherit = PR_FALSE;
    for (nsRuleNode* ruleNode = styleContext->GetRuleNode(); ruleNode;
         ruleNode = ruleNode->GetParent()) {
      nsIStyleRule *rule = ruleNode->GetRule();
      if (rule) {
        ruleData.mLevel = ruleNode->GetLevel();
        ruleData.mIsImportantRule = ruleNode->IsImportantRule();

        rule->MapRuleInfoInto(&ruleData);

        if (ruleData.mLevel == nsStyleSet::eAgentSheet ||
            ruleData.mLevel == nsStyleSet::eUserSheet) {
          
          
          
          for (PRUint32 i = 0; i < nValues; ++i) {
            nsCSSUnit unit = values[i]->GetUnit();
            if (unit != eCSSUnit_Null &&
                unit != eCSSUnit_Dummy &&
                unit != eCSSUnit_DummyInherit) {
              if (unit == eCSSUnit_Inherit) {
                haveExplicitUAInherit = PR_TRUE;
                values[i]->SetDummyInheritValue();
              } else {
                values[i]->SetDummyValue();
              }
            }
          }
        } else {
          
          
          for (PRUint32 i = 0; i < nValues; ++i) {
            if (values[i]->GetUnit() != eCSSUnit_Null &&
                values[i]->GetUnit() != eCSSUnit_Dummy && 
                values[i]->GetUnit() != eCSSUnit_DummyInherit) {
              
              
              
              
              if (aAuthorColorsAllowed ||
                  (i == backColorIndex &&
                   !values[i]->IsNonTransparentColor())) {
                return PR_TRUE;
              }

              values[i]->SetDummyValue();
            }
          }
        }
      }
    }

    if (haveExplicitUAInherit) {
      
      
      
      
      
      for (PRUint32 i = 0; i < nValues; ++i)
        if (values[i]->GetUnit() == eCSSUnit_Null)
          values[i]->SetDummyValue();
      for (PRUint32 i = 0; i < nValues; ++i)
        if (values[i]->GetUnit() == eCSSUnit_DummyInherit)
          values[i]->Reset();
      styleContext = styleContext->GetParent();
    }
  } while (haveExplicitUAInherit && styleContext);

  return PR_FALSE;
}
