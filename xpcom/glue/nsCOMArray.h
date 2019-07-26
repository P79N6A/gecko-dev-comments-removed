




#ifndef nsCOMArray_h__
#define nsCOMArray_h__

#include "mozilla/Attributes.h"

#include "nsCycleCollectionNoteChild.h"
#include "nsVoidArray.h"
#include "nsTArray.h"
#include "nsISupports.h"





class NS_COM_GLUE nsCOMArray_base
{
    friend class nsArray;
protected:
    nsCOMArray_base() {}
    nsCOMArray_base(int32_t aCount) : mArray(aCount) {}
    nsCOMArray_base(const nsCOMArray_base& other);
    ~nsCOMArray_base();

    int32_t IndexOf(nsISupports* aObject) const;
    int32_t IndexOfObject(nsISupports* aObject) const;

    typedef bool (* nsBaseArrayEnumFunc)
        (void* aElement, void *aData);
    
    
    bool EnumerateForwards(nsBaseArrayEnumFunc aFunc, void* aData) const;
    
    bool EnumerateBackwards(nsBaseArrayEnumFunc aFunc, void* aData) const;

    typedef int (* nsBaseArrayComparatorFunc)
        (nsISupports* aElement1, nsISupports* aElement2, void* aData);

    struct nsCOMArrayComparatorContext {
        nsBaseArrayComparatorFunc mComparatorFunc;
        void* mData;
    };

    static int nsCOMArrayComparator(const void* aElement1, const void* aElement2, void* aData);
    void Sort(nsBaseArrayComparatorFunc aFunc, void* aData);

    bool InsertObjectAt(nsISupports* aObject, int32_t aIndex);
    bool InsertObjectsAt(const nsCOMArray_base& aObjects, int32_t aIndex);
    bool ReplaceObjectAt(nsISupports* aObject, int32_t aIndex);
    bool AppendObject(nsISupports *aObject) {
        return InsertObjectAt(aObject, Count());
    }
    bool AppendObjects(const nsCOMArray_base& aObjects) {
        return InsertObjectsAt(aObjects, Count());
    }
    bool RemoveObject(nsISupports *aObject);

public:
    
    int32_t Count() const {
        return mArray.Length();
    }

    
    
    bool SetCount(int32_t aNewCount);

    
    void Clear();

    nsISupports* ObjectAt(int32_t aIndex) const {
        return mArray[aIndex];
    }
    
    nsISupports* SafeObjectAt(int32_t aIndex) const {
        return mArray.SafeElementAt(aIndex, nullptr);
    }

    nsISupports* operator[](int32_t aIndex) const {
        return ObjectAt(aIndex);
    }

    
    
    bool RemoveObjectAt(int32_t aIndex);

    
    
    bool RemoveObjectsAt(int32_t aIndex, int32_t aCount);

    
    
    bool SetCapacity(uint32_t aCapacity) {
      return mArray.SetCapacity(aCapacity);
    }

    typedef size_t (* nsBaseArraySizeOfElementIncludingThisFunc)
        (nsISupports* aElement, nsMallocSizeOfFun aMallocSizeOf, void *aData);

    
    
    
    size_t SizeOfExcludingThis(
             nsBaseArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis,
             nsMallocSizeOfFun aMallocSizeOf, void* aData = NULL) const;

private:
    
    
    nsTArray<nsISupports*> mArray;

    
    nsCOMArray_base& operator=(const nsCOMArray_base& other) MOZ_DELETE;
};

inline void
ImplCycleCollectionUnlink(nsCOMArray_base& aField)
{
    aField.Clear();
}

inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsCOMArray_base& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
    aFlags |= CycleCollectionEdgeNameArrayFlag;
    size_t length = aField.Count();
    for (size_t i = 0; i < length; ++i) {
        CycleCollectionNoteChild(aCallback, aField[i], aName, aFlags);
    }
}




















template <class T>
class nsCOMArray : public nsCOMArray_base
{
 public:
    nsCOMArray() {}
    nsCOMArray(int32_t aCount) : nsCOMArray_base(aCount) {}
    
    
    
    nsCOMArray(const nsCOMArray<T>& aOther) : nsCOMArray_base(aOther) { }

    ~nsCOMArray() {}

    
    T* ObjectAt(int32_t aIndex) const {
        return static_cast<T*>(nsCOMArray_base::ObjectAt(aIndex));
    }

    
    T* SafeObjectAt(int32_t aIndex) const {
        return static_cast<T*>(nsCOMArray_base::SafeObjectAt(aIndex));
    }

    
    T* operator[](int32_t aIndex) const {
        return ObjectAt(aIndex);
    }

    
    
    
    int32_t IndexOf(T* aObject) const {
        return nsCOMArray_base::IndexOf(static_cast<nsISupports*>(aObject));
    }

    
    
    
    
    
    int32_t IndexOfObject(T* aObject) const {
        return nsCOMArray_base::IndexOfObject(static_cast<nsISupports*>(aObject));
    }

    
    
    bool InsertObjectAt(T* aObject, int32_t aIndex) {
        return nsCOMArray_base::InsertObjectAt(static_cast<nsISupports*>(aObject), aIndex);
    }

    
    
    bool InsertObjectsAt(const nsCOMArray<T>& aObjects, int32_t aIndex) {
        return nsCOMArray_base::InsertObjectsAt(aObjects, aIndex);
    }

    
    
    bool ReplaceObjectAt(T* aObject, int32_t aIndex) {
        return nsCOMArray_base::ReplaceObjectAt(static_cast<nsISupports*>(aObject), aIndex);
    }

    
    
    
    typedef bool (* nsCOMArrayEnumFunc)
        (T* aElement, void *aData);
    
    
    bool EnumerateForwards(nsCOMArrayEnumFunc aFunc, void* aData) {
        return nsCOMArray_base::EnumerateForwards(nsBaseArrayEnumFunc(aFunc),
                                                  aData);
    }

    bool EnumerateBackwards(nsCOMArrayEnumFunc aFunc, void* aData) {
        return nsCOMArray_base::EnumerateBackwards(nsBaseArrayEnumFunc(aFunc),
                                                  aData);
    }
    
    typedef int (* nsCOMArrayComparatorFunc)
        (T* aElement1, T* aElement2, void* aData);
        
    void Sort(nsCOMArrayComparatorFunc aFunc, void* aData) {
        nsCOMArray_base::Sort(nsBaseArrayComparatorFunc(aFunc), aData);
    }

    
    bool AppendObject(T *aObject) {
        return nsCOMArray_base::AppendObject(static_cast<nsISupports*>(aObject));
    }

    
    bool AppendObjects(const nsCOMArray<T>& aObjects) {
        return nsCOMArray_base::AppendObjects(aObjects);
    }
    
    
    
    
    bool RemoveObject(T *aObject) {
        return nsCOMArray_base::RemoveObject(static_cast<nsISupports*>(aObject));
    }

    
    
    
    typedef size_t (* nsCOMArraySizeOfElementIncludingThisFunc)
        (T* aElement, nsMallocSizeOfFun aMallocSizeOf, void *aData);
    
    size_t SizeOfExcludingThis(
             nsCOMArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis, 
             nsMallocSizeOfFun aMallocSizeOf, void *aData = NULL) const {
        return nsCOMArray_base::SizeOfExcludingThis(
                 nsBaseArraySizeOfElementIncludingThisFunc(aSizeOfElementIncludingThis),
                 aMallocSizeOf, aData);
    }

private:

    
    nsCOMArray<T>& operator=(const nsCOMArray<T>& other) MOZ_DELETE;
};

template <typename T>
inline void
ImplCycleCollectionUnlink(nsCOMArray<T>& aField)
{
    aField.Clear();
}

template <typename E>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsCOMArray<E>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
    aFlags |= CycleCollectionEdgeNameArrayFlag;
    size_t length = aField.Count();
    for (size_t i = 0; i < length; ++i) {
        CycleCollectionNoteChild(aCallback, aField[i], aName, aFlags);
    }
}

#endif
