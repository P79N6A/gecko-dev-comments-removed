







































#ifndef __nsSystemSoundService_h__
#define __nsSystemSoundService_h__

#include "nsSound.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"

class nsSystemSoundPlayer;

class nsSystemSoundService : public nsSystemSoundServiceBase
{
public:
  nsSystemSoundService();
  virtual ~nsSystemSoundService();

  NS_DECL_ISYSTEMSOUNDSERVICE_GETINSTANCE(nsSystemSoundService)

  NS_DECL_ISUPPORTS

  NS_IMETHOD Beep();
  NS_IMETHOD PlayAlias(const nsAString &aSoundAlias);
  NS_IMETHOD PlayEventSound(PRUint32 aEventID);

protected:
  virtual nsresult Init();

private:
  nsCOMPtr<nsIThread> mPlayerThread;

  nsresult PostPlayer(nsSystemSoundPlayer *aPlayer);
};

#endif 
