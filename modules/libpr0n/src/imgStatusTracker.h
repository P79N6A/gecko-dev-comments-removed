






































#ifndef imgStatusTracker_h__
#define imgStatusTracker_h__

class nsIntRect;
class imgIContainer;
class imgRequest;
class imgRequestProxy;
class imgStatusNotifyRunnable;
class imgRequestNotifyRunnable;
namespace mozilla {
namespace imagelib {
class Image;
} 
} 


#include "nsCOMPtr.h"
#include "nsIRunnable.h"
#include "prtypes.h"
#include "nscore.h"

enum {
  stateRequestStarted    = PR_BIT(0),
  stateHasSize           = PR_BIT(1),
  stateDecodeStarted     = PR_BIT(2),
  stateDecodeStopped     = PR_BIT(3),
  stateFrameStopped      = PR_BIT(4),
  stateRequestStopped    = PR_BIT(5)
};












class imgStatusTracker
{
public:
  
  
  
  imgStatusTracker(mozilla::imagelib::Image* aImage);
  imgStatusTracker(const imgStatusTracker& aOther);

  
  
  
  
  
  void Notify(imgRequest* request, imgRequestProxy* proxy);

  
  
  
  
  
  void NotifyCurrentState(imgRequestProxy* proxy);

  
  
  
  
  void SyncNotify(imgRequestProxy* proxy);

  
  
  
  
  void EmulateRequestFinished(imgRequestProxy* proxy, nsresult aStatus,
                              PRBool aOnlySendStopRequest);

  
  
  PRBool IsLoading() const;

  
  PRUint32 GetImageStatus() const;

  
  
  

  
  void RecordCancel();

  
  
  void RecordLoaded();

  
  
  void RecordDecoded();

  
  void RecordStartDecode();
  void SendStartDecode(imgRequestProxy* aProxy);
  void RecordStartContainer(imgIContainer* aContainer);
  void SendStartContainer(imgRequestProxy* aProxy, imgIContainer* aContainer);
  void RecordStartFrame(PRUint32 aFrame);
  void SendStartFrame(imgRequestProxy* aProxy, PRUint32 aFrame);
  void RecordDataAvailable(PRBool aCurrentFrame, const nsIntRect* aRect);
  void SendDataAvailable(imgRequestProxy* aProxy, PRBool aCurrentFrame, const nsIntRect* aRect);
  void RecordStopFrame(PRUint32 aFrame);
  void SendStopFrame(imgRequestProxy* aProxy, PRUint32 aFrame);
  void RecordStopContainer(imgIContainer* aContainer);
  void SendStopContainer(imgRequestProxy* aProxy, imgIContainer* aContainer);
  void RecordStopDecode(nsresult status, const PRUnichar* statusArg);
  void SendStopDecode(imgRequestProxy* aProxy, nsresult aStatus, const PRUnichar* statusArg);
  void RecordDiscard();
  void SendDiscard(imgRequestProxy* aProxy);

  
  void RecordFrameChanged(imgIContainer* aContainer,
                          const nsIntRect* aDirtyRect);
  void SendFrameChanged(imgRequestProxy* aProxy, imgIContainer* aContainer,
                        const nsIntRect* aDirtyRect);

  
  void RecordStartRequest();
  void SendStartRequest(imgRequestProxy* aProxy);
  void RecordStopRequest(PRBool aLastPart, nsresult aStatus);
  void SendStopRequest(imgRequestProxy* aProxy, PRBool aLastPart, nsresult aStatus);

private:
  friend class imgStatusNotifyRunnable;
  friend class imgRequestNotifyRunnable;

  nsCOMPtr<nsIRunnable> mRequestRunnable;

  
  
  mozilla::imagelib::Image* mImage;
  PRUint32 mState;
  nsresult mImageStatus;
  PRPackedBool mHadLastPart;
};

#endif
