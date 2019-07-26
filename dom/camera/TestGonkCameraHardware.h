















#ifndef DOM_CAMERA_TESTGONKCAMERAHARDWARE_H
#define DOM_CAMERA_TESTGONKCAMERAHARDWARE_H

#include "GonkCameraHwMgr.h"

namespace android {

class TestGonkCameraHardware : public android::GonkCameraHardware
{
public:
  virtual int AutoFocus() MOZ_OVERRIDE;
  virtual int TakePicture() MOZ_OVERRIDE;
  virtual int StartPreview() MOZ_OVERRIDE;
  virtual int PushParameters(const mozilla::GonkCameraParameters& aParams) MOZ_OVERRIDE;
  virtual nsresult PullParameters(mozilla::GonkCameraParameters& aParams) MOZ_OVERRIDE;
  virtual int StartRecording() MOZ_OVERRIDE;
  virtual int StopRecording() MOZ_OVERRIDE;
  virtual int SetListener(const sp<GonkCameraListener>& aListener) MOZ_OVERRIDE;
  virtual int StoreMetaDataInBuffers(bool aEnabled) MOZ_OVERRIDE;

  virtual int
  PushParameters(const CameraParameters& aParams) MOZ_OVERRIDE
  {
    return GonkCameraHardware::PushParameters(aParams);
  }

  virtual void
  PullParameters(CameraParameters& aParams) MOZ_OVERRIDE
  {
    GonkCameraHardware::PullParameters(aParams);
  }

  TestGonkCameraHardware(mozilla::nsGonkCameraControl* aTarget,
                         uint32_t aCameraId,
                         const sp<Camera>& aCamera);
  virtual ~TestGonkCameraHardware();

  virtual nsresult Init() MOZ_OVERRIDE;

protected:
  const nsCString TestCase();
  const nsCString GetExtraParameters();
  bool IsTestCaseInternal(const char* aTest, const char* aFile, int aLine);
  int TestCaseError(int aDefaultError);

  int StartAutoFocusMoving(bool aIsMoving);

private:
  TestGonkCameraHardware(const TestGonkCameraHardware&) MOZ_DELETE;
  TestGonkCameraHardware& operator=(const TestGonkCameraHardware&) MOZ_DELETE;
};

#define IsTestCase(test)  IsTestCaseInternal((test), __FILE__, __LINE__)

} 

#endif 
