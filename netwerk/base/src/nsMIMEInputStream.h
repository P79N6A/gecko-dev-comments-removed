










































#ifndef _nsMIMEInputStream_h_
#define _nsMIMEInputStream_h_

#include "nsIMIMEInputStream.h"

#define NS_MIMEINPUTSTREAM_CLASSNAME  "nsMIMEInputStream"
#define NS_MIMEINPUTSTREAM_CONTRACTID "@mozilla.org/network/mime-input-stream;1"
#define NS_MIMEINPUTSTREAM_CID                       \
{ /* 58a1c31c-1dd2-11b2-a3f6-d36949d48268 */         \
    0x58a1c31c,                                      \
    0x1dd2,                                          \
    0x11b2,                                          \
    {0xa3, 0xf6, 0xd3, 0x69, 0x49, 0xd4, 0x82, 0x68} \
}

extern NS_METHOD nsMIMEInputStreamConstructor(nsISupports *outer,
                                              REFNSIID iid,
                                              void **result);

#endif 
