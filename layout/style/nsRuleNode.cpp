

















































#include "nsRuleNode.h"
#include "nscore.h"
#include "nsIServiceManager.h"
#include "nsIDeviceContext.h"
#include "nsIWidget.h"
#include "nsILookAndFeel.h"
#include "nsIPresShell.h"
#include "nsIThebesFontMetrics.h"
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
#include "nsStyleStructInlines.h"
#include "nsStyleTransformMatrix.h"
#include "nsCSSKeywords.h"
#include "nsCSSProps.h"
#include "nsTArray.h"





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
  NS_ASSERTION(aValue.IsLengthUnit(), "not a length unit");
  NS_ASSERTION(aStyleFont || aStyleContext, "Must have style data");
  NS_ASSERTION(!aStyleFont || !aStyleContext, "Duplicate sources of data");
  NS_ASSERTION(aPresContext, "Must have prescontext");

  if (aValue.IsFixedLengthUnit()) {
    return aPresContext->TwipsToAppUnits(aValue.GetLengthTwips());
  }
  nsCSSUnit unit = aValue.GetUnit();
  if (unit == eCSSUnit_Pixel) {
    return nsPresContext::CSSPixelsToAppUnits(aValue.GetFloatValue());
  }
  
  aCanStoreInRuleTree = PR_FALSE;
  if (!aStyleFont) {
    aStyleFont = aStyleContext->GetStyleFont();
  }
  if (aFontSize == -1) {
    
    
    aFontSize = aStyleFont->mFont.size;
  }
  switch (unit) {
    case eCSSUnit_RootEM: {
      nscoord rootFontSize;

      if (aUseProvidedRootEmSize) {
        
        
        
        
        
        rootFontSize = aFontSize;
      } else if (aStyleContext && !aStyleContext->GetParent()) {
        
        
        
        
        rootFontSize = aStyleFont->mFont.size;
      } else {
        
        
        nsRefPtr<nsStyleContext> rootStyle;
        const nsStyleFont *rootStyleFont = aStyleFont;
        nsIContent* docElement = aPresContext->Document()->GetRootContent();

        rootStyle = aPresContext->StyleSet()->ResolveStyleFor(docElement,
                                                              nsnull);
        if (rootStyle) {
          rootStyleFont = rootStyle->GetStyleFont();
          rootFontSize = rootStyleFont->mFont.size;
        }
      }

      return ScaleCoord(aValue, float(rootFontSize));
    }
    case eCSSUnit_EM: {
      return ScaleCoord(aValue, float(aFontSize));
      
    }
    case eCSSUnit_XHeight: {
      nsFont font = aStyleFont->mFont;
      font.size = aFontSize;
      nsCOMPtr<nsIFontMetrics> fm =
        aPresContext->GetMetricsFor(font, aUseUserFontSet);
      nscoord xHeight;
      fm->GetXHeight(xHeight);
      return ScaleCoord(aValue, float(xHeight));
    }
    case eCSSUnit_Char: {
      nsFont font = aStyleFont->mFont;
      font.size = aFontSize;
      nsCOMPtr<nsIFontMetrics> fm =
        aPresContext->GetMetricsFor(font, aUseUserFontSet);
      nsCOMPtr<nsIThebesFontMetrics> tfm(do_QueryInterface(fm));
      gfxFloat zeroWidth = (tfm->GetThebesFontGroup()->GetFontAt(0)
                            ->GetMetrics().zeroOrAveCharWidth);

      return ScaleCoord(aValue, NS_ceil(aPresContext->AppUnitsPerDevPixel() *
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

#define SETCOORD_LP     (SETCOORD_LENGTH | SETCOORD_PERCENT)
#define SETCOORD_LH     (SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_AH     (SETCOORD_AUTO | SETCOORD_INHERIT)
#define SETCOORD_LAH    (SETCOORD_AUTO | SETCOORD_LENGTH | SETCOORD_INHERIT)
#define SETCOORD_LPH    (SETCOORD_LP | SETCOORD_INHERIT)
#define SETCOORD_LPAH   (SETCOORD_LP | SETCOORD_AH)
#define SETCOORD_LPEH   (SETCOORD_LP | SETCOORD_ENUMERATED | SETCOORD_INHERIT)
#define SETCOORD_LPAEH  (SETCOORD_LPAH | SETCOORD_ENUMERATED)
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
  else if (((aMask & SETCOORD_LENGTH) != 0) && 
           aValue.IsLengthUnit()) {
    aCoord.SetCoordValue(CalcLength(aValue, aStyleContext, aPresContext,
                                    aCanStoreInRuleTree));
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
  else {
    result = PR_FALSE;  
  }
  return result;
}








static float GetFloatFromBoxPosition(PRInt32 aEnumValue)
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
  
  if (aValue.GetUnit() == eCSSUnit_Enumerated) {
    aResult.SetPercentValue(GetFloatFromBoxPosition(aValue.GetIntValue()));
    return;
  }

  
  PRBool result = SetCoord(aValue, aResult, nsStyleCoord(), SETCOORD_LP,
                           aContext, aPresContext, aCanStoreInRuleTree);
  NS_ABORT_IF_FALSE(result, "Incorrect data structure created by parsing code");
}

static void SetGradient(const nsCSSValue& aValue, nsPresContext* aPresContext,
                        nsStyleContext* aContext, nsStyleGradient& aResult,
                        PRBool& aCanStoreInRuleTree)
{
  NS_ABORT_IF_FALSE(aValue.GetUnit() == eCSSUnit_Gradient,
                    "The given data is not a gradient");

  nsCSSValueGradient* gradient = aValue.GetGradientValue();
  aResult.mIsRadial = gradient->mIsRadial;

  
  SetGradientCoord(gradient->mStartX, aPresContext, aContext,
                   aResult.mStartX, aCanStoreInRuleTree);

  SetGradientCoord(gradient->mStartY, aPresContext, aContext,
                   aResult.mStartY, aCanStoreInRuleTree);

  if (gradient->mIsRadial) {
    NS_ABORT_IF_FALSE(gradient->mStartRadius.IsLengthUnit(),
                      "Incorrect data structure created by parsing code");
    aResult.mStartRadius = CalcLength(gradient->mStartRadius, aContext,
                                      aPresContext, aCanStoreInRuleTree);
  }

  
  SetGradientCoord(gradient->mEndX, aPresContext, aContext,
                   aResult.mEndX, aCanStoreInRuleTree);

  SetGradientCoord(gradient->mEndY, aPresContext, aContext,
                   aResult.mEndY, aCanStoreInRuleTree);

  if (gradient->mIsRadial) {
    NS_ABORT_IF_FALSE(gradient->mEndRadius.IsLengthUnit(),
                      "Incorrect data structure created by parsing code");
    aResult.mEndRadius = CalcLength(gradient->mEndRadius, aContext,
                                    aPresContext, aCanStoreInRuleTree);
  }

  
  for (PRUint32 i = 0; i < gradient->mStops.Length(); i++) {
    nsStyleGradientStop stop;
    nsCSSValueGradientStop &valueStop = gradient->mStops[i];

    if (valueStop.mLocation.GetUnit() == eCSSUnit_Percent)
      stop.mPosition = valueStop.mLocation.GetPercentValue();
    else
      stop.mPosition = valueStop.mLocation.GetFloatValue();

    
    
    NS_ASSERTION(valueStop.mColor.GetUnit() != eCSSUnit_Inherit,
                 "inherit is not a valid color for gradient stops");
    SetColor(valueStop.mColor, NS_RGB(0, 0, 0), aPresContext,
             aContext, stop.mColor, aCanStoreInRuleTree);

    aResult.mStops.AppendElement(stop);
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
    mNoneBits(0)
{
  mChildren.asVoid = nsnull;
  MOZ_COUNT_CTOR(nsRuleNode);
  NS_IF_ADDREF(mRule);

  NS_ASSERTION(IsRoot() || GetLevel() == aLevel, "not enough bits");
  NS_ASSERTION(IsRoot() || IsImportantRule() == aIsImportant, "yikes");
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
      return nsnull;
    }
    if (entry->mRuleNode)
      next = entry->mRuleNode;
    else {
      next = entry->mRuleNode = new (mPresContext)
        nsRuleNode(mPresContext, this, aRule, aLevel, aIsImportantRule);
      if (!next) {
        PL_DHashTableRawRemove(ChildrenHash(), entry);
        return nsnull;
      }
    }
  } else if (!next) {
    
    next = new (mPresContext)
      nsRuleNode(mPresContext, this, aRule, aLevel, aIsImportantRule);
    if (!next) {
      return nsnull;
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









struct PropertyCheckData {
  size_t offset;
  
  
  
  nsCSSType type;
  PRUint32 flags;
};






typedef nsRuleNode::RuleDetail
  (* CheckCallbackFn)(const nsRuleDataStruct& aData,
                      nsRuleNode::RuleDetail aResult);


struct StructCheckData {
  const PropertyCheckData* props;
  PRInt32 nprops;
  CheckCallbackFn callback;
};







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

static void
ExamineCSSValuePair(const nsCSSValuePair* aValuePair,
                    PRUint32& aSpecifiedCount, PRUint32& aInheritedCount)
{
  NS_PRECONDITION(aValuePair, "Must have a value pair");
  
  ExamineCSSValue(aValuePair->mXValue, aSpecifiedCount, aInheritedCount);
  ExamineCSSValue(aValuePair->mYValue, aSpecifiedCount, aInheritedCount);
}

static void
ExamineCSSRect(const nsCSSRect* aRect,
               PRUint32& aSpecifiedCount, PRUint32& aInheritedCount)
{
  NS_PRECONDITION(aRect, "Must have a rect");

  NS_FOR_CSS_SIDES(side) {
    ExamineCSSValue(aRect->*(nsCSSRect::sides[side]),
                    aSpecifiedCount, aInheritedCount);
  }
}

static nsRuleNode::RuleDetail
CheckFontCallback(const nsRuleDataStruct& aData,
                  nsRuleNode::RuleDetail aResult)
{
  const nsRuleDataFont& fontData =
      static_cast<const nsRuleDataFont&>(aData);

  
  
  
  
  const nsCSSValue& size = fontData.mSize;
  const nsCSSValue& weight = fontData.mWeight;
  const nsCSSValue& stretch = fontData.mStretch;
  if ((size.IsRelativeLengthUnit() && size.GetUnit() != eCSSUnit_Pixel) ||
      size.GetUnit() == eCSSUnit_Percent ||
      (size.GetUnit() == eCSSUnit_Enumerated &&
       (size.GetIntValue() == NS_STYLE_FONT_SIZE_SMALLER ||
        size.GetIntValue() == NS_STYLE_FONT_SIZE_LARGER)) ||
#ifdef MOZ_MATHML
      fontData.mScriptLevel.GetUnit() == eCSSUnit_Integer ||
#endif
      (stretch.GetUnit() == eCSSUnit_Enumerated &&
       (stretch.GetIntValue() == NS_FONT_STRETCH_NARROWER ||
        stretch.GetIntValue() == NS_FONT_STRETCH_WIDER)) ||
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
CheckColorCallback(const nsRuleDataStruct& aData,
                   nsRuleNode::RuleDetail aResult)
{
  const nsRuleDataColor& colorData =
      static_cast<const nsRuleDataColor&>(aData);

  
  if (colorData.mColor.GetUnit() == eCSSUnit_EnumColor && 
      colorData.mColor.GetIntValue() == NS_COLOR_CURRENTCOLOR) {
    NS_ASSERTION(aResult == nsRuleNode::eRuleFullReset,
                 "we should already be counted as full-reset");
    aResult = nsRuleNode::eRuleFullInherited;
  }

  return aResult;
}

static nsRuleNode::RuleDetail
CheckTextCallback(const nsRuleDataStruct& aData,
                  nsRuleNode::RuleDetail aResult)
{
  const nsRuleDataText& textData =
    static_cast<const nsRuleDataText&>(aData);

  if (textData.mTextAlign.GetUnit() == eCSSUnit_Enumerated &&
      textData.mTextAlign.GetIntValue() ==
        NS_STYLE_TEXT_ALIGN_MOZ_CENTER_OR_INHERIT) {
    
    
    if (aResult == nsRuleNode::eRulePartialReset)
      aResult = nsRuleNode::eRulePartialMixed;
    else if (aResult == nsRuleNode::eRuleFullReset)
      aResult = nsRuleNode::eRuleFullMixed;
  }

  return aResult;
}



#define CSS_PROP_INCLUDE_NOT_CSS

#define CHECK_DATA_FOR_PROPERTY(name_, id_, method_, flags_, datastruct_, member_, type_, kwtable_) \
  { offsetof(nsRuleData##datastruct_, member_), type_, flags_ },

static const PropertyCheckData FontCheckProperties[] = {
#define CSS_PROP_FONT CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_FONT
};

static const PropertyCheckData DisplayCheckProperties[] = {
#define CSS_PROP_DISPLAY CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_DISPLAY
};

static const PropertyCheckData VisibilityCheckProperties[] = {
#define CSS_PROP_VISIBILITY CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_VISIBILITY
};

static const PropertyCheckData MarginCheckProperties[] = {
#define CSS_PROP_MARGIN CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_MARGIN
};

static const PropertyCheckData BorderCheckProperties[] = {
#define CSS_PROP_BORDER CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_BORDER
};

static const PropertyCheckData PaddingCheckProperties[] = {
#define CSS_PROP_PADDING CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_PADDING
};

static const PropertyCheckData OutlineCheckProperties[] = {
#define CSS_PROP_OUTLINE CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_OUTLINE
};

static const PropertyCheckData ListCheckProperties[] = {
#define CSS_PROP_LIST CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_LIST
};

static const PropertyCheckData ColorCheckProperties[] = {
#define CSS_PROP_COLOR CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_COLOR
};

static const PropertyCheckData BackgroundCheckProperties[] = {
#define CSS_PROP_BACKGROUND CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_BACKGROUND
};

static const PropertyCheckData PositionCheckProperties[] = {
#define CSS_PROP_POSITION CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_POSITION
};

static const PropertyCheckData TableCheckProperties[] = {
#define CSS_PROP_TABLE CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TABLE
};

static const PropertyCheckData TableBorderCheckProperties[] = {
#define CSS_PROP_TABLEBORDER CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TABLEBORDER
};

static const PropertyCheckData ContentCheckProperties[] = {
#define CSS_PROP_CONTENT CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_CONTENT
};

static const PropertyCheckData QuotesCheckProperties[] = {
#define CSS_PROP_QUOTES CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_QUOTES
};

static const PropertyCheckData TextCheckProperties[] = {
#define CSS_PROP_TEXT CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TEXT
};

static const PropertyCheckData TextResetCheckProperties[] = {
#define CSS_PROP_TEXTRESET CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_TEXTRESET
};

static const PropertyCheckData UserInterfaceCheckProperties[] = {
#define CSS_PROP_USERINTERFACE CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_USERINTERFACE
};

static const PropertyCheckData UIResetCheckProperties[] = {
#define CSS_PROP_UIRESET CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_UIRESET
};

static const PropertyCheckData XULCheckProperties[] = {
#define CSS_PROP_XUL CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_XUL
};

#ifdef MOZ_SVG
static const PropertyCheckData SVGCheckProperties[] = {
#define CSS_PROP_SVG CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_SVG
};

static const PropertyCheckData SVGResetCheckProperties[] = {
#define CSS_PROP_SVGRESET CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_SVGRESET
};  
#endif

static const PropertyCheckData ColumnCheckProperties[] = {
#define CSS_PROP_COLUMN CHECK_DATA_FOR_PROPERTY
#include "nsCSSPropList.h"
#undef CSS_PROP_COLUMN
};

#undef CSS_PROP_INCLUDE_NOT_CSS
#undef CHECK_DATA_FOR_PROPERTY
  
static const StructCheckData gCheckProperties[] = {

#define STYLE_STRUCT(name, checkdata_cb, ctor_args) \
  {name##CheckProperties, \
   sizeof(name##CheckProperties)/sizeof(PropertyCheckData), \
   checkdata_cb},
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
  {nsnull, 0, nsnull}

};





inline nsCSSValue&
ValueAtOffset(nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return * reinterpret_cast<nsCSSValue*>
                           (reinterpret_cast<char*>(&aRuleDataStruct) + aOffset);
}

inline const nsCSSValue&
ValueAtOffset(const nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return * reinterpret_cast<const nsCSSValue*>
                           (reinterpret_cast<const char*>(&aRuleDataStruct) + aOffset);
}

inline nsCSSRect*
RectAtOffset(nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return reinterpret_cast<nsCSSRect*>
                         (reinterpret_cast<char*>(&aRuleDataStruct) + aOffset);
}

inline const nsCSSRect*
RectAtOffset(const nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return reinterpret_cast<const nsCSSRect*>
                         (reinterpret_cast<const char*>(&aRuleDataStruct) + aOffset);
}

inline nsCSSValuePair*
ValuePairAtOffset(nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return reinterpret_cast<nsCSSValuePair*>
                         (reinterpret_cast<char*>(&aRuleDataStruct) + aOffset);
}

inline const nsCSSValuePair*
ValuePairAtOffset(const nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return reinterpret_cast<const nsCSSValuePair*>
                         (reinterpret_cast<const char*>(&aRuleDataStruct) + aOffset);
}

inline nsCSSValueList*&
ValueListAtOffset(nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return * reinterpret_cast<nsCSSValueList**>
                           (reinterpret_cast<char*>(&aRuleDataStruct) + aOffset);
}

inline const nsCSSValueList*
ValueListAtOffset(const nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return * reinterpret_cast<const nsCSSValueList*const*>
                           (reinterpret_cast<const char*>(&aRuleDataStruct) + aOffset);
}

inline nsCSSValuePairList*&
ValuePairListAtOffset(nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return * reinterpret_cast<nsCSSValuePairList**>
                           (reinterpret_cast<char*>(&aRuleDataStruct) + aOffset);
}

inline const nsCSSValuePairList*
ValuePairListAtOffset(const nsRuleDataStruct& aRuleDataStruct, size_t aOffset)
{
  return * reinterpret_cast<const nsCSSValuePairList*const*>
                           (reinterpret_cast<const char*>(&aRuleDataStruct) + aOffset);
}

#if defined(MOZ_MATHML) && defined(DEBUG)
static PRBool
AreAllMathMLPropertiesUndefined(const nsCSSFont& aRuleData)
{
  return aRuleData.mScriptLevel.GetUnit() == eCSSUnit_Null &&
         aRuleData.mScriptSizeMultiplier.GetUnit() == eCSSUnit_Null &&
         aRuleData.mScriptMinSize.GetUnit() == eCSSUnit_Null;
}
#endif

inline nsRuleNode::RuleDetail
nsRuleNode::CheckSpecifiedProperties(const nsStyleStructID aSID,
                                     const nsRuleDataStruct& aRuleDataStruct)
{
  const StructCheckData *structData = gCheckProperties + aSID;

  
  PRUint32 total = 0,      
           specified = 0,  
           inherited = 0;  
                           

  for (const PropertyCheckData *prop = structData->props,
                           *prop_end = prop + structData->nprops;
       prop != prop_end;
       ++prop)
    switch (prop->type) {

      case eCSSType_Value:
        ++total;
        ExamineCSSValue(ValueAtOffset(aRuleDataStruct, prop->offset),
                        specified, inherited);
        break;

      case eCSSType_Rect:
        total += 4;
        ExamineCSSRect(RectAtOffset(aRuleDataStruct, prop->offset),
                       specified, inherited);
        break;

      case eCSSType_ValuePair:
        total += 2;
        ExamineCSSValuePair(ValuePairAtOffset(aRuleDataStruct, prop->offset),
                            specified, inherited);
        break;
        
      case eCSSType_ValueList:
        {
          ++total;
          const nsCSSValueList* valueList =
              ValueListAtOffset(aRuleDataStruct, prop->offset);
          if (valueList) {
            ++specified;
            if (eCSSUnit_Inherit == valueList->mValue.GetUnit()) {
              ++inherited;
            }
          }
        }
        break;

      case eCSSType_ValuePairList:
        {
          ++total;
          const nsCSSValuePairList* valuePairList =
              ValuePairListAtOffset(aRuleDataStruct, prop->offset);
          if (valuePairList) {
            ++specified;
            if (eCSSUnit_Inherit == valuePairList->mXValue.GetUnit()) {
              ++inherited;
            }
          }
        }
        break;

      default:
        NS_NOTREACHED("unknown type");
        break;

    }

#if 0
  printf("CheckSpecifiedProperties: SID=%d total=%d spec=%d inh=%d.\n",
         aSID, total, specified, inherited);
#endif

#ifdef MOZ_MATHML
  NS_ASSERTION(aSID != eStyleStruct_Font ||
               mPresContext->Document()->GetMathMLEnabled() ||
               AreAllMathMLPropertiesUndefined(static_cast<const nsCSSFont&>(aRuleDataStruct)),
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

  if (structData->callback) {
    result = (*structData->callback)(aRuleDataStruct, result);
  }

  return result;
}

const void*
nsRuleNode::GetDisplayData(nsStyleContext* aContext)
{
  nsRuleDataDisplay displayData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Display), mPresContext, aContext);
  ruleData.mDisplayData = &displayData;

  return WalkRuleTree(eStyleStruct_Display, aContext, &ruleData, &displayData);
}

const void*
nsRuleNode::GetVisibilityData(nsStyleContext* aContext)
{
  nsRuleDataDisplay displayData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Visibility), mPresContext, aContext);
  ruleData.mDisplayData = &displayData;

  return WalkRuleTree(eStyleStruct_Visibility, aContext, &ruleData, &displayData);
}

const void*
nsRuleNode::GetTextData(nsStyleContext* aContext)
{
  nsRuleDataText textData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Text), mPresContext, aContext);
  ruleData.mTextData = &textData;

  const void* res = WalkRuleTree(eStyleStruct_Text, aContext, &ruleData, &textData);
  textData.mTextShadow = nsnull; 
  return res;
}

const void*
nsRuleNode::GetTextResetData(nsStyleContext* aContext)
{
  nsRuleDataText textData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(TextReset), mPresContext, aContext);
  ruleData.mTextData = &textData;

  return WalkRuleTree(eStyleStruct_TextReset, aContext, &ruleData, &textData);
}

const void*
nsRuleNode::GetUserInterfaceData(nsStyleContext* aContext)
{
  nsRuleDataUserInterface uiData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(UserInterface), mPresContext, aContext);
  ruleData.mUserInterfaceData = &uiData;

  const void* res = WalkRuleTree(eStyleStruct_UserInterface, aContext, &ruleData, &uiData);
  uiData.mCursor = nsnull; 
  return res;
}

const void*
nsRuleNode::GetUIResetData(nsStyleContext* aContext)
{
  nsRuleDataUserInterface uiData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(UIReset), mPresContext, aContext);
  ruleData.mUserInterfaceData = &uiData;

  return WalkRuleTree(eStyleStruct_UIReset, aContext, &ruleData, &uiData);
}

const void*
nsRuleNode::GetFontData(nsStyleContext* aContext)
{
  nsRuleDataFont fontData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Font), mPresContext, aContext);
  ruleData.mFontData = &fontData;

  return WalkRuleTree(eStyleStruct_Font, aContext, &ruleData, &fontData);
}

const void*
nsRuleNode::GetColorData(nsStyleContext* aContext)
{
  nsRuleDataColor colorData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Color), mPresContext, aContext);
  ruleData.mColorData = &colorData;

  return WalkRuleTree(eStyleStruct_Color, aContext, &ruleData, &colorData);
}

const void*
nsRuleNode::GetBackgroundData(nsStyleContext* aContext)
{
  nsRuleDataColor colorData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Background), mPresContext, aContext);
  ruleData.mColorData = &colorData;

  
  
  

  const void *res = WalkRuleTree(eStyleStruct_Background, aContext, &ruleData, &colorData);

  
  colorData.mBackImage = nsnull;
  colorData.mBackRepeat = nsnull;
  colorData.mBackAttachment = nsnull;
  colorData.mBackPosition = nsnull;
  colorData.mBackSize = nsnull;
  colorData.mBackClip = nsnull;
  colorData.mBackOrigin = nsnull;

  return res;
}

const void*
nsRuleNode::GetMarginData(nsStyleContext* aContext)
{
  nsRuleDataMargin marginData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Margin), mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  return WalkRuleTree(eStyleStruct_Margin, aContext, &ruleData, &marginData);
}

const void*
nsRuleNode::GetBorderData(nsStyleContext* aContext)
{
  nsRuleDataMargin marginData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Border), mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  const void* res = WalkRuleTree(eStyleStruct_Border, aContext, &ruleData, &marginData);
  
  
  marginData.mBoxShadow = nsnull;
  return res;
}

const void*
nsRuleNode::GetPaddingData(nsStyleContext* aContext)
{
  nsRuleDataMargin marginData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Padding), mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  
  
  

  return WalkRuleTree(eStyleStruct_Padding, aContext, &ruleData, &marginData);
}

const void*
nsRuleNode::GetOutlineData(nsStyleContext* aContext)
{
  nsRuleDataMargin marginData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Outline), mPresContext, aContext);
  ruleData.mMarginData = &marginData;

  return WalkRuleTree(eStyleStruct_Outline, aContext, &ruleData, &marginData);
}

const void*
nsRuleNode::GetListData(nsStyleContext* aContext)
{
  nsRuleDataList listData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(List), mPresContext, aContext);
  ruleData.mListData = &listData;

  return WalkRuleTree(eStyleStruct_List, aContext, &ruleData, &listData);
}

const void*
nsRuleNode::GetPositionData(nsStyleContext* aContext)
{
  nsRuleDataPosition posData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Position), mPresContext, aContext);
  ruleData.mPositionData = &posData;

  return WalkRuleTree(eStyleStruct_Position, aContext, &ruleData, &posData);
}

const void*
nsRuleNode::GetTableData(nsStyleContext* aContext)
{
  nsRuleDataTable tableData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Table), mPresContext, aContext);
  ruleData.mTableData = &tableData;

  return WalkRuleTree(eStyleStruct_Table, aContext, &ruleData, &tableData);
}

const void*
nsRuleNode::GetTableBorderData(nsStyleContext* aContext)
{
  nsRuleDataTable tableData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(TableBorder), mPresContext, aContext);
  ruleData.mTableData = &tableData;

  return WalkRuleTree(eStyleStruct_TableBorder, aContext, &ruleData, &tableData);
}

const void*
nsRuleNode::GetContentData(nsStyleContext* aContext)
{
  nsRuleDataContent contentData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Content), mPresContext, aContext);
  ruleData.mContentData = &contentData;

  const void* res = WalkRuleTree(eStyleStruct_Content, aContext, &ruleData, &contentData);
  contentData.mCounterIncrement = contentData.mCounterReset = nsnull;
  contentData.mContent = nsnull; 
  return res;
}

const void*
nsRuleNode::GetQuotesData(nsStyleContext* aContext)
{
  nsRuleDataContent contentData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Quotes), mPresContext, aContext);
  ruleData.mContentData = &contentData;

  const void* res = WalkRuleTree(eStyleStruct_Quotes, aContext, &ruleData, &contentData);
  contentData.mQuotes = nsnull; 
  return res;
}

const void*
nsRuleNode::GetXULData(nsStyleContext* aContext)
{
  nsRuleDataXUL xulData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(XUL), mPresContext, aContext);
  ruleData.mXULData = &xulData;

  return WalkRuleTree(eStyleStruct_XUL, aContext, &ruleData, &xulData);
}

const void*
nsRuleNode::GetColumnData(nsStyleContext* aContext)
{
  nsRuleDataColumn columnData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Column), mPresContext, aContext);
  ruleData.mColumnData = &columnData;

  return WalkRuleTree(eStyleStruct_Column, aContext, &ruleData, &columnData);
}

#ifdef MOZ_SVG
const void*
nsRuleNode::GetSVGData(nsStyleContext* aContext)
{
  nsRuleDataSVG svgData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(SVG), mPresContext, aContext);
  ruleData.mSVGData = &svgData;

  const void *res = WalkRuleTree(eStyleStruct_SVG, aContext, &ruleData, &svgData);
  svgData.mStrokeDasharray = nsnull; 
  return res;
}

const void*
nsRuleNode::GetSVGResetData(nsStyleContext* aContext)
{
  nsRuleDataSVG svgData; 
  nsRuleData ruleData(NS_STYLE_INHERIT_BIT(SVGReset), mPresContext, aContext);
  ruleData.mSVGData = &svgData;

  return WalkRuleTree(eStyleStruct_SVGReset, aContext, &ruleData, &svgData);
}
#endif




inline PRUint32
GetPseudoRestriction(nsStyleContext *aContext)
{
  
  PRUint32 pseudoRestriction = 0;
  nsIAtom *pseudoType = aContext->GetPseudoType();
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
                            nsRuleDataStruct& aRuleDataStruct,
                            PRUint32 aFlags)
{
  NS_ASSERTION(aFlags != 0, "aFlags must be nonzero");
  const StructCheckData *structData = gCheckProperties + aSID;

  for (const PropertyCheckData *prop = structData->props,
                           *prop_end = prop + structData->nprops;
       prop != prop_end;
       ++prop) {
    if ((prop->flags & aFlags) == aFlags)
      
      continue;

    switch (prop->type) {
      case eCSSType_Value:
        ValueAtOffset(aRuleDataStruct, prop->offset).Reset();
        break;
      case eCSSType_Rect:
        RectAtOffset(aRuleDataStruct, prop->offset)->Reset();
        break;
      case eCSSType_ValuePair:
        ValuePairAtOffset(aRuleDataStruct, prop->offset)->Reset();
        break;
      case eCSSType_ValueList:
        ValueListAtOffset(aRuleDataStruct, prop->offset) = nsnull;
        break;
      case eCSSType_ValuePairList:
        ValuePairListAtOffset(aRuleDataStruct, prop->offset) = nsnull;
        break;
      default:
        NS_NOTREACHED("unknown type");
        break;
    }
  }
}

const void*
nsRuleNode::WalkRuleTree(const nsStyleStructID aSID,
                         nsStyleContext* aContext, 
                         nsRuleData* aRuleData,
                         nsRuleDataStruct* aSpecificData)
{
  
  void* startStruct = nsnull;
  
  nsRuleNode* ruleNode = this;
  nsRuleNode* highestNode = nsnull; 
                                    
                                    
                                    
  nsRuleNode* rootNode = this; 
                               
                               
                               
                               
  RuleDetail detail = eRuleNone;
  PRUint32 bit = nsCachedStyleData::GetBitForSID(aSID);

  while (ruleNode) {
    
    
    if (ruleNode->mNoneBits & bit)
      break;

    
    
    
    
    if (detail == eRuleNone)
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
      aRuleData->mLevel = ruleNode->GetLevel();
      aRuleData->mIsImportantRule = ruleNode->IsImportantRule();
      rule->MapRuleInfoInto(aRuleData);
    }

    
    
    RuleDetail oldDetail = detail;
    detail = CheckSpecifiedProperties(aSID, *aSpecificData);
  
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
    UnsetPropertiesWithoutFlags(aSID, *aSpecificData, pseudoRestriction);

    
    
    
    detail = CheckSpecifiedProperties(aSID, *aSpecificData);
  }

  NS_ASSERTION(!startStruct || (detail != eRuleFullReset &&
                                detail != eRuleFullMixed &&
                                detail != eRuleFullInherited),
               "can't have start struct and be fully specified");

  PRBool isReset = nsCachedStyleData::IsReset(aSID);
  if (!highestNode)
    highestNode = rootNode;

  if (!aRuleData->mCanStoreInRuleTree)
    detail = eRulePartialMixed; 
                                

  if (detail == eRuleNone && startStruct && !aRuleData->mPostResolveCallback) {
    
    
    
    
    
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
             parentContext->GetPseudoType() == nsCSSPseudoElements::firstLine) {
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
  res = Compute##name##Data(startStruct, *aSpecificData, aContext,            \
                      highestNode, detail, aRuleData->mCanStoreInRuleTree);
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
#undef STYLE_STRUCT_TEST

  
  if (aRuleData->mPostResolveCallback && (NS_LIKELY(res != nsnull)))
    (*aRuleData->mPostResolveCallback)(const_cast<void*>(res), aRuleData);

  
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
        nscoord minimumFontSize =
          mPresContext->GetCachedIntPref(kPresContext_MinimumFontSize);

        if (minimumFontSize > 0 && !mPresContext->IsChrome()) {
          fontData->mFont.size = PR_MAX(fontData->mSize, minimumFontSize);
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

#ifdef MOZ_SVG
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
#endif
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
                                 PRUint8 aSide,
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
  }
}













#define COMPUTE_START_INHERITED(type_, ctorargs_, data_, parentdata_, rdtype_, rdata_) \
  NS_ASSERTION(aRuleDetail != eRuleFullInherited,                             \
               "should not have bothered calling Compute*Data");              \
                                                                              \
  nsStyleContext* parentContext = aContext->GetParent();                      \
                                                                              \
  const nsRuleData##rdtype_& rdata_ =                                         \
    static_cast<const nsRuleData##rdtype_&>(aData);                           \
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













#define COMPUTE_START_RESET(type_, ctorargs_, data_, parentdata_, rdtype_, rdata_) \
  NS_ASSERTION(aRuleDetail != eRuleFullInherited,                             \
               "should not have bothered calling Compute*Data");              \
                                                                              \
  nsStyleContext* parentContext = aContext->GetParent();                      \
  /* Reset structs don't inherit from first-line */                           \
  /* See similar code in WalkRuleTree */                                      \
  while (parentContext &&                                                     \
         parentContext->GetPseudoType() == nsCSSPseudoElements::firstLine) {  \
    parentContext = parentContext->GetParent();                               \
  }                                                                           \
                                                                              \
  const nsRuleData##rdtype_& rdata_ =                                         \
    static_cast<const nsRuleData##rdtype_&>(aData);                           \
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
  if (!canStoreInRuleTree)                                                    \
    /* We can't be cached in the rule node.  We have to be put right */       \
    /* on the style context. */                                               \
    aContext->SetStyle(eStyleStruct_##type_, data_);                          \
  else {                                                                      \
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
    NS_ASSERTION(!aHighestNode->mStyleData.mInheritedData->m##type_##Data,    \
                 "Going to leak style data");                                 \
    aHighestNode->mStyleData.mInheritedData->m##type_##Data = data_;          \
    /* Propagate the bit down. */                                             \
    PropagateDependentBit(NS_STYLE_INHERIT_BIT(type_), aHighestNode);         \
  }                                                                           \
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
    NS_ASSERTION(!aHighestNode->mStyleData.mResetData->m##type_##Data,        \
                 "Going to leak style data");                                 \
    aHighestNode->mStyleData.mResetData->m##type_##Data = data_;              \
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
    NSToCoordRound(PR_MIN(aParentFont->mScriptUnconstrainedSize*scriptLevelScale,
                          nscoord_MAX));
  
  nscoord scriptLevelSize =
    NSToCoordRound(PR_MIN(aParentFont->mSize*scriptLevelScale,
                          nscoord_MAX));
  if (scriptLevelScale <= 1.0) {
    if (aParentFont->mSize <= minScriptSize) {
      
      
      
      return aParentFont->mSize;
    }
    
    return PR_MAX(minScriptSize, scriptLevelSize);
  } else {
    
    NS_ASSERTION(*aUnconstrainedSize <= scriptLevelSize, "How can this ever happen?");
    
    return PR_MIN(scriptLevelSize, PR_MAX(*aUnconstrainedSize, minScriptSize));
  }
}
#endif

 void
nsRuleNode::SetFontSize(nsPresContext* aPresContext,
                        const nsRuleDataFont& aFontData,
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
  if (eCSSUnit_Enumerated == aFontData.mSize.GetUnit()) {
    PRInt32 value = aFontData.mSize.GetIntValue();
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
  else if (aFontData.mSize.IsLengthUnit()) {
    
    
    
    *aSize = CalcLengthWith(aFontData.mSize, aParentSize, aParentFont, nsnull,
                            aPresContext, aAtRoot, PR_TRUE,
                            aCanStoreInRuleTree);
    zoom = aFontData.mSize.IsFixedLengthUnit() ||
           aFontData.mSize.GetUnit() == eCSSUnit_Pixel;
  }
  else if (eCSSUnit_Percent == aFontData.mSize.GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    
    
    
    *aSize = NSToCoordRound(aParentSize *
                            aFontData.mSize.GetPercentValue());
    zoom = PR_FALSE;
  }
  else if (eCSSUnit_System_Font == aFontData.mSize.GetUnit()) {
    
    *aSize = aSystemFont.size;
    zoom = PR_TRUE;
  }
  else if (eCSSUnit_Inherit == aFontData.mSize.GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    
    
    
    *aSize = aScriptLevelAdjustedParentSize;
    zoom = PR_FALSE;
  }
  else if (eCSSUnit_Initial == aFontData.mSize.GetUnit()) {
    
    
    *aSize = baseSize;
    zoom = PR_TRUE;
  } else {
    NS_ASSERTION(eCSSUnit_Null == aFontData.mSize.GetUnit(),
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
                    PRUint8 aGenericFontID, const nsRuleDataFont& aFontData,
                    const nsStyleFont* aParentFont,
                    nsStyleFont* aFont, PRBool aUsedStartStruct,
                    PRBool& aCanStoreInRuleTree)
{
  const nsFont* defaultVariableFont =
    aPresContext->GetDefaultFont(kPresContext_DefaultVariableFont_ID);
  PRBool atRoot = !aContext->GetParent();

  
  nsFont systemFont;
  if (eCSSUnit_Enumerated == aFontData.mSystemFont.GetUnit()) {
    nsSystemFontID sysID;
    switch (aFontData.mSystemFont.GetIntValue()) {
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
          PR_MAX(defaultVariableFont->size - aPresContext->PointsToAppUnits(2), 0);
        break;
    }
#endif
  } else {
    
    systemFont = *defaultVariableFont;
  }


  
  NS_ASSERTION(eCSSUnit_Enumerated != aFontData.mFamily.GetUnit(),
               "system fonts should not be in mFamily anymore");
  if (eCSSUnit_Families == aFontData.mFamily.GetUnit()) {
    
    
    if (aGenericFontID == kGenericFont_NONE) {
      
      if (!aFont->mFont.name.IsEmpty())
        aFont->mFont.name.Append((PRUnichar)',');
      
      aFont->mFont.name.Append(defaultVariableFont->name);
    }
    aFont->mFont.familyNameQuirks =
        (aPresContext->CompatibilityMode() == eCompatibility_NavQuirks &&
         aFontData.mFamilyFromHTML);
    aFont->mFont.systemFont = PR_FALSE;
    
    
    
    aFont->mGenericID = aGenericFontID;
  }
  else if (eCSSUnit_System_Font == aFontData.mFamily.GetUnit()) {
    aFont->mFont.name = systemFont.name;
    aFont->mFont.familyNameQuirks = PR_FALSE;
    aFont->mFont.systemFont = PR_TRUE;
    aFont->mGenericID = kGenericFont_NONE;
  }
  else if (eCSSUnit_Inherit == aFontData.mFamily.GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    aFont->mFont.name = aParentFont->mFont.name;
    aFont->mFont.familyNameQuirks = aParentFont->mFont.familyNameQuirks;
    aFont->mFont.systemFont = aParentFont->mFont.systemFont;
    aFont->mGenericID = aParentFont->mGenericID;
  }
  else if (eCSSUnit_Initial == aFontData.mFamily.GetUnit()) {
    aFont->mFont.name = defaultVariableFont->name;
    aFont->mFont.familyNameQuirks = PR_FALSE;
    aFont->mFont.systemFont = defaultVariableFont->systemFont;
    aFont->mGenericID = kGenericFont_NONE;
  }

  
  
  
  
  if (aGenericFontID != kGenericFont_NONE) {
    aFont->mGenericID = aGenericFontID;
  }

  
  SetDiscrete(aFontData.mStyle, aFont->mFont.style, aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NORMAL | SETDSC_SYSTEM_FONT,
              aParentFont->mFont.style,
              defaultVariableFont->style,
              0, 0,
              NS_STYLE_FONT_STYLE_NORMAL,
              systemFont.style);

  
  SetDiscrete(aFontData.mVariant, aFont->mFont.variant, aCanStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NORMAL | SETDSC_SYSTEM_FONT,
              aParentFont->mFont.variant,
              defaultVariableFont->variant,
              0, 0,
              NS_STYLE_FONT_VARIANT_NORMAL,
              systemFont.variant);

  
  
  if (eCSSUnit_Enumerated == aFontData.mWeight.GetUnit()) {
    PRInt32 value = aFontData.mWeight.GetIntValue();
    switch (value) {
      case NS_STYLE_FONT_WEIGHT_NORMAL:
      case NS_STYLE_FONT_WEIGHT_BOLD:
        aFont->mFont.weight = value;
        break;
      case NS_STYLE_FONT_WEIGHT_BOLDER:
      case NS_STYLE_FONT_WEIGHT_LIGHTER:
        aCanStoreInRuleTree = PR_FALSE;
        aFont->mFont.weight = nsStyleUtil::ConstrainFontWeight(aParentFont->mFont.weight + value);
        break;
    }
  } else 
    SetDiscrete(aFontData.mWeight, aFont->mFont.weight, aCanStoreInRuleTree,
                SETDSC_INTEGER | SETDSC_NORMAL | SETDSC_SYSTEM_FONT,
                aParentFont->mFont.weight,
                defaultVariableFont->weight,
                0, 0,
                NS_STYLE_FONT_WEIGHT_NORMAL,
                systemFont.weight);

  
  if (eCSSUnit_Enumerated == aFontData.mStretch.GetUnit()) {
    PRInt32 value = aFontData.mStretch.GetIntValue();
    switch (value) {
      case NS_FONT_STRETCH_WIDER:
      case NS_FONT_STRETCH_NARROWER:
        aCanStoreInRuleTree = PR_FALSE;
        aFont->mFont.stretch = aParentFont->mFont.stretch + value;
        break;
      default:
        aFont->mFont.stretch = value;
        break;
    }
  } else
    SetDiscrete(aFontData.mStretch, aFont->mFont.stretch, aCanStoreInRuleTree,
                SETDSC_NORMAL | SETDSC_SYSTEM_FONT,
                aParentFont->mFont.stretch,
                defaultVariableFont->stretch,
                0, 0, NS_FONT_STRETCH_NORMAL, systemFont.stretch);

#ifdef MOZ_MATHML
  
  

  
  if (aFontData.mScriptMinSize.IsLengthUnit()) {
    
    
    
    aFont->mScriptMinSize =
      CalcLengthWith(aFontData.mScriptMinSize, aParentFont->mSize, aParentFont,
                     nsnull, aPresContext, atRoot, PR_TRUE,
                     aCanStoreInRuleTree);
  }

  
  SetFactor(aFontData.mScriptSizeMultiplier, aFont->mScriptSizeMultiplier,
            aCanStoreInRuleTree, aParentFont->mScriptSizeMultiplier,
            NS_MATHML_DEFAULT_SCRIPT_SIZE_MULTIPLIER,
            SETFCT_POSITIVE);
  
  
  if (eCSSUnit_Integer == aFontData.mScriptLevel.GetUnit()) {
    
    aFont->mScriptLevel = ClampTo8Bit(aParentFont->mScriptLevel + aFontData.mScriptLevel.GetIntValue());
  }
  else if (eCSSUnit_Number == aFontData.mScriptLevel.GetUnit()) {
    
    aFont->mScriptLevel = ClampTo8Bit(PRInt32(aFontData.mScriptLevel.GetFloatValue()));
  }
  else if (eCSSUnit_Inherit == aFontData.mScriptSizeMultiplier.GetUnit()) {
    aCanStoreInRuleTree = PR_FALSE;
    aFont->mScriptLevel = aParentFont->mScriptLevel;
  }
  else if (eCSSUnit_Initial == aFontData.mScriptSizeMultiplier.GetUnit()) {
    aFont->mScriptLevel = 0;
  }
#endif

  
  nscoord scriptLevelAdjustedParentSize = aParentFont->mSize;
#ifdef MOZ_MATHML
  nscoord scriptLevelAdjustedUnconstrainedParentSize;
  scriptLevelAdjustedParentSize =
    ComputeScriptLevelSize(aFont, aParentFont, aPresContext,
                           &scriptLevelAdjustedUnconstrainedParentSize);
  NS_ASSERTION(!aUsedStartStruct || aFont->mScriptUnconstrainedSize == aFont->mSize,
               "If we have a start struct, we should have reset everything coming in here");
#endif
  SetFontSize(aPresContext, aFontData, aFont, aParentFont, &aFont->mSize,
              systemFont, aParentFont->mSize, scriptLevelAdjustedParentSize,
              aUsedStartStruct, atRoot, aCanStoreInRuleTree);
#ifdef MOZ_MATHML
  if (aParentFont->mSize == aParentFont->mScriptUnconstrainedSize &&
      scriptLevelAdjustedParentSize == scriptLevelAdjustedUnconstrainedParentSize) {
    
    
    
    
    
    aFont->mScriptUnconstrainedSize = aFont->mSize;
  } else {
    SetFontSize(aPresContext, aFontData, aFont, aParentFont,
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

  
  if (eCSSUnit_System_Font == aFontData.mSizeAdjust.GetUnit()) {
    aFont->mFont.sizeAdjust = systemFont.sizeAdjust;
  } else
    SetFactor(aFontData.mSizeAdjust, aFont->mFont.sizeAdjust,
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
  
  for (PRInt32 i = contextPath.Length() - 1; i >= 0; --i) {
    nsStyleContext* context = contextPath[i];
    nsRuleDataFont fontData; 
    nsRuleData ruleData(NS_STYLE_INHERIT_BIT(Font), aPresContext, context);
    ruleData.mFontData = &fontData;

    
    
    
    
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
      fontData.mFamily.Reset();

    nsRuleNode::SetFont(aPresContext, context, aMinFontSize,
                        aGenericFontID, fontData, &parentFont, aFont,
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
                            const nsRuleDataStruct& aData, 
                            nsStyleContext* aContext, 
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Font, (mPresContext), font, parentFont,
                          Font, fontData)

  
  
  
  
  
  
  
  
  
  

  
  nscoord minimumFontSize =
    mPresContext->GetCachedIntPref(kPresContext_MinimumFontSize);

  if (minimumFontSize < 0)
    minimumFontSize = 0;

  PRBool useDocumentFonts =
    mPresContext->GetCachedBoolPref(kPresContext_UseDocumentFonts);

  
  
  
  
  if ((!useDocumentFonts || minimumFontSize > 0) && mPresContext->IsChrome()) {
    
    
    useDocumentFonts = PR_TRUE;
    minimumFontSize = 0;
  }

  
  PRUint8 generic = kGenericFont_NONE;
  
  
  if (eCSSUnit_Families == fontData.mFamily.GetUnit()) {
    fontData.mFamily.GetStringValue(font->mFont.name);
    
    
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
                        fontData, parentFont, font,
                        aStartStruct != nsnull, canStoreInRuleTree);
  }
  else {
    
    canStoreInRuleTree = PR_FALSE;
    nsRuleNode::SetGenericFont(mPresContext, aContext, generic,
                               minimumFontSize, font);
  }

  COMPUTE_END_INHERITED(Font, font)
}

already_AddRefed<nsCSSShadowArray>
nsRuleNode::GetShadowData(nsCSSValueList* aList,
                          nsStyleContext* aContext,
                          PRBool aIsBoxShadow,
                          PRBool& canStoreInRuleTree)
{
  PRUint32 arrayLength = 0;
  for (nsCSSValueList *list2 = aList; list2; list2 = list2->mNext)
    ++arrayLength;

  NS_ASSERTION(arrayLength > 0, "Non-null text-shadow list, yet we counted 0 items.");
  nsCSSShadowArray* shadowList = new(arrayLength) nsCSSShadowArray(arrayLength);

  if (!shadowList)
    return nsnull;

  nsStyleCoord tempCoord;
  PRBool unitOK;
  for (nsCSSShadowItem* item = shadowList->ShadowAt(0);
       aList;
       aList = aList->mNext, ++item) {
    nsCSSValue::Array *arr = aList->mValue.GetArrayValue();
    
    unitOK = SetCoord(arr->Item(0), tempCoord, nsStyleCoord(),
                      SETCOORD_LENGTH, aContext, mPresContext,
                      canStoreInRuleTree);
    NS_ASSERTION(unitOK, "unexpected unit");
    item->mXOffset = tempCoord.GetCoordValue();

    unitOK = SetCoord(arr->Item(1), tempCoord, nsStyleCoord(),
                      SETCOORD_LENGTH, aContext, mPresContext,
                      canStoreInRuleTree);
    NS_ASSERTION(unitOK, "unexpected unit");
    item->mYOffset = tempCoord.GetCoordValue();

    
    if (arr->Item(2).GetUnit() != eCSSUnit_Null) {
      unitOK = SetCoord(arr->Item(2), tempCoord, nsStyleCoord(),
                        SETCOORD_LENGTH, aContext, mPresContext,
                        canStoreInRuleTree);
      NS_ASSERTION(unitOK, "unexpected unit");
      item->mRadius = tempCoord.GetCoordValue();
    } else {
      item->mRadius = 0;
    }

    
    if (aIsBoxShadow && arr->Item(3).GetUnit() != eCSSUnit_Null) {
      unitOK = SetCoord(arr->Item(3), tempCoord, nsStyleCoord(),
                        SETCOORD_LENGTH, aContext, mPresContext,
                        canStoreInRuleTree);
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
                            const nsRuleDataStruct& aData, 
                            nsStyleContext* aContext, 
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Text, (), text, parentText, Text, textData)

    
  SetCoord(textData.mLetterSpacing, text->mLetterSpacing, parentText->mLetterSpacing,
           SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL,
           aContext, mPresContext, canStoreInRuleTree);

  
  nsCSSValueList* list = textData.mTextShadow;
  if (list) {
    text->mTextShadow = nsnull;

    
    
    if (eCSSUnit_Inherit == list->mValue.GetUnit()) {
      canStoreInRuleTree = PR_FALSE;
      text->mTextShadow = parentText->mTextShadow;
    } else if (eCSSUnit_Array == list->mValue.GetUnit()) {
      
      text->mTextShadow = GetShadowData(list, aContext, PR_FALSE,
                                        canStoreInRuleTree);
    }
  }

  
  if (eCSSUnit_Percent == textData.mLineHeight.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    
    text->mLineHeight.SetCoordValue(
        nscoord(float(aContext->GetStyleFont()->mFont.size) *
                textData.mLineHeight.GetPercentValue()));
  }
  else if (eCSSUnit_Initial == textData.mLineHeight.GetUnit() ||
           eCSSUnit_System_Font == textData.mLineHeight.GetUnit()) {
    text->mLineHeight.SetNormalValue();
  }
  else {
    SetCoord(textData.mLineHeight, text->mLineHeight, parentText->mLineHeight,
             SETCOORD_LEH | SETCOORD_FACTOR | SETCOORD_NORMAL,
             aContext, mPresContext, canStoreInRuleTree);
    if (textData.mLineHeight.IsFixedLengthUnit() ||
        textData.mLineHeight.GetUnit() == eCSSUnit_Pixel) {
      nscoord lh = nsStyleFont::ZoomText(mPresContext,
                                         text->mLineHeight.GetCoordValue());
      nscoord minimumFontSize =
        mPresContext->GetCachedIntPref(kPresContext_MinimumFontSize);

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


  
  if (eCSSUnit_String == textData.mTextAlign.GetUnit()) {
    NS_NOTYETIMPLEMENTED("align string");
  } else if (eCSSUnit_Enumerated == textData.mTextAlign.GetUnit() &&
             NS_STYLE_TEXT_ALIGN_MOZ_CENTER_OR_INHERIT ==
               textData.mTextAlign.GetIntValue()) {
    canStoreInRuleTree = PR_FALSE;
    PRUint8 parentAlign = parentText->mTextAlign;
    text->mTextAlign = (NS_STYLE_TEXT_ALIGN_DEFAULT == parentAlign) ?
      NS_STYLE_TEXT_ALIGN_CENTER : parentAlign;
  } else
    SetDiscrete(textData.mTextAlign, text->mTextAlign, canStoreInRuleTree,
                SETDSC_ENUMERATED, parentText->mTextAlign,
                NS_STYLE_TEXT_ALIGN_DEFAULT,
                0, 0, 0, 0);

  
  SetCoord(textData.mTextIndent, text->mTextIndent, parentText->mTextIndent,
           SETCOORD_LPH | SETCOORD_INITIAL_ZERO, aContext,
           mPresContext, canStoreInRuleTree);

  
  SetDiscrete(textData.mTextTransform, text->mTextTransform, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentText->mTextTransform,
              NS_STYLE_TEXT_TRANSFORM_NONE, 0,
              NS_STYLE_TEXT_TRANSFORM_NONE, 0, 0);

  
  SetDiscrete(textData.mWhiteSpace, text->mWhiteSpace, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NORMAL, parentText->mWhiteSpace,
              NS_STYLE_WHITESPACE_NORMAL, 0, 0,
              NS_STYLE_WHITESPACE_NORMAL, 0);
 
  
  nsStyleCoord tempCoord;
  if (SetCoord(textData.mWordSpacing, tempCoord,
               nsStyleCoord(parentText->mWordSpacing),
               SETCOORD_LH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL,
               aContext, mPresContext, canStoreInRuleTree)) {
    if (tempCoord.GetUnit() == eStyleUnit_Coord) {
      text->mWordSpacing = tempCoord.GetCoordValue();
    } else if (tempCoord.GetUnit() == eStyleUnit_Normal) {
      text->mWordSpacing = 0;
    } else {
      NS_NOTREACHED("unexpected unit");
    }
  } else {
    NS_ASSERTION(textData.mWordSpacing.GetUnit() == eCSSUnit_Null,
                 "unexpected unit");
  }

  
  SetDiscrete(textData.mWordWrap, text->mWordWrap, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NORMAL, parentText->mWordWrap,
              NS_STYLE_WORDWRAP_NORMAL, 0, 0,
              NS_STYLE_WORDWRAP_NORMAL, 0);

  COMPUTE_END_INHERITED(Text, text)
}

const void*
nsRuleNode::ComputeTextResetData(void* aStartStruct,
                                 const nsRuleDataStruct& aData, 
                                 nsStyleContext* aContext, 
                                 nsRuleNode* aHighestNode,
                                 const RuleDetail aRuleDetail,
                                 const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(TextReset, (), text, parentText, Text, textData)
  
  
  if (!SetCoord(textData.mVerticalAlign, text->mVerticalAlign,
                parentText->mVerticalAlign, SETCOORD_LPH | SETCOORD_ENUMERATED,
                aContext, mPresContext, canStoreInRuleTree)) {
    if (eCSSUnit_Initial == textData.mVerticalAlign.GetUnit()) {
      text->mVerticalAlign.SetIntValue(NS_STYLE_VERTICAL_ALIGN_BASELINE,
                                       eStyleUnit_Enumerated);
    }
  }

  
  if (eCSSUnit_Enumerated == textData.mDecoration.GetUnit()) {
    PRInt32 td = textData.mDecoration.GetIntValue();
    text->mTextDecoration = td;
    if (td & NS_STYLE_TEXT_DECORATION_PREF_ANCHORS) {
      PRBool underlineLinks =
        mPresContext->GetCachedBoolPref(kPresContext_UnderlineLinks);
      if (underlineLinks) {
        text->mTextDecoration |= NS_STYLE_TEXT_DECORATION_UNDERLINE;
      }
      else {
        text->mTextDecoration &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
      }
    }
  }
  else
    SetDiscrete(textData.mDecoration, text->mTextDecoration,
                canStoreInRuleTree, SETDSC_NONE,
                parentText->mTextDecoration,
                NS_STYLE_TEXT_DECORATION_NONE, 0,
                NS_STYLE_TEXT_DECORATION_NONE, 0, 0);

  
  SetDiscrete(textData.mUnicodeBidi, text->mUnicodeBidi, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NORMAL,
              parentText->mUnicodeBidi,
              NS_STYLE_UNICODE_BIDI_NORMAL, 0, 0,
              NS_STYLE_UNICODE_BIDI_NORMAL, 0);

  COMPUTE_END_RESET(TextReset, text)
}

const void*
nsRuleNode::ComputeUserInterfaceData(void* aStartStruct,
                                     const nsRuleDataStruct& aData, 
                                     nsStyleContext* aContext, 
                                     nsRuleNode* aHighestNode,
                                     const RuleDetail aRuleDetail,
                                     const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(UserInterface, (), ui, parentUI,
                          UserInterface, uiData)

  
  nsCSSValueList*  list = uiData.mCursor;
  if (nsnull != list) {
    delete [] ui->mCursorArray;
    ui->mCursorArray = nsnull;
    ui->mCursorArrayLength = 0;

    if (eCSSUnit_Inherit == list->mValue.GetUnit()) {
      canStoreInRuleTree = PR_FALSE;
      ui->mCursor = parentUI->mCursor;
      ui->CopyCursorArrayFrom(*parentUI);
    }
    else if (eCSSUnit_Initial == list->mValue.GetUnit()) {
      ui->mCursor = NS_STYLE_CURSOR_AUTO;
    }
    else {
      
      
      PRUint32 arrayLength = 0;
      nsCSSValueList* list2 = list;
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
              item->mImage = req;
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
      NS_ASSERTION(list->mValue.GetUnit() == eCSSUnit_Enumerated ||
                   list->mValue.GetUnit() == eCSSUnit_Auto,
                   "Unexpected fallback value at end of cursor list");

      if (eCSSUnit_Enumerated == list->mValue.GetUnit()) {
        ui->mCursor = list->mValue.GetIntValue();
      }
      else if (eCSSUnit_Auto == list->mValue.GetUnit()) {
        ui->mCursor = NS_STYLE_CURSOR_AUTO;
      }
    }
  }

  
  SetDiscrete(uiData.mUserInput, ui->mUserInput, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE | SETDSC_AUTO,
              parentUI->mUserInput,
              NS_STYLE_USER_INPUT_AUTO,
              NS_STYLE_USER_INPUT_AUTO,
              NS_STYLE_USER_INPUT_NONE,
              0, 0);

  
  SetDiscrete(uiData.mUserModify, ui->mUserModify, canStoreInRuleTree,
              SETDSC_ENUMERATED,
              parentUI->mUserModify,
              NS_STYLE_USER_MODIFY_READ_ONLY, 
              0, 0, 0, 0);

  
  SetDiscrete(uiData.mUserFocus, ui->mUserFocus, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE | SETDSC_NORMAL,
              parentUI->mUserFocus,
              NS_STYLE_USER_FOCUS_NONE,
              0,
              NS_STYLE_USER_FOCUS_NONE,
              NS_STYLE_USER_FOCUS_NORMAL,
              0);

  COMPUTE_END_INHERITED(UserInterface, ui)
}

const void*
nsRuleNode::ComputeUIResetData(void* aStartStruct,
                               const nsRuleDataStruct& aData, 
                               nsStyleContext* aContext, 
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(UIReset, (), ui, parentUI, UserInterface, uiData)
  
  
  SetDiscrete(uiData.mUserSelect, ui->mUserSelect, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE | SETDSC_AUTO,
              parentUI->mUserSelect,
              NS_STYLE_USER_SELECT_AUTO,
              NS_STYLE_USER_SELECT_AUTO,
              NS_STYLE_USER_SELECT_NONE,
              0, 0);

  
  SetDiscrete(uiData.mIMEMode, ui->mIMEMode, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NORMAL | SETDSC_AUTO,
              parentUI->mIMEMode,
              NS_STYLE_IME_MODE_AUTO,
              NS_STYLE_IME_MODE_AUTO,
              0,
              NS_STYLE_IME_MODE_NORMAL,
              0);

  
  SetDiscrete(uiData.mForceBrokenImageIcon, ui->mForceBrokenImageIcon,
              canStoreInRuleTree,
              SETDSC_INTEGER,
              parentUI->mForceBrokenImageIcon,
              0, 0, 0, 0, 0);

  
  SetDiscrete(uiData.mWindowShadow, ui->mWindowShadow, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentUI->mWindowShadow,
              NS_STYLE_WINDOW_SHADOW_DEFAULT, 0,
              NS_STYLE_WINDOW_SHADOW_NONE, 0, 0);

  COMPUTE_END_RESET(UIReset, ui)
}










static nsStyleTransformMatrix ReadTransforms(const nsCSSValueList* aList,
                                             nsStyleContext* aContext,
                                             nsPresContext* aPresContext,
                                             PRBool &aCanStoreInRuleTree)
{
  nsStyleTransformMatrix result;

  for (const nsCSSValueList* curr = aList; curr != nsnull; curr = curr->mNext) {
    const nsCSSValue &currElem = curr->mValue;
    NS_ASSERTION(currElem.GetUnit() == eCSSUnit_Function,
                 "Stream should consist solely of functions!");
    NS_ASSERTION(currElem.GetArrayValue()->Count() >= 1,
                 "Incoming function is too short!");

    
    nsStyleTransformMatrix currMatrix;
    currMatrix.SetToTransformFunction(currElem.GetArrayValue(), aContext,
                                      aPresContext, aCanStoreInRuleTree);
    result *= currMatrix;
  }
  return result;
}

const void*
nsRuleNode::ComputeDisplayData(void* aStartStruct,
                               const nsRuleDataStruct& aData, 
                               nsStyleContext* aContext, 
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Display, (), display, parentDisplay,
                      Display, displayData)

  
  SetFactor(displayData.mOpacity, display->mOpacity, canStoreInRuleTree,
            parentDisplay->mOpacity, 1.0f, SETFCT_OPACITY);

  
  SetDiscrete(displayData.mDisplay, display->mDisplay, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentDisplay->mDisplay,
              NS_STYLE_DISPLAY_INLINE, 0,
              NS_STYLE_DISPLAY_NONE, 0, 0);

  
  SetDiscrete(displayData.mAppearance, display->mAppearance, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentDisplay->mAppearance,
              NS_THEME_NONE, 0,
              NS_THEME_NONE, 0, 0);

  
  if (eCSSUnit_URL == displayData.mBinding.GetUnit()) {
    nsCSSValue::URL* url = displayData.mBinding.GetURLStructValue();
    NS_ASSERTION(url, "What's going on here?");
    
    if (NS_LIKELY(url->mURI)) {
      display->mBinding = url;
    } else {
      display->mBinding = nsnull;
    }
  }
  else if (eCSSUnit_None == displayData.mBinding.GetUnit() ||
           eCSSUnit_Initial == displayData.mBinding.GetUnit()) {
    display->mBinding = nsnull;
  }
  else if (eCSSUnit_Inherit == displayData.mBinding.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    display->mBinding = parentDisplay->mBinding;
  }

  
  SetDiscrete(displayData.mPosition, display->mPosition, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentDisplay->mPosition,
              NS_STYLE_POSITION_STATIC, 0, 0, 0, 0);

  
  SetDiscrete(displayData.mClear, display->mBreakType, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentDisplay->mBreakType,
              NS_STYLE_CLEAR_NONE, 0,
              NS_STYLE_CLEAR_NONE, 0, 0);

  
  
  
  
  
  if (eCSSUnit_Enumerated == displayData.mBreakBefore.GetUnit()) {
    display->mBreakBefore = (NS_STYLE_PAGE_BREAK_AVOID != displayData.mBreakBefore.GetIntValue());
  }
  else if (eCSSUnit_Auto == displayData.mBreakBefore.GetUnit() ||
           eCSSUnit_Initial == displayData.mBreakBefore.GetUnit()) {
    display->mBreakBefore = PR_FALSE;
  }
  else if (eCSSUnit_Inherit == displayData.mBreakBefore.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    display->mBreakBefore = parentDisplay->mBreakBefore;
  }

  if (eCSSUnit_Enumerated == displayData.mBreakAfter.GetUnit()) {
    display->mBreakAfter = (NS_STYLE_PAGE_BREAK_AVOID != displayData.mBreakAfter.GetIntValue());
  }
  else if (eCSSUnit_Auto == displayData.mBreakAfter.GetUnit() ||
           eCSSUnit_Initial == displayData.mBreakAfter.GetUnit()) {
    display->mBreakAfter = PR_FALSE;
  }
  else if (eCSSUnit_Inherit == displayData.mBreakAfter.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    display->mBreakAfter = parentDisplay->mBreakAfter;
  }
  

  
  SetDiscrete(displayData.mFloat, display->mFloats, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentDisplay->mFloats,
              NS_STYLE_FLOAT_NONE, 0,
              NS_STYLE_FLOAT_NONE, 0, 0);

  
  SetDiscrete(displayData.mOverflowX, display->mOverflowX, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentDisplay->mOverflowX,
              NS_STYLE_OVERFLOW_VISIBLE,
              NS_STYLE_OVERFLOW_AUTO,
              0, 0, 0);

  
  SetDiscrete(displayData.mOverflowY, display->mOverflowY, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentDisplay->mOverflowY,
              NS_STYLE_OVERFLOW_VISIBLE,
              NS_STYLE_OVERFLOW_AUTO,
              0, 0, 0);

  
  
  
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

  
  if (eCSSUnit_Inherit == displayData.mClip.mTop.GetUnit()) { 
    canStoreInRuleTree = PR_FALSE;
    display->mClipFlags = parentDisplay->mClipFlags;
    display->mClip = parentDisplay->mClip;
  }
  
  else if (eCSSUnit_Initial == displayData.mClip.mTop.GetUnit() ||
           eCSSUnit_RectIsAuto == displayData.mClip.mTop.GetUnit()) {
    display->mClipFlags = NS_STYLE_CLIP_AUTO;
    display->mClip.SetRect(0,0,0,0);
  }
  else if (eCSSUnit_Null != displayData.mClip.mTop.GetUnit()) {
    display->mClipFlags = 0; 

    if (eCSSUnit_Auto == displayData.mClip.mTop.GetUnit()) {
      display->mClip.y = 0;
      display->mClipFlags |= NS_STYLE_CLIP_TOP_AUTO;
    } 
    else if (displayData.mClip.mTop.IsLengthUnit()) {
      display->mClip.y = CalcLength(displayData.mClip.mTop, aContext,
                                    mPresContext, canStoreInRuleTree);
    }
    if (eCSSUnit_Auto == displayData.mClip.mBottom.GetUnit()) {
      
      
      
      display->mClip.height = NS_MAXSIZE;
      display->mClipFlags |= NS_STYLE_CLIP_BOTTOM_AUTO;
    } 
    else if (displayData.mClip.mBottom.IsLengthUnit()) {
      display->mClip.height = CalcLength(displayData.mClip.mBottom, aContext,
                                         mPresContext, canStoreInRuleTree) -
                              display->mClip.y;
    }
    if (eCSSUnit_Auto == displayData.mClip.mLeft.GetUnit()) {
      display->mClip.x = 0;
      display->mClipFlags |= NS_STYLE_CLIP_LEFT_AUTO;
    } 
    else if (displayData.mClip.mLeft.IsLengthUnit()) {
      display->mClip.x = CalcLength(displayData.mClip.mLeft, aContext,
                                    mPresContext, canStoreInRuleTree);
    }
    if (eCSSUnit_Auto == displayData.mClip.mRight.GetUnit()) {
      
      
      
      display->mClip.width = NS_MAXSIZE;
      display->mClipFlags |= NS_STYLE_CLIP_RIGHT_AUTO;
    } 
    else if (displayData.mClip.mRight.IsLengthUnit()) {
      display->mClip.width = CalcLength(displayData.mClip.mRight, aContext,
                                        mPresContext, canStoreInRuleTree) -
                             display->mClip.x;
    }
    display->mClipFlags &= ~NS_STYLE_CLIP_TYPE_MASK;
    display->mClipFlags |= NS_STYLE_CLIP_RECT;
  }

  if (display->mDisplay != NS_STYLE_DISPLAY_NONE) {
    
    
    

    if (nsCSSPseudoElements::firstLetter == aContext->GetPseudoType()) {
      
      
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
  
  
  const nsCSSValueList *head = displayData.mTransform;
  
  if (head != nsnull) {
    



    
    
    if (head->mValue.GetUnit() == eCSSUnit_None)
      display->mTransformPresent = PR_FALSE;
    
    
    else if (head->mValue.GetUnit() == eCSSUnit_Inherit)  {
      display->mTransformPresent = parentDisplay->mTransformPresent;
      if (parentDisplay->mTransformPresent)
        display->mTransform = parentDisplay->mTransform;
      canStoreInRuleTree = PR_FALSE;
    }
    
    else if (head->mValue.GetUnit() == eCSSUnit_Initial)
      display->mTransformPresent = PR_FALSE;
    
    



    else {
 
      display->mTransform = 
        ReadTransforms(head, aContext, mPresContext, canStoreInRuleTree);

      
      display->mTransformPresent = PR_TRUE;
    }
  }
  
  
  if (displayData.mTransformOrigin.mXValue.GetUnit() != eCSSUnit_Null ||
      displayData.mTransformOrigin.mXValue.GetUnit() != eCSSUnit_Null) {

    
    if (eCSSUnit_Enumerated == displayData.mTransformOrigin.mXValue.GetUnit())
      display->mTransformOrigin[0].SetPercentValue
        (GetFloatFromBoxPosition
         (displayData.mTransformOrigin.mXValue.GetIntValue()));
    else {
      
#ifdef DEBUG
      PRBool result =
#endif
        SetCoord(displayData.mTransformOrigin.mXValue,
                 display->mTransformOrigin[0],
                 parentDisplay->mTransformOrigin[0],
                 SETCOORD_LPH | SETCOORD_INITIAL_HALF,
                 aContext, mPresContext, canStoreInRuleTree);
      NS_ASSERTION(result, "Malformed -moz-transform-origin parse!");
    }

    
    if (eCSSUnit_Enumerated == displayData.mTransformOrigin.mYValue.GetUnit())
      display->mTransformOrigin[1].SetPercentValue
        (GetFloatFromBoxPosition
         (displayData.mTransformOrigin.mYValue.GetIntValue()));
    else {
      
#ifdef DEBUG
      PRBool result =
#endif
        SetCoord(displayData.mTransformOrigin.mYValue,
                 display->mTransformOrigin[1],
                 parentDisplay->mTransformOrigin[1],
                 SETCOORD_LPH | SETCOORD_INITIAL_HALF,
                 aContext, mPresContext, canStoreInRuleTree);
      NS_ASSERTION(result, "Malformed -moz-transform-origin parse!");
    }
  }

  COMPUTE_END_RESET(Display, display)
}

const void*
nsRuleNode::ComputeVisibilityData(void* aStartStruct,
                                  const nsRuleDataStruct& aData, 
                                  nsStyleContext* aContext, 
                                  nsRuleNode* aHighestNode,
                                  const RuleDetail aRuleDetail,
                                  const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Visibility, (mPresContext),
                          visibility, parentVisibility,
                          Display, displayData)

  
  SetDiscrete(displayData.mDirection, visibility->mDirection,
              canStoreInRuleTree,
              SETDSC_ENUMERATED, parentVisibility->mDirection,
              (GET_BIDI_OPTION_DIRECTION(mPresContext->GetBidi())
               == IBMBIDI_TEXTDIRECTION_RTL)
              ? NS_STYLE_DIRECTION_RTL : NS_STYLE_DIRECTION_LTR,
              0, 0, 0, 0);

  
  SetDiscrete(displayData.mVisibility, visibility->mVisible,
              canStoreInRuleTree,
              SETDSC_ENUMERATED, parentVisibility->mVisible,
              NS_STYLE_VISIBILITY_VISIBLE, 0, 0, 0, 0);

  
  
  if (eCSSUnit_Ident == displayData.mLang.GetUnit()) {
    if (!gLangService) {
      CallGetService(NS_LANGUAGEATOMSERVICE_CONTRACTID, &gLangService);
    }

    if (gLangService) {
      nsAutoString lang;
      displayData.mLang.GetStringValue(lang);
      visibility->mLangGroup = gLangService->LookupLanguage(lang);
    }
  } 

  COMPUTE_END_INHERITED(Visibility, visibility)
}

const void*
nsRuleNode::ComputeColorData(void* aStartStruct,
                             const nsRuleDataStruct& aData, 
                             nsStyleContext* aContext, 
                             nsRuleNode* aHighestNode,
                             const RuleDetail aRuleDetail,
                             const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Color, (mPresContext), color, parentColor,
                          Color, colorData)

  
  
  
  if (colorData.mColor.GetUnit() == eCSSUnit_EnumColor && 
      colorData.mColor.GetIntValue() == NS_COLOR_CURRENTCOLOR) {
    color->mColor = parentColor->mColor;
    canStoreInRuleTree = PR_FALSE;
  }
  else if (colorData.mColor.GetUnit() == eCSSUnit_Initial) {
    color->mColor = mPresContext->DefaultColor();
  }
  else {
    SetColor(colorData.mColor, parentColor->mColor, mPresContext, aContext,
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
struct BackgroundItemComputer<nsCSSValueList, nsStyleBackground::Image>
{
  static void ComputeValue(nsStyleContext* aStyleContext,
                           const nsCSSValueList* aSpecifiedValue,
                           nsStyleBackground::Image& aComputedValue,
                           PRBool& aCanStoreInRuleTree)
  {
    const nsCSSValue &value = aSpecifiedValue->mValue;
    if (eCSSUnit_Image == value.GetUnit()) {
      aComputedValue.SetImageData(value.GetImageValue());
    }
    else if (eCSSUnit_Gradient == value.GetUnit()) {
      nsStyleGradient* gradient = new nsStyleGradient();
      if (gradient) {
        SetGradient(value, aStyleContext->PresContext(), aStyleContext,
                    *gradient, aCanStoreInRuleTree);
        aComputedValue.SetGradientData(gradient);
      } else {
        aComputedValue.SetNull();
      }
    }
    else {
      NS_ASSERTION(eCSSUnit_None == value.GetUnit(), "unexpected unit");
      aComputedValue.SetNull();
    }
  }
};

struct BackgroundPositionAxis {
  nsCSSValue nsCSSValuePairList::*specified;
  nsStyleBackground::Position::PositionCoord
    nsStyleBackground::Position::*result;
  PRPackedBool nsStyleBackground::Position::*isPercent;
};

static const BackgroundPositionAxis gBGPosAxes[] = {
  { &nsCSSValuePairList::mXValue,
    &nsStyleBackground::Position::mXPosition,
    &nsStyleBackground::Position::mXIsPercent },
  { &nsCSSValuePairList::mYValue,
    &nsStyleBackground::Position::mYPosition,
    &nsStyleBackground::Position::mYIsPercent }
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
        (position.*(axis->result)).mFloat = specified.GetPercentValue();
        position.*(axis->isPercent) = PR_TRUE;
      }
      else if (specified.IsLengthUnit()) {
        (position.*(axis->result)).mCoord =
          CalcLength(specified, aStyleContext, aStyleContext->PresContext(),
                     aCanStoreInRuleTree);
        position.*(axis->isPercent) = PR_FALSE;
      }
      else if (eCSSUnit_Enumerated == specified.GetUnit()) {
        (position.*(axis->result)).mFloat =
          GetFloatFromBoxPosition(specified.GetIntValue());
        position.*(axis->isPercent) = PR_TRUE;
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
        (size.*(axis->result)).mFloat = specified.GetPercentValue();
        size.*(axis->type) = nsStyleBackground::Size::ePercentage;
      }
      else {
        NS_ABORT_IF_FALSE(specified.IsLengthUnit(), "unexpected unit");
        (size.*(axis->result)).mCoord =
          CalcLength(specified, aStyleContext, aStyleContext->PresContext(),
                     aCanStoreInRuleTree);
        size.*(axis->type) = nsStyleBackground::Size::eLength;
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


template <class SpecifiedValueItem, class ComputedValueItem>
static void
SetBackgroundList(nsStyleContext* aStyleContext,
                  const SpecifiedValueItem* aValueList,
                  nsAutoTArray< nsStyleBackground::Layer, 1> &aLayers,
                  const nsAutoTArray<nsStyleBackground::Layer, 1>
                                                                 &aParentLayers,
                  ComputedValueItem nsStyleBackground::Layer::* aResultLocation,
                  ComputedValueItem aInitialValue,
                  PRUint32 aParentItemCount,
                  PRUint32& aItemCount,
                  PRUint32& aMaxItemCount,
                  PRBool& aRebuild,
                  PRBool& aCanStoreInRuleTree)
{
  if (aValueList) {
    aRebuild = PR_TRUE;
    nsCSSValue SpecifiedValueItem::* initialInherit =
      InitialInheritLocationFor<SpecifiedValueItem>::Location();
    if (eCSSUnit_Inherit == (aValueList->*initialInherit).GetUnit()) {
      NS_ASSERTION(!aValueList->mNext, "should have only one value");
      aCanStoreInRuleTree = PR_FALSE;
      if (!aLayers.EnsureLengthAtLeast(aParentItemCount)) {
        NS_WARNING("out of memory");
        aParentItemCount = aLayers.Length();
      }
      aItemCount = aParentItemCount;
      for (PRUint32 i = 0; i < aParentItemCount; ++i) {
        aLayers[i].*aResultLocation = aParentLayers[i].*aResultLocation;
      }
    } else if (eCSSUnit_Initial == (aValueList->*initialInherit).GetUnit()) {
      NS_ASSERTION(!aValueList->mNext, "should have only one value");
      aItemCount = 1;
      aLayers[0].*aResultLocation = aInitialValue;
    } else {
      const SpecifiedValueItem *item = aValueList;
      aItemCount = 0;
      do {
        NS_ASSERTION((item->*initialInherit).GetUnit() != eCSSUnit_Inherit &&
                     (item->*initialInherit).GetUnit() != eCSSUnit_Initial,
                     "unexpected unit");
        ++aItemCount;
        if (!aLayers.EnsureLengthAtLeast(aItemCount)) {
          NS_WARNING("out of memory");
          --aItemCount;
          break;
        }
        BackgroundItemComputer<SpecifiedValueItem, ComputedValueItem>
          ::ComputeValue(aStyleContext, item,
                         aLayers[aItemCount-1].*aResultLocation,
                         aCanStoreInRuleTree);
        item = item->mNext;
      } while (item);
    }
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
                                  const nsRuleDataStruct& aData, 
                                  nsStyleContext* aContext, 
                                  nsRuleNode* aHighestNode,
                                  const RuleDetail aRuleDetail,
                                  const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Background, (), bg, parentBG, Color, colorData)

  
  if (eCSSUnit_Initial == colorData.mBackColor.GetUnit()) {
    bg->mBackgroundColor = NS_RGBA(0, 0, 0, 0);
  } else if (!SetColor(colorData.mBackColor, parentBG->mBackgroundColor,
                       mPresContext, aContext, bg->mBackgroundColor,
                       canStoreInRuleTree)) {
    NS_ASSERTION(eCSSUnit_Null == colorData.mBackColor.GetUnit(),
                 "unexpected color unit");
  }

  PRUint32 maxItemCount = 1;
  PRBool rebuild = PR_FALSE;

  
  nsStyleBackground::Image initialImage;
  SetBackgroundList(aContext, colorData.mBackImage, bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mImage,
                    initialImage, parentBG->mImageCount, bg->mImageCount,
                    maxItemCount, rebuild, canStoreInRuleTree);

  
  SetBackgroundList(aContext, colorData.mBackRepeat, bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mRepeat,
                    PRUint8(NS_STYLE_BG_REPEAT_XY), parentBG->mRepeatCount,
                    bg->mRepeatCount, maxItemCount, rebuild, canStoreInRuleTree);

  
  SetBackgroundList(aContext, colorData.mBackAttachment, bg->mLayers,
                    parentBG->mLayers,
                    &nsStyleBackground::Layer::mAttachment,
                    PRUint8(NS_STYLE_BG_ATTACHMENT_SCROLL),
                    parentBG->mAttachmentCount,
                    bg->mAttachmentCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  SetBackgroundList(aContext, colorData.mBackClip, bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mClip,
                    PRUint8(NS_STYLE_BG_CLIP_BORDER), parentBG->mClipCount,
                    bg->mClipCount, maxItemCount, rebuild, canStoreInRuleTree);

  
  SetDiscrete(colorData.mBackInlinePolicy, bg->mBackgroundInlinePolicy,
              canStoreInRuleTree, SETDSC_ENUMERATED,
              parentBG->mBackgroundInlinePolicy,
              NS_STYLE_BG_INLINE_POLICY_CONTINUOUS, 0, 0, 0, 0);

  
  SetBackgroundList(aContext, colorData.mBackOrigin, bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mOrigin,
                    PRUint8(NS_STYLE_BG_ORIGIN_PADDING), parentBG->mOriginCount,
                    bg->mOriginCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  nsStyleBackground::Position initialPosition;
  initialPosition.SetInitialValues();
  SetBackgroundList(aContext, colorData.mBackPosition, bg->mLayers,
                    parentBG->mLayers, &nsStyleBackground::Layer::mPosition,
                    initialPosition, parentBG->mPositionCount,
                    bg->mPositionCount, maxItemCount, rebuild,
                    canStoreInRuleTree);

  
  nsStyleBackground::Size initialSize;
  initialSize.SetInitialValues();
  SetBackgroundList(aContext, colorData.mBackSize, bg->mLayers,
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

  COMPUTE_END_RESET(Background, bg)
}

const void*
nsRuleNode::ComputeMarginData(void* aStartStruct,
                              const nsRuleDataStruct& aData, 
                              nsStyleContext* aContext, 
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Margin, (), margin, parentMargin, Margin, marginData)

  
  nsStyleCoord  coord;
  nsCSSRect ourMargin(marginData.mMargin);
  AdjustLogicalBoxProp(aContext,
                       marginData.mMarginLeftLTRSource,
                       marginData.mMarginLeftRTLSource,
                       marginData.mMarginStart, marginData.mMarginEnd,
                       NS_SIDE_LEFT, ourMargin, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       marginData.mMarginRightLTRSource,
                       marginData.mMarginRightRTLSource,
                       marginData.mMarginEnd, marginData.mMarginStart,
                       NS_SIDE_RIGHT, ourMargin, canStoreInRuleTree);
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentMargin->mMargin.Get(side);
    if (SetCoord(ourMargin.*(nsCSSRect::sides[side]),
                 coord, parentCoord, SETCOORD_LPAH | SETCOORD_INITIAL_ZERO,
                 aContext, mPresContext, canStoreInRuleTree)) {
      margin->mMargin.Set(side, coord);
    }
  }

  margin->RecalcData();
  COMPUTE_END_RESET(Margin, margin)
}

const void* 
nsRuleNode::ComputeBorderData(void* aStartStruct,
                              const nsRuleDataStruct& aData, 
                              nsStyleContext* aContext, 
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Border, (mPresContext), border, parentBorder,
                      Margin, marginData)

  
  {
    nsCSSValueList* list = marginData.mBoxShadow;
    if (list) {
      
      border->mBoxShadow = nsnull;

      if (eCSSUnit_Inherit == list->mValue.GetUnit()) {
        canStoreInRuleTree = PR_FALSE;
        border->mBoxShadow = parentBorder->mBoxShadow;
      } else if (eCSSUnit_Array == list->mValue.GetUnit()) {
        
        border->mBoxShadow = GetShadowData(list, aContext, PR_TRUE,
                                           canStoreInRuleTree);
      }
    }
  }

  
  nsStyleCoord  coord;
  nsCSSRect ourBorderWidth(marginData.mBorderWidth);
  AdjustLogicalBoxProp(aContext,
                       marginData.mBorderLeftWidthLTRSource,
                       marginData.mBorderLeftWidthRTLSource,
                       marginData.mBorderStartWidth,
                       marginData.mBorderEndWidth,
                       NS_SIDE_LEFT, ourBorderWidth, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       marginData.mBorderRightWidthLTRSource,
                       marginData.mBorderRightWidthRTLSource,
                       marginData.mBorderEndWidth,
                       marginData.mBorderStartWidth,
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
      
      else if (SetCoord(value, coord, nsStyleCoord(), SETCOORD_LENGTH,
                        aContext, mPresContext, canStoreInRuleTree)) {
        NS_ASSERTION(coord.GetUnit() == eStyleUnit_Coord, "unexpected unit");
        border->SetBorderWidth(side, coord.GetCoordValue());
      }
      else if (eCSSUnit_Inherit == value.GetUnit()) {
        canStoreInRuleTree = PR_FALSE;
        border->SetBorderWidth(side,
                               parentBorder->GetComputedBorder().side(side));
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

  
  nsCSSRect ourStyle(marginData.mBorderStyle);
  AdjustLogicalBoxProp(aContext,
                       marginData.mBorderLeftStyleLTRSource,
                       marginData.mBorderLeftStyleRTLSource,
                       marginData.mBorderStartStyle, marginData.mBorderEndStyle,
                       NS_SIDE_LEFT, ourStyle, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       marginData.mBorderRightStyleLTRSource,
                       marginData.mBorderRightStyleRTLSource,
                       marginData.mBorderEndStyle, marginData.mBorderStartStyle,
                       NS_SIDE_RIGHT, ourStyle, canStoreInRuleTree);
  { 
    NS_FOR_CSS_SIDES(side) {
      const nsCSSValue &value = ourStyle.*(nsCSSRect::sides[side]);
      nsCSSUnit unit = value.GetUnit();
      if (eCSSUnit_Enumerated == unit) {
        border->SetBorderStyle(side, value.GetIntValue());
      }
      else if (eCSSUnit_None == unit || eCSSUnit_Initial == unit) {
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
  
  { 
    NS_FOR_CSS_SIDES(side) {
      nsCSSValueList* list =
          marginData.mBorderColors.*(nsCSSValueListRect::sides[side]);
      if (list) {
        if (eCSSUnit_Initial == list->mValue.GetUnit() ||
            eCSSUnit_None == list->mValue.GetUnit()) {
          NS_ASSERTION(!list->mNext, "should have only one item");
          border->ClearBorderColors(side);
        }
        else if (eCSSUnit_Inherit == list->mValue.GetUnit()) {
          canStoreInRuleTree = PR_FALSE;
          NS_ASSERTION(!list->mNext, "should have only one item");
          nsBorderColors *parentColors;
          parentBorder->GetCompositeColors(side, &parentColors);
          if (parentColors) {
            border->EnsureBorderColors();
            border->ClearBorderColors(side);
            border->mBorderColors[side] = parentColors->Clone();
          } else {
            border->ClearBorderColors(side);
          }
        }
        else {
          
          
          border->EnsureBorderColors();
          border->ClearBorderColors(side);
          while (list) {
            if (SetColor(list->mValue, unused, mPresContext,
                         aContext, borderColor, canStoreInRuleTree))
              border->AppendBorderColor(side, borderColor);
            else {
              NS_NOTREACHED("unexpected item in -moz-border-*-colors list");
            }
            list = list->mNext;
          }
        }
      }
    }
  }

  
  nsCSSRect ourBorderColor(marginData.mBorderColor);
  PRBool foreground;
  AdjustLogicalBoxProp(aContext,
                       marginData.mBorderLeftColorLTRSource,
                       marginData.mBorderLeftColorRTLSource,
                       marginData.mBorderStartColor, marginData.mBorderEndColor,
                       NS_SIDE_LEFT, ourBorderColor, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       marginData.mBorderRightColorLTRSource,
                       marginData.mBorderRightColorRTLSource,
                       marginData.mBorderEndColor, marginData.mBorderStartColor,
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
    const nsCSSCornerSizes& borderRadius = marginData.mBorderRadius;
    NS_FOR_CSS_HALF_CORNERS(corner) {
      nsStyleCoord parentCoord = parentBorder->mBorderRadius.Get(corner);
      if (SetCoord(borderRadius.GetHalfCorner(corner),
                   coord, parentCoord, SETCOORD_LPH | SETCOORD_INITIAL_ZERO,
                   aContext, mPresContext, canStoreInRuleTree))
        border->mBorderRadius.Set(corner, coord);
    }
  }

  
  SetDiscrete(marginData.mFloatEdge, border->mFloatEdge, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentBorder->mFloatEdge,
              NS_STYLE_FLOAT_EDGE_CONTENT, 0, 0, 0, 0);
  
  
  if (eCSSUnit_Array == marginData.mBorderImage.GetUnit()) {
    nsCSSValue::Array *arr = marginData.mBorderImage.GetArrayValue();
    
    
    if (eCSSUnit_Image == arr->Item(0).GetUnit()) {
      border->SetBorderImage(arr->Item(0).GetImageValue());
    }
    
    
    NS_FOR_CSS_SIDES(side) {
      
      if (SetCoord(arr->Item(1 + side), coord, nsStyleCoord(),
                   SETCOORD_FACTOR | SETCOORD_PERCENT, aContext,
                   mPresContext, canStoreInRuleTree)) {
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
  } else if (eCSSUnit_None == marginData.mBorderImage.GetUnit() ||
             eCSSUnit_Initial == marginData.mBorderImage.GetUnit()) {
    border->mHaveBorderImageWidth = PR_FALSE;
    border->SetBorderImage(nsnull);
  } else if (eCSSUnit_Inherit == marginData.mBorderImage.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    NS_FOR_CSS_SIDES(side) {
      border->SetBorderImageWidthOverride(side, parentBorder->mBorderImageWidth.side(side));
    }
    border->mBorderImageSplit = parentBorder->mBorderImageSplit;
    border->mBorderImageHFill = parentBorder->mBorderImageHFill;
    border->mBorderImageVFill = parentBorder->mBorderImageVFill;
    border->mHaveBorderImageWidth = parentBorder->mHaveBorderImageWidth;
    border->SetBorderImage(parentBorder->GetBorderImage());
  }

  COMPUTE_END_RESET(Border, border)
}
  
const void*
nsRuleNode::ComputePaddingData(void* aStartStruct,
                               const nsRuleDataStruct& aData, 
                               nsStyleContext* aContext, 
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Padding, (), padding, parentPadding, Margin, marginData)

  
  nsStyleCoord  coord;
  nsCSSRect ourPadding(marginData.mPadding);
  AdjustLogicalBoxProp(aContext,
                       marginData.mPaddingLeftLTRSource,
                       marginData.mPaddingLeftRTLSource,
                       marginData.mPaddingStart, marginData.mPaddingEnd,
                       NS_SIDE_LEFT, ourPadding, canStoreInRuleTree);
  AdjustLogicalBoxProp(aContext,
                       marginData.mPaddingRightLTRSource,
                       marginData.mPaddingRightRTLSource,
                       marginData.mPaddingEnd, marginData.mPaddingStart,
                       NS_SIDE_RIGHT, ourPadding, canStoreInRuleTree);
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentPadding->mPadding.Get(side);
    if (SetCoord(ourPadding.*(nsCSSRect::sides[side]),
                 coord, parentCoord, SETCOORD_LPH | SETCOORD_INITIAL_ZERO,
                 aContext, mPresContext, canStoreInRuleTree)) {
      padding->mPadding.Set(side, coord);
    }
  }

  padding->RecalcData();
  COMPUTE_END_RESET(Padding, padding)
}

const void*
nsRuleNode::ComputeOutlineData(void* aStartStruct,
                               const nsRuleDataStruct& aData, 
                               nsStyleContext* aContext, 
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Outline, (mPresContext), outline, parentOutline,
                      Margin, marginData)

  
  if (eCSSUnit_Initial == marginData.mOutlineWidth.GetUnit()) {
    outline->mOutlineWidth =
      nsStyleCoord(NS_STYLE_BORDER_WIDTH_MEDIUM, eStyleUnit_Enumerated);
  }
  else {
    SetCoord(marginData.mOutlineWidth, outline->mOutlineWidth,
             parentOutline->mOutlineWidth, SETCOORD_LEH, aContext,
             mPresContext, canStoreInRuleTree);
  }

  
  nsStyleCoord tempCoord;
  if (SetCoord(marginData.mOutlineOffset, tempCoord,
               parentOutline->mOutlineOffset,
               SETCOORD_LH | SETCOORD_INITIAL_ZERO, aContext, mPresContext,
               canStoreInRuleTree)) {
    outline->mOutlineOffset = tempCoord.GetCoordValue();
  } else {
    NS_ASSERTION(marginData.mOutlineOffset.GetUnit() == eCSSUnit_Null,
                 "unexpected unit");
  }

  
  nscolor outlineColor;
  nscolor unused = NS_RGB(0,0,0);
  if (eCSSUnit_Inherit == marginData.mOutlineColor.GetUnit()) {
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
  else if (SetColor(marginData.mOutlineColor, unused, mPresContext,
                    aContext, outlineColor, canStoreInRuleTree))
    outline->SetOutlineColor(outlineColor);
  else if (eCSSUnit_Enumerated == marginData.mOutlineColor.GetUnit() ||
           eCSSUnit_Initial == marginData.mOutlineColor.GetUnit()) {
    outline->SetOutlineInitialColor();
  }

  
  { 
    nsStyleCoord coord;
    const nsCSSCornerSizes& outlineRadius = marginData.mOutlineRadius;
    NS_FOR_CSS_HALF_CORNERS(corner) {
      nsStyleCoord parentCoord = parentOutline->mOutlineRadius.Get(corner);
      if (SetCoord(outlineRadius.GetHalfCorner(corner),
                   coord, parentCoord, SETCOORD_LPH | SETCOORD_INITIAL_ZERO,
                   aContext, mPresContext, canStoreInRuleTree))
        outline->mOutlineRadius.Set(corner, coord);
    }
  }

  
  
  if (eCSSUnit_Enumerated == marginData.mOutlineStyle.GetUnit())
    outline->SetOutlineStyle(marginData.mOutlineStyle.GetIntValue());
  else if (eCSSUnit_None == marginData.mOutlineStyle.GetUnit() ||
           eCSSUnit_Initial == marginData.mOutlineStyle.GetUnit())
    outline->SetOutlineStyle(NS_STYLE_BORDER_STYLE_NONE);
  else if (eCSSUnit_Auto == marginData.mOutlineStyle.GetUnit()) {
    outline->SetOutlineStyle(NS_STYLE_BORDER_STYLE_AUTO);
  } else if (eCSSUnit_Inherit == marginData.mOutlineStyle.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    outline->SetOutlineStyle(parentOutline->GetOutlineStyle());
  }

  outline->RecalcData(mPresContext);
  COMPUTE_END_RESET(Outline, outline)
}

const void* 
nsRuleNode::ComputeListData(void* aStartStruct,
                            const nsRuleDataStruct& aData, 
                            nsStyleContext* aContext, 
                            nsRuleNode* aHighestNode,
                            const RuleDetail aRuleDetail,
                            const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(List, (), list, parentList, List, listData)

  
  SetDiscrete(listData.mType, list->mListStyleType, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentList->mListStyleType,
              NS_STYLE_LIST_STYLE_DISC, 0,
              NS_STYLE_LIST_STYLE_NONE, 0, 0);

  
  if (eCSSUnit_Image == listData.mImage.GetUnit()) {
    list->mListStyleImage = listData.mImage.GetImageValue();
  }
  else if (eCSSUnit_None == listData.mImage.GetUnit() ||
           eCSSUnit_Initial == listData.mImage.GetUnit()) {
    list->mListStyleImage = nsnull;
  }
  else if (eCSSUnit_Inherit == listData.mImage.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    list->mListStyleImage = parentList->mListStyleImage;
  }

  
  SetDiscrete(listData.mPosition, list->mListStylePosition, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentList->mListStylePosition,
              NS_STYLE_LIST_STYLE_POSITION_OUTSIDE, 0, 0, 0, 0);

  
  if (eCSSUnit_Inherit == listData.mImageRegion.mTop.GetUnit()) { 
    canStoreInRuleTree = PR_FALSE;
    list->mImageRegion = parentList->mImageRegion;
  }
  
  else if (eCSSUnit_Initial == listData.mImageRegion.mTop.GetUnit() ||
           eCSSUnit_RectIsAuto == listData.mImageRegion.mTop.GetUnit()) {
    list->mImageRegion.Empty();
  }
  else if (eCSSUnit_Null != listData.mImageRegion.mTop.GetUnit()) {
    if (eCSSUnit_Auto == listData.mImageRegion.mTop.GetUnit())
      list->mImageRegion.y = 0;
    else if (listData.mImageRegion.mTop.IsLengthUnit())
      list->mImageRegion.y =
        CalcLength(listData.mImageRegion.mTop, aContext, mPresContext,
                   canStoreInRuleTree);
      
    if (eCSSUnit_Auto == listData.mImageRegion.mBottom.GetUnit())
      list->mImageRegion.height = 0;
    else if (listData.mImageRegion.mBottom.IsLengthUnit())
      list->mImageRegion.height =
        CalcLength(listData.mImageRegion.mBottom, aContext, mPresContext,
                   canStoreInRuleTree) -
        list->mImageRegion.y;
  
    if (eCSSUnit_Auto == listData.mImageRegion.mLeft.GetUnit())
      list->mImageRegion.x = 0;
    else if (listData.mImageRegion.mLeft.IsLengthUnit())
      list->mImageRegion.x =
        CalcLength(listData.mImageRegion.mLeft, aContext, mPresContext,
                   canStoreInRuleTree);
      
    if (eCSSUnit_Auto == listData.mImageRegion.mRight.GetUnit())
      list->mImageRegion.width = 0;
    else if (listData.mImageRegion.mRight.IsLengthUnit())
      list->mImageRegion.width =
        CalcLength(listData.mImageRegion.mRight, aContext, mPresContext,
                   canStoreInRuleTree) -
        list->mImageRegion.x;
  }

  COMPUTE_END_INHERITED(List, list)
}

const void* 
nsRuleNode::ComputePositionData(void* aStartStruct,
                                const nsRuleDataStruct& aData, 
                                nsStyleContext* aContext, 
                                nsRuleNode* aHighestNode,
                                const RuleDetail aRuleDetail,
                                const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Position, (), pos, parentPos, Position, posData)

  
  nsStyleCoord  coord;
  NS_FOR_CSS_SIDES(side) {
    nsStyleCoord parentCoord = parentPos->mOffset.Get(side);
    if (SetCoord(posData.mOffset.*(nsCSSRect::sides[side]),
                 coord, parentCoord, SETCOORD_LPAH | SETCOORD_INITIAL_AUTO,
                 aContext, mPresContext, canStoreInRuleTree)) {
      pos->mOffset.Set(side, coord);
    }
  }

  SetCoord(posData.mWidth, pos->mWidth, parentPos->mWidth,
           SETCOORD_LPAEH | SETCOORD_INITIAL_AUTO, aContext,
           mPresContext, canStoreInRuleTree);
  SetCoord(posData.mMinWidth, pos->mMinWidth, parentPos->mMinWidth,
           SETCOORD_LPEH | SETCOORD_INITIAL_ZERO, aContext,
           mPresContext, canStoreInRuleTree);
  SetCoord(posData.mMaxWidth, pos->mMaxWidth, parentPos->mMaxWidth,
           SETCOORD_LPOEH | SETCOORD_INITIAL_NONE, aContext,
           mPresContext, canStoreInRuleTree);

  SetCoord(posData.mHeight, pos->mHeight, parentPos->mHeight,
           SETCOORD_LPAH | SETCOORD_INITIAL_AUTO, aContext,
           mPresContext, canStoreInRuleTree);
  SetCoord(posData.mMinHeight, pos->mMinHeight, parentPos->mMinHeight,
           SETCOORD_LPH | SETCOORD_INITIAL_ZERO, aContext,
           mPresContext, canStoreInRuleTree);
  SetCoord(posData.mMaxHeight, pos->mMaxHeight, parentPos->mMaxHeight,
           SETCOORD_LPOH | SETCOORD_INITIAL_NONE, aContext,
           mPresContext, canStoreInRuleTree);

  
  SetDiscrete(posData.mBoxSizing, pos->mBoxSizing, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentPos->mBoxSizing,
              NS_STYLE_BOX_SIZING_CONTENT, 0, 0, 0, 0);

  
  if (! SetCoord(posData.mZIndex, pos->mZIndex, parentPos->mZIndex,
                 SETCOORD_IA | SETCOORD_INITIAL_AUTO, aContext,
                 nsnull, canStoreInRuleTree)) {
    if (eCSSUnit_Inherit == posData.mZIndex.GetUnit()) {
      
      canStoreInRuleTree = PR_FALSE;
      pos->mZIndex = parentPos->mZIndex;
    }
  }

  COMPUTE_END_RESET(Position, pos)
}

const void* 
nsRuleNode::ComputeTableData(void* aStartStruct,
                             const nsRuleDataStruct& aData, 
                             nsStyleContext* aContext, 
                             nsRuleNode* aHighestNode,
                             const RuleDetail aRuleDetail,
                             const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Table, (), table, parentTable, Table, tableData)

  
  SetDiscrete(tableData.mLayout, table->mLayoutStrategy, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentTable->mLayoutStrategy,
              NS_STYLE_TABLE_LAYOUT_AUTO,
              NS_STYLE_TABLE_LAYOUT_AUTO, 0, 0, 0);

  
  if (eCSSUnit_Enumerated == tableData.mRules.GetUnit())
    table->mRules = tableData.mRules.GetIntValue();

  
  if (eCSSUnit_Enumerated == tableData.mFrame.GetUnit())
    table->mFrame = tableData.mFrame.GetIntValue();

  
  if (eCSSUnit_Enumerated == tableData.mCols.GetUnit() ||
      eCSSUnit_Integer == tableData.mCols.GetUnit())
    table->mCols = tableData.mCols.GetIntValue();

  
  if (eCSSUnit_Enumerated == tableData.mSpan.GetUnit() ||
      eCSSUnit_Integer == tableData.mSpan.GetUnit())
    table->mSpan = tableData.mSpan.GetIntValue();
    
  COMPUTE_END_RESET(Table, table)
}

const void* 
nsRuleNode::ComputeTableBorderData(void* aStartStruct,
                                   const nsRuleDataStruct& aData, 
                                   nsStyleContext* aContext, 
                                   nsRuleNode* aHighestNode,
                                   const RuleDetail aRuleDetail,
                                   const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(TableBorder, (mPresContext), table, parentTable,
                          Table, tableData)

  
  SetDiscrete(tableData.mBorderCollapse, table->mBorderCollapse,
              canStoreInRuleTree,
              SETDSC_ENUMERATED, parentTable->mBorderCollapse,
              NS_STYLE_BORDER_SEPARATE, 0, 0, 0, 0);

  
  nsStyleCoord tempCoord;
  if (SetCoord(tableData.mBorderSpacing.mXValue, tempCoord,
               parentTable->mBorderSpacingX,
               SETCOORD_LH | SETCOORD_INITIAL_ZERO,
               aContext, mPresContext, canStoreInRuleTree)) {
    table->mBorderSpacingX = tempCoord.GetCoordValue();
  } else {
    NS_ASSERTION(tableData.mBorderSpacing.mXValue.GetUnit() == eCSSUnit_Null,
                 "unexpected unit");
  }

  
  if (SetCoord(tableData.mBorderSpacing.mYValue, tempCoord,
               parentTable->mBorderSpacingY,
               SETCOORD_LH | SETCOORD_INITIAL_ZERO,
               aContext, mPresContext, canStoreInRuleTree)) {
    table->mBorderSpacingY = tempCoord.GetCoordValue();
  } else {
    NS_ASSERTION(tableData.mBorderSpacing.mYValue.GetUnit() == eCSSUnit_Null,
                 "unexpected unit");
  }

  
  SetDiscrete(tableData.mCaptionSide, table->mCaptionSide, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentTable->mCaptionSide,
              NS_STYLE_CAPTION_SIDE_TOP, 0, 0, 0, 0);

  
  SetDiscrete(tableData.mEmptyCells, table->mEmptyCells, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentTable->mEmptyCells,
              (mPresContext->CompatibilityMode() == eCompatibility_NavQuirks)
              ? NS_STYLE_TABLE_EMPTY_CELLS_SHOW_BACKGROUND     
              : NS_STYLE_TABLE_EMPTY_CELLS_SHOW,
              0, 0, 0, 0);

  COMPUTE_END_INHERITED(TableBorder, table)
}

const void* 
nsRuleNode::ComputeContentData(void* aStartStruct,
                               const nsRuleDataStruct& aData, 
                               nsStyleContext* aContext, 
                               nsRuleNode* aHighestNode,
                               const RuleDetail aRuleDetail,
                               const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Content, (), content, parentContent,
                      Content, contentData)

  
  PRUint32 count;
  nsAutoString  buffer;
  nsCSSValueList* contentValue = contentData.mContent;
  if (contentValue) {
    if (eCSSUnit_Normal == contentValue->mValue.GetUnit() ||
        eCSSUnit_None == contentValue->mValue.GetUnit() ||
        eCSSUnit_Initial == contentValue->mValue.GetUnit()) {
      
      content->AllocateContents(0);
    }
    else if (eCSSUnit_Inherit == contentValue->mValue.GetUnit()) {
      canStoreInRuleTree = PR_FALSE;
      count = parentContent->ContentCount();
      if (NS_SUCCEEDED(content->AllocateContents(count))) {
        while (0 < count--) {
          content->ContentAt(count) = parentContent->ContentAt(count);
        }
      }
    }
    else {
      count = 0;
      while (contentValue) {
        count++;
        contentValue = contentValue->mNext;
      }
      if (NS_SUCCEEDED(content->AllocateContents(count))) {
        const nsAutoString  nullStr;
        count = 0;
        contentValue = contentData.mContent;
        while (contentValue) {
          const nsCSSValue& value = contentValue->mValue;
          nsCSSUnit unit = value.GetUnit();
          nsStyleContentType type;
          nsStyleContentData &data = content->ContentAt(count++);
          switch (unit) {
            case eCSSUnit_String:   type = eStyleContentType_String;    break;
            case eCSSUnit_Image:    type = eStyleContentType_Image;       break;
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
                case NS_STYLE_CONTENT_ALT_CONTENT:
                  type = eStyleContentType_AltContent;    break;
                default:
                  NS_ERROR("bad content value");
              }
              break;
            default:
              NS_ERROR("bad content type");
          }
          data.mType = type;
          if (type == eStyleContentType_Image) {
            data.mContent.mImage = value.GetImageValue();
            NS_IF_ADDREF(data.mContent.mImage);
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
          contentValue = contentValue->mNext;
        }
      } 
    }
  }

  
  nsCSSValuePairList* ourIncrement = contentData.mCounterIncrement;
  if (ourIncrement) {
    if (eCSSUnit_None == ourIncrement->mXValue.GetUnit() ||
        eCSSUnit_Initial == ourIncrement->mXValue.GetUnit()) {
      content->AllocateCounterIncrements(0);
    }
    else if (eCSSUnit_Inherit == ourIncrement->mXValue.GetUnit()) {
      canStoreInRuleTree = PR_FALSE;
      count = parentContent->CounterIncrementCount();
      if (NS_SUCCEEDED(content->AllocateCounterIncrements(count))) {
        while (0 < count--) {
          const nsStyleCounterData *data =
            parentContent->GetCounterIncrementAt(count);
          content->SetCounterIncrementAt(count, data->mCounter, data->mValue);
        }
      }
    }
    else if (eCSSUnit_Ident == ourIncrement->mXValue.GetUnit()) {
      count = 0;
      while (ourIncrement) {
        count++;
        ourIncrement = ourIncrement->mNext;
      }
      if (NS_SUCCEEDED(content->AllocateCounterIncrements(count))) {
        count = 0;
        ourIncrement = contentData.mCounterIncrement;
        while (ourIncrement) {
          PRInt32 increment;
          if (eCSSUnit_Integer == ourIncrement->mYValue.GetUnit()) {
            increment = ourIncrement->mYValue.GetIntValue();
          }
          else {
            increment = 1;
          }
          ourIncrement->mXValue.GetStringValue(buffer);
          content->SetCounterIncrementAt(count++, buffer, increment);
          ourIncrement = ourIncrement->mNext;
        }
      }
    }
  }

  
  nsCSSValuePairList* ourReset = contentData.mCounterReset;
  if (ourReset) {
    if (eCSSUnit_None == ourReset->mXValue.GetUnit() ||
        eCSSUnit_Initial == ourReset->mXValue.GetUnit()) {
      content->AllocateCounterResets(0);
    }
    else if (eCSSUnit_Inherit == ourReset->mXValue.GetUnit()) {
      canStoreInRuleTree = PR_FALSE;
      count = parentContent->CounterResetCount();
      if (NS_SUCCEEDED(content->AllocateCounterResets(count))) {
        while (0 < count--) {
          const nsStyleCounterData *data =
            parentContent->GetCounterResetAt(count);
          content->SetCounterResetAt(count, data->mCounter, data->mValue);
        }
      }
    }
    else if (eCSSUnit_Ident == ourReset->mXValue.GetUnit()) {
      count = 0;
      while (ourReset) {
        count++;
        ourReset = ourReset->mNext;
      }
      if (NS_SUCCEEDED(content->AllocateCounterResets(count))) {
        count = 0;
        ourReset = contentData.mCounterReset;
        while (ourReset) {
          PRInt32 reset;
          if (eCSSUnit_Integer == ourReset->mYValue.GetUnit()) {
            reset = ourReset->mYValue.GetIntValue();
          }
          else {
            reset = 0;
          }
          ourReset->mXValue.GetStringValue(buffer);
          content->SetCounterResetAt(count++, buffer, reset);
          ourReset = ourReset->mNext;
        }
      }
    }
  }

  
  SetCoord(contentData.mMarkerOffset, content->mMarkerOffset, parentContent->mMarkerOffset,
           SETCOORD_LH | SETCOORD_AUTO | SETCOORD_INITIAL_AUTO, aContext,
           mPresContext, canStoreInRuleTree);
    
  COMPUTE_END_RESET(Content, content)
}

const void* 
nsRuleNode::ComputeQuotesData(void* aStartStruct,
                              const nsRuleDataStruct& aData, 
                              nsStyleContext* aContext, 
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(Quotes, (), quotes, parentQuotes,
                          Content, contentData)

  
  nsCSSValuePairList* ourQuotes = contentData.mQuotes;
  if (ourQuotes) {
    if (eCSSUnit_Inherit == ourQuotes->mXValue.GetUnit()) {
      canStoreInRuleTree = PR_FALSE;
      quotes->CopyFrom(*parentQuotes);
    }
    else if (eCSSUnit_Initial == ourQuotes->mXValue.GetUnit()) {
      quotes->SetInitial();
    }
    else if (eCSSUnit_None == ourQuotes->mXValue.GetUnit()) {
      quotes->AllocateQuotes(0);
    }
    else if (eCSSUnit_String == ourQuotes->mXValue.GetUnit()) {
      nsAutoString  buffer;
      nsAutoString  closeBuffer;
      PRUint32 count = 0;

      while (ourQuotes) {
        count++;
        ourQuotes = ourQuotes->mNext;
      }
      if (NS_SUCCEEDED(quotes->AllocateQuotes(count))) {
        count = 0;
        ourQuotes = contentData.mQuotes;
        while (ourQuotes) {
          ourQuotes->mXValue.GetStringValue(buffer);
          ourQuotes->mYValue.GetStringValue(closeBuffer);
          quotes->SetQuotesAt(count++, buffer, closeBuffer);
          ourQuotes = ourQuotes->mNext;
        }
      }
    }
  }

  COMPUTE_END_INHERITED(Quotes, quotes)
}

const void* 
nsRuleNode::ComputeXULData(void* aStartStruct,
                           const nsRuleDataStruct& aData, 
                           nsStyleContext* aContext, 
                           nsRuleNode* aHighestNode,
                           const RuleDetail aRuleDetail,
                           const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(XUL, (), xul, parentXUL, XUL, xulData)

  
  SetDiscrete(xulData.mBoxAlign, xul->mBoxAlign, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxAlign,
              NS_STYLE_BOX_ALIGN_STRETCH, 0, 0, 0, 0);

  
  SetDiscrete(xulData.mBoxDirection, xul->mBoxDirection, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxDirection,
              NS_STYLE_BOX_DIRECTION_NORMAL, 0, 0, 0, 0);

  
  SetFactor(xulData.mBoxFlex, xul->mBoxFlex, canStoreInRuleTree,
            parentXUL->mBoxFlex, 0.0f);

  
  SetDiscrete(xulData.mBoxOrient, xul->mBoxOrient, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxOrient,
              NS_STYLE_BOX_ORIENT_HORIZONTAL, 0, 0, 0, 0);

  
  SetDiscrete(xulData.mBoxPack, xul->mBoxPack, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentXUL->mBoxPack,
              NS_STYLE_BOX_PACK_START, 0, 0, 0, 0);

  
  SetDiscrete(xulData.mBoxOrdinal, xul->mBoxOrdinal, canStoreInRuleTree,
              SETDSC_INTEGER, parentXUL->mBoxOrdinal, 1,
              0, 0, 0, 0);

  if (eCSSUnit_Inherit == xulData.mStackSizing.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    xul->mStretchStack = parentXUL->mStretchStack;
  } else if (eCSSUnit_Initial == xulData.mStackSizing.GetUnit()) {
    xul->mStretchStack = PR_TRUE;
  } else if (eCSSUnit_Enumerated == xulData.mStackSizing.GetUnit()) {
    xul->mStretchStack = xulData.mStackSizing.GetIntValue() ==
      NS_STYLE_STACK_SIZING_STRETCH_TO_FIT;
  }

  COMPUTE_END_RESET(XUL, xul)
}

const void* 
nsRuleNode::ComputeColumnData(void* aStartStruct,
                              const nsRuleDataStruct& aData, 
                              nsStyleContext* aContext, 
                              nsRuleNode* aHighestNode,
                              const RuleDetail aRuleDetail,
                              const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(Column, (mPresContext), column, parent, Column, columnData)

  
  SetCoord(columnData.mColumnWidth,
           column->mColumnWidth, parent->mColumnWidth,
           SETCOORD_LAH | SETCOORD_INITIAL_AUTO,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetCoord(columnData.mColumnGap,
           column->mColumnGap, parent->mColumnGap,
           SETCOORD_LPH | SETCOORD_NORMAL | SETCOORD_INITIAL_NORMAL,
           aContext, mPresContext, canStoreInRuleTree);

  
  if (eCSSUnit_Auto == columnData.mColumnCount.GetUnit() ||
      eCSSUnit_Initial == columnData.mColumnCount.GetUnit()) {
    column->mColumnCount = NS_STYLE_COLUMN_COUNT_AUTO;
  } else if (eCSSUnit_Integer == columnData.mColumnCount.GetUnit()) {
    column->mColumnCount = columnData.mColumnCount.GetIntValue();
    
    column->mColumnCount = PR_MIN(column->mColumnCount, 1000);
  } else if (eCSSUnit_Inherit == columnData.mColumnCount.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    column->mColumnCount = parent->mColumnCount;
  }

  
  const nsCSSValue& widthValue = columnData.mColumnRuleWidth;
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
  else if (widthValue.IsLengthUnit()) {
    column->SetColumnRuleWidth(CalcLength(widthValue, aContext,
                                          mPresContext, canStoreInRuleTree));
  }

  
  const nsCSSValue& styleValue = columnData.mColumnRuleStyle;
  if (eCSSUnit_Enumerated == styleValue.GetUnit()) {
    column->mColumnRuleStyle = styleValue.GetIntValue();
  }
  else if (eCSSUnit_None == styleValue.GetUnit() ||
           eCSSUnit_Initial == styleValue.GetUnit()) {
    column->mColumnRuleStyle = NS_STYLE_BORDER_STYLE_NONE;
  }
  else if (eCSSUnit_Inherit == styleValue.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    column->mColumnRuleStyle = parent->mColumnRuleStyle;
  }

  
  const nsCSSValue& colorValue = columnData.mColumnRuleColor;
  if (eCSSUnit_Inherit == colorValue.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    column->mColumnRuleColorIsForeground = PR_FALSE;
    if (parent->mColumnRuleColorIsForeground) {
      column->mColumnRuleColor = parentContext->GetStyleColor()->mColor;
    } else {
      column->mColumnRuleColor = parent->mColumnRuleColor;
    }
  }
  else if (eCSSUnit_Initial == colorValue.GetUnit()) {
    column->mColumnRuleColorIsForeground = PR_TRUE;
  }
  else if (SetColor(colorValue, 0, mPresContext, aContext,
                    column->mColumnRuleColor, canStoreInRuleTree)) {
    column->mColumnRuleColorIsForeground = PR_FALSE;
  }

  COMPUTE_END_RESET(Column, column)
}

#ifdef MOZ_SVG
static void
SetSVGPaint(const nsCSSValuePair& aValue, const nsStyleSVGPaint& parentPaint,
            nsPresContext* aPresContext, nsStyleContext *aContext, 
            nsStyleSVGPaint& aResult, nsStyleSVGPaintType aInitialPaintType,
            PRBool& aCanStoreInRuleTree)
{
  nscolor color;

  if (aValue.mXValue.GetUnit() == eCSSUnit_Inherit) {
    aResult = parentPaint;
    aCanStoreInRuleTree = PR_FALSE;
  } else if (aValue.mXValue.GetUnit() == eCSSUnit_None) {
    aResult.SetType(eStyleSVGPaintType_None);
  } else if (aValue.mXValue.GetUnit() == eCSSUnit_Initial) {
    aResult.SetType(aInitialPaintType);
    aResult.mPaint.mColor = NS_RGB(0, 0, 0);
    aResult.mFallbackColor = NS_RGB(0, 0, 0);
  } else if (aValue.mXValue.GetUnit() == eCSSUnit_URL) {
    aResult.SetType(eStyleSVGPaintType_Server);
    aResult.mPaint.mPaintServer = aValue.mXValue.GetURLValue();
    NS_IF_ADDREF(aResult.mPaint.mPaintServer);
    if (aValue.mYValue.GetUnit() == eCSSUnit_None) {
      aResult.mFallbackColor = NS_RGBA(0, 0, 0, 0);
    } else {
      NS_ASSERTION(aValue.mYValue.GetUnit() != eCSSUnit_Inherit, "cannot inherit fallback colour");
      SetColor(aValue.mYValue, NS_RGB(0, 0, 0), aPresContext, aContext,
               aResult.mFallbackColor, aCanStoreInRuleTree);
    }
  } else if (SetColor(aValue.mXValue, parentPaint.mPaint.mColor, aPresContext,
                      aContext, color, aCanStoreInRuleTree)) {
    aResult.SetType(eStyleSVGPaintType_Color);
    aResult.mPaint.mColor = color;
  }
}

const void* 
nsRuleNode::ComputeSVGData(void* aStartStruct,
                           const nsRuleDataStruct& aData, 
                           nsStyleContext* aContext, 
                           nsRuleNode* aHighestNode,
                           const RuleDetail aRuleDetail,
                           const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_INHERITED(SVG, (), svg, parentSVG, SVG, SVGData)

  
  SetDiscrete(SVGData.mClipRule, svg->mClipRule, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mClipRule,
              NS_STYLE_FILL_RULE_NONZERO, 0, 0, 0, 0);

  
  SetDiscrete(SVGData.mColorInterpolation, svg->mColorInterpolation,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentSVG->mColorInterpolation,
              NS_STYLE_COLOR_INTERPOLATION_SRGB,
              NS_STYLE_COLOR_INTERPOLATION_AUTO, 0, 0, 0);

  
  SetDiscrete(SVGData.mColorInterpolationFilters,
              svg->mColorInterpolationFilters, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentSVG->mColorInterpolationFilters,
              NS_STYLE_COLOR_INTERPOLATION_LINEARRGB,
              NS_STYLE_COLOR_INTERPOLATION_AUTO, 0, 0, 0);

  
  SetSVGPaint(SVGData.mFill, parentSVG->mFill, mPresContext, aContext,
              svg->mFill, eStyleSVGPaintType_Color, canStoreInRuleTree);

  
  SetFactor(SVGData.mFillOpacity, svg->mFillOpacity, canStoreInRuleTree,
            parentSVG->mFillOpacity, 1.0f, SETFCT_OPACITY);

  
  SetDiscrete(SVGData.mFillRule, svg->mFillRule, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mFillRule,
              NS_STYLE_FILL_RULE_NONZERO, 0, 0, 0, 0);

  
  SetDiscrete(SVGData.mImageRendering, svg->mImageRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentSVG->mImageRendering,
              NS_STYLE_IMAGE_RENDERING_AUTO, 
              NS_STYLE_IMAGE_RENDERING_AUTO, 0, 0, 0);

  
  if (eCSSUnit_URL == SVGData.mMarkerEnd.GetUnit()) {
    svg->mMarkerEnd = SVGData.mMarkerEnd.GetURLValue();
  } else if (eCSSUnit_None == SVGData.mMarkerEnd.GetUnit() ||
             eCSSUnit_Initial == SVGData.mMarkerEnd.GetUnit()) {
    svg->mMarkerEnd = nsnull;
  } else if (eCSSUnit_Inherit == SVGData.mMarkerEnd.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svg->mMarkerEnd = parentSVG->mMarkerEnd;
  }

  
  if (eCSSUnit_URL == SVGData.mMarkerMid.GetUnit()) {
    svg->mMarkerMid = SVGData.mMarkerMid.GetURLValue();
  } else if (eCSSUnit_None == SVGData.mMarkerMid.GetUnit() ||
             eCSSUnit_Initial == SVGData.mMarkerMid.GetUnit()) {
    svg->mMarkerMid = nsnull;
  } else if (eCSSUnit_Inherit == SVGData.mMarkerMid.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svg->mMarkerMid = parentSVG->mMarkerMid;
  }

  
  if (eCSSUnit_URL == SVGData.mMarkerStart.GetUnit()) {
    svg->mMarkerStart = SVGData.mMarkerStart.GetURLValue();
  } else if (eCSSUnit_None == SVGData.mMarkerStart.GetUnit() ||
             eCSSUnit_Initial == SVGData.mMarkerStart.GetUnit()) {
    svg->mMarkerStart = nsnull;
  } else if (eCSSUnit_Inherit == SVGData.mMarkerStart.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svg->mMarkerStart = parentSVG->mMarkerStart;
  }

  
  SetDiscrete(SVGData.mPointerEvents, svg->mPointerEvents, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_NONE, parentSVG->mPointerEvents,
              NS_STYLE_POINTER_EVENTS_VISIBLEPAINTED, 0,
              NS_STYLE_POINTER_EVENTS_NONE, 0, 0);

  
  SetDiscrete(SVGData.mShapeRendering, svg->mShapeRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentSVG->mShapeRendering,
              NS_STYLE_SHAPE_RENDERING_AUTO, 
              NS_STYLE_SHAPE_RENDERING_AUTO, 0, 0, 0);

  
  SetSVGPaint(SVGData.mStroke, parentSVG->mStroke, mPresContext, aContext,
              svg->mStroke, eStyleSVGPaintType_None, canStoreInRuleTree);

  
  nsCSSValueList *list = SVGData.mStrokeDasharray;
  if (list) {
    if (eCSSUnit_Inherit == list->mValue.GetUnit()) {
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
    } else {
      delete [] svg->mStrokeDasharray;
      svg->mStrokeDasharray = nsnull;
      svg->mStrokeDasharrayLength = 0;
      
      if (eCSSUnit_Initial != list->mValue.GetUnit() &&
          eCSSUnit_None    != list->mValue.GetUnit()) {
        
        nsCSSValueList *value = SVGData.mStrokeDasharray;
        while (nsnull != value) {
          ++svg->mStrokeDasharrayLength;
          value = value->mNext;
        }
        
        NS_ASSERTION(svg->mStrokeDasharrayLength != 0, "no dasharray items");
        
        svg->mStrokeDasharray = new nsStyleCoord[svg->mStrokeDasharrayLength];

        if (svg->mStrokeDasharray) {
          value = SVGData.mStrokeDasharray;
          PRUint32 i = 0;
          while (nsnull != value) {
            SetCoord(value->mValue,
                     svg->mStrokeDasharray[i++], nsnull,
                     SETCOORD_LP | SETCOORD_FACTOR,
                     aContext, mPresContext, canStoreInRuleTree);
            value = value->mNext;
          }
        } else
          svg->mStrokeDasharrayLength = 0;
      }
    }
  }

  
  SetCoord(SVGData.mStrokeDashoffset,
           svg->mStrokeDashoffset, parentSVG->mStrokeDashoffset,
           SETCOORD_LPH | SETCOORD_FACTOR | SETCOORD_INITIAL_ZERO,
           aContext, mPresContext, canStoreInRuleTree);

  
  SetDiscrete(SVGData.mStrokeLinecap, svg->mStrokeLinecap, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mStrokeLinecap,
              NS_STYLE_STROKE_LINECAP_BUTT, 0, 0, 0, 0);

  
  SetDiscrete(SVGData.mStrokeLinejoin, svg->mStrokeLinejoin, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mStrokeLinejoin,
              NS_STYLE_STROKE_LINEJOIN_MITER, 0, 0, 0, 0);

  
  SetFactor(SVGData.mStrokeMiterlimit, svg->mStrokeMiterlimit,
            canStoreInRuleTree,
            parentSVG->mStrokeMiterlimit, 4.0f);

  
  SetFactor(SVGData.mStrokeOpacity, svg->mStrokeOpacity, canStoreInRuleTree,
            parentSVG->mStrokeOpacity, 1.0f, SETFCT_OPACITY);

  
  if (eCSSUnit_Initial == SVGData.mStrokeWidth.GetUnit()) {
    svg->mStrokeWidth.SetCoordValue(nsPresContext::CSSPixelsToAppUnits(1));
  } else {
    SetCoord(SVGData.mStrokeWidth,
             svg->mStrokeWidth, parentSVG->mStrokeWidth,
             SETCOORD_LPH | SETCOORD_FACTOR,
             aContext, mPresContext, canStoreInRuleTree);
  }

  
  SetDiscrete(SVGData.mTextAnchor, svg->mTextAnchor, canStoreInRuleTree,
              SETDSC_ENUMERATED, parentSVG->mTextAnchor,
              NS_STYLE_TEXT_ANCHOR_START, 0, 0, 0, 0);
  
  
  SetDiscrete(SVGData.mTextRendering, svg->mTextRendering, canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentSVG->mTextRendering,
              NS_STYLE_TEXT_RENDERING_AUTO,
              NS_STYLE_TEXT_RENDERING_AUTO, 0, 0, 0);

  COMPUTE_END_INHERITED(SVG, svg)
}

const void* 
nsRuleNode::ComputeSVGResetData(void* aStartStruct,
                                const nsRuleDataStruct& aData,
                                nsStyleContext* aContext, 
                                nsRuleNode* aHighestNode,
                                const RuleDetail aRuleDetail,
                                const PRBool aCanStoreInRuleTree)
{
  COMPUTE_START_RESET(SVGReset, (), svgReset, parentSVGReset, SVG, SVGData)

  
  if (eCSSUnit_Initial == SVGData.mStopColor.GetUnit()) {
    svgReset->mStopColor = NS_RGB(0, 0, 0);
  } else {
    SetColor(SVGData.mStopColor, parentSVGReset->mStopColor,
             mPresContext, aContext, svgReset->mStopColor, canStoreInRuleTree);
  }

  
  if (eCSSUnit_Initial == SVGData.mFloodColor.GetUnit()) {
    svgReset->mFloodColor = NS_RGB(0, 0, 0);
  } else {
    SetColor(SVGData.mFloodColor, parentSVGReset->mFloodColor,
             mPresContext, aContext, svgReset->mFloodColor, canStoreInRuleTree);
  }

  
  if (eCSSUnit_Initial == SVGData.mLightingColor.GetUnit()) {
    svgReset->mLightingColor = NS_RGB(255, 255, 255);
  } else {
    SetColor(SVGData.mLightingColor, parentSVGReset->mLightingColor,
             mPresContext, aContext, svgReset->mLightingColor,
             canStoreInRuleTree);
  }

  
  if (eCSSUnit_URL == SVGData.mClipPath.GetUnit()) {
    svgReset->mClipPath = SVGData.mClipPath.GetURLValue();
  } else if (eCSSUnit_None == SVGData.mClipPath.GetUnit() ||
             eCSSUnit_Initial == SVGData.mClipPath.GetUnit()) {
    svgReset->mClipPath = nsnull;
  } else if (eCSSUnit_Inherit == SVGData.mClipPath.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svgReset->mClipPath = parentSVGReset->mClipPath;
  }

  
  SetFactor(SVGData.mStopOpacity, svgReset->mStopOpacity, canStoreInRuleTree,
            parentSVGReset->mStopOpacity, 1.0f, SETFCT_OPACITY);

  
  SetFactor(SVGData.mFloodOpacity, svgReset->mFloodOpacity, canStoreInRuleTree,
            parentSVGReset->mFloodOpacity, 1.0f, SETFCT_OPACITY);

  
  SetDiscrete(SVGData.mDominantBaseline, svgReset->mDominantBaseline,
              canStoreInRuleTree,
              SETDSC_ENUMERATED | SETDSC_AUTO,
              parentSVGReset->mDominantBaseline,
              NS_STYLE_DOMINANT_BASELINE_AUTO,
              NS_STYLE_DOMINANT_BASELINE_AUTO, 0, 0, 0);

  
  if (eCSSUnit_URL == SVGData.mFilter.GetUnit()) {
    svgReset->mFilter = SVGData.mFilter.GetURLValue();
  } else if (eCSSUnit_None == SVGData.mFilter.GetUnit() ||
             eCSSUnit_Initial == SVGData.mFilter.GetUnit()) {
    svgReset->mFilter = nsnull;
  } else if (eCSSUnit_Inherit == SVGData.mFilter.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svgReset->mFilter = parentSVGReset->mFilter;
  }

  
  if (eCSSUnit_URL == SVGData.mMask.GetUnit()) {
    svgReset->mMask = SVGData.mMask.GetURLValue();
  } else if (eCSSUnit_None == SVGData.mMask.GetUnit() ||
             eCSSUnit_Initial == SVGData.mMask.GetUnit()) {
    svgReset->mMask = nsnull;
  } else if (eCSSUnit_Inherit == SVGData.mMask.GetUnit()) {
    canStoreInRuleTree = PR_FALSE;
    svgReset->mMask = parentSVGReset->mMask;
  }
  
  COMPUTE_END_RESET(SVGReset, svgReset)
}
#endif

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

  
#define STYLE_STRUCT_TEST aSID
#define STYLE_STRUCT(name, checkdata_cb, ctor_args) \
  data = Get##name##Data(aContext);
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
#undef STYLE_STRUCT_TEST

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
  data =                                                                      \
    static_cast<const nsStyle##name_ *>(Get##name_##Data(aContext));          \
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
    if (ChildrenAreHashed()) {
      PLDHashTable *children = ChildrenHash();
      PL_DHashTableEnumerate(children, SweepRuleNodeChildren, nsnull);
    } else {
      for (nsRuleNode **children = ChildrenListPtr(); *children; ) {
        nsRuleNode *next = (*children)->mNextSibling;
        if ((*children)->Sweep()) {
          
          
          *children = next;
        } else {
          
          children = &(*children)->mNextSibling;
        }
      }
    }
  }
  return PR_FALSE;
}

 PRBool
nsRuleNode::HasAuthorSpecifiedRules(nsStyleContext* aStyleContext,
                                    PRUint32 ruleTypeMask,
                                    PRBool aAuthorColorsAllowed)
{
  nsRuleDataColor colorData;
  nsRuleDataMargin marginData;
  nsCSSValue firstBackgroundImage;
  PRUint32 nValues = 0;

  PRUint32 inheritBits = 0;
  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND)
    inheritBits |= NS_STYLE_INHERIT_BIT(Background);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER)
    inheritBits |= NS_STYLE_INHERIT_BIT(Border);

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING)
    inheritBits |= NS_STYLE_INHERIT_BIT(Padding);

  
  nsRuleData ruleData(inheritBits,
                      aStyleContext->PresContext(), aStyleContext);
  ruleData.mColorData = &colorData;
  ruleData.mMarginData = &marginData;

  nsCSSValue* backgroundValues[] = {
    &colorData.mBackColor,
    &firstBackgroundImage
  };

  nsCSSValue* borderValues[] = {
    &marginData.mBorderColor.mTop,
    &marginData.mBorderStyle.mTop,
    &marginData.mBorderWidth.mTop,
    &marginData.mBorderColor.mRight,
    &marginData.mBorderStyle.mRight,
    &marginData.mBorderWidth.mRight,
    &marginData.mBorderColor.mBottom,
    &marginData.mBorderStyle.mBottom,
    &marginData.mBorderWidth.mBottom,
    &marginData.mBorderColor.mLeft,
    &marginData.mBorderStyle.mLeft,
    &marginData.mBorderWidth.mLeft
    
  };

  nsCSSValue* paddingValues[] = {
    &marginData.mPadding.mTop,
    &marginData.mPadding.mRight,
    &marginData.mPadding.mBottom,
    &marginData.mPadding.mLeft,
    &marginData.mPaddingStart,
    &marginData.mPaddingEnd
  };

  nsCSSValue* values[NS_ARRAY_LENGTH(backgroundValues) +
                     NS_ARRAY_LENGTH(borderValues) +
                     NS_ARRAY_LENGTH(paddingValues)];

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND) {
    memcpy(&values[nValues], backgroundValues, NS_ARRAY_LENGTH(backgroundValues) * sizeof(nsCSSValue*));
    nValues += NS_ARRAY_LENGTH(backgroundValues);
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_BORDER) {
    memcpy(&values[nValues], borderValues, NS_ARRAY_LENGTH(borderValues) * sizeof(nsCSSValue*));
    nValues += NS_ARRAY_LENGTH(borderValues);
  }

  if (ruleTypeMask & NS_AUTHOR_SPECIFIED_PADDING) {
    memcpy(&values[nValues], paddingValues, NS_ARRAY_LENGTH(paddingValues) * sizeof(nsCSSValue*));
    nValues += NS_ARRAY_LENGTH(paddingValues);
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

        if ((ruleTypeMask & NS_AUTHOR_SPECIFIED_BACKGROUND) &&
            colorData.mBackImage &&
            firstBackgroundImage.GetUnit() == eCSSUnit_Null) {
          
          firstBackgroundImage = colorData.mBackImage->mValue;
        }
        
        
        
        marginData.mBoxShadow = nsnull;
        colorData.mBackImage = nsnull;
        colorData.mBackRepeat = nsnull;
        colorData.mBackAttachment = nsnull;
        colorData.mBackPosition = nsnull;
        colorData.mBackClip = nsnull;
        colorData.mBackOrigin = nsnull;
        colorData.mBackSize = nsnull;

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
                  (values[i] == &colorData.mBackColor &&
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
