
































#ifndef IMPDPARSER_H_
#define IMPDPARSER_H_

#include "MPD.h"

namespace mozilla {
namespace net {

class IMPDParser
{
public:
  
  
  virtual MPD* Parse() = 0;
};

}
}

#endif 
