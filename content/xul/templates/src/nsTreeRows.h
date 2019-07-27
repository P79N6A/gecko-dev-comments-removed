




#ifndef nsTreeRows_h__
#define nsTreeRows_h__

#include "nsCOMPtr.h"
#include "nsTArray.h"
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
        eContainerType_Container    =  2
    };

    enum ContainerState {
        eContainerState_Unknown     =  0,
        eContainerState_Open        =  1,
        eContainerState_Closed      =  2
    };

    enum ContainerFill {
        eContainerFill_Unknown      =  0,
        eContainerFill_Empty        =  1,
        eContainerFill_Nonempty     =  2
    };

    class Subtree;

    




    struct Row {
        nsTemplateMatch* mMatch;
        ContainerType    mContainerType  : 4;
        ContainerState   mContainerState : 4;
        ContainerFill    mContainerFill  : 4;
        
        Subtree*         mSubtree; 
    };

    



    class Subtree {
    protected:
        friend class nsTreeRows; 

        


        Subtree* mParent;

        


        int32_t mCount;

        


        int32_t mCapacity;

        



        int32_t mSubtreeSize;

        


        Row* mRows;

    public:
        


        explicit Subtree(Subtree* aParent)
            : mParent(aParent),
              mCount(0),
              mCapacity(0),
              mSubtreeSize(0),
              mRows(nullptr) {}

        ~Subtree();

        


        int32_t Count() const { return mCount; }

        



        int32_t GetSubtreeSize() const { return mSubtreeSize; }

        


        const Row& operator[](int32_t aIndex) const {
            NS_PRECONDITION(aIndex >= 0 && aIndex < mCount, "bad index");
            return mRows[aIndex]; }

        


        Row& operator[](int32_t aIndex) {
            NS_PRECONDITION(aIndex >= 0 && aIndex < mCount, "bad index");
            return mRows[aIndex]; }

        


        void Clear();

    protected:
        


        iterator InsertRowAt(nsTemplateMatch* aMatch, int32_t aIndex);

        


        void RemoveRowAt(int32_t aChildIndex);
    };

    friend class Subtree;

protected:
    


    struct Link {
        Subtree* mParent;
        int32_t  mChildIndex;

        Link&
        operator=(const Link& aLink) {
            mParent = aLink.mParent;
            mChildIndex = aLink.mChildIndex;
            return *this; }

        bool
        operator==(const Link& aLink) const {
            return (mParent == aLink.mParent)
                && (mChildIndex == aLink.mChildIndex); }

        Subtree* GetParent() { return mParent; }
        const Subtree* GetParent() const { return mParent; }

        int32_t GetChildIndex() const { return mChildIndex; }

        Row& GetRow() { return (*mParent)[mChildIndex]; }
        const Row& GetRow() const { return (*mParent)[mChildIndex]; }
    };

public:
    


    class iterator {
    protected:
        int32_t mRowIndex;
        nsAutoTArray<Link, 8> mLink;

        void Next();
        void Prev();

        friend class Subtree; 
        friend class nsTreeRows; 

        


        void Append(Subtree* aParent, int32_t aChildIndex);

        


        void Push(Subtree *aParent, int32_t aChildIndex);

        


        void SetRowIndex(int32_t aRowIndex) { mRowIndex = aRowIndex; }

        


        Link& GetTop() { return mLink[mLink.Length() - 1]; }
        const Link& GetTop() const { return mLink[mLink.Length() - 1]; }

    public:
        iterator() : mRowIndex(-1) {}

        iterator(const iterator& aIterator);
        iterator& operator=(const iterator& aIterator);

        bool operator==(const iterator& aIterator) const;

        bool operator!=(const iterator& aIterator) const {
            return !aIterator.operator==(*this); }

        const Row& operator*() const { return GetTop().GetRow(); }
        Row& operator*() { return GetTop().GetRow(); }

        const Row* operator->() const { return &(GetTop().GetRow()); }
        Row* operator->() { return &(GetTop().GetRow()); }

        iterator& operator++() { Next(); return *this; }
        iterator operator++(int) { iterator temp(*this); Next(); return temp; }
        iterator& operator--() { Prev(); return *this; }
        iterator operator--(int) { iterator temp(*this); Prev(); return temp; }

        


        Subtree* GetParent() { return GetTop().GetParent(); }

        const Subtree* GetParent() const { return GetTop().GetParent(); }

        


        int32_t GetChildIndex() const { return GetTop().GetChildIndex(); }

        



        int32_t GetDepth() const { return mLink.Length(); }

        


        int32_t GetRowIndex() const { return mRowIndex; }

        


        iterator& Pop() { mLink.SetLength(GetDepth() - 1); return *this; }
    };

    


    iterator First();

    


    iterator Last();

    


    iterator FindByResource(nsIRDFResource* aResource);

    


    iterator Find(nsIXULTemplateResult* aResult);

    


    iterator operator[](int32_t aIndex);

    nsTreeRows() : mRoot(nullptr) {}
    ~nsTreeRows() {}

    




    Subtree*
    EnsureSubtreeFor(Subtree* aParent, int32_t aChildIndex);

    


    Subtree*
    EnsureSubtreeFor(iterator& aIterator) {
        return EnsureSubtreeFor(aIterator.GetParent(),
                                aIterator.GetChildIndex()); }

    




    Subtree*
    GetSubtreeFor(const Subtree* aParent,
                  int32_t aChildIndex,
                  int32_t* aSubtreeSize = nullptr);

    


    int32_t
    GetSubtreeSizeFor(const Subtree* aParent,
                      int32_t aChildIndex) {
        int32_t size;
        GetSubtreeFor(aParent, aChildIndex, &size);
        return size; }

    


    int32_t
    GetSubtreeSizeFor(const iterator& aIterator) {
        int32_t size;
        GetSubtreeFor(aIterator.GetParent(), aIterator.GetChildIndex(), &size);
        return size; }

    



    void
    RemoveSubtreeFor(Subtree* aParent, int32_t aChildIndex);

    



    void
    RemoveSubtreeFor(iterator& aIterator) {
        RemoveSubtreeFor(aIterator.GetParent(), aIterator.GetChildIndex()); }

    


    int32_t
    RemoveRowAt(iterator& aIterator) {
        iterator temp = aIterator--;
        Subtree* parent = temp.GetParent();
        parent->RemoveRowAt(temp.GetChildIndex());
        InvalidateCachedRow();
        return parent->Count(); }

    


    iterator
    InsertRowAt(nsTemplateMatch* aMatch, Subtree* aSubtree, int32_t aChildIndex) {
        InvalidateCachedRow();
        return aSubtree->InsertRowAt(aMatch, aChildIndex); }

    


    Row*
    GetRowsFor(Subtree* aSubtree) { return aSubtree->mRows; }

    


    void Clear();

    


    int32_t Count() const { return mRoot.GetSubtreeSize(); }

    


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
