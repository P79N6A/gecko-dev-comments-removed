





#if !defined(MediaMetadataManager_h__)
#define MediaMetadataManager_h__
#include "VideoUtils.h"
#include "mozilla/LinkedList.h"
#include "AbstractMediaDecoder.h"
#include "nsAutoPtr.h"

namespace mozilla {

  
  
  class TimedMetadata : public LinkedListElement<TimedMetadata> {
    public:
      
      int64_t mPublishTime;
      
      
      nsAutoPtr<MetadataTags> mTags;
      
      int mRate;
      
      int mChannels;
      
      bool mHasAudio;
  };

  
  
  class MediaMetadataManager
  {
    public:
      ~MediaMetadataManager() {
        TimedMetadata* element;
        while((element = mMetadataQueue.popFirst()) != nullptr) {
          delete element;
        }
      }
      void QueueMetadata(TimedMetadata* aMetadata) {
        mMetadataQueue.insertBack(aMetadata);
      }

      void DispatchMetadataIfNeeded(AbstractMediaDecoder* aDecoder, double aCurrentTime) {
        TimedMetadata* metadata = mMetadataQueue.getFirst();
        while (metadata && aCurrentTime >= static_cast<double>(metadata->mPublishTime) / USECS_PER_S) {
          nsCOMPtr<nsIRunnable> metadataUpdatedEvent =
            new mozilla::AudioMetadataEventRunner(aDecoder,
                                                  metadata->mChannels,
                                                  metadata->mRate,
                                                  metadata->mHasAudio,
                                                  metadata->mTags.forget());
          NS_DispatchToMainThread(metadataUpdatedEvent, NS_DISPATCH_NORMAL);
          mMetadataQueue.popFirst();
          metadata = mMetadataQueue.getFirst();
        }
      }
    protected:
      LinkedList<TimedMetadata> mMetadataQueue;
  };
}

#endif
