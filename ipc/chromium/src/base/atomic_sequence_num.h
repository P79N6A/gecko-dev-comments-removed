



#ifndef BASE_ATOMIC_SEQUENCE_NUM_H_
#define BASE_ATOMIC_SEQUENCE_NUM_H_

#include "base/atomicops.h"
#include "base/basictypes.h"

namespace base {

class AtomicSequenceNumber {
 public:
  AtomicSequenceNumber() : seq_(0) { }
  explicit AtomicSequenceNumber(base::LinkerInitialized x) {  }

  int GetNext() {
    return static_cast<int>(
        base::subtle::NoBarrier_AtomicIncrement(&seq_, 1) - 1);
  }

 private:
  base::subtle::Atomic32 seq_;
  DISALLOW_COPY_AND_ASSIGN(AtomicSequenceNumber);
};

}  

#endif  
