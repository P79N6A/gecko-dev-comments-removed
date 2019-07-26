



#include "CameraControlImpl.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace mozilla {
  class RecorderProfileManager;

  namespace layers {
    class GraphicBufferLocked;
  }
}





class FallbackCameraControl : public CameraControlImpl
{
public:
  FallbackCameraControl(uint32_t aCameraId) : CameraControlImpl(aCameraId) { }

  void OnAutoFocusComplete(bool aSuccess);
  void OnAutoFocusMoving(bool aIsMoving) { }
  void OnTakePictureComplete(uint8_t* aData, uint32_t aLength) { }
  void OnTakePictureError() { }
  void OnNewPreviewFrame(layers::GraphicBufferLocked* aBuffer) { }
  void OnRecorderEvent(int msg, int ext1, int ext2) { }
  void OnError(CameraControlListener::CameraErrorContext aWhere,
               CameraControlListener::CameraError aError) { }

  virtual nsresult Set(uint32_t aKey, const nsAString& aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, nsAString& aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Set(uint32_t aKey, double aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, double& aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Set(uint32_t aKey, int32_t aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, int32_t& aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Set(uint32_t aKey, int64_t aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, int64_t& aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Set(uint32_t aKey, const Size& aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, Size& aValue) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Set(uint32_t aKey, const nsTArray<Region>& aRegions) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, nsTArray<Region>& aRegions) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }

  virtual nsresult SetLocation(const Position& aLocation) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }

  virtual nsresult Get(uint32_t aKey, nsTArray<Size>& aSizes) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, nsTArray<nsString>& aValues) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }
  virtual nsresult Get(uint32_t aKey, nsTArray<double>& aValues) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }

  nsresult PushParameters() { return NS_ERROR_FAILURE; }
  nsresult PullParameters() { return NS_ERROR_FAILURE; }

protected:
  ~FallbackCameraControl();

  virtual nsresult StartPreviewImpl() { return NS_ERROR_FAILURE; }
  virtual nsresult StopPreviewImpl() { return NS_ERROR_FAILURE; }
  virtual nsresult AutoFocusImpl(bool aCancelExistingCall) { return NS_ERROR_FAILURE; }
  virtual nsresult StartFaceDetectionImpl() { return NS_ERROR_FAILURE; }
  virtual nsresult StopFaceDetectionImpl() { return NS_ERROR_FAILURE; }
  virtual nsresult TakePictureImpl() { return NS_ERROR_FAILURE; }
  virtual nsresult StartRecordingImpl(DeviceStorageFileDescriptor* aFileDescriptor,
                                      const StartRecordingOptions* aOptions = nullptr)
                                        { return NS_ERROR_FAILURE; }
  virtual nsresult StopRecordingImpl() { return NS_ERROR_FAILURE; }
  virtual nsresult PushParametersImpl() { return NS_ERROR_FAILURE; }
  virtual nsresult PullParametersImpl() { return NS_ERROR_FAILURE; }
  virtual already_AddRefed<RecorderProfileManager> GetRecorderProfileManagerImpl() { return nullptr; }

private:
  FallbackCameraControl(const FallbackCameraControl&) MOZ_DELETE;
  FallbackCameraControl& operator=(const FallbackCameraControl&) MOZ_DELETE;
};
