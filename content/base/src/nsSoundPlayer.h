






































#ifndef __nsSoundPlayer_h__
#define __nsSoundPlayer_h__

#include "nsISoundPlayer.h"
#include "nsIDOMEventListener.h"

#ifdef MOZ_MEDIA
#include "nsIDOMHTMLMediaElement.h"
#include "nsCOMArray.h"
#endif 

class nsSoundPlayer : public nsISoundPlayer,
                      public nsIDOMEventListener
{
public:
  nsSoundPlayer();
  virtual ~nsSoundPlayer();

  static nsSoundPlayer* GetInstance();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISOUNDPLAYER
  NS_DECL_NSIDOMEVENTLISTENER

protected:
  static nsSoundPlayer* sInstance;

#ifdef MOZ_MEDIA
  nsCOMArray<nsIDOMHTMLMediaElement> mAudioElements;

  nsresult CreateAudioElement(nsIDOMHTMLMediaElement **aElement);
  void RemoveEventListeners(nsIDOMHTMLMediaElement *aElement);
#endif 
};

#endif 
