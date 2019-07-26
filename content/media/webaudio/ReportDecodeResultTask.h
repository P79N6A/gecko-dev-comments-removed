





#ifndef ReportDecodeResultTask_h_
#define ReportDecodeResultTask_h_

#include "MediaBufferDecoder.h"

namespace mozilla {

class ReportDecodeResultTask : public nsRunnable
{
public:
  ReportDecodeResultTask(DecodeJob& aDecodeJob,
                         DecodeJob::ResultFn aFunction)
    : mDecodeJob(aDecodeJob)
    , mFunction(aFunction)
  {
    MOZ_ASSERT(aFunction);
  }

  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());

    (mDecodeJob.*mFunction)();

    return NS_OK;
  }

private:
  DecodeJob& mDecodeJob;
  DecodeJob::ResultFn mFunction;
};

}

#endif

