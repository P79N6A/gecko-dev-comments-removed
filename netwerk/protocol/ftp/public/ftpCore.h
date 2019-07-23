




































#ifndef __ftpCore_h___
#define __ftpCore_h___

#include "nsNetError.h"




#define NS_NET_STATUS_BEGIN_FTP_TRANSACTION \
    NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_NETWORK, 27)

#define NS_NET_STATUS_END_FTP_TRANSACTION \
    NS_ERROR_GENERATE_SUCCESS(NS_ERROR_MODULE_NETWORK, 28)

#endif 
