





































 


#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsPresContext.h"
#include "nsIStyleRule.h"

#include "nsCOMPtr.h"
#include "nsStyleSet.h"
#include "nsIPresShell.h"

#include "nsRuleNode.h"
#include "nsStyleContext.h"

#ifdef DEBUG

#endif




nsStyleContext::nsStyleContext(nsStyleContext* aParent,
                               nsIAtom* aPseudoTag,
                               nsRuleNode* aRuleNode,
                               nsPresContext* aPresContext)
  : mParent(aParent),
    mChild(nsnull),
    mEmptyChild(nsnull),
    mPseudoTag(aPseudoTag),
    mRuleNode(aRuleNode),
    mBits(0),
    mRefCnt(0)
{
  mNextSibling = this;
  mPrevSibling = this;
  if (mParent) {
    mParent->AddRef();
    mParent->AddChild(this);
#ifdef DEBUG
    nsRuleNode *r1 = mParent->GetRuleNode(), *r2 = aRuleNode;
    while (r1->GetParent())
      r1 = r1->GetParent();
    while (r2->GetParent())
      r2 = r2->GetParent();
    NS_ASSERTION(r1 == r2, "must be in the same rule tree as parent");
#endif
  }

  ApplyStyleFixups(aPresContext);

  #define eStyleStruct_LastItem (nsStyleStructID_Length - 1)
  NS_ASSERTION(NS_STYLE_INHERIT_MASK & NS_STYLE_INHERIT_BIT(LastItem),
               "NS_STYLE_INHERIT_MASK must be bigger, and other bits shifted");
  #undef eStyleStruct_LastItem
}

nsStyleContext::~nsStyleContext()
{
  NS_ASSERTION((nsnull == mChild) && (nsnull == mEmptyChild), "destructing context with children");

  nsPresContext *presContext = mRuleNode->GetPresContext();

  presContext->PresShell()->StyleSet()->
    NotifyStyleContextDestroyed(presContext, this);

  if (mParent) {
    mParent->RemoveChild(this);
    mParent->Release();
  }

  
  if (mCachedStyleData.mResetData || mCachedStyleData.mInheritedData) {
    mCachedStyleData.Destroy(mBits, presContext);
  }
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
  NS_PRECONDITION(nsnull != aChild && this == aChild->mParent, "bad argument");

  nsStyleContext **list = aChild->mRuleNode->IsRoot() ? &mEmptyChild : &mChild;

  if (aChild->mPrevSibling != aChild) { 
    if ((*list) == aChild) {
      (*list) = (*list)->mNextSibling;
    }
  } 
  else {
    NS_ASSERTION((*list) == aChild, "bad sibling pointers");
    (*list) = nsnull;
  }

  aChild->mPrevSibling->mNextSibling = aChild->mNextSibling;
  aChild->mNextSibling->mPrevSibling = aChild->mPrevSibling;
  aChild->mNextSibling = aChild;
  aChild->mPrevSibling = aChild;
}

already_AddRefed<nsStyleContext>
nsStyleContext::FindChildWithRules(const nsIAtom* aPseudoTag, 
                                   nsRuleNode* aRuleNode)
{
  PRUint32 threshold = 10; 
                           

  nsStyleContext* result = nsnull;
  nsStyleContext *list = aRuleNode->IsRoot() ? mEmptyChild : mChild;

  if (list) {
    nsStyleContext *child = list;
    do {
      if (child->mRuleNode == aRuleNode && child->mPseudoTag == aPseudoTag) {
        result = child;
        break;
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

const void* nsStyleContext::GetStyleData(nsStyleStructID aSID)
{
  const void* cachedData = mCachedStyleData.GetStyleData(aSID); 
  if (cachedData)
    return cachedData; 
  return mRuleNode->GetStyleData(aSID, this, PR_TRUE); 
}

#define STYLE_STRUCT(name_, checkdata_cb_, ctor_args_)                      \
  const nsStyle##name_ * nsStyleContext::GetStyle##name_ ()                 \
  {                                                                         \
    const nsStyle##name_ * cachedData = mCachedStyleData.GetStyle##name_(); \
    if (cachedData)                                                         \
      return cachedData; /* We have computed data stored on this node */    \
                         /* in the context tree. */                         \
    /* Else our rule node will take care of it for us. */                   \
    return mRuleNode->GetStyle##name_(this, PR_TRUE);                       \
  }
#include "nsStyleStructList.h"
#undef STYLE_STRUCT

const void* nsStyleContext::PeekStyleData(nsStyleStructID aSID)
{
  const void* cachedData = mCachedStyleData.GetStyleData(aSID); 
  if (cachedData)
    return cachedData; 
  return mRuleNode->GetStyleData(aSID, this, PR_FALSE); 
}




void* 
nsStyleContext::GetUniqueStyleData(const nsStyleStructID& aSID)
{
  
  
  
  
  
  const void *current = GetStyleData(aSID);
  if (!mChild && !mEmptyChild &&
      !(mBits & nsCachedStyleData::GetBitForSID(aSID)) &&
      mCachedStyleData.GetStyleData(aSID))
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
    NS_ERROR("Struct type not supported.  Please find another way to do this if you can!\n");
    return nsnull;
  }

  if (!result) {
    NS_WARNING("Ran out of memory while trying to allocate memory for a unique style struct! "
               "Returning the non-unique data.");
    return const_cast<void*>(current);
  }

  SetStyle(aSID, result);
  mBits &= ~nsCachedStyleData::GetBitForSID(aSID);

  return result;
}

void
nsStyleContext::SetStyle(nsStyleStructID aSID, void* aStruct)
{
  
  
  
  NS_ASSERTION(aSID >= 0 && aSID < nsStyleStructID_Length, "out of bounds");

  
  
  

  const nsCachedStyleData::StyleStructInfo& info =
      nsCachedStyleData::gInfo[aSID];
  char* resetOrInheritSlot = reinterpret_cast<char*>(&mCachedStyleData) +
                             info.mCachedStyleDataOffset;
  char* resetOrInherit = reinterpret_cast<char*>
                                         (*reinterpret_cast<void**>(resetOrInheritSlot));
  if (!resetOrInherit) {
    nsPresContext *presContext = mRuleNode->GetPresContext();
    if (mCachedStyleData.IsReset(aSID)) {
      mCachedStyleData.mResetData = new (presContext) nsResetStyleData;
      resetOrInherit = reinterpret_cast<char*>(mCachedStyleData.mResetData);
    } else {
      mCachedStyleData.mInheritedData =
          new (presContext) nsInheritedStyleData;
      resetOrInherit =
          reinterpret_cast<char*>(mCachedStyleData.mInheritedData);
    }
  }
  char* dataSlot = resetOrInherit + info.mInheritResetOffset;
  NS_ASSERTION(!*reinterpret_cast<void**>(dataSlot),
               "Going to leak style data");
  *reinterpret_cast<void**>(dataSlot) = aStruct;
}

void
nsStyleContext::ApplyStyleFixups(nsPresContext* aPresContext)
{
  
  
  if (mParent && mParent->HasTextDecorations())
    mBits |= NS_STYLE_HAS_TEXT_DECORATIONS;
  else {
    
    const nsStyleTextReset* text = GetStyleTextReset();
    if (text->mTextDecoration != NS_STYLE_TEXT_DECORATION_NONE &&
        text->mTextDecoration != NS_STYLE_TEXT_DECORATION_OVERRIDE_ALL)
      mBits |= NS_STYLE_HAS_TEXT_DECORATIONS;
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
        mutable_display->mDisplay = NS_STYLE_DISPLAY_TABLE;
      else
        mutable_display->mDisplay = NS_STYLE_DISPLAY_BLOCK;
    }
  }

  
  GetStyleUserInterface();
}

nsChangeHint
nsStyleContext::CalcStyleDifference(nsStyleContext* aOther)
{
  nsChangeHint hint = NS_STYLE_HINT_NONE;
  NS_ENSURE_TRUE(aOther, hint);
  
  
  
  
  
  
  
  

  
  
  
  
  
  PRBool compare = mRuleNode != aOther->mRuleNode;

  nsChangeHint maxHint = nsChangeHint(NS_STYLE_HINT_FRAMECHANGE |
      nsChangeHint_UpdateCursor);
  
#define DO_STRUCT_DIFFERENCE(struct_)                                         \
  PR_BEGIN_MACRO                                                              \
    NS_ASSERTION(NS_IsHintSubset(nsStyle##struct_::MaxDifference(), maxHint), \
                 "Struct placed in the wrong maxHint section");               \
    const nsStyle##struct_* this##struct_ =                                   \
        static_cast<const nsStyle##struct_*>(                                 \
                       PeekStyleData(eStyleStruct_##struct_));                \
    if (this##struct_) {                                                      \
      const nsStyle##struct_* other##struct_ = aOther->GetStyle##struct_();   \
      if (compare &&                                                          \
          !NS_IsHintSubset(maxHint, hint) &&                                  \
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

#ifdef MOZ_SVG
  maxHint = nsChangeHint(NS_STYLE_HINT_REFLOW | nsChangeHint_UpdateEffects);
  DO_STRUCT_DIFFERENCE(SVGReset);
  DO_STRUCT_DIFFERENCE(SVG);
#endif

  
  
  maxHint = NS_STYLE_HINT_REFLOW;
      
  
  
  
  DO_STRUCT_DIFFERENCE(Font);
  DO_STRUCT_DIFFERENCE(Margin);
  DO_STRUCT_DIFFERENCE(Padding);
  DO_STRUCT_DIFFERENCE(Border);
  DO_STRUCT_DIFFERENCE(Position);
  DO_STRUCT_DIFFERENCE(TextReset);

  
  
  maxHint = NS_STYLE_HINT_VISUAL;

  
  
  DO_STRUCT_DIFFERENCE(Color);
  DO_STRUCT_DIFFERENCE(Background);

#undef DO_STRUCT_DIFFERENCE

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
void nsStyleContext::List(FILE* out, PRInt32 aIndent)
{
  
  PRInt32 ix;
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

  if (nsnull != mChild) {
    nsStyleContext* child = mChild;
    do {
      child->List(out, aIndent + 1);
      child = child->mNextSibling;
    } while (mChild != child);
  }
  if (nsnull != mEmptyChild) {
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
  
  return aPresContext->AllocateFromShell(sz);
}



void 
nsStyleContext::Destroy()
{
  
  nsCOMPtr<nsPresContext> presContext = mRuleNode->GetPresContext();

  
  this->~nsStyleContext();

  
  
  presContext->FreeToShell(sizeof(nsStyleContext), this);
}

already_AddRefed<nsStyleContext>
NS_NewStyleContext(nsStyleContext* aParentContext,
                   nsIAtom* aPseudoTag,
                   nsRuleNode* aRuleNode,
                   nsPresContext* aPresContext)
{
  nsStyleContext* context = new (aPresContext) nsStyleContext(aParentContext, aPseudoTag, 
                                                              aRuleNode, aPresContext);
  if (context)
    context->AddRef();
  return context;
}

