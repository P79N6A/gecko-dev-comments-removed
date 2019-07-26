

































#include <algorithm>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

namespace internal {

void RepeatedPtrFieldBase::Reserve(int new_size) {
  if (total_size_ >= new_size) return;

  void** old_elements = elements_;
  total_size_ = max(total_size_ * 2, new_size);
  elements_ = new void*[total_size_];
  memcpy(elements_, old_elements, allocated_size_ * sizeof(elements_[0]));
  if (old_elements != initial_space_) {
    delete [] old_elements;
  }
}

void RepeatedPtrFieldBase::Swap(RepeatedPtrFieldBase* other) {
  void** swap_elements       = elements_;
  int    swap_current_size   = current_size_;
  int    swap_allocated_size = allocated_size_;
  int    swap_total_size     = total_size_;
  
  
  void* swap_initial_space[kInitialSize];
  memcpy(swap_initial_space, initial_space_, sizeof(initial_space_));

  elements_       = other->elements_;
  current_size_   = other->current_size_;
  allocated_size_ = other->allocated_size_;
  total_size_     = other->total_size_;
  memcpy(initial_space_, other->initial_space_, sizeof(initial_space_));

  other->elements_       = swap_elements;
  other->current_size_   = swap_current_size;
  other->allocated_size_ = swap_allocated_size;
  other->total_size_     = swap_total_size;
  memcpy(other->initial_space_, swap_initial_space, sizeof(swap_initial_space));

  if (elements_ == other->initial_space_) {
    elements_ = initial_space_;
  }
  if (other->elements_ == initial_space_) {
    other->elements_ = other->initial_space_;
  }
}

string* StringTypeHandlerBase::New() {
  return new string;
}
void StringTypeHandlerBase::Delete(string* value) {
  delete value;
}

}  


}  
}  
