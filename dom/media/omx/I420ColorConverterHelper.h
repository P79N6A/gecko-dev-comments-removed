





#ifndef I420_COLOR_CONVERTER_HELPER_H
#define I420_COLOR_CONVERTER_HELPER_H

#include <utils/RWLock.h>
#include <media/editor/II420ColorConverter.h>

#include <mozilla/Attributes.h>

namespace android {

class I420ColorConverterHelper {
public:
  I420ColorConverterHelper();
  ~I420ColorConverterHelper();

  int getDecoderOutputFormat();

  int convertDecoderOutputToI420(void* aDecoderBits,
                                 int aDecoderWidth,
                                 int aDecoderHeight,
                                 ARect aDecoderRect,
                                 void* aDstBits);

  int getEncoderInputFormat();

  int convertI420ToEncoderInput(void* aSrcBits,
                                int aSrcWidth,
                                int aSrcHeight,
                                int aEncoderWidth,
                                int aEncoderHeight,
                                ARect aEncoderRect,
                                void* aEncoderBits);

  int getEncoderInputBufferInfo(int aSrcWidth,
                                int aSrcHeight,
                                int* aEncoderWidth,
                                int* aEncoderHeight,
                                ARect* aEncoderRect,
                                int* aEncoderBufferSize);

private:
  mutable RWLock mLock;
  void *mHandle;
  II420ColorConverter mConverter;

  bool loadLocked();
  bool loadedLocked() const;
  void unloadLocked();

  bool ensureLoaded();

  I420ColorConverterHelper(const I420ColorConverterHelper &) MOZ_DELETE;
  const I420ColorConverterHelper &operator=(const I420ColorConverterHelper &) MOZ_DELETE;
};

} 

#endif 
