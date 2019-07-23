







































#include "nscore.h"
#include "plstr.h"
#include "prlink.h"

#include "nsSound.h"
#include "nsAppShell.h"

#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsCOMPtr.h"

#include <stdio.h>
#include <unistd.h>


NS_IMPL_THREADSAFE_ISUPPORTS2(nsSound, nsISound, nsIStreamLoaderObserver)


nsSound::nsSound()
{
  
}

nsSound::~nsSound()
{

}

NS_IMETHODIMP nsSound::Init()
{
  
#ifdef DEBUG_faulkner
  fprintf(stderr, "\n////////// nsSound::Init() in xlib called //////////\n");
#endif 
  return NS_OK;
}

NS_IMETHODIMP nsSound::OnStreamComplete(nsIStreamLoader *aLoader,
                                        nsISupports *context,
                                        nsresult aStatus,
                                        PRUint32 stringLen,
                                        const PRUint8 *string)
{
#ifdef DEBUG_faulkner
  fprintf(stderr, "\n////////// nsSound::Init() in xlib called //////////\n");
#endif 
  return NS_OK;
}

NS_METHOD nsSound::Beep()
{
#ifdef DEBUG_faulkner
  fprintf(stderr, "\n////////// nsSound::Beep() in xlib called //////////\n");
#endif 

  XBell(nsAppShell::mDisplay, 80);
  return NS_OK;
}

NS_METHOD nsSound::Play(nsIURL *aURL)
{
#ifdef DEBUG_faulkner
  fprintf(stderr, "\n////////// nsSound::Play() in xlib called //////////\n");
#endif 
  return NS_OK;
}

NS_IMETHODIMP nsSound::PlaySystemSound(const nsAString &aSoundAlias)
{
  return Beep();
}
