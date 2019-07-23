



#include "base/revocable_store.h"

#include "base/logging.h"

RevocableStore::Revocable::Revocable(RevocableStore* store)
    : store_reference_(store->owning_reference_) {
  
  DCHECK(store_reference_->store());
  store_reference_->store()->Add(this);
}

RevocableStore::Revocable::~Revocable() {
  if (!revoked()) {
    
    --(store_reference_->store()->count_);
  }
}

RevocableStore::RevocableStore() : count_(0) {
  
  owning_reference_ = new StoreRef(this);
}

RevocableStore::~RevocableStore() {
  
  owning_reference_->set_store(NULL);
}

void RevocableStore::Add(Revocable* item) {
  DCHECK(!item->revoked());
  ++count_;
}

void RevocableStore::RevokeAll() {
  
  owning_reference_->set_store(NULL);
  count_ = 0;

  
  
  
  owning_reference_ = new StoreRef(this);
}
