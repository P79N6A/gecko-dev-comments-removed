





































#include "txNodeSet.h"
#include "txLog.h"
#include "nsMemory.h"
#include "txXPathTreeWalker.h"





#ifdef NS_BUILD_REFCNT_LOGGING
#define LOG_CHUNK_MOVE(_start, _new_start, _count)            \
{                                                             \
    txXPathNode *start = const_cast<txXPathNode*>(_start);    \
    while (start < _start + _count) {                         \
        NS_LogDtor(start, "txXPathNode", sizeof(*start));     \
        ++start;                                              \
    }                                                         \
    start = const_cast<txXPathNode*>(_new_start);             \
    while (start < _new_start + _count) {                     \
        NS_LogCtor(start, "txXPathNode", sizeof(*start));     \
        ++start;                                              \
    }                                                         \
}
#else
#define LOG_CHUNK_MOVE(_start, _new_start, _count)
#endif

static const PRInt32 kTxNodeSetMinSize = 4;
static const PRInt32 kTxNodeSetGrowFactor = 2;

#define kForward   1
#define kReversed -1

txNodeSet::txNodeSet(txResultRecycler* aRecycler)
    : txAExprResult(aRecycler),
      mStart(nsnull),
      mEnd(nsnull),
      mStartBuffer(nsnull),
      mEndBuffer(nsnull),
      mDirection(kForward),
      mMarks(nsnull)
{
}

txNodeSet::txNodeSet(const txXPathNode& aNode, txResultRecycler* aRecycler)
    : txAExprResult(aRecycler),
      mStart(nsnull),
      mEnd(nsnull),
      mStartBuffer(nsnull),
      mEndBuffer(nsnull),
      mDirection(kForward),
      mMarks(nsnull)
{
    if (!ensureGrowSize(1)) {
        return;
    }

    new(mStart) txXPathNode(aNode);
    ++mEnd;
}

txNodeSet::txNodeSet(const txNodeSet& aSource, txResultRecycler* aRecycler)
    : txAExprResult(aRecycler),
      mStart(nsnull),
      mEnd(nsnull),
      mStartBuffer(nsnull),
      mEndBuffer(nsnull),
      mDirection(kForward),
      mMarks(nsnull)
{
    append(aSource);
}

txNodeSet::~txNodeSet()
{
    delete [] mMarks;

    if (mStartBuffer) {
        destroyElements(mStart, mEnd);

        nsMemory::Free(mStartBuffer);
    }
}

nsresult txNodeSet::add(const txXPathNode& aNode)
{
    NS_ASSERTION(mDirection == kForward,
                 "only append(aNode) is supported on reversed nodesets");

    if (isEmpty()) {
        return append(aNode);
    }

    PRBool dupe;
    txXPathNode* pos = findPosition(aNode, mStart, mEnd, dupe);

    if (dupe) {
        return NS_OK;
    }

    
    PRInt32 moveSize = mEnd - pos;
    PRInt32 offset = pos - mStart;
    if (!ensureGrowSize(1)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    
    pos = mStart + offset;

    if (moveSize > 0) {
        LOG_CHUNK_MOVE(pos, pos + 1, moveSize);
        memmove(pos + 1, pos, moveSize * sizeof(txXPathNode));
    }

    new(pos) txXPathNode(aNode);
    ++mEnd;

    return NS_OK;
}

nsresult txNodeSet::add(const txNodeSet& aNodes)
{
    return add(aNodes, copyElements, nsnull);
}

nsresult txNodeSet::addAndTransfer(txNodeSet* aNodes)
{
    
    nsresult rv = add(*aNodes, transferElements, destroyElements);
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef TX_DONT_RECYCLE_BUFFER
    if (aNodes->mStartBuffer) {
        nsMemory::Free(aNodes->mStartBuffer);
        aNodes->mStartBuffer = aNodes->mEndBuffer = nsnull;
    }
#endif
    aNodes->mStart = aNodes->mEnd = aNodes->mStartBuffer;

    return NS_OK;
}










































nsresult txNodeSet::add(const txNodeSet& aNodes, transferOp aTransfer,
                        destroyOp aDestroy)
{
    NS_ASSERTION(mDirection == kForward,
                 "only append(aNode) is supported on reversed nodesets");

    if (aNodes.isEmpty()) {
        return NS_OK;
    }

    if (!ensureGrowSize(aNodes.size())) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    if (mStart == mEnd ||
        txXPathNodeUtils::comparePosition(mEnd[-1], *aNodes.mStart) < 0) {
        aTransfer(mEnd, aNodes.mStart, aNodes.mEnd);
        mEnd += aNodes.size();

        return NS_OK;
    }

    
    txXPathNode* thisPos = mEnd;

    
    txXPathNode* otherPos = aNodes.mEnd;

    
    txXPathNode* insertPos = mEndBuffer;

    PRBool dupe;
    txXPathNode* pos;
    PRInt32 count;
    while (thisPos > mStart || otherPos > aNodes.mStart) {
        
        
        if (thisPos > mStart) {
            pos = findPosition(thisPos[-1], aNodes.mStart, otherPos, dupe);

            if (dupe) {
                const txXPathNode *deletePos = thisPos;
                --thisPos; 
                
                while (thisPos > mStart && pos > aNodes.mStart &&
                       thisPos[-1] == pos[-1]) {
                    --thisPos;
                    --pos;
                }

                if (aDestroy) {
                    aDestroy(thisPos, deletePos);
                }
            }
        }
        else {
            pos = aNodes.mStart;
        }

        
        count = otherPos - pos;
        if (count > 0) {
            insertPos -= count;
            aTransfer(insertPos, pos, otherPos);
            otherPos -= count;
        }

        
        
        if (otherPos > aNodes.mStart) {
            pos = findPosition(otherPos[-1], mStart, thisPos, dupe);

            if (dupe) {
                const txXPathNode *deletePos = otherPos;
                --otherPos; 
                
                while (otherPos > aNodes.mStart && pos > mStart &&
                       otherPos[-1] == pos[-1]) {
                    --otherPos;
                    --pos;
                }

                if (aDestroy) {
                    aDestroy(otherPos, deletePos);
                }
            }
        }
        else {
            pos = mStart;
        }

        
        
        count = thisPos - pos;
        if (count > 0) {
            insertPos -= count;
            LOG_CHUNK_MOVE(pos, insertPos, count);
            memmove(insertPos, pos, count * sizeof(txXPathNode));
            thisPos -= count;
        }
    }
    mStart = insertPos;
    mEnd = mEndBuffer;
    
    return NS_OK;
}











nsresult
txNodeSet::append(const txXPathNode& aNode)
{
    if (!ensureGrowSize(1)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (mDirection == kForward) {
        new(mEnd) txXPathNode(aNode);
        ++mEnd;

        return NS_OK;
    }

    new(--mStart) txXPathNode(aNode);

    return NS_OK;
}

nsresult
txNodeSet::append(const txNodeSet& aNodes)
{
    NS_ASSERTION(mDirection == kForward,
                 "only append(aNode) is supported on reversed nodesets");

    if (aNodes.isEmpty()) {
        return NS_OK;
    }

    PRInt32 appended = aNodes.size();
    if (!ensureGrowSize(appended)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    copyElements(mEnd, aNodes.mStart, aNodes.mEnd);
    mEnd += appended;

    return NS_OK;
}

nsresult
txNodeSet::mark(PRInt32 aIndex)
{
    NS_ASSERTION(aIndex >= 0 && mStart && mEnd - mStart > aIndex,
                 "index out of bounds");
    if (!mMarks) {
        PRInt32 length = size();
        mMarks = new PRPackedBool[length];
        NS_ENSURE_TRUE(mMarks, NS_ERROR_OUT_OF_MEMORY);
        memset(mMarks, 0, length * sizeof(PRPackedBool));
    }
    if (mDirection == kForward) {
        mMarks[aIndex] = PR_TRUE;
    }
    else {
        mMarks[size() - aIndex - 1] = PR_TRUE;
    }

    return NS_OK;
}

nsresult
txNodeSet::sweep()
{
    if (!mMarks) {
        
        clear();
    }

    PRInt32 chunk, pos = 0;
    PRInt32 length = size();
    txXPathNode* insertion = mStartBuffer;

    while (pos < length) {
        while (pos < length && !mMarks[pos]) {
            
            mStart[pos].~txXPathNode();
            ++pos;
        }
        
        chunk = 0;
        while (pos < length && mMarks[pos]) {
            ++pos;
            ++chunk;
        }
        
        if (chunk > 0) {
            LOG_CHUNK_MOVE(mStart + pos - chunk, insertion, chunk);
            memmove(insertion, mStart + pos - chunk,
                    chunk * sizeof(txXPathNode));
            insertion += chunk;
        }
    }
    mStart = mStartBuffer;
    mEnd = insertion;
    delete [] mMarks;
    mMarks = nsnull;

    return NS_OK;
}

void
txNodeSet::clear()
{
    destroyElements(mStart, mEnd);
#ifdef TX_DONT_RECYCLE_BUFFER
    if (mStartBuffer) {
        nsMemory::Free(mStartBuffer);
        mStartBuffer = mEndBuffer = nsnull;
    }
#endif
    mStart = mEnd = mStartBuffer;
    delete [] mMarks;
    mMarks = nsnull;
    mDirection = kForward;
}

PRInt32
txNodeSet::indexOf(const txXPathNode& aNode, PRUint32 aStart) const
{
    NS_ASSERTION(mDirection == kForward,
                 "only append(aNode) is supported on reversed nodesets");

    if (!mStart || mStart == mEnd) {
        return -1;
    }

    txXPathNode* pos = mStart + aStart;
    for (; pos < mEnd; ++pos) {
        if (*pos == aNode) {
            return pos - mStart;
        }
    }

    return -1;
}

const txXPathNode&
txNodeSet::get(PRInt32 aIndex) const
{
    if (mDirection == kForward) {
        return mStart[aIndex];
    }

    return mEnd[-aIndex - 1];
}

short
txNodeSet::getResultType()
{
    return txAExprResult::NODESET;
}

PRBool
txNodeSet::booleanValue()
{
    return !isEmpty();
}
double
txNodeSet::numberValue()
{
    nsAutoString str;
    stringValue(str);

    return Double::toDouble(str);
}

void
txNodeSet::stringValue(nsString& aStr)
{
    NS_ASSERTION(mDirection == kForward,
                 "only append(aNode) is supported on reversed nodesets");
    if (isEmpty()) {
        return;
    }
    txXPathNodeUtils::appendNodeValue(get(0), aStr);
}

const nsString*
txNodeSet::stringValuePointer()
{
    return nsnull;
}

PRBool txNodeSet::ensureGrowSize(PRInt32 aSize)
{
    
    if (mDirection == kForward && aSize <= mEndBuffer - mEnd) {
        return PR_TRUE;
    }

    if (mDirection == kReversed && aSize <= mStart - mStartBuffer) {
        return PR_TRUE;
    }

    
    PRInt32 oldSize = mEnd - mStart;
    PRInt32 oldLength = mEndBuffer - mStartBuffer;
    PRInt32 ensureSize = oldSize + aSize;
    if (ensureSize <= oldLength) {
        
        txXPathNode* dest = mStartBuffer;
        if (mDirection == kReversed) {
            dest = mEndBuffer - oldSize;
        }
        LOG_CHUNK_MOVE(mStart, dest, oldSize);
        memmove(dest, mStart, oldSize * sizeof(txXPathNode));
        mStart = dest;
        mEnd = dest + oldSize;
            
        return PR_TRUE;
    }

    
    
    PRInt32 newLength = PR_MAX(oldLength, kTxNodeSetMinSize);

    while (newLength < ensureSize) {
        newLength *= kTxNodeSetGrowFactor;
    }

    txXPathNode* newArr = static_cast<txXPathNode*>
                                     (nsMemory::Alloc(newLength *
                                                         sizeof(txXPathNode)));
    if (!newArr) {
        return PR_FALSE;
    }

    txXPathNode* dest = newArr;
    if (mDirection == kReversed) {
        dest += newLength - oldSize;
    }

    if (oldSize > 0) {
        LOG_CHUNK_MOVE(mStart, dest, oldSize);
        memcpy(dest, mStart, oldSize * sizeof(txXPathNode));
    }

    if (mStartBuffer) {
#ifdef DEBUG
        memset(mStartBuffer, 0,
               (mEndBuffer - mStartBuffer) * sizeof(txXPathNode));
#endif
        nsMemory::Free(mStartBuffer);
    }

    mStartBuffer = newArr;
    mEndBuffer = mStartBuffer + newLength;
    mStart = dest;
    mEnd = dest + oldSize;

    return PR_TRUE;
}

txXPathNode*
txNodeSet::findPosition(const txXPathNode& aNode, txXPathNode* aFirst,
                        txXPathNode* aLast, PRBool& aDupe) const
{
    aDupe = PR_FALSE;
    if (aLast - aFirst <= 2) {
        
        txXPathNode* pos = aFirst;
        for (; pos < aLast; ++pos) {
            PRIntn cmp = txXPathNodeUtils::comparePosition(aNode, *pos);
            if (cmp < 0) {
                return pos;
            }

            if (cmp == 0) {
                aDupe = PR_TRUE;

                return pos;
            }
        }
        return pos;
    }

    
    txXPathNode* midpos = aFirst + (aLast - aFirst) / 2;
    PRIntn cmp = txXPathNodeUtils::comparePosition(aNode, *midpos);
    if (cmp == 0) {
        aDupe = PR_TRUE;

        return midpos;
    }

    if (cmp > 0) {
        return findPosition(aNode, midpos + 1, aLast, aDupe);
    }

    

    return findPosition(aNode, aFirst, midpos, aDupe);
}


void
txNodeSet::copyElements(txXPathNode* aDest,
                        const txXPathNode* aStart, const txXPathNode* aEnd)
{
    const txXPathNode* pos = aStart;
    while (pos < aEnd) {
        new(aDest) txXPathNode(*pos);
        ++aDest;
        ++pos;
    }
}


void
txNodeSet::transferElements(txXPathNode* aDest,
                            const txXPathNode* aStart, const txXPathNode* aEnd)
{
    LOG_CHUNK_MOVE(aStart, aDest, (aEnd - aStart));
    memcpy(aDest, aStart, (aEnd - aStart) * sizeof(txXPathNode));
}
