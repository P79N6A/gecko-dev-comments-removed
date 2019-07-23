





































#ifndef nsTreeRows_h__
#define nsTreeRows_h__

#include "nsCOMPtr.h"
#include "pldhash.h"
#include "nsIXULTemplateResult.h"
#include "nsTemplateMatch.h"
#include "nsIRDFResource.h"







class nsTreeRows
{
public:
    class iterator;
    friend class iterator;

    enum Direction { eDirection_Forwards = +1, eDirection_Backwards = -1 };

    
    
    enum ContainerType {
        eContainerType_Unknown      =  0,
        eContainerType_Noncontainer =  1,
        eContainerType_Container    = -2
    };

    enum ContainerState {
        eContainerState_Unknown     =  0,
        eContainerState_Open        =  1,
        eContainerState_Closed      = -2
    };

    enum ContainerFill {
        eContainerFill_Unknown      =  0,
        eContainerFill_Empty        =  1,
        eContainerFill_Nonempty     = -2
    };

    class Subtree;

    




    struct Row {
        nsTemplateMatch* mMatch;
        ContainerType    mContainerType  : 2;
        ContainerState   mContainerState : 2;
        ContainerFill    mContainerFill  : 2;
        
        Subtree*         mSubtree; 
    };

    



    class Subtree {
    protected:
        friend class nsTreeRows; 

        


        Subtree* mParent;

        


        PRInt32 mCount;

        


        PRInt32 mCapacity;

        



        PRInt32 mSubtreeSize;

        


        Row* mRows;

    public:
        


        Subtree(Subtree* aParent)
            : mParent(aParent),
              mCount(0),
              mCapacity(0),
              mSubtreeSize(0),
              mRows(nsnull) {}

        ~Subtree();

        


        PRInt32 Count() const { return mCount; }

        



        PRInt32 GetSubtreeSize() const { return mSubtreeSize; }

        


        const Row& operator[](PRInt32 aIndex) const {
            NS_PRECONDITION(aIndex >= 0 && aIndex < mCount, "bad index");
            return mRows[aIndex]; }

        


        Row& operator[](PRInt32 aIndex) {
            NS_PRECONDITION(aIndex >= 0 && aIndex < mCount, "bad index");
            return mRows[aIndex]; }

        


        void Clear();

    protected:
        


        iterator InsertRowAt(nsTemplateMatch* aMatch, PRInt32 aIndex);

        


        void RemoveRowAt(PRInt32 aChildIndex);
    };

    friend class Subtree;

    enum { kMaxDepth = 32 };

protected:
    


    struct Link {
        Subtree* mParent;
        PRInt32  mChildIndex;

        Link&
        operator=(const Link& aLink) {
            mParent = aLink.mParent;
            mChildIndex = aLink.mChildIndex;
            return *this; }

        PRBool
        operator==(const Link& aLink) const {
            return (mParent == aLink.mParent)
                && (mChildIndex == aLink.mChildIndex); }

        Subtree* GetParent() { return mParent; }
        const Subtree* GetParent() const { return mParent; }

        PRInt32 GetChildIndex() const { return mChildIndex; }

        Row& GetRow() { return (*mParent)[mChildIndex]; }
        const Row& GetRow() const { return (*mParent)[mChildIndex]; }
    };

public:
    


    class iterator {
    protected:
        PRInt32 mTop;
        PRInt32 mRowIndex;
        Link    mLink[kMaxDepth];

        void Next();
        void Prev();

        friend class Subtree; 
        friend class nsTreeRows; 

        


        void Append(Subtree* aParent, PRInt32 aChildIndex);

        


        void Push(Subtree *aParent, PRInt32 aChildIndex);

        


        void SetRowIndex(PRInt32 aRowIndex) { mRowIndex = aRowIndex; }

    public:
        iterator() : mTop(-1), mRowIndex(-1) {}

        iterator(const iterator& aIterator);
        iterator& operator=(const iterator& aIterator);

        PRBool operator==(const iterator& aIterator) const;

        PRBool operator!=(const iterator& aIterator) const {
            return !aIterator.operator==(*this); }

        const Row& operator*() const { return mLink[mTop].GetRow(); }
        Row& operator*() { return mLink[mTop].GetRow(); }

        const Row* operator->() const { return &(mLink[mTop].GetRow()); }
        Row* operator->() { return &(mLink[mTop].GetRow()); }

        iterator& operator++() { Next(); return *this; }
        iterator operator++(int) { iterator temp(*this); Next(); return temp; }
        iterator& operator--() { Prev(); return *this; }
        iterator operator--(int) { iterator temp(*this); Prev(); return temp; }

        


        Subtree* GetParent() {
            return mLink[mTop].GetParent(); }

        const Subtree* GetParent() const {
            return mLink[mTop].GetParent(); }

        


        PRInt32 GetChildIndex() const {
            return mLink[mTop].GetChildIndex(); }

        



        PRInt32 GetDepth() const { return mTop + 1; }

        


        PRInt32 GetRowIndex() const { return mRowIndex; }

        


        iterator& Pop() { --mTop; return *this; }
    };

    


    iterator First();

    


    iterator Last();

    


    iterator FindByResource(nsIRDFResource* aResource);

    


    iterator Find(nsIXULTemplateResult* aResult);

    


    iterator operator[](PRInt32 aIndex);

    nsTreeRows() : mRoot(nsnull) {}
    ~nsTreeRows() {}

    




    Subtree*
    EnsureSubtreeFor(Subtree* aParent, PRInt32 aChildIndex);

    


    Subtree*
    EnsureSubtreeFor(iterator& aIterator) {
        return EnsureSubtreeFor(aIterator.GetParent(),
                                aIterator.GetChildIndex()); }

    




    Subtree*
    GetSubtreeFor(const Subtree* aParent,
                  PRInt32 aChildIndex,
                  PRInt32* aSubtreeSize = nsnull);

    


    PRInt32
    GetSubtreeSizeFor(const Subtree* aParent,
                      PRInt32 aChildIndex) {
        PRInt32 size;
        GetSubtreeFor(aParent, aChildIndex, &size);
        return size; }

    


    PRInt32
    GetSubtreeSizeFor(const iterator& aIterator) {
        PRInt32 size;
        GetSubtreeFor(aIterator.GetParent(), aIterator.GetChildIndex(), &size);
        return size; }

    



    void
    RemoveSubtreeFor(Subtree* aParent, PRInt32 aChildIndex);

    



    void
    RemoveSubtreeFor(iterator& aIterator) {
        RemoveSubtreeFor(aIterator.GetParent(), aIterator.GetChildIndex()); }

    


    PRInt32
    RemoveRowAt(iterator& aIterator) {
        iterator temp = aIterator--;
        Subtree* parent = temp.GetParent();
        parent->RemoveRowAt(temp.GetChildIndex());
        InvalidateCachedRow();
        return parent->Count(); }

    


    iterator
    InsertRowAt(nsTemplateMatch* aMatch, Subtree* aSubtree, PRInt32 aChildIndex) {
        InvalidateCachedRow();
        return aSubtree->InsertRowAt(aMatch, aChildIndex); }

    


    Row*
    GetRowsFor(Subtree* aSubtree) { return aSubtree->mRows; }

    


    void Clear();

    


    PRInt32 Count() const { return mRoot.GetSubtreeSize(); }

    


    Subtree* GetRoot() { return &mRoot; }

    


    void SetRootResource(nsIRDFResource* aResource) {
        mRootResource = aResource; }

    


    nsIRDFResource* GetRootResource() {
        return mRootResource.get(); }

    



    void
    InvalidateCachedRow() { mLastRow = iterator(); }

protected:
    


    Subtree mRoot;

    


    nsCOMPtr<nsIRDFResource> mRootResource;

    




    iterator mLastRow;
};


#endif 
