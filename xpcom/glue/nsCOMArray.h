





































#ifndef nsCOMArray_h__
#define nsCOMArray_h__

#include "nsVoidArray.h"
#include "nsISupports.h"





class NS_COM_GLUE nsCOMArray_base
{
    friend class nsArray;
protected:
    nsCOMArray_base() {}
    nsCOMArray_base(PRInt32 aCount) : mArray(aCount) {}
    nsCOMArray_base(const nsCOMArray_base& other);
    ~nsCOMArray_base();

    PRInt32 IndexOf(nsISupports* aObject) const {
        return mArray.IndexOf(aObject);
    }

    PRInt32 IndexOfObject(nsISupports* aObject) const;

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
    bool InsertObjectAt(nsISupports* aObject, PRInt32 aIndex);
    bool InsertObjectsAt(const nsCOMArray_base& aObjects, PRInt32 aIndex);
    bool ReplaceObjectAt(nsISupports* aObject, PRInt32 aIndex);
    bool AppendObject(nsISupports *aObject) {
        return InsertObjectAt(aObject, Count());
    }
    bool AppendObjects(const nsCOMArray_base& aObjects) {
        return InsertObjectsAt(aObjects, Count());
    }
    bool RemoveObject(nsISupports *aObject);
    bool RemoveObjectAt(PRInt32 aIndex);
    bool RemoveObjectsAt(PRInt32 aIndex, PRInt32 aCount);

public:
    
    
    PRInt32 Count() const {
        return mArray.Count();
    }
    
    
    bool SetCount(PRInt32 aNewCount);

    nsISupports* ObjectAt(PRInt32 aIndex) const {
        return static_cast<nsISupports*>(mArray.FastElementAt(aIndex));
    }
    
    nsISupports* SafeObjectAt(PRInt32 aIndex) const {
        return static_cast<nsISupports*>(mArray.SafeElementAt(aIndex));
    }

    nsISupports* operator[](PRInt32 aIndex) const {
        return ObjectAt(aIndex);
    }

    
    
    bool SetCapacity(PRUint32 aCapacity) {
      return aCapacity > 0 ? mArray.SizeTo(static_cast<PRInt32>(aCapacity))
                           : true;
    }

private:
    
    
    nsVoidArray mArray;

    
    nsCOMArray_base& operator=(const nsCOMArray_base& other);
};



















template <class T>
class nsCOMArray : public nsCOMArray_base
{
 public:
    nsCOMArray() {}
    nsCOMArray(PRInt32 aCount) : nsCOMArray_base(aCount) {}
    
    
    
    nsCOMArray(const nsCOMArray<T>& aOther) : nsCOMArray_base(aOther) { }

    ~nsCOMArray() {}

    
    T* ObjectAt(PRInt32 aIndex) const {
        return static_cast<T*>(nsCOMArray_base::ObjectAt(aIndex));
    }

    
    T* SafeObjectAt(PRInt32 aIndex) const {
        return static_cast<T*>(nsCOMArray_base::SafeObjectAt(aIndex));
    }

    
    T* operator[](PRInt32 aIndex) const {
        return ObjectAt(aIndex);
    }

    
    
    
    PRInt32 IndexOf(T* aObject) const {
        return nsCOMArray_base::IndexOf(static_cast<nsISupports*>(aObject));
    }

    
    
    
    
    
    PRInt32 IndexOfObject(T* aObject) const {
        return nsCOMArray_base::IndexOfObject(static_cast<nsISupports*>(aObject));
    }

    
    
    bool InsertObjectAt(T* aObject, PRInt32 aIndex) {
        return nsCOMArray_base::InsertObjectAt(static_cast<nsISupports*>(aObject), aIndex);
    }

    
    
    bool InsertObjectsAt(const nsCOMArray<T>& aObjects, PRInt32 aIndex) {
        return nsCOMArray_base::InsertObjectsAt(aObjects, aIndex);
    }

    
    
    bool ReplaceObjectAt(T* aObject, PRInt32 aIndex) {
        return nsCOMArray_base::ReplaceObjectAt(static_cast<nsISupports*>(aObject), aIndex);
    }

    
    

    
    PRInt32 Count() const {
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

    
    
    bool RemoveObjectAt(PRInt32 aIndex) {
        return nsCOMArray_base::RemoveObjectAt(aIndex);
    }

    
    
    bool RemoveObjectsAt(PRInt32 aIndex, PRInt32 aCount) {
        return nsCOMArray_base::RemoveObjectsAt(aIndex, aCount);
    }

private:

    
    nsCOMArray<T>& operator=(const nsCOMArray<T>& other);
};


#endif
