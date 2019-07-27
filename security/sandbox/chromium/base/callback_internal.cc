



#include "base/callback_internal.h"

#include "base/logging.h"

namespace base {
namespace internal {

void CallbackBase::Reset() {
  polymorphic_invoke_ = NULL;
  
  
  bind_state_ = NULL;
}

bool CallbackBase::Equals(const CallbackBase& other) const {
  return bind_state_.get() == other.bind_state_.get() &&
         polymorphic_invoke_ == other.polymorphic_invoke_;
}

CallbackBase::CallbackBase(BindStateBase* bind_state)
    : bind_state_(bind_state),
      polymorphic_invoke_(NULL) {
  DCHECK(!bind_state_.get() || bind_state_->HasOneRef());
}

CallbackBase::~CallbackBase() {
}

}  
}  
