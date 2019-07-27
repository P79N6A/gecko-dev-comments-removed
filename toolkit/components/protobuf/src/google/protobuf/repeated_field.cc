

































#include <algorithm>

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/stubs/common.h>

namespace google {
namespace protobuf {

namespace internal {

void RepeatedPtrFieldBase::Reserve(int new_size) {
  if (total_size_ >= new_size) return;

  void** old_elements = elements_;
  total_size_ = max(kMinRepeatedFieldAllocationSize,
                    max(total_size_ * 2, new_size));
  elements_ = new void*[total_size_];
  if (old_elements != NULL) {
    memcpy(elements_, old_elements, allocated_size_ * sizeof(elements_[0]));
    delete [] old_elements;
  }
}

void RepeatedPtrFieldBase::Swap(RepeatedPtrFieldBase* other) {
  if (this == other) return;
  void** swap_elements       = elements_;
  int    swap_current_size   = current_size_;
  int    swap_allocated_size = allocated_size_;
  int    swap_total_size     = total_size_;

  elements_       = other->elements_;
  current_size_   = other->current_size_;
  allocated_size_ = other->allocated_size_;
  total_size_     = other->total_size_;

  other->elements_       = swap_elements;
  other->current_size_   = swap_current_size;
  other->allocated_size_ = swap_allocated_size;
  other->total_size_     = swap_total_size;
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
