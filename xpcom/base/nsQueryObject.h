





#ifndef nsQueryObject_h
#define nsQueryObject_h

#include "nsCOMPtr.h"
#include "nsRefPtr.h"



template<class T>
class nsQueryObject : public nsCOMPtr_helper
{
public:
  explicit nsQueryObject(T* aRawPtr)
    : mRawPtr(aRawPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID& aIID,
                                          void** aResult) const
  {
    nsresult status = mRawPtr ? mRawPtr->QueryInterface(aIID, aResult)
                              : NS_ERROR_NULL_POINTER;
    return status;
  }
private:
  T* mRawPtr;
};

template<class T>
class nsQueryObjectWithError : public nsCOMPtr_helper
{
public:
  nsQueryObjectWithError(T* aRawPtr, nsresult* aErrorPtr)
    : mRawPtr(aRawPtr), mErrorPtr(aErrorPtr)
  {
  }

  virtual nsresult NS_FASTCALL operator()(const nsIID& aIID,
                                          void** aResult) const
  {
    nsresult status = mRawPtr ? mRawPtr->QueryInterface(aIID, aResult)
                              : NS_ERROR_NULL_POINTER;
    if (mErrorPtr) {
      *mErrorPtr = status;
    }
    return status;
  }
private:
  T* mRawPtr;
  nsresult* mErrorPtr;
};





template<class T>
inline nsQueryObject<T>
do_QueryObject(T* aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline nsQueryObject<T>
do_QueryObject(nsCOMPtr<T>& aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline nsQueryObject<T>
do_QueryObject(nsRefPtr<T>& aRawPtr)
{
  return nsQueryObject<T>(aRawPtr);
}

template<class T>
inline nsQueryObjectWithError<T>
do_QueryObject(T* aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}

template<class T>
inline nsQueryObjectWithError<T>
do_QueryObject(nsCOMPtr<T>& aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}

template<class T>
inline nsQueryObjectWithError<T>
do_QueryObject(nsRefPtr<T>& aRawPtr, nsresult* aErrorPtr)
{
  return nsQueryObjectWithError<T>(aRawPtr, aErrorPtr);
}



#endif 
