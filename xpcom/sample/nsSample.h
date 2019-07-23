










































#include "nsISample.h"












#define NS_SAMPLE_CID \
{ 0x7cb5b7a0, 0x7d7, 0x11d3, { 0xbd, 0xe2, 0x0, 0x0, 0x64, 0x65, 0x73, 0x74 } }

#define NS_SAMPLE_CONTRACTID "@mozilla.org/sample;1"


class nsSampleImpl : public nsISample
{
public:
    nsSampleImpl();

    






    
    NS_DECL_ISUPPORTS

    









    NS_DECL_NSISAMPLE

    















    
    

    




    

    






    

private:
    ~nsSampleImpl();

    char* mValue;
};
