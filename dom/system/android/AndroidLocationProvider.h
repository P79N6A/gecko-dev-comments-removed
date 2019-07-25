



































#ifndef AndroidLocationProvider_h
#define AndroidLocationProvider_h

#include "nsIGeolocationProvider.h"

class AndroidLocationProvider : public nsIGeolocationProvider
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIGEOLOCATIONPROVIDER

    AndroidLocationProvider();
private:
    ~AndroidLocationProvider();
};

#endif 
