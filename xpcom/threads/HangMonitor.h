





#ifndef mozilla_HangMonitor_h
#define mozilla_HangMonitor_h

#include "mozilla/MemoryReporting.h"
#include "nsString.h"

namespace mozilla {
namespace HangMonitor {




enum ActivityType
{
  
  kUIActivity,

  
  kActivityNoUIAVail,

  
  kActivityUIAVail,

  
  kGeneralActivity
};




void Startup();




void Shutdown();





class HangAnnotations
{
public:
  virtual ~HangAnnotations() {}

  virtual void AddAnnotation(const nsAString& aName, const int32_t aData) = 0;
  virtual void AddAnnotation(const nsAString& aName, const double aData) = 0;
  virtual void AddAnnotation(const nsAString& aName, const nsAString& aData) = 0;
  virtual void AddAnnotation(const nsAString& aName, const nsACString& aData) = 0;
  virtual void AddAnnotation(const nsAString& aName, const bool aData) = 0;

  class Enumerator
  {
  public:
    virtual ~Enumerator() {}
    virtual bool Next(nsAString& aOutName, nsAString& aOutValue) = 0;
  };

  virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const = 0;
  virtual bool IsEmpty() const = 0;
  virtual bool GetEnumerator(Enumerator **aOutEnum) = 0;
};

class Annotator
{
public:
  



  virtual void AnnotateHang(HangAnnotations& aAnnotations) = 0;
};






void RegisterAnnotator(Annotator& aAnnotator);






void UnregisterAnnotator(Annotator& aAnnotator);







void NotifyActivity(ActivityType activityType = kGeneralActivity);





void Suspend();

} 
} 

#endif 
