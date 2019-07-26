



 


#include "mozilla/DebugOnly.h"

#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsPresContext.h"
#include "nsIStyleRule.h"

#include "nsCOMPtr.h"
#include "nsStyleSet.h"
#include "nsIPresShell.h"

#include "nsRuleNode.h"
#include "nsStyleContext.h"
#include "prlog.h"
#include "nsStyleAnimation.h"
#include "sampler.h"

#ifdef DEBUG

#endif

using namespace mozilla;




nsStyleContext::nsStyleContext(nsStyleContext* aParent,
                               nsIAtom* aPseudoTag,
                               nsCSSPseudoElements::Type aPseudoType,
                               nsRuleNode* aRuleNode)
  : mParent(aParent),
    mChild(nullptr),
    mEmptyChild(nullptr),
    mPseudoTag(aPseudoTag),
    mRuleNode(aRuleNode),
    mAllocations(nullptr),
    mCachedResetData(nullptr),
    mBits(((uint32_t)aPseudoType) << NS_STYLE_CONTEXT_TYPE_SHIFT),
    mRefCnt(0)
{
  
  
  MOZ_STATIC_ASSERT((UINT32_MAX >> NS_STYLE_CONTEXT_TYPE_SHIFT) >=
                    nsCSSPseudoElements::ePseudo_MAX,
                    "pseudo element bits no longer fit in a uint32_t");
  MOZ_ASSERT(aRuleNode);

  mNextSibling = this;
  mPrevSibling = this;
  if (mParent) {
    mParent->AddRef();
    mParent->AddChild(this);
#ifdef DEBUG
    nsRuleNode *r1 = mParent->RuleNode(), *r2 = aRuleNode;
    while (r1->GetParent())
      r1 = r1->GetParent();
    while (r2->GetParent())
      r2 = r2->GetParent();
    NS_ASSERTION(r1 == r2, "must be in the same rule tree as parent");
#endif
  }

  mRuleNode->AddRef();
  mRuleNode->SetUsedDirectly(); 

  ApplyStyleFixups();

  #define eStyleStruct_LastItem (nsStyleStructID_Length - 1)
  NS_ASSERTION(NS_STYLE_INHERIT_MASK & NS_STYLE_INHERIT_BIT(LastItem),
               "NS_STYLE_INHERIT_MASK must be bigger, and other bits shifted");
  #undef eStyleStruct_LastItem
}

nsStyleContext::~nsStyleContext()
{
  NS_ASSERTION((nullptr == mChild) && (nullptr == mEmptyChild), "destructing context with children");

  nsPresContext *presContext = mRuleNode->PresContext();

  mRuleNode->Release();

  presContext->PresShell()->StyleSet()->
    NotifyStyleContextDestroyed(presContext, this);

  if (mParent) {
    mParent->RemoveChild(this);
    mParent->Release();
  }

  
  mCachedInheritedData.DestroyStructs(mBits, presContext);
  if (mCachedResetData) {
    mCachedResetData->Destroy(mBits, presContext);
  }

  FreeAllocations(presContext);
}

void nsStyleContext::AddChild(nsStyleContext* aChild)
{
  NS_ASSERTION(aChild->mPrevSibling == aChild &&
               aChild->mNextSibling == aChild,
               "child already in a child list");

  nsStyleContext **list = aChild->mRuleNode->IsRoot() ? &mEmptyChild : &mChild;

  
  if (*list) {
    
    aChild->mNextSibling = (*list);
    aChild->mPrevSibling = (*list)->mPrevSibling;
    (*list)->mPrevSibling->mNextSibling = aChild;
    (*list)->mPrevSibling = aChild;
  }
  (*list) = aChild;
}

void nsStyleContext::RemoveChild(nsStyleContext* aChild)
{
  NS_PRECONDITION(nullptr != aChild && this == aChild->mParent, "bad argument");

  nsStyleContext **list = aChild->mRuleNode->IsRoot() ? &mEmptyChild : &mChild;

  if (aChild->mPrevSibling != aChild) { 
    if ((*list) == aChild) {
      (*list) = (*list)->mNextSibling;
    }
  } 
  else {
    NS_ASSERTION((*list) == aChild, "bad sibling pointers");
    (*list) = nullptr;
  }

  aChild->mPrevSibling->mNextSibling = aChild->mNextSibling;
  aChild->mNextSibling->mPrevSibling = aChild->mPrevSibling;
  aChild->mNextSibling = aChild;
  aChild->mPrevSibling = aChild;
}

already_AddRefed<nsStyleContext>
nsStyleContext::FindChildWithRules(const nsIAtom* aPseudoTag, 
                                   nsRuleNode* aRuleNode,
                                   nsRuleNode* aRulesIfVisited,
                                   bool aRelevantLinkVisited)
{
  NS_ABORT_IF_FALSE(aRulesIfVisited || !aRelevantLinkVisited,
    "aRelevantLinkVisited should only be set when we have a separate style");
  uint32_t threshold = 10; 
                           

  nsStyleContext* result = nullptr;
  nsStyleContext *list = aRuleNode->IsRoot() ? mEmptyChild : mChild;

  if (list) {
    nsStyleContext *child = list;
    do {
      if (child->mRuleNode == aRuleNode &&
          child->mPseudoTag == aPseudoTag &&
          !child->IsStyleIfVisited() &&
          child->RelevantLinkVisited() == aRelevantLinkVisited) {
        bool match = false;
        if (aRulesIfVisited) {
          match = child->GetStyleIfVisited() &&
                  child->GetStyleIfVisited()->mRuleNode == aRulesIfVisited;
        } else {
          match = !child->GetStyleIfVisited();
        }
        if (match) {
          result = child;
          break;
        }
      }
      child = child->mNextSibling;
      threshold--;
      if (threshold == 0)
        break;
    } while (child != list);
  }

  if (result) {
    if (result != list) {
      
      RemoveChild(result);
      AddChild(result);
    }

    
    result->AddRef();
  }

  return result;
}

const void* nsStyleContext::GetCachedStyleData(nsStyleStructID aSID)
{
  const void* cachedData;
  if (nsCachedStyleData::IsReset(aSID)) {
    if (mCachedResetData) {
      cachedData = mCachedResetData->mStyleStructs[aSID];
    } else {
      cachedData = nullptr;
    }
  } else {
    cachedData = mCachedInheritedData.mStyleStructs[aSID];
  }
  return cachedData;
}

const void* nsStyleContext::GetStyleData(nsStyleStructID aSID)
{
  const void* cachedData = GetCachedStyleData(aSID);
  if (cachedData)
    return cachedData; 
  return mRuleNode->GetStyleData(aSID, this, true); 
}




void* 
nsStyleContext::GetUniqueStyleData(const nsStyleStructID& aSID)
{
  
  
  
  
  
  const void *current = GetStyleData(aSID);
  if (!mChild && !mEmptyChild &&
      !(mBits & nsCachedStyleData::GetBitForSID(aSID)) &&
      GetCachedStyleData(aSID))
    return const_cast<void*>(current);

  void* result;
  nsPresContext *presContext = PresContext();
  switch (aSID) {

#define UNIQUE_CASE(c_)                                                       \
  case eStyleStruct_##c_:                                                     \
    result = new (presContext) nsStyle##c_(                                   \
      * static_cast<const nsStyle##c_ *>(current));                           \
    break;

  UNIQUE_CASE(Display)
  UNIQUE_CASE(Background)
  UNIQUE_CASE(Text)
  UNIQUE_CASE(TextReset)

#undef UNIQUE_CASE

  default:
    NS_ERROR("Struct type not supported.  Please find another way to do this if you can!");
    return nullptr;
  }

  SetStyle(aSID, result);
  mBits &= ~nsCachedStyleData::GetBitForSID(aSID);

  return result;
}

void
nsStyleContext::SetStyle(nsStyleStructID aSID, void* aStruct)
{
  
  
  
  NS_ASSERTION(aSID >= 0 && aSID < nsStyleStructID_Length, "out of bounds");

  
  
  

  void** dataSlot;
  if (nsCachedStyleData::IsReset(aSID)) {
    if (!mCachedResetData) {
      mCachedResetData = new (mRuleNode->PresContext()) nsResetStyleData;
    }
    dataSlot = &mCachedResetData->mStyleStructs[aSID];
  } else {
    dataSlot = &mCachedInheritedData.mStyleStructs[aSID];
  }
  NS_ASSERTION(!*dataSlot || (mBits & nsCachedStyleData::GetBitForSID(aSID)),
               "Going to leak style data");
  *dataSlot = aStruct;
}

void
nsStyleContext::ApplyStyleFixups()
{
  
  
  if (mParent && mParent->HasTextDecorationLines()) {
    mBits |= NS_STYLE_HAS_TEXT_DECORATION_LINES;
  } else {
    
    const nsStyleTextReset* text = GetStyleTextReset();
    uint8_t decorationLine = text->mTextDecorationLine;
    if (decorationLine != NS_STYLE_TEXT_DECORATION_LINE_NONE &&
        decorationLine != NS_STYLE_TEXT_DECORATION_LINE_OVERRIDE_ALL) {
      mBits |= NS_STYLE_HAS_TEXT_DECORATION_LINES;
    }
  }

  if ((mParent && mParent->HasPseudoElementData()) || mPseudoTag) {
    mBits |= NS_STYLE_HAS_PSEUDO_ELEMENT_DATA;
  }

  
  const nsStyleDisplay* disp = GetStyleDisplay();
  if (disp->mDisplay == NS_STYLE_DISPLAY_TABLE) {
    
    
    
    const nsStyleText* text = GetStyleText();
    
    if (text->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_CENTER ||
        text->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_RIGHT)
    {
      nsStyleText* uniqueText = (nsStyleText*)GetUniqueStyleData(eStyleStruct_Text);
      uniqueText->mTextAlign = NS_STYLE_TEXT_ALIGN_DEFAULT;
    }
  }

  
  
  
  
  
  
  
  if (!mParent) {
    if (disp->mDisplay != NS_STYLE_DISPLAY_NONE &&
        disp->mDisplay != NS_STYLE_DISPLAY_BLOCK &&
        disp->mDisplay != NS_STYLE_DISPLAY_TABLE) {
      nsStyleDisplay *mutable_display = static_cast<nsStyleDisplay*>
                                                   (GetUniqueStyleData(eStyleStruct_Display));
      
      
      
      
      
      if (mutable_display->mDisplay == NS_STYLE_DISPLAY_INLINE_TABLE)
        mutable_display->mOriginalDisplay = mutable_display->mDisplay =
          NS_STYLE_DISPLAY_TABLE;
      else
        mutable_display->mOriginalDisplay = mutable_display->mDisplay =
          NS_STYLE_DISPLAY_BLOCK;
    }
  }

  
  
  
  
  
#ifdef MOZ_FLEXBOX
  if (mParent) {
    const nsStyleDisplay* parentDisp = mParent->GetStyleDisplay();
    if ((parentDisp->mDisplay == NS_STYLE_DISPLAY_FLEX ||
         parentDisp->mDisplay == NS_STYLE_DISPLAY_INLINE_FLEX) &&
        GetPseudo() != nsCSSAnonBoxes::mozNonElement) {
      uint8_t displayVal = disp->mDisplay;
      
      
      
      
      
      if (NS_STYLE_DISPLAY_TABLE_CAPTION      != displayVal &&
          NS_STYLE_DISPLAY_TABLE_ROW_GROUP    != displayVal &&
          NS_STYLE_DISPLAY_TABLE_HEADER_GROUP != displayVal &&
          NS_STYLE_DISPLAY_TABLE_FOOTER_GROUP != displayVal &&
          NS_STYLE_DISPLAY_TABLE_COLUMN_GROUP != displayVal &&
          NS_STYLE_DISPLAY_TABLE_COLUMN       != displayVal &&
          NS_STYLE_DISPLAY_TABLE_ROW          != displayVal &&
          NS_STYLE_DISPLAY_TABLE_CELL         != displayVal) {

        
        
        
        
        
        nsRuleNode::EnsureBlockDisplay(displayVal);
        if (displayVal != disp->mDisplay) {
          NS_ASSERTION(!disp->IsAbsolutelyPositionedStyle(),
                       "We shouldn't be changing the display value of "
                       "positioned content (and we should have already "
                       "converted its display value to be block-level...)");
          nsStyleDisplay *mutable_display =
            static_cast<nsStyleDisplay*>(GetUniqueStyleData(eStyleStruct_Display));
          mutable_display->mDisplay = displayVal;
        }
      }
    }
  }
#endif 

  
  GetStyleUserInterface();
}

nsChangeHint
nsStyleContext::CalcStyleDifference(nsStyleContext* aOther,
                                    nsChangeHint aParentHintsNotHandledForDescendants)
{
  SAMPLE_LABEL("nsStyleContext", "CalcStyleDifference");

  NS_ABORT_IF_FALSE(NS_IsHintSubset(aParentHintsNotHandledForDescendants,
                                    nsChangeHint_Hints_NotHandledForDescendants),
                    "caller is passing inherited hints, but shouldn't be");

  nsChangeHint hint = NS_STYLE_HINT_NONE;
  NS_ENSURE_TRUE(aOther, hint);
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool compare = mRuleNode != aOther->mRuleNode;

#define DO_STRUCT_DIFFERENCE(struct_)                                         \
  PR_BEGIN_MACRO                                                              \
    const nsStyle##struct_* this##struct_ = PeekStyle##struct_();             \
    if (this##struct_) {                                                      \
      const nsStyle##struct_* other##struct_ = aOther->GetStyle##struct_();   \
      nsChangeHint maxDifference = nsStyle##struct_::MaxDifference();         \
      if ((compare ||                                                         \
           (maxDifference & aParentHintsNotHandledForDescendants)) &&         \
          !NS_IsHintSubset(maxDifference, hint) &&                            \
          this##struct_ != other##struct_) {                                  \
        NS_ASSERTION(NS_IsHintSubset(                                         \
             this##struct_->CalcDifference(*other##struct_),                  \
             nsStyle##struct_::MaxDifference()),                              \
             "CalcDifference() returned bigger hint than MaxDifference()");   \
        NS_UpdateHint(hint, this##struct_->CalcDifference(*other##struct_));  \
      }                                                                       \
    }                                                                         \
  PR_END_MACRO

  
  
  
  
  
  DO_STRUCT_DIFFERENCE(Display);
  DO_STRUCT_DIFFERENCE(XUL);
  DO_STRUCT_DIFFERENCE(Column);
  DO_STRUCT_DIFFERENCE(Content);
  DO_STRUCT_DIFFERENCE(UserInterface);
  DO_STRUCT_DIFFERENCE(Visibility);
  DO_STRUCT_DIFFERENCE(Outline);
  DO_STRUCT_DIFFERENCE(TableBorder);
  DO_STRUCT_DIFFERENCE(Table);
  DO_STRUCT_DIFFERENCE(UIReset);
  DO_STRUCT_DIFFERENCE(Text);
  DO_STRUCT_DIFFERENCE(List);
  DO_STRUCT_DIFFERENCE(Quotes);
  DO_STRUCT_DIFFERENCE(SVGReset);
  DO_STRUCT_DIFFERENCE(SVG);
  DO_STRUCT_DIFFERENCE(Position);
  DO_STRUCT_DIFFERENCE(Font);
  DO_STRUCT_DIFFERENCE(Margin);
  DO_STRUCT_DIFFERENCE(Padding);
  DO_STRUCT_DIFFERENCE(Border);
  DO_STRUCT_DIFFERENCE(TextReset);
  DO_STRUCT_DIFFERENCE(Background);
  DO_STRUCT_DIFFERENCE(Color);

#undef DO_STRUCT_DIFFERENCE

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsStyleContext *thisVis = GetStyleIfVisited(),
                *otherVis = aOther->GetStyleIfVisited();
  if (!thisVis != !otherVis) {
    
    
    NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
  } else if (thisVis && !NS_IsHintSubset(nsChangeHint_RepaintFrame, hint)) {
    
    bool change = false;

    
    
    
    
    
    if (PeekStyleColor()) {
      if (thisVis->GetStyleColor()->mColor !=
          otherVis->GetStyleColor()->mColor) {
        change = true;
      }
    }

    
    if (!change && PeekStyleBackground()) {
      if (thisVis->GetStyleBackground()->mBackgroundColor !=
          otherVis->GetStyleBackground()->mBackgroundColor) {
        change = true;
      }
    }

    
    if (!change && PeekStyleBorder()) {
      const nsStyleBorder *thisVisBorder = thisVis->GetStyleBorder();
      const nsStyleBorder *otherVisBorder = otherVis->GetStyleBorder();
      NS_FOR_CSS_SIDES(side) {
        bool thisFG, otherFG;
        nscolor thisColor, otherColor;
        thisVisBorder->GetBorderColor(side, thisColor, thisFG);
        otherVisBorder->GetBorderColor(side, otherColor, otherFG);
        if (thisFG != otherFG || (!thisFG && thisColor != otherColor)) {
          change = true;
          break;
        }
      }
    }

    
    if (!change && PeekStyleOutline()) {
      const nsStyleOutline *thisVisOutline = thisVis->GetStyleOutline();
      const nsStyleOutline *otherVisOutline = otherVis->GetStyleOutline();
      bool haveColor;
      nscolor thisColor, otherColor;
      if (thisVisOutline->GetOutlineInitialColor() != 
            otherVisOutline->GetOutlineInitialColor() ||
          (haveColor = thisVisOutline->GetOutlineColor(thisColor)) != 
            otherVisOutline->GetOutlineColor(otherColor) ||
          (haveColor && thisColor != otherColor)) {
        change = true;
      }
    }

    
    if (!change && PeekStyleColumn()) {
      const nsStyleColumn *thisVisColumn = thisVis->GetStyleColumn();
      const nsStyleColumn *otherVisColumn = otherVis->GetStyleColumn();
      if (thisVisColumn->mColumnRuleColor != otherVisColumn->mColumnRuleColor ||
          thisVisColumn->mColumnRuleColorIsForeground !=
            otherVisColumn->mColumnRuleColorIsForeground) {
        change = true;
      }
    }

    
    if (!change && PeekStyleTextReset()) {
      const nsStyleTextReset *thisVisTextReset = thisVis->GetStyleTextReset();
      const nsStyleTextReset *otherVisTextReset = otherVis->GetStyleTextReset();
      nscolor thisVisDecColor, otherVisDecColor;
      bool thisVisDecColorIsFG, otherVisDecColorIsFG;
      thisVisTextReset->GetDecorationColor(thisVisDecColor,
                                           thisVisDecColorIsFG);
      otherVisTextReset->GetDecorationColor(otherVisDecColor,
                                            otherVisDecColorIsFG);
      if (thisVisDecColorIsFG != otherVisDecColorIsFG ||
          (!thisVisDecColorIsFG && thisVisDecColor != otherVisDecColor)) {
        change = true;
      }
    }

    
    if (!change && PeekStyleSVG()) {
      const nsStyleSVG *thisVisSVG = thisVis->GetStyleSVG();
      const nsStyleSVG *otherVisSVG = otherVis->GetStyleSVG();
      if (thisVisSVG->mFill != otherVisSVG->mFill ||
          thisVisSVG->mStroke != otherVisSVG->mStroke) {
        change = true;
      }
    }

    if (change) {
      NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
    }
  }

  return hint;
}

void
nsStyleContext::Mark()
{
  
  mRuleNode->Mark();

  
  if (mChild) {
    nsStyleContext* child = mChild;
    do {
      child->Mark();
      child = child->mNextSibling;
    } while (mChild != child);
  }
  
  if (mEmptyChild) {
    nsStyleContext* child = mEmptyChild;
    do {
      child->Mark();
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
}

#ifdef DEBUG
void nsStyleContext::List(FILE* out, int32_t aIndent)
{
  
  int32_t ix;
  for (ix = aIndent; --ix >= 0; ) fputs("  ", out);
  fprintf(out, "%p(%d) parent=%p ",
          (void*)this, mRefCnt, (void *)mParent);
  if (mPseudoTag) {
    nsAutoString  buffer;
    mPseudoTag->ToString(buffer);
    fputs(NS_LossyConvertUTF16toASCII(buffer).get(), out);
    fputs(" ", out);
  }

  if (mRuleNode) {
    fputs("{\n", out);
    nsRuleNode* ruleNode = mRuleNode;
    while (ruleNode) {
      nsIStyleRule *styleRule = ruleNode->GetRule();
      if (styleRule) {
        styleRule->List(out, aIndent + 1);
      }
      ruleNode = ruleNode->GetParent();
    }
    for (ix = aIndent; --ix >= 0; ) fputs("  ", out);
    fputs("}\n", out);
  }
  else {
    fputs("{}\n", out);
  }

  if (nullptr != mChild) {
    nsStyleContext* child = mChild;
    do {
      child->List(out, aIndent + 1);
      child = child->mNextSibling;
    } while (mChild != child);
  }
  if (nullptr != mEmptyChild) {
    nsStyleContext* child = mEmptyChild;
    do {
      child->List(out, aIndent + 1);
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
}
#endif



void* 
nsStyleContext::operator new(size_t sz, nsPresContext* aPresContext) CPP_THROW_NEW
{
  
  return aPresContext->PresShell()->AllocateByObjectID(nsPresArena::nsStyleContext_id, sz);
}



void 
nsStyleContext::Destroy()
{
  
  nsRefPtr<nsPresContext> presContext = mRuleNode->PresContext();

  
  this->~nsStyleContext();

  
  
  presContext->PresShell()->FreeByObjectID(nsPresArena::nsStyleContext_id, this);
}

already_AddRefed<nsStyleContext>
NS_NewStyleContext(nsStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsCSSPseudoElements::Type aPseudoType,
                   nsRuleNode* aRuleNode)
{
  nsStyleContext* context =
    new (aRuleNode->PresContext())
      nsStyleContext(aParentContext, aPseudoTag, aPseudoType, aRuleNode);
  context->AddRef();
  return context;
}

static inline void
ExtractAnimationValue(nsCSSProperty aProperty,
                      nsStyleContext* aStyleContext,
                      nsStyleAnimation::Value& aResult)
{
  DebugOnly<bool> success =
    nsStyleAnimation::ExtractComputedValue(aProperty, aStyleContext, aResult);
  NS_ABORT_IF_FALSE(success,
                    "aProperty must be extractable by nsStyleAnimation");
}

static nscolor
ExtractColor(nsCSSProperty aProperty,
             nsStyleContext *aStyleContext)
{
  nsStyleAnimation::Value val;
  ExtractAnimationValue(aProperty, aStyleContext, val);
  return val.GetColorValue();
}

static nscolor
ExtractColorLenient(nsCSSProperty aProperty,
                    nsStyleContext *aStyleContext)
{
  nsStyleAnimation::Value val;
  ExtractAnimationValue(aProperty, aStyleContext, val);
  if (val.GetUnit() == nsStyleAnimation::eUnit_Color) {
    return val.GetColorValue();
  }
  return NS_RGBA(0, 0, 0, 0);
}

struct ColorIndexSet {
  uint8_t colorIndex, alphaIndex;
};

static const ColorIndexSet gVisitedIndices[2] = { { 0, 0 }, { 1, 0 } };

nscolor
nsStyleContext::GetVisitedDependentColor(nsCSSProperty aProperty)
{
  NS_ASSERTION(aProperty == eCSSProperty_color ||
               aProperty == eCSSProperty_background_color ||
               aProperty == eCSSProperty_border_top_color ||
               aProperty == eCSSProperty_border_right_color_value ||
               aProperty == eCSSProperty_border_bottom_color ||
               aProperty == eCSSProperty_border_left_color_value ||
               aProperty == eCSSProperty_outline_color ||
               aProperty == eCSSProperty__moz_column_rule_color ||
               aProperty == eCSSProperty_text_decoration_color ||
               aProperty == eCSSProperty_fill ||
               aProperty == eCSSProperty_stroke,
               "we need to add to nsStyleContext::CalcStyleDifference");

  bool isPaintProperty = aProperty == eCSSProperty_fill ||
                         aProperty == eCSSProperty_stroke;

  nscolor colors[2];
  colors[0] = isPaintProperty ? ExtractColorLenient(aProperty, this)
                              : ExtractColor(aProperty, this);

  nsStyleContext *visitedStyle = this->GetStyleIfVisited();
  if (!visitedStyle) {
    return colors[0];
  }

  colors[1] = isPaintProperty ? ExtractColorLenient(aProperty, visitedStyle)
                              : ExtractColor(aProperty, visitedStyle);

  return nsStyleContext::CombineVisitedColors(colors,
                                              this->RelevantLinkVisited());
}

 nscolor
nsStyleContext::CombineVisitedColors(nscolor *aColors, bool aLinkIsVisited)
{
  if (NS_GET_A(aColors[1]) == 0) {
    
    
    
    
    aLinkIsVisited = false;
  }

  
  
  const ColorIndexSet &set =
    gVisitedIndices[aLinkIsVisited ? 1 : 0];

  nscolor colorColor = aColors[set.colorIndex];
  nscolor alphaColor = aColors[set.alphaIndex];
  return NS_RGBA(NS_GET_R(colorColor), NS_GET_G(colorColor),
                 NS_GET_B(colorColor), NS_GET_A(alphaColor));
}

void*
nsStyleContext::Alloc(size_t aSize)
{
  nsIPresShell *shell = PresContext()->PresShell();

  aSize += offsetof(AllocationHeader, mStorageStart);
  AllocationHeader *alloc =
    static_cast<AllocationHeader*>(shell->AllocateMisc(aSize));

  alloc->mSize = aSize; 

  alloc->mNext = mAllocations;
  mAllocations = alloc;

  return static_cast<void*>(&alloc->mStorageStart);
}

void
nsStyleContext::FreeAllocations(nsPresContext *aPresContext)
{
  nsIPresShell *shell = aPresContext->PresShell();

  for (AllocationHeader *alloc = mAllocations, *next; alloc; alloc = next) {
    next = alloc->mNext;
    shell->FreeMisc(alloc->mSize, alloc);
  }
}
