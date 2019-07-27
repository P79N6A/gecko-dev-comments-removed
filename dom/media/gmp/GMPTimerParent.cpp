




#include "GMPTimerParent.h"
#include "nsComponentManagerUtils.h"
#include "mozilla/unused.h"

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

extern PRLogModuleInfo* GetGMPLog();

#define LOGD(msg) MOZ_LOG(GetGMPLog(), mozilla::LogLevel::Debug, msg)
#define LOG(level, msg) MOZ_LOG(GetGMPLog(), (level), msg)

#ifdef __CLASS__
#undef __CLASS__
#endif
#define __CLASS__ "GMPParent"

namespace gmp {

GMPTimerParent::GMPTimerParent(nsIThread* aGMPThread)
  : mGMPThread(aGMPThread)
  , mIsOpen(true)
{
}

bool
GMPTimerParent::RecvSetTimer(const uint32_t& aTimerId,
                             const uint32_t& aTimeoutMs)
{
  LOGD(("%s::%s: %p mIsOpen=%d", __CLASS__, __FUNCTION__, this, mIsOpen));

  MOZ_ASSERT(mGMPThread == NS_GetCurrentThread());

  if (!mIsOpen) {
    return true;
  }

  nsresult rv;
  nsAutoPtr<Context> ctx(new Context());
  ctx->mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, true);

  ctx->mId = aTimerId;
  rv = ctx->mTimer->SetTarget(mGMPThread);
  NS_ENSURE_SUCCESS(rv, true);
  ctx->mParent = this;

  rv = ctx->mTimer->InitWithFuncCallback(&GMPTimerParent::GMPTimerExpired,
                                          ctx,
                                          aTimeoutMs,
                                          nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, true);

  mTimers.PutEntry(ctx.forget());

  return true;
}

void
GMPTimerParent::Shutdown()
{
  LOGD(("%s::%s: %p mIsOpen=%d", __CLASS__, __FUNCTION__, this, mIsOpen));

  MOZ_ASSERT(mGMPThread == NS_GetCurrentThread());
  mTimers.EnumerateEntries(GMPTimerParent::CancelTimers, nullptr);
  mTimers.Clear();
  mIsOpen = false;
}


PLDHashOperator
GMPTimerParent::CancelTimers(nsPtrHashKey<Context>* aContext, void* aClosure)
{
  auto context = aContext->GetKey();
  context->mTimer->Cancel();
  delete context;
  return PL_DHASH_NEXT;
}

void
GMPTimerParent::ActorDestroy(ActorDestroyReason aWhy)
{
  LOGD(("%s::%s: %p mIsOpen=%d", __CLASS__, __FUNCTION__, this, mIsOpen));

  Shutdown();
}


void
GMPTimerParent::GMPTimerExpired(nsITimer *aTimer, void *aClosure)
{
  MOZ_ASSERT(aClosure);
  nsAutoPtr<Context> ctx(static_cast<Context*>(aClosure));
  MOZ_ASSERT(ctx->mParent);
  if (ctx->mParent) {
    ctx->mParent->TimerExpired(ctx);
  }
}

void
GMPTimerParent::TimerExpired(Context* aContext)
{
  LOGD(("%s::%s: %p mIsOpen=%d", __CLASS__, __FUNCTION__, this, mIsOpen));
  MOZ_ASSERT(mGMPThread == NS_GetCurrentThread());

  if (!mIsOpen) {
    return;
  }

  uint32_t id = aContext->mId;
  mTimers.RemoveEntry(aContext);
  if (id) {
    unused << SendTimerExpired(id);
  }
}

} 
} 
