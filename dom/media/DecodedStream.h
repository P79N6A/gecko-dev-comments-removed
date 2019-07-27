





#ifndef DecodedStream_h_
#define DecodedStream_h_

#include "nsRefPtr.h"
#include "nsTArray.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {

class MediaInputPort;
class SourceMediaStream;
class ProcessedMediaStream;
class DecodedStream;
class DecodedStreamGraphListener;
class OutputStreamData;
class OutputStreamListener;
class ReentrantMonitor;

namespace layers {
class Image;
}









class DecodedStreamData {
public:
  DecodedStreamData(int64_t aInitialTime, SourceMediaStream* aStream);
  ~DecodedStreamData();
  bool IsFinished() const;
  int64_t GetClock() const;

  


  
  int64_t mAudioFramesWritten;
  
  
  const int64_t mInitialTime; 
  
  
  
  int64_t mNextVideoTime; 
  int64_t mNextAudioTime; 
  
  
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
  void Init(DecodedStream* aDecodedStream, ProcessedMediaStream* aStream);
  nsRefPtr<ProcessedMediaStream> mStream;
  
  nsRefPtr<MediaInputPort> mPort;
  nsRefPtr<OutputStreamListener> mListener;
};

class DecodedStream {
public:
  explicit DecodedStream(ReentrantMonitor& aMonitor);
  DecodedStreamData* GetData();
  void DestroyData();
  void RecreateData(int64_t aInitialTime, SourceMediaStream* aStream);
  nsTArray<OutputStreamData>& OutputStreams();
  ReentrantMonitor& GetReentrantMonitor();
  void Connect(ProcessedMediaStream* aStream, bool aFinishWhenEnded);

private:
  void Connect(OutputStreamData* aStream);

  UniquePtr<DecodedStreamData> mData;
  
  nsTArray<OutputStreamData> mOutputStreams;
  ReentrantMonitor& mMonitor;
};

} 

#endif 
