





#ifndef mozilla_dom_CPOWManagerGetter_h
#define mozilla_dom_CPOWManagerGetter_h

namespace mozilla {

namespace jsipc {
class CPOWManager;
} 

namespace dom {
class CPOWManagerGetter
{
public:
  virtual mozilla::jsipc::CPOWManager* GetCPOWManager() = 0;
};
} 

} 

#endif 
