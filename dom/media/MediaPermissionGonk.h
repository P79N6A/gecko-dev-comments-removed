



#ifndef DOM_MEDIA_MEDIAPERMISSIONGONK_H
#define DOM_MEDIA_MEDIAPERMISSIONGONK_H

#include "nsError.h"
#include "nsIObserver.h"
#include "nsISupportsImpl.h"
#include "GetUserMediaRequest.h"

namespace mozilla {





class MediaPermissionManager : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static MediaPermissionManager* GetInstance();
  virtual ~MediaPermissionManager();

private:
  MediaPermissionManager();
  nsresult Deinit();
  nsresult HandleRequest(nsRefPtr<dom::GetUserMediaRequest> &req);
};

} 
#endif 

