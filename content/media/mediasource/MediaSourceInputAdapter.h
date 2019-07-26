





#ifndef MOZILLA_MEDIASOURCEINPUTADAPTER_H_
#define MOZILLA_MEDIASOURCEINPUTADAPTER_H_

#include "nsIAsyncInputStream.h"
#include "nsCycleCollectionParticipant.h"
#include "MediaSource.h"

namespace mozilla {
namespace dom {

class MediaSourceInputAdapter MOZ_FINAL : public nsIAsyncInputStream
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(MediaSourceInputAdapter)
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSIASYNCINPUTSTREAM

  MediaSourceInputAdapter(MediaSource* aMediaSource);
  ~MediaSourceInputAdapter();

  void NotifyListener();

private:
  uint64_t Available();

  nsRefPtr<MediaSource> mMediaSource;
  nsCOMPtr<nsIInputStreamCallback> mCallback;
  nsCOMPtr<nsIEventTarget> mCallbackTarget;
  int64_t mOffset;
  uint32_t mNotifyThreshold;
  bool mClosed;
};

} 
} 
#endif 
