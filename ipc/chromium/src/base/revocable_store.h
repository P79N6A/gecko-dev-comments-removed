



#ifndef BASE_REVOCABLE_STORE_H_
#define BASE_REVOCABLE_STORE_H_

#include "base/basictypes.h"
#include "nsISupportsImpl.h"
#include "nsAutoPtr.h"


class RevocableStore {
 public:
  
  
  
  
  
  class StoreRef MOZ_FINAL {
   public:
    NS_INLINE_DECL_THREADSAFE_REFCOUNTING(StoreRef)
    explicit StoreRef(RevocableStore* store) : store_(store) { }

    void set_store(RevocableStore* store) { store_ = store; }
    RevocableStore* store() const { return store_; }

   protected:
    ~StoreRef() {}
   private:
    RevocableStore* store_;

    DISALLOW_EVIL_CONSTRUCTORS(StoreRef);
  };

  
  
  class Revocable {
   public:
    explicit Revocable(RevocableStore* store);
    ~Revocable();

    
    bool revoked() const { return !store_reference_->store(); }

  private:
    
    
    nsRefPtr<StoreRef> store_reference_;

    DISALLOW_EVIL_CONSTRUCTORS(Revocable);
  };

  RevocableStore();
  ~RevocableStore();

  
  void RevokeAll();

  
  bool empty() const { return count_ == 0; }

 private:
  friend class Revocable;

  
  
  void Add(Revocable* item);

  
  nsRefPtr<StoreRef> owning_reference_;

  
  int count_;

  DISALLOW_EVIL_CONSTRUCTORS(RevocableStore);
};

#endif  
