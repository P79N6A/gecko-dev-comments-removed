





































#ifndef nsMaybeWeakPtr_h_
#define nsMaybeWeakPtr_h_

#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsTArray.h"

#ifdef MOZILLA_1_8_BRANCH
#define NS_GET_TEMPLATE_IID NS_GET_IID
#endif




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
  nsMaybeWeakPtr(nsISupports *ref) { mPtr = ref; }
  nsMaybeWeakPtr(const nsCOMPtr<nsIWeakReference> &ref) { mPtr = ref; }
  nsMaybeWeakPtr(const nsCOMPtr<T> &ref) { mPtr = ref; }

  PRBool operator==(const nsMaybeWeakPtr<T> &other) const {
    return mPtr == other.mPtr;
  }

  operator const nsCOMPtr<T>() const { return GetValue(); }

protected:
  const nsCOMPtr<T> GetValue() const {
    return nsCOMPtr<T>(dont_AddRef(NS_STATIC_CAST(T*,
                                                  GetValueAs(NS_GET_TEMPLATE_IID(T)))));
  }
};





class nsMaybeWeakPtrArray_base
{
protected:
  static nsresult AppendWeakElementBase(nsTArray_base *aArray,
                                        nsISupports *aElement, PRBool aWeak);
  static nsresult RemoveWeakElementBase(nsTArray_base *aArray,
                                         nsISupports *aElement);

  typedef nsTArray< nsMaybeWeakPtr<nsISupports> > isupports_type;
};

template<class T>
class nsMaybeWeakPtrArray : public nsTArray< nsMaybeWeakPtr<T> >,
                            private nsMaybeWeakPtrArray_base
{
public:
  nsresult AppendWeakElement(T *aElement, PRBool aOwnsWeak)
  {
    return AppendWeakElementBase(this, aElement, aOwnsWeak);
  }

  nsresult RemoveWeakElement(T *aElement)
  {
    return RemoveWeakElementBase(this, aElement);
  }
};




#define ENUMERATE_WEAKARRAY(array, type, method)                           \
  for (PRUint32 array_idx = 0; array_idx < array.Length(); ++array_idx) {  \
    const nsCOMPtr<type> &e = array.ElementAt(array_idx);                  \
    if (e)                                                                 \
      e->method;                                                           \
  }

#endif
