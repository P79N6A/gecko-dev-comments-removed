



 


#include "mozilla/DebugOnly.h"

#include "nsCSSAnonBoxes.h"
#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsPresContext.h"
#include "nsIStyleRule.h"

#include "nsCOMPtr.h"
#include "nsStyleSet.h"
#include "nsIPresShell.h"

#include "nsRuleNode.h"
#include "nsStyleContext.h"
#include "mozilla/StyleAnimationValue.h"
#include "GeckoProfiler.h"
#include "nsIDocument.h"
#include "nsPrintfCString.h"
#include "mozilla/Preferences.h"

#ifdef DEBUG

#endif

using namespace mozilla;



#ifdef DEBUG




enum DebugStyleStruct {
#define STYLE_STRUCT(name, checkdata_cb) eDebugStyleStruct_##name,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
};

#define STYLE_STRUCT(name, checkdata_cb) \
  static_assert(static_cast<int>(eDebugStyleStruct_##name) == \
                  static_cast<int>(eStyleStruct_##name), \
                "Style struct IDs are not declared in order?");
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

const uint32_t nsStyleContext::sDependencyTable[] = {
#define STYLE_STRUCT(name, checkdata_cb)
#define STYLE_STRUCT_DEP(dep) NS_STYLE_INHERIT_BIT(dep) |
#define STYLE_STRUCT_END() 0,
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
#undef STYLE_STRUCT_DEP
#undef STYLE_STRUCT_END
};


static bool sExpensiveStyleStructAssertionsEnabled;
#endif

nsStyleContext::nsStyleContext(nsStyleContext* aParent,
                               nsIAtom* aPseudoTag,
                               nsCSSPseudoElements::Type aPseudoType,
                               nsRuleNode* aRuleNode,
                               bool aSkipParentDisplayBasedStyleFixup)
  : mParent(aParent)
  , mChild(nullptr)
  , mEmptyChild(nullptr)
  , mPseudoTag(aPseudoTag)
  , mRuleNode(aRuleNode)
  , mCachedResetData(nullptr)
  , mBits(((uint64_t)aPseudoType) << NS_STYLE_CONTEXT_TYPE_SHIFT)
  , mRefCnt(0)
#ifdef DEBUG
  , mFrameRefCnt(0)
  , mComputingStruct(nsStyleStructID_None)
#endif
{
  
  
  static_assert((UINT64_MAX >> NS_STYLE_CONTEXT_TYPE_SHIFT) >=
                nsCSSPseudoElements::ePseudo_MAX,
                "pseudo element bits no longer fit in a uint64_t");
  MOZ_ASSERT(aRuleNode);

#ifdef DEBUG
  static_assert(MOZ_ARRAY_LENGTH(nsStyleContext::sDependencyTable)
                  == nsStyleStructID_Length,
                "Number of items in dependency table doesn't match IDs");
#endif

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

  ApplyStyleFixups(aSkipParentDisplayBasedStyleFixup);

  #define eStyleStruct_LastItem (nsStyleStructID_Length - 1)
  NS_ASSERTION(NS_STYLE_INHERIT_MASK & NS_STYLE_INHERIT_BIT(LastItem),
               "NS_STYLE_INHERIT_MASK must be bigger, and other bits shifted");
  #undef eStyleStruct_LastItem
}

nsStyleContext::~nsStyleContext()
{
  NS_ASSERTION((nullptr == mChild) && (nullptr == mEmptyChild), "destructing context with children");

  nsPresContext *presContext = mRuleNode->PresContext();
  nsStyleSet* styleSet = presContext->PresShell()->StyleSet();

  NS_ASSERTION(styleSet->GetRuleTree() == mRuleNode->RuleTree() ||
               styleSet->IsInRuleTreeReconstruct(),
               "destroying style context from old rule tree too late");

#ifdef DEBUG
  if (sExpensiveStyleStructAssertionsEnabled) {
    
    
    
    nsStyleContext* root = this;
    while (root->mParent) {
      root = root->mParent;
    }
    root->AssertStructsNotUsedElsewhere(this,
                                        std::numeric_limits<int32_t>::max());
  } else {
    
    
    AssertStructsNotUsedElsewhere(this, 2);
  }
#endif

  mRuleNode->Release();

  styleSet->NotifyStyleContextDestroyed(this);

  if (mParent) {
    mParent->RemoveChild(this);
    mParent->Release();
  }

  
  mCachedInheritedData.DestroyStructs(mBits, presContext);
  if (mCachedResetData) {
    mCachedResetData->Destroy(mBits, presContext);
  }
}

#ifdef DEBUG
void
nsStyleContext::AssertStructsNotUsedElsewhere(
                                       nsStyleContext* aDestroyingContext,
                                       int32_t aLevels) const
{
  if (aLevels == 0) {
    return;
  }

  void* data;

  if (mBits & NS_STYLE_IS_GOING_AWAY) {
    return;
  }

  if (this != aDestroyingContext) {
    nsInheritedStyleData& destroyingInheritedData =
      aDestroyingContext->mCachedInheritedData;
#define STYLE_STRUCT_INHERITED(name_, checkdata_cb)                            \
    data = destroyingInheritedData.mStyleStructs[eStyleStruct_##name_];        \
    if (data &&                                                                \
        !(aDestroyingContext->mBits & NS_STYLE_INHERIT_BIT(name_)) &&          \
         (mCachedInheritedData.mStyleStructs[eStyleStruct_##name_] == data)) { \
      printf_stderr("style struct %p found on style context %p\n", data, this);\
      nsString url;                                                            \
      PresContext()->Document()->GetURL(url);                                  \
      printf_stderr("  in %s\n", NS_ConvertUTF16toUTF8(url).get());            \
      MOZ_ASSERT(false, "destroying " #name_ " style struct still present "    \
                        "in style context tree");                              \
    }
#define STYLE_STRUCT_RESET(name_, checkdata_cb)

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET

    if (mCachedResetData) {
      nsResetStyleData* destroyingResetData =
        aDestroyingContext->mCachedResetData;
      if (destroyingResetData) {
#define STYLE_STRUCT_INHERITED(name_, checkdata_cb_)
#define STYLE_STRUCT_RESET(name_, checkdata_cb)                                \
        data = destroyingResetData->mStyleStructs[eStyleStruct_##name_];       \
        if (data &&                                                            \
            !(aDestroyingContext->mBits & NS_STYLE_INHERIT_BIT(name_)) &&      \
            (mCachedResetData->mStyleStructs[eStyleStruct_##name_] == data)) { \
          printf_stderr("style struct %p found on style context %p\n", data,   \
                        this);                                                 \
          nsString url;                                                        \
          PresContext()->Document()->GetURL(url);                              \
          printf_stderr("  in %s\n", NS_ConvertUTF16toUTF8(url).get());        \
          MOZ_ASSERT(false, "destroying " #name_ " style struct still present "\
                            "in style context tree");                          \
        }

#include "nsStyleStructList.h"

#undef STYLE_STRUCT_INHERITED
#undef STYLE_STRUCT_RESET
      }
    }
  }

  if (mChild) {
    const nsStyleContext* child = mChild;
    do {
      child->AssertStructsNotUsedElsewhere(aDestroyingContext, aLevels - 1);
      child = child->mNextSibling;
    } while (child != mChild);
  }

  if (mEmptyChild) {
    const nsStyleContext* child = mEmptyChild;
    do {
      child->AssertStructsNotUsedElsewhere(aDestroyingContext, aLevels - 1);
      child = child->mNextSibling;
    } while (child != mEmptyChild);
  }
}
#endif

void nsStyleContext::AddChild(nsStyleContext* aChild)
{
  NS_ASSERTION(aChild->mPrevSibling == aChild &&
               aChild->mNextSibling == aChild,
               "child already in a child list");

  nsStyleContext **listPtr = aChild->mRuleNode->IsRoot() ? &mEmptyChild : &mChild;
  
  
  nsStyleContext *list = *listPtr;

  
  if (list) {
    
    aChild->mNextSibling = list;
    aChild->mPrevSibling = list->mPrevSibling;
    list->mPrevSibling->mNextSibling = aChild;
    list->mPrevSibling = aChild;
  }
  (*listPtr) = aChild;
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

void
nsStyleContext::MoveTo(nsStyleContext* aNewParent)
{
  MOZ_ASSERT(aNewParent != mParent);

  
  
  
  DebugOnly<uint64_t> mask = NS_STYLE_IN_DISPLAY_NONE_SUBTREE;
  MOZ_ASSERT((mParent->mBits & mask) == (aNewParent->mBits & mask));

  
  
  MOZ_ASSERT(!IsStyleIfVisited());
  MOZ_ASSERT(!aNewParent->IsStyleIfVisited());

  nsStyleContext* oldParent = mParent;

  aNewParent->AddRef();

  mParent->RemoveChild(this);

  mParent = aNewParent;
  mParent->AddChild(this);

  oldParent->Release();
}

already_AddRefed<nsStyleContext>
nsStyleContext::FindChildWithRules(const nsIAtom* aPseudoTag, 
                                   nsRuleNode* aRuleNode,
                                   nsRuleNode* aRulesIfVisited,
                                   bool aRelevantLinkVisited)
{
  uint32_t threshold = 10; 
                           

  nsRefPtr<nsStyleContext> result;
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
        if (match && !(child->mBits & NS_STYLE_INELIGIBLE_FOR_SHARING)) {
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
    result->mBits |= NS_STYLE_IS_SHARED;
  }

  return result.forget();
}

 bool
nsStyleContext::ListContainsStyleContextThatUsesGrandancestorStyle(const nsStyleContext* aHead)
{
  if (aHead) {
    const nsStyleContext* child = aHead;
    do {
      if (child->UsesGrandancestorStyle()) {
        return true;
      }
      child = child->mNextSibling;
    } while (child != aHead);
  }

  return false;
}

bool
nsStyleContext::HasChildThatUsesGrandancestorStyle() const
{
  return ListContainsStyleContextThatUsesGrandancestorStyle(mEmptyChild) ||
         ListContainsStyleContextThatUsesGrandancestorStyle(mChild);
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

const void* nsStyleContext::StyleData(nsStyleStructID aSID)
{
  const void* cachedData = GetCachedStyleData(aSID);
  if (cachedData)
    return cachedData; 
  return mRuleNode->GetStyleData(aSID, this, true); 
}




void* 
nsStyleContext::GetUniqueStyleData(const nsStyleStructID& aSID)
{
  
  
  
  
  
  const void *current = StyleData(aSID);
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
  UNIQUE_CASE(Text)
  UNIQUE_CASE(TextReset)
  UNIQUE_CASE(Visibility)

#undef UNIQUE_CASE

  default:
    NS_ERROR("Struct type not supported.  Please find another way to do this if you can!");
    return nullptr;
  }

  SetStyle(aSID, result);
  mBits &= ~static_cast<uint64_t>(nsCachedStyleData::GetBitForSID(aSID));

  return result;
}



void*
nsStyleContext::CreateEmptyStyleData(const nsStyleStructID& aSID)
{
  MOZ_ASSERT(!mChild && !mEmptyChild &&
             !(mBits & nsCachedStyleData::GetBitForSID(aSID)) &&
             !GetCachedStyleData(aSID),
             "This style should not have been computed");

  void* result;
  nsPresContext* presContext = PresContext();
  switch (aSID) {
#define UNIQUE_CASE(c_, ...) \
    case eStyleStruct_##c_: \
      result = new (presContext) nsStyle##c_(__VA_ARGS__); \
      break;

  UNIQUE_CASE(Border, presContext)
  UNIQUE_CASE(Padding)

#undef UNIQUE_CASE

  default:
    NS_ERROR("Struct type not supported.");
    return nullptr;
  }

  
  
  
  SetStyle(aSID, result);
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

static bool
ShouldSuppressLineBreak(const nsStyleDisplay* aStyleDisplay,
                        const nsStyleContext* aContainerContext,
                        const nsStyleDisplay* aContainerDisplay)
{
  
  if (aStyleDisplay->IsOutOfFlowStyle()) {
    return false;
  }
  if (aContainerContext->ShouldSuppressLineBreak()) {
    
    
    if (aContainerDisplay->mDisplay == NS_STYLE_DISPLAY_INLINE ||
        aContainerDisplay->mDisplay == NS_STYLE_DISPLAY_RUBY ||
        aContainerDisplay->mDisplay == NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER) {
      return true;
    }
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if ((aContainerDisplay->IsRubyDisplayType() &&
       aStyleDisplay->mDisplay != NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER &&
       aStyleDisplay->mDisplay != NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER) ||
      
      
      aStyleDisplay->mDisplay == NS_STYLE_DISPLAY_RUBY_BASE ||
      aStyleDisplay->mDisplay == NS_STYLE_DISPLAY_RUBY_TEXT) {
    return true;
  }
  return false;
}

void
nsStyleContext::ApplyStyleFixups(bool aSkipParentDisplayBasedStyleFixup)
{
  
  
  if (mParent && mParent->HasTextDecorationLines()) {
    mBits |= NS_STYLE_HAS_TEXT_DECORATION_LINES;
  } else {
    
    const nsStyleTextReset* text = StyleTextReset();
    uint8_t decorationLine = text->mTextDecorationLine;
    if (decorationLine != NS_STYLE_TEXT_DECORATION_LINE_NONE &&
        decorationLine != NS_STYLE_TEXT_DECORATION_LINE_OVERRIDE_ALL) {
      mBits |= NS_STYLE_HAS_TEXT_DECORATION_LINES;
    }
  }

  if ((mParent && mParent->HasPseudoElementData()) || mPseudoTag) {
    mBits |= NS_STYLE_HAS_PSEUDO_ELEMENT_DATA;
  }

  
  
  if (mPseudoTag == nsCSSAnonBoxes::viewport) {
    nsPresContext* presContext = PresContext();
    mozilla::dom::Element* docElement = presContext->Document()->GetRootElement();
    if (docElement) {
      nsRefPtr<nsStyleContext> rootStyle =
        presContext->StyleSet()->ResolveStyleFor(docElement, nullptr);
      auto dir = rootStyle->StyleVisibility()->mDirection;
      if (dir != StyleVisibility()->mDirection) {
        nsStyleVisibility* uniqueVisibility =
          (nsStyleVisibility*)GetUniqueStyleData(eStyleStruct_Visibility);
        uniqueVisibility->mDirection = dir;
      }
    }
  }

  
  const nsStyleDisplay* disp = StyleDisplay();
  if (disp->mDisplay == NS_STYLE_DISPLAY_TABLE) {
    
    
    
    const nsStyleText* text = StyleText();
    
    if (text->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_CENTER ||
        text->mTextAlign == NS_STYLE_TEXT_ALIGN_MOZ_RIGHT)
    {
      nsStyleText* uniqueText = (nsStyleText*)GetUniqueStyleData(eStyleStruct_Text);
      uniqueText->mTextAlign = NS_STYLE_TEXT_ALIGN_DEFAULT;
    }
  }

  
  
  
  
  
  
  
  if (!mParent) {
    uint8_t displayVal = disp->mDisplay;
    if (displayVal != NS_STYLE_DISPLAY_CONTENTS) {
      nsRuleNode::EnsureBlockDisplay(displayVal, true);
    } else {
      
      
      
      displayVal = NS_STYLE_DISPLAY_BLOCK;
    }
    if (displayVal != disp->mDisplay) {
      nsStyleDisplay *mutable_display =
        static_cast<nsStyleDisplay*>(GetUniqueStyleData(eStyleStruct_Display));

      
      
      
      
      
      mutable_display->mOriginalDisplay = mutable_display->mDisplay =
        displayVal;
    }
  }

  
  
  
  
  
  
  
  if (!aSkipParentDisplayBasedStyleFixup && mParent) {
    
    
    
    
    
    nsStyleContext* containerContext = mParent;
    const nsStyleDisplay* containerDisp = containerContext->StyleDisplay();
    while (containerDisp->mDisplay == NS_STYLE_DISPLAY_CONTENTS) {
      if (!containerContext->GetParent()) {
        break;
      }
      containerContext = containerContext->GetParent();
      containerDisp = containerContext->StyleDisplay();
    }
    if (containerDisp->IsFlexOrGridDisplayType() &&
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

    if (::ShouldSuppressLineBreak(disp, containerContext, containerDisp)) {
      mBits |= NS_STYLE_SUPPRESS_LINEBREAK;
      uint8_t displayVal = disp->mDisplay;
      nsRuleNode::EnsureInlineDisplay(displayVal);
      if (displayVal != disp->mDisplay) {
        nsStyleDisplay *mutable_display =
          static_cast<nsStyleDisplay*>(GetUniqueStyleData(eStyleStruct_Display));
        mutable_display->mDisplay = displayVal;
      }
    }
  }

  
  if ((mParent && mParent->IsInDisplayNoneSubtree()) ||
      disp->mDisplay == NS_STYLE_DISPLAY_NONE) {
    mBits |= NS_STYLE_IN_DISPLAY_NONE_SUBTREE;
  }

  
  if (disp->mDisplay == NS_STYLE_DISPLAY_RUBY_BASE_CONTAINER ||
      disp->mDisplay == NS_STYLE_DISPLAY_RUBY_TEXT_CONTAINER) {
    CreateEmptyStyleData(eStyleStruct_Border);
    CreateEmptyStyleData(eStyleStruct_Padding);
  }
  if (disp->IsRubyDisplayType()) {
    
    
    
    const nsStyleTextReset* textReset = StyleTextReset();
    uint8_t unicodeBidi = textReset->mUnicodeBidi;
    if (unicodeBidi == NS_STYLE_UNICODE_BIDI_NORMAL ||
        unicodeBidi == NS_STYLE_UNICODE_BIDI_EMBED) {
      unicodeBidi = NS_STYLE_UNICODE_BIDI_ISOLATE;
    } else if (unicodeBidi == NS_STYLE_UNICODE_BIDI_OVERRIDE) {
      unicodeBidi = NS_STYLE_UNICODE_BIDI_ISOLATE_OVERRIDE;
    }
    if (unicodeBidi != textReset->mUnicodeBidi) {
      auto mutableTextReset = static_cast<nsStyleTextReset*>(
        GetUniqueStyleData(eStyleStruct_TextReset));
      mutableTextReset->mUnicodeBidi = unicodeBidi;
    }
  }

  
  
  if (disp->mDisplay == NS_STYLE_DISPLAY_INLINE && mParent) {
    
    
    bool thisHorizontal =
      StyleVisibility()->mWritingMode == NS_STYLE_WRITING_MODE_HORIZONTAL_TB;
    bool parentHorizontal = mParent->StyleVisibility()->mWritingMode ==
                              NS_STYLE_WRITING_MODE_HORIZONTAL_TB;
    if (thisHorizontal != parentHorizontal) {
      nsStyleDisplay *mutable_display =
        static_cast<nsStyleDisplay*>(GetUniqueStyleData(eStyleStruct_Display));
      mutable_display->mOriginalDisplay = mutable_display->mDisplay =
        NS_STYLE_DISPLAY_INLINE_BLOCK;
    }
  }

  
  StyleUserInterface();
}

nsChangeHint
nsStyleContext::CalcStyleDifference(nsStyleContext* aOther,
                                    nsChangeHint aParentHintsNotHandledForDescendants,
                                    uint32_t* aEqualStructs)
{
  PROFILER_LABEL("nsStyleContext", "CalcStyleDifference",
    js::ProfileEntry::Category::CSS);

  MOZ_ASSERT(NS_IsHintSubset(aParentHintsNotHandledForDescendants,
                             nsChangeHint_Hints_NotHandledForDescendants),
             "caller is passing inherited hints, but shouldn't be");

  static_assert(nsStyleStructID_Length <= 32,
                "aEqualStructs is not big enough");

  *aEqualStructs = 0;

  nsChangeHint hint = NS_STYLE_HINT_NONE;
  NS_ENSURE_TRUE(aOther, hint);
  
  
  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  bool compare = mRuleNode != aOther->mRuleNode;

  
  
  
  const nsStyleVariables* thisVariables = PeekStyleVariables();
  if (thisVariables) {
    const nsStyleVariables* otherVariables = aOther->StyleVariables();
    if (thisVariables->mVariables == otherVariables->mVariables) {
      *aEqualStructs |= nsCachedStyleData::GetBitForSID(eStyleStruct_Variables);
    } else {
      compare = true;
    }
  } else {
    *aEqualStructs |= nsCachedStyleData::GetBitForSID(eStyleStruct_Variables);
  }

  DebugOnly<int> styleStructCount = 1;  

#define DO_STRUCT_DIFFERENCE(struct_)                                         \
  PR_BEGIN_MACRO                                                              \
    const nsStyle##struct_* this##struct_ = PeekStyle##struct_();             \
    if (this##struct_) {                                                      \
      const nsStyle##struct_* other##struct_ = aOther->Style##struct_();      \
      nsChangeHint maxDifference = nsStyle##struct_::MaxDifference();         \
      nsChangeHint maxDifferenceNeverInherited =                              \
        nsStyle##struct_::MaxDifferenceNeverInherited();                      \
      if (this##struct_ == other##struct_) {                                  \
        /* The very same struct, so we know that there will be no */          \
        /* differences.                                           */          \
        *aEqualStructs |= NS_STYLE_INHERIT_BIT(struct_);                      \
      } else if (compare ||                                                   \
                 (NS_SubtractHint(maxDifference,                              \
                                  maxDifferenceNeverInherited) &              \
                  aParentHintsNotHandledForDescendants)) {                    \
        nsChangeHint difference =                                             \
            this##struct_->CalcDifference(*other##struct_);                   \
        NS_ASSERTION(NS_IsHintSubset(difference, maxDifference),              \
                     "CalcDifference() returned bigger hint than "            \
                     "MaxDifference()");                                      \
        NS_UpdateHint(hint, difference);                                      \
        if (!difference) {                                                    \
          *aEqualStructs |= NS_STYLE_INHERIT_BIT(struct_);                    \
        }                                                                     \
      } else {                                                                \
        /* We still must call CalcDifference to see if there were any */      \
        /* changes so that we can set *aEqualStructs appropriately.   */      \
        nsChangeHint difference =                                             \
            this##struct_->CalcDifference(*other##struct_);                   \
        NS_ASSERTION(NS_IsHintSubset(difference, maxDifference),              \
                     "CalcDifference() returned bigger hint than "            \
                     "MaxDifference()");                                      \
        if (!difference) {                                                    \
          *aEqualStructs |= NS_STYLE_INHERIT_BIT(struct_);                    \
        }                                                                     \
      }                                                                       \
    } else {                                                                  \
      *aEqualStructs |= NS_STYLE_INHERIT_BIT(struct_);                        \
    }                                                                         \
    styleStructCount++;                                                       \
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

  MOZ_ASSERT(styleStructCount == nsStyleStructID_Length,
             "missing a call to DO_STRUCT_DIFFERENCE");

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nsStyleContext *thisVis = GetStyleIfVisited(),
                *otherVis = aOther->GetStyleIfVisited();
  if (!thisVis != !otherVis) {
    
    
    NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
  } else if (thisVis && !NS_IsHintSubset(nsChangeHint_RepaintFrame, hint)) {
    
    bool change = false;

    
    
    
    
    
    if (PeekStyleColor()) {
      if (thisVis->StyleColor()->mColor !=
          otherVis->StyleColor()->mColor) {
        change = true;
      }
    }

    
    if (!change && PeekStyleBackground()) {
      if (thisVis->StyleBackground()->mBackgroundColor !=
          otherVis->StyleBackground()->mBackgroundColor) {
        change = true;
      }
    }

    
    if (!change && PeekStyleBorder()) {
      const nsStyleBorder *thisVisBorder = thisVis->StyleBorder();
      const nsStyleBorder *otherVisBorder = otherVis->StyleBorder();
      NS_FOR_CSS_SIDES(side) {
        bool thisFG, otherFG;
        
        
        nscolor thisColor = NS_RGBA(0, 0, 0, 0);
        nscolor otherColor = NS_RGBA(0, 0, 0, 0);
        thisVisBorder->GetBorderColor(side, thisColor, thisFG);
        otherVisBorder->GetBorderColor(side, otherColor, otherFG);
        if (thisFG != otherFG || (!thisFG && thisColor != otherColor)) {
          change = true;
          break;
        }
      }
    }

    
    if (!change && PeekStyleOutline()) {
      const nsStyleOutline *thisVisOutline = thisVis->StyleOutline();
      const nsStyleOutline *otherVisOutline = otherVis->StyleOutline();
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
      const nsStyleColumn *thisVisColumn = thisVis->StyleColumn();
      const nsStyleColumn *otherVisColumn = otherVis->StyleColumn();
      if (thisVisColumn->mColumnRuleColor != otherVisColumn->mColumnRuleColor ||
          thisVisColumn->mColumnRuleColorIsForeground !=
            otherVisColumn->mColumnRuleColorIsForeground) {
        change = true;
      }
    }

    
    if (!change && PeekStyleTextReset()) {
      const nsStyleTextReset *thisVisTextReset = thisVis->StyleTextReset();
      const nsStyleTextReset *otherVisTextReset = otherVis->StyleTextReset();
      
      
      nscolor thisVisDecColor = NS_RGBA(0, 0, 0, 0);
      nscolor otherVisDecColor = NS_RGBA(0, 0, 0, 0);
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
      const nsStyleSVG *thisVisSVG = thisVis->StyleSVG();
      const nsStyleSVG *otherVisSVG = otherVis->StyleSVG();
      if (thisVisSVG->mFill != otherVisSVG->mFill ||
          thisVisSVG->mStroke != otherVisSVG->mStroke) {
        change = true;
      }
    }

    if (change) {
      NS_UpdateHint(hint, nsChangeHint_RepaintFrame);
    }
  }

  return NS_SubtractHint(hint, nsChangeHint_NeutralChange);
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
void nsStyleContext::List(FILE* out, int32_t aIndent, bool aListDescendants)
{
  nsAutoCString str;
  
  int32_t ix;
  for (ix = aIndent; --ix >= 0; ) {
    str.AppendLiteral("  ");
  }
  str.Append(nsPrintfCString("%p(%d) parent=%p ",
                             (void*)this, mRefCnt, (void *)mParent));
  if (mPseudoTag) {
    nsAutoString  buffer;
    mPseudoTag->ToString(buffer);
    AppendUTF16toUTF8(buffer, str);
    str.Append(' ');
  }

  if (mRuleNode) {
    fprintf_stderr(out, "%s{\n", str.get());
    str.Truncate();
    nsRuleNode* ruleNode = mRuleNode;
    while (ruleNode) {
      nsIStyleRule *styleRule = ruleNode->GetRule();
      if (styleRule) {
        styleRule->List(out, aIndent + 1);
      }
      ruleNode = ruleNode->GetParent();
    }
    for (ix = aIndent; --ix >= 0; ) {
      str.AppendLiteral("  ");
    }
    fprintf_stderr(out, "%s}\n", str.get());
  }
  else {
    fprintf_stderr(out, "%s{}\n", str.get());
  }

  if (aListDescendants) {
    if (nullptr != mChild) {
      nsStyleContext* child = mChild;
      do {
        child->List(out, aIndent + 1, aListDescendants);
        child = child->mNextSibling;
      } while (mChild != child);
    }
    if (nullptr != mEmptyChild) {
      nsStyleContext* child = mEmptyChild;
      do {
        child->List(out, aIndent + 1, aListDescendants);
        child = child->mNextSibling;
      } while (mEmptyChild != child);
    }
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
                   nsRuleNode* aRuleNode,
                   bool aSkipParentDisplayBasedStyleFixup)
{
  nsRefPtr<nsStyleContext> context =
    new (aRuleNode->PresContext())
    nsStyleContext(aParentContext, aPseudoTag, aPseudoType, aRuleNode,
                   aSkipParentDisplayBasedStyleFixup);
  return context.forget();
}

static inline void
ExtractAnimationValue(nsCSSProperty aProperty,
                      nsStyleContext* aStyleContext,
                      StyleAnimationValue& aResult)
{
  DebugOnly<bool> success =
    StyleAnimationValue::ExtractComputedValue(aProperty, aStyleContext,
                                              aResult);
  MOZ_ASSERT(success,
             "aProperty must be extractable by StyleAnimationValue");
}

static nscolor
ExtractColor(nsCSSProperty aProperty,
             nsStyleContext *aStyleContext)
{
  StyleAnimationValue val;
  ExtractAnimationValue(aProperty, aStyleContext, val);
  return val.GetColorValue();
}

static nscolor
ExtractColorLenient(nsCSSProperty aProperty,
                    nsStyleContext *aStyleContext)
{
  StyleAnimationValue val;
  ExtractAnimationValue(aProperty, aStyleContext, val);
  if (val.GetUnit() == StyleAnimationValue::eUnit_Color) {
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
               aProperty == eCSSProperty_border_right_color ||
               aProperty == eCSSProperty_border_bottom_color ||
               aProperty == eCSSProperty_border_left_color ||
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

#ifdef DEBUG
 void
nsStyleContext::AssertStyleStructMaxDifferenceValid()
{
#define STYLE_STRUCT(name, checkdata_cb)                                     \
    MOZ_ASSERT(NS_IsHintSubset(nsStyle##name::MaxDifferenceNeverInherited(), \
                               nsStyle##name::MaxDifference()));
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
}

 const char*
nsStyleContext::StructName(nsStyleStructID aSID)
{
  switch (aSID) {
#define STYLE_STRUCT(name_, checkdata_cb)                                     \
    case eStyleStruct_##name_:                                                \
      return #name_;
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
    default:
      return "Unknown";
  }
}

 bool
nsStyleContext::LookupStruct(const nsACString& aName, nsStyleStructID& aResult)
{
  if (false)
    ;
#define STYLE_STRUCT(name_, checkdata_cb_)                                    \
  else if (aName.EqualsLiteral(#name_))                                       \
    aResult = eStyleStruct_##name_;
#include "nsStyleStructList.h"
#undef STYLE_STRUCT
  else
    return false;
  return true;
}
#endif

bool
nsStyleContext::HasSameCachedStyleData(nsStyleContext* aOther,
                                       nsStyleStructID aSID)
{
  return GetCachedStyleData(aSID) == aOther->GetCachedStyleData(aSID);
}

void
nsStyleContext::SwapStyleData(nsStyleContext* aNewContext, uint32_t aStructs)
{
  static_assert(nsStyleStructID_Length <= 32, "aStructs is not big enough");

  for (nsStyleStructID i = nsStyleStructID_Inherited_Start;
       i < nsStyleStructID_Inherited_Start + nsStyleStructID_Inherited_Count;
       i = nsStyleStructID(i + 1)) {
    uint32_t bit = nsCachedStyleData::GetBitForSID(i);
    if (!(aStructs & bit)) {
      continue;
    }
    void*& thisData = mCachedInheritedData.mStyleStructs[i];
    void*& otherData = aNewContext->mCachedInheritedData.mStyleStructs[i];
    if (mBits & bit) {
      if (thisData == otherData) {
        thisData = nullptr;
      }
    } else if (!(aNewContext->mBits & bit) && thisData && otherData) {
      std::swap(thisData, otherData);
    }
  }

  for (nsStyleStructID i = nsStyleStructID_Reset_Start;
       i < nsStyleStructID_Reset_Start + nsStyleStructID_Reset_Count;
       i = nsStyleStructID(i + 1)) {
    uint32_t bit = nsCachedStyleData::GetBitForSID(i);
    if (!(aStructs & bit)) {
      continue;
    }
    if (!mCachedResetData) {
      mCachedResetData = new (mRuleNode->PresContext()) nsResetStyleData;
    }
    if (!aNewContext->mCachedResetData) {
      aNewContext->mCachedResetData =
        new (mRuleNode->PresContext()) nsResetStyleData;
    }
    void*& thisData = mCachedResetData->mStyleStructs[i];
    void*& otherData = aNewContext->mCachedResetData->mStyleStructs[i];
    if (mBits & bit) {
      if (thisData == otherData) {
        thisData = nullptr;
      }
    } else if (!(aNewContext->mBits & bit) && thisData && otherData) {
      std::swap(thisData, otherData);
    }
  }
}

void
nsStyleContext::ClearCachedInheritedStyleDataOnDescendants(uint32_t aStructs)
{
  if (mChild) {
    nsStyleContext* child = mChild;
    do {
      child->DoClearCachedInheritedStyleDataOnDescendants(aStructs);
      child = child->mNextSibling;
    } while (mChild != child);
  }
  if (mEmptyChild) {
    nsStyleContext* child = mEmptyChild;
    do {
      child->DoClearCachedInheritedStyleDataOnDescendants(aStructs);
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
}

void
nsStyleContext::DoClearCachedInheritedStyleDataOnDescendants(uint32_t aStructs)
{
  NS_ASSERTION(mFrameRefCnt == 0, "frame still referencing style context");
  for (nsStyleStructID i = nsStyleStructID_Inherited_Start;
       i < nsStyleStructID_Inherited_Start + nsStyleStructID_Inherited_Count;
       i = nsStyleStructID(i + 1)) {
    uint32_t bit = nsCachedStyleData::GetBitForSID(i);
    if (aStructs & bit) {
      if (!(mBits & bit) && mCachedInheritedData.mStyleStructs[i]) {
        aStructs &= ~bit;
      } else {
        mCachedInheritedData.mStyleStructs[i] = nullptr;
      }
    }
  }

  if (mCachedResetData) {
    for (nsStyleStructID i = nsStyleStructID_Reset_Start;
         i < nsStyleStructID_Reset_Start + nsStyleStructID_Reset_Count;
         i = nsStyleStructID(i + 1)) {
      uint32_t bit = nsCachedStyleData::GetBitForSID(i);
      if (aStructs & bit) {
        if (!(mBits & bit) && mCachedResetData->mStyleStructs[i]) {
          aStructs &= ~bit;
        } else {
          mCachedResetData->mStyleStructs[i] = nullptr;
        }
      }
    }
  }

  if (aStructs == 0) {
    return;
  }

  ClearCachedInheritedStyleDataOnDescendants(aStructs);
}

void
nsStyleContext::SetIneligibleForSharing()
{
  if (mBits & NS_STYLE_INELIGIBLE_FOR_SHARING) {
    return;
  }
  mBits |= NS_STYLE_INELIGIBLE_FOR_SHARING;
  if (mChild) {
    nsStyleContext* child = mChild;
    do {
      child->SetIneligibleForSharing();
      child = child->mNextSibling;
    } while (mChild != child);
  }
  if (mEmptyChild) {
    nsStyleContext* child = mEmptyChild;
    do {
      child->SetIneligibleForSharing();
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
}

#ifdef RESTYLE_LOGGING
nsCString
nsStyleContext::GetCachedStyleDataAsString(uint32_t aStructs)
{
  nsCString structs;
  for (nsStyleStructID i = nsStyleStructID(0);
       i < nsStyleStructID_Length;
       i = nsStyleStructID(i + 1)) {
    if (aStructs & nsCachedStyleData::GetBitForSID(i)) {
      const void* data = GetCachedStyleData(i);
      if (!structs.IsEmpty()) {
        structs.Append(' ');
      }
      structs.AppendPrintf("%s=%p", StructName(i), data);
      if (HasCachedInheritedStyleData(i)) {
        structs.AppendLiteral("(dependent)");
      } else {
        structs.AppendLiteral("(owned)");
      }
    }
  }
  return structs;
}

int32_t&
nsStyleContext::LoggingDepth()
{
  static int32_t depth = 0;
  return depth;
}

void
nsStyleContext::LogStyleContextTree(int32_t aLoggingDepth, uint32_t aStructs)
{
  LoggingDepth() = aLoggingDepth;
  LogStyleContextTree(true, aStructs);
}

void
nsStyleContext::LogStyleContextTree(bool aFirst, uint32_t aStructs)
{
  nsCString structs = GetCachedStyleDataAsString(aStructs);
  if (!structs.IsEmpty()) {
    structs.Append(' ');
  }

  nsCString pseudo;
  if (mPseudoTag) {
    nsAutoString pseudoTag;
    mPseudoTag->ToString(pseudoTag);
    AppendUTF16toUTF8(pseudoTag, pseudo);
    pseudo.Append(' ');
  }

  nsCString flags;
  if (IsStyleIfVisited()) {
    flags.AppendLiteral("IS_STYLE_IF_VISITED ");
  }
  if (UsesGrandancestorStyle()) {
    flags.AppendLiteral("USES_GRANDANCESTOR_STYLE ");
  }
  if (IsShared()) {
    flags.AppendLiteral("IS_SHARED ");
  }

  nsCString parent;
  if (aFirst) {
    parent.AppendPrintf("parent=%p ", mParent);
  }

  LOG_RESTYLE("%p(%d) %s%s%s%s",
              this, mRefCnt,
              structs.get(), pseudo.get(), flags.get(), parent.get());

  LOG_RESTYLE_INDENT();

  if (nullptr != mChild) {
    nsStyleContext* child = mChild;
    do {
      child->LogStyleContextTree(false, aStructs);
      child = child->mNextSibling;
    } while (mChild != child);
  }
  if (nullptr != mEmptyChild) {
    nsStyleContext* child = mEmptyChild;
    do {
      child->LogStyleContextTree(false, aStructs);
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
}
#endif

#ifdef DEBUG
 void
nsStyleContext::Initialize()
{
  Preferences::AddBoolVarCache(
      &sExpensiveStyleStructAssertionsEnabled,
      "layout.css.expensive-style-struct-assertions.enabled");
}
#endif
