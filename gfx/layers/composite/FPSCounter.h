




#ifndef mozilla_layers_opengl_FPSCounter_h_
#define mozilla_layers_opengl_FPSCounter_h_

#include <algorithm>                    
#include <stddef.h>                     
#include <map>                          
#include "GLDefs.h"                     
#include "mozilla/RefPtr.h"             
#include "mozilla/TimeStamp.h"          
#include "nsTArray.h"                   
#include "prio.h"                       

namespace mozilla {
namespace layers {

class DataTextureSource;
class Compositor;


const int kFpsDumpInterval = 10;



const int kMaxFrames = 2400;

























class FPSCounter {
public:
  explicit FPSCounter(const char* aName);
  ~FPSCounter();

  void AddFrame(TimeStamp aTimestamp);
  double AddFrameAndGetFps(TimeStamp aTimestamp);
  double GetFPS(TimeStamp aTimestamp);

private:
  void      Init();
  bool      CapturedFullInterval(TimeStamp aTimestamp);

  
  void      ResetReverseIterator();
  bool      HasNext(TimeStamp aTimestamp, double aDuration = kFpsDumpInterval);
  TimeStamp GetNextTimeStamp();
  int       GetLatestReadIndex();
  TimeStamp GetLatestTimeStamp();
  void      WriteFrameTimeStamps(PRFileDesc* fd);
  bool      IteratedFullInterval(TimeStamp aTimestamp, double aDuration);

  void      PrintFPS();
  int       BuildHistogram(std::map<int, int>& aHistogram);
  void      PrintHistogram(std::map<int, int>& aHistogram);
  double    GetMean(std::map<int,int> aHistogram);
  double    GetStdDev(std::map<int, int> aHistogram);
  nsresult  WriteFrameTimeStamps();

  





  nsAutoTArray<TimeStamp, kMaxFrames> mFrameTimestamps;
  int mWriteIndex;      
  int mIteratorIndex;   
  const char* mFPSName;
  TimeStamp mLastInterval;
};

struct FPSState {
  FPSState();
  void DrawFPS(TimeStamp, int offsetX, int offsetY, unsigned, Compositor* aCompositor);
  void NotifyShadowTreeTransaction() {
    mTransactionFps.AddFrame(TimeStamp::Now());
  }

  FPSCounter mCompositionFps;
  FPSCounter mTransactionFps;

private:
  RefPtr<DataTextureSource> mFPSTextureSource;
};

} 
} 

#endif 
