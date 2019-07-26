



#ifndef mozilla_dom_ResourceStatsConotrl_h
#define mozilla_dom_ResourceStatsConotrl_h


struct JSContext;
class JSObject;

namespace mozilla {
namespace dom {

class ResourceStatsControl
{
public:
  static bool HasResourceStatsSupport(JSContext* ,
                                      JSObject* aGlobal);
};

} 
} 

#endif 
