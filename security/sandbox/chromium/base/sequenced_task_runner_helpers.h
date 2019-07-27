



#ifndef BASE_SEQUENCED_TASK_RUNNER_HELPERS_H_
#define BASE_SEQUENCED_TASK_RUNNER_HELPERS_H_

#include "base/basictypes.h"
#include "base/debug/alias.h"





namespace tracked_objects {
class Location;
}

namespace base {

namespace subtle {
template <class T, class R> class DeleteHelperInternal;
template <class T, class R> class ReleaseHelperInternal;
}









template <class T>
class DeleteHelper {
 private:
  template <class T2, class R> friend class subtle::DeleteHelperInternal;

  static void DoDelete(const void* object) {
    delete reinterpret_cast<const T*>(object);
  }

  DISALLOW_COPY_AND_ASSIGN(DeleteHelper);
};

template <class T>
class ReleaseHelper {
 private:
  template <class T2, class R> friend class subtle::ReleaseHelperInternal;

  static void DoRelease(const void* object) {
    reinterpret_cast<const T*>(object)->Release();
  }

  DISALLOW_COPY_AND_ASSIGN(ReleaseHelper);
};

namespace subtle {


















template <class T, class ReturnType>
class DeleteHelperInternal {
 public:
  template <class SequencedTaskRunnerType>
  static ReturnType DeleteViaSequencedTaskRunner(
      SequencedTaskRunnerType* sequenced_task_runner,
      const tracked_objects::Location& from_here,
      const T* object) {
    return sequenced_task_runner->DeleteSoonInternal(
        from_here, &DeleteHelper<T>::DoDelete, object);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(DeleteHelperInternal);
};

template <class T, class ReturnType>
class ReleaseHelperInternal {
 public:
  template <class SequencedTaskRunnerType>
  static ReturnType ReleaseViaSequencedTaskRunner(
      SequencedTaskRunnerType* sequenced_task_runner,
      const tracked_objects::Location& from_here,
      const T* object) {
    return sequenced_task_runner->ReleaseSoonInternal(
        from_here, &ReleaseHelper<T>::DoRelease, object);
  }

 private:
  DISALLOW_COPY_AND_ASSIGN(ReleaseHelperInternal);
};

}  

}  

#endif  
