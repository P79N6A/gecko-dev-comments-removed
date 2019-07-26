



#ifndef BASE_ATOMIC_SEQUENCE_NUM_H_
#define BASE_ATOMIC_SEQUENCE_NUM_H_

#include "base/atomicops.h"
#include "base/basictypes.h"

namespace base {

class AtomicSequenceNumber;








class StaticAtomicSequenceNumber {
 public:
  inline int GetNext() {
    return static_cast<int>(
        base::subtle::NoBarrier_AtomicIncrement(&seq_, 1) - 1);
  }

 private:
  friend class AtomicSequenceNumber;

  inline void Reset() {
    base::subtle::Release_Store(&seq_, 0);
  }

  base::subtle::Atomic32 seq_;
};





class AtomicSequenceNumber {
 public:
  AtomicSequenceNumber() {
    seq_.Reset();
  }

  inline int GetNext() {
    return seq_.GetNext();
  }

 private:
  StaticAtomicSequenceNumber seq_;
  DISALLOW_COPY_AND_ASSIGN(AtomicSequenceNumber);
};

}  

#endif  
