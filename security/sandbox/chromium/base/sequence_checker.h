



#ifndef BASE_SEQUENCE_CHECKER_H_
#define BASE_SEQUENCE_CHECKER_H_


#if (!defined(NDEBUG) || defined(DCHECK_ALWAYS_ON))
#define ENABLE_SEQUENCE_CHECKER 1
#else
#define ENABLE_SEQUENCE_CHECKER 0
#endif

#include "base/sequence_checker_impl.h"

namespace base {





class SequenceCheckerDoNothing {
 public:
  bool CalledOnValidSequencedThread() const {
    return true;
  }

  void DetachFromSequence() {}
};



















#if ENABLE_SEQUENCE_CHECKER
class SequenceChecker : public SequenceCheckerImpl {
};
#else
class SequenceChecker : public SequenceCheckerDoNothing {
};
#endif  

#undef ENABLE_SEQUENCE_CHECKER

}  

#endif  
