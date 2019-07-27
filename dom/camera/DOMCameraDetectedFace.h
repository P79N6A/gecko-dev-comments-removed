



#ifndef DOM_CAMERA_DOMCAMERADETECTEDFACE_H
#define DOM_CAMERA_DOMCAMERADETECTEDFACE_H

#include "mozilla/dom/CameraControlBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/DOMRect.h"
#include "mozilla/dom/DOMPoint.h"
#include "ICameraControl.h"

namespace mozilla {

namespace dom {

class DOMCameraDetectedFace final : public nsISupports
                                  , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMCameraDetectedFace)

  
  
  
  
  
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  static already_AddRefed<DOMCameraDetectedFace> Constructor(const GlobalObject& aGlobal,
                                                             const dom::CameraDetectedFaceInit& aFace,
                                                             ErrorResult& aRv);

  DOMCameraDetectedFace(nsISupports* aParent, const ICameraControl::Face& aFace);

  uint32_t Id()       { return mId; }
  uint32_t Score()    { return mScore; }
  bool HasLeftEye()   { return mLeftEye; }
  bool HasRightEye()  { return mRightEye; }
  bool HasMouth()     { return mMouth; }

  dom::DOMRect* Bounds()        { return mBounds; }

  dom::DOMPoint* GetLeftEye()  { return mLeftEye; }
  dom::DOMPoint* GetRightEye() { return mRightEye; }
  dom::DOMPoint* GetMouth()    { return mMouth; }

  nsISupports*
  GetParentObject() const
  {
    MOZ_ASSERT(mParent);
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

protected:
  DOMCameraDetectedFace(nsISupports* aParent, const dom::CameraDetectedFaceInit& aFace);
  virtual ~DOMCameraDetectedFace() { }

  nsCOMPtr<nsISupports> mParent;

  uint32_t mId;
  uint32_t mScore;

  nsRefPtr<dom::DOMRect> mBounds;

  nsRefPtr<dom::DOMPoint> mLeftEye;
  nsRefPtr<dom::DOMPoint> mRightEye;
  nsRefPtr<dom::DOMPoint> mMouth;
};

} 

} 

#endif 
