





#ifndef mozilla_dom_bluetooth_mediametadata_h__
#define mozilla_dom_bluetooth_mediametadata_h__

#include "jsapi.h"
#include "nsString.h"

BEGIN_BLUETOOTH_NAMESPACE

class MediaMetaData
{
public:
  MediaMetaData();

  nsresult Init(JSContext* aCx, const jsval* aVal);

  nsString mAlbum;
  nsString mArtist;
  int64_t mDuration;
  int64_t mMediaNumber;
  nsString mTitle;
  int64_t mTotalMediaCount;
};

END_BLUETOOTH_NAMESPACE

#endif
