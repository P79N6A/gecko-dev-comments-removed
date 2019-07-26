




#ifndef nsCOMArray_h__
#define nsCOMArray_h__

#include "mozilla/Attributes.h"

#include "nsVoidArray.h"
#include "nsISupports.h"





class NS_COM_GLUE nsCOMArray_base
{
    friend class nsArray;
protected:
    nsCOMArray_base() {}
    nsCOMArray_base(int32_t aCount) : mArray(aCount) {}
    nsCOMArray_base(const nsCOMArray_base& other);
    ~nsCOMArray_base();

    int32_t IndexOf(nsISupports* aObject) const {
        return mArray.IndexOf(aObject);
    }

    int32_t IndexOfObject(nsISupports* aObject) const;

    bool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData) {
        return mArray.EnumerateForwards(aFunc, aData);
    }
    
    bool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData) {
        return mArray.EnumerateBackwards(aFunc, aData);
    }
    
    void Sort(nsVoidArrayComparatorFunc aFunc, void* aData) {
        mArray.Sort(aFunc, aData);
    }
    
    
    
    
    void Clear();
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
    bool RemoveObjectAt(int32_t aIndex);
    bool RemoveObjectsAt(int32_t aIndex, int32_t aCount);

public:
    
    
    int32_t Count() const {
        return mArray.Count();
    }
    
    
    bool SetCount(int32_t aNewCount);

    nsISupports* ObjectAt(int32_t aIndex) const {
        return static_cast<nsISupports*>(mArray.FastElementAt(aIndex));
    }
    
    nsISupports* SafeObjectAt(int32_t aIndex) const {
        return static_cast<nsISupports*>(mArray.SafeElementAt(aIndex));
    }

    nsISupports* operator[](int32_t aIndex) const {
        return ObjectAt(aIndex);
    }

    
    
    bool SetCapacity(uint32_t aCapacity) {
      return aCapacity > 0 ? mArray.SizeTo(static_cast<int32_t>(aCapacity))
                           : true;
    }

    
    
    
    size_t SizeOfExcludingThis(
             nsVoidArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis,
             nsMallocSizeOfFun aMallocSizeOf, void* aData = NULL) const {
        return mArray.SizeOfExcludingThis(aSizeOfElementIncludingThis,
                                          aMallocSizeOf, aData);
    }
    
private:
    
    
    nsVoidArray mArray;

    
    nsCOMArray_base& operator=(const nsCOMArray_base& other) MOZ_DELETE;
};



















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

    
    

    
    int32_t Count() const {
        return nsCOMArray_base::Count();
    }

    
    void Clear() {
        nsCOMArray_base::Clear();
    }

    
    
    
    typedef bool (* nsCOMArrayEnumFunc)
        (T* aElement, void *aData);
    
    
    bool EnumerateForwards(nsCOMArrayEnumFunc aFunc, void* aData) {
        return nsCOMArray_base::EnumerateForwards(nsVoidArrayEnumFunc(aFunc),
                                                  aData);
    }

    bool EnumerateBackwards(nsCOMArrayEnumFunc aFunc, void* aData) {
        return nsCOMArray_base::EnumerateBackwards(nsVoidArrayEnumFunc(aFunc),
                                                  aData);
    }
    
    typedef int (* nsCOMArrayComparatorFunc)
        (T* aElement1, T* aElement2, void* aData);
        
    void Sort(nsCOMArrayComparatorFunc aFunc, void* aData) {
        nsCOMArray_base::Sort(nsVoidArrayComparatorFunc(aFunc), aData);
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

    
    
    bool RemoveObjectAt(int32_t aIndex) {
        return nsCOMArray_base::RemoveObjectAt(aIndex);
    }

    
    
    bool RemoveObjectsAt(int32_t aIndex, int32_t aCount) {
        return nsCOMArray_base::RemoveObjectsAt(aIndex, aCount);
    }

    
    
    
    typedef size_t (* nsCOMArraySizeOfElementIncludingThisFunc)
        (T* aElement, nsMallocSizeOfFun aMallocSizeOf, void *aData);
    
    size_t SizeOfExcludingThis(
             nsCOMArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis, 
             nsMallocSizeOfFun aMallocSizeOf, void *aData = NULL) const {
        return nsCOMArray_base::SizeOfExcludingThis(
                 nsVoidArraySizeOfElementIncludingThisFunc(aSizeOfElementIncludingThis),
                 aMallocSizeOf, aData);
    }

private:

    
    nsCOMArray<T>& operator=(const nsCOMArray<T>& other) MOZ_DELETE;
};


#endif
