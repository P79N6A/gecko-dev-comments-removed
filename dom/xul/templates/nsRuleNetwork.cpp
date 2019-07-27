

















#include "nscore.h"
#include "nsCOMPtr.h"
#include "plhash.h"

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

using namespace mozilla;






nsresult
MemoryElementSet::Add(MemoryElement* aElement)
{
    for (ConstIterator element = First(); element != Last(); ++element) {
        if (*element == *aElement) {
            
            
            
            delete aElement;
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

    List* list = new List(aAssignment);
    if (! list)
        return NS_ERROR_OUT_OF_MEMORY;

    list->mRefCnt     = 1;
    list->mNext       = mAssignments;

    mAssignments = list;

    return NS_OK;
}

int32_t
nsAssignmentSet::Count() const
{
    int32_t count = 0;
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment)
        ++count;

    return count;
}

bool
nsAssignmentSet::HasAssignment(nsIAtom* aVariable, nsIRDFNode* aValue) const
{
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (assignment->mVariable == aVariable && assignment->mValue == aValue)
            return true;
    }

    return false;
}

bool
nsAssignmentSet::HasAssignmentFor(nsIAtom* aVariable) const
{
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (assignment->mVariable == aVariable)
            return true;
    }

    return false;
}

bool
nsAssignmentSet::GetAssignmentFor(nsIAtom* aVariable, nsIRDFNode** aValue) const
{
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (assignment->mVariable == aVariable) {
            *aValue = assignment->mValue;
            NS_IF_ADDREF(*aValue);
            return true;
        }
    }

    *aValue = nullptr;
    return false;
}

bool
nsAssignmentSet::Equals(const nsAssignmentSet& aSet) const
{
    if (aSet.mAssignments == mAssignments)
        return true;

    
    if (Count() != aSet.Count())
        return false;

    
    nsCOMPtr<nsIRDFNode> value;
    for (ConstIterator assignment = First(); assignment != Last(); ++assignment) {
        if (! aSet.GetAssignmentFor(assignment->mVariable, getter_AddRefs(value)))
            return false;

        if (assignment->mValue != value)
            return false;
    }

    return true;
}



PLHashNumber
Instantiation::Hash(const void* aKey)
{
    const Instantiation* inst = static_cast<const Instantiation*>(aKey);

    PLHashNumber result = 0;

    nsAssignmentSet::ConstIterator last = inst->mAssignments.Last();
    for (nsAssignmentSet::ConstIterator assignment = inst->mAssignments.First();
         assignment != last; ++assignment)
        result ^= assignment->Hash();

    return result;
}


int
Instantiation::Compare(const void* aLeft, const void* aRight)
{
    const Instantiation* left  = static_cast<const Instantiation*>(aLeft);
    const Instantiation* right = static_cast<const Instantiation*>(aRight);

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


bool
InstantiationSet::HasAssignmentFor(nsIAtom* aVariable) const
{
    return !Empty() ? First()->mAssignments.HasAssignmentFor(aVariable) : false;
}

















TestNode::TestNode(TestNode* aParent)
    : mParent(aParent)
{
}

nsresult
TestNode::Propagate(InstantiationSet& aInstantiations,
                    bool aIsUpdate, bool& aTakenInstantiations)
{
    PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
           ("TestNode[%p]: Propagate() begin", this));

    aTakenInstantiations = false;

    nsresult rv = FilterInstantiations(aInstantiations, nullptr);
    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    bool shouldCopy = (mKids.Count() > 1);

    
    if (! aInstantiations.Empty()) {
        ReteNodeSet::Iterator last = mKids.Last();
        for (ReteNodeSet::Iterator kid = mKids.First(); kid != last; ++kid) {
            PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
                   ("TestNode[%p]: Propagate() passing to child %p", this, kid.operator->()));

            
            if (shouldCopy) {
                bool owned = false;
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

    
    
    
    
    
    bool cantHandleYet = false;
    rv = FilterInstantiations(aInstantiations, &cantHandleYet);
    if (NS_FAILED(rv)) return rv;

    if (mParent && (!aInstantiations.Empty() || cantHandleYet)) {
        
        
        

        PR_LOG(gXULTemplateLog, PR_LOG_DEBUG,
               ("TestNode[%p]: Constrain() passing to parent %p", this, mParent));

        rv = mParent->Constrain(aInstantiations);

        if (NS_SUCCEEDED(rv) && cantHandleYet)
            rv = FilterInstantiations(aInstantiations, nullptr);
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




ReteNodeSet::ReteNodeSet()
    : mNodes(nullptr), mCount(0), mCapacity(0)
{
}

ReteNodeSet::~ReteNodeSet()
{
    Clear();
}

nsresult
ReteNodeSet::Add(ReteNode* aNode)
{
    NS_PRECONDITION(aNode != nullptr, "null ptr");
    if (! aNode)
        return NS_ERROR_NULL_POINTER;

    if (mCount >= mCapacity) {
        int32_t capacity = mCapacity + 4;
        ReteNode** nodes = new ReteNode*[capacity];
        if (! nodes)
            return NS_ERROR_OUT_OF_MEMORY;

        for (int32_t i = mCount - 1; i >= 0; --i)
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
    mNodes = nullptr;
    mCount = mCapacity = 0;
    return NS_OK;
}
