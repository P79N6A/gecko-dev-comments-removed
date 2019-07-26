




#ifndef mozilla_layers_opengl_FPSCounter_h_
#define mozilla_layers_opengl_FPSCounter_h_

#include <stddef.h>                     
#include <algorithm>                    
#include "GLDefs.h"                     
#include "mozilla/TimeStamp.h"          
#include "nsTArray.h"                   
#include "VBOArena.h"                   

namespace mozilla {
namespace gl {
class GLContext;
}
namespace layers {

class DataTextureSource;
class ShaderProgramOGL;

const double kFpsWindowMs = 250.0;
const size_t kNumFrameTimeStamps = 16;
struct FPSCounter {
  FPSCounter() : mCurrentFrameIndex(0) {
      mFrames.SetLength(kNumFrameTimeStamps);
  }

  
  
  
  
  nsAutoTArray<TimeStamp, kNumFrameTimeStamps> mFrames;
  size_t mCurrentFrameIndex;

  void AddFrame(TimeStamp aNewFrame) {
    mFrames[mCurrentFrameIndex] = aNewFrame;
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % kNumFrameTimeStamps;
  }

  double AddFrameAndGetFps(TimeStamp aCurrentFrame) {
    AddFrame(aCurrentFrame);
    return EstimateFps(aCurrentFrame);
  }

  double GetFpsAt(TimeStamp aNow) {
    return EstimateFps(aNow);
  }

private:
  double EstimateFps(TimeStamp aNow) {
    TimeStamp beginningOfWindow =
      (aNow - TimeDuration::FromMilliseconds(kFpsWindowMs));
    TimeStamp earliestFrameInWindow = aNow;
    size_t numFramesDrawnInWindow = 0;
    for (size_t i = 0; i < kNumFrameTimeStamps; ++i) {
      const TimeStamp& frame = mFrames[i];
      if (!frame.IsNull() && frame > beginningOfWindow) {
        ++numFramesDrawnInWindow;
        earliestFrameInWindow = std::min(earliestFrameInWindow, frame);
      }
    }
    double realWindowSecs = (aNow - earliestFrameInWindow).ToSeconds();
    if (realWindowSecs == 0.0 || numFramesDrawnInWindow == 1) {
      return 0.0;
    }
    return double(numFramesDrawnInWindow - 1) / realWindowSecs;
  }
};

struct FPSState {
  FPSCounter mCompositionFps;
  FPSCounter mTransactionFps;

  FPSState() {}

  void DrawFPS(TimeStamp, int offsetX, int offsetY, unsigned, Compositor* aCompositor);

  void NotifyShadowTreeTransaction() {
    mTransactionFps.AddFrame(TimeStamp::Now());
  }

private:
  RefPtr<DataTextureSource> mFPSTextureSource;
};

}
}

#endif 
