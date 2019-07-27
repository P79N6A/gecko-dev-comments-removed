










#include <algorithm>

#include "mozilla/ArrayUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Likely.h"
#include "mozilla/LookAndFeel.h"

#include "nsDeviceContext.h"
#include "nsRuleNode.h"
#include "nscore.h"
#include "nsIWidget.h"
#include "nsIPresShell.h"
#include "nsFontMetrics.h"
#include "gfxFont.h"
#include "nsCSSPseudoElements.h"
#include "nsThemeConstants.h"
#include "pldhash.h"
#include "nsStyleContext.h"
#include "nsStyleSet.h"
#include "nsStyleStruct.h"
#include "nsSize.h"
#include "nsRuleData.h"
#include "nsIStyleRule.h"
#include "nsBidiUtils.h"
#include "nsStyleStructInlines.h"
#include "nsCSSProps.h"
#include "nsTArray.h"
#include "nsContentUtils.h"
#include "CSSCalc.h"
#include "nsPrintfCString.h"
#include "nsRenderingContext.h"
#include "nsStyleUtil.h"
#include "nsIDocument.h"
#include "prtime.h"
#include "CSSVariableResolver.h"
#include "nsCSSParser.h"
#include "CounterStyleManager.h"
#include "nsCSSPropertySet.h"

#if defined(_MSC_VER) || defined(__MINGW32__)
#include <malloc.h>
#ifdef _MSC_VER
#define alloca _alloca
#endif
#endif
#ifdef SOLARIS
#include <alloca.h>
#endif

using std::max;
using std::min;
using namespace mozilla;
using namespace mozilla::dom;

#define NS_SET_IMAGE_REQUEST(method_, context_, request_)                   \
  if ((context_)->PresContext()->IsDynamic()) {                               \
    method_(request_);                                                      \
  } else {                                                                  \
    nsRefPtr<imgRequestProxy> req = nsContentUtils::GetStaticRequest(request_); \
    method_(req);                                                           \
  }

#define NS_SET_IMAGE_REQUEST_WITH_DOC(method_, context_, requestgetter_)      \
  {                                                                           \
    nsIDocument* doc = (context_)->PresContext()->Document();                 \
    NS_SET_IMAGE_REQUEST(method_, context_, requestgetter_(doc))              \
  }



static void
  ComputePositionValue(nsStyleContext* aStyleContext,
                       const nsCSSValue& aValue,
                       nsStyleBackground::Position& aComputedValue,
                       bool& aCanStoreInRuleTree);





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

 bool
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

 const PLDHashTableOps
nsRuleNode::ChildrenHashOps = {
  
  
  
  
  ChildrenHashHashKey,
  ChildrenHashMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  nullptr
};















void
nsRuleNode::EnsureBlockDisplay(uint8_t& display,
                               bool aConvertListItem )
{
  
  switch (display) {
  case NS_STYLE_DISPLAY_LIST_ITEM :
    if (aConvertListItem) {
      display = NS_STYLE_DISPLAY_BLOCK;
      break;
    } 
  case NS_STYLE_DISPLAY_NONE :
  case NS_STYLE_DISPLAY_CONTENTS :
    
  case NS_STYLE_DISPLAY_TABLE :
  case NS_STYLE_DISPLAY_BLOCK :
  case NS_STYLE_DISPLAY_FLEX :
  case NS_STYLE_DISPLAY_GRID :
    
    
    
    
    
    break;

  case NS_STYLE_DISPLAY_INLINE_TABLE :
    
    display = NS_STYLE_DISPLAY_TABLE;
    break;

  case NS_STYLE_DISPLAY_INLINE_FLEX:
    
    display = NS_STYLE_DISPLAY_FLEX;
    break;

  case NS_STYLE_DISPLAY_INLINE_GRID:
    
    display = NS_STYLE_DISPLAY_GRID;
    break;

  default :
    
    display = NS_STYLE_DISPLAY_BLOCK;
  }
}





void
nsRuleNode::EnsureInlineDisplay(uint8_t& display)
{
  
  switch (display) {
    case NS_STYLE_DISPLAY_BLOCK :
      display = NS_STYLE_DISPLAY_INLINE_BLOCK;
      break;
    case NS_STYLE_DISPLAY_TABLE :
      display = NS_STYLE_DISPLAY_INLINE_TABLE;
      break;
    case NS_STYLE_DISPLAY_FLEX :
      display = NS_STYLE_DISPLAY_INLINE_FLEX;
      break;
    case NS_STYLE_DISPLAY_GRID :
      display = NS_STYLE_DISPLAY_INLINE_GRID;
      break;
    case NS_STYLE_DISPLAY_BOX:
      display = NS_STYLE_DISPLAY_INLINE_BOX;
      break;
    case NS_STYLE_DISPLAY_STACK:
      display = NS_STYLE_DISPLAY_INLINE_STACK;
      break;
  }
}

static nscoord CalcLengthWith(const nsCSSValue& aValue,
                              nscoord aFontSize,
                              const nsStyleFont* aStyleFont,
                              nsStyleContext* aStyleContext,
                              nsPresContext* aPresContext,
                              bool aUseProvidedRootEmSize,
                              bool aUseUserFontSet,
                              bool& aCanStoreInRuleTree);

struct CalcLengthCalcOps : public css::BasicCoordCalcOps,
                           public css::NumbersAlreadyNormalizedOps
{
  
  const nscoord mFontSize;
  const nsStyleFont* const mStyleFont;
  nsStyleContext* const mStyleContext;
  nsPresContext* const mPresContext;
  const bool mUseProvidedRootEmSize;
  const bool mUseUserFontSet;
  bool& mCanStoreInRuleTree;

  CalcLengthCalcOps(nscoord aFontSize, const nsStyleFont* aStyleFont,
                    nsStyleContext* aStyleContext, nsPresContext* aPresContext,
                    bool aUseProvidedRootEmSize, bool aUseUserFontSet,
                    bool& aCanStoreInRuleTree)
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
    return CalcLengthWith(aValue, mFontSize, mStyleFont,
                          mStyleContext, mPresContext, mUseProvidedRootEmSize,
                          mUseUserFontSet, mCanStoreInRuleTree);
  }
};

static inline nscoord ScaleCoordRound(const nsCSSValue& aValue, float aFactor)
{
  return NSToCoordRoundWithClamp(aValue.GetFloatValue() * aFactor);
}

static inline nscoord ScaleViewportCoordTrunc(const nsCSSValue& aValue,
                                              nscoord aViewportSize)
{
  
  
  
  
  
  return NSToCoordTruncClamped(aValue.GetFloatValue() *
                               aViewportSize / 100.0f);
}

already_AddRefed<nsFontMetrics>
GetMetricsFor(nsPresContext* aPresContext,
              nsStyleContext* aStyleContext,
              const nsStyleFont* aStyleFont,
              nscoord aFontSize, 
              bool aUseUserFontSet)
{
  nsFont font = aStyleFont->mFont;
  font.size = aFontSize;
  gfxUserFontSet *fs = nullptr;
  if (aUseUserFontSet) {
    fs = aPresContext->GetUserFontSet();
  }
  gfxTextPerfMetrics *tp = aPresContext->GetTextPerfMetrics();
  gfxFont::Orientation orientation = gfxFont::eHorizontal;
  if (aStyleContext) {
    WritingMode wm(aStyleContext);
    if (wm.IsVertical() && !wm.IsSideways()) {
      orientation = gfxFont::eVertical;
    }
  }
  nsRefPtr<nsFontMetrics> fm;
  aPresContext->DeviceContext()->GetMetricsFor(font,
                                               aStyleFont->mLanguage,
                                               aStyleFont->mExplicitLanguage,
                                               orientation,
                                               fs, tp, *getter_AddRefs(fm));
  return fm.forget();
}


static nsSize CalcViewportUnitsScale(nsPresContext* aPresContext)
{
  
  
  
  aPresContext->SetUsesViewportUnits(true);

  
  
  
  
  nsSize viewportSize(aPresContext->GetVisibleArea().Size());

  
  
  nsIScrollableFrame* scrollFrame =
    aPresContext->PresShell()->GetRootScrollFrameAsScrollable();
  if (scrollFrame) {
    ScrollbarStyles styles(scrollFrame->GetScrollbarStyles());

    if (styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL ||
        styles.mVertical == NS_STYLE_OVERFLOW_SCROLL) {
      
      nsRenderingContext context(
        aPresContext->PresShell()->CreateReferenceRenderingContext());
      nsMargin sizes(scrollFrame->GetDesiredScrollbarSizes(aPresContext, &context));

      if (styles.mHorizontal == NS_STYLE_OVERFLOW_SCROLL) {
        
        
        viewportSize.height -= sizes.TopBottom();
      }

      if (styles.mVertical == NS_STYLE_OVERFLOW_SCROLL) {
        
        
        viewportSize.width -= sizes.LeftRight();
      }
    }
  }

  return viewportSize;
}

static nscoord CalcLengthWith(const nsCSSValue& aValue,
                              nscoord aFontSize,
                              const nsStyleFont* aStyleFont,
                              nsStyleContext* aStyleContext,
                              nsPresContext* aPresContext,
                              bool aUseProvidedRootEmSize,
                              
                              
                              
                              bool aUseUserFontSet,
                              bool& aCanStoreInRuleTree)
{
  NS_ASSERTION(aValue.IsLengthUnit() || aValue.IsCalcUnit(),
               "not a length or calc unit");
  NS_ASSERTION(aStyleFont || aStyleContext,
               "Must have style data");
  NS_ASSERTION(!aStyleFont || !aStyleContext,
               "Duplicate sources of data");
  NS_ASSERTION(aPresContext, "Must have prescontext");

  if (aValue.IsFixedLengthUnit()) {
    return aValue.GetFixedLength(aPresContext);
  }
  if (aValue.IsPixelLengthUnit()) {
    return aValue.GetPixelLength();
  }
  if (aValue.IsCalcUnit()) {
    
    
    
    
    
    CalcLengthCalcOps ops(aFontSize, aStyleFont,
                          aStyleContext, aPresContext,
                          aUseProvidedRootEmSize, aUseUserFontSet,
                          aCanStoreInRuleTree);
    return css::ComputeCalc(aValue, ops);
  }
  switch (aValue.GetUnit()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    case eCSSUnit_ViewportWidth: {
      nscoord viewportWidth = CalcViewportUnitsScale(aPresContext).width;
      return ScaleViewportCoordTrunc(aValue, viewportWidth);
    }
    case eCSSUnit_ViewportHeight: {
      nscoord viewportHeight = CalcViewportUnitsScale(aPresContext).height;
      return ScaleViewportCoordTrunc(aValue, viewportHeight);
    }
    case eCSSUnit_ViewportMin: {
      nsSize vuScale(CalcViewportUnitsScale(aPresContext));
      nscoord viewportMin = min(vuScale.width, vuScale.height);
      return ScaleViewportCoordTrunc(aValue, viewportMin);
    }
    case eCSSUnit_ViewportMax: {
      nsSize vuScale(CalcViewportUnitsScale(aPresContext));
      nscoord viewportMax = max(vuScale.width, vuScale.height);
      return ScaleViewportCoordTrunc(aValue, viewportMax);
    }
    
    
    
    
    
    
    
    case eCSSUnit_RootEM: {
      aPresContext->SetUsesRootEMUnits(true);
      nscoord rootFontSize;

      
      
      
      const nsStyleFont *styleFont =
        aStyleFont ? aStyleFont : aStyleContext->StyleFont();

      if (aUseProvidedRootEmSize) {
        
        
        
        
        
        if (aFontSize == -1) {
          
          
          aFontSize = styleFont->mFont.size;
        }
        rootFontSize = aFontSize;
      } else if (aStyleContext && !aStyleContext->GetParent()) {
        
        
        
        
        rootFontSize = styleFont->mFont.size;
      } else {
        
        
        nsRefPtr<nsStyleContext> rootStyle;
        const nsStyleFont *rootStyleFont = styleFont;
        Element* docElement = aPresContext->Document()->GetRootElement();

        if (docElement) {
          nsIFrame* rootFrame = docElement->GetPrimaryFrame();
          if (rootFrame) {
            rootStyle = rootFrame->StyleContext();
          } else {
            rootStyle = aPresContext->StyleSet()->ResolveStyleFor(docElement,
                                                                  nullptr);
          }
          rootStyleFont = rootStyle->StyleFont();
        }

        rootFontSize = rootStyleFont->mFont.size;
      }

      return ScaleCoordRound(aValue, float(rootFontSize));
    }
    default:
      
      
      break;
  }
  
  
  aCanStoreInRuleTree = false;
  const nsStyleFont *styleFont =
    aStyleFont ? aStyleFont : aStyleContext->StyleFont();
  if (aFontSize == -1) {
    
    
    aFontSize = styleFont->mFont.size;
  }
  switch (aValue.GetUnit()) {
    case eCSSUnit_EM: {
      
      
      return ScaleCoordRound(aValue, float(aFontSize));
    }
    case eCSSUnit_XHeight: {
      aPresContext->SetUsesExChUnits(true);
      nsRefPtr<nsFontMetrics> fm =
        GetMetricsFor(aPresContext, aStyleContext, styleFont,
                      aFontSize, aUseUserFontSet);
      return ScaleCoordRound(aValue, float(fm->XHeight()));
    }
    case eCSSUnit_Char: {
      aPresContext->SetUsesExChUnits(true);
      nsRefPtr<nsFontMetrics> fm =
        GetMetricsFor(aPresContext, aStyleContext, styleFont,
                      aFontSize, aUseUserFontSet);
      gfxFloat zeroWidth =
        fm->GetThebesFontGroup()->GetFirstValidFont()->
          GetMetrics(fm->Orientation()).zeroOrAveCharWidth;

      return ScaleCoordRound(aValue, ceil(aPresContext->AppUnitsPerDevPixel() *
                                          zeroWidth));
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
                       bool& aCanStoreInRuleTree)
{
  NS_ASSERTION(aStyleContext, "Must have style data");

  return CalcLengthWith(aValue, -1, nullptr,
                        aStyleContext, aPresContext,
                        false, true, aCanStoreInRuleTree);
}


static inline nscoord CalcLength(const nsCSSValue& aValue,
                                 nsStyleContext* aStyleContext,
                                 nsPresContext* aPresContext,
                                 bool& aCanStoreInRuleTree)
{
  return nsRuleNode::CalcLength(aValue, aStyleContext,
                                aPresContext, aCanStoreInRuleTree);
}

 nscoord
nsRuleNode::CalcLengthWithInitialFont(nsPresContext* aPresContext,
                                      const nsCSSValue& aValue)
{
  nsStyleFont defaultFont(aPresContext); 
  bool canStoreInRuleTree;
  return CalcLengthWith(aValue, -1, &defaultFont,
                        nullptr, aPresContext,
                        true, false, canStoreInRuleTree);
}

struct LengthPercentPairCalcOps : public css::NumbersAlreadyNormalizedOps
{
  typedef nsRuleNode::ComputedCalc result_type;

  LengthPercentPairCalcOps(nsStyleContext* aContext,
                           nsPresContext* aPresContext,
                           bool& aCanStoreInRuleTree)
    : mContext(aContext),
      mPresContext(aPresContext),
      mCanStoreInRuleTree(aCanStoreInRuleTree),
      mHasPercent(false) {}

  nsStyleContext* mContext;
  nsPresContext* mPresContext;
  bool& mCanStoreInRuleTree;
  bool mHasPercent;

  result_type ComputeLeafValue(const nsCSSValue& aValue)
  {
    if (aValue.GetUnit() == eCSSUnit_Percent) {
      mHasPercent = true;
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
    MOZ_ASSERT(aCalcFunction == eCSSUnit_Calc_Minus,
               "min() and max() are not allowed in calc() on transform");
    return result_type(NSCoordSaturatingSubtract(aValue1.mLength,
                                                 aValue2.mLength, 0),
                       aValue1.mPercent - aValue2.mPercent);
  }

  result_type
  MergeMultiplicativeL(nsCSSUnit aCalcFunction,
                       float aValue1, result_type aValue2)
  {
    MOZ_ASSERT(aCalcFunction == eCSSUnit_Calc_Times_L,
               "unexpected unit");
    return result_type(NSCoordSaturatingMultiply(aValue2.mLength, aValue1),
                       aValue1 * aValue2.mPercent);
  }

  result_type
  MergeMultiplicativeR(nsCSSUnit aCalcFunction,
                       result_type aValue1, float aValue2)
  {
    MOZ_ASSERT(aCalcFunction == eCSSUnit_Calc_Times_R ||
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
                            bool& aCanStoreInRuleTree)
{
  LengthPercentPairCalcOps ops(aStyleContext, aStyleContext->PresContext(),
                               aCanStoreInRuleTree);
  nsRuleNode::ComputedCalc vals = ComputeCalc(aValue, ops);

  nsStyleCoord::Calc* calcObj = new nsStyleCoord::Calc;

  calcObj->mLength = vals.mLength;
  calcObj->mPercent = vals.mPercent;
  calcObj->mHasPercent = ops.mHasPercent;

  aCoord.SetCalcValue(calcObj);
}

 nsRuleNode::ComputedCalc
nsRuleNode::SpecifiedCalcToComputedCalc(const nsCSSValue& aValue,
                                        nsStyleContext* aStyleContext,
                                        nsPresContext* aPresContext,
                                        bool& aCanStoreInRuleTree)
{
  LengthPercentPairCalcOps ops(aStyleContext, aPresContext,
                               aCanStoreInRuleTree);
  return ComputeCalc(aValue, ops);
}



 nscoord
nsRuleNode::ComputeComputedCalc(const nsStyleCoord& aValue,
                                nscoord aPercentageBasis)
{
  nsStyleCoord::Calc* calc = aValue.GetCalcValue();
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
      MOZ_ASSERT(false, "unexpected unit");
      return 0;
  }
}








static float
GetFloatFromBoxPosition(int32_t aEnumValue)
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
#define SETCOORD_INITIAL_HUNDRED_PCT    0x00004000
#define SETCOORD_INITIAL_FACTOR_ONE     0x00008000
#define SETCOORD_INITIAL_FACTOR_ZERO    0x00010000
#define SETCOORD_CALC_LENGTH_ONLY       0x00020000
#define SETCOORD_CALC_CLAMP_NONNEGATIVE 0x00040000 // modifier for CALC_LENGTH_ONLY
#define SETCOORD_STORE_CALC             0x00080000
#define SETCOORD_BOX_POSITION           0x00100000 // exclusive with _ENUMERATED
#define SETCOORD_ANGLE                  0x00200000
#define SETCOORD_UNSET_INHERIT          0x00400000
#define SETCOORD_UNSET_INITIAL          0x00800000

#define SETCOORD_LP     (SETCOORD_LENGTH | SETCOORD_PERCENT)
#define SETCOORD_LH     (SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_AH     (SETCOORD_AUTO | SETCOORD_INHERIT)
#define SETCOORD_LAH    (SETCOORD_AUTO | SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_LPH    (SETCOORD_LP | SETCOORD_INHERIT)
#define SETCOORD_LPAH   (SETCOORD_LP | SETCOORD_AH)
#define SETCOORD_LPE    (SETCOORD_LP | SETCOORD_ENUMERATED)
#define SETCOORD_LPEH   (SETCOORD_LPE | SETCOORD_INHERIT)
#define SETCOORD_LPAEH  (SETCOORD_LPAH | SETCOORD_ENUMERATED)
#define SETCOORD_LPO    (SETCOORD_LP | SETCOORD_NONE)
#define SETCOORD_LPOH   (SETCOORD_LPH | SETCOORD_NONE)
#define SETCOORD_LPOEH  (SETCOORD_LPOH | SETCOORD_ENUMERATED)
#define SETCOORD_LE     (SETCOORD_LENGTH | SETCOORD_ENUMERATED)
#define SETCOORD_LEH    (SETCOORD_LE | SETCOORD_INHERIT)
#define SETCOORD_IA     (SETCOORD_INTEGER | SETCOORD_AUTO)
#define SETCOORD_LAE    (SETCOORD_LENGTH | SETCOORD_AUTO | SETCOORD_ENUMERATED)


static bool SetCoord(const nsCSSValue& aValue, nsStyleCoord& aCoord,
                       const nsStyleCoord& aParentCoord,
                       int32_t aMask, nsStyleContext* aStyleContext,
                       nsPresContext* aPresContext,
                       bool& aCanStoreInRuleTree)
{
  bool result = true;
  if (aValue.GetUnit() == eCSSUnit_Null) {
    result = false;
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
  else if ((((aMask & SETCOORD_INHERIT) != 0) &&
            aValue.GetUnit() == eCSSUnit_Inherit) ||
           (((aMask & SETCOORD_UNSET_INHERIT) != 0) &&
            aValue.GetUnit() == eCSSUnit_Unset)) {
    aCoord = aParentCoord;  
    aCanStoreInRuleTree = false;
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
  else if (((aMask & SETCOORD_STORE_CALC) != 0) &&
           (aValue.IsCalcUnit())) {
    SpecifiedCalcToComputedCalc(aValue, aCoord, aStyleContext,
                                aCanStoreInRuleTree);
  }
  else if (aValue.GetUnit() == eCSSUnit_Initial ||
           (aValue.GetUnit() == eCSSUnit_Unset &&
            ((aMask & SETCOORD_UNSET_INITIAL) != 0))) {
    if ((aMask & SETCOORD_INITIAL_AUTO) != 0) {
      aCoord.SetAutoValue();
    }
    else if ((aMask & SETCOORD_INITIAL_ZERO) != 0) {
      aCoord.SetCoordValue(0);
    }
    else if ((aMask & SETCOORD_INITIAL_FACTOR_ZERO) != 0) {
      aCoord.SetFactorValue(0.0f);
    }
    else if ((aMask & SETCOORD_INITIAL_NONE) != 0) {
      aCoord.SetNoneValue();
    }
    else if ((aMask & SETCOORD_INITIAL_NORMAL) != 0) {
      aCoord.SetNormalValue();
    }
    else if ((aMask & SETCOORD_INITIAL_HALF) != 0) {
      aCoord.SetPercentValue(0.5f);
    }
    else if ((aMask & SETCOORD_INITIAL_HUNDRED_PCT) != 0) {
      aCoord.SetPercentValue(1.0f);
    }
    else if ((aMask & SETCOORD_INITIAL_FACTOR_ONE) != 0) {
      aCoord.SetFactorValue(1.0f);
    }
    else {
      result = false;  
    }
  }
  else if ((aMask & SETCOORD_ANGLE) != 0 &&
           (aValue.IsAngularUnit())) {
    nsStyleUnit unit;
    switch (aValue.GetUnit()) {
      case eCSSUnit_Degree: unit = eStyleUnit_Degree; break;
      case eCSSUnit_Grad:   unit = eStyleUnit_Grad; break;
      case eCSSUnit_Radian: unit = eStyleUnit_Radian; break;
      case eCSSUnit_Turn:   unit = eStyleUnit_Turn; break;
      default: NS_NOTREACHED("unrecognized angular unit");
        unit = eStyleUnit_Degree;
    }
    aCoord.SetAngleValue(aValue.GetAngleValue(), unit);
  }
  else {
    result = false;  
  }
  return result;
}



static inline bool SetAbsCoord(const nsCSSValue& aValue,
                                 nsStyleCoord& aCoord,
                                 int32_t aMask)
{
  MOZ_ASSERT((aMask & (SETCOORD_LH | SETCOORD_UNSET_INHERIT |
                       SETCOORD_UNSET_INITIAL)) == 0,
             "does not handle SETCOORD_LENGTH, SETCOORD_INHERIT and "
             "SETCOORD_UNSET_*");

  
  
  const nsStyleCoord dummyParentCoord;
  nsStyleContext* dummyStyleContext = nullptr;
  nsPresContext* dummyPresContext = nullptr;
  bool dummyCanStoreInRuleTree = true;

  bool rv = SetCoord(aValue, aCoord, dummyParentCoord, aMask,
                       dummyStyleContext, dummyPresContext,
                       dummyCanStoreInRuleTree);
  MOZ_ASSERT(dummyCanStoreInRuleTree,
             "SetCoord() should not modify dummyCanStoreInRuleTree.");

  return rv;
}




static bool
SetPairCoords(const nsCSSValue& aValue,
              nsStyleCoord& aCoordX, nsStyleCoord& aCoordY,
              const nsStyleCoord& aParentX, const nsStyleCoord& aParentY,
              int32_t aMask, nsStyleContext* aStyleContext,
              nsPresContext* aPresContext, bool& aCanStoreInRuleTree)
{
  const nsCSSValue& valX =
    aValue.GetUnit() == eCSSUnit_Pair ? aValue.GetPairValue().mXValue : aValue;
  const nsCSSValue& valY =
    aValue.GetUnit() == eCSSUnit_Pair ? aValue.GetPairValue().mYValue : aValue;

  bool cX = SetCoord(valX, aCoordX, aParentX, aMask, aStyleContext,
                       aPresContext, aCanStoreInRuleTree);
  mozilla::DebugOnly<bool> cY = SetCoord(valY, aCoordY, aParentY, aMask, 
                       aStyleContext, aPresContext, aCanStoreInRuleTree);
  MOZ_ASSERT(cX == cY, "changed one but not the other");
  return cX;
}

static bool SetColor(const nsCSSValue& aValue, const nscolor aParentColor,
                       nsPresContext* aPresContext, nsStyleContext *aContext,
                       nscolor& aResult, bool& aCanStoreInRuleTree)
{
  bool    result = false;
  nsCSSUnit unit = aValue.GetUnit();

  if (aValue.IsNumericColorUnit()) {
    aResult = aValue.GetColorValue();
    result = true;
  }
  else if (eCSSUnit_Ident == unit) {
    nsAutoString  value;
    aValue.GetStringValue(value);
    nscolor rgba;
    if (NS_ColorNameToRGB(value, &rgba)) {
      aResult = rgba;
      result = true;
    }
  }
  else if (eCSSUnit_EnumColor == unit) {
    int32_t intValue = aValue.GetIntValue();
    if (0 <= intValue) {
      LookAndFeel::ColorID colorID = (LookAndFeel::ColorID) intValue;
      if (NS_SUCCEEDED(LookAndFeel::GetColor(colorID, &aResult))) {
        result = true;
      }
    }
    else {
      aResult = NS_RGB(0, 0, 0);
      result = false;
      switch (intValue) {
        case NS_COLOR_MOZ_HYPERLINKTEXT:
          if (aPresContext) {
            aResult = aPresContext->DefaultLinkColor();
            result = true;
          }
          break;
        case NS_COLOR_MOZ_VISITEDHYPERLINKTEXT:
          if (aPresContext) {
            aResult = aPresContext->DefaultVisitedLinkColor();
            result = true;
          }
          break;
        case NS_COLOR_MOZ_ACTIVEHYPERLINKTEXT:
          if (aPresContext) {
            aResult = aPresContext->DefaultActiveLinkColor();
            result = true;
          }
          break;
        case NS_COLOR_CURRENTCOLOR:
          
          
          aCanStoreInRuleTree = false;
          if (aContext) {
            aResult = aContext->StyleColor()->mColor;
            result = true;
          }
          break;
        case NS_COLOR_MOZ_DEFAULT_COLOR:
          if (aPresContext) {
            aResult = aPresContext->DefaultColor();
            result = true;
          }
          break;
        case NS_COLOR_MOZ_DEFAULT_BACKGROUND_COLOR:
          if (aPresContext) {
            aResult = aPresContext->DefaultBackgroundColor();
            result = true;
          }
          break;
        default:
          NS_NOTREACHED("Should never have an unknown negative colorID.");
          break;
      }
    }
  }
  else if (eCSSUnit_Inherit == unit) {
    aResult = aParentColor;
    result = true;
    aCanStoreInRuleTree = false;
  }
  else if (eCSSUnit_Enumerated == unit &&
           aValue.GetIntValue() == NS_STYLE_COLOR_INHERIT_FROM_BODY) {
    NS_ASSERTION(aPresContext->CompatibilityMode() == eCompatibility_NavQuirks,
                 "Should only get this value in quirks mode");
    
    
    
    aResult = aPresContext->BodyTextColor();
    result = true;
    aCanStoreInRuleTree = false;
  }
  return result;
}

static void SetGradientCoord(const nsCSSValue& aValue, nsPresContext* aPresContext,
                             nsStyleContext* aContext, nsStyleCoord& aResult,
                             bool& aCanStoreInRuleTree)
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
                        bool& aCanStoreInRuleTree)
{
  MOZ_ASSERT(aValue.GetUnit() == eCSSUnit_Gradient,
             "The given data is not a gradient");

  const nsCSSValueGradient* gradient = aValue.GetGradientValue();

  if (gradient->mIsExplicitSize) {
    SetCoord(gradient->GetRadiusX(), aResult.mRadiusX, nsStyleCoord(),
             SETCOORD_LP | SETCOORD_STORE_CALC,
             aContext, aPresContext, aCanStoreInRuleTree);
    if (gradient->GetRadiusY().GetUnit() != eCSSUnit_None) {
      SetCoord(gradient->GetRadiusY(), aResult.mRadiusY, nsStyleCoord(),
               SETCOORD_LP | SETCOORD_STORE_CALC,
               aContext, aPresContext, aCanStoreInRuleTree);
      aResult.mShape = NS_STYLE_GRADIENT_SHAPE_ELLIPTICAL;
    } else {
      aResult.mRadiusY = aResult.mRadiusX;
      aResult.mShape = NS_STYLE_GRADIENT_SHAPE_CIRCULAR;
    }
    aResult.mSize = NS_STYLE_GRADIENT_SIZE_EXPLICIT_SIZE;
  } else if (gradient->mIsRadial) {
    if (gradient->GetRadialShape().GetUnit() == eCSSUnit_Enumerated) {
      aResult.mShape = gradient->GetRadialShape().GetIntValue();
    } else {
      NS_ASSERTION(gradient->GetRadialShape().GetUnit() == eCSSUnit_None,
                   "bad unit for radial shape");
      aResult.mShape = NS_STYLE_GRADIENT_SHAPE_ELLIPTICAL;
    }
    if (gradient->GetRadialSize().GetUnit() == eCSSUnit_Enumerated) {
      aResult.mSize = gradient->GetRadialSize().GetIntValue();
    } else {
      NS_ASSERTION(gradient->GetRadialSize().GetUnit() == eCSSUnit_None,
                   "bad unit for radial shape");
      aResult.mSize = NS_STYLE_GRADIENT_SIZE_FARTHEST_CORNER;
    }
  } else {
    NS_ASSERTION(gradient->GetRadialShape().GetUnit() == eCSSUnit_None,
                 "bad unit for linear shape");
    NS_ASSERTION(gradient->GetRadialSize().GetUnit() == eCSSUnit_None,
                 "bad unit for linear size");
    aResult.mShape = NS_STYLE_GRADIENT_SHAPE_LINEAR;
    aResult.mSize = NS_STYLE_GRADIENT_SIZE_FARTHEST_CORNER;
  }

  aResult.mLegacySyntax = gradient->mIsLegacySyntax;

  
  SetGradientCoord(gradient->mBgPos.mXValue, aPresContext, aContext,
                   aResult.mBgPosX, aCanStoreInRuleTree);

  SetGradientCoord(gradient->mBgPos.mYValue, aPresContext, aContext,
                   aResult.mBgPosY, aCanStoreInRuleTree);

  aResult.mRepeating = gradient->mIsRepeating;

  
  const nsStyleCoord dummyParentCoord;
  if (!SetCoord(gradient->mAngle, aResult.mAngle, dummyParentCoord, SETCOORD_ANGLE,
                aContext, aPresContext, aCanStoreInRuleTree)) {
    NS_ASSERTION(gradient->mAngle.GetUnit() == eCSSUnit_None,
                 "bad unit for gradient angle");
    aResult.mAngle.SetNoneValue();
  }

  
  for (uint32_t i = 0; i < gradient->mStops.Length(); i++) {
    nsStyleGradientStop stop;
    const nsCSSValueGradientStop &valueStop = gradient->mStops[i];

    if (!SetCoord(valueStop.mLocation, stop.mLocation,
                  nsStyleCoord(), SETCOORD_LPO | SETCOORD_STORE_CALC,
                  aContext, aPresContext, aCanStoreInRuleTree)) {
      NS_NOTREACHED("unexpected unit for gradient stop location");
    }

    stop.mIsInterpolationHint = valueStop.mIsInterpolationHint;

    
    
    NS_ASSERTION(valueStop.mColor.GetUnit() != eCSSUnit_Inherit,
                 "inherit is not a valid color for gradient stops");
    if (!valueStop.mIsInterpolationHint) {
      SetColor(valueStop.mColor, NS_RGB(0, 0, 0), aPresContext,
              aContext, stop.mColor, aCanStoreInRuleTree);
    } else {
      
      
      stop.mColor = NS_RGB(0, 0, 0);
    }

    aResult.mStops.AppendElement(stop);
  }
}


static void SetStyleImageToImageRect(nsStyleContext* aStyleContext,
                                     const nsCSSValue& aValue,
                                     nsStyleImage& aResult)
{
  MOZ_ASSERT(aValue.GetUnit() == eCSSUnit_Function &&
             aValue.EqualsFunction(eCSSKeyword__moz_image_rect),
             "the value is not valid -moz-image-rect()");

  nsCSSValue::Array* arr = aValue.GetArrayValue();
  MOZ_ASSERT(arr && arr->Count() == 6, "invalid number of arguments");

  
  if (arr->Item(1).GetUnit() == eCSSUnit_Image) {
    NS_SET_IMAGE_REQUEST_WITH_DOC(aResult.SetImageData,
                                  aStyleContext,
                                  arr->Item(1).GetImageValue)
  } else {
    NS_WARNING("nsCSSValue::Image::Image() failed?");
  }

  
  nsStyleSides cropRect;
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord coord;
    const nsCSSValue& val = arr->Item(2 + side);

#ifdef DEBUG
    bool unitOk =
#endif
      SetAbsCoord(val, coord, SETCOORD_FACTOR | SETCOORD_PERCENT);
    MOZ_ASSERT(unitOk, "Incorrect data structure created by CSS parser");
    cropRect.Set(side, coord);
  }
  aResult.SetCropRect(&cropRect);
}

static void SetStyleImage(nsStyleContext* aStyleContext,
                          const nsCSSValue& aValue,
                          nsStyleImage& aResult,
                          bool& aCanStoreInRuleTree)
{
  if (aValue.GetUnit() == eCSSUnit_Null) {
    return;
  }

  aResult.SetNull();

  switch (aValue.GetUnit()) {
    case eCSSUnit_Image:
      NS_SET_IMAGE_REQUEST_WITH_DOC(aResult.SetImageData,
                                    aStyleContext,
                                    aValue.GetImageValue)
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
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
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
#define SETDSC_UNSET_INHERIT          0x00400000
#define SETDSC_UNSET_INITIAL          0x00800000


template <typename FieldT,
          typename T1, typename T2, typename T3, typename T4, typename T5>
static void
SetDiscrete(const nsCSSValue& aValue, FieldT & aField,
            bool& aCanStoreInRuleTree, uint32_t aMask,
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
    aCanStoreInRuleTree = false;
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

  case eCSSUnit_Unset:
    if (aMask & SETDSC_UNSET_INHERIT) {
      aCanStoreInRuleTree = false;
      aField = aParentValue;
      return;
    }
    if (aMask & SETDSC_UNSET_INITIAL) {
      aField = aInitialValue;
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
#define SETFCT_UNSET_INHERIT  0x00400000
#define SETFCT_UNSET_INITIAL  0x00800000

static void
SetFactor(const nsCSSValue& aValue, float& aField, bool& aCanStoreInRuleTree,
          float aParentValue, float aInitialValue, uint32_t aFlags = 0)
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
    aCanStoreInRuleTree = false;
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

  case eCSSUnit_Unset:
    if (aFlags & SETFCT_UNSET_INHERIT) {
      aCanStoreInRuleTree = false;
      aField = aParentValue;
      return;
    }
    if (aFlags & SETFCT_UNSET_INITIAL) {
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
  
  return aPresContext->PresShell()->AllocateByObjectID(nsPresArena::nsRuleNode_id, sz);
}

 PLDHashOperator
nsRuleNode::EnqueueRuleNodeChildren(PLDHashTable *table, PLDHashEntryHdr *hdr,
                                    uint32_t number, void *arg)
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
    destroyQueue = nullptr;
    destroyQueueTail = &destroyQueue;
  }

  if (ChildrenAreHashed()) {
    PLDHashTable *children = ChildrenHash();
    PL_DHashTableEnumerate(children, EnqueueRuleNodeChildren,
                           &destroyQueueTail);
    *destroyQueueTail = nullptr; 
    PL_DHashTableDestroy(children);
  } else if (HaveChildren()) {
    *destroyQueueTail = ChildrenList();
    do {
      destroyQueueTail = &(*destroyQueueTail)->mNextSibling;
    } while (*destroyQueueTail);
  }
  mChildren.asVoid = nullptr;

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

  
  
  mPresContext->PresShell()->FreeByObjectID(nsPresArena::nsRuleNode_id, this);
}

nsRuleNode* nsRuleNode::CreateRootNode(nsPresContext* aPresContext)
{
  return new (aPresContext)
    nsRuleNode(aPresContext, nullptr, nullptr, 0xff, false);
}

nsRuleNode::nsRuleNode(nsPresContext* aContext, nsRuleNode* aParent,
                       nsIStyleRule* aRule, uint8_t aLevel,
                       bool aIsImportant)
  : mPresContext(aContext),
    mParent(aParent),
    mRule(aRule),
    mNextSibling(nullptr),
    mDependentBits((uint32_t(aLevel) << NS_RULE_NODE_LEVEL_SHIFT) |
                   (aIsImportant ? NS_RULE_NODE_IS_IMPORTANT : 0)),
    mNoneBits(0),
    mRefCnt(0)
{
  MOZ_ASSERT(aContext);
  MOZ_ASSERT(IsRoot() == !aRule,
             "non-root rule nodes must have a rule");

  mChildren.asVoid = nullptr;
  MOZ_COUNT_CTOR(nsRuleNode);

  if (mRule) {
    mRule->AddRef();
  }

  NS_ASSERTION(IsRoot() || GetLevel() == aLevel, "not enough bits");
  NS_ASSERTION(IsRoot() || IsImportantRule() == aIsImportant, "yikes");
  


  if (!IsRoot()) {
    mParent->AddRef();
    aContext->StyleSet()->RuleNodeUnused();
  }

  
  
  MOZ_ASSERT(IsRoot() || GetLevel() != nsStyleSet::eAnimationSheet ||
             mParent->IsRoot() ||
             mParent->GetLevel() != nsStyleSet::eAnimationSheet,
             "must be only one rule at animation level");
}

nsRuleNode::~nsRuleNode()
{
  MOZ_COUNT_DTOR(nsRuleNode);
  if (mStyleData.mResetData || mStyleData.mInheritedData)
    mStyleData.Destroy(mDependentBits, mPresContext);
  if (mRule) {
    mRule->Release();
  }
}

nsRuleNode*
nsRuleNode::Transition(nsIStyleRule* aRule, uint8_t aLevel,
                       bool aIsImportantRule)
{
  nsRuleNode* next = nullptr;
  nsRuleNode::Key key(aRule, aLevel, aIsImportantRule);

  if (HaveChildren() && !ChildrenAreHashed()) {
    int32_t numKids = 0;
    nsRuleNode* curr = ChildrenList();
    while (curr && curr->GetKey() != key) {
      curr = curr->mNextSibling;
      ++numKids;
    }
    if (curr)
      next = curr;
    else if (numKids >= kMaxChildrenInList)
      ConvertChildrenToHash(numKids);
  }

  if (ChildrenAreHashed()) {
    ChildrenHashEntry *entry = static_cast<ChildrenHashEntry*>
      (PL_DHashTableAdd(ChildrenHash(), &key, fallible));
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

nsRuleNode*
nsRuleNode::RuleTree()
{
  nsRuleNode* n = this;
  while (n->mParent) {
    n = n->mParent;
  }
  return n;
}

void nsRuleNode::SetUsedDirectly()
{
  mDependentBits |= NS_RULE_NODE_USED_DIRECTLY;

  
  
  
  if (mDependentBits & NS_STYLE_INHERIT_MASK) {
    for (nsStyleStructID sid = nsStyleStructID(0); sid < nsStyleStructID_Length;
         sid = nsStyleStructID(sid + 1)) {
      uint32_t bit = nsCachedStyleData::GetBitForSID(sid);
      if (mDependentBits & bit) {
        nsRuleNode *source = mParent;
        while ((source->mDependentBits & bit) && !source->IsUsedDirectly()) {
          source = source->mParent;
        }
        void *data = source->mStyleData.GetStyleData(sid);
        NS_ASSERTION(data, "unexpected null struct");
        mStyleData.SetStyleData(sid, mPresContext, data);
      }
    }
  }
}

void
nsRuleNode::ConvertChildrenToHash(int32_t aNumKids)
{
  NS_ASSERTION(!ChildrenAreHashed() && HaveChildren(),
               "must have a non-empty list of children");
  PLDHashTable *hash = PL_NewDHashTable(&ChildrenHashOps,
                                        sizeof(ChildrenHashEntry),
                                        aNumKids);
  if (!hash)
    return;
  for (nsRuleNode* curr = ChildrenList(); curr; curr = curr->mNextSibling) {
    
    ChildrenHashEntry *entry = static_cast<ChildrenHashEntry*>(
      PL_DHashTableAdd(hash, curr->mRule, fallible));
    NS_ASSERTION(!entry->mRuleNode, "duplicate entries in list");
    entry->mRuleNode = curr;
  }
  SetChildrenHash(hash);
}

inline void
nsRuleNode::PropagateNoneBit(uint32_t aBit, nsRuleNode* aHighestNode)
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
nsRuleNode::PropagateDependentBit(nsStyleStructID aSID, nsRuleNode* aHighestNode,
                                  void* aStruct)
{
  NS_ASSERTION(aStruct, "expected struct");

  uint32_t bit = nsCachedStyleData::GetBitForSID(aSID);
  for (nsRuleNode* curr = this; curr != aHighestNode; curr = curr->mParent) {
    if (curr->mDependentBits & bit) {
#ifdef DEBUG
      while (curr != aHighestNode) {
        NS_ASSERTION(curr->mDependentBits & bit, "bit not set");
        curr = curr->mParent;
      }
#endif
      break;
    }

    curr->mDependentBits |= bit;

    if (curr->IsUsedDirectly()) {
      curr->mStyleData.SetStyleData(aSID, mPresContext, aStruct);
    }
  }
}

 void
nsRuleNode::PropagateGrandancestorBit(nsStyleContext* aContext,
                                      nsStyleContext* aContextInheritedFrom)
{
  MOZ_ASSERT(aContext);
  MOZ_ASSERT(aContextInheritedFrom &&
             aContextInheritedFrom != aContext &&
             aContextInheritedFrom != aContext->GetParent(),
             "aContextInheritedFrom must be an ancestor of aContext's parent");

  aContext->AddStyleBit(NS_STYLE_USES_GRANDANCESTOR_STYLE);

  nsStyleContext* context = aContext->GetParent();
  if (!context) {
    return;
  }

  for (;;) {
    nsStyleContext* parent = context->GetParent();
    if (!parent) {
      MOZ_ASSERT(false, "aContextInheritedFrom must be an ancestor of "
                        "aContext's parent");
      break;
    }
    if (parent == aContextInheritedFrom) {
      break;
    }
    context->AddStyleBit(NS_STYLE_USES_GRANDANCESTOR_STYLE);
    context = parent;
  }
}











typedef nsRuleNode::RuleDetail
  (* CheckCallbackFn)(const nsRuleData* aRuleData,
                      nsRuleNode::RuleDetail aResult);







inline void
ExamineCSSValue(const nsCSSValue& aValue,
                uint32_t& aSpecifiedCount,
                uint32_t& aInheritedCount,
                uint32_t& aUnsetCount)
{
  if (aValue.GetUnit() != eCSSUnit_Null) {
    ++aSpecifiedCount;
    if (aValue.GetUnit() == eCSSUnit_Inherit) {
      ++aInheritedCount;
    } else if (aValue.GetUnit() == eCSSUnit_Unset) {
      ++aUnsetCount;
    }
  }
}

static nsRuleNode::RuleDetail
CheckFontCallback(const nsRuleData* aRuleData,
                  nsRuleNode::RuleDetail aResult)
{
  
  
  
  
  const nsCSSValue& size = *aRuleData->ValueForFontSize();
  const nsCSSValue& weight = *aRuleData->ValueForFontWeight();
  if ((size.IsRelativeLengthUnit() && size.GetUnit() != eCSSUnit_RootEM) ||
      size.GetUnit() == eCSSUnit_Percent ||
      (size.GetUnit() == eCSSUnit_Enumerated &&
       (size.GetIntValue() == NS_STYLE_FONT_SIZE_SMALLER ||
        size.GetIntValue() == NS_STYLE_FONT_SIZE_LARGER)) ||
      aRuleData->ValueForScriptLevel()->GetUnit() == eCSSUnit_Integer ||
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

static nsRuleNode::RuleDetail
CheckVariablesCallback(const nsRuleData* aRuleData,
                       nsRuleNode::RuleDetail aResult)
{
  
  
  if (aRuleData->mVariables) {
    return nsRuleNode::eRulePartialMixed;
  }
  return nsRuleNode::eRuleNone;
}

#define FLAG_DATA_FOR_PROPERTY(name_, id_, method_, flags_, pref_,          \
                               parsevariant_, kwtable_, stylestructoffset_, \
                               animtype_)                                   \
  flags_,



static const uint32_t gFontFlags[] = {
#define CSS_PROP_FONT FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_FONT
};

static const uint32_t gDisplayFlags[] = {
#define CSS_PROP_DISPLAY FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_DISPLAY
};

static const uint32_t gVisibilityFlags[] = {
#define CSS_PROP_VISIBILITY FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_VISIBILITY
};

static const uint32_t gMarginFlags[] = {
#define CSS_PROP_MARGIN FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_MARGIN
};

static const uint32_t gBorderFlags[] = {
#define CSS_PROP_BORDER FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_BORDER
};

static const uint32_t gPaddingFlags[] = {
#define CSS_PROP_PADDING FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_PADDING
};

static const uint32_t gOutlineFlags[] = {
#define CSS_PROP_OUTLINE FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_OUTLINE
};

static const uint32_t gListFlags[] = {
#define CSS_PROP_LIST FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_LIST
};

static const uint32_t gColorFlags[] = {
#define CSS_PROP_COLOR FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_COLOR
};

static const uint32_t gBackgroundFlags[] = {
#define CSS_PROP_BACKGROUND FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_BACKGROUND
};

static const uint32_t gPositionFlags[] = {
#define CSS_PROP_POSITION FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_POSITION
};

static const uint32_t gTableFlags[] = {
#define CSS_PROP_TABLE FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TABLE
};

static const uint32_t gTableBorderFlags[] = {
#define CSS_PROP_TABLEBORDER FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TABLEBORDER
};

static const uint32_t gContentFlags[] = {
#define CSS_PROP_CONTENT FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_CONTENT
};

static const uint32_t gQuotesFlags[] = {
#define CSS_PROP_QUOTES FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_QUOTES
};

static const uint32_t gTextFlags[] = {
#define CSS_PROP_TEXT FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TEXT
};

static const uint32_t gTextResetFlags[] = {
#define CSS_PROP_TEXTRESET FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TEXTRESET
};

static const uint32_t gUserInterfaceFlags[] = {
#define CSS_PROP_USERINTERFACE FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_USERINTERFACE
};

static const uint32_t gUIResetFlags[] = {
#define CSS_PROP_UIRESET FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_UIRESET
};

static const uint32_t gXULFlags[] = {
#define CSS_PROP_XUL FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_XUL
};

static const uint32_t gSVGFlags[] = {
#define CSS_PROP_SVG FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_SVG
};

static const uint32_t gSVGResetFlags[] = {
#define CSS_PROP_SVGRESET FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_SVGRESET
};

static const uint32_t gColumnFlags[] = {
#define CSS_PROP_COLUMN FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_COLUMN
};



static const uint32_t gVariablesFlags[] = {
  0,
#define CSS_PROP_VARIABLES FLAG_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_VARIABLES
};
static_assert(sizeof(gVariablesFlags) == sizeof(uint32_t),
              "if nsStyleVariables has properties now you can remove the dummy "
              "gVariablesFlags entry");

#undef FLAG_DATA_FOR_PROPERTY

static const uint32_t* gFlagsByStruct[] = {

#define STYLE_STRUCT(name, checkdata_cb) \
  g##name##Flags,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

};

static const CheckCallbackFn gCheckCallbacks[] = {

#define STYLE_STRUCT(name, checkdata_cb) \
  checkdata_cb,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

};

#ifdef DEBUG
static bool
AreAllMathMLPropertiesUndefined(const nsRuleData* aRuleData)
{
  return
    aRuleData->ValueForScriptLevel()->GetUnit() == eCSSUnit_Null &&
    aRuleData->ValueForScriptSizeMultiplier()->GetUnit() == eCSSUnit_Null &&
    aRuleData->ValueForScriptMinSize()->GetUnit() == eCSSUnit_Null &&
    aRuleData->ValueForMathVariant()->GetUnit() == eCSSUnit_Null &&
    aRuleData->ValueForMathDisplay()->GetUnit() == eCSSUnit_Null;
}
#endif

inline nsRuleNode::RuleDetail
nsRuleNode::CheckSpecifiedProperties(const nsStyleStructID aSID,
                                     const nsRuleData* aRuleData)
{
  
  uint32_t total = 0,      
           specified = 0,  
           inherited = 0,  
                           
           unset = 0;      

  
  MOZ_ASSERT(aRuleData->mValueOffsets[aSID] == 0,
             "we assume the value offset is zero instead of adding it");
  for (nsCSSValue *values = aRuleData->mValueStorage,
              *values_end = values + nsCSSProps::PropertyCountInStruct(aSID);
       values != values_end; ++values) {
    ++total;
    ExamineCSSValue(*values, specified, inherited, unset);
  }

  if (!nsCachedStyleData::IsReset(aSID)) {
    
    inherited += unset;
    unset = 0;
  }

#if 0
  printf("CheckSpecifiedProperties: SID=%d total=%d spec=%d inh=%d.\n",
         aSID, total, specified, inherited);
#endif

  NS_ASSERTION(aSID != eStyleStruct_Font ||
               mPresContext->Document()->GetMathMLEnabled() ||
               AreAllMathMLPropertiesUndefined(aRuleData),
               "MathML style property was defined even though MathML is disabled");

  




  nsRuleNode::RuleDetail result;
  if (inherited == total)
    result = eRuleFullInherited;
  else if (specified == total
           
           
           
           
           
           
           || (aSID == eStyleStruct_Font && specified + 5 == total &&
               !mPresContext->Document()->GetMathMLEnabled())
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




inline uint32_t
GetPseudoRestriction(nsStyleContext *aContext)
{
  
  uint32_t pseudoRestriction = 0;
  nsIAtom *pseudoType = aContext->GetPseudo();
  if (pseudoType) {
    if (pseudoType == nsCSSPseudoElements::firstLetter) {
      pseudoRestriction = CSS_PROPERTY_APPLIES_TO_FIRST_LETTER;
    } else if (pseudoType == nsCSSPseudoElements::firstLine) {
      pseudoRestriction = CSS_PROPERTY_APPLIES_TO_FIRST_LINE;
    } else if (pseudoType == nsCSSPseudoElements::mozPlaceholder) {
      pseudoRestriction = CSS_PROPERTY_APPLIES_TO_PLACEHOLDER;
    }
  }
  return pseudoRestriction;
}

static void
UnsetPropertiesWithoutFlags(const nsStyleStructID aSID,
                            nsRuleData* aRuleData,
                            uint32_t aFlags)
{
  NS_ASSERTION(aFlags != 0, "aFlags must be nonzero");

  const uint32_t *flagData = gFlagsByStruct[aSID];

  
  MOZ_ASSERT(aRuleData->mValueOffsets[aSID] == 0,
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
    MOZ_ASSERT(size_t(aStorage) % NS_ALIGNMENT_OF(nsCSSValue) == 0,
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

 bool
nsRuleNode::ResolveVariableReferences(const nsStyleStructID aSID,
                                      nsRuleData* aRuleData,
                                      nsStyleContext* aContext)
{
  MOZ_ASSERT(aSID != eStyleStruct_Variables);
  MOZ_ASSERT(aRuleData->mSIDs & nsCachedStyleData::GetBitForSID(aSID));
  MOZ_ASSERT(aRuleData->mValueOffsets[aSID] == 0);

  nsCSSParser parser;
  bool anyTokenStreams = false;

  
  size_t nprops = nsCSSProps::PropertyCountInStruct(aSID);
  for (nsCSSValue* value = aRuleData->mValueStorage,
                  *values_end = aRuleData->mValueStorage + nprops;
       value != values_end; value++) {
    if (value->GetUnit() != eCSSUnit_TokenStream) {
      continue;
    }

    const CSSVariableValues* variables =
      &aContext->StyleVariables()->mVariables;
    nsCSSValueTokenStream* tokenStream = value->GetTokenStreamValue();

    
    
    
    
    
    
    

    
    parser.ParsePropertyWithVariableReferences(
        tokenStream->mPropertyID, tokenStream->mShorthandPropertyID,
        tokenStream->mTokenStream, variables, aRuleData,
        tokenStream->mSheetURI, tokenStream->mBaseURI,
        tokenStream->mSheetPrincipal, nullptr,
        tokenStream->mLineNumber, tokenStream->mLineOffset);
    aRuleData->mCanStoreInRuleTree = false;
    anyTokenStreams = true;
  }

  return anyTokenStreams;
}

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

  
  void* startStruct = nullptr;

  nsRuleNode* ruleNode = this;
  nsRuleNode* highestNode = nullptr; 
                                    
                                    
                                    
  nsRuleNode* rootNode = this; 
                               
                               
                               
                               
  RuleDetail detail = eRuleNone;
  uint32_t bit = nsCachedStyleData::GetBitForSID(aSID);

  while (ruleNode) {
    
    
    if (ruleNode->mNoneBits & bit)
      break;

    
    
    
    
    
    while ((ruleNode->mDependentBits & bit) && !ruleNode->IsUsedDirectly()) {
      NS_ASSERTION(ruleNode->mStyleData.GetStyleData(aSID) == nullptr,
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

  bool recomputeDetail = false;

  
  
  
  if (aSID != eStyleStruct_Variables) {
    
    
    
    
    recomputeDetail = ResolveVariableReferences(aSID, &ruleData, aContext);
  }

  
  
  
  uint32_t pseudoRestriction = GetPseudoRestriction(aContext);
  if (pseudoRestriction) {
    UnsetPropertiesWithoutFlags(aSID, &ruleData, pseudoRestriction);

    
    
    
    recomputeDetail = true;
  }

  if (recomputeDetail) {
    detail = CheckSpecifiedProperties(aSID, &ruleData);
  }

  NS_ASSERTION(!startStruct || (detail != eRuleFullReset &&
                                detail != eRuleFullMixed &&
                                detail != eRuleFullInherited),
               "can't have start struct and be fully specified");

  bool isReset = nsCachedStyleData::IsReset(aSID);
  if (!highestNode)
    highestNode = rootNode;

  if (!ruleData.mCanStoreInRuleTree)
    detail = eRulePartialMixed; 
                                

  if (detail == eRuleNone && startStruct) {
    
    
    
    
    
    PropagateDependentBit(aSID, ruleNode, startStruct);
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
      if (parentContext && parentContext != aContext->GetParent()) {
        PropagateGrandancestorBit(aContext, parentContext);
      }
    }
    if (parentContext) {
      
      
      
      
      const void* parentStruct = parentContext->StyleData(aSID);
      aContext->AddStyleBit(bit); 
      aContext->SetStyle(aSID, const_cast<void*>(parentStruct));
      return parentStruct;
    }
    else
      
      
      return SetDefaultOnRoot(aSID, aContext);
  }

  
  const void* res;
#define STYLE_STRUCT_TEST aSID
#define STYLE_STRUCT(name, checkdata_cb)                                      \
  res = Compute##name##Data(startStruct, &ruleData, aContext,                 \
                            highestNode, detail, ruleData.mCanStoreInRuleTree);
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
#undef STYLE_STRUCT_TEST

  
  return res;
}

const void*
nsRuleNode::SetDefaultOnRoot(const nsStyleStructID aSID, nsStyleContext* aContext)
{
  switch (aSID) {
    case eStyleStruct_Font:
    {
      nsStyleFont* fontData = new (mPresContext) nsStyleFont(mPresContext);
      nscoord minimumFontSize = mPresContext->MinFontSize(fontData->mLanguage);

      if (minimumFontSize > 0 && !mPresContext->IsChrome()) {
        fontData->mFont.size = std::max(fontData->mSize, minimumFontSize);
      }
      else {
        fontData->mFont.size = fontData->mSize;
      }
      aContext->SetStyle(eStyleStruct_Font, fontData);
      return fontData;
    }
    case eStyleStruct_Display:
    {
      nsStyleDisplay* disp = new (mPresContext) nsStyleDisplay();
      aContext->SetStyle(eStyleStruct_Display, disp);
      return disp;
    }
    case eStyleStruct_Visibility:
    {
      nsStyleVisibility* vis = new (mPresContext) nsStyleVisibility(mPresContext);
      aContext->SetStyle(eStyleStruct_Visibility, vis);
      return vis;
    }
    case eStyleStruct_Text:
    {
      nsStyleText* text = new (mPresContext) nsStyleText();
      aContext->SetStyle(eStyleStruct_Text, text);
      return text;
    }
    case eStyleStruct_TextReset:
    {
      nsStyleTextReset* text = new (mPresContext) nsStyleTextReset();
      aContext->SetStyle(eStyleStruct_TextReset, text);
      return text;
    }
    case eStyleStruct_Color:
    {
      nsStyleColor* color = new (mPresContext) nsStyleColor(mPresContext);
      aContext->SetStyle(eStyleStruct_Color, color);
      return color;
    }
    case eStyleStruct_Background:
    {
      nsStyleBackground* bg = new (mPresContext) nsStyleBackground();
      aContext->SetStyle(eStyleStruct_Background, bg);
      return bg;
    }
    case eStyleStruct_Margin:
    {
      nsStyleMargin* margin = new (mPresContext) nsStyleMargin();
      aContext->SetStyle(eStyleStruct_Margin, margin);
      return margin;
    }
    case eStyleStruct_Border:
    {
      nsStyleBorder* border = new (mPresContext) nsStyleBorder(mPresContext);
      aContext->SetStyle(eStyleStruct_Border, border);
      return border;
    }
    case eStyleStruct_Padding:
    {
      nsStylePadding* padding = new (mPresContext) nsStylePadding();
      aContext->SetStyle(eStyleStruct_Padding, padding);
      return padding;
    }
    case eStyleStruct_Outline:
    {
      nsStyleOutline* outline = new (mPresContext) nsStyleOutline(mPresContext);
      aContext->SetStyle(eStyleStruct_Outline, outline);
      return outline;
    }
    case eStyleStruct_List:
    {
      nsStyleList* list = new (mPresContext) nsStyleList(mPresContext);
      aContext->SetStyle(eStyleStruct_List, list);
      return list;
    }
    case eStyleStruct_Position:
    {
      nsStylePosition* pos = new (mPresContext) nsStylePosition();
      aContext->SetStyle(eStyleStruct_Position, pos);
      return pos;
    }
    case eStyleStruct_Table:
    {
      nsStyleTable* table = new (mPresContext) nsStyleTable();
      aContext->SetStyle(eStyleStruct_Table, table);
      return table;
    }
    case eStyleStruct_TableBorder:
    {
      nsStyleTableBorder* table = new (mPresContext) nsStyleTableBorder();
      aContext->SetStyle(eStyleStruct_TableBorder, table);
      return table;
    }
    case eStyleStruct_Content:
    {
      nsStyleContent* content = new (mPresContext) nsStyleContent();
      aContext->SetStyle(eStyleStruct_Content, content);
      return content;
    }
    case eStyleStruct_Quotes:
    {
      nsStyleQuotes* quotes = new (mPresContext) nsStyleQuotes();
      aContext->SetStyle(eStyleStruct_Quotes, quotes);
      return quotes;
    }
    case eStyleStruct_UserInterface:
    {
      nsStyleUserInterface* ui = new (mPresContext) nsStyleUserInterface();
      aContext->SetStyle(eStyleStruct_UserInterface, ui);
      return ui;
    }
    case eStyleStruct_UIReset:
    {
      nsStyleUIReset* ui = new (mPresContext) nsStyleUIReset();
      aContext->SetStyle(eStyleStruct_UIReset, ui);
      return ui;
    }
    case eStyleStruct_XUL:
    {
      nsStyleXUL* xul = new (mPresContext) nsStyleXUL();
      aContext->SetStyle(eStyleStruct_XUL, xul);
      return xul;
    }
    case eStyleStruct_Column:
    {
      nsStyleColumn* column = new (mPresContext) nsStyleColumn(mPresContext);
      aContext->SetStyle(eStyleStruct_Column, column);
      return column;
    }
    case eStyleStruct_SVG:
    {
      nsStyleSVG* svg = new (mPresContext) nsStyleSVG();
      aContext->SetStyle(eStyleStruct_SVG, svg);
      return svg;
    }
    case eStyleStruct_SVGReset:
    {
      nsStyleSVGReset* svgReset = new (mPresContext) nsStyleSVGReset();
      aContext->SetStyle(eStyleStruct_SVGReset, svgReset);
      return svgReset;
    }
    case eStyleStruct_Variables:
    {
      nsStyleVariables* vars = new (mPresContext) nsStyleVariables();
      aContext->SetStyle(eStyleStruct_Variables, vars);
      return vars;
    }
    default:
      



      MOZ_ASSERT(false, "unexpected SID");
      return nullptr;
  }
  return nullptr;
}











#define COMPUTE_START_INHERITED(type_, ctorargs_, data_, parentdata_)         \
  NS_ASSERTION(aRuleDetail != eRuleFullInherited,                             \
               "should not have bothered calling Compute*Data");              \
                                                                              \
  nsStyleContext* parentContext = aContext->GetParent();                      \
                                                                              \
  nsStyle##type_* data_ = nullptr;                                            \
  mozilla::Maybe<nsStyle##type_> maybeFakeParentData;                         \
  const nsStyle##type_* parentdata_ = nullptr;                                \
  bool canStoreInRuleTree = aCanStoreInRuleTree;                              \
                                                                              \
  /* If |canStoreInRuleTree| might be true by the time we're done, we */      \
  /* can't call parentContext->Style##type_() since it could recur into */    \
  /* setting the same struct on the same rule node, causing a leak. */        \
  if (aRuleDetail != eRuleFullReset &&                                        \
      (!aStartStruct || (aRuleDetail != eRulePartialReset &&                  \
                         aRuleDetail != eRuleNone))) {                        \
    if (parentContext) {                                                      \
      parentdata_ = parentContext->Style##type_();                            \
    } else {                                                                  \
      maybeFakeParentData.emplace ctorargs_;                                  \
      parentdata_ = maybeFakeParentData.ptr();                                \
    }                                                                         \
  }                                                                           \
  if (aStartStruct)                                                           \
    /* We only need to compute the delta between this computed data and */    \
    /* our computed data. */                                                  \
    data_ = new (mPresContext)                                                \
            nsStyle##type_(*static_cast<nsStyle##type_*>(aStartStruct));      \
  else {                                                                      \
    if (aRuleDetail != eRuleFullMixed && aRuleDetail != eRuleFullReset) {     \
      /* No question. We will have to inherit. Go ahead and init */           \
      /* with inherited vals from parent. */                                  \
      canStoreInRuleTree = false;                                             \
      if (parentdata_)                                                        \
        data_ = new (mPresContext) nsStyle##type_(*parentdata_);              \
      else                                                                    \
        data_ = new (mPresContext) nsStyle##type_ ctorargs_;                  \
    }                                                                         \
    else                                                                      \
      data_ = new (mPresContext) nsStyle##type_ ctorargs_;                    \
  }                                                                           \
                                                                              \
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
  /* If |canStoreInRuleTree| might be true by the time we're done, we */      \
  /* can't call parentContext->Style##type_() since it could recur into */    \
  /* setting the same struct on the same rule node, causing a leak. */        \
  mozilla::Maybe<nsStyle##type_> maybeFakeParentData;                         \
  const nsStyle##type_* parentdata_ = data_;                                  \
  if (aRuleDetail != eRuleFullReset &&                                        \
      aRuleDetail != eRulePartialReset &&                                     \
      aRuleDetail != eRuleNone) {                                             \
    if (parentContext) {                                                      \
      parentdata_ = parentContext->Style##type_();                            \
    } else {                                                                  \
      maybeFakeParentData.emplace ctorargs_;                                  \
      parentdata_ = maybeFakeParentData.ptr();                                \
    }                                                                         \
  }                                                                           \
  bool canStoreInRuleTree = aCanStoreInRuleTree;







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
    }                                                                         \
    NS_ASSERTION(!aHighestNode->mStyleData.mInheritedData->                   \
                   mStyleStructs[eStyleStruct_##type_],                       \
                 "Going to leak style data");                                 \
    aHighestNode->mStyleData.mInheritedData->                                 \
      mStyleStructs[eStyleStruct_##type_] = data_;                            \
    /* Propagate the bit down. */                                             \
    PropagateDependentBit(eStyleStruct_##type_, aHighestNode, data_);         \
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
    }                                                                         \
    NS_ASSERTION(!aHighestNode->mStyleData.mResetData->                       \
                   mStyleStructs[eStyleStruct_##type_],                       \
                 "Going to leak style data");                                 \
    aHighestNode->mStyleData.mResetData->                                     \
      mStyleStructs[eStyleStruct_##type_] = data_;                            \
    /* Propagate the bit down. */                                             \
    PropagateDependentBit(eStyleStruct_##type_, aHighestNode, data_);         \
  }                                                                           \
                                                                              \
  return data_;














static nscoord
ComputeScriptLevelSize(const nsStyleFont* aFont, const nsStyleFont* aParentFont,
                       nsPresContext* aPresContext, nscoord* aUnconstrainedSize)
{
  int32_t scriptLevelChange =
    aFont->mScriptLevel - aParentFont->mScriptLevel;
  if (scriptLevelChange == 0) {
    *aUnconstrainedSize = aParentFont->mScriptUnconstrainedSize;
    
    
    
    return aParentFont->mSize;
  }

  
  nscoord minScriptSize = aParentFont->mScriptMinSize;
  if (aFont->mAllowZoom) {
    minScriptSize = nsStyleFont::ZoomText(aPresContext, minScriptSize);
  }

  double scriptLevelScale =
    pow(aParentFont->mScriptSizeMultiplier, scriptLevelChange);
  
  
  *aUnconstrainedSize =
    NSToCoordRoundWithClamp(aParentFont->mScriptUnconstrainedSize*scriptLevelScale);
  
  nscoord scriptLevelSize =
    NSToCoordRoundWithClamp(aParentFont->mSize*scriptLevelScale);
  if (scriptLevelScale <= 1.0) {
    if (aParentFont->mSize <= minScriptSize) {
      
      
      
      return aParentFont->mSize;
    }
    
    return std::max(minScriptSize, scriptLevelSize);
  } else {
    
    NS_ASSERTION(*aUnconstrainedSize <= scriptLevelSize, "How can this ever happen?");
    
    return std::min(scriptLevelSize, std::max(*aUnconstrainedSize, minScriptSize));
  }
}


 nscoord
nsRuleNode::CalcFontPointSize(int32_t aHTMLSize, int32_t aBasePointSize,
                              nsPresContext* aPresContext,
                              nsFontSizeType aFontSizeType)
{
#define sFontSizeTableMin  9 
#define sFontSizeTableMax 16 







  static int32_t sStrictFontSizeTable[sFontSizeTableMax - sFontSizeTableMin + 1][8] =
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





















  static int32_t sQuirksFontSizeTable[sFontSizeTableMax - sFontSizeTableMin + 1][8] =
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

  static int32_t sFontSizeFactors[8] = { 60,75,89,100,120,150,200,300 };

  static int32_t sCSSColumns[7]  = {0, 1, 2, 3, 4, 5, 6}; 
  static int32_t sHTMLColumns[7] = {1, 2, 3, 4, 5, 6, 7}; 

  double dFontSize;

  if (aFontSizeType == eFontSize_HTML) {
    aHTMLSize--;    
  }

  if (aHTMLSize < 0)
    aHTMLSize = 0;
  else if (aHTMLSize > 6)
    aHTMLSize = 6;

  int32_t* column;
  switch (aFontSizeType)
  {
    case eFontSize_HTML: column = sHTMLColumns; break;
    case eFontSize_CSS:  column = sCSSColumns;  break;
  }

  
  int32_t fontSize = nsPresContext::AppUnitsToIntCSSPixels(aBasePointSize);

  if ((fontSize >= sFontSizeTableMin) && (fontSize <= sFontSizeTableMax))
  {
    int32_t row = fontSize - sFontSizeTableMin;

    if (aPresContext->CompatibilityMode() == eCompatibility_NavQuirks) {
      dFontSize = nsPresContext::CSSPixelsToAppUnits(sQuirksFontSizeTable[row][column[aHTMLSize]]);
    } else {
      dFontSize = nsPresContext::CSSPixelsToAppUnits(sStrictFontSizeTable[row][column[aHTMLSize]]);
    }
  }
  else
  {
    int32_t factor = sFontSizeFactors[column[aHTMLSize]];
    dFontSize = (factor * aBasePointSize) / 100;
  }


  if (1.0 < dFontSize) {
    return (nscoord)dFontSize;
  }
  return (nscoord)1;
}






 nscoord
nsRuleNode::FindNextSmallerFontSize(nscoord aFontSize, int32_t aBasePointSize, 
                                    nsPresContext* aPresContext,
                                    nsFontSizeType aFontSizeType)
{
  int32_t index;
  int32_t indexMin;
  int32_t indexMax;
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
  
  smallestIndexFontSize = CalcFontPointSize(indexMin, aBasePointSize, aPresContext, aFontSizeType);
  largestIndexFontSize = CalcFontPointSize(indexMax, aBasePointSize, aPresContext, aFontSizeType); 
  if (aFontSize > smallestIndexFontSize) {
    if (aFontSize < NSToCoordRound(float(largestIndexFontSize) * 1.5)) { 
      
      for (index = indexMax; index >= indexMin; index--) {
        indexFontSize = CalcFontPointSize(index, aBasePointSize, aPresContext, aFontSizeType);
        if (indexFontSize < aFontSize)
          break;
      } 
      
      if (indexFontSize == smallestIndexFontSize) {
        smallerIndexFontSize = indexFontSize - onePx;
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aPresContext, aFontSizeType);
      } else if (indexFontSize == largestIndexFontSize) {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aPresContext, aFontSizeType);
        largerIndexFontSize = NSToCoordRound(float(largestIndexFontSize) * 1.5);
      } else {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aPresContext, aFontSizeType);
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aPresContext, aFontSizeType);
      }
      
      relativePosition = float(aFontSize - indexFontSize) / float(largerIndexFontSize - indexFontSize);            
      
      smallerSize = smallerIndexFontSize + NSToCoordRound(relativePosition * (indexFontSize - smallerIndexFontSize));      
    }
    else {  
      smallerSize = NSToCoordRound(float(aFontSize) / 1.5);
    }
  }
  else { 
    smallerSize = std::max(aFontSize - onePx, onePx);
  }
  return smallerSize;
}





 nscoord
nsRuleNode::FindNextLargerFontSize(nscoord aFontSize, int32_t aBasePointSize, 
                                   nsPresContext* aPresContext,
                                   nsFontSizeType aFontSizeType)
{
  int32_t index;
  int32_t indexMin;
  int32_t indexMax;
  float relativePosition;
  nscoord adjustment;
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
  
  smallestIndexFontSize = CalcFontPointSize(indexMin, aBasePointSize, aPresContext, aFontSizeType);
  largestIndexFontSize = CalcFontPointSize(indexMax, aBasePointSize, aPresContext, aFontSizeType); 
  if (aFontSize > (smallestIndexFontSize - onePx)) {
    if (aFontSize < largestIndexFontSize) { 
      
      for (index = indexMin; index <= indexMax; index++) { 
        indexFontSize = CalcFontPointSize(index, aBasePointSize, aPresContext, aFontSizeType);
        if (indexFontSize > aFontSize)
          break;
      }
      
      if (indexFontSize == smallestIndexFontSize) {
        smallerIndexFontSize = indexFontSize - onePx;
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aPresContext, aFontSizeType);
      } else if (indexFontSize == largestIndexFontSize) {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aPresContext, aFontSizeType);
        largerIndexFontSize = NSCoordSaturatingMultiply(largestIndexFontSize, 1.5);
      } else {
        smallerIndexFontSize = CalcFontPointSize(index-1, aBasePointSize, aPresContext, aFontSizeType);
        largerIndexFontSize = CalcFontPointSize(index+1, aBasePointSize, aPresContext, aFontSizeType);
      }
      
      relativePosition = float(aFontSize - smallerIndexFontSize) / float(indexFontSize - smallerIndexFontSize);
      
      adjustment = NSCoordSaturatingNonnegativeMultiply(largerIndexFontSize - indexFontSize, relativePosition);
      largerSize = NSCoordSaturatingAdd(indexFontSize, adjustment);
    }
    else {  
      largerSize = NSCoordSaturatingMultiply(aFontSize, 1.5);
    }
  }
  else { 
    largerSize = NSCoordSaturatingAdd(aFontSize, onePx);
  }
  return largerSize;
}

struct SetFontSizeCalcOps : public css::BasicCoordCalcOps,
                            public css::NumbersAlreadyNormalizedOps
{
  
  const nscoord mParentSize;
  const nsStyleFont* const mParentFont;
  nsPresContext* const mPresContext;
  const bool mAtRoot;
  bool& mCanStoreInRuleTree;

  SetFontSizeCalcOps(nscoord aParentSize, const nsStyleFont* aParentFont,
                     nsPresContext* aPresContext, bool aAtRoot,
                     bool& aCanStoreInRuleTree)
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
      
      
      
      size = CalcLengthWith(aValue, mParentSize,
                            mParentFont,
                            nullptr, mPresContext, mAtRoot,
                            true, mCanStoreInRuleTree);
      if (!aValue.IsRelativeLengthUnit() && mParentFont->mAllowZoom) {
        size = nsStyleFont::ZoomText(mPresContext, size);
      }
    }
    else if (eCSSUnit_Percent == aValue.GetUnit()) {
      mCanStoreInRuleTree = false;
      
      
      
      
      size = NSCoordSaturatingMultiply(mParentSize, aValue.GetPercentValue());
    } else {
      MOZ_ASSERT(false, "unexpected value");
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
                        bool aUsedStartStruct,
                        bool aAtRoot,
                        bool& aCanStoreInRuleTree)
{
  
  
  bool sizeIsZoomedAccordingToParent = false;

  int32_t baseSize = (int32_t) aPresContext->
    GetDefaultFont(aFont->mGenericID, aFont->mLanguage)->size;
  const nsCSSValue* sizeValue = aRuleData->ValueForFontSize();
  if (eCSSUnit_Enumerated == sizeValue->GetUnit()) {
    int32_t value = sizeValue->GetIntValue();

    if ((NS_STYLE_FONT_SIZE_XXSMALL <= value) &&
        (value <= NS_STYLE_FONT_SIZE_XXLARGE)) {
      *aSize = CalcFontPointSize(value, baseSize,
                       aPresContext, eFontSize_CSS);
    }
    else if (NS_STYLE_FONT_SIZE_XXXLARGE == value) {
      
      *aSize = CalcFontPointSize(value, baseSize, aPresContext);
    }
    else if (NS_STYLE_FONT_SIZE_LARGER  == value ||
             NS_STYLE_FONT_SIZE_SMALLER == value) {
      aCanStoreInRuleTree = false;

      
      
      
      
      
      nscoord parentSize = aParentSize;
      if (aParentFont->mAllowZoom) {
        parentSize = nsStyleFont::UnZoomText(aPresContext, parentSize);
      }

      if (NS_STYLE_FONT_SIZE_LARGER == value) {
        *aSize = FindNextLargerFontSize(parentSize,
                         baseSize, aPresContext, eFontSize_CSS);

        NS_ASSERTION(*aSize >= parentSize,
                     "FindNextLargerFontSize failed");
      }
      else {
        *aSize = FindNextSmallerFontSize(parentSize,
                         baseSize, aPresContext, eFontSize_CSS);
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
    SetFontSizeCalcOps ops(aParentSize, aParentFont,
                           aPresContext, aAtRoot,
                           aCanStoreInRuleTree);
    *aSize = css::ComputeCalc(*sizeValue, ops);
    if (*aSize < 0) {
      MOZ_ASSERT(sizeValue->IsCalcUnit(),
                 "negative lengths and percents should be rejected by parser");
      *aSize = 0;
    }
    
    
    sizeIsZoomedAccordingToParent = true;
  }
  else if (eCSSUnit_System_Font == sizeValue->GetUnit()) {
    
    *aSize = aSystemFont.size;
  }
  else if (eCSSUnit_Inherit == sizeValue->GetUnit() ||
           eCSSUnit_Unset == sizeValue->GetUnit()) {
    aCanStoreInRuleTree = false;
    
    
    
    *aSize = aScriptLevelAdjustedParentSize;
    sizeIsZoomedAccordingToParent = true;
  }
  else if (eCSSUnit_Initial == sizeValue->GetUnit()) {
    
    
    *aSize = baseSize;
  } else {
    NS_ASSERTION(eCSSUnit_Null == sizeValue->GetUnit(),
                 "What kind of font-size value is this?");
    
    
    
    
    if (!aUsedStartStruct && aParentSize != aScriptLevelAdjustedParentSize) {
      
      
      
      aCanStoreInRuleTree = false;
      *aSize = aScriptLevelAdjustedParentSize;
      sizeIsZoomedAccordingToParent = true;
    } else {
      return;
    }
  }

  
  
  bool currentlyZoomed = sizeIsZoomedAccordingToParent &&
                         aParentFont->mAllowZoom;
  if (!currentlyZoomed && aFont->mAllowZoom) {
    *aSize = nsStyleFont::ZoomText(aPresContext, *aSize);
  } else if (currentlyZoomed && !aFont->mAllowZoom) {
    *aSize = nsStyleFont::UnZoomText(aPresContext, *aSize);
  }
}

static int8_t ClampTo8Bit(int32_t aValue) {
  if (aValue < -128)
    return -128;
  if (aValue > 127)
    return 127;
  return int8_t(aValue);
}

 void
nsRuleNode::SetFont(nsPresContext* aPresContext, nsStyleContext* aContext,
                    uint8_t aGenericFontID, const nsRuleData* aRuleData,
                    const nsStyleFont* aParentFont,
                    nsStyleFont* aFont, bool aUsedStartStruct,
                    bool& aCanStoreInRuleTree)
{
  bool atRoot = !aContext->GetParent();

  
  bool allowZoom;
  const nsCSSValue* textZoomValue = aRuleData->ValueForTextZoom();
  if (eCSSUnit_Null != textZoomValue->GetUnit()) {
    if (eCSSUnit_Inherit == textZoomValue->GetUnit()) {
      allowZoom = aParentFont->mAllowZoom;
    } else if (eCSSUnit_None == textZoomValue->GetUnit()) {
      allowZoom = false;
    } else {
      MOZ_ASSERT(eCSSUnit_Initial == textZoomValue->GetUnit(),
                 "unexpected unit");
      allowZoom = true;
    }
    aFont->EnableZoom(aPresContext, allowZoom);
  }

  
  
  
  
  
  
  const nsCSSValue* langValue = aRuleData->ValueForLang();
  if (eCSSUnit_Ident == langValue->GetUnit()) {
    nsAutoString lang;
    langValue->GetStringValue(lang);

    nsContentUtils::ASCIIToLower(lang);
    aFont->mLanguage = do_GetAtom(lang);
    aFont->mExplicitLanguage = true;
  }

  const nsFont* defaultVariableFont =
    aPresContext->GetDefaultFont(kPresContext_DefaultVariableFont_ID,
                                 aFont->mLanguage);

  
  static_assert(
    NS_STYLE_FONT_CAPTION        == LookAndFeel::eFont_Caption &&
    NS_STYLE_FONT_ICON           == LookAndFeel::eFont_Icon &&
    NS_STYLE_FONT_MENU           == LookAndFeel::eFont_Menu &&
    NS_STYLE_FONT_MESSAGE_BOX    == LookAndFeel::eFont_MessageBox &&
    NS_STYLE_FONT_SMALL_CAPTION  == LookAndFeel::eFont_SmallCaption &&
    NS_STYLE_FONT_STATUS_BAR     == LookAndFeel::eFont_StatusBar &&
    NS_STYLE_FONT_WINDOW         == LookAndFeel::eFont_Window &&
    NS_STYLE_FONT_DOCUMENT       == LookAndFeel::eFont_Document &&
    NS_STYLE_FONT_WORKSPACE      == LookAndFeel::eFont_Workspace &&
    NS_STYLE_FONT_DESKTOP        == LookAndFeel::eFont_Desktop &&
    NS_STYLE_FONT_INFO           == LookAndFeel::eFont_Info &&
    NS_STYLE_FONT_DIALOG         == LookAndFeel::eFont_Dialog &&
    NS_STYLE_FONT_BUTTON         == LookAndFeel::eFont_Button &&
    NS_STYLE_FONT_PULL_DOWN_MENU == LookAndFeel::eFont_PullDownMenu &&
    NS_STYLE_FONT_LIST           == LookAndFeel::eFont_List &&
    NS_STYLE_FONT_FIELD          == LookAndFeel::eFont_Field,
    "LookAndFeel.h system-font constants out of sync with nsStyleConsts.h");

  
  nsFont systemFont = *defaultVariableFont;
  const nsCSSValue* systemFontValue = aRuleData->ValueForSystemFont();
  if (eCSSUnit_Enumerated == systemFontValue->GetUnit()) {
    gfxFontStyle fontStyle;
    LookAndFeel::FontID fontID =
      (LookAndFeel::FontID)systemFontValue->GetIntValue();
    float devPerCSS =
      (float)nsPresContext::AppUnitsPerCSSPixel() /
      aPresContext->DeviceContext()->AppUnitsPerDevPixelAtUnitFullZoom();
    nsAutoString systemFontName;
    if (LookAndFeel::GetFont(fontID, systemFontName, fontStyle, devPerCSS)) {
      systemFontName.Trim("\"'");
      systemFont.fontlist = FontFamilyList(systemFontName, eUnquotedName);
      systemFont.fontlist.SetDefaultFontType(eFamily_none);
      systemFont.style = fontStyle.style;
      systemFont.systemFont = fontStyle.systemFont;
      systemFont.weight = fontStyle.weight;
      systemFont.stretch = fontStyle.stretch;
      systemFont.decorations = NS_FONT_DECORATION_NONE;
      systemFont.size =
        NSFloatPixelsToAppUnits(fontStyle.size,
                                aPresContext->DeviceContext()->
                                  AppUnitsPerDevPixelAtUnitFullZoom());
      
      systemFont.sizeAdjust = fontStyle.sizeAdjust;

#ifdef XP_WIN
      
      
      
      

      if (fontID == LookAndFeel::eFont_Field ||
          fontID == LookAndFeel::eFont_Button ||
          fontID == LookAndFeel::eFont_List) {
        
        
        
        
        
        
        
        
        
        
        
        systemFont.size =
          std::max(defaultVariableFont->size -
                 nsPresContext::CSSPointsToAppUnits(2), 0);
      }
#endif
    }
  }

  
  const nsCSSValue* familyValue = aRuleData->ValueForFontFamily();
  NS_ASSERTION(eCSSUnit_Enumerated != familyValue->GetUnit(),
               "system fonts should not be in mFamily anymore");
  if (eCSSUnit_FontFamilyList == familyValue->GetUnit()) {
    
    
    if (aGenericFontID == kGenericFont_NONE) {
      uint32_t len = defaultVariableFont->fontlist.Length();
      FontFamilyType generic = defaultVariableFont->fontlist.FirstGeneric();
      NS_ASSERTION(len == 1 &&
                   (generic == eFamily_serif || generic == eFamily_sans_serif),
                   "default variable font must be a single serif or sans-serif");
      if (len == 1 && generic != eFamily_none) {
        aFont->mFont.fontlist.SetDefaultFontType(generic);
      }
    } else {
      aFont->mFont.fontlist.SetDefaultFontType(eFamily_none);
    }
    aFont->mFont.systemFont = false;
    
    
    
    aFont->mGenericID = aGenericFontID;
  }
  else if (eCSSUnit_System_Font == familyValue->GetUnit()) {
    aFont->mFont.fontlist = systemFont.fontlist;
    aFont->mFont.systemFont = true;
    aFont->mGenericID = kGenericFont_NONE;
  }
  else if (eCSSUnit_Inherit == familyValue->GetUnit() ||
           eCSSUnit_Unset == familyValue->GetUnit()) {
    aCanStoreInRuleTree = false;
    aFont->mFont.fontlist = aParentFont->mFont.fontlist;
    aFont->mFont.systemFont = aParentFont->mFont.systemFont;
    aFont->mGenericID = aParentFont->mGenericID;
  }
  else if (eCSSUnit_Initial == familyValue->GetUnit()) {
    aFont->mFont.fontlist = defaultVariableFont->fontlist;
    aFont->mFont.systemFont = defaultVariableFont->systemFont;
    aFont->mGenericID = kGenericFont_NONE;
  }

  
  
  
  
  if (aGenericFontID != kGenericFont_NONE) {
    aFont->mGenericID = aGenericFontID;
  }

  
  SetDiscrete(*aRuleData->ValueForMathVariant(), aFont->mMathVariant,
              aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              aParentFont->mMathVariant, NS_MATHML_MATHVARIANT_NONE,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForMathDisplay(), aFont->mMathDisplay,
              aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              aParentFont->mMathDisplay, NS_MATHML_DISPLAYSTYLE_INLINE,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForOsxFontSmoothing(),
              aFont->mFont.smoothing, aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              aParentFont->mFont.smoothing,
              defaultVariableFont->smoothing,
              0, 0, 0, 0);

  
  if (aFont->mMathVariant != NS_MATHML_MATHVARIANT_NONE) {
    
    aFont->mFont.style = NS_FONT_STYLE_NORMAL;
  } else {
    SetDiscrete(*aRuleData->ValueForFontStyle(),
                aFont->mFont.style, aCanStoreInRuleTree,
                SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT | SETDSC_UNSET_INHERIT,
                aParentFont->mFont.style,
                defaultVariableFont->style,
                0, 0, 0, systemFont.style);
  }

  
  
  const nsCSSValue* weightValue = aRuleData->ValueForFontWeight();
  if (aFont->mMathVariant != NS_MATHML_MATHVARIANT_NONE) {
    
    aFont->mFont.weight = NS_FONT_WEIGHT_NORMAL;
  } else if (eCSSUnit_Enumerated == weightValue->GetUnit()) {
    int32_t value = weightValue->GetIntValue();
    switch (value) {
      case NS_STYLE_FONT_WEIGHT_NORMAL:
      case NS_STYLE_FONT_WEIGHT_BOLD:
        aFont->mFont.weight = value;
        break;
      case NS_STYLE_FONT_WEIGHT_BOLDER: {
        aCanStoreInRuleTree = false;
        int32_t inheritedValue = aParentFont->mFont.weight;
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
        aCanStoreInRuleTree = false;
        int32_t inheritedValue = aParentFont->mFont.weight;
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
                SETDSC_INTEGER | SETDSC_SYSTEM_FONT | SETDSC_UNSET_INHERIT,
                aParentFont->mFont.weight,
                defaultVariableFont->weight,
                0, 0, 0, systemFont.weight);

  
  SetDiscrete(*aRuleData->ValueForFontStretch(),
              aFont->mFont.stretch, aCanStoreInRuleTree,
              SETDSC_SYSTEM_FONT | SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              aParentFont->mFont.stretch,
              defaultVariableFont->stretch,
              0, 0, 0, systemFont.stretch);

  
  

  
  const nsCSSValue* scriptMinSizeValue = aRuleData->ValueForScriptMinSize();
  if (scriptMinSizeValue->IsLengthUnit()) {
    
    
    
    aFont->mScriptMinSize =
      CalcLengthWith(*scriptMinSizeValue, aParentFont->mSize,
                     aParentFont,
                     nullptr, aPresContext, atRoot, true,
                     aCanStoreInRuleTree);
  }

  
  SetFactor(*aRuleData->ValueForScriptSizeMultiplier(),
            aFont->mScriptSizeMultiplier,
            aCanStoreInRuleTree, aParentFont->mScriptSizeMultiplier,
            NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER,
            SETFCT_POSITIVE | SETFCT_UNSET_INHERIT);

  
  const nsCSSValue* scriptLevelValue = aRuleData->ValueForScriptLevel();
  if (eCSSUnit_Integer == scriptLevelValue->GetUnit()) {
    
    aCanStoreInRuleTree = false;
    aFont->mScriptLevel = ClampTo8Bit(aParentFont->mScriptLevel + scriptLevelValue->GetIntValue());
  }
  else if (eCSSUnit_Number == scriptLevelValue->GetUnit()) {
    
    aFont->mScriptLevel = ClampTo8Bit(int32_t(scriptLevelValue->GetFloatValue()));
  }
  else if (eCSSUnit_Auto == scriptLevelValue->GetUnit()) {
    
    aCanStoreInRuleTree = false;
    aFont->mScriptLevel = ClampTo8Bit(aParentFont->mScriptLevel +
                                      (aParentFont->mMathDisplay ==
                                       NS_MATHML_DISPLAYSTYLE_INLINE ? 1 : 0));
  }
  else if (eCSSUnit_Inherit == scriptLevelValue->GetUnit() ||
           eCSSUnit_Unset == scriptLevelValue->GetUnit()) {
    aCanStoreInRuleTree = false;
    aFont->mScriptLevel = aParentFont->mScriptLevel;
  }
  else if (eCSSUnit_Initial == scriptLevelValue->GetUnit()) {
    aFont->mScriptLevel = 0;
  }

  
  SetDiscrete(*aRuleData->ValueForFontKerning(),
              aFont->mFont.kerning, aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT | SETDSC_UNSET_INHERIT,
              aParentFont->mFont.kerning,
              defaultVariableFont->kerning,
              0, 0, 0, systemFont.kerning);

  
  SetDiscrete(*aRuleData->ValueForFontSynthesis(),
              aFont->mFont.synthesis, aCanStoreInRuleTree,
              SETDSC_NONE | SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT |
                SETDSC_UNSET_INHERIT,
              aParentFont->mFont.synthesis,
              defaultVariableFont->synthesis,
              0, 0, 0, systemFont.synthesis);

  
  
  const nsCSSValue* variantAlternatesValue =
    aRuleData->ValueForFontVariantAlternates();
  int32_t variantAlternates = 0;

  switch (variantAlternatesValue->GetUnit()) {
  case eCSSUnit_Inherit:
  case eCSSUnit_Unset:
    aFont->mFont.CopyAlternates(aParentFont->mFont);
    aCanStoreInRuleTree = false;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Normal:
    aFont->mFont.variantAlternates = 0;
    aFont->mFont.alternateValues.Clear();
    aFont->mFont.featureValueLookup = nullptr;
    break;

  case eCSSUnit_Pair:
    NS_ASSERTION(variantAlternatesValue->GetPairValue().mXValue.GetUnit() ==
                   eCSSUnit_Enumerated, "strange unit for variantAlternates");
    variantAlternates =
      variantAlternatesValue->GetPairValue().mXValue.GetIntValue();
    aFont->mFont.variantAlternates = variantAlternates;

    if (variantAlternates & NS_FONT_VARIANT_ALTERNATES_FUNCTIONAL_MASK) {
      
      aFont->mFont.featureValueLookup =
        aPresContext->StyleSet()->GetFontFeatureValuesLookup();

      NS_ASSERTION(variantAlternatesValue->GetPairValue().mYValue.GetUnit() ==
                   eCSSUnit_List, "function list not a list value");
      nsStyleUtil::ComputeFunctionalAlternates(
        variantAlternatesValue->GetPairValue().mYValue.GetListValue(),
        aFont->mFont.alternateValues);
    }
    break;

  default:
    break;
  }

  
  SetDiscrete(*aRuleData->ValueForFontVariantCaps(),
              aFont->mFont.variantCaps, aCanStoreInRuleTree,
              SETDSC_NORMAL | SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT |
                SETDSC_UNSET_INHERIT,
              aParentFont->mFont.variantCaps,
              defaultVariableFont->variantCaps,
              0, 0, 0, systemFont.variantCaps);

  
  
  SetDiscrete(*aRuleData->ValueForFontVariantEastAsian(),
              aFont->mFont.variantEastAsian, aCanStoreInRuleTree,
              SETDSC_NORMAL | SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT |
                SETDSC_UNSET_INHERIT,
              aParentFont->mFont.variantEastAsian,
              defaultVariableFont->variantEastAsian,
              0, 0, 0, systemFont.variantEastAsian);

  
  
  SetDiscrete(*aRuleData->ValueForFontVariantLigatures(),
              aFont->mFont.variantLigatures, aCanStoreInRuleTree,
              SETDSC_NORMAL | SETDSC_NONE | SETDSC_ENUMERATED |
                SETDSC_SYSTEM_FONT | SETDSC_UNSET_INHERIT,
              aParentFont->mFont.variantLigatures,
              defaultVariableFont->variantLigatures,
              0, NS_FONT_VARIANT_LIGATURES_NONE, 0, systemFont.variantLigatures);

  
  
  SetDiscrete(*aRuleData->ValueForFontVariantNumeric(),
              aFont->mFont.variantNumeric, aCanStoreInRuleTree,
              SETDSC_NORMAL | SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT |
                SETDSC_UNSET_INHERIT,
              aParentFont->mFont.variantNumeric,
              defaultVariableFont->variantNumeric,
              0, 0, 0, systemFont.variantNumeric);

  
  
  SetDiscrete(*aRuleData->ValueForFontVariantPosition(),
              aFont->mFont.variantPosition, aCanStoreInRuleTree,
              SETDSC_NORMAL | SETDSC_ENUMERATED | SETDSC_SYSTEM_FONT |
                SETDSC_UNSET_INHERIT,
              aParentFont->mFont.variantPosition,
              defaultVariableFont->variantPosition,
              0, 0, 0, systemFont.variantPosition);

  
  const nsCSSValue* featureSettingsValue =
    aRuleData->ValueForFontFeatureSettings();

  switch (featureSettingsValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Normal:
  case eCSSUnit_Initial:
    aFont->mFont.fontFeatureSettings.Clear();
    break;

  case eCSSUnit_Inherit:
  case eCSSUnit_Unset:
    aCanStoreInRuleTree = false;
    aFont->mFont.fontFeatureSettings = aParentFont->mFont.fontFeatureSettings;
    break;

  case eCSSUnit_System_Font:
    aFont->mFont.fontFeatureSettings = systemFont.fontFeatureSettings;
    break;

  case eCSSUnit_PairList:
  case eCSSUnit_PairListDep:
    ComputeFontFeatures(featureSettingsValue->GetPairListValue(),
                        aFont->mFont.fontFeatureSettings);
    break;

  default:
    MOZ_ASSERT(false, "unexpected value unit");
    break;
  }

  
  const nsCSSValue* languageOverrideValue =
    aRuleData->ValueForFontLanguageOverride();
  if (eCSSUnit_Inherit == languageOverrideValue->GetUnit() ||
      eCSSUnit_Unset == languageOverrideValue->GetUnit()) {
    aCanStoreInRuleTree = false;
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
  nscoord scriptLevelAdjustedUnconstrainedParentSize;
  scriptLevelAdjustedParentSize =
    ComputeScriptLevelSize(aFont, aParentFont, aPresContext,
                           &scriptLevelAdjustedUnconstrainedParentSize);
  NS_ASSERTION(!aUsedStartStruct || aFont->mScriptUnconstrainedSize == aFont->mSize,
               "If we have a start struct, we should have reset everything coming in here");
  SetFontSize(aPresContext, aRuleData, aFont, aParentFont,
              &aFont->mSize,
              systemFont, aParentFont->mSize, scriptLevelAdjustedParentSize,
              aUsedStartStruct, atRoot, aCanStoreInRuleTree);
  if (aParentFont->mSize == aParentFont->mScriptUnconstrainedSize &&
      scriptLevelAdjustedParentSize == scriptLevelAdjustedUnconstrainedParentSize) {
    
    
    
    
    
    aFont->mScriptUnconstrainedSize = aFont->mSize;
  } else {
    SetFontSize(aPresContext, aRuleData, aFont, aParentFont,
                &aFont->mScriptUnconstrainedSize,
                systemFont, aParentFont->mScriptUnconstrainedSize,
                scriptLevelAdjustedUnconstrainedParentSize,
                aUsedStartStruct, atRoot, aCanStoreInRuleTree);
  }
  NS_ASSERTION(aFont->mScriptUnconstrainedSize <= aFont->mSize,
               "scriptminsize should never be making things bigger");

  nscoord fontSize = aFont->mSize;

  
  
  if (fontSize > 0) {
    nscoord minFontSize = aPresContext->MinFontSize(aFont->mLanguage);
    if (minFontSize < 0) {
      minFontSize = 0;
    }
    if (fontSize < minFontSize && !aPresContext->IsChrome()) {
      
      fontSize = minFontSize;
    }
  }
  aFont->mFont.size = fontSize;

  
  const nsCSSValue* sizeAdjustValue = aRuleData->ValueForFontSizeAdjust();
  if (eCSSUnit_System_Font == sizeAdjustValue->GetUnit()) {
    aFont->mFont.sizeAdjust = systemFont.sizeAdjust;
  } else
    SetFactor(*sizeAdjustValue, aFont->mFont.sizeAdjust,
              aCanStoreInRuleTree, aParentFont->mFont.sizeAdjust, -1.0f,
              SETFCT_NONE | SETFCT_UNSET_INHERIT);
}

 void
nsRuleNode::ComputeFontFeatures(const nsCSSValuePairList *aFeaturesList,
                                nsTArray<gfxFontFeature>& aFeatureSettings)
{
  aFeatureSettings.Clear();
  for (const nsCSSValuePairList* p = aFeaturesList; p; p = p->mNext) {
    gfxFontFeature feat = {0, 0};

    MOZ_ASSERT(aFeaturesList->mXValue.GetUnit() == eCSSUnit_String,
               "unexpected value unit");

    
    nsAutoString tag;
    p->mXValue.GetStringValue(tag);
    if (tag.Length() != 4) {
      continue;
    }
    
    
    feat.mTag = (tag[0] << 24) | (tag[1] << 16) | (tag[2] << 8)  | tag[3];

    
    NS_ASSERTION(p->mYValue.GetUnit() == eCSSUnit_Integer,
                 "should have found an integer unit");
    feat.mValue = p->mYValue.GetIntValue();

    aFeatureSettings.AppendElement(feat);
  }
}







 void
nsRuleNode::SetGenericFont(nsPresContext* aPresContext,
                           nsStyleContext* aContext,
                           uint8_t aGenericFontID,
                           nsStyleFont* aFont)
{
  
  nsAutoTArray<nsStyleContext*, 8> contextPath;
  contextPath.AppendElement(aContext);
  nsStyleContext* higherContext = aContext->GetParent();
  while (higherContext) {
    if (higherContext->StyleFont()->mGenericID == aGenericFontID) {
      
      break;
    }
    contextPath.AppendElement(higherContext);
    higherContext = higherContext->GetParent();
  }

  

  
  
  
  const nsFont* defaultFont =
    aPresContext->GetDefaultFont(aGenericFontID, aFont->mLanguage);
  nsStyleFont parentFont(*defaultFont, aPresContext);
  if (higherContext) {
    const nsStyleFont* tmpFont = higherContext->StyleFont();
    parentFont = *tmpFont;
  }
  *aFont = parentFont;

  bool dummy;
  uint32_t fontBit = nsCachedStyleData::GetBitForSID(eStyleStruct_Font);

  
  
  
  size_t nprops = nsCSSProps::PropertyCountInStruct(eStyleStruct_Font);
  void* dataStorage = alloca(nprops * sizeof(nsCSSValue));

  for (int32_t i = contextPath.Length() - 1; i >= 0; --i) {
    nsStyleContext* context = contextPath[i];
    AutoCSSValueArray dataArray(dataStorage, nprops);

    nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Font), dataArray.get(),
                        aPresContext, context);
    ruleData.mValueOffsets[eStyleStruct_Font] = 0;

    
    
    
    
    for (nsRuleNode* ruleNode = context->RuleNode(); ruleNode;
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

    ResolveVariableReferences(eStyleStruct_Font, &ruleData, aContext);

    nsRuleNode::SetFont(aPresContext, context,
                        aGenericFontID, &ruleData, &parentFont, aFont,
                        false, dummy);

    parentFont = *aFont;
  }

  if (higherContext && contextPath.Length() > 1) {
    
    
    
    PropagateGrandancestorBit(aContext, higherContext);
  }
}

const void*
nsRuleNode::ComputeFontData(void* aStartStruct,
                            const nsRuleData* aRuleData,
                            nsStyleContext* aContext,
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Font, (mPresContext), font, parentFont)

  
  
  
  
  
  
  
  
  
  

  bool useDocumentFonts =
    mPresContext->GetCachedBoolPref(kPresContext_UseDocumentFonts);

  
  
  
  if (!useDocumentFonts && mPresContext->IsChrome()) {
    
    
    useDocumentFonts = true;
  }

  
  uint8_t generic = kGenericFont_NONE;
  
  
  const nsCSSValue* familyValue = aRuleData->ValueForFontFamily();
  if (eCSSUnit_FontFamilyList == familyValue->GetUnit()) {
    const FontFamilyList* fontlist = familyValue->GetFontFamilyListValue();
    FontFamilyList& fl = font->mFont.fontlist;
    fl = *fontlist;

    
    FontFamilyType fontType = fontlist->FirstGeneric();

    
    if (fontlist->Length() == 1) {
      switch (fontType) {
        case eFamily_serif:
          generic = kGenericFont_serif;
          break;
        case eFamily_sans_serif:
          generic = kGenericFont_sans_serif;
          break;
        case eFamily_monospace:
          generic = kGenericFont_monospace;
          break;
        case eFamily_cursive:
          generic = kGenericFont_cursive;
          break;
        case eFamily_fantasy:
          generic = kGenericFont_fantasy;
          break;
        case eFamily_moz_fixed:
          generic = kGenericFont_moz_fixed;
          break;
        default:
          break;
      }
    }

    
    
    if (!useDocumentFonts) {
      switch (fontType) {
        case eFamily_monospace:
          fl = FontFamilyList(eFamily_monospace);
          generic = kGenericFont_monospace;
          break;
        case eFamily_moz_fixed:
          fl = FontFamilyList(eFamily_moz_fixed);
          generic = kGenericFont_moz_fixed;
          break;
        default:
          fl.Clear();
          generic = kGenericFont_NONE;
          break;
      }
    }
  }

  
  if (generic == kGenericFont_NONE) {
    
    nsRuleNode::SetFont(mPresContext, aContext, generic,
                        aRuleData, parentFont, font,
                        aStartStruct != nullptr, canStoreInRuleTree);
  }
  else {
    
    canStoreInRuleTree = false;
    nsRuleNode::SetGenericFont(mPresContext, aContext, generic,
                               font);
  }

  COMPUTE_END_INHERITED(Font, font)
}

template <typename T>
inline uint32_t ListLength(const T* aList)
{
  uint32_t len = 0;
  while (aList) {
    len++;
    aList = aList->mNext;
  }
  return len;
}



already_AddRefed<nsCSSShadowArray>
nsRuleNode::GetShadowData(const nsCSSValueList* aList,
                          nsStyleContext* aContext,
                          bool aIsBoxShadow,
                          bool& aCanStoreInRuleTree)
{
  uint32_t arrayLength = ListLength(aList);

  MOZ_ASSERT(arrayLength > 0,
             "Non-null text-shadow list, yet we counted 0 items.");
  nsRefPtr<nsCSSShadowArray> shadowList =
    new(arrayLength) nsCSSShadowArray(arrayLength);

  if (!shadowList)
    return nullptr;

  nsStyleCoord tempCoord;
  DebugOnly<bool> unitOK;
  for (nsCSSShadowItem* item = shadowList->ShadowAt(0);
       aList;
       aList = aList->mNext, ++item) {
    MOZ_ASSERT(aList->mValue.GetUnit() == eCSSUnit_Array,
               "expecting a plain array value");
    nsCSSValue::Array *arr = aList->mValue.GetArrayValue();
    
    unitOK = SetCoord(arr->Item(0), tempCoord, nsStyleCoord(),
                      SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY,
                      aContext, mPresContext, aCanStoreInRuleTree);
    NS_ASSERTION(unitOK, "unexpected unit");
    item->mXOffset = tempCoord.GetCoordValue();

    unitOK = SetCoord(arr->Item(1), tempCoord, nsStyleCoord(),
                      SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY,
                      aContext, mPresContext, aCanStoreInRuleTree);
    NS_ASSERTION(unitOK, "unexpected unit");
    item->mYOffset = tempCoord.GetCoordValue();

    
    if (arr->Item(2).GetUnit() != eCSSUnit_Null) {
      unitOK = SetCoord(arr->Item(2), tempCoord, nsStyleCoord(),
                        SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY |
                          SETCOORD_CALC_CLAMP_NONNEGATIVE,
                        aContext, mPresContext, aCanStoreInRuleTree);
      NS_ASSERTION(unitOK, "unexpected unit");
      item->mRadius = tempCoord.GetCoordValue();
    } else {
      item->mRadius = 0;
    }

    
    if (aIsBoxShadow && arr->Item(3).GetUnit() != eCSSUnit_Null) {
      unitOK = SetCoord(arr->Item(3), tempCoord, nsStyleCoord(),
                        SETCOORD_LENGTH | SETCOORD_CALC_LENGTH_ONLY,
                        aContext, mPresContext, aCanStoreInRuleTree);
      NS_ASSERTION(unitOK, "unexpected unit");
      item->mSpread = tempCoord.GetCoordValue();
    } else {
      item->mSpread = 0;
    }

    if (arr->Item(4).GetUnit() != eCSSUnit_Null) {
      item->mHasColor = true;
      
      unitOK = SetColor(arr->Item(4), 0, mPresContext, aContext, item->mColor,
                        aCanStoreInRuleTree);
      NS_ASSERTION(unitOK, "unexpected unit");
    }

    if (aIsBoxShadow && arr->Item(5).GetUnit() == eCSSUnit_Enumerated) {
      NS_ASSERTION(arr->Item(5).GetIntValue() == NS_STYLE_BOX_SHADOW_INSET,
                   "invalid keyword type for box shadow");
      item->mInset = true;
    } else {
      item->mInset = false;
    }
  }

  return shadowList.forget();
}

const void*
nsRuleNode::ComputeTextData(void* aStartStruct,
                            const nsRuleData* aRuleData,
                            nsStyleContext* aContext,
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Text, (), text, parentText)

  
  SetDiscrete(*aRuleData->ValueForTabSize(),
              text->mTabSize, canStoreInRuleTree,
              SETDSC_INTEGER | SETDSC_UNSET_INHERIT, parentText->mTabSize,
              NS_STYLE_TABSIZE_INITIAL, 0, 0, 0, 0);

  
  SetCoord(*aRuleData->ValueForLetterSpacing(),
           text->mLetterSpacing, parentText->mLetterSpacing,
           SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL |
             SETCOORD_CALC_LENGTH_ONLY | SETCOORD_UNSET_INHERIT,
           aContext, mPresContext, canStoreInRuleTree);

  
  const nsCSSValue* textShadowValue = aRuleData->ValueForTextShadow();
  if (textShadowValue->GetUnit() != eCSSUnit_Null) {
    text->mTextShadow = nullptr;

    
    
    if (textShadowValue->GetUnit() == eCSSUnit_Inherit ||
        textShadowValue->GetUnit() == eCSSUnit_Unset) {
      canStoreInRuleTree = false;
      text->mTextShadow = parentText->mTextShadow;
    } else if (textShadowValue->GetUnit() == eCSSUnit_List ||
               textShadowValue->GetUnit() == eCSSUnit_ListDep) {
      
      text->mTextShadow = GetShadowData(textShadowValue->GetListValue(),
                                        aContext, false, canStoreInRuleTree);
    }
  }

  
  const nsCSSValue* lineHeightValue = aRuleData->ValueForLineHeight();
  if (eCSSUnit_Percent == lineHeightValue->GetUnit()) {
    canStoreInRuleTree = false;
    
    text->mLineHeight.SetCoordValue(
        NSToCoordRound(float(aContext->StyleFont()->mFont.size) *
                       lineHeightValue->GetPercentValue()));
  }
  else if (eCSSUnit_Initial == lineHeightValue->GetUnit() ||
           eCSSUnit_System_Font == lineHeightValue->GetUnit()) {
    text->mLineHeight.SetNormalValue();
  }
  else {
    SetCoord(*lineHeightValue, text->mLineHeight, parentText->mLineHeight,
             SETCOORD_LEH | SETCOORD_FACTOR | SETCOORD_NORMAL |
               SETCOORD_UNSET_INHERIT,
             aContext, mPresContext, canStoreInRuleTree);
    if (lineHeightValue->IsLengthUnit() &&
        !lineHeightValue->IsRelativeLengthUnit()) {
      nscoord lh = nsStyleFont::ZoomText(mPresContext,
                                         text->mLineHeight.GetCoordValue());

      canStoreInRuleTree = false;
      const nsStyleFont *font = aContext->StyleFont();
      nscoord minimumFontSize = mPresContext->MinFontSize(font->mLanguage);

      if (minimumFontSize > 0 && !mPresContext->IsChrome()) {
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
  text->mTextAlignTrue = false;
  if (eCSSUnit_String == textAlignValue->GetUnit()) {
    NS_NOTYETIMPLEMENTED("align string");
  } else if (eCSSUnit_Enumerated == textAlignValue->GetUnit() &&
             NS_STYLE_TEXT_ALIGN_MOZ_CENTER_OR_INHERIT ==
               textAlignValue->GetIntValue()) {
    canStoreInRuleTree = false;
    uint8_t parentAlign = parentText->mTextAlign;
    text->mTextAlign = (NS_STYLE_TEXT_ALIGN_DEFAULT == parentAlign) ?
      NS_STYLE_TEXT_ALIGN_CENTER : parentAlign;
  } else {
    if (eCSSUnit_Pair == textAlignValue->GetUnit()) {
      
      text->mTextAlignTrue = true;
      const nsCSSValuePair& textAlignValuePair = textAlignValue->GetPairValue();
      textAlignValue = &textAlignValuePair.mXValue;
      if (eCSSUnit_Enumerated == textAlignValue->GetUnit()) {
        if (textAlignValue->GetIntValue() == NS_STYLE_TEXT_ALIGN_TRUE) {
          textAlignValue = &textAlignValuePair.mYValue;
        }
      } else if (eCSSUnit_String == textAlignValue->GetUnit()) {
        NS_NOTYETIMPLEMENTED("align string");
      }
    } else if (eCSSUnit_Inherit == textAlignValue->GetUnit() ||
               eCSSUnit_Unset == textAlignValue->GetUnit()) {
      text->mTextAlignTrue = parentText->mTextAlignTrue;
    }
    SetDiscrete(*textAlignValue, text->mTextAlign, canStoreInRuleTree,
                SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
                parentText->mTextAlign,
                NS_STYLE_TEXT_ALIGN_DEFAULT, 0, 0, 0, 0);
  }

  
  const nsCSSValue* textAlignLastValue = aRuleData->ValueForTextAlignLast();
  text->mTextAlignLastTrue = false;
  if (eCSSUnit_Pair == textAlignLastValue->GetUnit()) {
    
    text->mTextAlignLastTrue = true;
    const nsCSSValuePair& textAlignLastValuePair = textAlignLastValue->GetPairValue();
    textAlignLastValue = &textAlignLastValuePair.mXValue;
    if (eCSSUnit_Enumerated == textAlignLastValue->GetUnit()) {
      if (textAlignLastValue->GetIntValue() == NS_STYLE_TEXT_ALIGN_TRUE) {
        textAlignLastValue = &textAlignLastValuePair.mYValue;
      }
    }
  } else if (eCSSUnit_Inherit == textAlignLastValue->GetUnit() ||
             eCSSUnit_Unset == textAlignLastValue->GetUnit()) {
    text->mTextAlignLastTrue = parentText->mTextAlignLastTrue;
  }
  SetDiscrete(*textAlignLastValue, text->mTextAlignLast,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mTextAlignLast,
              NS_STYLE_TEXT_ALIGN_AUTO, 0, 0, 0, 0);

  
  SetCoord(*aRuleData->ValueForTextIndent(), text->mTextIndent, parentText->mTextIndent,
           SETCOORD_LPH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INHERIT,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetDiscrete(*aRuleData->ValueForTextTransform(), text->mTextTransform, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mTextTransform,
              NS_STYLE_TEXT_TRANSFORM_NONE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForWhiteSpace(), text->mWhiteSpace, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mWhiteSpace,
              NS_STYLE_WHITESPACE_NORMAL, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForWordBreak(), text->mWordBreak, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mWordBreak,
              NS_STYLE_WORDBREAK_NORMAL, 0, 0, 0, 0);

  
  nsStyleCoord tempCoord;
  const nsCSSValue* wordSpacingValue = aRuleData->ValueForWordSpacing();
  if (SetCoord(*wordSpacingValue, tempCoord,
               nsStyleCoord(parentText->mWordSpacing,
                            nsStyleCoord::CoordConstructor),
               SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL |
                 SETCOORD_CALC_LENGTH_ONLY | SETCOORD_UNSET_INHERIT,
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
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mWordWrap,
              NS_STYLE_WORDWRAP_NORMAL, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForHyphens(), text->mHyphens, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mHyphens,
              NS_STYLE_HYPHENS_MANUAL, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForRubyAlign(),
              text->mRubyAlign, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mRubyAlign,
              NS_STYLE_RUBY_ALIGN_SPACE_AROUND, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForRubyPosition(),
              text->mRubyPosition, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mRubyPosition,
              NS_STYLE_RUBY_POSITION_OVER, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForTextSizeAdjust(), text->mTextSizeAdjust,
              canStoreInRuleTree,
              SETDSC_NONE | SETDSC_AUTO | SETDSC_UNSET_INHERIT,
              parentText->mTextSizeAdjust,
              NS_STYLE_TEXT_SIZE_ADJUST_AUTO, 
              NS_STYLE_TEXT_SIZE_ADJUST_AUTO, 
              NS_STYLE_TEXT_SIZE_ADJUST_NONE, 
              0, 0);

  
  SetDiscrete(*aRuleData->ValueForTextCombineUpright(),
              text->mTextCombineUpright,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mTextCombineUpright,
              NS_STYLE_TEXT_COMBINE_UPRIGHT_NONE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForControlCharacterVisibility(),
              text->mControlCharacterVisibility,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentText->mControlCharacterVisibility,
              NS_STYLE_CONTROL_CHARACTER_VISIBILITY_HIDDEN, 0, 0, 0, 0);

  COMPUTE_END_INHERITED(Text, text)
}

const void*
nsRuleNode::ComputeTextResetData(void* aStartStruct,
                                 const nsRuleData* aRuleData,
                                 nsStyleContext* aContext,
                                 nsRuleNode* aHighestNode,
                                 const RuleDetail aRuleDetail,
                                 const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(TextReset, (), text, parentText)

  
  const nsCSSValue* verticalAlignValue = aRuleData->ValueForVerticalAlign();
  if (!SetCoord(*verticalAlignValue, text->mVerticalAlign,
                parentText->mVerticalAlign,
                SETCOORD_LPH | SETCOORD_ENUMERATED | SETCOORD_STORE_CALC,
                aContext, mPresContext, canStoreInRuleTree)) {
    if (eCSSUnit_Initial == verticalAlignValue->GetUnit() ||
        eCSSUnit_Unset == verticalAlignValue->GetUnit()) {
      text->mVerticalAlign.SetIntValue(NS_STYLE_VERTICAL_ALIGN_BASELINE,
                                       eStyleUnit_Enumerated);
    }
  }

  
  const nsCSSValue* decorationLineValue =
    aRuleData->ValueForTextDecorationLine();
  if (eCSSUnit_Enumerated == decorationLineValue->GetUnit()) {
    int32_t td = decorationLineValue->GetIntValue();
    text->mTextDecorationLine = td;
    if (td & NS_STYLE_TEXT_DECORATION_LINE_PREF_ANCHORS) {
      bool underlineLinks =
        mPresContext->GetCachedBoolPref(kPresContext_UnderlineLinks);
      if (underlineLinks) {
        text->mTextDecorationLine |= NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE;
      }
      else {
        text->mTextDecorationLine &= ~NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE;
      }
    }
  } else if (eCSSUnit_Inherit == decorationLineValue->GetUnit()) {
    canStoreInRuleTree = false;
    text->mTextDecorationLine = parentText->mTextDecorationLine;
  } else if (eCSSUnit_Initial == decorationLineValue->GetUnit() ||
             eCSSUnit_Unset == decorationLineValue->GetUnit()) {
    text->mTextDecorationLine = NS_STYLE_TEXT_DECORATION_LINE_NONE;
  }

  
  const nsCSSValue* decorationColorValue =
    aRuleData->ValueForTextDecorationColor();
  nscolor decorationColor;
  if (eCSSUnit_Inherit == decorationColorValue->GetUnit()) {
    canStoreInRuleTree = false;
    if (parentContext) {
      bool isForeground;
      parentText->GetDecorationColor(decorationColor, isForeground);
      if (isForeground) {
        text->SetDecorationColor(parentContext->StyleColor()->mColor);
      } else {
        text->SetDecorationColor(decorationColor);
      }
    } else {
      text->SetDecorationColorToForeground();
    }
  }
  else if (eCSSUnit_EnumColor == decorationColorValue->GetUnit() &&
           decorationColorValue->GetIntValue() == NS_COLOR_CURRENTCOLOR) {
    text->SetDecorationColorToForeground();
  }
  else if (SetColor(*decorationColorValue, 0, mPresContext, aContext,
                    decorationColor, canStoreInRuleTree)) {
    text->SetDecorationColor(decorationColor);
  }
  else if (eCSSUnit_Initial == decorationColorValue->GetUnit() ||
           eCSSUnit_Unset == decorationColorValue->GetUnit() ||
           eCSSUnit_Enumerated == decorationColorValue->GetUnit()) {
    MOZ_ASSERT(eCSSUnit_Enumerated != decorationColorValue->GetUnit() ||
               decorationColorValue->GetIntValue() ==
                 NS_STYLE_COLOR_MOZ_USE_TEXT_COLOR,
               "unexpected enumerated value");
    text->SetDecorationColorToForeground();
  }

  
  const nsCSSValue* decorationStyleValue =
    aRuleData->ValueForTextDecorationStyle();
  if (eCSSUnit_Enumerated == decorationStyleValue->GetUnit()) {
    text->SetDecorationStyle(decorationStyleValue->GetIntValue());
  } else if (eCSSUnit_Inherit == decorationStyleValue->GetUnit()) {
    text->SetDecorationStyle(parentText->GetDecorationStyle());
    canStoreInRuleTree = false;
  } else if (eCSSUnit_Initial == decorationStyleValue->GetUnit() ||
             eCSSUnit_Unset == decorationStyleValue->GetUnit()) {
    text->SetDecorationStyle(NS_STYLE_TEXT_DECORATION_STYLE_SOLID);
  }

  
  const nsCSSValue* textOverflowValue =
    aRuleData->ValueForTextOverflow();
  if (eCSSUnit_Initial == textOverflowValue->GetUnit() ||
      eCSSUnit_Unset == textOverflowValue->GetUnit()) {
    text->mTextOverflow = nsStyleTextOverflow();
  } else if (eCSSUnit_Inherit == textOverflowValue->GetUnit()) {
    canStoreInRuleTree = false;
    text->mTextOverflow = parentText->mTextOverflow;
  } else if (eCSSUnit_Enumerated == textOverflowValue->GetUnit()) {
    
    SetDiscrete(*textOverflowValue, text->mTextOverflow.mRight.mType,
                canStoreInRuleTree,
                SETDSC_ENUMERATED, parentText->mTextOverflow.mRight.mType,
                NS_STYLE_TEXT_OVERFLOW_CLIP, 0, 0, 0, 0);
    text->mTextOverflow.mRight.mString.Truncate();
    text->mTextOverflow.mLeft.mType = NS_STYLE_TEXT_OVERFLOW_CLIP;
    text->mTextOverflow.mLeft.mString.Truncate();
    text->mTextOverflow.mLogicalDirections = true;
  } else if (eCSSUnit_String == textOverflowValue->GetUnit()) {
    
    text->mTextOverflow.mRight.mType = NS_STYLE_TEXT_OVERFLOW_STRING;
    textOverflowValue->GetStringValue(text->mTextOverflow.mRight.mString);
    text->mTextOverflow.mLeft.mType = NS_STYLE_TEXT_OVERFLOW_CLIP;
    text->mTextOverflow.mLeft.mString.Truncate();
    text->mTextOverflow.mLogicalDirections = true;
  } else if (eCSSUnit_Pair == textOverflowValue->GetUnit()) {
    
    text->mTextOverflow.mLogicalDirections = false;
    const nsCSSValuePair& textOverflowValuePair =
      textOverflowValue->GetPairValue();

    const nsCSSValue *textOverflowLeftValue = &textOverflowValuePair.mXValue;
    if (eCSSUnit_Enumerated == textOverflowLeftValue->GetUnit()) {
      SetDiscrete(*textOverflowLeftValue, text->mTextOverflow.mLeft.mType,
                  canStoreInRuleTree,
                  SETDSC_ENUMERATED, parentText->mTextOverflow.mLeft.mType,
                  NS_STYLE_TEXT_OVERFLOW_CLIP, 0, 0, 0, 0);
      text->mTextOverflow.mLeft.mString.Truncate();
    } else if (eCSSUnit_String == textOverflowLeftValue->GetUnit()) {
      textOverflowLeftValue->GetStringValue(text->mTextOverflow.mLeft.mString);
      text->mTextOverflow.mLeft.mType = NS_STYLE_TEXT_OVERFLOW_STRING;
    }

    const nsCSSValue *textOverflowRightValue = &textOverflowValuePair.mYValue;
    if (eCSSUnit_Enumerated == textOverflowRightValue->GetUnit()) {
      SetDiscrete(*textOverflowRightValue, text->mTextOverflow.mRight.mType,
                  canStoreInRuleTree,
                  SETDSC_ENUMERATED, parentText->mTextOverflow.mRight.mType,
                  NS_STYLE_TEXT_OVERFLOW_CLIP, 0, 0, 0, 0);
      text->mTextOverflow.mRight.mString.Truncate();
    } else if (eCSSUnit_String == textOverflowRightValue->GetUnit()) {
      textOverflowRightValue->GetStringValue(text->mTextOverflow.mRight.mString);
      text->mTextOverflow.mRight.mType = NS_STYLE_TEXT_OVERFLOW_STRING;
    }
  }

  
  SetDiscrete(*aRuleData->ValueForUnicodeBidi(), text->mUnicodeBidi, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentText->mUnicodeBidi,
              NS_STYLE_UNICODE_BIDI_NORMAL, 0, 0, 0, 0);

  COMPUTE_END_RESET(TextReset, text)
}

const void*
nsRuleNode::ComputeUserInterfaceData(void* aStartStruct,
                                     const nsRuleData* aRuleData,
                                     nsStyleContext* aContext,
                                     nsRuleNode* aHighestNode,
                                     const RuleDetail aRuleDetail,
                                     const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(UserInterface, (), ui, parentUI)

  
  const nsCSSValue* cursorValue = aRuleData->ValueForCursor();
  nsCSSUnit cursorUnit = cursorValue->GetUnit();
  if (cursorUnit != eCSSUnit_Null) {
    delete [] ui->mCursorArray;
    ui->mCursorArray = nullptr;
    ui->mCursorArrayLength = 0;

    if (cursorUnit == eCSSUnit_Inherit ||
        cursorUnit == eCSSUnit_Unset) {
      canStoreInRuleTree = false;
      ui->mCursor = parentUI->mCursor;
      ui->CopyCursorArrayFrom(*parentUI);
    }
    else if (cursorUnit == eCSSUnit_Initial) {
      ui->mCursor = NS_STYLE_CURSOR_AUTO;
    }
    else {
      
      
      MOZ_ASSERT(cursorUnit == eCSSUnit_List || cursorUnit == eCSSUnit_ListDep,
                 "unrecognized cursor unit");
      const nsCSSValueList* list = cursorValue->GetListValue();
      const nsCSSValueList* list2 = list;
      nsIDocument* doc = aContext->PresContext()->Document();
      uint32_t arrayLength = 0;
      for ( ; list->mValue.GetUnit() == eCSSUnit_Array; list = list->mNext)
        if (list->mValue.GetArrayValue()->Item(0).GetImageValue(doc))
          ++arrayLength;

      if (arrayLength != 0) {
        ui->mCursorArray = new nsCursorImage[arrayLength];
        if (ui->mCursorArray) {
          ui->mCursorArrayLength = arrayLength;

          for (nsCursorImage *item = ui->mCursorArray;
               list2->mValue.GetUnit() == eCSSUnit_Array;
               list2 = list2->mNext) {
            nsCSSValue::Array *arr = list2->mValue.GetArrayValue();
            imgIRequest *req = arr->Item(0).GetImageValue(doc);
            if (req) {
              item->SetImage(req);
              if (arr->Item(1).GetUnit() != eCSSUnit_Null) {
                item->mHaveHotspot = true;
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
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentUI->mUserInput,
              NS_STYLE_USER_INPUT_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForUserModify(),
              ui->mUserModify, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentUI->mUserModify,
              NS_STYLE_USER_MODIFY_READ_ONLY,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForUserFocus(),
              ui->mUserFocus, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentUI->mUserFocus,
              NS_STYLE_USER_FOCUS_NONE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForWindowDragging(),
              ui->mWindowDragging, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentUI->mWindowDragging,
              NS_STYLE_WINDOW_DRAGGING_NO_DRAG, 0, 0, 0, 0);

  COMPUTE_END_INHERITED(UserInterface, ui)
}

const void*
nsRuleNode::ComputeUIResetData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(UIReset, (), ui, parentUI)

  
  SetDiscrete(*aRuleData->ValueForUserSelect(),
              ui->mUserSelect, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentUI->mUserSelect,
              NS_STYLE_USER_SELECT_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForImeMode(),
              ui->mIMEMode, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentUI->mIMEMode,
              NS_STYLE_IME_MODE_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForForceBrokenImageIcon(),
              ui->mForceBrokenImageIcon,
              canStoreInRuleTree,
              SETDSC_INTEGER | SETDSC_UNSET_INITIAL,
              parentUI->mForceBrokenImageIcon,
              0, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForWindowShadow(),
              ui->mWindowShadow, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentUI->mWindowShadow,
              NS_STYLE_WINDOW_SHADOW_DEFAULT, 0, 0, 0, 0);

  COMPUTE_END_RESET(UIReset, ui)
}



struct TransitionPropInfo {
  nsCSSProperty property;
  
  uint32_t nsStyleDisplay::* sdCount;
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



struct TransitionPropData {
  const nsCSSValueList *list;
  nsCSSUnit unit;
  uint32_t num;
};

static uint32_t
CountTransitionProps(const TransitionPropInfo* aInfo,
                     TransitionPropData* aData,
                     size_t aLength,
                     nsStyleDisplay* aDisplay,
                     const nsStyleDisplay* aParentDisplay,
                     const nsRuleData* aRuleData,
                     bool& aCanStoreInRuleTree)
{
  
  
  
  
  
  
  
  

  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  

  uint32_t numTransitions = 0;
  for (size_t i = 0; i < aLength; ++i) {
    const TransitionPropInfo& info = aInfo[i];
    TransitionPropData& data = aData[i];

    
    

    const nsCSSValue& value = *aRuleData->ValueFor(info.property);
    data.unit = value.GetUnit();
    data.list = (value.GetUnit() == eCSSUnit_List ||
                 value.GetUnit() == eCSSUnit_ListDep)
                  ? value.GetListValue() : nullptr;

    
    
    
    
    
    
    
    
    
    


    
    if (data.unit == eCSSUnit_Inherit) {
      data.num = aParentDisplay->*(info.sdCount);
      aCanStoreInRuleTree = false;
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
                               const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Display, (), display, parentDisplay)

  
  
  
  
  
  display->mDisplay = display->mOriginalDisplay;
  display->mFloats = display->mOriginalFloats;

  
  
  TransitionPropData transitionPropData[4];
  TransitionPropData& delay = transitionPropData[0];
  TransitionPropData& duration = transitionPropData[1];
  TransitionPropData& property = transitionPropData[2];
  TransitionPropData& timingFunction = transitionPropData[3];

#define FOR_ALL_TRANSITION_PROPS(var_) \
                                      for (uint32_t var_ = 0; var_ < 4; ++var_)

  
  uint32_t numTransitions =
    CountTransitionProps(transitionPropInfo, transitionPropData,
                         ArrayLength(transitionPropData),
                         display, parentDisplay, aRuleData,
                         canStoreInRuleTree);

  display->mTransitions.SetLength(numTransitions);

  FOR_ALL_TRANSITION_PROPS(p) {
    const TransitionPropInfo& i = transitionPropInfo[p];
    TransitionPropData& d = transitionPropData[p];

    display->*(i.sdCount) = d.num;
  }

  
  for (uint32_t i = 0; i < numTransitions; ++i) {
    StyleTransition *transition = &display->mTransitions[i];

    if (i >= delay.num) {
      transition->SetDelay(display->mTransitions[i % delay.num].GetDelay());
    } else if (delay.unit == eCSSUnit_Inherit) {
      
      
      
      MOZ_ASSERT(i < parentDisplay->mTransitionDelayCount,
                 "delay.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      transition->SetDelay(parentDisplay->mTransitions[i].GetDelay());
    } else if (delay.unit == eCSSUnit_Initial ||
               delay.unit == eCSSUnit_Unset) {
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
      MOZ_ASSERT(i < parentDisplay->mTransitionDurationCount,
                 "duration.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      transition->SetDuration(parentDisplay->mTransitions[i].GetDuration());
    } else if (duration.unit == eCSSUnit_Initial ||
               duration.unit == eCSSUnit_Unset) {
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
      MOZ_ASSERT(i < parentDisplay->mTransitionPropertyCount,
                 "property.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      transition->CopyPropertyFrom(parentDisplay->mTransitions[i]);
    } else if (property.unit == eCSSUnit_Initial ||
               property.unit == eCSSUnit_Unset) {
      transition->SetProperty(eCSSPropertyExtra_all_properties);
    } else if (property.unit == eCSSUnit_None) {
      transition->SetProperty(eCSSPropertyExtra_no_properties);
    } else if (property.list) {
      const nsCSSValue &val = property.list->mValue;

      if (val.GetUnit() == eCSSUnit_Ident) {
        nsDependentString
          propertyStr(property.list->mValue.GetStringBufferValue());
        nsCSSProperty prop =
          nsCSSProps::LookupProperty(propertyStr,
                                     nsCSSProps::eEnabledForAllContent);
        if (prop == eCSSProperty_UNKNOWN) {
          transition->SetUnknownProperty(propertyStr);
        } else {
          transition->SetProperty(prop);
        }
      } else {
        MOZ_ASSERT(val.GetUnit() == eCSSUnit_All,
                   "Invalid transition property unit");
        transition->SetProperty(eCSSPropertyExtra_all_properties);
      }
    }

    if (i >= timingFunction.num) {
      transition->SetTimingFunction(
        display->mTransitions[i % timingFunction.num].GetTimingFunction());
    } else if (timingFunction.unit == eCSSUnit_Inherit) {
      MOZ_ASSERT(i < parentDisplay->mTransitionTimingFunctionCount,
                 "timingFunction.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      transition->SetTimingFunction(
        parentDisplay->mTransitions[i].GetTimingFunction());
    } else if (timingFunction.unit == eCSSUnit_Initial ||
               timingFunction.unit == eCSSUnit_Unset) {
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
    for (uint32_t var_ = 0; var_ < 8; ++var_)

  

  uint32_t numAnimations =
    CountTransitionProps(animationPropInfo, animationPropData,
                         ArrayLength(animationPropData),
                         display, parentDisplay, aRuleData,
                         canStoreInRuleTree);

  display->mAnimations.SetLength(numAnimations);

  FOR_ALL_ANIMATION_PROPS(p) {
    const TransitionPropInfo& i = animationPropInfo[p];
    TransitionPropData& d = animationPropData[p];

    display->*(i.sdCount) = d.num;
  }

  
  for (uint32_t i = 0; i < numAnimations; ++i) {
    StyleAnimation *animation = &display->mAnimations[i];

    if (i >= animDelay.num) {
      animation->SetDelay(display->mAnimations[i % animDelay.num].GetDelay());
    } else if (animDelay.unit == eCSSUnit_Inherit) {
      
      
      
      MOZ_ASSERT(i < parentDisplay->mAnimationDelayCount,
                 "animDelay.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetDelay(parentDisplay->mAnimations[i].GetDelay());
    } else if (animDelay.unit == eCSSUnit_Initial ||
               animDelay.unit == eCSSUnit_Unset) {
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
      MOZ_ASSERT(i < parentDisplay->mAnimationDurationCount,
                 "animDuration.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetDuration(parentDisplay->mAnimations[i].GetDuration());
    } else if (animDuration.unit == eCSSUnit_Initial ||
               animDuration.unit == eCSSUnit_Unset) {
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
      MOZ_ASSERT(i < parentDisplay->mAnimationNameCount,
                 "animName.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetName(parentDisplay->mAnimations[i].GetName());
    } else if (animName.unit == eCSSUnit_Initial ||
               animName.unit == eCSSUnit_Unset) {
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
          MOZ_ASSERT(false, "Invalid animation-name unit");
      }
    }

    if (i >= animTimingFunction.num) {
      animation->SetTimingFunction(
        display->mAnimations[i % animTimingFunction.num].GetTimingFunction());
    } else if (animTimingFunction.unit == eCSSUnit_Inherit) {
      MOZ_ASSERT(i < parentDisplay->mAnimationTimingFunctionCount,
                 "animTimingFunction.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetTimingFunction(
        parentDisplay->mAnimations[i].GetTimingFunction());
    } else if (animTimingFunction.unit == eCSSUnit_Initial ||
               animTimingFunction.unit == eCSSUnit_Unset) {
      animation->SetTimingFunction(
        nsTimingFunction(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE));
    } else if (animTimingFunction.list) {
      ComputeTimingFunction(animTimingFunction.list->mValue,
                            animation->TimingFunctionSlot());
    }

    if (i >= animDirection.num) {
      animation->SetDirection(display->mAnimations[i % animDirection.num].GetDirection());
    } else if (animDirection.unit == eCSSUnit_Inherit) {
      MOZ_ASSERT(i < parentDisplay->mAnimationDirectionCount,
                 "animDirection.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetDirection(parentDisplay->mAnimations[i].GetDirection());
    } else if (animDirection.unit == eCSSUnit_Initial ||
               animDirection.unit == eCSSUnit_Unset) {
      animation->SetDirection(NS_STYLE_ANIMATION_DIRECTION_NORMAL);
    } else if (animDirection.list) {
      MOZ_ASSERT(animDirection.list->mValue.GetUnit() == eCSSUnit_Enumerated,
                 "Invalid animation-direction unit");

      animation->SetDirection(animDirection.list->mValue.GetIntValue());
    }

    if (i >= animFillMode.num) {
      animation->SetFillMode(display->mAnimations[i % animFillMode.num].GetFillMode());
    } else if (animFillMode.unit == eCSSUnit_Inherit) {
      MOZ_ASSERT(i < parentDisplay->mAnimationFillModeCount,
                 "animFillMode.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetFillMode(parentDisplay->mAnimations[i].GetFillMode());
    } else if (animFillMode.unit == eCSSUnit_Initial ||
               animFillMode.unit == eCSSUnit_Unset) {
      animation->SetFillMode(NS_STYLE_ANIMATION_FILL_MODE_NONE);
    } else if (animFillMode.list) {
      MOZ_ASSERT(animFillMode.list->mValue.GetUnit() == eCSSUnit_Enumerated,
                 "Invalid animation-fill-mode unit");

      animation->SetFillMode(animFillMode.list->mValue.GetIntValue());
    }

    if (i >= animPlayState.num) {
      animation->SetPlayState(display->mAnimations[i % animPlayState.num].GetPlayState());
    } else if (animPlayState.unit == eCSSUnit_Inherit) {
      MOZ_ASSERT(i < parentDisplay->mAnimationPlayStateCount,
                 "animPlayState.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetPlayState(parentDisplay->mAnimations[i].GetPlayState());
    } else if (animPlayState.unit == eCSSUnit_Initial ||
               animPlayState.unit == eCSSUnit_Unset) {
      animation->SetPlayState(NS_STYLE_ANIMATION_PLAY_STATE_RUNNING);
    } else if (animPlayState.list) {
      MOZ_ASSERT(animPlayState.list->mValue.GetUnit() == eCSSUnit_Enumerated,
                 "Invalid animation-play-state unit");

      animation->SetPlayState(animPlayState.list->mValue.GetIntValue());
    }

    if (i >= animIterationCount.num) {
      animation->SetIterationCount(display->mAnimations[i % animIterationCount.num].GetIterationCount());
    } else if (animIterationCount.unit == eCSSUnit_Inherit) {
      MOZ_ASSERT(i < parentDisplay->mAnimationIterationCountCount,
                 "animIterationCount.num computed incorrectly");
      MOZ_ASSERT(!canStoreInRuleTree,
                 "should have made canStoreInRuleTree false above");
      animation->SetIterationCount(parentDisplay->mAnimations[i].GetIterationCount());
    } else if (animIterationCount.unit == eCSSUnit_Initial ||
               animIterationCount.unit == eCSSUnit_Unset) {
      animation->SetIterationCount(1.0f);
    } else if (animIterationCount.list) {
      switch (animIterationCount.list->mValue.GetUnit()) {
        case eCSSUnit_Enumerated:
          MOZ_ASSERT(animIterationCount.list->mValue.GetIntValue() ==
                       NS_STYLE_ANIMATION_ITERATION_COUNT_INFINITE,
                     "unexpected value");
          animation->SetIterationCount(NS_IEEEPositiveInfinity());
          break;
        case eCSSUnit_Number:
          animation->SetIterationCount(
            animIterationCount.list->mValue.GetFloatValue());
          break;
        default:
          MOZ_ASSERT(false,
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

  
  SetFactor(*aRuleData->ValueForOpacity(), display->mOpacity, canStoreInRuleTree,
            parentDisplay->mOpacity, 1.0f,
            SETFCT_OPACITY | SETFCT_UNSET_INITIAL);

  
  SetDiscrete(*aRuleData->ValueForDisplay(), display->mDisplay, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mDisplay,
              NS_STYLE_DISPLAY_INLINE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForMixBlendMode(), display->mMixBlendMode,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mMixBlendMode, NS_STYLE_BLEND_NORMAL,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForScrollBehavior(), display->mScrollBehavior,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mScrollBehavior, NS_STYLE_SCROLL_BEHAVIOR_AUTO,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForScrollSnapTypeX(), display->mScrollSnapTypeX,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mScrollSnapTypeX, NS_STYLE_SCROLL_SNAP_TYPE_NONE,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForScrollSnapTypeY(), display->mScrollSnapTypeY,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mScrollSnapTypeY, NS_STYLE_SCROLL_SNAP_TYPE_NONE,
              0, 0, 0, 0);

  
  const nsCSSValue& scrollSnapPointsX = *aRuleData->ValueForScrollSnapPointsX();
  switch (scrollSnapPointsX.GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
    case eCSSUnit_None:
      display->mScrollSnapPointsX.SetNoneValue();
      break;
    case eCSSUnit_Inherit:
      display->mScrollSnapPointsX = parentDisplay->mScrollSnapPointsX;
      canStoreInRuleTree = false;
      break;
    case eCSSUnit_Function: {
      nsCSSValue::Array* func = scrollSnapPointsX.GetArrayValue();
      NS_ASSERTION(func->Item(0).GetKeywordValue() == eCSSKeyword_repeat,
                   "Expected repeat(), got another function name");
      nsStyleCoord coord;
      if (SetCoord(func->Item(1), coord, nsStyleCoord(),
                   SETCOORD_LP | SETCOORD_STORE_CALC |
                   SETCOORD_CALC_CLAMP_NONNEGATIVE,
                   aContext, mPresContext, canStoreInRuleTree)) {
        NS_ASSERTION(coord.GetUnit() == eStyleUnit_Coord ||
                     coord.GetUnit() == eStyleUnit_Percent ||
                     coord.GetUnit() == eStyleUnit_Calc,
                     "unexpected unit");
        display->mScrollSnapPointsX = coord;
      }
      break;
    }
    default:
      NS_NOTREACHED("unexpected unit");
  }

  
  const nsCSSValue& scrollSnapPointsY = *aRuleData->ValueForScrollSnapPointsY();
  switch (scrollSnapPointsY.GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
    case eCSSUnit_None:
      display->mScrollSnapPointsY.SetNoneValue();
      break;
    case eCSSUnit_Inherit:
      display->mScrollSnapPointsY = parentDisplay->mScrollSnapPointsY;
      canStoreInRuleTree = false;
      break;
    case eCSSUnit_Function: {
      nsCSSValue::Array* func = scrollSnapPointsY.GetArrayValue();
      NS_ASSERTION(func->Item(0).GetKeywordValue() == eCSSKeyword_repeat,
                   "Expected repeat(), got another function name");
      nsStyleCoord coord;
      if (SetCoord(func->Item(1), coord, nsStyleCoord(),
                   SETCOORD_LP | SETCOORD_STORE_CALC |
                   SETCOORD_CALC_CLAMP_NONNEGATIVE,
                   aContext, mPresContext, canStoreInRuleTree)) {
        NS_ASSERTION(coord.GetUnit() == eStyleUnit_Coord ||
                     coord.GetUnit() == eStyleUnit_Percent ||
                     coord.GetUnit() == eStyleUnit_Calc,
                     "unexpected unit");
        display->mScrollSnapPointsY = coord;
      }
      break;
    }
    default:
      NS_NOTREACHED("unexpected unit");
  }

  
  const nsCSSValue& snapDestination = *aRuleData->ValueForScrollSnapDestination();
  switch (snapDestination.GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
      display->mScrollSnapDestination.SetInitialZeroValues();
      break;
    case eCSSUnit_Inherit:
      display->mScrollSnapDestination = parentDisplay->mScrollSnapDestination;
      canStoreInRuleTree = false;
      break;
    default: {
        ComputePositionValue(aContext, snapDestination,
                             display->mScrollSnapDestination, canStoreInRuleTree);
      }
  }

  
  typedef nsStyleBackground::Position Position;

  const nsCSSValue& snapCoordinate = *aRuleData->ValueForScrollSnapCoordinate();
  switch (snapCoordinate.GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
    case eCSSUnit_None:
      
      display->mScrollSnapCoordinate.Clear();
      break;
    case eCSSUnit_Inherit:
      display->mScrollSnapCoordinate = parentDisplay->mScrollSnapCoordinate;
      canStoreInRuleTree = false;
      break;
    case eCSSUnit_List: {
      display->mScrollSnapCoordinate.Clear();
      const nsCSSValueList* item = snapCoordinate.GetListValue();
      do {
        NS_ASSERTION(item->mValue.GetUnit() != eCSSUnit_Null &&
                     item->mValue.GetUnit() != eCSSUnit_Inherit &&
                     item->mValue.GetUnit() != eCSSUnit_Initial &&
                     item->mValue.GetUnit() != eCSSUnit_Unset,
                     "unexpected unit");
        Position* pos = display->mScrollSnapCoordinate.AppendElement();
        ComputePositionValue(aContext, item->mValue, *pos, canStoreInRuleTree);
        item = item->mNext;
      } while(item);
      break;
    }
    default:
      NS_NOTREACHED("unexpected unit");
  }

  
  SetDiscrete(*aRuleData->ValueForIsolation(), display->mIsolation,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mIsolation, NS_STYLE_ISOLATION_AUTO,
              0, 0, 0, 0);

  
  
  
  display->mOriginalDisplay = display->mDisplay;

  
  SetDiscrete(*aRuleData->ValueForAppearance(),
              display->mAppearance, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mAppearance,
              NS_THEME_NONE, 0, 0, 0, 0);

  
  const nsCSSValue* bindingValue = aRuleData->ValueForBinding();
  if (eCSSUnit_URL == bindingValue->GetUnit()) {
    mozilla::css::URLValue* url = bindingValue->GetURLStructValue();
    NS_ASSERTION(url, "What's going on here?");

    if (MOZ_LIKELY(url->GetURI())) {
      display->mBinding = url;
    } else {
      display->mBinding = nullptr;
    }
  }
  else if (eCSSUnit_None == bindingValue->GetUnit() ||
           eCSSUnit_Initial == bindingValue->GetUnit() ||
           eCSSUnit_Unset == bindingValue->GetUnit()) {
    display->mBinding = nullptr;
  }
  else if (eCSSUnit_Inherit == bindingValue->GetUnit()) {
    canStoreInRuleTree = false;
    display->mBinding = parentDisplay->mBinding;
  }

  
  SetDiscrete(*aRuleData->ValueForPosition(), display->mPosition, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mPosition,
              NS_STYLE_POSITION_STATIC, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForClear(), display->mBreakType, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mBreakType,
              NS_STYLE_CLEAR_NONE, 0, 0, 0, 0);

  
  
  
  
  
  const nsCSSValue* breakBeforeValue = aRuleData->ValueForPageBreakBefore();
  if (eCSSUnit_Enumerated == breakBeforeValue->GetUnit()) {
    display->mBreakBefore =
      (NS_STYLE_PAGE_BREAK_AVOID != breakBeforeValue->GetIntValue() &&
       NS_STYLE_PAGE_BREAK_AUTO  != breakBeforeValue->GetIntValue());
  }
  else if (eCSSUnit_Initial == breakBeforeValue->GetUnit() ||
           eCSSUnit_Unset == breakBeforeValue->GetUnit()) {
    display->mBreakBefore = false;
  }
  else if (eCSSUnit_Inherit == breakBeforeValue->GetUnit()) {
    canStoreInRuleTree = false;
    display->mBreakBefore = parentDisplay->mBreakBefore;
  }

  const nsCSSValue* breakAfterValue = aRuleData->ValueForPageBreakAfter();
  if (eCSSUnit_Enumerated == breakAfterValue->GetUnit()) {
    display->mBreakAfter =
      (NS_STYLE_PAGE_BREAK_AVOID != breakAfterValue->GetIntValue() &&
       NS_STYLE_PAGE_BREAK_AUTO  != breakAfterValue->GetIntValue());
  }
  else if (eCSSUnit_Initial == breakAfterValue->GetUnit() ||
           eCSSUnit_Unset == breakAfterValue->GetUnit()) {
    display->mBreakAfter = false;
  }
  else if (eCSSUnit_Inherit == breakAfterValue->GetUnit()) {
    canStoreInRuleTree = false;
    display->mBreakAfter = parentDisplay->mBreakAfter;
  }
  

  
  SetDiscrete(*aRuleData->ValueForPageBreakInside(),
              display->mBreakInside, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mBreakInside,
              NS_STYLE_PAGE_BREAK_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForTouchAction(), display->mTouchAction,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO | SETDSC_NONE |
                SETDSC_UNSET_INITIAL,
              parentDisplay->mTouchAction,
              NS_STYLE_TOUCH_ACTION_AUTO,
              NS_STYLE_TOUCH_ACTION_AUTO,
              NS_STYLE_TOUCH_ACTION_NONE, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForFloat(),
              display->mFloats, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mFloats,
              NS_STYLE_FLOAT_NONE, 0, 0, 0, 0);
  
  display->mOriginalFloats = display->mFloats;

  
  SetDiscrete(*aRuleData->ValueForOverflowX(),
              display->mOverflowX, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mOverflowX,
              NS_STYLE_OVERFLOW_VISIBLE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForOverflowY(),
              display->mOverflowY, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mOverflowY,
              NS_STYLE_OVERFLOW_VISIBLE, 0, 0, 0, 0);

  
  
  
  if (display->mOverflowX != display->mOverflowY &&
      (display->mOverflowX == NS_STYLE_OVERFLOW_VISIBLE ||
       display->mOverflowX == NS_STYLE_OVERFLOW_CLIP ||
       display->mOverflowY == NS_STYLE_OVERFLOW_VISIBLE ||
       display->mOverflowY == NS_STYLE_OVERFLOW_CLIP)) {
    
    
    canStoreInRuleTree = false;

    
    
    if (display->mOverflowX == NS_STYLE_OVERFLOW_CLIP)
      display->mOverflowX = NS_STYLE_OVERFLOW_HIDDEN;
    if (display->mOverflowY == NS_STYLE_OVERFLOW_CLIP)
      display->mOverflowY = NS_STYLE_OVERFLOW_HIDDEN;

    
    
    if (display->mOverflowX == NS_STYLE_OVERFLOW_VISIBLE)
      display->mOverflowX = NS_STYLE_OVERFLOW_AUTO;
    if (display->mOverflowY == NS_STYLE_OVERFLOW_VISIBLE)
      display->mOverflowY = NS_STYLE_OVERFLOW_AUTO;
  }

  SetDiscrete(*aRuleData->ValueForOverflowClipBox(), display->mOverflowClipBox,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mOverflowClipBox,
              NS_STYLE_OVERFLOW_CLIP_BOX_PADDING_BOX, 0, 0, 0, 0);

  SetDiscrete(*aRuleData->ValueForResize(), display->mResize, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mResize,
              NS_STYLE_RESIZE_NONE, 0, 0, 0, 0);

  
  const nsCSSValue* clipValue = aRuleData->ValueForClip();
  switch (clipValue->GetUnit()) {
  case eCSSUnit_Inherit:
    canStoreInRuleTree = false;
    display->mClipFlags = parentDisplay->mClipFlags;
    display->mClip = parentDisplay->mClip;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
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
    MOZ_ASSERT(false, "unrecognized clip unit");
  }

  if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
    
    
    

    nsIAtom* pseudo = aContext->GetPseudo();
    if (pseudo && display->mDisplay == NS_STYLE_DISPLAY_CONTENTS) {
      
      
      
      
      
      
      
      
      
      display->mOriginalDisplay = display->mDisplay = NS_STYLE_DISPLAY_INLINE;
      canStoreInRuleTree = false;
    }

    if (nsCSSPseudoElements::firstLetter == pseudo) {
      
      
      
      display->mOriginalDisplay = display->mDisplay = NS_STYLE_DISPLAY_INLINE;

      
      
      
      canStoreInRuleTree = false;
    }

    if (display->IsAbsolutelyPositionedStyle()) {
      
      
      EnsureBlockDisplay(display->mDisplay);
      display->mFloats = NS_STYLE_FLOAT_NONE;

      
      
      
      
      
      
    } else if (display->mFloats != NS_STYLE_FLOAT_NONE) {
      
      
      EnsureBlockDisplay(display->mDisplay);

      
      
      
      
      
    }

  }

  
  const nsCSSValue* transformValue = aRuleData->ValueForTransform();
  switch (transformValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
  case eCSSUnit_None:
    display->mSpecifiedTransform = nullptr;
    break;

  case eCSSUnit_Inherit:
    display->mSpecifiedTransform = parentDisplay->mSpecifiedTransform;
    canStoreInRuleTree = false;
    break;

  case eCSSUnit_SharedList: {
    nsCSSValueSharedList* list = transformValue->GetSharedListValue();
    nsCSSValueList* head = list->mHead;
    MOZ_ASSERT(head, "transform list must have at least one item");
    
    if (head->mValue.GetUnit() == eCSSUnit_None) {
      MOZ_ASSERT(head->mNext == nullptr, "none must be alone");
      display->mSpecifiedTransform = nullptr;
    } else {
      display->mSpecifiedTransform = list;
    }
    break;
  }

  default:
    MOZ_ASSERT(false, "unrecognized transform unit");
  }

  
  const nsCSSValue* willChangeValue = aRuleData->ValueForWillChange();
  switch (willChangeValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep: {
    display->mWillChange.Clear();
    display->mWillChangeBitField = 0;
    for (const nsCSSValueList* item = willChangeValue->GetListValue();
         item; item = item->mNext)
    {
      if (item->mValue.UnitHasStringValue()) {
        nsAutoString buffer;
        item->mValue.GetStringValue(buffer);
        display->mWillChange.AppendElement(buffer);

        if (buffer.EqualsLiteral("transform")) {
          display->mWillChangeBitField |= NS_STYLE_WILL_CHANGE_TRANSFORM;
        }
        if (buffer.EqualsLiteral("opacity")) {
          display->mWillChangeBitField |= NS_STYLE_WILL_CHANGE_OPACITY;
        }
        if (buffer.EqualsLiteral("scroll-position")) {
          display->mWillChangeBitField |= NS_STYLE_WILL_CHANGE_SCROLL;
        }

        nsCSSProperty prop =
          nsCSSProps::LookupProperty(buffer,
                                     nsCSSProps::eEnabledForAllContent);
        if (prop != eCSSProperty_UNKNOWN &&
            nsCSSProps::PropHasFlags(prop,
                                     CSS_PROPERTY_CREATES_STACKING_CONTEXT))
        {
          display->mWillChangeBitField |= NS_STYLE_WILL_CHANGE_STACKING_CONTEXT;
        }
      }
    }
    break;
  }

  case eCSSUnit_Inherit:
    display->mWillChange = parentDisplay->mWillChange;
    display->mWillChangeBitField = parentDisplay->mWillChangeBitField;
    canStoreInRuleTree = false;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
  case eCSSUnit_Auto:
    display->mWillChange.Clear();
    display->mWillChangeBitField = 0;
    break;

  default:
    MOZ_ASSERT(false, "unrecognized will-change unit");
  }

  
  const nsCSSValue* transformOriginValue =
    aRuleData->ValueForTransformOrigin();
  if (transformOriginValue->GetUnit() != eCSSUnit_Null) {
    const nsCSSValue& valX =
      transformOriginValue->GetUnit() == eCSSUnit_Triplet ?
        transformOriginValue->GetTripletValue().mXValue : *transformOriginValue;
    const nsCSSValue& valY =
      transformOriginValue->GetUnit() == eCSSUnit_Triplet ?
        transformOriginValue->GetTripletValue().mYValue : *transformOriginValue;
    const nsCSSValue& valZ =
      transformOriginValue->GetUnit() == eCSSUnit_Triplet ?
        transformOriginValue->GetTripletValue().mZValue : *transformOriginValue;

    mozilla::DebugOnly<bool> cX =
       SetCoord(valX, display->mTransformOrigin[0],
                parentDisplay->mTransformOrigin[0],
                SETCOORD_LPH | SETCOORD_INITIAL_HALF |
                  SETCOORD_BOX_POSITION | SETCOORD_STORE_CALC |
                  SETCOORD_UNSET_INITIAL,
                aContext, mPresContext, canStoreInRuleTree);

     mozilla::DebugOnly<bool> cY =
       SetCoord(valY, display->mTransformOrigin[1],
                parentDisplay->mTransformOrigin[1],
                SETCOORD_LPH | SETCOORD_INITIAL_HALF |
                  SETCOORD_BOX_POSITION | SETCOORD_STORE_CALC |
                  SETCOORD_UNSET_INITIAL,
                aContext, mPresContext, canStoreInRuleTree);

     if (valZ.GetUnit() == eCSSUnit_Null) {
       
       
       
       display->mTransformOrigin[2].SetCoordValue(0);
     } else {
       mozilla::DebugOnly<bool> cZ =
         SetCoord(valZ, display->mTransformOrigin[2],
                  parentDisplay->mTransformOrigin[2],
                  SETCOORD_LH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC |
                    SETCOORD_UNSET_INITIAL,
                  aContext, mPresContext, canStoreInRuleTree);
       MOZ_ASSERT(cY == cZ, "changed one but not the other");
     }
     MOZ_ASSERT(cX == cY, "changed one but not the other");
     NS_ASSERTION(cX, "Malformed -moz-transform-origin parse!");
  }

  const nsCSSValue* perspectiveOriginValue =
    aRuleData->ValueForPerspectiveOrigin();
  if (perspectiveOriginValue->GetUnit() != eCSSUnit_Null) {
    mozilla::DebugOnly<bool> result =
      SetPairCoords(*perspectiveOriginValue,
                    display->mPerspectiveOrigin[0],
                    display->mPerspectiveOrigin[1],
                    parentDisplay->mPerspectiveOrigin[0],
                    parentDisplay->mPerspectiveOrigin[1],
                    SETCOORD_LPH | SETCOORD_INITIAL_HALF |
                      SETCOORD_BOX_POSITION | SETCOORD_STORE_CALC |
                      SETCOORD_UNSET_INITIAL,
                    aContext, mPresContext, canStoreInRuleTree);
    NS_ASSERTION(result, "Malformed -moz-perspective-origin parse!");
  }

  SetCoord(*aRuleData->ValueForPerspective(), 
           display->mChildPerspective, parentDisplay->mChildPerspective,
           SETCOORD_LAH | SETCOORD_INITIAL_NONE | SETCOORD_NONE |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);

  SetDiscrete(*aRuleData->ValueForBackfaceVisibility(),
              display->mBackfaceVisibility, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mBackfaceVisibility,
              NS_STYLE_BACKFACE_VISIBILITY_VISIBLE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForTransformStyle(),
              display->mTransformStyle, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mTransformStyle,
              NS_STYLE_TRANSFORM_STYLE_FLAT, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForOrient(),
              display->mOrient, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentDisplay->mOrient,
              NS_STYLE_ORIENT_INLINE, 0, 0, 0, 0);

  COMPUTE_END_RESET(Display, display)
}

const void*
nsRuleNode::ComputeVisibilityData(void* aStartStruct,
                                  const nsRuleData* aRuleData,
                                  nsStyleContext* aContext,
                                  nsRuleNode* aHighestNode,
                                  const RuleDetail aRuleDetail,
                                  const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Visibility, (mPresContext),
                          visibility, parentVisibility)

  
  
  

  
  SetDiscrete(*aRuleData->ValueForDirection(), visibility->mDirection,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentVisibility->mDirection,
              (GET_BIDI_OPTION_DIRECTION(mPresContext->GetBidi())
               == IBMBIDI_TEXTDIRECTION_RTL)
              ? NS_STYLE_DIRECTION_RTL : NS_STYLE_DIRECTION_LTR,
              0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForVisibility(), visibility->mVisible,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentVisibility->mVisible,
              NS_STYLE_VISIBILITY_VISIBLE, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForPointerEvents(), visibility->mPointerEvents,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentVisibility->mPointerEvents,
              NS_STYLE_POINTER_EVENTS_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForWritingMode(), visibility->mWritingMode,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentVisibility->mWritingMode,
              NS_STYLE_WRITING_MODE_HORIZONTAL_TB, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForTextOrientation(), visibility->mTextOrientation,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentVisibility->mTextOrientation,
              NS_STYLE_TEXT_ORIENTATION_MIXED, 0, 0, 0, 0);

  
  const nsCSSValue* orientation = aRuleData->ValueForImageOrientation();
  if (orientation->GetUnit() == eCSSUnit_Inherit ||
      orientation->GetUnit() == eCSSUnit_Unset) {
    canStoreInRuleTree = false;
    visibility->mImageOrientation = parentVisibility->mImageOrientation;
  } else if (orientation->GetUnit() == eCSSUnit_Initial) {
    visibility->mImageOrientation = nsStyleImageOrientation();
  } else if (orientation->IsAngularUnit()) {
    double angle = orientation->GetAngleValueInRadians();
    visibility->mImageOrientation =
      nsStyleImageOrientation::CreateAsAngleAndFlip(angle, false);
  } else if (orientation->GetUnit() == eCSSUnit_Array) {
    const nsCSSValue::Array* array = orientation->GetArrayValue();
    MOZ_ASSERT(array->Item(0).IsAngularUnit(),
               "First image-orientation value is not an angle");
    MOZ_ASSERT(array->Item(1).GetUnit() == eCSSUnit_Enumerated &&
               array->Item(1).GetIntValue() == NS_STYLE_IMAGE_ORIENTATION_FLIP,
               "Second image-orientation value is not 'flip'");
    double angle = array->Item(0).GetAngleValueInRadians();
    visibility->mImageOrientation =
      nsStyleImageOrientation::CreateAsAngleAndFlip(angle, true);
    
  } else if (orientation->GetUnit() == eCSSUnit_Enumerated) {
    switch (orientation->GetIntValue()) {
      case NS_STYLE_IMAGE_ORIENTATION_FLIP:
        visibility->mImageOrientation = nsStyleImageOrientation::CreateAsFlip();
        break;
      case NS_STYLE_IMAGE_ORIENTATION_FROM_IMAGE:
        visibility->mImageOrientation = nsStyleImageOrientation::CreateAsFromImage();
        break;
      default:
        NS_NOTREACHED("Invalid image-orientation enumerated value");
    }
  } else {
    MOZ_ASSERT(orientation->GetUnit() == eCSSUnit_Null, "Should be null unit");
  }

  COMPUTE_END_INHERITED(Visibility, visibility)
}

const void*
nsRuleNode::ComputeColorData(void* aStartStruct,
                             const nsRuleData* aRuleData,
                             nsStyleContext* aContext,
                             nsRuleNode* aHighestNode,
                             const RuleDetail aRuleDetail,
                             const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Color, (mPresContext), color, parentColor)

  
  
  
  const nsCSSValue* colorValue = aRuleData->ValueForColor();
  if ((colorValue->GetUnit() == eCSSUnit_EnumColor &&
       colorValue->GetIntValue() == NS_COLOR_CURRENTCOLOR) ||
      colorValue->GetUnit() == eCSSUnit_Unset) {
    color->mColor = parentColor->mColor;
    canStoreInRuleTree = false;
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


template <class SpecifiedValueItem, class ComputedValueItem>
struct BackgroundItemComputer {
};

template <>
struct BackgroundItemComputer<nsCSSValueList, uint8_t>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValueList* aSpecifiedValue,
                           uint8_t& aComputedValue,
                           bool& aCanStoreInRuleTree)
  {
    SetDiscrete(aSpecifiedValue->mValue, aComputedValue, aCanStoreInRuleTree,
                SETDSC_ENUMERATED, uint8_t(0), 0, 0, 0, 0, 0);
  }
};

template <>
struct BackgroundItemComputer<nsCSSValuePairList, nsStyleBackground::Repeat>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValuePairList* aSpecifiedValue,
                           nsStyleBackground::Repeat& aComputedValue,
                           bool& aCanStoreInRuleTree)
  {
    NS_ASSERTION(aSpecifiedValue->mXValue.GetUnit() == eCSSUnit_Enumerated &&
                 (aSpecifiedValue->mYValue.GetUnit() == eCSSUnit_Enumerated ||
                  aSpecifiedValue->mYValue.GetUnit() == eCSSUnit_Null),
                 "Invalid unit");
    
    bool hasContraction = true;
    uint8_t value = aSpecifiedValue->mXValue.GetIntValue();
    switch (value) {
    case NS_STYLE_BG_REPEAT_REPEAT_X:
      aComputedValue.mXRepeat = NS_STYLE_BG_REPEAT_REPEAT;
      aComputedValue.mYRepeat = NS_STYLE_BG_REPEAT_NO_REPEAT;
      break;
    case NS_STYLE_BG_REPEAT_REPEAT_Y:
      aComputedValue.mXRepeat = NS_STYLE_BG_REPEAT_NO_REPEAT;
      aComputedValue.mYRepeat = NS_STYLE_BG_REPEAT_REPEAT;
      break;
    default:
      aComputedValue.mXRepeat = value;
      hasContraction = false;
      break;
    }
    
    if (hasContraction) {
      NS_ASSERTION(aSpecifiedValue->mYValue.GetUnit() == eCSSUnit_Null,
                   "Invalid unit.");
      return;
    }
    
    switch (aSpecifiedValue->mYValue.GetUnit()) {
    case eCSSUnit_Null:
      aComputedValue.mYRepeat = aComputedValue.mXRepeat;
      break;
    case eCSSUnit_Enumerated:
      value = aSpecifiedValue->mYValue.GetIntValue();
      NS_ASSERTION(value == NS_STYLE_BG_REPEAT_NO_REPEAT ||
                   value == NS_STYLE_BG_REPEAT_REPEAT, "Unexpected value");
      aComputedValue.mYRepeat = value;
      break;
    default:
      NS_NOTREACHED("Unexpected CSS value");
      break;
    }
  }
};

template <>
struct BackgroundItemComputer<nsCSSValueList, nsStyleImage>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValueList* aSpecifiedValue,
                           nsStyleImage& aComputedValue,
                           bool& aCanStoreInRuleTree)
  {
    SetStyleImage(aStyleContext, aSpecifiedValue->mValue, aComputedValue,
                  aCanStoreInRuleTree);
  }
};





typedef nsStyleBackground::Position::PositionCoord PositionCoord;
static void
ComputePositionCoord(nsStyleContext* aStyleContext,
                     const nsCSSValue& aEdge,
                     const nsCSSValue& aOffset,
                     PositionCoord* aResult,
                     bool& aCanStoreInRuleTree)
{
  if (eCSSUnit_Percent == aOffset.GetUnit()) {
    aResult->mLength = 0;
    aResult->mPercent = aOffset.GetPercentValue();
    aResult->mHasPercent = true;
  } else if (aOffset.IsLengthUnit()) {
    aResult->mLength = CalcLength(aOffset, aStyleContext,
                                  aStyleContext->PresContext(),
                                  aCanStoreInRuleTree);
    aResult->mPercent = 0.0f;
    aResult->mHasPercent = false;
  } else if (aOffset.IsCalcUnit()) {
    LengthPercentPairCalcOps ops(aStyleContext,
                                 aStyleContext->PresContext(),
                                 aCanStoreInRuleTree);
    nsRuleNode::ComputedCalc vals = ComputeCalc(aOffset, ops);
    aResult->mLength = vals.mLength;
    aResult->mPercent = vals.mPercent;
    aResult->mHasPercent = ops.mHasPercent;
  } else {
    aResult->mLength = 0;
    aResult->mPercent = 0.0f;
    aResult->mHasPercent = false;
    NS_ASSERTION(aOffset.GetUnit() == eCSSUnit_Null, "unexpected unit");
  }

  if (eCSSUnit_Enumerated == aEdge.GetUnit()) {
    int sign;
    if (aEdge.GetIntValue() & (NS_STYLE_BG_POSITION_BOTTOM |
                               NS_STYLE_BG_POSITION_RIGHT)) {
      sign = -1;
    } else {
      sign = 1;
    }
    aResult->mPercent = GetFloatFromBoxPosition(aEdge.GetIntValue()) +
                        sign * aResult->mPercent;
    aResult->mLength = sign * aResult->mLength;
    aResult->mHasPercent = true;
  } else {
    NS_ASSERTION(eCSSUnit_Null == aEdge.GetUnit(), "unexpected unit");
  }
}



static void
ComputePositionValue(nsStyleContext* aStyleContext,
                     const nsCSSValue& aValue,
                     nsStyleBackground::Position& aComputedValue,
                     bool& aCanStoreInRuleTree)
{
  NS_ASSERTION(aValue.GetUnit() == eCSSUnit_Array,
               "unexpected unit for CSS <position> value");

  nsRefPtr<nsCSSValue::Array> positionArray = aValue.GetArrayValue();
  const nsCSSValue &xEdge   = positionArray->Item(0);
  const nsCSSValue &xOffset = positionArray->Item(1);
  const nsCSSValue &yEdge   = positionArray->Item(2);
  const nsCSSValue &yOffset = positionArray->Item(3);

  NS_ASSERTION((eCSSUnit_Enumerated == xEdge.GetUnit()  ||
                eCSSUnit_Null       == xEdge.GetUnit()) &&
               (eCSSUnit_Enumerated == yEdge.GetUnit()  ||
                eCSSUnit_Null       == yEdge.GetUnit()) &&
               eCSSUnit_Enumerated != xOffset.GetUnit()  &&
               eCSSUnit_Enumerated != yOffset.GetUnit(),
               "Invalid background position");

  ComputePositionCoord(aStyleContext, xEdge, xOffset,
                       &aComputedValue.mXPosition,
                       aCanStoreInRuleTree);

  ComputePositionCoord(aStyleContext, yEdge, yOffset,
                       &aComputedValue.mYPosition,
                       aCanStoreInRuleTree);
}

template <>
struct BackgroundItemComputer<nsCSSValueList, nsStyleBackground::Position>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValueList* aSpecifiedValue,
                           nsStyleBackground::Position& aComputedValue,
                           bool& aCanStoreInRuleTree)
  {
    ComputePositionValue(aStyleContext, aSpecifiedValue->mValue,
                         aComputedValue, aCanStoreInRuleTree);
  }
};


struct BackgroundSizeAxis {
  nsCSSValue nsCSSValuePairList::* specified;
  nsStyleBackground::Size::Dimension nsStyleBackground::Size::* result;
  uint8_t nsStyleBackground::Size::* type;
};

static const BackgroundSizeAxis gBGSizeAxes[] = {
  { &nsCSSValuePairList::mXValue,
    &nsStyleBackground::Size::mWidth,
    &nsStyleBackground::Size::mWidthType },
  { &nsCSSValuePairList::mYValue,
    &nsStyleBackground::Size::mHeight,
    &nsStyleBackground::Size::mHeightType }
};

template <>
struct BackgroundItemComputer<nsCSSValuePairList, nsStyleBackground::Size>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValuePairList* aSpecifiedValue,
                           nsStyleBackground::Size& aComputedValue,
                           bool& aCanStoreInRuleTree)
  {
    nsStyleBackground::Size &size = aComputedValue;
    for (const BackgroundSizeAxis *axis = gBGSizeAxes,
                        *axis_end = ArrayEnd(gBGSizeAxes);
         axis < axis_end; ++axis) {
      const nsCSSValue &specified = aSpecifiedValue->*(axis->specified);
      if (eCSSUnit_Auto == specified.GetUnit()) {
        size.*(axis->type) = nsStyleBackground::Size::eAuto;
      }
      else if (eCSSUnit_Enumerated == specified.GetUnit()) {
        static_assert(nsStyleBackground::Size::eContain ==
                      NS_STYLE_BG_SIZE_CONTAIN &&
                      nsStyleBackground::Size::eCover ==
                      NS_STYLE_BG_SIZE_COVER,
                      "background size constants out of sync");
        MOZ_ASSERT(specified.GetIntValue() == NS_STYLE_BG_SIZE_CONTAIN ||
                   specified.GetIntValue() == NS_STYLE_BG_SIZE_COVER,
                   "invalid enumerated value for size coordinate");
        size.*(axis->type) = specified.GetIntValue();
      }
      else if (eCSSUnit_Null == specified.GetUnit()) {
        MOZ_ASSERT(axis == gBGSizeAxes + 1,
                   "null allowed only as height value, and only "
                   "for contain/cover/initial/inherit");
#ifdef DEBUG
        {
          const nsCSSValue &widthValue = aSpecifiedValue->mXValue;
          MOZ_ASSERT(widthValue.GetUnit() != eCSSUnit_Inherit &&
                     widthValue.GetUnit() != eCSSUnit_Initial &&
                     widthValue.GetUnit() != eCSSUnit_Unset,
                     "initial/inherit/unset should already have been handled");
          MOZ_ASSERT(widthValue.GetUnit() == eCSSUnit_Enumerated &&
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
        (size.*(axis->result)).mHasPercent = true;
        size.*(axis->type) = nsStyleBackground::Size::eLengthPercentage;
      }
      else if (specified.IsLengthUnit()) {
        (size.*(axis->result)).mLength =
          CalcLength(specified, aStyleContext, aStyleContext->PresContext(),
                     aCanStoreInRuleTree);
        (size.*(axis->result)).mPercent = 0.0f;
        (size.*(axis->result)).mHasPercent = false;
        size.*(axis->type) = nsStyleBackground::Size::eLengthPercentage;
      } else {
        MOZ_ASSERT(specified.IsCalcUnit(), "unexpected unit");
        LengthPercentPairCalcOps ops(aStyleContext,
                                     aStyleContext->PresContext(),
                                     aCanStoreInRuleTree);
        nsRuleNode::ComputedCalc vals = ComputeCalc(specified, ops);
        (size.*(axis->result)).mLength = vals.mLength;
        (size.*(axis->result)).mPercent = vals.mPercent;
        (size.*(axis->result)).mHasPercent = ops.mHasPercent;
        size.*(axis->type) = nsStyleBackground::Size::eLengthPercentage;
      }
    }

    MOZ_ASSERT(size.mWidthType < nsStyleBackground::Size::eDimensionType_COUNT,
               "bad width type");
    MOZ_ASSERT(size.mHeightType < nsStyleBackground::Size::eDimensionType_COUNT,
               "bad height type");
    MOZ_ASSERT((size.mWidthType != nsStyleBackground::Size::eContain &&
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
                  uint32_t aParentItemCount,
                  uint32_t& aItemCount,
                  uint32_t& aMaxItemCount,
                  bool& aRebuild,
                  bool& aCanStoreInRuleTree)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aRebuild = true;
    aCanStoreInRuleTree = false;
    aLayers.EnsureLengthAtLeast(aParentItemCount);
    aItemCount = aParentItemCount;
    for (uint32_t i = 0; i < aParentItemCount; ++i) {
      aLayers[i].*aResultLocation = aParentLayers[i].*aResultLocation;
    }
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    aRebuild = true;
    aItemCount = 1;
    aLayers[0].*aResultLocation = aInitialValue;
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep: {
    aRebuild = true;
    aItemCount = 0;
    const nsCSSValueList* item = aValue.GetListValue();
    do {
      NS_ASSERTION(item->mValue.GetUnit() != eCSSUnit_Null &&
                   item->mValue.GetUnit() != eCSSUnit_Inherit &&
                   item->mValue.GetUnit() != eCSSUnit_Initial &&
                   item->mValue.GetUnit() != eCSSUnit_Unset,
                   "unexpected unit");
      ++aItemCount;
      aLayers.EnsureLengthAtLeast(aItemCount);
      BackgroundItemComputer<nsCSSValueList, ComputedValueItem>
        ::ComputeValue(aStyleContext, item,
                       aLayers[aItemCount-1].*aResultLocation,
                       aCanStoreInRuleTree);
      item = item->mNext;
    } while (item);
    break;
  }

  default:
    MOZ_ASSERT(false, "unexpected unit");
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
                      uint32_t aParentItemCount,
                      uint32_t& aItemCount,
                      uint32_t& aMaxItemCount,
                      bool& aRebuild,
                      bool& aCanStoreInRuleTree)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aRebuild = true;
    aCanStoreInRuleTree = false;
    aLayers.EnsureLengthAtLeast(aParentItemCount);
    aItemCount = aParentItemCount;
    for (uint32_t i = 0; i < aParentItemCount; ++i) {
      aLayers[i].*aResultLocation = aParentLayers[i].*aResultLocation;
    }
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    aRebuild = true;
    aItemCount = 1;
    aLayers[0].*aResultLocation = aInitialValue;
    break;

  case eCSSUnit_PairList:
  case eCSSUnit_PairListDep: {
    aRebuild = true;
    aItemCount = 0;
    const nsCSSValuePairList* item = aValue.GetPairListValue();
    do {
      NS_ASSERTION(item->mXValue.GetUnit() != eCSSUnit_Inherit &&
                   item->mXValue.GetUnit() != eCSSUnit_Initial &&
                   item->mXValue.GetUnit() != eCSSUnit_Unset &&
                   item->mYValue.GetUnit() != eCSSUnit_Inherit &&
                   item->mYValue.GetUnit() != eCSSUnit_Initial &&
                   item->mYValue.GetUnit() != eCSSUnit_Unset,
                   "unexpected unit");
      ++aItemCount;
      aLayers.EnsureLengthAtLeast(aItemCount);
      BackgroundItemComputer<nsCSSValuePairList, ComputedValueItem>
        ::ComputeValue(aStyleContext, item,
                       aLayers[aItemCount-1].*aResultLocation,
                       aCanStoreInRuleTree);
      item = item->mNext;
    } while (item);
    break;
  }

  default:
    MOZ_ASSERT(false, "unexpected unit");
  }

  if (aItemCount > aMaxItemCount)
    aMaxItemCount = aItemCount;
}

template <class ComputedValueItem>
static void
FillBackgroundList(nsAutoTArray< nsStyleBackground::Layer, 1> &aLayers,
    ComputedValueItem nsStyleBackground::Layer::* aResultLocation,
    uint32_t aItemCount, uint32_t aFillCount)
{
  NS_PRECONDITION(aFillCount <= aLayers.Length(), "unexpected array length");
  for (uint32_t sourceLayer = 0, destLayer = aItemCount;
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
                                  const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Background, (), bg, parentBG)

  
  const nsCSSValue* backColorValue = aRuleData->ValueForBackgroundColor();
  if (eCSSUnit_Initial == backColorValue->GetUnit() ||
      eCSSUnit_Unset == backColorValue->GetUnit()) {
    bg->mBackgroundColor = NS_RGBA(0, 0, 0, 0);
  } else if (!SetColor(*backColorValue, parentBG->mBackgroundColor,
                       mPresContext, aContext, bg->mBackgroundColor,
                       canStoreInRuleTree)) {
    NS_ASSERTION(eCSSUnit_Null == backColorValue->GetUnit(),
                 "unexpected color unit");
  }

  uint32_t maxItemCount = 1;
  bool rebuild = false;

  
  nsStyleImage initialImage;
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundImage(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mImage,
                    initialImage, parentBG->mImageCount, bg->mImageCount,
                    maxItemCount, rebuild, canStoreInRuleTree);

  
  nsStyleBackground::Repeat initialRepeat;
  initialRepeat.SetInitialValues();
  SetBackgroundPairList(aContext, *aRuleData->ValueForBackgroundRepeat(),
                        bg->mLayers,
                        parentBG->mLayers, &nsStyleBackground::Layer::mRepeat,
                        initialRepeat, parentBG->mRepeatCount,
                        bg->mRepeatCount, maxItemCount, rebuild, 
                        canStoreInRuleTree);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundAttachment(),
                    bg->mLayers, parentBG->mLayers,
                    &nsStyleBackground::Layer::mAttachment,
                    uint8_t(NS_STYLE_BG_ATTACHMENT_SCROLL),
                    parentBG->mAttachmentCount,
                    bg->mAttachmentCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundClip(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mClip,
                    uint8_t(NS_STYLE_BG_CLIP_BORDER), parentBG->mClipCount,
                    bg->mClipCount, maxItemCount, rebuild, canStoreInRuleTree);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundBlendMode(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mBlendMode,
                    uint8_t(NS_STYLE_BLEND_NORMAL), parentBG->mBlendModeCount,
                    bg->mBlendModeCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundOrigin(),
                    bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mOrigin,
                    uint8_t(NS_STYLE_BG_ORIGIN_PADDING), parentBG->mOriginCount,
                    bg->mOriginCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  nsStyleBackground::Position initialPosition;
  initialPosition.SetInitialPercentValues(0.0f);
  SetBackgroundList(aContext, *aRuleData->ValueForBackgroundPosition(),
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

    uint32_t fillCount = bg->mImageCount;
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mImage,
                       bg->mImageCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mRepeat,
                       bg->mRepeatCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mAttachment,
                       bg->mAttachmentCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mClip,
                       bg->mClipCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mBlendMode,
                       bg->mBlendModeCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mOrigin,
                       bg->mOriginCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mPosition,
                       bg->mPositionCount, fillCount);
    FillBackgroundList(bg->mLayers, &nsStyleBackground::Layer::mSize,
                       bg->mSizeCount, fillCount);
  }

  
  for (uint32_t i = 0; i < bg->mImageCount; ++i)
    bg->mLayers[i].TrackImages(aContext->PresContext());

  COMPUTE_END_RESET(Background, bg)
}

const void*
nsRuleNode::ComputeMarginData(void* aStartStruct,
                              const nsRuleData* aRuleData,
                              nsStyleContext* aContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Margin, (), margin, parentMargin)

  
  const nsCSSProperty* subprops =
    nsCSSProps::SubpropertyEntryFor(eCSSProperty_margin);
  nsStyleCoord coord;
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentMargin->mMargin.Get(side);
    if (SetCoord(*aRuleData->ValueFor(subprops[side]),
                 coord, parentCoord,
                 SETCOORD_LPAH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC |
                   SETCOORD_UNSET_INITIAL,
                 aContext, mPresContext, canStoreInRuleTree)) {
      margin->mMargin.Set(side, coord);
    }
  }

  margin->RecalcData();
  COMPUTE_END_RESET(Margin, margin)
}

static void
SetBorderImageRect(const nsCSSValue& aValue,
                    nsCSSRect& aRect)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    aRect.Reset();
    break;
  case eCSSUnit_Rect:
    aRect = aValue.GetRectValue();
    break;
  case eCSSUnit_Inherit:
  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    aRect.SetAllSidesTo(aValue);
    break;
  default:
    NS_ASSERTION(false, "Unexpected border image value for rect.");
  }
}

static void
SetBorderImagePair(const nsCSSValue& aValue,
                    nsCSSValuePair& aPair)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    aPair.Reset();
    break;
  case eCSSUnit_Pair:
    aPair = aValue.GetPairValue();
    break;
  case eCSSUnit_Inherit:
  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    aPair.SetBothValuesTo(aValue);
    break;
  default:
    NS_ASSERTION(false, "Unexpected border image value for pair.");
  }
}

static void
SetBorderImageSlice(const nsCSSValue& aValue,
                     nsCSSValue& aSlice,
                     nsCSSValue& aFill)
{
  const nsCSSValueList* valueList;
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    aSlice.Reset();
    aFill.Reset();
    break;
  case eCSSUnit_List:
    
    valueList = aValue.GetListValue();
    aSlice = valueList->mValue;

    
    valueList = valueList->mNext;
    if (valueList) {
      aFill = valueList->mValue;
    } else {
      aFill.SetInitialValue();
    }
    break;
  case eCSSUnit_Inherit:
  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    aSlice = aValue;
    aFill = aValue;
    break;
  default:
    NS_ASSERTION(false, "Unexpected border image value for pair.");
  }
}

const void*
nsRuleNode::ComputeBorderData(void* aStartStruct,
                              const nsRuleData* aRuleData,
                              nsStyleContext* aContext,
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Border, (mPresContext), border, parentBorder)

  
  SetDiscrete(*aRuleData->ValueForBoxDecorationBreak(),
              border->mBoxDecorationBreak, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentBorder->mBoxDecorationBreak,
              NS_STYLE_BOX_DECORATION_BREAK_SLICE, 0, 0, 0, 0);

  
  const nsCSSValue* boxShadowValue = aRuleData->ValueForBoxShadow();
  switch (boxShadowValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
  case eCSSUnit_None:
    border->mBoxShadow = nullptr;
    break;

  case eCSSUnit_Inherit:
    border->mBoxShadow = parentBorder->mBoxShadow;
    canStoreInRuleTree = false;
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep:
    border->mBoxShadow = GetShadowData(boxShadowValue->GetListValue(),
                                       aContext, true, canStoreInRuleTree);
    break;

  default:
    MOZ_ASSERT(false, "unrecognized shadow unit");
  }

  
  nsStyleCoord coord;
  {
    const nsCSSProperty* subprops =
      nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_width);
    NS_FOR_CSS_SIDES(side) {
      const nsCSSValue& value = *aRuleData->ValueFor(subprops[side]);
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
        
        border->SetBorderWidth(side, std::max(coord.GetCoordValue(), 0));
      }
      else if (eCSSUnit_Inherit == value.GetUnit()) {
        canStoreInRuleTree = false;
        border->SetBorderWidth(side,
                               parentBorder->GetComputedBorder().Side(side));
      }
      else if (eCSSUnit_Initial == value.GetUnit() ||
               eCSSUnit_Unset == value.GetUnit()) {
        border->SetBorderWidth(side,
          (mPresContext->GetBorderWidthTable())[NS_STYLE_BORDER_WIDTH_MEDIUM]);
      }
      else {
        NS_ASSERTION(eCSSUnit_Null == value.GetUnit(),
                     "missing case handling border width");
      }
    }
  }

  
  {
    const nsCSSProperty* subprops =
      nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_style);
    NS_FOR_CSS_SIDES(side) {
      const nsCSSValue& value = *aRuleData->ValueFor(subprops[side]);
      nsCSSUnit unit = value.GetUnit();
      MOZ_ASSERT(eCSSUnit_None != unit,
                 "'none' should be handled as enumerated value");
      if (eCSSUnit_Enumerated == unit) {
        border->SetBorderStyle(side, value.GetIntValue());
      }
      else if (eCSSUnit_Initial == unit ||
               eCSSUnit_Unset == unit) {
        border->SetBorderStyle(side, NS_STYLE_BORDER_STYLE_NONE);
      }
      else if (eCSSUnit_Inherit == unit) {
        canStoreInRuleTree = false;
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
    case eCSSUnit_Unset:
    case eCSSUnit_None:
      border->ClearBorderColors(side);
      break;

    case eCSSUnit_Inherit: {
      canStoreInRuleTree = false;
      border->ClearBorderColors(side);
      if (parentContext) {
        nsBorderColors *parentColors;
        parentBorder->GetCompositeColors(side, &parentColors);
        if (parentColors) {
          border->EnsureBorderColors();
          border->mBorderColors[side] = parentColors->Clone();
        }
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
      MOZ_ASSERT(false, "unrecognized border color unit");
    }
  }

  
  {
    const nsCSSProperty* subprops =
      nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_color);
    bool foreground;
    NS_FOR_CSS_SIDES(side) {
      const nsCSSValue& value = *aRuleData->ValueFor(subprops[side]);
      if (eCSSUnit_Inherit == value.GetUnit()) {
        canStoreInRuleTree = false;
        if (parentContext) {
          parentBorder->GetBorderColor(side, borderColor, foreground);
          if (foreground) {
            
            
            
            
            
            border->SetBorderColor(side, parentContext->StyleColor()->mColor);
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
          default:
            NS_NOTREACHED("Unexpected enumerated color");
            break;
        }
      }
      else if (eCSSUnit_Initial == value.GetUnit() ||
               eCSSUnit_Unset == value.GetUnit()) {
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
                          SETCOORD_STORE_CALC | SETCOORD_UNSET_INITIAL,
                        aContext, mPresContext, canStoreInRuleTree)) {
        border->mBorderRadius.Set(cx, coordX);
        border->mBorderRadius.Set(cy, coordY);
      }
    }
  }

  
  SetDiscrete(*aRuleData->ValueForFloatEdge(),
              border->mFloatEdge, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentBorder->mFloatEdge,
              NS_STYLE_FLOAT_EDGE_CONTENT, 0, 0, 0, 0);

  
  const nsCSSValue* borderImageSource = aRuleData->ValueForBorderImageSource();
  if (borderImageSource->GetUnit() == eCSSUnit_Inherit) {
    canStoreInRuleTree = false;
    border->mBorderImageSource = parentBorder->mBorderImageSource;
  } else {
    SetStyleImage(aContext,
                  *borderImageSource,
                  border->mBorderImageSource,
                  canStoreInRuleTree);
  }

  nsCSSValue borderImageSliceValue;
  nsCSSValue borderImageSliceFill;
  SetBorderImageSlice(*aRuleData->ValueForBorderImageSlice(),
                      borderImageSliceValue, borderImageSliceFill);

  
  SetDiscrete(borderImageSliceFill,
              border->mBorderImageFill,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentBorder->mBorderImageFill,
              NS_STYLE_BORDER_IMAGE_SLICE_NOFILL, 0, 0, 0, 0);

  nsCSSRect borderImageSlice;
  SetBorderImageRect(borderImageSliceValue, borderImageSlice);

  nsCSSRect borderImageWidth;
  SetBorderImageRect(*aRuleData->ValueForBorderImageWidth(),
                     borderImageWidth);

  nsCSSRect borderImageOutset;
  SetBorderImageRect(*aRuleData->ValueForBorderImageOutset(),
                     borderImageOutset);

  NS_FOR_CSS_SIDES (side) {
    
    if (SetCoord(borderImageSlice.*(nsCSSRect::sides[side]), coord,
                 parentBorder->mBorderImageSlice.Get(side),
                 SETCOORD_FACTOR | SETCOORD_PERCENT |
                   SETCOORD_INHERIT | SETCOORD_INITIAL_HUNDRED_PCT |
                   SETCOORD_UNSET_INITIAL,
                 aContext, mPresContext, canStoreInRuleTree)) {
      border->mBorderImageSlice.Set(side, coord);
    }

    
    
    if (SetCoord(borderImageWidth.*(nsCSSRect::sides[side]), coord,
                 parentBorder->mBorderImageWidth.Get(side),
                 SETCOORD_LPAH | SETCOORD_FACTOR | SETCOORD_INITIAL_FACTOR_ONE |
                   SETCOORD_UNSET_INITIAL,
                 aContext, mPresContext, canStoreInRuleTree)) {
      border->mBorderImageWidth.Set(side, coord);
    }

    
    if (SetCoord(borderImageOutset.*(nsCSSRect::sides[side]), coord,
                 parentBorder->mBorderImageOutset.Get(side),
                 SETCOORD_LENGTH | SETCOORD_FACTOR |
                   SETCOORD_INHERIT | SETCOORD_INITIAL_FACTOR_ZERO |
                   SETCOORD_UNSET_INITIAL,
                 aContext, mPresContext, canStoreInRuleTree)) {
      border->mBorderImageOutset.Set(side, coord);
    }
  }

  
  nsCSSValuePair borderImageRepeat;
  SetBorderImagePair(*aRuleData->ValueForBorderImageRepeat(),
                     borderImageRepeat);

  SetDiscrete(borderImageRepeat.mXValue,
              border->mBorderImageRepeatH,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentBorder->mBorderImageRepeatH,
              NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH, 0, 0, 0, 0);

  SetDiscrete(borderImageRepeat.mYValue,
              border->mBorderImageRepeatV,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentBorder->mBorderImageRepeatV,
              NS_STYLE_BORDER_IMAGE_REPEAT_STRETCH, 0, 0, 0, 0);

  border->TrackImage(aContext->PresContext());

  COMPUTE_END_RESET(Border, border)
}

const void*
nsRuleNode::ComputePaddingData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Padding, (), padding, parentPadding)

  
  const nsCSSProperty* subprops =
    nsCSSProps::SubpropertyEntryFor(eCSSProperty_padding);
  nsStyleCoord coord;
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentPadding->mPadding.Get(side);
    if (SetCoord(*aRuleData->ValueFor(subprops[side]),
                 coord, parentCoord,
                 SETCOORD_LPH | SETCOORD_INITIAL_ZERO | SETCOORD_STORE_CALC |
                   SETCOORD_UNSET_INITIAL,
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
                               const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Outline, (mPresContext), outline, parentOutline)

  
  const nsCSSValue* outlineWidthValue = aRuleData->ValueForOutlineWidth();
  if (eCSSUnit_Initial == outlineWidthValue->GetUnit() ||
      eCSSUnit_Unset == outlineWidthValue->GetUnit()) {
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
               SETCOORD_LH | SETCOORD_INITIAL_ZERO | SETCOORD_CALC_LENGTH_ONLY |
                 SETCOORD_UNSET_INITIAL,
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
    canStoreInRuleTree = false;
    if (parentContext) {
      if (parentOutline->GetOutlineColor(outlineColor))
        outline->SetOutlineColor(outlineColor);
      else {
        
        
        
        
        
        outline->SetOutlineColor(parentContext->StyleColor()->mColor);
      }
    } else {
      outline->SetOutlineInitialColor();
    }
  }
  else if (SetColor(*outlineColorValue, unused, mPresContext,
                    aContext, outlineColor, canStoreInRuleTree))
    outline->SetOutlineColor(outlineColor);
  else if (eCSSUnit_Enumerated == outlineColorValue->GetUnit() ||
           eCSSUnit_Initial == outlineColorValue->GetUnit() ||
           eCSSUnit_Unset == outlineColorValue->GetUnit()) {
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
                          SETCOORD_STORE_CALC | SETCOORD_UNSET_INITIAL,
                        aContext, mPresContext, canStoreInRuleTree)) {
        outline->mOutlineRadius.Set(cx, coordX);
        outline->mOutlineRadius.Set(cy, coordY);
      }
    }
  }

  
  
  const nsCSSValue* outlineStyleValue = aRuleData->ValueForOutlineStyle();
  nsCSSUnit unit = outlineStyleValue->GetUnit();
  MOZ_ASSERT(eCSSUnit_None != unit && eCSSUnit_Auto != unit,
             "'none' and 'auto' should be handled as enumerated values");
  if (eCSSUnit_Enumerated == unit) {
    outline->SetOutlineStyle(outlineStyleValue->GetIntValue());
  } else if (eCSSUnit_Initial == unit ||
             eCSSUnit_Unset == unit) {
    outline->SetOutlineStyle(NS_STYLE_BORDER_STYLE_NONE);
  } else if (eCSSUnit_Inherit == unit) {
    canStoreInRuleTree = false;
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
                            const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(List, (mPresContext), list, parentList)

  
  const nsCSSValue* typeValue = aRuleData->ValueForListStyleType();
  switch (typeValue->GetUnit()) {
    case eCSSUnit_Unset:
    case eCSSUnit_Inherit: {
      canStoreInRuleTree = false;
      nsString type;
      parentList->GetListStyleType(type);
      list->SetListStyleType(type, parentList->GetCounterStyle());
      break;
    }
    case eCSSUnit_Initial:
      list->SetListStyleType(NS_LITERAL_STRING("disc"), mPresContext);
      break;
    case eCSSUnit_Ident: {
      nsString typeIdent;
      typeValue->GetStringValue(typeIdent);
      list->SetListStyleType(typeIdent, mPresContext);
      break;
    }
    case eCSSUnit_String: {
      nsString str;
      typeValue->GetStringValue(str);
      list->SetListStyleType(NS_LITERAL_STRING(""),
                             new AnonymousCounterStyle(str));
      break;
    }
    case eCSSUnit_Enumerated: {
      
      
      int32_t intValue = typeValue->GetIntValue();
      nsAutoString name;
      switch (intValue) {
        case NS_STYLE_LIST_STYLE_LOWER_ROMAN:
          name.AssignLiteral(MOZ_UTF16("lower-roman"));
          break;
        case NS_STYLE_LIST_STYLE_UPPER_ROMAN:
          name.AssignLiteral(MOZ_UTF16("upper-roman"));
          break;
        case NS_STYLE_LIST_STYLE_LOWER_ALPHA:
          name.AssignLiteral(MOZ_UTF16("lower-alpha"));
          break;
        case NS_STYLE_LIST_STYLE_UPPER_ALPHA:
          name.AssignLiteral(MOZ_UTF16("upper-alpha"));
          break;
        default:
          CopyASCIItoUTF16(nsCSSProps::ValueToKeyword(
                  intValue, nsCSSProps::kListStyleKTable), name);
          break;
      }
      list->SetListStyleType(name, mPresContext);
      break;
    }
    case eCSSUnit_Symbols:
      list->SetListStyleType(
        NS_LITERAL_STRING(""),
        new AnonymousCounterStyle(typeValue->GetArrayValue()));
      break;
    case eCSSUnit_Null:
      break;
    default:
      NS_NOTREACHED("Unexpected value unit");
  }

  
  const nsCSSValue* imageValue = aRuleData->ValueForListStyleImage();
  if (eCSSUnit_Image == imageValue->GetUnit()) {
    NS_SET_IMAGE_REQUEST_WITH_DOC(list->SetListStyleImage,
                                  aContext,
                                  imageValue->GetImageValue)
  }
  else if (eCSSUnit_None == imageValue->GetUnit() ||
           eCSSUnit_Initial == imageValue->GetUnit()) {
    list->SetListStyleImage(nullptr);
  }
  else if (eCSSUnit_Inherit == imageValue->GetUnit() ||
           eCSSUnit_Unset == imageValue->GetUnit()) {
    canStoreInRuleTree = false;
    NS_SET_IMAGE_REQUEST(list->SetListStyleImage,
                         aContext,
                         parentList->GetListStyleImage())
  }

  
  SetDiscrete(*aRuleData->ValueForListStylePosition(),
              list->mListStylePosition, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentList->mListStylePosition,
              NS_STYLE_LIST_STYLE_POSITION_OUTSIDE, 0, 0, 0, 0);

  
  const nsCSSValue* imageRegionValue = aRuleData->ValueForImageRegion();
  switch (imageRegionValue->GetUnit()) {
  case eCSSUnit_Inherit:
  case eCSSUnit_Unset:
    canStoreInRuleTree = false;
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
    MOZ_ASSERT(false, "unrecognized image-region unit");
  }

  COMPUTE_END_INHERITED(List, list)
}

static void
SetGridTrackBreadth(const nsCSSValue& aValue,
                    nsStyleCoord& aResult,
                    nsStyleContext* aStyleContext,
                    nsPresContext* aPresContext,
                    bool& aCanStoreInRuleTree)
{
  nsCSSUnit unit = aValue.GetUnit();
  if (unit == eCSSUnit_FlexFraction) {
    aResult.SetFlexFractionValue(aValue.GetFloatValue());
  } else {
    MOZ_ASSERT(unit != eCSSUnit_Inherit && unit != eCSSUnit_Unset,
               "Unexpected value that would use dummyParentCoord");
    const nsStyleCoord dummyParentCoord;
    SetCoord(aValue, aResult, dummyParentCoord,
             SETCOORD_LPE | SETCOORD_STORE_CALC,
             aStyleContext, aPresContext, aCanStoreInRuleTree);
  }
}

static void
SetGridTrackSize(const nsCSSValue& aValue,
                 nsStyleCoord& aResultMin,
                 nsStyleCoord& aResultMax,
                 nsStyleContext* aStyleContext,
                 nsPresContext* aPresContext,
                 bool& aCanStoreInRuleTree)
{
  if (aValue.GetUnit() == eCSSUnit_Function) {
    
    nsCSSValue::Array* func = aValue.GetArrayValue();
    NS_ASSERTION(func->Item(0).GetKeywordValue() == eCSSKeyword_minmax,
                 "Expected minmax(), got another function name");
    SetGridTrackBreadth(func->Item(1), aResultMin,
                        aStyleContext, aPresContext, aCanStoreInRuleTree);
    SetGridTrackBreadth(func->Item(2), aResultMax,
                        aStyleContext, aPresContext, aCanStoreInRuleTree);
  } else if (aValue.GetUnit() == eCSSUnit_Auto) {
    
    aResultMin.SetIntValue(NS_STYLE_GRID_TRACK_BREADTH_MIN_CONTENT,
                           eStyleUnit_Enumerated);
    aResultMax.SetIntValue(NS_STYLE_GRID_TRACK_BREADTH_MAX_CONTENT,
                           eStyleUnit_Enumerated);
  } else {
    
    
    SetGridTrackBreadth(aValue, aResultMin,
                        aStyleContext, aPresContext, aCanStoreInRuleTree);
    aResultMax = aResultMin;
  }
}

static void
SetGridAutoColumnsRows(const nsCSSValue& aValue,
                       nsStyleCoord& aResultMin,
                       nsStyleCoord& aResultMax,
                       const nsStyleCoord& aParentValueMin,
                       const nsStyleCoord& aParentValueMax,
                       nsStyleContext* aStyleContext,
                       nsPresContext* aPresContext,
                       bool& aCanStoreInRuleTree)

{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aCanStoreInRuleTree = false;
    aResultMin = aParentValueMin;
    aResultMax = aParentValueMax;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    
    
    
    aResultMin.SetIntValue(NS_STYLE_GRID_TRACK_BREADTH_MIN_CONTENT,
                           eStyleUnit_Enumerated);
    aResultMax.SetIntValue(NS_STYLE_GRID_TRACK_BREADTH_MAX_CONTENT,
                           eStyleUnit_Enumerated);
    break;

  default:
    SetGridTrackSize(aValue, aResultMin, aResultMax,
                     aStyleContext, aPresContext, aCanStoreInRuleTree);
  }
}

static void
AppendGridLineNames(const nsCSSValue& aValue,
                    nsStyleGridTemplate& aResult)
{
  
  nsTArray<nsString>* nameList = aResult.mLineNameLists.AppendElement();
  
  if (aValue.GetUnit() != eCSSUnit_Null) {
    const nsCSSValueList* item = aValue.GetListValue();
    do {
      nsString* name = nameList->AppendElement();
      item->mValue.GetStringValue(*name);
      item = item->mNext;
    } while (item);
  }
}

static void
SetGridTrackList(const nsCSSValue& aValue,
                 nsStyleGridTemplate& aResult,
                 const nsStyleGridTemplate& aParentValue,
                 nsStyleContext* aStyleContext,
                 nsPresContext* aPresContext,
                 bool& aCanStoreInRuleTree)

{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aCanStoreInRuleTree = false;
    aResult.mIsSubgrid = aParentValue.mIsSubgrid;
    aResult.mLineNameLists = aParentValue.mLineNameLists;
    aResult.mMinTrackSizingFunctions = aParentValue.mMinTrackSizingFunctions;
    aResult.mMaxTrackSizingFunctions = aParentValue.mMaxTrackSizingFunctions;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
  case eCSSUnit_None:
    aResult.mIsSubgrid = false;
    aResult.mLineNameLists.Clear();
    aResult.mMinTrackSizingFunctions.Clear();
    aResult.mMaxTrackSizingFunctions.Clear();
    break;

  default:
    aResult.mLineNameLists.Clear();
    aResult.mMinTrackSizingFunctions.Clear();
    aResult.mMaxTrackSizingFunctions.Clear();
    const nsCSSValueList* item = aValue.GetListValue();
    if (item->mValue.GetUnit() == eCSSUnit_Enumerated &&
        item->mValue.GetIntValue() == NS_STYLE_GRID_TEMPLATE_SUBGRID) {
      
      aResult.mIsSubgrid = true;
      item = item->mNext;
      while (item) {
        AppendGridLineNames(item->mValue, aResult);
        item = item->mNext;
      }
    } else {
      
      
      
      
      aResult.mIsSubgrid = false;
      for (;;) {
        AppendGridLineNames(item->mValue, aResult);
        item = item->mNext;

        if (!item) {
          break;
        }

        nsStyleCoord& min = *aResult.mMinTrackSizingFunctions.AppendElement();
        nsStyleCoord& max = *aResult.mMaxTrackSizingFunctions.AppendElement();
        SetGridTrackSize(item->mValue, min, max,
                         aStyleContext, aPresContext, aCanStoreInRuleTree);

        item = item->mNext;
        MOZ_ASSERT(item, "Expected a eCSSUnit_List of odd length");
      }
      MOZ_ASSERT(!aResult.mMinTrackSizingFunctions.IsEmpty() &&
                 aResult.mMinTrackSizingFunctions.Length() ==
                 aResult.mMaxTrackSizingFunctions.Length() &&
                 aResult.mMinTrackSizingFunctions.Length() + 1 ==
                 aResult.mLineNameLists.Length(),
                 "Inconstistent array lengths for nsStyleGridTemplate");
    }
  }
}

static void
SetGridTemplateAreas(const nsCSSValue& aValue,
                     nsRefPtr<css::GridTemplateAreasValue>* aResult,
                     css::GridTemplateAreasValue* aParentValue,
                     bool& aCanStoreInRuleTree)
{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aCanStoreInRuleTree = false;
    *aResult = aParentValue;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
  case eCSSUnit_None:
    *aResult = nullptr;
    break;

  default:
    *aResult = aValue.GetGridTemplateAreas();
  }
}

static void
SetGridLine(const nsCSSValue& aValue,
            nsStyleGridLine& aResult,
            const nsStyleGridLine& aParentValue,
            bool& aCanStoreInRuleTree)

{
  switch (aValue.GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
    aCanStoreInRuleTree = false;
    aResult = aParentValue;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
  case eCSSUnit_Auto:
    aResult.SetAuto();
    break;

  default:
    aResult.SetAuto();  
    const nsCSSValueList* item = aValue.GetListValue();
    do {
      if (item->mValue.GetUnit() == eCSSUnit_Enumerated) {
        aResult.mHasSpan = true;
      } else if (item->mValue.GetUnit() == eCSSUnit_Integer) {
        aResult.mInteger = item->mValue.GetIntValue();
      } else if (item->mValue.GetUnit() == eCSSUnit_Ident) {
        item->mValue.GetStringValue(aResult.mLineName);
      } else {
        NS_ASSERTION(false, "Unexpected unit");
      }
      item = item->mNext;
    } while (item);
    MOZ_ASSERT(!aResult.IsAuto(),
               "should have set something away from default value");
  }
}

const void*
nsRuleNode::ComputePositionData(void* aStartStruct,
                                const nsRuleData* aRuleData,
                                nsStyleContext* aContext,
                                nsRuleNode* aHighestNode,
                                const RuleDetail aRuleDetail,
                                const bool aCanStoreInRuleTree)
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
                 SETCOORD_LPAH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC |
                   SETCOORD_UNSET_INITIAL,
                 aContext, mPresContext, canStoreInRuleTree)) {
      pos->mOffset.Set(side, coord);
    }
  }

  SetCoord(*aRuleData->ValueForWidth(), pos->mWidth, parentPos->mWidth,
           SETCOORD_LPAEH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);
  SetCoord(*aRuleData->ValueForMinWidth(), pos->mMinWidth, parentPos->mMinWidth,
           SETCOORD_LPAEH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);
  SetCoord(*aRuleData->ValueForMaxWidth(), pos->mMaxWidth, parentPos->mMaxWidth,
           SETCOORD_LPOEH | SETCOORD_INITIAL_NONE | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);

  
  
  
  
  const nsCSSValue* height = aRuleData->ValueForHeight();
  SetCoord(height->GetUnit() == eCSSUnit_Enumerated ?
             nsCSSValue(eCSSUnit_Unset) : *height,
           pos->mHeight, parentPos->mHeight,
           SETCOORD_LPAH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);
  const nsCSSValue* minHeight = aRuleData->ValueForMinHeight();
  SetCoord(minHeight->GetUnit() == eCSSUnit_Enumerated ?
             nsCSSValue(eCSSUnit_Unset) : *minHeight,
           pos->mMinHeight, parentPos->mMinHeight,
           SETCOORD_LPAH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);
  const nsCSSValue* maxHeight = aRuleData->ValueForMaxHeight();
  SetCoord(maxHeight->GetUnit() == eCSSUnit_Enumerated ?
             nsCSSValue(eCSSUnit_Unset) : *maxHeight,
           pos->mMaxHeight, parentPos->mMaxHeight,
           SETCOORD_LPOH | SETCOORD_INITIAL_NONE | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetDiscrete(*aRuleData->ValueForBoxSizing(),
              pos->mBoxSizing, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentPos->mBoxSizing,
              NS_STYLE_BOX_SIZING_CONTENT, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForAlignContent(),
              pos->mAlignContent, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentPos->mAlignContent,
              NS_STYLE_ALIGN_CONTENT_STRETCH, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForAlignItems(),
              pos->mAlignItems, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentPos->mAlignItems,
              NS_STYLE_ALIGN_ITEMS_INITIAL_VALUE, 0, 0, 0, 0);

  
  
  
  
  
  
  
  
  
  
  
  if (aRuleData->ValueForAlignSelf()->GetUnit() == eCSSUnit_Inherit) {
    
    
    
    uint8_t inheritedAlignSelf = parentPos->mAlignSelf;
    if (inheritedAlignSelf == NS_STYLE_ALIGN_SELF_AUTO) {
      if (!parentContext) {
        
        
        inheritedAlignSelf = NS_STYLE_ALIGN_ITEMS_INITIAL_VALUE;
      } else {
        
        
        nsStyleContext* grandparentContext = parentContext->GetParent();
        if (!grandparentContext) {
          
          
          inheritedAlignSelf = NS_STYLE_ALIGN_ITEMS_INITIAL_VALUE;
        } else {
          
          
          const nsStylePosition* grandparentPos =
            grandparentContext->StylePosition();
          inheritedAlignSelf = grandparentPos->mAlignItems;
          aContext->AddStyleBit(NS_STYLE_USES_GRANDANCESTOR_STYLE);
        }
      }
    }

    pos->mAlignSelf = inheritedAlignSelf;
    canStoreInRuleTree = false;
  } else {
    SetDiscrete(*aRuleData->ValueForAlignSelf(),
                pos->mAlignSelf, canStoreInRuleTree,
                SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
                parentPos->mAlignSelf, 
                NS_STYLE_ALIGN_SELF_AUTO, 
                0, 0, 0, 0);
  }

  
  
  SetCoord(*aRuleData->ValueForFlexBasis(), pos->mFlexBasis, parentPos->mFlexBasis,
           SETCOORD_LPAEH | SETCOORD_INITIAL_AUTO | SETCOORD_STORE_CALC |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetDiscrete(*aRuleData->ValueForFlexDirection(),
              pos->mFlexDirection, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentPos->mFlexDirection,
              NS_STYLE_FLEX_DIRECTION_ROW, 0, 0, 0, 0);

  
  SetFactor(*aRuleData->ValueForFlexGrow(),
            pos->mFlexGrow, canStoreInRuleTree,
            parentPos->mFlexGrow, 0.0f,
            SETFCT_UNSET_INITIAL);

  
  SetFactor(*aRuleData->ValueForFlexShrink(),
            pos->mFlexShrink, canStoreInRuleTree,
            parentPos->mFlexShrink, 1.0f,
            SETFCT_UNSET_INITIAL);

  
  SetDiscrete(*aRuleData->ValueForFlexWrap(),
              pos->mFlexWrap, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentPos->mFlexWrap,
              NS_STYLE_FLEX_WRAP_NOWRAP, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForOrder(),
              pos->mOrder, canStoreInRuleTree,
              SETDSC_INTEGER | SETDSC_UNSET_INITIAL,
              parentPos->mOrder,
              NS_STYLE_ORDER_INITIAL, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForJustifyContent(),
              pos->mJustifyContent, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentPos->mJustifyContent,
              NS_STYLE_JUSTIFY_CONTENT_FLEX_START, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForObjectFit(),
              pos->mObjectFit, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentPos->mObjectFit,
              NS_STYLE_OBJECT_FIT_FILL, 0, 0, 0, 0);

  
  const nsCSSValue& objectPosition = *aRuleData->ValueForObjectPosition();
  switch (objectPosition.GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_Inherit:
      canStoreInRuleTree = false;
      pos->mObjectPosition = parentPos->mObjectPosition;
      break;
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
      pos->mObjectPosition.SetInitialPercentValues(0.5f);
      break;
    default:
      ComputePositionValue(aContext, objectPosition,
                           pos->mObjectPosition, canStoreInRuleTree);
  }

  
  const nsCSSValue& gridAutoFlow = *aRuleData->ValueForGridAutoFlow();
  switch (gridAutoFlow.GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_Inherit:
      canStoreInRuleTree = false;
      pos->mGridAutoFlow = parentPos->mGridAutoFlow;
      break;
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
      pos->mGridAutoFlow = NS_STYLE_GRID_AUTO_FLOW_ROW;
      break;
    default:
      NS_ASSERTION(gridAutoFlow.GetUnit() == eCSSUnit_Enumerated,
                   "Unexpected unit");
      pos->mGridAutoFlow = gridAutoFlow.GetIntValue();
  }

  
  SetGridAutoColumnsRows(*aRuleData->ValueForGridAutoColumns(),
                         pos->mGridAutoColumnsMin,
                         pos->mGridAutoColumnsMax,
                         parentPos->mGridAutoColumnsMin,
                         parentPos->mGridAutoColumnsMax,
                         aContext, mPresContext, canStoreInRuleTree);

  
  SetGridAutoColumnsRows(*aRuleData->ValueForGridAutoRows(),
                         pos->mGridAutoRowsMin,
                         pos->mGridAutoRowsMax,
                         parentPos->mGridAutoRowsMin,
                         parentPos->mGridAutoRowsMax,
                         aContext, mPresContext, canStoreInRuleTree);

  
  SetGridTrackList(*aRuleData->ValueForGridTemplateColumns(),
                   pos->mGridTemplateColumns, parentPos->mGridTemplateColumns,
                   aContext, mPresContext, canStoreInRuleTree);

  
  SetGridTrackList(*aRuleData->ValueForGridTemplateRows(),
                   pos->mGridTemplateRows, parentPos->mGridTemplateRows,
                   aContext, mPresContext, canStoreInRuleTree);

  
  SetGridTemplateAreas(*aRuleData->ValueForGridTemplateAreas(),
                       &pos->mGridTemplateAreas,
                       parentPos->mGridTemplateAreas,
                       canStoreInRuleTree);

  
  SetGridLine(*aRuleData->ValueForGridColumnStart(),
              pos->mGridColumnStart,
              parentPos->mGridColumnStart,
              canStoreInRuleTree);

  
  SetGridLine(*aRuleData->ValueForGridColumnEnd(),
              pos->mGridColumnEnd,
              parentPos->mGridColumnEnd,
              canStoreInRuleTree);

  
  SetGridLine(*aRuleData->ValueForGridRowStart(),
              pos->mGridRowStart,
              parentPos->mGridRowStart,
              canStoreInRuleTree);

  
  SetGridLine(*aRuleData->ValueForGridRowEnd(),
              pos->mGridRowEnd,
              parentPos->mGridRowEnd,
              canStoreInRuleTree);

  
  const nsCSSValue* zIndexValue = aRuleData->ValueForZIndex();
  if (! SetCoord(*zIndexValue, pos->mZIndex, parentPos->mZIndex,
                 SETCOORD_IA | SETCOORD_INITIAL_AUTO | SETCOORD_UNSET_INITIAL,
                 aContext, nullptr, canStoreInRuleTree)) {
    if (eCSSUnit_Inherit == zIndexValue->GetUnit()) {
      
      canStoreInRuleTree = false;
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
                             const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Table, (), table, parentTable)

  
  SetDiscrete(*aRuleData->ValueForTableLayout(),
              table->mLayoutStrategy, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentTable->mLayoutStrategy,
              NS_STYLE_TABLE_LAYOUT_AUTO, 0, 0, 0, 0);

  
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
                                   const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(TableBorder, (), table, parentTable)

  
  SetDiscrete(*aRuleData->ValueForBorderCollapse(), table->mBorderCollapse,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentTable->mBorderCollapse,
              NS_STYLE_BORDER_SEPARATE, 0, 0, 0, 0);

  const nsCSSValue* borderSpacingValue = aRuleData->ValueForBorderSpacing();
  
  if (borderSpacingValue->GetUnit() != eCSSUnit_Null) {
    nsStyleCoord parentCol(parentTable->mBorderSpacingCol,
                           nsStyleCoord::CoordConstructor);
    nsStyleCoord parentRow(parentTable->mBorderSpacingRow,
                           nsStyleCoord::CoordConstructor);
    nsStyleCoord coordCol, coordRow;

#ifdef DEBUG
    bool result =
#endif
      SetPairCoords(*borderSpacingValue,
                    coordCol, coordRow, parentCol, parentRow,
                    SETCOORD_LH | SETCOORD_INITIAL_ZERO |
                      SETCOORD_CALC_LENGTH_ONLY |
                      SETCOORD_CALC_CLAMP_NONNEGATIVE | SETCOORD_UNSET_INHERIT,
                    aContext, mPresContext, canStoreInRuleTree);
    NS_ASSERTION(result, "malformed table border value");
    table->mBorderSpacingCol = coordCol.GetCoordValue();
    table->mBorderSpacingRow = coordRow.GetCoordValue();
  }

  
  SetDiscrete(*aRuleData->ValueForCaptionSide(),
              table->mCaptionSide, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentTable->mCaptionSide,
              NS_STYLE_CAPTION_SIDE_TOP, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForEmptyCells(),
              table->mEmptyCells, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentTable->mEmptyCells,
              NS_STYLE_TABLE_EMPTY_CELLS_SHOW,
              0, 0, 0, 0);

  COMPUTE_END_INHERITED(TableBorder, table)
}

const void*
nsRuleNode::ComputeContentData(void* aStartStruct,
                               const nsRuleData* aRuleData,
                               nsStyleContext* aContext,
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const bool aCanStoreInRuleTree)
{
  uint32_t count;
  nsAutoString buffer;

  COMPUTE_START_RESET(Content, (), content, parentContent)

  
  const nsCSSValue* contentValue = aRuleData->ValueForContent();
  switch (contentValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Normal:
  case eCSSUnit_None:
  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    
    content->AllocateContents(0);
    break;

  case eCSSUnit_Inherit:
    canStoreInRuleTree = false;
    count = parentContent->ContentCount();
    if (NS_SUCCEEDED(content->AllocateContents(count))) {
      while (0 < count--) {
        content->ContentAt(count) = parentContent->ContentAt(count);
      }
    }
    break;

  case eCSSUnit_Enumerated: {
    MOZ_ASSERT(contentValue->GetIntValue() == NS_STYLE_CONTENT_ALT_CONTENT,
               "unrecognized solitary content keyword");
    content->AllocateContents(1);
    nsStyleContentData& data = content->ContentAt(0);
    data.mType = eStyleContentType_AltContent;
    data.mContent.mString = nullptr;
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
              type = eStyleContentType_Uninitialized;
            }
            break;
          default:
            NS_ERROR("bad content type");
            type = eStyleContentType_Uninitialized;
          }
          data.mType = type;
          if (type == eStyleContentType_Image) {
            NS_SET_IMAGE_REQUEST_WITH_DOC(data.SetImage,
                                          aContext,
                                          value.GetImageValue);
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
            data.mContent.mString = nullptr;
          }
          contentValueList = contentValueList->mNext;
        }
      }
      break;
  }

  default:
    MOZ_ASSERT(false, "unrecognized content unit");
  }

  
  const nsCSSValue* counterIncrementValue =
    aRuleData->ValueForCounterIncrement();
  switch (counterIncrementValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_None:
  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    content->AllocateCounterIncrements(0);
    break;

  case eCSSUnit_Inherit:
    canStoreInRuleTree = false;
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
    MOZ_ASSERT(ourIncrement->mXValue.GetUnit() == eCSSUnit_Ident,
               "unexpected value unit");
    count = ListLength(ourIncrement);
    if (NS_FAILED(content->AllocateCounterIncrements(count))) {
      break;
    }

    count = 0;
    for (const nsCSSValuePairList* p = ourIncrement; p; p = p->mNext, count++) {
      int32_t increment;
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
    MOZ_ASSERT(false, "unexpected value unit");
  }

  
  const nsCSSValue* counterResetValue = aRuleData->ValueForCounterReset();
  switch (counterResetValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_None:
  case eCSSUnit_Initial:
  case eCSSUnit_Unset:
    content->AllocateCounterResets(0);
    break;

  case eCSSUnit_Inherit:
    canStoreInRuleTree = false;
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
    MOZ_ASSERT(ourReset->mXValue.GetUnit() == eCSSUnit_Ident,
               "unexpected value unit");
    count = ListLength(ourReset);
    if (NS_FAILED(content->AllocateCounterResets(count))) {
      break;
    }

    count = 0;
    for (const nsCSSValuePairList* p = ourReset; p; p = p->mNext, count++) {
      int32_t reset;
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
    MOZ_ASSERT(false, "unexpected value unit");
  }

  
  SetCoord(*aRuleData->ValueForMarkerOffset(), content->mMarkerOffset, parentContent->mMarkerOffset,
           SETCOORD_LH | SETCOORD_AUTO | SETCOORD_INITIAL_AUTO |
             SETCOORD_CALC_LENGTH_ONLY | SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);

  
  for (uint32_t i = 0; i < content->ContentCount(); ++i) {
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
                              const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Quotes, (), quotes, parentQuotes)

  
  const nsCSSValue* quotesValue = aRuleData->ValueForQuotes();
  switch (quotesValue->GetUnit()) {
  case eCSSUnit_Null:
    break;
  case eCSSUnit_Inherit:
  case eCSSUnit_Unset:
    canStoreInRuleTree = false;
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
    uint32_t count = ListLength(ourQuotes);
    if (NS_FAILED(quotes->AllocateQuotes(count))) {
      break;
    }
    count = 0;
    while (ourQuotes) {
      MOZ_ASSERT(ourQuotes->mXValue.GetUnit() == eCSSUnit_String &&
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
    MOZ_ASSERT(false, "unexpected value unit");
  }

  COMPUTE_END_INHERITED(Quotes, quotes)
}

const void*
nsRuleNode::ComputeXULData(void* aStartStruct,
                           const nsRuleData* aRuleData,
                           nsStyleContext* aContext,
                           nsRuleNode* aHighestNode,
                           const RuleDetail aRuleDetail,
                           const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(XUL, (), xul, parentXUL)

  
  SetDiscrete(*aRuleData->ValueForBoxAlign(),
              xul->mBoxAlign, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentXUL->mBoxAlign,
              NS_STYLE_BOX_ALIGN_STRETCH, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForBoxDirection(),
              xul->mBoxDirection, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentXUL->mBoxDirection,
              NS_STYLE_BOX_DIRECTION_NORMAL, 0, 0, 0, 0);

  
  SetFactor(*aRuleData->ValueForBoxFlex(),
            xul->mBoxFlex, canStoreInRuleTree,
            parentXUL->mBoxFlex, 0.0f,
            SETFCT_UNSET_INITIAL);

  
  SetDiscrete(*aRuleData->ValueForBoxOrient(),
              xul->mBoxOrient, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentXUL->mBoxOrient,
              NS_STYLE_BOX_ORIENT_HORIZONTAL, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForBoxPack(),
              xul->mBoxPack, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentXUL->mBoxPack,
              NS_STYLE_BOX_PACK_START, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForBoxOrdinalGroup(),
              xul->mBoxOrdinal, canStoreInRuleTree,
              SETDSC_INTEGER | SETDSC_UNSET_INITIAL,
              parentXUL->mBoxOrdinal, 1,
              0, 0, 0, 0);

  const nsCSSValue* stackSizingValue = aRuleData->ValueForStackSizing();
  if (eCSSUnit_Inherit == stackSizingValue->GetUnit()) {
    canStoreInRuleTree = false;
    xul->mStretchStack = parentXUL->mStretchStack;
  } else if (eCSSUnit_Initial == stackSizingValue->GetUnit() ||
             eCSSUnit_Unset == stackSizingValue->GetUnit()) {
    xul->mStretchStack = true;
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
                              const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Column, (mPresContext), column, parent)

  
  SetCoord(*aRuleData->ValueForColumnWidth(),
           column->mColumnWidth, parent->mColumnWidth,
           SETCOORD_LAH | SETCOORD_INITIAL_AUTO |
             SETCOORD_CALC_LENGTH_ONLY | SETCOORD_CALC_CLAMP_NONNEGATIVE |
             SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetCoord(*aRuleData->ValueForColumnGap(),
           column->mColumnGap, parent->mColumnGap,
           SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL |
             SETCOORD_CALC_LENGTH_ONLY | SETCOORD_UNSET_INITIAL,
           aContext, mPresContext, canStoreInRuleTree);
  
  if (column->mColumnGap.GetUnit() == eStyleUnit_Coord) {
    column->mColumnGap.SetCoordValue(
      std::max(column->mColumnGap.GetCoordValue(), 0));
  }

  
  const nsCSSValue* columnCountValue = aRuleData->ValueForColumnCount();
  if (eCSSUnit_Auto == columnCountValue->GetUnit() ||
      eCSSUnit_Initial == columnCountValue->GetUnit() ||
      eCSSUnit_Unset == columnCountValue->GetUnit()) {
    column->mColumnCount = NS_STYLE_COLUMN_COUNT_AUTO;
  } else if (eCSSUnit_Integer == columnCountValue->GetUnit()) {
    column->mColumnCount = columnCountValue->GetIntValue();
    
    column->mColumnCount = std::min(column->mColumnCount,
                                    nsStyleColumn::kMaxColumnCount);
  } else if (eCSSUnit_Inherit == columnCountValue->GetUnit()) {
    canStoreInRuleTree = false;
    column->mColumnCount = parent->mColumnCount;
  }

  
  const nsCSSValue& widthValue = *aRuleData->ValueForColumnRuleWidth();
  if (eCSSUnit_Initial == widthValue.GetUnit() ||
      eCSSUnit_Unset == widthValue.GetUnit()) {
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
    canStoreInRuleTree = false;
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
  MOZ_ASSERT(eCSSUnit_None != styleValue.GetUnit(),
             "'none' should be handled as enumerated value");
  if (eCSSUnit_Enumerated == styleValue.GetUnit()) {
    column->mColumnRuleStyle = styleValue.GetIntValue();
  }
  else if (eCSSUnit_Initial == styleValue.GetUnit() ||
           eCSSUnit_Unset == styleValue.GetUnit()) {
    column->mColumnRuleStyle = NS_STYLE_BORDER_STYLE_NONE;
  }
  else if (eCSSUnit_Inherit == styleValue.GetUnit()) {
    canStoreInRuleTree = false;
    column->mColumnRuleStyle = parent->mColumnRuleStyle;
  }

  
  const nsCSSValue& colorValue = *aRuleData->ValueForColumnRuleColor();
  if (eCSSUnit_Inherit == colorValue.GetUnit()) {
    canStoreInRuleTree = false;
    column->mColumnRuleColorIsForeground = false;
    if (parent->mColumnRuleColorIsForeground) {
      if (parentContext) {
        column->mColumnRuleColor = parentContext->StyleColor()->mColor;
      } else {
        nsStyleColor defaultColumnRuleColor(mPresContext);
        column->mColumnRuleColor = defaultColumnRuleColor.mColor;
      }
    } else {
      column->mColumnRuleColor = parent->mColumnRuleColor;
    }
  }
  else if (eCSSUnit_Initial == colorValue.GetUnit() ||
           eCSSUnit_Unset == colorValue.GetUnit() ||
           eCSSUnit_Enumerated == colorValue.GetUnit()) {
    column->mColumnRuleColorIsForeground = true;
  }
  else if (SetColor(colorValue, 0, mPresContext, aContext,
                    column->mColumnRuleColor, canStoreInRuleTree)) {
    column->mColumnRuleColorIsForeground = false;
  }

  
  SetDiscrete(*aRuleData->ValueForColumnFill(),
                column->mColumnFill, canStoreInRuleTree,
                SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
                parent->mColumnFill,
                NS_STYLE_COLUMN_FILL_BALANCE,
                0, 0, 0, 0);

  COMPUTE_END_RESET(Column, column)
}

static void
SetSVGPaint(const nsCSSValue& aValue, const nsStyleSVGPaint& parentPaint,
            nsPresContext* aPresContext, nsStyleContext *aContext,
            nsStyleSVGPaint& aResult, nsStyleSVGPaintType aInitialPaintType,
            bool& aCanStoreInRuleTree)
{
  nscolor color;

  if (aValue.GetUnit() == eCSSUnit_Inherit ||
      aValue.GetUnit() == eCSSUnit_Unset) {
    aResult = parentPaint;
    aCanStoreInRuleTree = false;
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

    if (pair.mXValue.GetUnit() == eCSSUnit_URL) {
      aResult.SetType(eStyleSVGPaintType_Server);
      aResult.mPaint.mPaintServer = pair.mXValue.GetURLValue();
      NS_IF_ADDREF(aResult.mPaint.mPaintServer);
    } else if (pair.mXValue.GetUnit() == eCSSUnit_Enumerated) {

      switch (pair.mXValue.GetIntValue()) {
      case NS_COLOR_CONTEXT_FILL:
        aResult.SetType(eStyleSVGPaintType_ContextFill);
        break;
      case NS_COLOR_CONTEXT_STROKE:
        aResult.SetType(eStyleSVGPaintType_ContextStroke);
        break;
      default:
        NS_NOTREACHED("unknown keyword as paint server value");
      }

    } else {
      NS_NOTREACHED("malformed paint server value");
    }

    if (pair.mYValue.GetUnit() == eCSSUnit_None) {
      aResult.mFallbackColor = NS_RGBA(0, 0, 0, 0);
    } else {
      MOZ_ASSERT(pair.mYValue.GetUnit() != eCSSUnit_Inherit,
                 "cannot inherit fallback colour");
      SetColor(pair.mYValue, NS_RGB(0, 0, 0), aPresContext, aContext,
               aResult.mFallbackColor, aCanStoreInRuleTree);
    }
  } else {
    MOZ_ASSERT(aValue.GetUnit() == eCSSUnit_Null,
               "malformed paint server value");
  }
}

static void
SetSVGOpacity(const nsCSSValue& aValue,
              float& aOpacityField, nsStyleSVGOpacitySource& aOpacityTypeField,
              bool& aCanStoreInRuleTree,
              float aParentOpacity, nsStyleSVGOpacitySource aParentOpacityType)
{
  if (eCSSUnit_Enumerated == aValue.GetUnit()) {
    switch (aValue.GetIntValue()) {
    case NS_STYLE_CONTEXT_FILL_OPACITY:
      aOpacityTypeField = eStyleSVGOpacitySource_ContextFillOpacity;
      break;
    case NS_STYLE_CONTEXT_STROKE_OPACITY:
      aOpacityTypeField = eStyleSVGOpacitySource_ContextStrokeOpacity;
      break;
    default:
      NS_NOTREACHED("SetSVGOpacity: Unknown keyword");
    }
    
    aOpacityField = 1.0f;
  } else if (eCSSUnit_Inherit == aValue.GetUnit() ||
             eCSSUnit_Unset == aValue.GetUnit()) {
    aCanStoreInRuleTree = false;
    aOpacityField = aParentOpacity;
    aOpacityTypeField = aParentOpacityType;
  } else if (eCSSUnit_Null != aValue.GetUnit()) {
    SetFactor(aValue, aOpacityField, aCanStoreInRuleTree,
              aParentOpacity, 1.0f, SETFCT_OPACITY);
    aOpacityTypeField = eStyleSVGOpacitySource_Normal;
  }
}

const void*
nsRuleNode::ComputeSVGData(void* aStartStruct,
                           const nsRuleData* aRuleData,
                           nsStyleContext* aContext,
                           nsRuleNode* aHighestNode,
                           const RuleDetail aRuleDetail,
                           const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(SVG, (), svg, parentSVG)

  
  SetDiscrete(*aRuleData->ValueForClipRule(),
              svg->mClipRule, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mClipRule,
              NS_STYLE_FILL_RULE_NONZERO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForColorInterpolation(),
              svg->mColorInterpolation, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mColorInterpolation,
              NS_STYLE_COLOR_INTERPOLATION_SRGB, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForColorInterpolationFilters(),
              svg->mColorInterpolationFilters, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mColorInterpolationFilters,
              NS_STYLE_COLOR_INTERPOLATION_LINEARRGB, 0, 0, 0, 0);

  
  SetSVGPaint(*aRuleData->ValueForFill(),
              parentSVG->mFill, mPresContext, aContext,
              svg->mFill, eStyleSVGPaintType_Color, canStoreInRuleTree);

  
  
  nsStyleSVGOpacitySource contextFillOpacity = svg->mFillOpacitySource;
  SetSVGOpacity(*aRuleData->ValueForFillOpacity(),
                svg->mFillOpacity, contextFillOpacity, canStoreInRuleTree,
                parentSVG->mFillOpacity, parentSVG->mFillOpacitySource);
  svg->mFillOpacitySource = contextFillOpacity;

  
  SetDiscrete(*aRuleData->ValueForFillRule(),
              svg->mFillRule, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mFillRule,
              NS_STYLE_FILL_RULE_NONZERO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForImageRendering(),
              svg->mImageRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mImageRendering,
              NS_STYLE_IMAGE_RENDERING_AUTO, 0, 0, 0, 0);

  
  const nsCSSValue* markerEndValue = aRuleData->ValueForMarkerEnd();
  if (eCSSUnit_URL == markerEndValue->GetUnit()) {
    svg->mMarkerEnd = markerEndValue->GetURLValue();
  } else if (eCSSUnit_None == markerEndValue->GetUnit() ||
             eCSSUnit_Initial == markerEndValue->GetUnit()) {
    svg->mMarkerEnd = nullptr;
  } else if (eCSSUnit_Inherit == markerEndValue->GetUnit() ||
             eCSSUnit_Unset == markerEndValue->GetUnit()) {
    canStoreInRuleTree = false;
    svg->mMarkerEnd = parentSVG->mMarkerEnd;
  }

  
  const nsCSSValue* markerMidValue = aRuleData->ValueForMarkerMid();
  if (eCSSUnit_URL == markerMidValue->GetUnit()) {
    svg->mMarkerMid = markerMidValue->GetURLValue();
  } else if (eCSSUnit_None == markerMidValue->GetUnit() ||
             eCSSUnit_Initial == markerMidValue->GetUnit()) {
    svg->mMarkerMid = nullptr;
  } else if (eCSSUnit_Inherit == markerMidValue->GetUnit() ||
             eCSSUnit_Unset == markerMidValue->GetUnit()) {
    canStoreInRuleTree = false;
    svg->mMarkerMid = parentSVG->mMarkerMid;
  }

  
  const nsCSSValue* markerStartValue = aRuleData->ValueForMarkerStart();
  if (eCSSUnit_URL == markerStartValue->GetUnit()) {
    svg->mMarkerStart = markerStartValue->GetURLValue();
  } else if (eCSSUnit_None == markerStartValue->GetUnit() ||
             eCSSUnit_Initial == markerStartValue->GetUnit()) {
    svg->mMarkerStart = nullptr;
  } else if (eCSSUnit_Inherit == markerStartValue->GetUnit() ||
             eCSSUnit_Unset == markerStartValue->GetUnit()) {
    canStoreInRuleTree = false;
    svg->mMarkerStart = parentSVG->mMarkerStart;
  }

  
  const nsCSSValue* paintOrderValue = aRuleData->ValueForPaintOrder();
  switch (paintOrderValue->GetUnit()) {
    case eCSSUnit_Null:
      break;

    case eCSSUnit_Enumerated:
      static_assert
        (NS_STYLE_PAINT_ORDER_BITWIDTH * NS_STYLE_PAINT_ORDER_LAST_VALUE <= 8,
         "SVGStyleStruct::mPaintOrder not big enough");
      svg->mPaintOrder = static_cast<uint8_t>(paintOrderValue->GetIntValue());
      break;

    case eCSSUnit_Inherit:
    case eCSSUnit_Unset:
      canStoreInRuleTree = false;
      svg->mPaintOrder = parentSVG->mPaintOrder;
      break;

    case eCSSUnit_Initial:
      svg->mPaintOrder = NS_STYLE_PAINT_ORDER_NORMAL;
      break;

    default:
      NS_NOTREACHED("unexpected unit");
  }

  
  SetDiscrete(*aRuleData->ValueForShapeRendering(),
              svg->mShapeRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mShapeRendering,
              NS_STYLE_SHAPE_RENDERING_AUTO, 0, 0, 0, 0);

  
  SetSVGPaint(*aRuleData->ValueForStroke(),
              parentSVG->mStroke, mPresContext, aContext,
              svg->mStroke, eStyleSVGPaintType_None, canStoreInRuleTree);

  
  const nsCSSValue* strokeDasharrayValue = aRuleData->ValueForStrokeDasharray();
  switch (strokeDasharrayValue->GetUnit()) {
  case eCSSUnit_Null:
    break;

  case eCSSUnit_Inherit:
  case eCSSUnit_Unset:
    canStoreInRuleTree = false;
    svg->mStrokeDasharrayFromObject = parentSVG->mStrokeDasharrayFromObject;
    
    
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

  case eCSSUnit_Enumerated:
    MOZ_ASSERT(strokeDasharrayValue->GetIntValue() ==
                     NS_STYLE_STROKE_PROP_CONTEXT_VALUE,
               "Unknown keyword for stroke-dasharray");
    svg->mStrokeDasharrayFromObject = true;
    delete [] svg->mStrokeDasharray;
    svg->mStrokeDasharray = nullptr;
    svg->mStrokeDasharrayLength = 0;
    break;

  case eCSSUnit_Initial:
  case eCSSUnit_None:
    svg->mStrokeDasharrayFromObject = false;
    delete [] svg->mStrokeDasharray;
    svg->mStrokeDasharray = nullptr;
    svg->mStrokeDasharrayLength = 0;
    break;

  case eCSSUnit_List:
  case eCSSUnit_ListDep: {
    svg->mStrokeDasharrayFromObject = false;
    delete [] svg->mStrokeDasharray;
    svg->mStrokeDasharray = nullptr;
    svg->mStrokeDasharrayLength = 0;

    
    const nsCSSValueList *value = strokeDasharrayValue->GetListValue();
    svg->mStrokeDasharrayLength = ListLength(value);

    NS_ASSERTION(svg->mStrokeDasharrayLength != 0, "no dasharray items");

    svg->mStrokeDasharray = new nsStyleCoord[svg->mStrokeDasharrayLength];

    if (svg->mStrokeDasharray) {
      uint32_t i = 0;
      while (nullptr != value) {
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
    MOZ_ASSERT(false, "unrecognized dasharray unit");
  }

  
  const nsCSSValue *strokeDashoffsetValue =
    aRuleData->ValueForStrokeDashoffset();
  svg->mStrokeDashoffsetFromObject =
    strokeDashoffsetValue->GetUnit() == eCSSUnit_Enumerated &&
    strokeDashoffsetValue->GetIntValue() == NS_STYLE_STROKE_PROP_CONTEXT_VALUE;
  if (svg->mStrokeDashoffsetFromObject) {
    svg->mStrokeDashoffset.SetCoordValue(0);
  } else {
    SetCoord(*aRuleData->ValueForStrokeDashoffset(),
             svg->mStrokeDashoffset, parentSVG->mStrokeDashoffset,
             SETCOORD_LPH | SETCOORD_FACTOR | SETCOORD_INITIAL_ZERO |
               SETCOORD_UNSET_INHERIT,
             aContext, mPresContext, canStoreInRuleTree);
  }

  
  SetDiscrete(*aRuleData->ValueForStrokeLinecap(),
              svg->mStrokeLinecap, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mStrokeLinecap,
              NS_STYLE_STROKE_LINECAP_BUTT, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForStrokeLinejoin(),
              svg->mStrokeLinejoin, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mStrokeLinejoin,
              NS_STYLE_STROKE_LINEJOIN_MITER, 0, 0, 0, 0);

  
  SetFactor(*aRuleData->ValueForStrokeMiterlimit(),
            svg->mStrokeMiterlimit,
            canStoreInRuleTree,
            parentSVG->mStrokeMiterlimit, 4.0f,
            SETFCT_UNSET_INHERIT);

  
  nsStyleSVGOpacitySource contextStrokeOpacity = svg->mStrokeOpacitySource;
  SetSVGOpacity(*aRuleData->ValueForStrokeOpacity(),
                svg->mStrokeOpacity, contextStrokeOpacity, canStoreInRuleTree,
                parentSVG->mStrokeOpacity, parentSVG->mStrokeOpacitySource);
  svg->mStrokeOpacitySource = contextStrokeOpacity;

  
  const nsCSSValue* strokeWidthValue = aRuleData->ValueForStrokeWidth();
  switch (strokeWidthValue->GetUnit()) {
  case eCSSUnit_Enumerated:
    MOZ_ASSERT(strokeWidthValue->GetIntValue() ==
                 NS_STYLE_STROKE_PROP_CONTEXT_VALUE,
               "Unrecognized keyword for stroke-width");
    svg->mStrokeWidthFromObject = true;
    svg->mStrokeWidth.SetCoordValue(nsPresContext::CSSPixelsToAppUnits(1));
    break;

  case eCSSUnit_Initial:
    svg->mStrokeWidthFromObject = false;
    svg->mStrokeWidth.SetCoordValue(nsPresContext::CSSPixelsToAppUnits(1));
    break;

  default:
    svg->mStrokeWidthFromObject = false;
    SetCoord(*strokeWidthValue,
             svg->mStrokeWidth, parentSVG->mStrokeWidth,
             SETCOORD_LPH | SETCOORD_FACTOR | SETCOORD_UNSET_INHERIT,
             aContext, mPresContext, canStoreInRuleTree);
  }

  
  SetDiscrete(*aRuleData->ValueForTextAnchor(),
              svg->mTextAnchor, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mTextAnchor,
              NS_STYLE_TEXT_ANCHOR_START, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForTextRendering(),
              svg->mTextRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INHERIT,
              parentSVG->mTextRendering,
              NS_STYLE_TEXT_RENDERING_AUTO, 0, 0, 0, 0);

  COMPUTE_END_INHERITED(SVG, svg)
}

void
nsRuleNode::SetStyleClipPathToCSSValue(nsStyleClipPath* aStyleClipPath,
                                       const nsCSSValue* aValue,
                                       nsStyleContext* aStyleContext,
                                       nsPresContext* aPresContext,
                                       bool& aCanStoreInRuleTree)
{
  MOZ_ASSERT(aValue->GetUnit() != eCSSUnit_ListDep ||
             aValue->GetUnit() != eCSSUnit_List,
             "expected a basic shape or reference box");

  const nsCSSValueList* cur = aValue->GetListValue();

  uint8_t sizingBox = NS_STYLE_CLIP_SHAPE_SIZING_NOBOX;
  nsStyleBasicShape* basicShape = nullptr;
  for (unsigned i = 0; i < 2; ++i) {
    if (!cur) {
      break;
    }
    if (cur->mValue.GetUnit() == eCSSUnit_Function) {
      nsCSSValue::Array* shapeFunction = cur->mValue.GetArrayValue();
      nsCSSKeyword functionName =
        (nsCSSKeyword)shapeFunction->Item(0).GetIntValue();
      if (functionName == eCSSKeyword_polygon) {
        MOZ_ASSERT(!basicShape, "did not expect value");
        basicShape = new nsStyleBasicShape(nsStyleBasicShape::ePolygon);
        MOZ_ASSERT(shapeFunction->Count() > 1,
                   "polygon has wrong number of arguments");
        size_t j = 1;
        if (shapeFunction->Item(j).GetUnit() == eCSSUnit_Enumerated) {
          basicShape->SetFillRule(shapeFunction->Item(j).GetIntValue());
          ++j;
        }
        const int32_t mask = SETCOORD_PERCENT | SETCOORD_LENGTH |
                             SETCOORD_STORE_CALC;
        const nsCSSValuePairList* curPair =
          shapeFunction->Item(j).GetPairListValue();
        nsTArray<nsStyleCoord>& coordinates = basicShape->Coordinates();
        while (curPair) {
          nsStyleCoord xCoord, yCoord;
          DebugOnly<bool> didSetCoordX = SetCoord(curPair->mXValue, xCoord,
                                                  nsStyleCoord(), mask,
                                                  aStyleContext, aPresContext,
                                                  aCanStoreInRuleTree);
          coordinates.AppendElement(xCoord);
          MOZ_ASSERT(didSetCoordX, "unexpected x coordinate unit");
          DebugOnly<bool> didSetCoordY = SetCoord(curPair->mYValue, yCoord,
                                                  nsStyleCoord(), mask,
                                                  aStyleContext, aPresContext,
                                                  aCanStoreInRuleTree);
          coordinates.AppendElement(yCoord);
          MOZ_ASSERT(didSetCoordY, "unexpected y coordinate unit");
          curPair = curPair->mNext;
        }
      } else if (functionName == eCSSKeyword_circle ||
                 functionName == eCSSKeyword_ellipse) {
        nsStyleBasicShape::Type type = functionName == eCSSKeyword_circle ?
                                       nsStyleBasicShape::eCircle :
                                       nsStyleBasicShape::eEllipse;
        MOZ_ASSERT(!basicShape, "did not expect value");
        basicShape = new nsStyleBasicShape(type);
        const int32_t mask = SETCOORD_PERCENT | SETCOORD_LENGTH |
                             SETCOORD_STORE_CALC | SETCOORD_ENUMERATED;
        size_t count = type == nsStyleBasicShape::eCircle ? 2 : 3;
        MOZ_ASSERT(shapeFunction->Count() == count + 1,
                   "unexpected arguments count");
        MOZ_ASSERT(type == nsStyleBasicShape::eCircle ||
                   (shapeFunction->Item(1).GetUnit() == eCSSUnit_Null) ==
                   (shapeFunction->Item(2).GetUnit() == eCSSUnit_Null),
                   "ellipse should have two radii or none");
        for (size_t j = 1; j < count; ++j) {
          const nsCSSValue& val = shapeFunction->Item(j);
          nsStyleCoord radius;
          if (val.GetUnit() != eCSSUnit_Null) {
            DebugOnly<bool> didSetRadius = SetCoord(val, radius,
                                                    nsStyleCoord(), mask,
                                                    aStyleContext,
                                                    aPresContext,
                                                    aCanStoreInRuleTree);
            MOZ_ASSERT(didSetRadius, "unexpected radius unit");
          } else {
            radius.SetIntValue(NS_RADIUS_CLOSEST_SIDE, eStyleUnit_Enumerated);
          }
          basicShape->Coordinates().AppendElement(radius);
        }
        const nsCSSValue& positionVal = shapeFunction->Item(count);
        if (positionVal.GetUnit() == eCSSUnit_Array) {
          ComputePositionValue(aStyleContext, positionVal,
                               basicShape->GetPosition(),
                               aCanStoreInRuleTree);
        } else {
            MOZ_ASSERT(positionVal.GetUnit() == eCSSUnit_Null,
                       "expected no value");
        }
      } else if (functionName == eCSSKeyword_inset) {
        MOZ_ASSERT(!basicShape, "did not expect value");
        basicShape = new nsStyleBasicShape(nsStyleBasicShape::eInset);
        MOZ_ASSERT(shapeFunction->Count() == 6,
                   "inset function has wrong number of arguments");
        MOZ_ASSERT(shapeFunction->Item(1).GetUnit() != eCSSUnit_Null,
                   "no shape arguments defined");
        const int32_t mask = SETCOORD_PERCENT | SETCOORD_LENGTH |
                             SETCOORD_STORE_CALC;
        nsTArray<nsStyleCoord>& coords = basicShape->Coordinates();
        for (size_t j = 1; j <= 4; ++j) {
          const nsCSSValue& val = shapeFunction->Item(j);
          nsStyleCoord inset;
          
          if (val.GetUnit() == eCSSUnit_Null) {
            if (j == 4) {
              inset = coords[1];
            } else {
              MOZ_ASSERT(j != 1, "first argument not specified");
              inset = coords[0];
            }
          } else {
            DebugOnly<bool> didSetInset = SetCoord(val, inset,
                                                   nsStyleCoord(), mask,
                                                   aStyleContext, aPresContext,
                                                   aCanStoreInRuleTree);
            MOZ_ASSERT(didSetInset, "unexpected inset unit");
          }
          coords.AppendElement(inset);
        }

        nsStyleCorners& insetRadius = basicShape->GetRadius();
        if (shapeFunction->Item(5).GetUnit() == eCSSUnit_Array) {
          nsCSSValue::Array* radiiArray = shapeFunction->Item(5).GetArrayValue();
          NS_FOR_CSS_FULL_CORNERS(corner) {
            int cx = NS_FULL_TO_HALF_CORNER(corner, false);
            int cy = NS_FULL_TO_HALF_CORNER(corner, true);
            const nsCSSValue& radius = radiiArray->Item(corner);
            nsStyleCoord coordX, coordY;
            DebugOnly<bool> didSetRadii = SetPairCoords(radius, coordX, coordY,
                                                        nsStyleCoord(),
                                                        nsStyleCoord(), mask,
                                                        aStyleContext,
                                                        aPresContext,
                                                        aCanStoreInRuleTree);
            MOZ_ASSERT(didSetRadii, "unexpected radius unit");
            insetRadius.Set(cx, coordX);
            insetRadius.Set(cy, coordY);
          }
        } else {
          MOZ_ASSERT(shapeFunction->Item(5).GetUnit() == eCSSUnit_Null,
                     "unexpected value");
          
          nsStyleCoord zero;
          zero.SetCoordValue(0);
          NS_FOR_CSS_HALF_CORNERS(j) {
            insetRadius.Set(j, zero);
          }
        }
      } else {
        NS_NOTREACHED("unexpected basic shape function");
        return;
      }
    } else if (cur->mValue.GetUnit() == eCSSUnit_Enumerated) {
      int32_t type = cur->mValue.GetIntValue();
      if (type > NS_STYLE_CLIP_SHAPE_SIZING_VIEW ||
          type < NS_STYLE_CLIP_SHAPE_SIZING_NOBOX) {
        NS_NOTREACHED("unexpected reference box");
        return;
      }
      sizingBox = (uint8_t)type;
    } else {
      NS_NOTREACHED("unexpected value");
      return;
    }
    cur = cur->mNext;
  }

  if (basicShape) {
    aStyleClipPath->SetBasicShape(basicShape, sizingBox);
  } else {
    aStyleClipPath->SetSizingBox(sizingBox);
  }
}


bool
nsRuleNode::SetStyleFilterToCSSValue(nsStyleFilter* aStyleFilter,
                                     const nsCSSValue& aValue,
                                     nsStyleContext* aStyleContext,
                                     nsPresContext* aPresContext,
                                     bool& aCanStoreInRuleTree)
{
  nsCSSUnit unit = aValue.GetUnit();
  if (unit == eCSSUnit_URL) {
    nsIURI* url = aValue.GetURLValue();
    if (!url)
      return false;
    aStyleFilter->SetURL(url);
    return true;
  }

  MOZ_ASSERT(unit == eCSSUnit_Function, "expected a filter function");

  nsCSSValue::Array* filterFunction = aValue.GetArrayValue();
  nsCSSKeyword functionName =
    (nsCSSKeyword)filterFunction->Item(0).GetIntValue();

  int32_t type;
  DebugOnly<bool> foundKeyword =
    nsCSSProps::FindKeyword(functionName,
                            nsCSSProps::kFilterFunctionKTable,
                            type);
  MOZ_ASSERT(foundKeyword, "unknown filter type");
  if (type == NS_STYLE_FILTER_DROP_SHADOW) {
    nsRefPtr<nsCSSShadowArray> shadowArray = GetShadowData(
      filterFunction->Item(1).GetListValue(),
      aStyleContext,
      false,
      aCanStoreInRuleTree);
    aStyleFilter->SetDropShadow(shadowArray);
    return true;
  }

  int32_t mask = SETCOORD_PERCENT | SETCOORD_FACTOR;
  if (type == NS_STYLE_FILTER_BLUR) {
    mask = SETCOORD_LENGTH |
           SETCOORD_CALC_LENGTH_ONLY |
           SETCOORD_CALC_CLAMP_NONNEGATIVE;
  } else if (type == NS_STYLE_FILTER_HUE_ROTATE) {
    mask = SETCOORD_ANGLE;
  }

  MOZ_ASSERT(filterFunction->Count() == 2,
             "all filter functions should have exactly one argument");

  nsCSSValue& arg = filterFunction->Item(1);
  nsStyleCoord filterParameter;
  DebugOnly<bool> didSetCoord = SetCoord(arg, filterParameter,
                                         nsStyleCoord(), mask,
                                         aStyleContext, aPresContext,
                                         aCanStoreInRuleTree);
  aStyleFilter->SetFilterParameter(filterParameter, type);
  MOZ_ASSERT(didSetCoord, "unexpected unit");
  return true;
}

const void*
nsRuleNode::ComputeSVGResetData(void* aStartStruct,
                                const nsRuleData* aRuleData,
                                nsStyleContext* aContext,
                                nsRuleNode* aHighestNode,
                                const RuleDetail aRuleDetail,
                                const bool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(SVGReset, (), svgReset, parentSVGReset)

  
  const nsCSSValue* stopColorValue = aRuleData->ValueForStopColor();
  if (eCSSUnit_Initial == stopColorValue->GetUnit() ||
      eCSSUnit_Unset == stopColorValue->GetUnit()) {
    svgReset->mStopColor = NS_RGB(0, 0, 0);
  } else {
    SetColor(*stopColorValue, parentSVGReset->mStopColor,
             mPresContext, aContext, svgReset->mStopColor, canStoreInRuleTree);
  }

  
  const nsCSSValue* floodColorValue = aRuleData->ValueForFloodColor();
  if (eCSSUnit_Initial == floodColorValue->GetUnit() ||
      eCSSUnit_Unset == floodColorValue->GetUnit()) {
    svgReset->mFloodColor = NS_RGB(0, 0, 0);
  } else {
    SetColor(*floodColorValue, parentSVGReset->mFloodColor,
             mPresContext, aContext, svgReset->mFloodColor, canStoreInRuleTree);
  }

  
  const nsCSSValue* lightingColorValue = aRuleData->ValueForLightingColor();
  if (eCSSUnit_Initial == lightingColorValue->GetUnit() ||
      eCSSUnit_Unset == lightingColorValue->GetUnit()) {
    svgReset->mLightingColor = NS_RGB(255, 255, 255);
  } else {
    SetColor(*lightingColorValue, parentSVGReset->mLightingColor,
             mPresContext, aContext, svgReset->mLightingColor,
             canStoreInRuleTree);
  }

  
  const nsCSSValue* clipPathValue = aRuleData->ValueForClipPath();
  switch (clipPathValue->GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_None:
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
      svgReset->mClipPath = nsStyleClipPath();
      break;
    case eCSSUnit_Inherit:
      canStoreInRuleTree = false;
      svgReset->mClipPath = parentSVGReset->mClipPath;
      break;
    case eCSSUnit_URL: {
      svgReset->mClipPath = nsStyleClipPath();
      nsIURI* url = clipPathValue->GetURLValue();
      if (url) {
        svgReset->mClipPath.SetURL(url);
      }
      break;
    }
    case eCSSUnit_List:
    case eCSSUnit_ListDep: {
      svgReset->mClipPath = nsStyleClipPath();
      SetStyleClipPathToCSSValue(&svgReset->mClipPath, clipPathValue, aContext,
                                 mPresContext, canStoreInRuleTree);
      break;
    }
    default:
      NS_NOTREACHED("unexpected unit");
  }

  
  SetFactor(*aRuleData->ValueForStopOpacity(),
            svgReset->mStopOpacity, canStoreInRuleTree,
            parentSVGReset->mStopOpacity, 1.0f,
            SETFCT_OPACITY | SETFCT_UNSET_INITIAL);

  
  SetFactor(*aRuleData->ValueForFloodOpacity(),
            svgReset->mFloodOpacity, canStoreInRuleTree,
            parentSVGReset->mFloodOpacity, 1.0f,
            SETFCT_OPACITY | SETFCT_UNSET_INITIAL);

  
  SetDiscrete(*aRuleData->ValueForDominantBaseline(),
              svgReset->mDominantBaseline,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentSVGReset->mDominantBaseline,
              NS_STYLE_DOMINANT_BASELINE_AUTO, 0, 0, 0, 0);

  
  SetDiscrete(*aRuleData->ValueForVectorEffect(),
              svgReset->mVectorEffect,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentSVGReset->mVectorEffect,
              NS_STYLE_VECTOR_EFFECT_NONE, 0, 0, 0, 0);

  
  const nsCSSValue* filterValue = aRuleData->ValueForFilter();
  switch (filterValue->GetUnit()) {
    case eCSSUnit_Null:
      break;
    case eCSSUnit_None:
    case eCSSUnit_Initial:
    case eCSSUnit_Unset:
      svgReset->mFilters.Clear();
      break;
    case eCSSUnit_Inherit:
      canStoreInRuleTree = false;
      svgReset->mFilters = parentSVGReset->mFilters;
      break;
    case eCSSUnit_List:
    case eCSSUnit_ListDep: {
      svgReset->mFilters.Clear();
      const nsCSSValueList* cur = filterValue->GetListValue();
      while (cur) {
        nsStyleFilter styleFilter;
        if (!SetStyleFilterToCSSValue(&styleFilter, cur->mValue, aContext,
                                      mPresContext, canStoreInRuleTree)) {
          svgReset->mFilters.Clear();
          break;
        }
        MOZ_ASSERT(styleFilter.GetType() != NS_STYLE_FILTER_NONE,
                   "filter should be set");
        svgReset->mFilters.AppendElement(styleFilter);
        cur = cur->mNext;
      }
      break;
    }
    default:
      NS_NOTREACHED("unexpected unit");
  }

  
  const nsCSSValue* maskValue = aRuleData->ValueForMask();
  if (eCSSUnit_URL == maskValue->GetUnit()) {
    svgReset->mMask = maskValue->GetURLValue();
  } else if (eCSSUnit_None == maskValue->GetUnit() ||
             eCSSUnit_Initial == maskValue->GetUnit() ||
             eCSSUnit_Unset == maskValue->GetUnit()) {
    svgReset->mMask = nullptr;
  } else if (eCSSUnit_Inherit == maskValue->GetUnit()) {
    canStoreInRuleTree = false;
    svgReset->mMask = parentSVGReset->mMask;
  }

  
  SetDiscrete(*aRuleData->ValueForMaskType(),
              svgReset->mMaskType,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_UNSET_INITIAL,
              parentSVGReset->mMaskType,
              NS_STYLE_MASK_TYPE_LUMINANCE, 0, 0, 0, 0);

  COMPUTE_END_RESET(SVGReset, svgReset)
}

const void*
nsRuleNode::ComputeVariablesData(void* aStartStruct,
                                 const nsRuleData* aRuleData,
                                 nsStyleContext* aContext,
                                 nsRuleNode* aHighestNode,
                                 const RuleDetail aRuleDetail,
                                 const bool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Variables, (), variables, parentVariables)

  MOZ_ASSERT(aRuleData->mVariables,
             "shouldn't be in ComputeVariablesData if there were no variable "
             "declarations specified");

  CSSVariableResolver resolver(&variables->mVariables);
  resolver.Resolve(&parentVariables->mVariables,
                   aRuleData->mVariables);
  canStoreInRuleTree = false;

  COMPUTE_END_INHERITED(Variables, variables)
}

const void*
nsRuleNode::GetStyleData(nsStyleStructID aSID,
                         nsStyleContext* aContext,
                         bool aComputeData)
{
  NS_ASSERTION(IsUsedDirectly(),
               "if we ever call this on rule nodes that aren't used "
               "directly, we should adjust handling of mDependentBits "
               "in some way.");

  const void *data;
  data = mStyleData.GetStyleData(aSID);
  if (MOZ_LIKELY(data != nullptr))
    return data; 

  if (MOZ_UNLIKELY(!aComputeData))
    return nullptr;

  
  data = WalkRuleTree(aSID, aContext);

  MOZ_ASSERT(data, "should have aborted on out-of-memory");
  return data;
}

void
nsRuleNode::Mark()
{
  for (nsRuleNode *node = this;
       node && !(node->mDependentBits & NS_RULE_NODE_GC_MARK);
       node = node->mParent)
    node->mDependentBits |= NS_RULE_NODE_GC_MARK;
}

bool
nsRuleNode::DestroyIfNotMarked()
{
  
  
  
  
  if (!(mDependentBits & NS_RULE_NODE_GC_MARK) &&
      
      !(IsRoot() && mPresContext->StyleSet()->GetRuleTree() == this)) {
    Destroy();
    return true;
  }

  
  mDependentBits &= ~NS_RULE_NODE_GC_MARK;
  return false;
}

PLDHashOperator
nsRuleNode::SweepHashEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                           uint32_t number, void *arg)
{
  ChildrenHashEntry *entry = static_cast<ChildrenHashEntry*>(hdr);
  nsRuleNode* node = entry->mRuleNode;
  if (node->DestroyIfNotMarked()) {
    return PL_DHASH_REMOVE; 
  }
  if (node->HaveChildren()) {
    
    
    nsRuleNode** headQ = static_cast<nsRuleNode**>(arg);
    node->mNextSibling = *headQ;
    *headQ = node;
  }
  return PL_DHASH_NEXT;
}

void
nsRuleNode::SweepChildren(nsTArray<nsRuleNode*>& aSweepQueue)
{
  NS_ASSERTION(!(mDependentBits & NS_RULE_NODE_GC_MARK),
               "missing DestroyIfNotMarked() call");
  NS_ASSERTION(HaveChildren(),
               "why call SweepChildren with no children?");
  uint32_t childrenDestroyed = 0;
  nsRuleNode* survivorsWithChildren = nullptr;
  if (ChildrenAreHashed()) {
    PLDHashTable* children = ChildrenHash();
    uint32_t oldChildCount = children->EntryCount();
    PL_DHashTableEnumerate(children, SweepHashEntry, &survivorsWithChildren);
    childrenDestroyed = oldChildCount - children->EntryCount();
    if (childrenDestroyed == oldChildCount) {
      PL_DHashTableDestroy(children);
      mChildren.asVoid = nullptr;
    }
  } else {
    for (nsRuleNode** children = ChildrenListPtr(); *children; ) {
      nsRuleNode* next = (*children)->mNextSibling;
      if ((*children)->DestroyIfNotMarked()) {
        
        
        *children = next;
        ++childrenDestroyed;
      } else {
        children = &(*children)->mNextSibling;
      }
    }
    survivorsWithChildren = ChildrenList();
  }
  if (survivorsWithChildren) {
    aSweepQueue.AppendElement(survivorsWithChildren);
  }
  NS_ASSERTION(childrenDestroyed <= mRefCnt, "wrong ref count");
  mRefCnt -= childrenDestroyed;
  NS_POSTCONDITION(IsRoot() || mRefCnt > 0,
                   "We didn't get swept, so we'd better have style contexts "
                   "pointing to us or to one of our descendants, which means "
                   "we'd better have a nonzero mRefCnt here!");
}

bool
nsRuleNode::Sweep()
{
  NS_ASSERTION(IsRoot(), "must start sweeping at a root");
  NS_ASSERTION(!mNextSibling, "root must not have mNextSibling");

  if (DestroyIfNotMarked()) {
    return true;
  }

  nsAutoTArray<nsRuleNode*, 70> sweepQueue;
  sweepQueue.AppendElement(this);
  while (!sweepQueue.IsEmpty()) {
    nsTArray<nsRuleNode*>::index_type last = sweepQueue.Length() - 1;
    nsRuleNode* ruleNode = sweepQueue[last];
    sweepQueue.RemoveElementAt(last);
    for (; ruleNode; ruleNode = ruleNode->mNextSibling) {
      if (ruleNode->HaveChildren()) {
        ruleNode->SweepChildren(sweepQueue);
      }
    }
  }
  return false;
}

 bool
nsRuleNode::HasAuthorSpecifiedRules(nsStyleContext* aStyleContext,
                                    uint32_t ruleTypeMask,
                                    bool aAuthorColorsAllowed)
{
  uint32_t inheritBits = 0;
  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND)
    inheritBits |= NS_STYLE_INHERIT_BIT(Background);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER)
    inheritBits |= NS_STYLE_INHERIT_BIT(Border);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING)
    inheritBits |= NS_STYLE_INHERIT_BIT(Padding);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_TEXT_SHADOW)
    inheritBits |= NS_STYLE_INHERIT_BIT(Text);

  
  size_t nprops = 0,
         backgroundOffset, borderOffset, paddingOffset, textShadowOffset;

  

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

  
  size_t inheritedOffset = nprops;

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_TEXT_SHADOW) {
    textShadowOffset = nprops;
    nprops += nsCSSProps::PropertyCountInStruct(eStyleStruct_Text);
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

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_TEXT_SHADOW) {
    ruleData.mValueOffsets[eStyleStruct_Text] = textShadowOffset;
  }

  static const nsCSSProperty backgroundValues[] = {
    eCSSProperty_background_color,
    eCSSProperty_background_image,
  };

  static const nsCSSProperty borderValues[] = {
    eCSSProperty_border_top_color,
    eCSSProperty_border_top_style,
    eCSSProperty_border_top_width,
    eCSSProperty_border_right_color,
    eCSSProperty_border_right_style,
    eCSSProperty_border_right_width,
    eCSSProperty_border_bottom_color,
    eCSSProperty_border_bottom_style,
    eCSSProperty_border_bottom_width,
    eCSSProperty_border_left_color,
    eCSSProperty_border_left_style,
    eCSSProperty_border_left_width,
    eCSSProperty_border_top_left_radius,
    eCSSProperty_border_top_right_radius,
    eCSSProperty_border_bottom_right_radius,
    eCSSProperty_border_bottom_left_radius,
  };

  static const nsCSSProperty paddingValues[] = {
    eCSSProperty_padding_top,
    eCSSProperty_padding_right,
    eCSSProperty_padding_bottom,
    eCSSProperty_padding_left,
  };

  static const nsCSSProperty textShadowValues[] = {
    eCSSProperty_text_shadow
  };

  
  size_t nValues = 0;

  nsCSSValue* values[MOZ_ARRAY_LENGTH(backgroundValues) +
                     MOZ_ARRAY_LENGTH(borderValues) +
                     MOZ_ARRAY_LENGTH(paddingValues) +
                     MOZ_ARRAY_LENGTH(textShadowValues)];

  nsCSSProperty properties[MOZ_ARRAY_LENGTH(backgroundValues) +
                           MOZ_ARRAY_LENGTH(borderValues) +
                           MOZ_ARRAY_LENGTH(paddingValues) +
                           MOZ_ARRAY_LENGTH(textShadowValues)];

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND) {
    for (uint32_t i = 0, i_end = ArrayLength(backgroundValues);
         i < i_end; ++i) {
      properties[nValues] = backgroundValues[i];
      values[nValues++] = ruleData.ValueFor(backgroundValues[i]);
    }
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER) {
    for (uint32_t i = 0, i_end = ArrayLength(borderValues);
         i < i_end; ++i) {
      properties[nValues] = borderValues[i];
      values[nValues++] = ruleData.ValueFor(borderValues[i]);
    }
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING) {
    for (uint32_t i = 0, i_end = ArrayLength(paddingValues);
         i < i_end; ++i) {
      properties[nValues] = paddingValues[i];
      values[nValues++] = ruleData.ValueFor(paddingValues[i]);
    }
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_TEXT_SHADOW) {
    for (uint32_t i = 0, i_end = ArrayLength(textShadowValues);
         i < i_end; ++i) {
      properties[nValues] = textShadowValues[i];
      values[nValues++] = ruleData.ValueFor(textShadowValues[i]);
    }
  }

  nsStyleContext* styleContext = aStyleContext;

  
  
  
  
  
  
  bool haveExplicitUAInherit;
  do {
    haveExplicitUAInherit = false;
    for (nsRuleNode* ruleNode = styleContext->RuleNode(); ruleNode;
         ruleNode = ruleNode->GetParent()) {
      nsIStyleRule *rule = ruleNode->GetRule();
      if (rule) {
        ruleData.mLevel = ruleNode->GetLevel();
        ruleData.mIsImportantRule = ruleNode->IsImportantRule();

        rule->MapRuleInfoInto(&ruleData);

        if (ruleData.mLevel == nsStyleSet::eAgentSheet ||
            ruleData.mLevel == nsStyleSet::eUserSheet) {
          
          
          
          for (uint32_t i = 0; i < nValues; ++i) {
            nsCSSUnit unit = values[i]->GetUnit();
            if (unit != eCSSUnit_Null &&
                unit != eCSSUnit_Dummy &&
                unit != eCSSUnit_DummyInherit) {
              if (unit == eCSSUnit_Inherit ||
                  (i >= inheritedOffset && unit == eCSSUnit_Unset)) {
                haveExplicitUAInherit = true;
                values[i]->SetDummyInheritValue();
              } else {
                values[i]->SetDummyValue();
              }
            }
          }
        } else {
          
          
          for (uint32_t i = 0; i < nValues; ++i) {
            if (values[i]->GetUnit() != eCSSUnit_Null &&
                values[i]->GetUnit() != eCSSUnit_Dummy && 
                values[i]->GetUnit() != eCSSUnit_DummyInherit) {
              
              
              
              
              
              if (aAuthorColorsAllowed ||
                  !nsCSSProps::PropHasFlags(properties[i],
                     CSS_PROPERTY_IGNORED_WHEN_COLORS_DISABLED) ||
                  (properties[i] == eCSSProperty_background_color &&
                   !values[i]->IsNonTransparentColor())) {
                return true;
              }

              values[i]->SetDummyValue();
            }
          }
        }
      }
    }

    if (haveExplicitUAInherit) {
      
      
      
      
      
      for (uint32_t i = 0; i < nValues; ++i)
        if (values[i]->GetUnit() == eCSSUnit_Null)
          values[i]->SetDummyValue();
      for (uint32_t i = 0; i < nValues; ++i)
        if (values[i]->GetUnit() == eCSSUnit_DummyInherit)
          values[i]->Reset();
      styleContext = styleContext->GetParent();
    }
  } while (haveExplicitUAInherit && styleContext);

  return false;
}

 void
nsRuleNode::ComputePropertiesOverridingAnimation(
                              const nsTArray<nsCSSProperty>& aProperties,
                              nsStyleContext* aStyleContext,
                              nsCSSPropertySet& aPropertiesOverridden)
{
  



  uint32_t structBits = 0;
  size_t nprops = 0;
  size_t offsets[nsStyleStructID_Length];
  for (size_t propIdx = 0, propEnd = aProperties.Length();
       propIdx < propEnd; ++propIdx) {
    nsCSSProperty prop = aProperties[propIdx];
    nsStyleStructID sid = nsCSSProps::kSIDTable[prop];
    uint32_t bit = nsCachedStyleData::GetBitForSID(sid);
    if (!(structBits & bit)) {
      structBits |= bit;
      offsets[sid] = nprops;
      nprops += nsCSSProps::PropertyCountInStruct(sid);
    }
  }

  void* dataStorage = alloca(nprops * sizeof(nsCSSValue));
  AutoCSSValueArray dataArray(dataStorage, nprops);

  
  nsRuleData ruleData(structBits, dataArray.get(),
                      aStyleContext->PresContext(), aStyleContext);
  for (nsStyleStructID sid = nsStyleStructID(0);
       sid < nsStyleStructID_Length; sid = nsStyleStructID(sid + 1)) {
    if (structBits & nsCachedStyleData::GetBitForSID(sid)) {
      ruleData.mValueOffsets[sid] = offsets[sid];
    }
  }

  



  for (nsRuleNode* ruleNode = aStyleContext->RuleNode(); ruleNode;
       ruleNode = ruleNode->GetParent()) {
    nsIStyleRule *rule = ruleNode->GetRule();
    if (rule) {
      ruleData.mLevel = ruleNode->GetLevel();
      ruleData.mIsImportantRule = ruleNode->IsImportantRule();

      
      
      
      
      
      
      
      
      
      if (ruleData.mLevel == nsStyleSet::eTransitionSheet) {
        continue;
      }

      if (!ruleData.mIsImportantRule) {
        
        break;
      }

      rule->MapRuleInfoInto(&ruleData);
    }
  }

  


  for (size_t propIdx = 0, propEnd = aProperties.Length();
       propIdx < propEnd; ++propIdx) {
    nsCSSProperty prop = aProperties[propIdx];
    if (ruleData.ValueFor(prop)->GetUnit() != eCSSUnit_Null) {
      aPropertiesOverridden.AddProperty(prop);
    }
  }
}


bool
nsRuleNode::ComputeColor(const nsCSSValue& aValue, nsPresContext* aPresContext,
                         nsStyleContext* aStyleContext, nscolor& aResult)
{
  MOZ_ASSERT(aValue.GetUnit() != eCSSUnit_Inherit,
             "aValue shouldn't have eCSSUnit_Inherit");
  MOZ_ASSERT(aValue.GetUnit() != eCSSUnit_Initial,
             "aValue shouldn't have eCSSUnit_Initial");
  MOZ_ASSERT(aValue.GetUnit() != eCSSUnit_Unset,
             "aValue shouldn't have eCSSUnit_Unset");

  bool canStoreInRuleTree;
  bool ok = SetColor(aValue, NS_RGB(0, 0, 0), aPresContext, aStyleContext,
                     aResult, canStoreInRuleTree);
  MOZ_ASSERT(ok || !(aPresContext && aStyleContext));
  return ok;
}
