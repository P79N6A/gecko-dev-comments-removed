





#include "I420ColorConverterHelper.h"

#include <dlfcn.h>

#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo *gI420ColorConverterHelperLog;
#define LOG(msg...) PR_LOG(gI420ColorConverterHelperLog, PR_LOG_WARNING, (msg))
#else
#define LOG(x...)
#endif

namespace android {

I420ColorConverterHelper::I420ColorConverterHelper()
  : mHandle(nullptr)
  , mConverter({nullptr, nullptr, nullptr, nullptr, nullptr})
{
#ifdef PR_LOGGING
  if (!gI420ColorConverterHelperLog) {
    gI420ColorConverterHelperLog = PR_NewLogModule("I420ColorConverterHelper");
  }
#endif
}

I420ColorConverterHelper::~I420ColorConverterHelper()
{
  RWLock::AutoWLock awl(mLock);
  unloadLocked();
}


bool
I420ColorConverterHelper::loadLocked()
{
  if (loadedLocked()) {
    return true;
  }
  unloadLocked();

  
  mHandle = dlopen("libI420colorconvert.so", RTLD_NOW);
  if (mHandle == nullptr) {
    LOG("libI420colorconvert.so not found");
    return false;
  }

  
  
  
  typedef int (* getConverterFn)(II420ColorConverter *converter);
  getConverterFn getI420ColorConverter =
      (getConverterFn) dlsym(mHandle, "getI420ColorConverter");
  if (getI420ColorConverter == nullptr) {
    LOG("Cannot load getI420ColorConverter from libI420colorconvert.so");
    unloadLocked();
    return false;
  }

  
  getI420ColorConverter(&mConverter);
  if (mConverter.getDecoderOutputFormat == nullptr ||
      mConverter.convertDecoderOutputToI420 == nullptr ||
      mConverter.getEncoderInputFormat == nullptr ||
      mConverter.convertI420ToEncoderInput == nullptr ||
      mConverter.getEncoderInputBufferInfo == nullptr) {
    LOG("Failed to initialize I420 color converter");
    unloadLocked();
    return false;
  }

  return true;
}


bool
I420ColorConverterHelper::loadedLocked() const
{
  if (mHandle == nullptr ||
      mConverter.getDecoderOutputFormat == nullptr ||
      mConverter.convertDecoderOutputToI420 == nullptr ||
      mConverter.getEncoderInputFormat == nullptr ||
      mConverter.convertI420ToEncoderInput == nullptr ||
      mConverter.getEncoderInputBufferInfo == nullptr) {
    return false;
  }
  return true;
}


void
I420ColorConverterHelper::unloadLocked()
{
  if (mHandle != nullptr) {
    dlclose(mHandle);
  }
  mHandle = nullptr;
  mConverter.getDecoderOutputFormat = nullptr;
  mConverter.convertDecoderOutputToI420 = nullptr;
  mConverter.getEncoderInputFormat = nullptr;
  mConverter.convertI420ToEncoderInput = nullptr;
  mConverter.getEncoderInputBufferInfo = nullptr;
}

bool
I420ColorConverterHelper::ensureLoaded()
{
  {
    RWLock::AutoRLock arl(mLock);
    
    if (loadedLocked()) {
      return true;
    }
  }

  {
    RWLock::AutoWLock awl(mLock);
    
    if (loadedLocked()) {
      return true;
    }

    
    unloadLocked();
    if (loadLocked()) {
      return true;
    }

    
    unloadLocked();
  }

  return false;
}

int
I420ColorConverterHelper::getDecoderOutputFormat()
{
  if (!ensureLoaded()) {
    return -1;
  }

  RWLock::AutoRLock arl(mLock);
  if (mConverter.getDecoderOutputFormat != nullptr) {
    return mConverter.getDecoderOutputFormat();
  }
  return -1;
}

int
I420ColorConverterHelper::convertDecoderOutputToI420(
    void* decoderBits, int decoderWidth, int decoderHeight,
    ARect decoderRect, void* dstBits)
{
  if (!ensureLoaded()) {
    return -1;
  }

  RWLock::AutoRLock arl(mLock);
  if (mConverter.convertDecoderOutputToI420 != nullptr) {
    return mConverter.convertDecoderOutputToI420(decoderBits,
        decoderWidth, decoderHeight, decoderRect, dstBits);
  }
  return -1;
}

int
I420ColorConverterHelper::getEncoderInputFormat()
{
  if (!ensureLoaded()) {
    return -1;
  }

  RWLock::AutoRLock arl(mLock);
  if (mConverter.getEncoderInputFormat != nullptr) {
    return mConverter.getEncoderInputFormat();
  }
  return -1;
}

int
I420ColorConverterHelper::convertI420ToEncoderInput(void* aSrcBits,
                                                    int aSrcWidth,
                                                    int aSrcHeight,
                                                    int aEncoderWidth,
                                                    int aEncoderHeight,
                                                    ARect aEncoderRect,
                                                    void* aEncoderBits)
{
  if (!ensureLoaded()) {
    return -1;
  }

  RWLock::AutoRLock arl(mLock);
  if (mConverter.convertI420ToEncoderInput != nullptr) {
    return mConverter.convertI420ToEncoderInput(aSrcBits, aSrcWidth, aSrcHeight,
        aEncoderWidth, aEncoderHeight, aEncoderRect, aEncoderBits);
  }
  return -1;
}

int
I420ColorConverterHelper::getEncoderInputBufferInfo(int aSrcWidth,
                                                    int aSrcHeight,
                                                    int* aEncoderWidth,
                                                    int* aEncoderHeight,
                                                    ARect* aEncoderRect,
                                                    int* aEncoderBufferSize)
{
  if (!ensureLoaded()) {
    return -1;
  }

  RWLock::AutoRLock arl(mLock);
  if (mConverter.getEncoderInputBufferInfo != nullptr) {
    return mConverter.getEncoderInputBufferInfo(aSrcWidth, aSrcHeight,
        aEncoderWidth, aEncoderHeight, aEncoderRect, aEncoderBufferSize);
  }
  return -1;
}

} 
