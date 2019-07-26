





















#include "nscore.h"
#include "mozilla/GuardObjects.h"
#include "nsIVisualEventTracer.h"

#ifdef MOZ_VISUAL_EVENT_TRACER









#define MOZ_EVENT_TRACER_NAME_OBJECT(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eName, instance, name)



#define MOZ_EVENT_TRACER_COMPOUND_NAME(instance, name, name2) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eName, instance, name, name2)









#define MOZ_EVENT_TRACER_MARK(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eShot, instance, name)




















#define MOZ_EVENT_TRACER_WAIT(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eWait, instance, name)



#define MOZ_EVENT_TRACER_EXEC(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eExec, instance, name)






#define MOZ_EVENT_TRACER_DONE(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eDone, instance, name)





#define MOZ_EVENT_TRACER_WAIT_THREADSAFE(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eWait | mozilla::eventtracer::eThreadConcurrent, instance, name)
#define MOZ_EVENT_TRACER_EXEC_THREADSAFE(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eExec | mozilla::eventtracer::eThreadConcurrent, instance, name)
#define MOZ_EVENT_TRACER_DONE_THREASAFE(instance, name) \
  mozilla::eventtracer::Mark(mozilla::eventtracer::eDone | mozilla::eventtracer::eThreadConcurrent, instance, name)

#else 



#define MOZ_EVENT_TRACER_NAME_OBJECT(instance, name) (void)0
#define MOZ_EVENT_TRACER_COMPOUND_NAME(instance, name, name2) (void)0
#define MOZ_EVENT_TRACER_MARK(instance, name) (void)0
#define MOZ_EVENT_TRACER_WAIT(instance, name) (void)0
#define MOZ_EVENT_TRACER_EXEC(instance, name) (void)0
#define MOZ_EVENT_TRACER_DONE(instance, name) (void)0
#define MOZ_EVENT_TRACER_WAIT_THREADSAFE(instance, name) (void)0
#define MOZ_EVENT_TRACER_EXEC_THREADSAFE(instance, name) (void)0
#define MOZ_EVENT_TRACER_DONE_THREASAFE(instance, name) (void)0

#endif


namespace mozilla {
namespace eventtracer {


void Init();



void Shutdown();

enum MarkType {
  eNone, 
  eName, 

  eShot, 
  eWait, 
  eExec, 
  eDone, 
  eLast, 

  

  
  eThreadConcurrent = 0x10000
};














void Mark(uint32_t aType, void * aItem, 
          const char * aText, const char * aText2 = 0);












class NS_STACK_CLASS AutoEventTracer
{
public:
  AutoEventTracer(void * aInstance, 
               uint32_t aTypeOn, 
               uint32_t aTypeOff, 
               const char * aName, 
               const char * aName2 = 0 
               MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
    : mInstance(aInstance)
    , mName(aName)
    , mName2(aName2)
    , mTypeOn(aTypeOn)
    , mTypeOff(aTypeOff)
  {
    MOZ_GUARD_OBJECT_NOTIFIER_INIT;

    ::mozilla::eventtracer::Mark(mTypeOn, mInstance, mName, mName2);
  }

  ~AutoEventTracer()
  {
    ::mozilla::eventtracer::Mark(mTypeOff, mInstance, mName, mName2);
  }

private:
  void * mInstance;
  const char * mName;
  const char * mName2;
  uint32_t mTypeOn;
  uint32_t mTypeOff;

  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
};

#ifdef MOZ_VISUAL_EVENT_TRACER



class VisualEventTracer : public nsIVisualEventTracer
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVISUALEVENTTRACER
};

#define NS_VISUALEVENTTRACER_CID \
  { 0xb9e5e102, 0xc2f4, 0x497a,  \
    { 0xa6, 0xe4, 0x54, 0xde, 0xf3, 0x71, 0xf3, 0x9d } }
#define NS_VISUALEVENTTRACER_CONTRACTID "@mozilla.org/base/visual-event-tracer;1"
#define NS_VISUALEVENTTRACER_CLASSNAME "Visual Event Tracer"

#endif

} 
} 
