






































#ifndef _NSTLSSOCKETPROVIDER_H_
#define _NSTLSSOCKETPROVIDER_H_

#include "nsISocketProvider.h"


#define NS_STARTTLSSOCKETPROVIDER_CLASSNAME  "Mozilla STARTTLS Capable Socket Provider Component"
#define NS_STARTTLSSOCKETPROVIDER_CID \
{ /* b9507aec-1dd1-11b2-8cd5-c48ee0c50307 */         \
     0xb9507aec,                                     \
     0x1dd1,                                         \
     0x11b2,                                         \
    {0x8c, 0xd5, 0xc4, 0x8e, 0xe0, 0xc5, 0x03, 0x07} \
}

class nsTLSSocketProvider : public nsISocketProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISOCKETPROVIDER

  
  nsTLSSocketProvider();
  virtual ~nsTLSSocketProvider();
};

#endif 
