





































 


#include "nsStyleConsts.h"
#include "nsString.h"
#include "nsPresContext.h"
#include "nsIStyleRule.h"
#include "nsCRT.h"

#include "nsCOMPtr.h"
#include "nsStyleSet.h"
#include "nsIPresShell.h"
#include "prenv.h"

#include "nsRuleNode.h"
#include "nsUnitConversion.h"
#include "nsStyleContext.h"
#include "imgIRequest.h"

#include "nsPrintfCString.h"

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


PRBool nsStyleContext::Equals(const nsStyleContext* aOther) const
{
  PRBool  result = PR_TRUE;
  const nsStyleContext* other = (nsStyleContext*)aOther;

  if (other != this) {
    if (mParent != other->mParent) {
      result = PR_FALSE;
    }
    else if (mBits != other->mBits) {
      result = PR_FALSE;
    }
    else if (mPseudoTag != other->mPseudoTag) {
      result = PR_FALSE;
    }
    else if (mRuleNode != other->mRuleNode) {
      result = PR_FALSE;
    }
  }
  return result;
}



const nsStyleStruct* nsStyleContext::GetStyleData(nsStyleStructID aSID)
{
  const nsStyleStruct* cachedData = mCachedStyleData.GetStyleData(aSID); 
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

inline const nsStyleStruct* nsStyleContext::PeekStyleData(nsStyleStructID aSID)
{
  const nsStyleStruct* cachedData = mCachedStyleData.GetStyleData(aSID); 
  if (cachedData)
    return cachedData; 
  return mRuleNode->GetStyleData(aSID, this, PR_FALSE); 
}




nsStyleStruct* 
nsStyleContext::GetUniqueStyleData(const nsStyleStructID& aSID)
{
  
  
  
  
  
  const nsStyleStruct *current = GetStyleData(aSID);
  if (!mChild && !mEmptyChild &&
      !(mBits & nsCachedStyleData::GetBitForSID(aSID)) &&
      mCachedStyleData.GetStyleData(aSID))
    return NS_CONST_CAST(nsStyleStruct*, current);

  nsStyleStruct* result;
  nsPresContext *presContext = PresContext();
  switch (aSID) {

#define UNIQUE_CASE(c_)                                                       \
  case eStyleStruct_##c_:                                                     \
    result = new (presContext) nsStyle##c_(                                   \
      * NS_STATIC_CAST(const nsStyle##c_ *, current));                        \
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
    NS_WARNING("Ran out of memory while trying to allocate memory for a unique nsStyleStruct! "
               "Returning the non-unique data.");
    return NS_CONST_CAST(nsStyleStruct*, current);
  }

  SetStyle(aSID, result);
  mBits &= ~nsCachedStyleData::GetBitForSID(aSID);

  return result;
}

void
nsStyleContext::SetStyle(nsStyleStructID aSID, nsStyleStruct* aStruct)
{
  
  
  
  NS_ASSERTION(aSID >= 0 && aSID < nsStyleStructID_Length, "out of bounds");

  
  
  

  const nsCachedStyleData::StyleStructInfo& info =
      nsCachedStyleData::gInfo[aSID];
  char* resetOrInheritSlot = NS_REINTERPRET_CAST(char*, &mCachedStyleData) +
                             info.mCachedStyleDataOffset;
  char* resetOrInherit = NS_REINTERPRET_CAST(char*,
      *NS_REINTERPRET_CAST(void**, resetOrInheritSlot));
  if (!resetOrInherit) {
    nsPresContext *presContext = mRuleNode->GetPresContext();
    if (mCachedStyleData.IsReset(aSID)) {
      mCachedStyleData.mResetData = new (presContext) nsResetStyleData;
      resetOrInherit = NS_REINTERPRET_CAST(char*, mCachedStyleData.mResetData);
    } else {
      mCachedStyleData.mInheritedData =
          new (presContext) nsInheritedStyleData;
      resetOrInherit =
          NS_REINTERPRET_CAST(char*, mCachedStyleData.mInheritedData);
    }
  }
  char* dataSlot = resetOrInherit + info.mInheritResetOffset;
  *NS_REINTERPRET_CAST(nsStyleStruct**, dataSlot) = aStruct;
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
      nsStyleDisplay *mutable_display = NS_STATIC_CAST(nsStyleDisplay*,
        GetUniqueStyleData(eStyleStruct_Display));
      if (mutable_display->mDisplay == NS_STYLE_DISPLAY_INLINE_TABLE)
        mutable_display->mDisplay = NS_STYLE_DISPLAY_TABLE;
      else
        mutable_display->mDisplay = NS_STYLE_DISPLAY_BLOCK;
    }
  }

  
  GetStyleUserInterface();
}

void
nsStyleContext::ClearStyleData(nsPresContext* aPresContext)
{
  
  if (mCachedStyleData.mResetData || mCachedStyleData.mInheritedData)
    mCachedStyleData.Destroy(mBits, aPresContext);

  mBits = 0; 

  ApplyStyleFixups(aPresContext);

  if (mChild) {
    nsStyleContext* child = mChild;
    do {
      child->ClearStyleData(aPresContext);
      child = child->mNextSibling;
    } while (mChild != child);
  }
  
  if (mEmptyChild) {
    nsStyleContext* child = mEmptyChild;
    do {
      child->ClearStyleData(aPresContext);
      child = child->mNextSibling;
    } while (mEmptyChild != child);
  }
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
        NS_STATIC_CAST(const nsStyle##struct_*,                               \
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
  DO_STRUCT_DIFFERENCE(List);
  
  
  DO_STRUCT_DIFFERENCE(Quotes);

  
  
  maxHint = NS_STYLE_HINT_REFLOW;
      
  
  
  
  DO_STRUCT_DIFFERENCE(Font);
  DO_STRUCT_DIFFERENCE(Margin);
  DO_STRUCT_DIFFERENCE(Padding);
  DO_STRUCT_DIFFERENCE(Border);
  DO_STRUCT_DIFFERENCE(Position);
  DO_STRUCT_DIFFERENCE(Text);
  DO_STRUCT_DIFFERENCE(TextReset);

  
  
  maxHint = NS_STYLE_HINT_VISUAL;

  
  
  DO_STRUCT_DIFFERENCE(Color);
  DO_STRUCT_DIFFERENCE(Background);
#ifdef MOZ_SVG
  DO_STRUCT_DIFFERENCE(SVG);
#endif

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

class URICString : public nsCAutoString {
public:
  URICString(nsIURI* aURI) {
    if (aURI) {
      aURI->GetSpec(*this);
    } else {
      Assign("[none]");
    }
  }

  URICString(imgIRequest* aImageRequest) {
    nsCOMPtr<nsIURI> uri;
    if (aImageRequest) {
      aImageRequest->GetURI(getter_AddRefs(uri));
    }
    if (uri) {
      uri->GetSpec(*this);
    } else {
      Assign("[none]");
    }
  }
  
  URICString& operator=(const URICString& aOther) {
    Assign(aOther);
    return *this;
  }
};

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

static void IndentBy(FILE* out, PRInt32 aIndent) {
  while (--aIndent >= 0) fputs("  ", out);
}

void nsStyleContext::DumpRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent)
{
  nsAutoString str;

  
  IndentBy(out,aIndent);
  const nsStyleFont* font = GetStyleFont();
  fprintf(out, "<font %s %d %d %d />\n", 
          NS_ConvertUTF16toUTF8(font->mFont.name).get(),
          font->mFont.size,
          font->mSize,
          font->mFlags);

  
  IndentBy(out,aIndent);
  const nsStyleColor* color = GetStyleColor();
  fprintf(out, "<color data=\"%ld\"/>\n", 
    (long)color->mColor);

  
  IndentBy(out,aIndent);
  const nsStyleBackground* bg = GetStyleBackground();
  fprintf(out, "<background data=\"%d %d %d %ld %ld %ld %s\"/>\n",
    (int)bg->mBackgroundAttachment,
    (int)bg->mBackgroundFlags,
    (int)bg->mBackgroundRepeat,
    (long)bg->mBackgroundColor,
    
    (long)bg->mBackgroundXPosition.mCoord, 
    (long)bg->mBackgroundYPosition.mCoord, 
    URICString(bg->mBackgroundImage).get());
 
  
  IndentBy(out,aIndent);
  fprintf(out, "<spacing data=\"");

  const nsStyleMargin* margin = GetStyleMargin();
  margin->mMargin.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  
  const nsStylePadding* padding = GetStylePadding();
  padding->mPadding.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  
  const nsStyleBorder* border = GetStyleBorder();
#ifdef NS_COORD_IS_FLOAT
  const char format [] = "top: %ftw right: %ftw bottom: %ftw left: %ftw";
#else
  const char format [] = "top: %dtw right: %dtw bottom: %dtw left: %dtw";
#endif
  nsPrintfCString output(format,
                         border->GetBorderWidth(NS_SIDE_TOP),
                         border->GetBorderWidth(NS_SIDE_RIGHT),
                         border->GetBorderWidth(NS_SIDE_BOTTOM),
                         border->GetBorderWidth(NS_SIDE_LEFT));
  fprintf(out, "%s ", output.get());
  border->mBorderRadius.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  
  const nsStyleOutline* outline = GetStyleOutline();
  outline->mOutlineRadius.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  outline->mOutlineWidth.ToString(str);
  fprintf(out, "%s", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "%d", (int)border->mFloatEdge);
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleList* list = GetStyleList();
  fprintf(out, "<list data=\"%d %d %s\" />\n",
    (int)list->mListStyleType,
    (int)list->mListStyleType,
    URICString(list->mListStyleImage).get());

  
  IndentBy(out,aIndent);
  const nsStylePosition* pos = GetStylePosition();
  fprintf(out, "<position data=\"");
  pos->mOffset.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  pos->mWidth.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  pos->mMinWidth.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  pos->mMaxWidth.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  pos->mHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  pos->mMinHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  pos->mMaxHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "%d ", (int)pos->mBoxSizing);
  pos->mZIndex.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleText* text = GetStyleText();
  fprintf(out, "<text data=\"%d %d %d ",
    (int)text->mTextAlign,
    (int)text->mTextTransform,
    (int)text->mWhiteSpace);
  text->mLetterSpacing.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  text->mLineHeight.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  text->mTextIndent.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  text->mWordSpacing.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "\" />\n");
  
  
  IndentBy(out,aIndent);
  const nsStyleTextReset* textReset = GetStyleTextReset();
  fprintf(out, "<textreset data=\"%d ",
    (int)textReset->mTextDecoration);
  textReset->mVerticalAlign.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleDisplay* disp = GetStyleDisplay();
  fprintf(out, "<display data=\"%d %d %f %d %d %d %d %d %d %d %ld %ld %ld %ld %s\" />\n",
    (int)disp->mPosition,
    (int)disp->mDisplay,
    (float)disp->mOpacity,      
    (int)disp->mFloats,
    (int)disp->mBreakType,
    (int)disp->mBreakBefore,
    (int)disp->mBreakAfter,
    (int)disp->mOverflowX,
    (int)disp->mOverflowY,
    (int)disp->mClipFlags,
    (long)disp->mClip.x,
    (long)disp->mClip.y,
    (long)disp->mClip.width,
    (long)disp->mClip.height,
    URICString(disp->mBinding).get()
    );
  
  
  IndentBy(out,aIndent);
  const nsStyleVisibility* vis = GetStyleVisibility();
  fprintf(out, "<visibility data=\"%d %d\" />\n",
    (int)vis->mDirection,
    (int)vis->mVisible
    );

  
  IndentBy(out,aIndent);
  const nsStyleTable* table = GetStyleTable();
  fprintf(out, "<table data=\"%d %d %d ",
    (int)table->mLayoutStrategy,
    (int)table->mFrame,
    (int)table->mRules);
  fprintf(out, "%ld %ld ",
    (long)table->mCols,
    (long)table->mSpan);
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleTableBorder* tableBorder = GetStyleTableBorder();
  fprintf(out, "<tableborder data=\"%d ",
    (int)tableBorder->mBorderCollapse);
  tableBorder->mBorderSpacingX.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  tableBorder->mBorderSpacingY.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "%d %d ",
    (int)tableBorder->mCaptionSide,
    (int)tableBorder->mEmptyCells);
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleContent* content = GetStyleContent();
  fprintf(out, "<content data=\"%ld %ld %ld ",
    (long)content->ContentCount(),
    (long)content->CounterIncrementCount(),
    (long)content->CounterResetCount());
  
  content->mMarkerOffset.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleQuotes* quotes = GetStyleQuotes();
  fprintf(out, "<quotes data=\"%ld ",
    (long)quotes->QuotesCount());
  
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleUserInterface* ui = GetStyleUserInterface();
  fprintf(out, "<ui data=\"%d %d %d %d\" />\n",
    (int)ui->mUserInput,
    (int)ui->mUserModify,
    (int)ui->mUserFocus, 
    (int)ui->mCursor);

  
  IndentBy(out,aIndent);
  const nsStyleUIReset* uiReset = GetStyleUIReset();
  fprintf(out, "<uireset data=\"%d\" />\n",
    (int)uiReset->mUserSelect);

  
  IndentBy(out,aIndent);
  const nsStyleColumn* column = GetStyleColumn();
  fprintf(out, "<column data=\"%d ",
    (int)column->mColumnCount);
  column->mColumnWidth.ToString(str);
  fprintf(out, "%s ", NS_ConvertUTF16toUTF8(str).get());
  column->mColumnGap.ToString(str);
  fprintf(out, "%s", NS_ConvertUTF16toUTF8(str).get());
  fprintf(out, "\" />\n");

  
  IndentBy(out,aIndent);
  const nsStyleXUL* xul = GetStyleXUL();
  fprintf(out, "<xul data=\"%d %d %d %d %d %d",
    (int)xul->mBoxAlign,
    (int)xul->mBoxDirection,
    (int)xul->mBoxFlex,
    (int)xul->mBoxOrient,
    (int)xul->mBoxPack,
    (int)xul->mBoxOrdinal);
  fprintf(out, "\" />\n");

#ifdef MOZ_SVG
  
  IndentBy(out,aIndent);
  const nsStyleSVG* svg = GetStyleSVG();
  fprintf(out, "<svg data=\"%d ",(int)svg->mFill.mType);
  if (svg->mFill.mType == eStyleSVGPaintType_Server)
    fprintf(out, "%s %ld ", URICString(svg->mFill.mPaint.mPaintServer).get(),
                            (long)svg->mFill.mFallbackColor);
  else
    fprintf(out, "%ld ", (long)svg->mFill.mPaint.mColor);

  fprintf(out, "%d ", (int)svg->mStroke.mType);
  if (svg->mStroke.mType == eStyleSVGPaintType_Server)
    fprintf(out, "%s %ld ", URICString(svg->mStroke.mPaint.mPaintServer).get(),
                            (long)svg->mStroke.mFallbackColor);
  else
    fprintf(out, "%ld ", (long)svg->mStroke.mPaint.mColor);

  fprintf(out, "%s %s %s ",
          URICString(svg->mMarkerEnd).get(),
          URICString(svg->mMarkerMid).get(),
          URICString(svg->mMarkerStart).get());

  for (PRUint32 i = 0; i < svg->mStrokeDasharrayLength; i++) {
    svg->mStrokeDasharray[i].ToString(str);
    fprintf(out,
            "%s%c",
            NS_ConvertUTF16toUTF8(str).get(),
            (i == svg->mStrokeDasharrayLength) ? ' ' : ',');
  }

  svg->mStrokeDashoffset.ToString(str);
  fprintf(out, "%f %s %f %f ",
          svg->mFillOpacity,
          NS_ConvertUTF16toUTF8(str).get(),
          svg->mStrokeMiterlimit,
          svg->mStrokeOpacity);
  svg->mStrokeWidth.ToString(str);
  fprintf(out, "%s %d %d %d %d %d %d %d %d %d %d %d\" />\n",
          NS_ConvertUTF16toUTF8(str).get(),
          (int)svg->mStrokeDasharrayLength,
          (int)svg->mClipRule,
          (int)svg->mColorInterpolation,
          (int)svg->mColorInterpolationFilters,
          (int)svg->mFillRule,
          (int)svg->mPointerEvents,
          (int)svg->mShapeRendering,
          (int)svg->mStrokeLinecap,
          (int)svg->mStrokeLinejoin,
          (int)svg->mTextAnchor,
          (int)svg->mTextRendering);

  
  IndentBy(out,aIndent);
  const nsStyleSVGReset* svgReset = GetStyleSVGReset();

  fprintf(out, "<svgreset data=\"%ld ", (long)svgReset->mStopColor);

  fprintf(out, "%ld ", (long)svgReset->mFloodColor);

  fprintf(out, "%s %s %s %f %f %d\" />\n",
          URICString(svgReset->mClipPath).get(),
          URICString(svgReset->mFilter).get(),
          URICString(svgReset->mMask).get(),
          svgReset->mStopOpacity,
          svgReset->mFloodOpacity,
          (int)svgReset->mDominantBaseline);
#endif
  
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

