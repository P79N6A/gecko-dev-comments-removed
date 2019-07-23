







































#ifndef nsSound_h_
#define nsSound_h_

#include "nsISound.h"
#include "nsIStreamLoader.h"

class nsSound : public nsISound,
                public nsIStreamLoaderObserver
{
public: 
    nsSound();
    virtual ~nsSound();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISOUND
    NS_DECL_NSISTREAMLOADEROBSERVER
};

#endif
