





#ifndef DecodedStream_h_
#define DecodedStream_h_

#include "nsRefPtr.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {

class MediaDecoder;
class MediaInputPort;
class SourceMediaStream;
class ProcessedMediaStream;
class DecodedStreamGraphListener;
class OutputStreamData;
class OutputStreamListener;

namespace layers {
class Image;
}









class DecodedStreamData {
public:
  DecodedStreamData(MediaDecoder* aDecoder, int64_t aInitialTime,
                    SourceMediaStream* aStream);
  ~DecodedStreamData();
  bool IsFinished() const;
  int64_t GetClock() const;

  


  
  int64_t mAudioFramesWritten;
  
  
  const int64_t mInitialTime; 
  
  
  
  int64_t mNextVideoTime; 
  int64_t mNextAudioTime; 
  MediaDecoder* mDecoder;
  
  
  nsRefPtr<layers::Image> mLastVideoImage;
  gfx::IntSize mLastVideoImageDisplaySize;
  
  
  bool mStreamInitialized;
  bool mHaveSentFinish;
  bool mHaveSentFinishAudio;
  bool mHaveSentFinishVideo;

  
  const nsRefPtr<SourceMediaStream> mStream;
  nsRefPtr<DecodedStreamGraphListener> mListener;
  
  
  bool mHaveBlockedForPlayState;
  
  
  bool mHaveBlockedForStateMachineNotPlaying;
  
  
  bool mEOSVideoCompensation;
};

class OutputStreamData {
public:
  
  
  
  
  OutputStreamData();
  ~OutputStreamData();
  void Init(MediaDecoder* aDecoder, ProcessedMediaStream* aStream);
  nsRefPtr<ProcessedMediaStream> mStream;
  
  nsRefPtr<MediaInputPort> mPort;
  nsRefPtr<OutputStreamListener> mListener;
};

class DecodedStream {
public:
  DecodedStreamData* GetData();
  void DestroyData();
  void RecreateData(MediaDecoder* aDecoder, int64_t aInitialTime,
                    SourceMediaStream* aStream);

private:
  UniquePtr<DecodedStreamData> mData;
};

} 

#endif 
