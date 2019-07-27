


























#ifndef nsRuleNetwork_h__
#define nsRuleNetwork_h__

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsIAtom.h"
#include "nsIDOMNode.h"
#include "plhash.h"
#include "pldhash.h"
#include "nsIRDFNode.h"

class nsXULTemplateResultSetRDF;











class MemoryElement {
protected:
    MemoryElement() { MOZ_COUNT_CTOR(MemoryElement); }

public:
    virtual ~MemoryElement() { MOZ_COUNT_DTOR(MemoryElement); }

    virtual const char* Type() const = 0;
    virtual PLHashNumber Hash() const = 0;
    virtual bool Equals(const MemoryElement& aElement) const = 0;

    bool operator==(const MemoryElement& aMemoryElement) const {
        return Equals(aMemoryElement);
    }

    bool operator!=(const MemoryElement& aMemoryElement) const {
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

    protected:
        ~List() {
            MOZ_COUNT_DTOR(MemoryElementSet::List);
            delete mElement;
            NS_IF_RELEASE(mNext); }

    public:
        int32_t AddRef() { return ++mRefCnt; }

        int32_t Release() {
            int32_t refcnt = --mRefCnt;
            if (refcnt == 0) delete this;
            return refcnt; }

        MemoryElement* mElement;
        int32_t        mRefCnt;
        List*          mNext;
    };

    List* mElements;

public:
    MemoryElementSet() : mElements(nullptr) {
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
        explicit ConstIterator(List* aElementList) : mCurrent(aElementList) {
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

        bool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        bool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        List* mCurrent;
    };

    ConstIterator First() const { return ConstIterator(mElements); }
    ConstIterator Last() const { return ConstIterator(nullptr); }

    
    nsresult Add(MemoryElement* aElement);
};






class nsAssignment {
public:
    const nsCOMPtr<nsIAtom> mVariable;
    nsCOMPtr<nsIRDFNode> mValue;

    nsAssignment(nsIAtom* aVariable, nsIRDFNode* aValue)
        : mVariable(aVariable),
          mValue(aValue)
        { MOZ_COUNT_CTOR(nsAssignment); }

    nsAssignment(const nsAssignment& aAssignment)
        : mVariable(aAssignment.mVariable),
          mValue(aAssignment.mValue)
        { MOZ_COUNT_CTOR(nsAssignment); }

    ~nsAssignment() { MOZ_COUNT_DTOR(nsAssignment); }

    bool operator==(const nsAssignment& aAssignment) const {
        return mVariable == aAssignment.mVariable && mValue == aAssignment.mValue; }

    bool operator!=(const nsAssignment& aAssignment) const {
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
        explicit List(const nsAssignment& aAssignment) : mAssignment(aAssignment) {
            MOZ_COUNT_CTOR(nsAssignmentSet::List); }

    protected:
        ~List() {
            MOZ_COUNT_DTOR(nsAssignmentSet::List);
            NS_IF_RELEASE(mNext); }

    public:

        int32_t AddRef() { return ++mRefCnt; }

        int32_t Release() {
            int32_t refcnt = --mRefCnt;
            if (refcnt == 0) delete this;
            return refcnt; }

        nsAssignment mAssignment;
        int32_t mRefCnt;
        List*   mNext;
    };

    List* mAssignments;

public:
    nsAssignmentSet()
        : mAssignments(nullptr)
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
        explicit ConstIterator(List* aAssignmentList) : mCurrent(aAssignmentList) {
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

        bool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        bool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        List* mCurrent;
    };

    ConstIterator First() const { return ConstIterator(mAssignments); }
    ConstIterator Last() const { return ConstIterator(nullptr); }

public:
    





    nsresult Add(const nsAssignment& aElement);

    






    bool HasAssignment(nsIAtom* aVariable, nsIRDFNode* aValue) const;

    




    bool HasAssignment(const nsAssignment& aAssignment) const {
        return HasAssignment(aAssignment.mVariable, aAssignment.mValue); }

    






    bool HasAssignmentFor(nsIAtom* aVariable) const;

    







    bool GetAssignmentFor(nsIAtom* aVariable, nsIRDFNode** aValue) const;

    



    int32_t Count() const;

    



    bool IsEmpty() const { return mAssignments == nullptr; }

    bool Equals(const nsAssignmentSet& aSet) const;
    bool operator==(const nsAssignmentSet& aSet) const { return Equals(aSet); }
    bool operator!=(const nsAssignmentSet& aSet) const { return !Equals(aSet); }
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

    bool Equals(const Instantiation& aInstantiation) const {
        return mAssignments == aInstantiation.mAssignments; }

    bool operator==(const Instantiation& aInstantiation) const {
        return Equals(aInstantiation); }

    bool operator!=(const Instantiation& aInstantiation) const {
        return !Equals(aInstantiation); }

    static PLHashNumber Hash(const void* aKey);
    static int Compare(const void* aLeft, const void* aRight);
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
        explicit ConstIterator(List* aList) : mCurrent(aList) {}

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

        bool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        bool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }
    };

    ConstIterator First() const { return ConstIterator(mHead.mNext); }
    ConstIterator Last() const { return ConstIterator(const_cast<List*>(&mHead)); }

    class Iterator : public ConstIterator {
    public:
        explicit Iterator(List* aList) : ConstIterator(aList) {}

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

        bool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        bool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

        friend class InstantiationSet;
    };

    Iterator First() { return Iterator(mHead.mNext); }
    Iterator Last() { return Iterator(&mHead); }

    bool Empty() const { return First() == Last(); }

    Iterator Append(const Instantiation& aInstantiation) {
        return Insert(Last(), aInstantiation); }

    Iterator Insert(Iterator aBefore, const Instantiation& aInstantiation);

    Iterator Erase(Iterator aElement);

    void Clear();

    bool HasAssignmentFor(nsIAtom* aVariable) const;
};






class ReteNode
{
public:
    ReteNode() {}
    virtual ~ReteNode() {}

    

























    virtual nsresult Propagate(InstantiationSet& aInstantiations,
                               bool aIsUpdate, bool& aTakenInstantiations) = 0;
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
        explicit ConstIterator(ReteNode** aNode) : mCurrent(aNode) {}

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

        bool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        bool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }

    protected:
        friend class Iterator; 
        ReteNode** mCurrent;
    };

    ConstIterator First() const { return ConstIterator(mNodes); }
    ConstIterator Last() const { return ConstIterator(mNodes + mCount); }

    class Iterator : public ConstIterator {
    public:
        explicit Iterator(ReteNode** aNode) : ConstIterator(aNode) {}

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

        bool operator==(const ConstIterator& aConstIterator) const {
            return mCurrent == aConstIterator.mCurrent; }

        bool operator!=(const ConstIterator& aConstIterator) const {
            return mCurrent != aConstIterator.mCurrent; }
    };

    Iterator First() { return Iterator(mNodes); }
    Iterator Last() { return Iterator(mNodes + mCount); }

    int32_t Count() const { return mCount; }

protected:
    ReteNode** mNodes;
    int32_t mCount;
    int32_t mCapacity;
};











class TestNode : public ReteNode
{
public:
    explicit TestNode(TestNode* aParent);

    



    TestNode* GetParent() const { return mParent; }

    



































    virtual nsresult Propagate(InstantiationSet& aInstantiations,
                               bool aIsUpdate, bool& aTakenInstantiations) override;

    

















    virtual nsresult Constrain(InstantiationSet& aInstantiations);

    












    virtual nsresult FilterInstantiations(InstantiationSet& aInstantiations,
                                          bool* aCantHandleYet) const = 0;
    

    




    nsresult AddChild(ReteNode* aNode) { return mKids.Add(aNode); }

    



    nsresult RemoveAllChildren() { return mKids.Clear(); }

protected:
    TestNode* mParent;
    ReteNodeSet mKids;
};

#endif 
