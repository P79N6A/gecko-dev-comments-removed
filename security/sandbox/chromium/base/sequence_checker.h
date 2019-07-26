



#ifndef BASE_SEQUENCE_CHECKER_H_
#define BASE_SEQUENCE_CHECKER_H_

#include "base/memory/ref_counted.h"


#if (!defined(NDEBUG) || defined(DCHECK_ALWAYS_ON))
#define ENABLE_SEQUENCE_CHECKER 1
#else
#define ENABLE_SEQUENCE_CHECKER 0
#endif

#if ENABLE_SEQUENCE_CHECKER
#include "base/sequence_checker_impl.h"
#endif

namespace base {

class SequencedTaskRunner;





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
