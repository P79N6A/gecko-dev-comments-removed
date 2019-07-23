











































#ifndef nsStubImageDecoderObserver_h_
#define nsStubImageDecoderObserver_h_

#include "imgIDecoderObserver.h"












class nsStubImageDecoderObserver : public imgIDecoderObserver {
public:
    NS_DECL_IMGICONTAINEROBSERVER
    NS_DECL_IMGIDECODEROBSERVER
};

#endif
