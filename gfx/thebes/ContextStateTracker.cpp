




#include "ContextStateTracker.h"
#include "GLContext.h"
#ifdef MOZ_ENABLE_PROFILER_SPS
#include "ProfilerMarkers.h"
#endif

namespace mozilla {

void
ContextStateTrackerOGL::PushOGLSection(GLContext* aGL, const char* aSectionName)
{
  if (!profiler_feature_active("gpu")) {
    return;
  }

  if (!aGL->IsSupported(gl::GLFeature::query_objects)) {
    return;
  }

  if (mSectionStack.Length() > 0) {
    
    
    aGL->fEndQuery(LOCAL_GL_TIME_ELAPSED);
    Top().mCpuTimeEnd = TimeStamp::Now();
  }

  ContextState newSection(aSectionName);

  GLuint queryObject;
  aGL->fGenQueries(1, &queryObject);
  newSection.mStartQueryHandle = queryObject;
  newSection.mCpuTimeStart = TimeStamp::Now();

  aGL->fBeginQuery(LOCAL_GL_TIME_ELAPSED_EXT, queryObject);

  mSectionStack.AppendElement(newSection);
}

void
ContextStateTrackerOGL::PopOGLSection(GLContext* aGL, const char* aSectionName)
{
  
  
  if (mSectionStack.Length() == 0) {
    return;
  }

  int i = mSectionStack.Length() - 1;
  MOZ_ASSERT(strcmp(mSectionStack[i].mSectionName, aSectionName) == 0);
  aGL->fEndQuery(LOCAL_GL_TIME_ELAPSED);
  mSectionStack[i].mCpuTimeEnd = TimeStamp::Now();
  mCompletedSections.AppendElement(mSectionStack[i]);
  mSectionStack.RemoveElementAt(i);

  if (i - 1 >= 0) {
    const char* sectionToRestore = Top().mSectionName;

    
    
    
    mCompletedSections.AppendElement(Top());
    mSectionStack.RemoveElementAt(i - 1);

    ContextState newSection(sectionToRestore);

    GLuint queryObject;
    aGL->fGenQueries(1, &queryObject);
    newSection.mStartQueryHandle = queryObject;
    newSection.mCpuTimeStart = TimeStamp::Now();

    aGL->fBeginQuery(LOCAL_GL_TIME_ELAPSED_EXT, queryObject);

    mSectionStack.AppendElement(newSection);
  }

  Flush(aGL);
}

void
ContextStateTrackerOGL::Flush(GLContext* aGL)
{
  TimeStamp now = TimeStamp::Now();

  while (mCompletedSections.Length() != 0) {
    
    
    
    if (mCompletedSections[0].mCpuTimeEnd + TimeDuration::FromMilliseconds(200) > now) {
      break;
    }

    GLuint handle = mCompletedSections[0].mStartQueryHandle;

    
    
    GLuint returned = 0;
    aGL->fGetQueryObjectuiv(handle, LOCAL_GL_QUERY_RESULT_AVAILABLE, &returned);

    if (!returned) {
      break;
    }

    GLuint gpuTime = 0;
    aGL->fGetQueryObjectuiv(handle, LOCAL_GL_QUERY_RESULT, &gpuTime);

    aGL->fDeleteQueries(1, &handle);

#ifdef MOZ_ENABLE_PROFILER_SPS
    PROFILER_MARKER_PAYLOAD("gpu_timer_query", new GPUMarkerPayload(
      mCompletedSections[0].mCpuTimeStart,
      mCompletedSections[0].mCpuTimeEnd,
      0,
      gpuTime
    ));
#endif

    mCompletedSections.RemoveElementAt(0);
  }
}

void
ContextStateTrackerOGL::DestroyOGL(GLContext* aGL)
{
  while (mCompletedSections.Length() != 0) {
    GLuint handle = (GLuint)mCompletedSections[0].mStartQueryHandle;
    aGL->fDeleteQueries(1, &handle);
    mCompletedSections.RemoveElementAt(0);
  }
}

} 

