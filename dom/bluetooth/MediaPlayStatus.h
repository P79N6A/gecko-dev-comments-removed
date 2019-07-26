





#ifndef mozilla_dom_bluetooth_mediaplaystatus_h__
#define mozilla_dom_bluetooth_mediaplaystatus_h__

#include "jsapi.h"
#include "nsString.h"

BEGIN_BLUETOOTH_NAMESPACE

class MediaPlayStatus
{
public:
  MediaPlayStatus();

  nsresult Init(JSContext* aCx, const jsval* aVal);

  int64_t mDuration;
  nsString mPlayStatus;
  int64_t mPosition;
};

END_BLUETOOTH_NAMESPACE

#endif
