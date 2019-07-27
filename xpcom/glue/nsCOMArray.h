





#ifndef nsCOMArray_h__
#define nsCOMArray_h__

#include "mozilla/Attributes.h"
#include "mozilla/MemoryReporting.h"

#include "nsCycleCollectionNoteChild.h"
#include "nsTArray.h"
#include "nsISupports.h"





class nsCOMArray_base
{
  friend class nsArrayBase;
protected:
  nsCOMArray_base() {}
  explicit nsCOMArray_base(int32_t aCount) : mArray(aCount) {}
  nsCOMArray_base(const nsCOMArray_base& aOther);
  ~nsCOMArray_base();

  int32_t IndexOf(nsISupports* aObject, uint32_t aStartIndex = 0) const;
  bool Contains(nsISupports* aObject) const
  {
    return IndexOf(aObject) != -1;
  }

  int32_t IndexOfObject(nsISupports* aObject) const;
  bool ContainsObject(nsISupports* aObject) const
  {
    return IndexOfObject(aObject) != -1;
  }

  typedef bool (*nsBaseArrayEnumFunc)(void* aElement, void* aData);

  
  bool EnumerateForwards(nsBaseArrayEnumFunc aFunc, void* aData) const;

  bool EnumerateBackwards(nsBaseArrayEnumFunc aFunc, void* aData) const;

  typedef int (*nsBaseArrayComparatorFunc)(nsISupports* aElement1,
                                           nsISupports* aElement2,
                                           void* aData);

  struct nsCOMArrayComparatorContext
  {
    nsBaseArrayComparatorFunc mComparatorFunc;
    void* mData;
  };

  static int nsCOMArrayComparator(const void* aElement1, const void* aElement2,
                                  void* aData);
  void Sort(nsBaseArrayComparatorFunc aFunc, void* aData);

  bool InsertObjectAt(nsISupports* aObject, int32_t aIndex);
  void InsertElementAt(uint32_t aIndex, nsISupports* aElement);
  bool InsertObjectsAt(const nsCOMArray_base& aObjects, int32_t aIndex);
  void InsertElementsAt(uint32_t aIndex, const nsCOMArray_base& aElements);
  void InsertElementsAt(uint32_t aIndex, nsISupports* const* aElements,
                        uint32_t aCount);
  bool ReplaceObjectAt(nsISupports* aObject, int32_t aIndex);
  void ReplaceElementAt(uint32_t aIndex, nsISupports* aElement)
  {
    nsISupports* oldElement = mArray[aIndex];
    NS_IF_ADDREF(mArray[aIndex] = aElement);
    NS_IF_RELEASE(oldElement);
  }
  bool AppendObject(nsISupports* aObject)
  {
    return InsertObjectAt(aObject, Count());
  }
  void AppendElement(nsISupports* aElement)
  {
    InsertElementAt(Length(), aElement);
  }
  bool AppendObjects(const nsCOMArray_base& aObjects)
  {
    return InsertObjectsAt(aObjects, Count());
  }
  void AppendElements(const nsCOMArray_base& aElements)
  {
    return InsertElementsAt(Length(), aElements);
  }
  void AppendElements(nsISupports* const* aElements, uint32_t aCount)
  {
    return InsertElementsAt(Length(), aElements, aCount);
  }
  bool RemoveObject(nsISupports* aObject);
  nsISupports** Elements() { return mArray.Elements(); }
  void SwapElements(nsCOMArray_base& aOther)
  {
    mArray.SwapElements(aOther.mArray);
  }

  void Adopt(nsISupports** aElements, uint32_t aCount);
  uint32_t Forget(nsISupports*** aElements);
public:
  
  int32_t Count() const { return mArray.Length(); }
  
  uint32_t Length() const { return mArray.Length(); }
  bool IsEmpty() const { return mArray.IsEmpty(); }

  
  
  bool SetCount(int32_t aNewCount);
  
  void TruncateLength(uint32_t aNewLength)
  {
    if (mArray.Length() > aNewLength) {
      RemoveElementsAt(aNewLength, mArray.Length() - aNewLength);
    }
  }

  
  void Clear();

  nsISupports* ObjectAt(int32_t aIndex) const { return mArray[aIndex]; }
  
  nsISupports* ElementAt(uint32_t aIndex) const { return mArray[aIndex]; }

  nsISupports* SafeObjectAt(int32_t aIndex) const
  {
    return mArray.SafeElementAt(aIndex, nullptr);
  }
  
  nsISupports* SafeElementAt(uint32_t aIndex) const
  {
    return mArray.SafeElementAt(aIndex, nullptr);
  }

  nsISupports* operator[](int32_t aIndex) const { return mArray[aIndex]; }

  
  
  bool RemoveObjectAt(int32_t aIndex);
  
  void RemoveElementAt(uint32_t aIndex);

  
  
  bool RemoveObjectsAt(int32_t aIndex, int32_t aCount);
  
  void RemoveElementsAt(uint32_t aIndex, uint32_t aCount);

  void SwapElementsAt(uint32_t aIndex1, uint32_t aIndex2)
  {
    nsISupports* tmp = mArray[aIndex1];
    mArray[aIndex1] = mArray[aIndex2];
    mArray[aIndex2] = tmp;
  }

  
  
  void SetCapacity(uint32_t aCapacity) { mArray.SetCapacity(aCapacity); }
  uint32_t Capacity() { return mArray.Capacity(); }

  typedef size_t (*nsBaseArraySizeOfElementIncludingThisFunc)(
    nsISupports* aElement, mozilla::MallocSizeOf aMallocSizeOf, void* aData);

  
  
  
  size_t SizeOfExcludingThis(
    nsBaseArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis,
    mozilla::MallocSizeOf aMallocSizeOf, void* aData = nullptr) const;

private:

  
  nsTArray<nsISupports*> mArray;

  
  nsCOMArray_base& operator=(const nsCOMArray_base& aOther) MOZ_DELETE;
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
  int32_t length = aField.Count();
  for (int32_t i = 0; i < length; ++i) {
    CycleCollectionNoteChild(aCallback, aField[i], aName, aFlags);
  }
}




















template<class T>
class nsCOMArray : public nsCOMArray_base
{
public:
  nsCOMArray() {}
  explicit nsCOMArray(int32_t aCount) : nsCOMArray_base(aCount) {}
  explicit nsCOMArray(const nsCOMArray<T>& aOther) : nsCOMArray_base(aOther) {}
  nsCOMArray(nsCOMArray<T>&& aOther) { SwapElements(aOther); }
  ~nsCOMArray() {}

  
  nsCOMArray<T>& operator=(nsCOMArray<T> && aOther)
  {
    SwapElements(aOther);
    return *this;
  }

  
  T* ObjectAt(int32_t aIndex) const
  {
    return static_cast<T*>(nsCOMArray_base::ObjectAt(aIndex));
  }
  
  T* ElementAt(uint32_t aIndex) const
  {
    return static_cast<T*>(nsCOMArray_base::ElementAt(aIndex));
  }

  
  T* SafeObjectAt(int32_t aIndex) const
  {
    return static_cast<T*>(nsCOMArray_base::SafeObjectAt(aIndex));
  }
  
  T* SafeElementAt(uint32_t aIndex) const
  {
    return static_cast<T*>(nsCOMArray_base::SafeElementAt(aIndex));
  }

  
  T* operator[](int32_t aIndex) const { return ObjectAt(aIndex); }

  
  
  
  int32_t IndexOf(T* aObject, uint32_t aStartIndex = 0) const
  {
    return nsCOMArray_base::IndexOf(aObject, aStartIndex);
  }
  bool Contains(T* aObject) const
  {
    return nsCOMArray_base::Contains(aObject);
  }

  
  
  
  
  
  int32_t IndexOfObject(T* aObject) const
  {
    return nsCOMArray_base::IndexOfObject(aObject);
  }
  bool ContainsObject(nsISupports* aObject) const
  {
    return nsCOMArray_base::ContainsObject(aObject);
  }

  
  
  bool InsertObjectAt(T* aObject, int32_t aIndex)
  {
    return nsCOMArray_base::InsertObjectAt(aObject, aIndex);
  }
  
  void InsertElementAt(uint32_t aIndex, T* aElement)
  {
    nsCOMArray_base::InsertElementAt(aIndex, aElement);
  }

  
  
  bool InsertObjectsAt(const nsCOMArray<T>& aObjects, int32_t aIndex)
  {
    return nsCOMArray_base::InsertObjectsAt(aObjects, aIndex);
  }
  
  void InsertElementsAt(uint32_t aIndex, const nsCOMArray<T>& aElements)
  {
    nsCOMArray_base::InsertElementsAt(aIndex, aElements);
  }
  void InsertElementsAt(uint32_t aIndex, T* const* aElements, uint32_t aCount)
  {
    nsCOMArray_base::InsertElementsAt(
      aIndex, reinterpret_cast<nsISupports* const*>(aElements), aCount);
  }

  
  
  bool ReplaceObjectAt(T* aObject, int32_t aIndex)
  {
    return nsCOMArray_base::ReplaceObjectAt(aObject, aIndex);
  }
  
  void ReplaceElementAt(uint32_t aIndex, T* aElement)
  {
    nsCOMArray_base::ReplaceElementAt(aIndex, aElement);
  }

  
  
  
  typedef bool (*nsCOMArrayEnumFunc)(T* aElement, void* aData);

  
  bool EnumerateForwards(nsCOMArrayEnumFunc aFunc, void* aData)
  {
    return nsCOMArray_base::EnumerateForwards(nsBaseArrayEnumFunc(aFunc),
                                              aData);
  }

  bool EnumerateBackwards(nsCOMArrayEnumFunc aFunc, void* aData)
  {
    return nsCOMArray_base::EnumerateBackwards(nsBaseArrayEnumFunc(aFunc),
                                               aData);
  }

  typedef int (*nsCOMArrayComparatorFunc)(T* aElement1, T* aElement2,
                                          void* aData);

  void Sort(nsCOMArrayComparatorFunc aFunc, void* aData)
  {
    nsCOMArray_base::Sort(nsBaseArrayComparatorFunc(aFunc), aData);
  }

  
  bool AppendObject(T* aObject)
  {
    return nsCOMArray_base::AppendObject(aObject);
  }
  
  void AppendElement(T* aElement)
  {
    nsCOMArray_base::AppendElement(aElement);
  }

  
  bool AppendObjects(const nsCOMArray<T>& aObjects)
  {
    return nsCOMArray_base::AppendObjects(aObjects);
  }
  
  void AppendElements(const nsCOMArray<T>& aElements)
  {
    return nsCOMArray_base::AppendElements(aElements);
  }
  void AppendElements(T* const* aElements, uint32_t aCount)
  {
    InsertElementsAt(Length(), aElements, aCount);
  }

  
  
  
  bool RemoveObject(T* aObject)
  {
    return nsCOMArray_base::RemoveObject(aObject);
  }
  
  bool RemoveElement(T* aElement)
  {
    return nsCOMArray_base::RemoveObject(aElement);
  }

  T** Elements()
  {
    return reinterpret_cast<T**>(nsCOMArray_base::Elements());
  }
  void SwapElements(nsCOMArray<T>& aOther)
  {
    nsCOMArray_base::SwapElements(aOther);
  }

  
  
  
  typedef size_t (*nsCOMArraySizeOfElementIncludingThisFunc)(
    T* aElement, mozilla::MallocSizeOf aMallocSizeOf, void* aData);

  size_t SizeOfExcludingThis(
      nsCOMArraySizeOfElementIncludingThisFunc aSizeOfElementIncludingThis,
      mozilla::MallocSizeOf aMallocSizeOf, void* aData = nullptr) const
  {
    return nsCOMArray_base::SizeOfExcludingThis(
      nsBaseArraySizeOfElementIncludingThisFunc(aSizeOfElementIncludingThis),
      aMallocSizeOf, aData);
  }

  










  void Adopt(T** aElements, uint32_t aSize)
  {
    nsCOMArray_base::Adopt(reinterpret_cast<nsISupports**>(aElements), aSize);
  }

  







  uint32_t Forget(T*** aElements)
  {
    return nsCOMArray_base::Forget(reinterpret_cast<nsISupports***>(aElements));
  }

private:

  
  nsCOMArray<T>& operator=(const nsCOMArray<T>& aOther) MOZ_DELETE;
};

template<typename T>
inline void
ImplCycleCollectionUnlink(nsCOMArray<T>& aField)
{
  aField.Clear();
}

template<typename E>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsCOMArray<E>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  aFlags |= CycleCollectionEdgeNameArrayFlag;
  int32_t length = aField.Count();
  for (int32_t i = 0; i < length; ++i) {
    CycleCollectionNoteChild(aCallback, aField[i], aName, aFlags);
  }
}

#endif
