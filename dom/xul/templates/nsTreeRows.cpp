




#include "nsString.h"
#include "nsTreeRows.h"
#include <algorithm>

nsTreeRows::Subtree*
nsTreeRows::EnsureSubtreeFor(Subtree* aParent,
                             int32_t aChildIndex)
{
    Subtree* subtree = GetSubtreeFor(aParent, aChildIndex);

    if (! subtree) {
        subtree = aParent->mRows[aChildIndex].mSubtree = new Subtree(aParent);
        InvalidateCachedRow();
    }

    return subtree;
}

nsTreeRows::Subtree*
nsTreeRows::GetSubtreeFor(const Subtree* aParent,
                              int32_t aChildIndex,
                              int32_t* aSubtreeSize)
{
    NS_PRECONDITION(aParent, "no parent");
    NS_PRECONDITION(aChildIndex >= 0, "bad child index");

    Subtree* result = nullptr;

    if (aChildIndex < aParent->mCount)
        result = aParent->mRows[aChildIndex].mSubtree;

    if (aSubtreeSize)
        *aSubtreeSize = result ? result->mSubtreeSize : 0;

    return result;
}

void
nsTreeRows::RemoveSubtreeFor(Subtree* aParent, int32_t aChildIndex)
{
    NS_PRECONDITION(aParent, "no parent");
    NS_PRECONDITION(aChildIndex >= 0 && aChildIndex < aParent->mCount, "bad child index");

    Row& row = aParent->mRows[aChildIndex];

    if (row.mSubtree) {
        int32_t subtreeSize = row.mSubtree->GetSubtreeSize();

        delete row.mSubtree;
        row.mSubtree = nullptr;

        for (Subtree* subtree = aParent; subtree != nullptr; subtree = subtree->mParent)
            subtree->mSubtreeSize -= subtreeSize;
    }

    InvalidateCachedRow();
}

nsTreeRows::iterator
nsTreeRows::First()
{
    iterator result;
    result.Append(&mRoot, 0);
    result.SetRowIndex(0);
    return result;
}

nsTreeRows::iterator
nsTreeRows::Last()
{
    iterator result;

    
    Subtree* current = &mRoot;
    int32_t count = current->Count();
    do  {
        int32_t last = count - 1;
        result.Append(current, last);
        current = count ? GetSubtreeFor(current, last) : nullptr;
    } while (current && ((count = current->Count()) != 0));

    
    result.GetTop().mChildIndex++;

    
    result.SetRowIndex(mRoot.GetSubtreeSize() + 1);

    return result;
}

nsTreeRows::iterator
nsTreeRows::operator[](int32_t aRow)
{
    
    
    
    int32_t last = mLastRow.GetRowIndex();
    if (last != -1) {
        if (aRow == last)
            return mLastRow;
        else if (last + 1 == aRow)
            return ++mLastRow;
        else if (last - 1 == aRow)
            return --mLastRow;
    }

    
    
    
    
    
    iterator result;
    Subtree* current = &mRoot;

    int32_t index = 0;
    result.SetRowIndex(aRow);

    do {
        int32_t subtreeSize;
        Subtree* subtree = GetSubtreeFor(current, index, &subtreeSize);

        if (subtreeSize >= aRow) {
            result.Append(current, index);
            current = subtree;
            index = 0;
            --aRow;
        }
        else {
            ++index;
            aRow -= subtreeSize + 1;
        }
    } while (aRow >= 0);

    mLastRow = result;
    return result;
}

nsTreeRows::iterator
nsTreeRows::FindByResource(nsIRDFResource* aResource)
{
    
    iterator last = Last();
    iterator iter;

    nsresult rv;
    nsAutoString resourceid;
    bool stringmode = false;

    for (iter = First(); iter != last; ++iter) {
        if (!stringmode) {
            nsCOMPtr<nsIRDFResource> findres;
            rv = iter->mMatch->mResult->GetResource(getter_AddRefs(findres));
            if (NS_FAILED(rv)) return last;

            if (findres == aResource)
                break;

            if (! findres) {
                const char *uri;
                aResource->GetValueConst(&uri);
                CopyUTF8toUTF16(uri, resourceid);

                
                stringmode = true;
            }
        }

        
        if (stringmode) {
            nsAutoString findid;
            rv = iter->mMatch->mResult->GetId(findid);
            if (NS_FAILED(rv)) return last;

            if (resourceid.Equals(findid))
                break;
        }
    }

    return iter;
}

nsTreeRows::iterator
nsTreeRows::Find(nsIXULTemplateResult *aResult)
{
    
    iterator last = Last();
    iterator iter;

    for (iter = First(); iter != last; ++iter) {
        if (aResult == iter->mMatch->mResult)
          break;
    }

    return iter;
}

void
nsTreeRows::Clear()
{
    mRoot.Clear();
    InvalidateCachedRow();
}






nsTreeRows::Subtree::~Subtree()
{
    Clear();
}

void
nsTreeRows::Subtree::Clear()
{
    for (int32_t i = mCount - 1; i >= 0; --i)
        delete mRows[i].mSubtree;

    delete[] mRows;

    mRows = nullptr;
    mCount = mCapacity = mSubtreeSize = 0;
}

nsTreeRows::iterator
nsTreeRows::Subtree::InsertRowAt(nsTemplateMatch* aMatch, int32_t aIndex)
{
    if (mCount >= mCapacity || aIndex >= mCapacity) {
        int32_t newCapacity = std::max(mCapacity * 2, aIndex + 1);
        Row* newRows = new Row[newCapacity];
        if (! newRows)
            return iterator();

        for (int32_t i = mCount - 1; i >= 0; --i)
            newRows[i] = mRows[i];

        delete[] mRows;

        mRows = newRows;
        mCapacity = newCapacity;
    }

    for (int32_t i = mCount - 1; i >= aIndex; --i)
        mRows[i + 1] = mRows[i];

    mRows[aIndex].mMatch = aMatch;
    mRows[aIndex].mContainerType = eContainerType_Unknown;
    mRows[aIndex].mContainerState = eContainerState_Unknown;
    mRows[aIndex].mContainerFill = eContainerFill_Unknown;
    mRows[aIndex].mSubtree = nullptr;
    ++mCount;

    
    int32_t rowIndex = 0;
    iterator result;
    result.Push(this, aIndex);

    for ( ; --aIndex >= 0; ++rowIndex) {
        
        const Subtree *subtree = mRows[aIndex].mSubtree;
        if (subtree)
            rowIndex += subtree->mSubtreeSize;
    }

    Subtree *subtree = this;
    do {
        
        ++subtree->mSubtreeSize;

        Subtree *parent = subtree->mParent;
        if (! parent)
            break;

        
        int32_t count = parent->Count();
        for (aIndex = 0; aIndex < count; ++aIndex, ++rowIndex) {
            const Subtree *child = (*parent)[aIndex].mSubtree;
            if (subtree == child)
                break;

            if (child)
                rowIndex += child->mSubtreeSize;
        }

        NS_ASSERTION(aIndex < count, "couldn't find subtree in parent");

        result.Push(parent, aIndex);
        subtree = parent;
        ++rowIndex; 
    } while (1);

    result.SetRowIndex(rowIndex);
    return result;
}

void
nsTreeRows::Subtree::RemoveRowAt(int32_t aIndex)
{
    NS_PRECONDITION(aIndex >= 0 && aIndex < Count(), "bad index");
    if (aIndex < 0 || aIndex >= Count())
        return;

    
    int32_t subtreeSize = mRows[aIndex].mSubtree
        ? mRows[aIndex].mSubtree->GetSubtreeSize()
        : 0;

    ++subtreeSize;

    delete mRows[aIndex].mSubtree;

    for (int32_t i = aIndex + 1; i < mCount; ++i)
        mRows[i - 1] = mRows[i];

    --mCount;

    for (Subtree* subtree = this; subtree != nullptr; subtree = subtree->mParent)
        subtree->mSubtreeSize -= subtreeSize;
}






nsTreeRows::iterator::iterator(const iterator& aIterator)
    : mRowIndex(aIterator.mRowIndex),
      mLink(aIterator.mLink)
{
}

nsTreeRows::iterator&
nsTreeRows::iterator::operator=(const iterator& aIterator)
{
    mRowIndex = aIterator.mRowIndex;
    mLink = aIterator.mLink;
    return *this;
}

void
nsTreeRows::iterator::Append(Subtree* aParent, int32_t aChildIndex)
{
    Link *link = mLink.AppendElement();
    if (link) {
        link->mParent     = aParent;
        link->mChildIndex = aChildIndex;
    }
    else
        NS_ERROR("out of memory");
}

void
nsTreeRows::iterator::Push(Subtree *aParent, int32_t aChildIndex)
{
    Link *link = mLink.InsertElementAt(0);
    if (link) {
        link->mParent     = aParent;
        link->mChildIndex = aChildIndex;
    }
    else
        NS_ERROR("out of memory");
}

bool
nsTreeRows::iterator::operator==(const iterator& aIterator) const
{
    if (GetDepth() != aIterator.GetDepth())
        return false;

    if (GetDepth() == 0)
        return true;

    return GetTop() == aIterator.GetTop();
}

void
nsTreeRows::iterator::Next()
{
    NS_PRECONDITION(GetDepth() > 0, "cannot increment an uninitialized iterator");

    
    ++mRowIndex;

    Link& top = GetTop();

    
    
    Subtree* subtree = top.GetRow().mSubtree;

    if (subtree && subtree->Count()) {
        Append(subtree, 0);
        return;
    }

    
    if (top.mChildIndex >= top.mParent->Count() - 1) {
        
        
        
        int32_t unfinished;
        for (unfinished = GetDepth() - 2; unfinished >= 0; --unfinished) {
            const Link& link = mLink[unfinished];
            if (link.mChildIndex < link.mParent->Count() - 1)
                break;
        }

        
        
        
        if (unfinished < 0) {
            top.mChildIndex++;
            return;
        }

        
        
        mLink.SetLength(unfinished + 1);
    }

    
    ++(GetTop().mChildIndex);
}

void
nsTreeRows::iterator::Prev()
{
    NS_PRECONDITION(GetDepth() > 0, "cannot increment an uninitialized iterator");

    
    --mRowIndex;

    
    --(GetTop().mChildIndex);

    
    if (GetTop().mChildIndex < 0) {
        
        
        
        int32_t unfinished;
        for (unfinished = GetDepth() - 2; unfinished >= 0; --unfinished) {
            const Link& link = mLink[unfinished];
            if (link.mChildIndex >= 0)
                break;
        }

        
        
        
        if (unfinished < 0)
            return;

        
        
        mLink.SetLength(unfinished + 1);
        return;
    }

    
    
    
    Subtree* parent = GetTop().GetParent();
    int32_t index = GetTop().GetChildIndex();

    Subtree* subtree = (*parent)[index].mSubtree;

    if (subtree && subtree->Count()) {
        do {
            index = subtree->Count() - 1;
            Append(subtree, index);

            parent = subtree;
            subtree = (*parent)[index].mSubtree;
        } while (subtree && subtree->Count());
    }
}
