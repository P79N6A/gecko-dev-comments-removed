



#ifndef RemoteSpellcheckEngineChild_h_
#define RemoteSpellcheckEngineChild_h_

#include "mozilla/PRemoteSpellcheckEngineChild.h"

namespace mozilla {
class RemoteSpellcheckEngineChild : public mozilla::PRemoteSpellcheckEngineChild
{
public:
  RemoteSpellcheckEngineChild();

  ~RemoteSpellcheckEngineChild();
};

} 

#endif 
