



#ifndef BASE_SEQUENCE_CHECKER_IMPL_H_
#define BASE_SEQUENCE_CHECKER_IMPL_H_

#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/synchronization/lock.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_checker_impl.h"

namespace base {






class BASE_EXPORT SequenceCheckerImpl {
 public:
  SequenceCheckerImpl();
  ~SequenceCheckerImpl();

  
  
  
  bool CalledOnValidSequencedThread() const;

  
  
  void DetachFromSequence();

 private:
  void EnsureSequenceTokenAssigned() const;

  
  mutable Lock lock_;

  
  ThreadCheckerImpl thread_checker_;
  mutable bool sequence_token_assigned_;

  mutable SequencedWorkerPool::SequenceToken sequence_token_;

  DISALLOW_COPY_AND_ASSIGN(SequenceCheckerImpl);
};

}  

#endif  
