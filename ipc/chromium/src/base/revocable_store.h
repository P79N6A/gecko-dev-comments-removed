



#ifndef BASE_REVOCABLE_STORE_H_
#define BASE_REVOCABLE_STORE_H_

#include "base/ref_counted.h"


class RevocableStore {
 public:
  
  
  
  
  
  class StoreRef : public base::RefCounted<StoreRef> {
   public:
    StoreRef(RevocableStore* store) : store_(store) { }

    void set_store(RevocableStore* store) { store_ = store; }
    RevocableStore* store() const { return store_; }

   private:
    RevocableStore* store_;

    DISALLOW_EVIL_CONSTRUCTORS(StoreRef);
  };

  
  
  class Revocable {
   public:
    Revocable(RevocableStore* store);
    ~Revocable();

    
    bool revoked() const { return !store_reference_->store(); }

  private:
    
    
    scoped_refptr<StoreRef> store_reference_;

    DISALLOW_EVIL_CONSTRUCTORS(Revocable);
  };

  RevocableStore();
  ~RevocableStore();

  
  void RevokeAll();

  
  bool empty() const { return count_ == 0; }

 private:
  friend class Revocable;

  
  
  void Add(Revocable* item);

  
  scoped_refptr<StoreRef> owning_reference_;

  
  int count_;

  DISALLOW_EVIL_CONSTRUCTORS(RevocableStore);
};

#endif  
