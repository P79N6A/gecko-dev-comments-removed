



#ifndef DOM_CAMERA_DOMCAMERADETECTEDFACE_H
#define DOM_CAMERA_DOMCAMERADETECTEDFACE_H

#include "mozilla/dom/CameraControlBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/DOMRect.h"
#include "ICameraControl.h"

namespace mozilla {

namespace dom {

class DOMCameraPoint MOZ_FINAL : public nsISupports
                               , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMCameraPoint)

  
  
  
  
  
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  DOMCameraPoint(nsISupports* aParent, const ICameraControl::Point& aPoint)
    : mParent(aParent)
    , mX(aPoint.x)
    , mY(aPoint.y)
  {
    SetIsDOMBinding();
  }

  void
  SetPoint(int32_t aX, int32_t aY)
  {
    mX = aX;
    mY = aY;
  }

  int32_t X() { return mX; }
  int32_t Y() { return mY; }

  void SetX(int32_t aX) { mX = aX; }
  void SetY(int32_t aY) { mY = aY; }

  nsISupports*
  GetParentObject() const
  {
    MOZ_ASSERT(mParent);
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

protected:
  virtual ~DOMCameraPoint() { }

  nsCOMPtr<nsISupports> mParent;
  int32_t mX;
  int32_t mY;
};

class DOMCameraDetectedFace MOZ_FINAL : public nsISupports
                                      , public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(DOMCameraDetectedFace)

  
  
  
  
  
  static bool HasSupport(JSContext* aCx, JSObject* aGlobal);

  DOMCameraDetectedFace(nsISupports* aParent, const ICameraControl::Face& aFace);

  uint32_t Id()       { return mId; }
  uint32_t Score()    { return mScore; }
  bool HasLeftEye()   { return mLeftEye; }
  bool HasRightEye()  { return mRightEye; }
  bool HasMouth()     { return mMouth; }

  dom::DOMRect* Bounds()        { return mBounds; }

  DOMCameraPoint* GetLeftEye()  { return mLeftEye; }
  DOMCameraPoint* GetRightEye() { return mRightEye; }
  DOMCameraPoint* GetMouth()    { return mMouth; }

  nsISupports*
  GetParentObject() const
  {
    MOZ_ASSERT(mParent);
    return mParent;
  }

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

protected:
  virtual ~DOMCameraDetectedFace() { }

  nsCOMPtr<nsISupports> mParent;

  uint32_t mId;
  uint32_t mScore;

  nsRefPtr<dom::DOMRect> mBounds;

  nsRefPtr<DOMCameraPoint> mLeftEye;
  nsRefPtr<DOMCameraPoint> mRightEye;
  nsRefPtr<DOMCameraPoint> mMouth;
};

} 

} 

#endif 
