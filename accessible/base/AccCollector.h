



#ifndef mozilla_a11y_AccCollector_h__
#define mozilla_a11y_AccCollector_h__

#include "Filters.h"

#include "nsTArray.h"

namespace mozilla {
namespace a11y {

class Accessible;





class AccCollector
{
public:
  AccCollector(Accessible* aRoot, filters::FilterFuncPtr aFilterFunc);
  virtual ~AccCollector();

  


  uint32_t Count();

  


  Accessible* GetAccessibleAt(uint32_t aIndex);

  


  virtual int32_t GetIndexAt(Accessible* aAccessible);

protected:
  


  Accessible* EnsureNGetObject(uint32_t aIndex);

  


  int32_t EnsureNGetIndex(Accessible* aAccessible);

  


  virtual void AppendObject(Accessible* aAccessible);

  filters::FilterFuncPtr mFilterFunc;
  Accessible* mRoot;
  uint32_t mRootChildIdx;

  nsTArray<Accessible*> mObjects;

private:
  AccCollector();
  AccCollector(const AccCollector&);
  AccCollector& operator =(const AccCollector&);
};





class EmbeddedObjCollector MOZ_FINAL : public AccCollector
{
public:
  virtual ~EmbeddedObjCollector() { }

public:
  virtual int32_t GetIndexAt(Accessible* aAccessible);

protected:
  
  explicit EmbeddedObjCollector(Accessible* aRoot) :
    AccCollector(aRoot, filters::GetEmbeddedObject) { }

  virtual void AppendObject(Accessible* aAccessible);

  friend class Accessible;
};

} 
} 

#endif
