




























































#ifndef nsRuleNetwork_h__
#define nsRuleNetwork_h__

#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDOMNode.h"
#include "plhash.h"
#include "pldhash.h"
#include "nsCRT.h"
#include "nsIRDFNode.h"

class nsIRDFResource;
class nsXULTemplateResultSetRDF;
class nsXULTemplateQueryProcessorRDF;











class MemoryElement {
public:
    MemoryElement() { MOZ_COUNT_CTOR(MemoryElement); }
    virtual ~MemoryElement() { MOZ_COUNT_DTOR(MemoryElement); }

    virtual const char* Type() const = 0;
    virtual PLHashNumber Hash() const = 0;
    virtual PRBool Equals(const MemoryElement& aElement) const = 0;
    virtual MemoryElement* Clone(void* aPool) const = 0;

    PRBool operator==(const MemoryElement& aMemoryElement) const {
        return Equals(aMemoryElement);
    }

    PRBool operator!=(const MemoryElement& aMemoryElement) const {
        return !Equals(aMemoryElement);
    }
};






class MemoryElementSet {
public:
    class ConstIterator;
    friend class ConstIterator;

protected:
    class List {
    public:
        List() { MOZ_COUNT_CTOR(MemoryElementSet::List); }

        ~List() {
            MOZ_COUNT_DTOR(MemoryElementSet::List);
            delete mElement;
            NS_IF_RELEASE(mNext); }

        PRInt32 AddRef() { return ++mRefCnt; }

        PRInt32 Release() {
            PRInt32 refcnt = --mRefCnt;
            if (refcnt == 0) delete this;
            return refcnt; }

        MemoryElement* mElement;
        PRInt32        mRefCnt;
        List*          mNext;
    };

    List* mElements;

public:
    MemoryElementSet() : mElements(nsnull) {
        MOZ_COUNT_CTOR(MemoryElementSet); }

    MemoryElementSet(const MemoryElementSet& aSet) : mElements(aSet.mElements) {
        MOZ_COUNT_CTOR(MemoryElementSet);
        NS_IF_ADDREF(mElements); }

    MemoryElementSet& operator=(const MemoryElementSet& aSet) {
        NS_IF_RELEASE(mElements);
        mElements = aSet.mElements;
        NS_IF_ADDREF(mElements);
        return *this; }
        
    ~MemoryElementSet() {
        MOZ_COUNT_DTOR(MemoryElementSet);
        NS_IF_RELEASE(mElements); }

public:
    class ConstIterator {
    public:
        ConstIterator(List* aElementList) : mCurrent(aElementList) {
            NS_IF_ADDREF(mCurrent); }

        ConstIterator(const ConstIterator& aConstIterator)
            : mCurrent(aConstIterator.mCurrent) {
            NS_IF_ADDREF(mCurrent); }

        ConstIterator& operator=(const ConstIterator& aConstIterator) {
            NS_IF_RELEASE(mCurrent);
            mCurrent = aConstIterator.mCurrent;
            NS_IF_ADDREF(mCurrent);
            return *this; }

        ~ConstIterator() { NS_IF_RELEASE(mCurrent); }

        ConstIterator& operator++() {
            List* next = mCurrent->mNext;
            NS_RELEASE(mCurrent);
            mCurrent = next;
            NS_IF_ADDREF(mCurrent);
            return *this; }

        ConstIterator operator++(int) {
            ConstIterator result(*this);
            List* next = mCurrent->mNext;
            NS_RELEASE(mCurrent);
            mCurrent = next;
            NS_IF_ADDREF(mCurrent);
            return result; }

        const MemoryElement& operator*() const {
            return *mCurrent->mElement; }

        const MemoryElement* operator->() const {
            return mCurrent->mElement; }

        PRBool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        PRBool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        List* mCurrent;
    };

    ConstIterator First() const { return ConstIterator(mElements); }
    ConstIterator Last() const { return ConstIterator(nsnull); }

    
    nsresult Add(MemoryElement* aElement);
};






class nsAssignment {
public:
    nsCOMPtr<nsIAtom> mVariable;
    nsCOMPtr<nsIRDFNode> mValue;

    nsAssignment() : mValue()
        { MOZ_COUNT_CTOR(nsAssignment); }

    nsAssignment(nsIAtom* aVariable, nsIRDFNode* aValue)
        : mVariable(aVariable),
          mValue(aValue)
        { MOZ_COUNT_CTOR(nsAssignment); }

    nsAssignment(const nsAssignment& aAssignment)
        : mVariable(aAssignment.mVariable),
          mValue(aAssignment.mValue)
        { MOZ_COUNT_CTOR(nsAssignment); }

    ~nsAssignment() { MOZ_COUNT_DTOR(nsAssignment); }

    nsAssignment& operator=(const nsAssignment& aAssignment) {
        mVariable = aAssignment.mVariable;
        mValue    = aAssignment.mValue;
        return *this; }

    PRBool operator==(const nsAssignment& aAssignment) const {
        return mVariable == aAssignment.mVariable && mValue == aAssignment.mValue; }

    PRBool operator!=(const nsAssignment& aAssignment) const {
        return mVariable != aAssignment.mVariable || mValue != aAssignment.mValue; }

    PLHashNumber Hash() const {
        
        PLHashNumber temp = PLHashNumber(NS_PTR_TO_INT32(mValue.get())) >> 2; 
        return (temp & 0xffff) | NS_PTR_TO_INT32(mVariable.get()); }
};








class nsAssignmentSet {
public:
    class ConstIterator;
    friend class ConstIterator;

protected:
    class List {
    public:
        List() { MOZ_COUNT_CTOR(nsAssignmentSet::List); }

        ~List() {
            MOZ_COUNT_DTOR(nsAssignmentSet::List);
            NS_IF_RELEASE(mNext); }

        PRInt32 AddRef() { return ++mRefCnt; }

        PRInt32 Release() {
            PRInt32 refcnt = --mRefCnt;
            if (refcnt == 0) delete this;
            return refcnt; }

        nsAssignment mAssignment;
        PRInt32 mRefCnt;
        List*   mNext;
    };

    List* mAssignments;

public:
    nsAssignmentSet()
        : mAssignments(nsnull)
        { MOZ_COUNT_CTOR(nsAssignmentSet); }

    nsAssignmentSet(const nsAssignmentSet& aSet)
        : mAssignments(aSet.mAssignments) {
        MOZ_COUNT_CTOR(nsAssignmentSet);
        NS_IF_ADDREF(mAssignments); }

    nsAssignmentSet& operator=(const nsAssignmentSet& aSet) {
        NS_IF_RELEASE(mAssignments);
        mAssignments = aSet.mAssignments;
        NS_IF_ADDREF(mAssignments);
        return *this; }

    ~nsAssignmentSet() {
        MOZ_COUNT_DTOR(nsAssignmentSet);
        NS_IF_RELEASE(mAssignments); }

public:
    class ConstIterator {
    public:
        ConstIterator(List* aAssignmentList) : mCurrent(aAssignmentList) {
            NS_IF_ADDREF(mCurrent); }

        ConstIterator(const ConstIterator& aConstIterator)
            : mCurrent(aConstIterator.mCurrent) {
            NS_IF_ADDREF(mCurrent); }

        ConstIterator& operator=(const ConstIterator& aConstIterator) {
            NS_IF_RELEASE(mCurrent);
            mCurrent = aConstIterator.mCurrent;
            NS_IF_ADDREF(mCurrent);
            return *this; }

        ~ConstIterator() { NS_IF_RELEASE(mCurrent); }

        ConstIterator& operator++() {
            List* next = mCurrent->mNext;
            NS_RELEASE(mCurrent);
            mCurrent = next;
            NS_IF_ADDREF(mCurrent);
            return *this; }

        ConstIterator operator++(int) {
            ConstIterator result(*this);
            List* next = mCurrent->mNext;
            NS_RELEASE(mCurrent);
            mCurrent = next;
            NS_IF_ADDREF(mCurrent);
            return result; }

        const nsAssignment& operator*() const {
            return mCurrent->mAssignment; }

        const nsAssignment* operator->() const {
            return &mCurrent->mAssignment; }

        PRBool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        PRBool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        List* mCurrent;
    };

    ConstIterator First() const { return ConstIterator(mAssignments); }
    ConstIterator Last() const { return ConstIterator(nsnull); }

public:
    





    nsresult Add(const nsAssignment& aElement);

    






    PRBool HasAssignment(nsIAtom* aVariable, nsIRDFNode* aValue) const;

    




    PRBool HasAssignment(const nsAssignment& aAssignment) const {
        return HasAssignment(aAssignment.mVariable, aAssignment.mValue); }

    






    PRBool HasAssignmentFor(nsIAtom* aVariable) const;

    







    PRBool GetAssignmentFor(nsIAtom* aVariable, nsIRDFNode** aValue) const;

    



    PRInt32 Count() const;

    



    PRBool IsEmpty() const { return mAssignments == nsnull; }

    PRBool Equals(const nsAssignmentSet& aSet) const;
    PRBool operator==(const nsAssignmentSet& aSet) const { return Equals(aSet); }
    PRBool operator!=(const nsAssignmentSet& aSet) const { return !Equals(aSet); }
};


















class Instantiation
{
public:
    


    nsAssignmentSet  mAssignments;

    


    MemoryElementSet mSupport;

    Instantiation() { MOZ_COUNT_CTOR(Instantiation); }

    Instantiation(const Instantiation& aInstantiation)
        : mAssignments(aInstantiation.mAssignments),
          mSupport(aInstantiation.mSupport) {
        MOZ_COUNT_CTOR(Instantiation); }

    Instantiation& operator=(const Instantiation& aInstantiation) {
        mAssignments = aInstantiation.mAssignments;
        mSupport  = aInstantiation.mSupport;
        return *this; }

    ~Instantiation() { MOZ_COUNT_DTOR(Instantiation); }

    







    nsresult AddAssignment(nsIAtom* aVariable, nsIRDFNode* aValue) {
        mAssignments.Add(nsAssignment(aVariable, aValue));
        return NS_OK; }

    







    nsresult AddSupportingElement(MemoryElement* aMemoryElement) {
        mSupport.Add(aMemoryElement);
        return NS_OK; }

    PRBool Equals(const Instantiation& aInstantiation) const {
        return mAssignments == aInstantiation.mAssignments; }

    PRBool operator==(const Instantiation& aInstantiation) const {
        return Equals(aInstantiation); }

    PRBool operator!=(const Instantiation& aInstantiation) const {
        return !Equals(aInstantiation); }

    static PLHashNumber Hash(const void* aKey);
    static PRIntn Compare(const void* aLeft, const void* aRight);
};







class InstantiationSet
{
public:
    InstantiationSet();
    InstantiationSet(const InstantiationSet& aInstantiationSet);
    InstantiationSet& operator=(const InstantiationSet& aInstantiationSet);

    ~InstantiationSet() {
        MOZ_COUNT_DTOR(InstantiationSet);
        Clear(); }

    class ConstIterator;
    friend class ConstIterator;

    class Iterator;
    friend class Iterator;

    friend class nsXULTemplateResultSetRDF; 

protected:
    class List {
    public:
        Instantiation mInstantiation;
        List*         mNext;
        List*         mPrev;

        List() { MOZ_COUNT_CTOR(InstantiationSet::List); }
        ~List() { MOZ_COUNT_DTOR(InstantiationSet::List); }
    };

    List mHead;

public:
    class ConstIterator {
    protected:
        friend class Iterator; 
        List* mCurrent;

    public:
        ConstIterator(List* aList) : mCurrent(aList) {}

        ConstIterator(const ConstIterator& aConstIterator)
            : mCurrent(aConstIterator.mCurrent) {}

        ConstIterator& operator=(const ConstIterator& aConstIterator) {
            mCurrent = aConstIterator.mCurrent;
            return *this; }

        ConstIterator& operator++() {
            mCurrent = mCurrent->mNext;
            return *this; }

        ConstIterator operator++(int) {
            ConstIterator result(*this);
            mCurrent = mCurrent->mNext;
            return result; }

        ConstIterator& operator--() {
            mCurrent = mCurrent->mPrev;
            return *this; }

        ConstIterator operator--(int) {
            ConstIterator result(*this);
            mCurrent = mCurrent->mPrev;
            return result; }

        const Instantiation& operator*() const {
            return mCurrent->mInstantiation; }

        const Instantiation* operator->() const {
            return &mCurrent->mInstantiation; }

        PRBool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        PRBool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }
    };

    ConstIterator First() const { return ConstIterator(mHead.mNext); }
    ConstIterator Last() const { return ConstIterator(NS_CONST_CAST(List*, &mHead)); }

    class Iterator : public ConstIterator {
    public:
        Iterator(List* aList) : ConstIterator(aList) {}

        Iterator& operator++() {
            mCurrent = mCurrent->mNext;
            return *this; }

        Iterator operator++(int) {
            Iterator result(*this);
            mCurrent = mCurrent->mNext;
            return result; }

        Iterator& operator--() {
            mCurrent = mCurrent->mPrev;
            return *this; }

        Iterator operator--(int) {
            Iterator result(*this);
            mCurrent = mCurrent->mPrev;
            return result; }

        Instantiation& operator*() const {
            return mCurrent->mInstantiation; }

        Instantiation* operator->() const {
            return &mCurrent->mInstantiation; }

        PRBool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        PRBool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

        friend class InstantiationSet;
    };

    Iterator First() { return Iterator(mHead.mNext); }
    Iterator Last() { return Iterator(&mHead); }

    PRBool Empty() const { return First() == Last(); }

    Iterator Append(const Instantiation& aInstantiation) {
        return Insert(Last(), aInstantiation); }

    Iterator Insert(Iterator aBefore, const Instantiation& aInstantiation);

    Iterator Erase(Iterator aElement);

    void Clear();

    PRBool HasAssignmentFor(nsIAtom* aVariable) const;
};






class ReteNode
{
public:
    ReteNode() {}
    virtual ~ReteNode() {}

    

























    virtual nsresult Propagate(InstantiationSet& aInstantiations,
                               PRBool aIsUpdate, PRBool& aTakenInstantiations) = 0;
};






class ReteNodeSet
{
public:
    ReteNodeSet();
    ~ReteNodeSet();

    nsresult Add(ReteNode* aNode);
    nsresult Clear();

    class Iterator;

    class ConstIterator {
    public:
        ConstIterator(ReteNode** aNode) : mCurrent(aNode) {}

        ConstIterator(const ConstIterator& aConstIterator)
            : mCurrent(aConstIterator.mCurrent) {}

        ConstIterator& operator=(const ConstIterator& aConstIterator) {
            mCurrent = aConstIterator.mCurrent;
            return *this; }

        ConstIterator& operator++() {
            ++mCurrent;
            return *this; }

        ConstIterator operator++(int) {
            ConstIterator result(*this);
            ++mCurrent;
            return result; }

        const ReteNode* operator*() const {
            return *mCurrent; }

        const ReteNode* operator->() const {
            return *mCurrent; }

        PRBool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        PRBool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        friend class Iterator; 
        ReteNode** mCurrent;
    };

    ConstIterator First() const { return ConstIterator(mNodes); }
    ConstIterator Last() const { return ConstIterator(mNodes + mCount); }

    class Iterator : public ConstIterator {
    public:
        Iterator(ReteNode** aNode) : ConstIterator(aNode) {}

        Iterator& operator++() {
            ++mCurrent;
            return *this; }

        Iterator operator++(int) {
            Iterator result(*this);
            ++mCurrent;
            return result; }

        ReteNode* operator*() const {
            return *mCurrent; }

        ReteNode* operator->() const {
            return *mCurrent; }

        PRBool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        PRBool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }
    };

    Iterator First() { return Iterator(mNodes); }
    Iterator Last() { return Iterator(mNodes + mCount); }

    PRInt32 Count() const { return mCount; }

protected:
    ReteNode** mNodes;
    PRInt32 mCount;
    PRInt32 mCapacity;
};











class TestNode : public ReteNode
{
public:
    TestNode(TestNode* aParent);

    



    TestNode* GetParent() const { return mParent; }

    



































    virtual nsresult Propagate(InstantiationSet& aInstantiations,
                               PRBool aIsUpdate, PRBool& aTakenInstantiations);

    

















    virtual nsresult Constrain(InstantiationSet& aInstantiations);

    












    virtual nsresult FilterInstantiations(InstantiationSet& aInstantiations,
                                          PRBool* aCantHandleYet) const = 0;
    

    





    PRBool HasAncestor(const ReteNode* aNode) const;

    




    nsresult AddChild(ReteNode* aNode) { return mKids.Add(aNode); }

    



    nsresult RemoveAllChildren() { return mKids.Clear(); }

protected:
    TestNode* mParent;
    ReteNodeSet mKids;
};

#endif 
