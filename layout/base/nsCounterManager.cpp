







































#include "nsCounterManager.h"

#include "nsIFrame.h"
#include "nsBulletFrame.h" 
#include "nsContentUtils.h"
#include "nsCSSPseudoElements.h"



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
        mValueAfter = aList->ValueBefore(this) + mChangeValue;
    }
}


void
nsCounterUseNode::GetText(nsString& aResult)
{
    aResult.Truncate();

    nsAutoVoidArray stack;
    stack.AppendElement(static_cast<nsCounterNode*>(this));

    if (mAllCounters && mScopeStart)
        for (nsCounterNode *n = mScopeStart; n->mScopePrev; n = n->mScopeStart)
            stack.AppendElement(n->mScopePrev);

    PRInt32 style = mCounterStyle->Item(mAllCounters ? 2 : 1).GetIntValue();
    const PRUnichar* separator;
    if (mAllCounters)
        separator = mCounterStyle->Item(1).GetStringBufferValue();

    for (PRInt32 i = stack.Count() - 1;; --i) {
        nsCounterNode *n = static_cast<nsCounterNode*>(stack[i]);
        nsBulletFrame::AppendCounterText(style, n->mValueAfter, aResult);
        if (i == 0)
            break;
        NS_ASSERTION(mAllCounters, "yikes, separator is uninitialized");
        aResult.Append(separator);
    }
}




static nsIContent *
GetScopeContent(nsCounterNode *aNode)
{
  nsIContent *content = aNode->mParentContent;
  if (!aNode->mPseudoType) {
    content = content->GetParent();
  }
  return content;
}

void
nsCounterList::SetScope(nsCounterNode *aNode)
{
    
    
    
    
    
    
    
    
    

    if (aNode == First()) {
        aNode->mScopeStart = nsnull;
        aNode->mScopePrev = nsnull;
        return;
    }

    nsIContent *nodeContent = GetScopeContent(aNode);

    for (nsCounterNode *prev = Prev(aNode), *start;
         prev; prev = start->mScopePrev) {
        
        
        
        
        
        start = (prev->mType == nsCounterNode::RESET || !prev->mScopeStart)
                  ? prev : prev->mScopeStart;

        
        nsIContent *startContent = GetScopeContent(start);
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

    aNode->mScopeStart = nsnull;
    aNode->mScopePrev  = nsnull;
}

void
nsCounterList::RecalcAll()
{
    mDirty = PR_FALSE;

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
{
    mNames.Init(16);
}

PRBool
nsCounterManager::AddCounterResetsAndIncrements(nsIFrame *aFrame)
{
    nsStyleContext *styleContext = aFrame->GetStyleContext();
    const nsStyleContent *styleContent = styleContext->GetStyleContent();
    if (!styleContent->CounterIncrementCount() &&
        !styleContent->CounterResetCount())
        return PR_FALSE;

    nsIContent *content = aFrame->GetContent();
    nsIAtom *pseudo =
      nsGenConNode::ToGeneratedContentType(styleContext->GetPseudoType());
    if (pseudo) {
        
        
        
        NS_ASSERTION(content->Tag() ==
                     (pseudo == nsCSSPseudoElements::before
                         ? nsGkAtoms::mozgeneratedcontentbefore 
                         : nsGkAtoms::mozgeneratedcontentafter),
                     "Not a generated content node");
        
        content = content->GetParent();
    }

    
    
    PRInt32 i, i_end;
    PRBool dirty = PR_FALSE;
    for (i = 0, i_end = styleContent->CounterResetCount(); i != i_end; ++i)
        dirty |= AddResetOrIncrement(content, styleContext, i,
                                     styleContent->GetCounterResetAt(i),
                                     nsCounterChangeNode::RESET);
    for (i = 0, i_end = styleContent->CounterIncrementCount(); i != i_end; ++i)
        dirty |= AddResetOrIncrement(content, styleContext, i,
                                     styleContent->GetCounterIncrementAt(i),
                                     nsCounterChangeNode::INCREMENT);
    return dirty;
}

PRBool
nsCounterManager::AddResetOrIncrement(nsIContent *aContent,
                                      nsStyleContext *aStyleContext,
                                      PRInt32 aIndex,
                                      const nsStyleCounterData *aCounterData,
                                      nsCounterNode::Type aType)
{
    nsCounterChangeNode *node =
        new nsCounterChangeNode(aContent, aStyleContext, aType,
                                aCounterData->mValue, aIndex);
    if (!node)
        return PR_FALSE;

    nsCounterList *counterList = CounterListFor(aCounterData->mCounter);
    if (!counterList) {
        NS_NOTREACHED("CounterListFor failed (should only happen on OOM)");
        return PR_FALSE;
    }

    counterList->Insert(node);
    if (!counterList->IsLast(node)) {
        
        
        counterList->SetDirty();
        return PR_TRUE;
    }

    
    
    if (NS_LIKELY(!counterList->IsDirty())) {
        node->Calc(counterList);
    }
    return PR_FALSE;
}

nsCounterList*
nsCounterManager::CounterListFor(const nsSubstring& aCounterName)
{
    
    
    nsCounterList *counterList;
    if (!mNames.Get(aCounterName, &counterList)) {
        counterList = new nsCounterList();
        if (!counterList)
            return nsnull;
        if (!mNames.Put(aCounterName, counterList)) {
            delete counterList;
            return nsnull;
        }
    }
    return counterList;
}

PR_STATIC_CALLBACK(PLDHashOperator)
RecalcDirtyLists(const nsAString& aKey, nsCounterList* aList, void* aClosure)
{
    if (aList->IsDirty())
        aList->RecalcAll();
    return PL_DHASH_NEXT;
}

void
nsCounterManager::RecalcAll()
{
    mNames.EnumerateRead(RecalcDirtyLists, nsnull);
}

struct DestroyNodesData {
    DestroyNodesData(nsIContent* aParentContent, nsIAtom *aPseudo)
        : mParentContent(aParentContent)
        , mPseudo(aPseudo)
        , mDestroyedAny(PR_FALSE)
    {
    }

    nsIContent *mParentContent;
    nsIAtom *mPseudo;
    PRBool mDestroyedAny;
};

PR_STATIC_CALLBACK(PLDHashOperator)
DestroyNodesInList(const nsAString& aKey, nsCounterList* aList, void* aClosure)
{
    DestroyNodesData *data = static_cast<DestroyNodesData*>(aClosure);
    if (aList->DestroyNodesFor(data->mParentContent, data->mPseudo)) {
        data->mDestroyedAny = PR_TRUE;
        aList->SetDirty();
    }
    return PL_DHASH_NEXT;
}

PRBool
nsCounterManager::DestroyNodesFor(nsIContent* aParentContent, nsIAtom *aPseudo)
{
    DestroyNodesData data(aParentContent, aPseudo);
    mNames.EnumerateRead(DestroyNodesInList, &data);
    return data.mDestroyedAny;
}

#ifdef DEBUG
PR_STATIC_CALLBACK(PLDHashOperator)
DumpList(const nsAString& aKey, nsCounterList* aList, void* aClosure)
{
    printf("Counter named \"%s\":\n", NS_ConvertUTF16toUTF8(aKey).get());
    nsCounterNode *node = aList->First();

    if (node) {
        PRInt32 i = 0;
        do {
            nsCAutoString pseudo;
            if (node->mPseudoType) {
              node->mPseudoType->ToUTF8String(pseudo);
            }
            const char *types[] = { "RESET", "INCREMENT", "USE" };
            printf("  Node #%d @%p content=%p pseudo=%s index=%d type=%s valAfter=%d\n"
                   "       scope-start=%p scope-prev=%p",
                   i++, (void*)node, (void*)node->mParentContent, pseudo.get(),
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
    mNames.EnumerateRead(DumpList, nsnull);
    printf("\n\n");
}
#endif
