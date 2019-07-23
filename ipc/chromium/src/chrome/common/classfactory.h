






#ifndef _CLASSFACTORY_H_
#define _CLASSFACTORY_H_

#include <unknwn.h>





class GenericClassFactory : public IClassFactory {
public:
  GenericClassFactory();
  ~GenericClassFactory();

  
  STDMETHOD(QueryInterface)(REFIID iid, LPVOID* ppvObject);
  STDMETHOD_(ULONG, AddRef)();
  STDMETHOD_(ULONG, Release)();

  
  STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid,
                            LPVOID* ppvObject) = 0;
  STDMETHOD(LockServer)(BOOL fLock);

  
  static LONG GetObjectCount() { return object_count_; }

protected:
  LONG reference_count_; 
  static LONG object_count_; 
};




template <class T>
class OneClassFactory : public GenericClassFactory
{
public:
  
  STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObject);
};


template <class T>
STDMETHODIMP OneClassFactory<T>::CreateInstance(LPUNKNOWN pUnkOuter,
                                                REFIID riid, void** result) {
  *result = NULL;

  if(pUnkOuter != NULL)
    return CLASS_E_NOAGGREGATION;

  T* const obj = new T();
  if(!obj)
    return E_OUTOFMEMORY;

  obj->AddRef();
  HRESULT const hr = obj->QueryInterface(riid, result);
  obj->Release();

  return hr;
}

#endif
