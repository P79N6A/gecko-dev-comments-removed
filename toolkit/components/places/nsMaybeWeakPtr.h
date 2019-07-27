




#ifndef nsMaybeWeakPtr_h_
#define nsMaybeWeakPtr_h_

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsTArray.h"
#include "nsCycleCollectionNoteChild.h"




class nsMaybeWeakPtr_base
{
protected:
  
  void* GetValueAs(const nsIID& iid) const;

  nsCOMPtr<nsISupports> mPtr;
};

template<class T>
class nsMaybeWeakPtr : private nsMaybeWeakPtr_base
{
public:
  MOZ_IMPLICIT nsMaybeWeakPtr(nsISupports *ref) { mPtr = ref; }
  MOZ_IMPLICIT nsMaybeWeakPtr(const nsCOMPtr<nsIWeakReference> &ref) { mPtr = ref; }
  MOZ_IMPLICIT nsMaybeWeakPtr(const nsCOMPtr<T> &ref) { mPtr = ref; }

  bool operator==(const nsMaybeWeakPtr<T> &other) const {
    return mPtr == other.mPtr;
  }

  operator const nsCOMPtr<T>() const { return GetValue(); }

protected:
  const nsCOMPtr<T> GetValue() const {
    return nsCOMPtr<T>(dont_AddRef(static_cast<T*>
                                              (GetValueAs(NS_GET_TEMPLATE_IID(T)))));
  }
};





typedef nsTArray< nsMaybeWeakPtr<nsISupports> > isupports_array_type;
nsresult NS_AppendWeakElementBase(isupports_array_type *aArray,
                                  nsISupports *aElement, bool aWeak);
nsresult NS_RemoveWeakElementBase(isupports_array_type *aArray,
                                  nsISupports *aElement);

template<class T>
class nsMaybeWeakPtrArray : public nsTArray< nsMaybeWeakPtr<T> >
{
public:
  nsresult AppendWeakElement(T *aElement, bool aOwnsWeak)
  {
    return NS_AppendWeakElementBase(
      reinterpret_cast<isupports_array_type*>(this), aElement, aOwnsWeak);
  }

  nsresult RemoveWeakElement(T *aElement)
  {
    return NS_RemoveWeakElementBase(
      reinterpret_cast<isupports_array_type*>(this), aElement);
  }
};

template <typename T>
inline void
ImplCycleCollectionUnlink(nsMaybeWeakPtrArray<T>& aField)
{
  aField.Clear();
}

template <typename E>
inline void
ImplCycleCollectionTraverse(nsCycleCollectionTraversalCallback& aCallback,
                            nsMaybeWeakPtrArray<E>& aField,
                            const char* aName,
                            uint32_t aFlags = 0)
{
  aFlags |= CycleCollectionEdgeNameArrayFlag;
  size_t length = aField.Length();
  for (size_t i = 0; i < length; ++i) {
    CycleCollectionNoteChild(aCallback, aField[i].get(), aName, aFlags);
  }
}




#define ENUMERATE_WEAKARRAY(array, type, method)                           \
  for (uint32_t array_idx = 0; array_idx < array.Length(); ++array_idx) {  \
    const nsCOMPtr<type> &e = array.ElementAt(array_idx);                  \
    if (e)                                                                 \
      e->method;                                                           \
  }

#endif
