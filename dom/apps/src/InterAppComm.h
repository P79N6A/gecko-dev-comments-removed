



#ifndef mozilla_dom_apps_InterAppComm_h
#define mozilla_dom_apps_InterAppComm_h


struct JSContext;
class JSObject;

namespace mozilla {
namespace dom {

class InterAppComm
{
public:
  static bool EnabledForScope(JSContext* , JSObject* aObj);
};

} 
} 

#endif 
