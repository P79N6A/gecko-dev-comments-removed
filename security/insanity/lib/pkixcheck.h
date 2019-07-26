
















#ifndef insanity__pkixcheck_h
#define insanity__pkixcheck_h

#include "pkixutil.h"
#include "certt.h"

namespace insanity { namespace pkix {

Result CheckTimes(const CERTCertificate* cert, PRTime time);

} } 

#endif 
