



#ifndef OTS_GLYF_H_
#define OTS_GLYF_H_

#include <new>
#include <utility>  
#include <vector>

#include "ots.h"

namespace ots {

struct OpenTypeGLYF {
  std::vector<std::pair<const uint8_t*, size_t> > iov;
};

}  

#endif  
