





#include "MediaDataDecoderProxy.h"

namespace mozilla {

void
MediaDataDecoderCallbackProxy::Error()
{
  mProxyCallback->Error();
  mProxyDecoder->Shutdown();
}

void
MediaDataDecoderCallbackProxy::FlushComplete()
{
  mProxyDecoder->FlushComplete();
}

nsresult
MediaDataDecoderProxy::Init()
{
  MOZ_ASSERT(!mIsShutdown);
  nsRefPtr<InitTask> task(new InitTask(mProxyDecoder));
  nsresult rv = mProxyThread->Dispatch(task, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(task->Result(), task->Result());

  return NS_OK;
}

nsresult
MediaDataDecoderProxy::Input(mp4_demuxer::MP4Sample* aSample)
{
  MOZ_ASSERT(!IsOnProxyThread());
  MOZ_ASSERT(!mIsShutdown);

  nsRefPtr<nsIRunnable> task(new InputTask(mProxyDecoder, aSample));
  nsresult rv = mProxyThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
MediaDataDecoderProxy::Flush()
{
  MOZ_ASSERT(!IsOnProxyThread());
  MOZ_ASSERT(!mIsShutdown);

  mFlushComplete.Set(false);

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(mProxyDecoder, &MediaDataDecoder::Flush);
  nsresult rv = mProxyThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  mFlushComplete.WaitUntil(true);

  return NS_OK;
}

nsresult
MediaDataDecoderProxy::Drain()
{
  MOZ_ASSERT(!IsOnProxyThread());
  MOZ_ASSERT(!mIsShutdown);

  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(mProxyDecoder, &MediaDataDecoder::Drain);
  nsresult rv = mProxyThread->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
MediaDataDecoderProxy::Shutdown()
{
  
  MOZ_ASSERT(!mIsShutdown);
#if defined(DEBUG)
  mIsShutdown = true;
#endif
  nsRefPtr<nsIRunnable> task;
  task = NS_NewRunnableMethod(mProxyDecoder, &MediaDataDecoder::Shutdown);
  nsresult rv = mProxyThread->Dispatch(task, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

void
MediaDataDecoderProxy::FlushComplete()
{
  mFlushComplete.Set(true);
}

} 
