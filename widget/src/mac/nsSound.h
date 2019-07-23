




































#ifndef __nsSound_h__
#define __nsSound_h__

#include "nsISound.h"
#include "nsSupportsArray.h"

class nsIURI;
class nsIChannel;
class nsICacheSession;

class nsSound : public nsISound
{
public:

                      nsSound();
  virtual             ~nsSound();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISOUND

  nsresult            AddRequest(nsISupports* aSoundRequest);
  nsresult            RemoveRequest(nsISupports* aSoundRequest);

  nsresult            GetCacheSession(nsICacheSession** outCacheSession);
  nsresult            GetSoundFromCache(nsIURI* inURI, nsISupports** outSound);
  nsresult            PutSoundInCache(nsIChannel* inChannel, PRUint32 inDataSize, nsISupports* inSound);

protected:

  nsresult            GetSoundResourceName(const char* inSoundName, StringPtr outResourceName);

protected:

  nsSupportsArray     mSoundRequests;    

};

#endif 
