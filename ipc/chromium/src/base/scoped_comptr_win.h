



#ifndef BASE_SCOPED_COMPTR_WIN_H_
#define BASE_SCOPED_COMPTR_WIN_H_

#include <unknwn.h>

#include "base/logging.h"
#include "base/ref_counted.h"



template <class Interface>
class BlockIUnknownMethods : public Interface {
 private:
  STDMETHOD(QueryInterface)(REFIID iid, void** object) = 0;
  STDMETHOD_(ULONG, AddRef)() = 0;
  STDMETHOD_(ULONG, Release)() = 0;
};




template <class Interface, const IID* interface_id = &__uuidof(Interface)>
class ScopedComPtr : public scoped_refptr<Interface> {
 public:
  typedef scoped_refptr<Interface> ParentClass;

  ScopedComPtr() {
  }

  explicit ScopedComPtr(Interface* p) : ParentClass(p) {
  }

  explicit ScopedComPtr(const ScopedComPtr<Interface, interface_id>& p)
      : ParentClass(p) {
  }

  ~ScopedComPtr() {
    
    
    COMPILE_ASSERT(sizeof(ScopedComPtr<Interface, interface_id>) ==
                   sizeof(Interface*), ScopedComPtrSize);
  }

  
  
  
  
  void Release() {
    if (ptr_ != NULL) {
      ptr_->Release();
      ptr_ = NULL;
    }
  }

  
  
  Interface* Detach() {
    Interface* p = ptr_;
    ptr_ = NULL;
    return p;
  }

  
  void Attach(Interface* p) {
    DCHECK(ptr_ == NULL);
    ptr_ = p;
  }

  
  
  
  
  Interface** Receive() {
    DCHECK(ptr_ == NULL) << "Object leak. Pointer must be NULL";
    return &ptr_;
  }

  template <class Query>
  HRESULT QueryInterface(Query** p) {
    DCHECK(p != NULL);
    DCHECK(ptr_ != NULL);
    
    
    
    return ptr_->QueryInterface(p);
  }

  
  
  HRESULT QueryFrom(IUnknown* object) {
    DCHECK(object != NULL);
    return object->QueryInterface(Receive());
  }

  
  HRESULT CreateInstance(const CLSID& clsid, IUnknown* outer = NULL,
                         DWORD context = CLSCTX_ALL) {
    DCHECK(ptr_ == NULL);
    HRESULT hr = ::CoCreateInstance(clsid, outer, context, *interface_id,
                                    reinterpret_cast<void**>(&ptr_));
    return hr;
  }

  
  bool IsSameObject(IUnknown* other) {
    if (!other && !ptr_)
      return true;

    if (!other || !ptr_)
      return false;

    ScopedComPtr<IUnknown> my_identity;
    QueryInterface(my_identity.Receive());

    ScopedComPtr<IUnknown> other_identity;
    other->QueryInterface(other_identity.Receive());

    return static_cast<IUnknown*>(my_identity) ==
           static_cast<IUnknown*>(other_identity);
  }

  
  
  
  
  
  
  
  
  
  
  BlockIUnknownMethods<Interface>* operator->() const {
    DCHECK(ptr_ != NULL);
    return reinterpret_cast<BlockIUnknownMethods<Interface>*>(ptr_);
  }

  
  using scoped_refptr<Interface>::operator=;

  

  static const IID& iid() {
    return *interface_id;
  }
};

#endif  
