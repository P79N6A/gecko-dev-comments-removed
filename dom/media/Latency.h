





#ifndef MOZILLA_LATENCY_H
#define MOZILLA_LATENCY_H

#include "mozilla/TimeStamp.h"
#include "prlog.h"
#include "nsCOMPtr.h"
#include "nsIThread.h"
#include "mozilla/Monitor.h"
#include "nsISupportsImpl.h"
#include "nsIObserver.h"

class AsyncLatencyLogger;

PRLogModuleInfo* GetLatencyLog();


class AsyncLatencyLogger : public nsIObserver
{
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

public:

  enum LatencyLogIndex {
    AudioMediaStreamTrack = 0,
    VideoMediaStreamTrack,
    Cubeb,
    AudioStream,
    NetEQ,
    AudioCaptureBase, 
    AudioCapture, 
    AudioTrackInsertion, 
    MediaPipelineAudioInsertion, 
    AudioTransmit, 
    AudioReceive, 
    MediaPipelineAudioPlayout, 
    MediaStreamCreate, 
    AudioStreamCreate, 
    AudioSendRTP,
    AudioRecvRTP,
    _MAX_INDEX
  };
  
  void Log(LatencyLogIndex index, uint64_t aID, int64_t aValue);
  
  void Log(LatencyLogIndex index, uint64_t aID, int64_t aValue,
           mozilla::TimeStamp &aTime);
  
  void WriteLog(LatencyLogIndex index, uint64_t aID, int64_t aValue,
                mozilla::TimeStamp timestamp);
  
  void GetStartTime(mozilla::TimeStamp &aStart);

  static AsyncLatencyLogger* Get(bool aStartTimer = false);
  static void InitializeStatics();
  
  static void ShutdownLogger();
private:
  AsyncLatencyLogger();
  virtual ~AsyncLatencyLogger();
  int64_t GetTimeStamp();
  void Init();
  
  
  void Shutdown();
  
  nsCOMPtr<nsIThread> mThread;
  
  
  
  mozilla::TimeStamp mStart;
  
  
  
  mozilla::Mutex mMutex;
};



void LogLatency(AsyncLatencyLogger::LatencyLogIndex index, uint64_t aID, int64_t aValue);
void LogLatency(uint32_t index, uint64_t aID, int64_t aValue);

void LogTime(AsyncLatencyLogger::LatencyLogIndex index, uint64_t aID, int64_t aValue);
void LogTime(uint32_t index, uint64_t aID, int64_t aValue);

void LogTime(AsyncLatencyLogger::LatencyLogIndex index, uint64_t aID, int64_t aValue,
             mozilla::TimeStamp &aTime);


#define LATENCY_STREAM_ID(source, trackID) \
  ((((uint64_t) (source)) & ~0x0F) | (trackID))

#endif
