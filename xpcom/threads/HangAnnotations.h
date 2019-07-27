





#ifndef mozilla_HangAnnotations_h
#define mozilla_HangAnnotations_h

#include <set>

#include "mozilla/MemoryReporting.h"
#include "mozilla/Mutex.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/Vector.h"
#include "nsString.h"

namespace mozilla {
namespace HangMonitor {





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
  virtual UniquePtr<Enumerator> GetEnumerator() = 0;
};

typedef UniquePtr<HangAnnotations> HangAnnotationsPtr;
typedef Vector<HangAnnotationsPtr> HangAnnotationsVector;

class Annotator
{
public:
  



  virtual void AnnotateHang(HangAnnotations& aAnnotations) = 0;
};






void RegisterAnnotator(Annotator& aAnnotator);






void UnregisterAnnotator(Annotator& aAnnotator);





HangAnnotationsPtr ChromeHangAnnotatorCallout();

namespace Observer {

class Annotators
{
public:
  Annotators();
  ~Annotators();

  bool Register(Annotator& aAnnotator);
  bool Unregister(Annotator& aAnnotator);

  HangAnnotationsPtr GatherAnnotations();

private:
  Mutex                mMutex;
  std::set<Annotator*> mAnnotators;
};

} 

} 
} 

#endif 
