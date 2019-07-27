





#ifndef ReportDecodeResultTask_h_
#define ReportDecodeResultTask_h_

#include "mozilla/Attributes.h"
#include "MediaBufferDecoder.h"

namespace mozilla {

class ReportDecodeResultTask final : public nsRunnable
{
public:
  ReportDecodeResultTask(DecodeJob& aDecodeJob,
                         DecodeJob::ResultFn aFunction)
    : mDecodeJob(aDecodeJob)
    , mFunction(aFunction)
  {
    MOZ_ASSERT(aFunction);
  }

  NS_IMETHOD Run() override
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

