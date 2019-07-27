




#ifndef GFX_CONTEXTSTATETRACKER_H
#define GFX_CONTEXTSTATETRACKER_H

#include "GLTypes.h"
#include "mozilla/TimeStamp.h"
#include "nsTArray.h"
#include <string.h>

namespace mozilla {
namespace gl {
class GLContext;
} 






class ContextStateTracker {
public:
  ContextStateTracker() {}

private:

  bool IsProfiling() { return true; }

protected:
  typedef GLuint TimerQueryHandle;

  class ContextState {
  public:
    explicit ContextState(const char* aSectionName)
      : mSectionName(aSectionName)
    {}

    const char* mSectionName;
    mozilla::TimeStamp mCpuTimeStart;
    mozilla::TimeStamp mCpuTimeEnd;
    TimerQueryHandle mStartQueryHandle;
  };

  ContextState& Top() {
    MOZ_ASSERT(mSectionStack.Length());
    return mSectionStack[mSectionStack.Length() - 1];
  }

  nsTArray<ContextState> mCompletedSections;
  nsTArray<ContextState> mSectionStack;
};
















class ContextStateTrackerOGL final : public ContextStateTracker {
  typedef mozilla::gl::GLContext GLContext;
public:
  void PushOGLSection(GLContext* aGL, const char* aSectionName);
  void PopOGLSection(GLContext* aGL, const char* aSectionName);
  void DestroyOGL(GLContext* aGL);
private:
  void Flush(GLContext* aGL);
};

} 

#endif

