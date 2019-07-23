







































#ifndef nsTopProgressNotifier_h__
#define nsTopProgressNotifier_h__

#include "nsCOMPtr.h"
#include "nsIXPINotifier.h"
#include "nsVoidArray.h"
#include "prlock.h"

class nsTopProgressListener : public nsIXPIListener
{
    public:

        nsTopProgressListener();
        virtual ~nsTopProgressListener();

        long RegisterListener(nsIXPIListener * newListener);
        void UnregisterListener(long id);
        void SetActiveListener(nsIXPIListener *aListener) 
            { mActive = aListener; }

        NS_DECL_ISUPPORTS

        
        NS_DECL_NSIXPILISTENER
   
   private:
        nsVoidArray     *mListeners;
        PRLock          *mLock;
        nsCOMPtr<nsIXPIListener>  mActive;
};

#endif
