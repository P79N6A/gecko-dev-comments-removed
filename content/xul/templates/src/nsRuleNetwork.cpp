



















































#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsCRT.h"
#include "nsIComponentManager.h"
#include "nsIContent.h"
#include "plhash.h"
#include "nsReadableUtils.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gXULTemplateLog;

#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsXULContentUtils.h"

#endif

#include "nsRuleNetwork.h"
#include "nsXULTemplateResultSetRDF.h"
#include "nsRDFConMemberTestNode.h"
#include "nsRDFPropertyTestNode.h"

PRBool MemoryElement::gPoolInited;
nsFixedSizeAllocator MemoryElement::gPool;


PRBool
MemoryElement::Init()
{
    if (!gPoolInited) {
        const size_t bucketsizes[] = {
            sizeof (nsRDFConMemberTestNode::Element),
            sizeof (nsRDFPropertyTestNode::Element)
        };

        if (NS_FAILED(gPool.Init("MemoryElement", bucketsizes,
                                 NS_ARRAY_LENGTH(bucketsizes), 256)))
            return PR_FALSE;

        gPoolInited = PR_TRUE;
    }

    return PR_TRUE;
}






nsresult
MemoryElementSet::Add(MemoryElement* aElement)
{
    for (ConstIterator element = First(); element != Last(); ++element) {
        if (*element == *aElement) {
            
            
            
            aElement->Destroy();
            return NS_OK;
        }
    }

    List* list = new List;
    if (! list)
        return NS_ERROR_OUT_OF_MEMORY;

    list->mElement = aElement;
    list->mRefCnt  = 1;
    list->mNext    = mElements;

    mElements = list;

    return NS_OK;
}




nsresult
nsAssignmentSet::Add(const nsAssignment& aAssignment)
{
    NS_PRECONDITION(! HasAssignmentFor(aAssignment.mVariable), "variable already bound");

    
    if (HasAssignmentFor(aAssignment.mVariable))
        return NS_ERROR_UNEXPECTED;

    List* list = new List;
    if (! list)
        return NS_ERROR_OUT_OF_MEMORY;

    list->mAssignment = aAssignment;
    list->mRefCnt     = 1;
    list->mNext       = mAssignments;

    mAssignments = list;

    return NS_OK;
}

PRInt32
nsAssignmentSet::Count() const
{
    PRInt32 count = 0;
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment)
        ++count;

    return count;
}

PRBool
nsAssignmentSet::HasAssignment(nsIAtom* aVariable, nsIRDFNode* aValue) const
{
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (assignment->mVariable == aVariable && assignment->mValue == aValue)
            return PR_TRUE;
    }

    return PR_FALSE;
}

PRBool
nsAssignmentSet::HasAssignmentFor(nsIAtom* aVariable) const
{
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (assignment->mVariable == aVariable)
            return PR_TRUE;
    }

    return PR_FALSE;
}

PRBool
nsAssignmentSet::GetAssignmentFor(nsIAtom* aVariable, nsIRDFNode** aValue) const
{
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (assignment->mVariable == aVariable) {
            *aValue = assignment->mValue;
            NS_IF_ADDREF(*aValue);
            return PR_TRUE;
        }
    }

    *aValue = nsnull;
    return PR_FALSE;
}

PRBool
nsAssignmentSet::Equals(const nsAssignmentSet& aSet) const
{
    if (aSet.mAssignments == mAssignments)
        return PR_TRUE;

    
    if (Count() != aSet.Count())
        return PR_FALSE;

    
    nsCOMPtr<nsIRDFNode> value;
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (! aSet.GetAssignmentFor(assignment->mVariable, getter_AddRefs(value)))
            return PR_FALSE;

        if (assignment->mValue != value)
            return PR_FALSE;
    }

    return PR_TRUE;
}



PLHashNumber
Instantiation::Hash(const void* aKey)
{
    const Instantiation* inst = NS_STATIC_CAST(const Instantiation*, aKey);

    PLHashNumber result = 0;

    nsAssignmentSet::ConstIterator last = inst->mAssignments.Last();
    for (nsAssignmentSet::ConstIterator assignment = inst->mAssignments.First();
         assignment != last; ++assignment)
        result ^= assignment->Hash();

    return result;
}


PRIntn
Instantiation::Compare(const void* aLeft, const void* aRight)
{
    const Instantiation* left  = NS_STATIC_CAST(const Instantiation*, aLeft);
    const Instantiation* right = NS_STATIC_CAST(const Instantiation*, aRight);

    return *left == *right;
}







InstantiationSet::InstantiationSet()
{
    mHead.mPrev = mHead.mNext = &mHead;
    MOZ_COUNT_CTOR(InstantiationSet);
}


InstantiationSet::InstantiationSet(const InstantiationSet& aInstantiationSet)
{
    mHead.mPrev = mHead.mNext = &mHead;

    
    ConstIterator last = aInstantiationSet.Last();
    for (ConstIterator inst = aInstantiationSet.First(); inst != last; ++inst)
        Append(*inst);

    MOZ_COUNT_CTOR(InstantiationSet);
}

InstantiationSet&
InstantiationSet::operator=(const InstantiationSet& aInstantiationSet)
{
    
    Clear();

    ConstIterator last = aInstantiationSet.Last();
    for (ConstIterator inst = aInstantiationSet.First(); inst != last; ++inst)
        Append(*inst);

    return *this;
}


void
InstantiationSet::Clear()
{
    Iterator inst = First();
    while (inst != Last())
        Erase(inst++);
}


InstantiationSet::Iterator
InstantiationSet::Insert(Iterator aIterator, const Instantiation& aInstantiation)
{
    List* newelement = new List();
    if (newelement) {
        newelement->mInstantiation = aInstantiation;

        aIterator.mCurrent->mPrev->mNext = newelement;

        newelement->mNext = aIterator.mCurrent;
        newelement->mPrev = aIterator.mCurrent->mPrev;

        aIterator.mCurrent->mPrev = newelement;
    }
    return aIterator;
}

InstantiationSet::Iterator
InstantiationSet::Erase(Iterator aIterator)
{
    Iterator result = aIterator;
    ++result;
    aIterator.mCurrent->mNext->mPrev = aIterator.mCurrent->mPrev;
    aIterator.mCurrent->mPrev->mNext = aIterator.mCurrent->mNext;
    delete aIterator.mCurrent;
    return result;
}


PRBool
InstantiationSet::HasAssignmentFor(nsIAtom* aVariable) const
{
    return !Empty() ? First()->mAssignments.HasAssignmentFor(aVariable) : PR_FALSE;
}

















TestNode::TestNode(TestNode* aParent)
    : mParent(aParent)
{
}

nsresult
TestNode::Propagate(InstantiationSet& aInstantiations,
                    PRBool aIsUpdate, PRBool& aTakenInstantiations)
{
    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
           ("TestNode[%p]: Propagate() begin", this));

    aTakenInstantiations = PR_FALSE;

    nsresult rv = FilterInstantiations(aInstantiations, nsnull);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    PRBool shouldCopy = (mKids.Count() > 1);

    
    if (! aInstantiations.Empty()) {
        ReteNodeSet::Iterator last = mKids.Last();
        for (ReteNodeSet::Iterator kid = mKids.First(); kid != last; ++kid) {
            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("TestNode[%p]: Propagate() passing to child %p", this, kid.operator->()));

            
            if (shouldCopy) {
                PRBool owned = PR_FALSE;
                InstantiationSet* instantiations =
                    new InstantiationSet(aInstantiations);
                if (!instantiations)
                    return NS_ERROR_OUT_OF_MEMORY;
                rv = kid->Propagate(*instantiations, aIsUpdate, owned);
                if (!owned)
                    delete instantiations;
                if (NS_FAILED(rv))
                    return rv;
            }
            else {
                rv = kid->Propagate(aInstantiations, aIsUpdate, aTakenInstantiations);
                if (NS_FAILED(rv))
                    return rv;
            }
        }
    }

    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
           ("TestNode[%p]: Propagate() end", this));

    return NS_OK;
}


nsresult
TestNode::Constrain(InstantiationSet& aInstantiations)
{
    nsresult rv;

    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
           ("TestNode[%p]: Constrain() begin", this));

    
    
    
    
    
    PRBool cantHandleYet = PR_FALSE;
    rv = FilterInstantiations(aInstantiations, &cantHandleYet);
    if (NS_FAILED(rv)) return rv;

    if ((mParent && ! aInstantiations.Empty()) || cantHandleYet) {
        
        
        

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("TestNode[%p]: Constrain() passing to parent %p", this, mParent));

        rv = mParent->Constrain(aInstantiations);

        if (NS_SUCCEEDED(rv) && cantHandleYet)
            rv = FilterInstantiations(aInstantiations, nsnull);
    }
    else {
        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("TestNode[%p]: Constrain() failed", this));

        rv = NS_OK;
    }

    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
           ("TestNode[%p]: Constrain() end", this));

    return rv;
}


PRBool
TestNode::HasAncestor(const ReteNode* aNode) const
{
    return aNode == this || (mParent && mParent->HasAncestor(aNode));
}




ReteNodeSet::ReteNodeSet()
    : mNodes(nsnull), mCount(0), mCapacity(0)
{
}

ReteNodeSet::~ReteNodeSet()
{
    Clear();
}

nsresult
ReteNodeSet::Add(ReteNode* aNode)
{
    NS_PRECONDITION(aNode != nsnull, "null ptr");
    if (! aNode)
        return NS_ERROR_NULL_POINTER;

    if (mCount >= mCapacity) {
        PRInt32 capacity = mCapacity + 4;
        ReteNode** nodes = new ReteNode*[capacity];
        if (! nodes)
            return NS_ERROR_OUT_OF_MEMORY;

        for (PRInt32 i = mCount - 1; i >= 0; --i)
            nodes[i] = mNodes[i];

        delete[] mNodes;

        mNodes = nodes;
        mCapacity = capacity;
    }

    mNodes[mCount++] = aNode;
    return NS_OK;
}

nsresult
ReteNodeSet::Clear()
{
    delete[] mNodes;
    mNodes = nsnull;
    mCount = mCapacity = 0;
    return NS_OK;
}
