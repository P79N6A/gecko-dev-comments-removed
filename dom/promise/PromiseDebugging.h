





#ifndef mozilla_dom_PromiseDebugging_h
#define mozilla_dom_PromiseDebugging_h

namespace mozilla {
namespace dom {

class Promise;
struct PromiseDebuggingStateHolder;
class GlobalObject;

class PromiseDebugging
{
public:
  static void GetState(GlobalObject&, Promise& aPromise,
                       PromiseDebuggingStateHolder& aState);
};

} 
} 

#endif 
