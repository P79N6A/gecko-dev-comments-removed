





#ifndef mozilla_AppleUtils_h
#define mozilla_AppleUtils_h

#include <AudioToolbox/AudioToolbox.h>
#include "mozilla/Attributes.h"
#include "nsError.h"

namespace mozilla {

struct AppleUtils {
  
  static nsresult GetProperty(AudioFileStreamID aAudioFileStream,
                              AudioFileStreamPropertyID aPropertyID,
                              void* aData);

  
  static void SetCFDict(CFMutableDictionaryRef dict,
                        const char* key,
                        const char* value);
  
  static void SetCFDict(CFMutableDictionaryRef dict,
                        const char* key,
                        int32_t value);
  
  static void SetCFDict(CFMutableDictionaryRef dict,
                        const char* key,
                        bool value);

  
  
  
  static nsresult GetRichestDecodableFormat(
    AudioFileStreamID aAudioFileStream, AudioStreamBasicDescription& aFormat);
};



template <class T>
class AutoCFRelease {
public:
  MOZ_IMPLICIT AutoCFRelease(T aRef)
    : mRef(aRef)
  {
  }
  ~AutoCFRelease()
  {
    if (mRef) {
      CFRelease(mRef);
    }
  }
  
  operator T()
  {
    return mRef;
  }
  
  T* receive()
  {
    return &mRef;
  }

private:
  
  AutoCFRelease<T>& operator=(const AutoCFRelease<T>&);
  T mRef;
};

} 

#endif 
