







#include "nsCounterManager.h"
#include "nsBulletFrame.h" 
#include "nsContentUtils.h"
#include "nsTArray.h"
#include "mozilla/Likely.h"
#include "nsIContent.h"
#include "WritingModes.h"

using namespace mozilla;

bool
nsCounterUseNode::InitTextFrame(nsGenConList* aList,
        nsIFrame* aPseudoFrame, nsIFrame* aTextFrame)
{
  nsCounterNode::InitTextFrame(aList, aPseudoFrame, aTextFrame);

  nsCounterList *counterList = static_cast<nsCounterList*>(aList);
  counterList->Insert(this);
  bool dirty = counterList->IsDirty();
  if (!dirty) {
    if (counterList->IsLast(this)) {
      Calc(counterList);
      nsAutoString contentString;
      GetText(contentString);
      aTextFrame->GetContent()->SetText(contentString, false);
    } else {
      
      
      
      counterList->SetDirty();
      return true;
    }
  }
  
  return false;
}

CounterStyle*
nsCounterUseNode::GetCounterStyle()
{
    if (!mCounterStyle) {
        const nsCSSValue& style = mCounterFunction->Item(mAllCounters ? 2 : 1);
        mCounterStyle = mPresContext->CounterStyleManager()->
            BuildCounterStyle(nsDependentString(style.GetStringBufferValue()));
    }
    return mCounterStyle;
}



void nsCounterUseNode::Calc(nsCounterList *aList)
{
    NS_ASSERTION(!aList->IsDirty(),
                 "Why are we calculating with a dirty list?");
    mValueAfter = aList->ValueBefore(this);
}



void nsCounterChangeNode::Calc(nsCounterList *aList)
{
    NS_ASSERTION(!aList->IsDirty(),
                 "Why are we calculating with a dirty list?");
    if (mType == RESET) {
        mValueAfter = mChangeValue;
    } else {
        NS_ASSERTION(mType == INCREMENT, "invalid type");
        mValueAfter = nsCounterManager::IncrementCounter(aList->ValueBefore(this),
                                                         mChangeValue);
    }
}


void
nsCounterUseNode::GetText(nsString& aResult)
{
    aResult.Truncate();

    nsAutoTArray<nsCounterNode*, 8> stack;
    stack.AppendElement(static_cast<nsCounterNode*>(this));

    if (mAllCounters && mScopeStart)
        for (nsCounterNode *n = mScopeStart; n->mScopePrev; n = n->mScopeStart)
            stack.AppendElement(n->mScopePrev);

    const char16_t* separator;
    if (mAllCounters)
        separator = mCounterFunction->Item(1).GetStringBufferValue();

    CounterStyle* style = GetCounterStyle();
    WritingMode wm = mPseudoFrame ?
        mPseudoFrame->GetWritingMode() : WritingMode();
    for (uint32_t i = stack.Length() - 1;; --i) {
        nsCounterNode *n = stack[i];
        nsAutoString text;
        bool isTextRTL;
        style->GetCounterText(n->mValueAfter, wm, text, isTextRTL);
        aResult.Append(text);
        if (i == 0)
            break;
        NS_ASSERTION(mAllCounters, "yikes, separator is uninitialized");
        aResult.Append(separator);
    }
}

void
nsCounterList::SetScope(nsCounterNode *aNode)
{
    
    
    
    
    
    
    
    
    

    if (aNode == First()) {
        aNode->mScopeStart = nullptr;
        aNode->mScopePrev = nullptr;
        return;
    }

    
    
    
    nsIContent *nodeContent = aNode->mPseudoFrame->GetContent()->GetParent();

    for (nsCounterNode *prev = Prev(aNode), *start;
         prev; prev = start->mScopePrev) {
        
        
        
        
        
        start = (prev->mType == nsCounterNode::RESET || !prev->mScopeStart)
                  ? prev : prev->mScopeStart;

        
        nsIContent *startContent = start->mPseudoFrame->GetContent()->GetParent();
        NS_ASSERTION(nodeContent || !startContent,
                     "null check on startContent should be sufficient to "
                     "null check nodeContent as well, since if nodeContent "
                     "is for the root, startContent (which is before it) "
                     "must be too");

             
        if (!(aNode->mType == nsCounterNode::RESET &&
              nodeContent == startContent) &&
              
              
            (!startContent ||
             nsContentUtils::ContentIsDescendantOf(nodeContent,
                                                   startContent))) {
            aNode->mScopeStart = start;
            aNode->mScopePrev  = prev;
            return;
        }
    }

    aNode->mScopeStart = nullptr;
    aNode->mScopePrev  = nullptr;
}

void
nsCounterList::RecalcAll()
{
    mDirty = false;

    nsCounterNode *node = First();
    if (!node)
        return;

    do {
        SetScope(node);
        node->Calc(this);

        if (node->mType == nsCounterNode::USE) {
            nsCounterUseNode *useNode = node->UseNode();
            
            
            
            if (useNode->mText) {
                nsAutoString text;
                useNode->GetText(text);
                useNode->mText->SetData(text);
            }
        }
    } while ((node = Next(node)) != First());
}

nsCounterManager::nsCounterManager()
    : mNames()
{
}

bool
nsCounterManager::AddCounterResetsAndIncrements(nsIFrame *aFrame)
{
    const nsStyleContent *styleContent = aFrame->StyleContent();
    if (!styleContent->CounterIncrementCount() &&
        !styleContent->CounterResetCount())
        return false;

    
    
    int32_t i, i_end;
    bool dirty = false;
    for (i = 0, i_end = styleContent->CounterResetCount(); i != i_end; ++i)
        dirty |= AddResetOrIncrement(aFrame, i,
                                     styleContent->GetCounterResetAt(i),
                                     nsCounterChangeNode::RESET);
    for (i = 0, i_end = styleContent->CounterIncrementCount(); i != i_end; ++i)
        dirty |= AddResetOrIncrement(aFrame, i,
                                     styleContent->GetCounterIncrementAt(i),
                                     nsCounterChangeNode::INCREMENT);
    return dirty;
}

bool
nsCounterManager::AddResetOrIncrement(nsIFrame *aFrame, int32_t aIndex,
                                      const nsStyleCounterData *aCounterData,
                                      nsCounterNode::Type aType)
{
    nsCounterChangeNode *node =
        new nsCounterChangeNode(aFrame, aType, aCounterData->mValue, aIndex);

    nsCounterList *counterList = CounterListFor(aCounterData->mCounter);
    if (!counterList) {
        NS_NOTREACHED("CounterListFor failed (should only happen on OOM)");
        return false;
    }

    counterList->Insert(node);
    if (!counterList->IsLast(node)) {
        
        
        counterList->SetDirty();
        return true;
    }

    
    
    if (MOZ_LIKELY(!counterList->IsDirty())) {
        node->Calc(counterList);
    }
    return false;
}

nsCounterList*
nsCounterManager::CounterListFor(const nsSubstring& aCounterName)
{
    
    
    nsCounterList *counterList;
    if (!mNames.Get(aCounterName, &counterList)) {
        counterList = new nsCounterList();
        mNames.Put(aCounterName, counterList);
    }
    return counterList;
}

static PLDHashOperator
RecalcDirtyLists(const nsAString& aKey, nsCounterList* aList, void* aClosure)
{
    if (aList->IsDirty())
        aList->RecalcAll();
    return PL_DHASH_NEXT;
}

void
nsCounterManager::RecalcAll()
{
    mNames.EnumerateRead(RecalcDirtyLists, nullptr);
}

static PLDHashOperator
SetCounterStylesDirty(const nsAString& aKey,
                      nsCounterList* aList,
                      void* aClosure)
{
    nsCounterNode* first = aList->First();
    if (first) {
        bool changed = false;
        nsCounterNode* node = first;
        do {
            if (node->mType == nsCounterNode::USE) {
                node->UseNode()->SetCounterStyleDirty();
                changed = true;
            }
        } while ((node = aList->Next(node)) != first);
        if (changed) {
            aList->SetDirty();
        }
    }
    return PL_DHASH_NEXT;
}

void
nsCounterManager::SetAllCounterStylesDirty()
{
    mNames.EnumerateRead(SetCounterStylesDirty, nullptr);
}

struct DestroyNodesData {
    explicit DestroyNodesData(nsIFrame *aFrame)
        : mFrame(aFrame)
        , mDestroyedAny(false)
    {
    }

    nsIFrame *mFrame;
    bool mDestroyedAny;
};

static PLDHashOperator
DestroyNodesInList(const nsAString& aKey, nsCounterList* aList, void* aClosure)
{
    DestroyNodesData *data = static_cast<DestroyNodesData*>(aClosure);
    if (aList->DestroyNodesFor(data->mFrame)) {
        data->mDestroyedAny = true;
        aList->SetDirty();
    }
    return PL_DHASH_NEXT;
}

bool
nsCounterManager::DestroyNodesFor(nsIFrame *aFrame)
{
    DestroyNodesData data(aFrame);
    mNames.EnumerateRead(DestroyNodesInList, &data);
    return data.mDestroyedAny;
}

#ifdef DEBUG
static PLDHashOperator
DumpList(const nsAString& aKey, nsCounterList* aList, void* aClosure)
{
    printf("Counter named \"%s\":\n", NS_ConvertUTF16toUTF8(aKey).get());
    nsCounterNode *node = aList->First();

    if (node) {
        int32_t i = 0;
        do {
            const char *types[] = { "RESET", "INCREMENT", "USE" };
            printf("  Node #%d @%p frame=%p index=%d type=%s valAfter=%d\n"
                   "       scope-start=%p scope-prev=%p",
                   i++, (void*)node, (void*)node->mPseudoFrame,
                   node->mContentIndex, types[node->mType], node->mValueAfter,
                   (void*)node->mScopeStart, (void*)node->mScopePrev);
            if (node->mType == nsCounterNode::USE) {
                nsAutoString text;
                node->UseNode()->GetText(text);
                printf(" text=%s", NS_ConvertUTF16toUTF8(text).get());
            }
            printf("\n");
        } while ((node = aList->Next(node)) != aList->First());
    }
    return PL_DHASH_NEXT;
}

void
nsCounterManager::Dump()
{
    printf("\n\nCounter Manager Lists:\n");
    mNames.EnumerateRead(DumpList, nullptr);
    printf("\n\n");
}
#endif
