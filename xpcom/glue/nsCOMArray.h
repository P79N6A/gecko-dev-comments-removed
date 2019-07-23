





































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

    PRBool EnumerateForwards(nsVoidArrayEnumFunc aFunc, void* aData) {
        return mArray.EnumerateForwards(aFunc, aData);
    }
    
    PRBool EnumerateBackwards(nsVoidArrayEnumFunc aFunc, void* aData) {
        return mArray.EnumerateBackwards(aFunc, aData);
    }
    
    void Sort(nsVoidArrayComparatorFunc aFunc, void* aData) {
        mArray.Sort(aFunc, aData);
    }
    
    
    
    
    void Clear();
    PRBool InsertObjectAt(nsISupports* aObject, PRInt32 aIndex);
    PRBool InsertObjectsAt(const nsCOMArray_base& aObjects, PRInt32 aIndex);
    PRBool ReplaceObjectAt(nsISupports* aObject, PRInt32 aIndex);
    PRBool AppendObject(nsISupports *aObject) {
        return InsertObjectAt(aObject, Count());
    }
    PRBool AppendObjects(const nsCOMArray_base& aObjects) {
        return InsertObjectsAt(aObjects, Count());
    }
    PRBool RemoveObject(nsISupports *aObject);
    PRBool RemoveObjectAt(PRInt32 aIndex);

public:
    
    
    PRInt32 Count() const {
        return mArray.Count();
    }

    nsISupports* ObjectAt(PRInt32 aIndex) const {
        return static_cast<nsISupports*>(mArray.FastElementAt(aIndex));
    }
    
    nsISupports* SafeObjectAt(PRInt32 aIndex) const {
        return static_cast<nsISupports*>(mArray.SafeElementAt(aIndex));
    }

    nsISupports* operator[](PRInt32 aIndex) const {
        return ObjectAt(aIndex);
    }

    
    
    PRBool SetCapacity(PRUint32 aCapacity) {
      return aCapacity > 0 ? mArray.SizeTo(static_cast<PRInt32>(aCapacity))
                           : PR_TRUE;
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

    
    
    PRBool InsertObjectAt(T* aObject, PRInt32 aIndex) {
        return nsCOMArray_base::InsertObjectAt(static_cast<nsISupports*>(aObject), aIndex);
    }

    
    
    PRBool InsertObjectsAt(const nsCOMArray<T>& aObjects, PRInt32 aIndex) {
        return nsCOMArray_base::InsertObjectsAt(aObjects, aIndex);
    }

    
    
    PRBool ReplaceObjectAt(T* aObject, PRInt32 aIndex) {
        return nsCOMArray_base::ReplaceObjectAt(static_cast<nsISupports*>(aObject), aIndex);
    }

    
    

    
    PRInt32 Count() const {
        return nsCOMArray_base::Count();
    }

    
    void Clear() {
        nsCOMArray_base::Clear();
    }

    
    
    
    typedef PRBool (* PR_CALLBACK nsCOMArrayEnumFunc)
        (T* aElement, void *aData);
    
    
    PRBool EnumerateForwards(nsCOMArrayEnumFunc aFunc, void* aData) {
        return nsCOMArray_base::EnumerateForwards(nsVoidArrayEnumFunc(aFunc),
                                                  aData);
    }

    PRBool EnumerateBackwards(nsCOMArrayEnumFunc aFunc, void* aData) {
        return nsCOMArray_base::EnumerateBackwards(nsVoidArrayEnumFunc(aFunc),
                                                  aData);
    }
    
    typedef int (* PR_CALLBACK nsCOMArrayComparatorFunc)
        (T* aElement1, T* aElement2, void* aData);
        
    void Sort(nsCOMArrayComparatorFunc aFunc, void* aData) {
        nsCOMArray_base::Sort(nsVoidArrayComparatorFunc(aFunc), aData);
    }

    
    PRBool AppendObject(T *aObject) {
        return nsCOMArray_base::AppendObject(static_cast<nsISupports*>(aObject));
    }

    
    PRBool AppendObjects(const nsCOMArray<T>& aObjects) {
        return nsCOMArray_base::AppendObjects(aObjects);
    }
    
    
    
    
    PRBool RemoveObject(T *aObject) {
        return nsCOMArray_base::RemoveObject(static_cast<nsISupports*>(aObject));
    }

    
    
    PRBool RemoveObjectAt(PRInt32 aIndex) {
        return nsCOMArray_base::RemoveObjectAt(aIndex);
    }

private:

    
    nsCOMArray<T>& operator=(const nsCOMArray<T>& other);
};


#endif
