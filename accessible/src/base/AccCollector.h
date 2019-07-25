



#ifndef AccCollector_h_
#define AccCollector_h_

#include "filters.h"

#include "nscore.h"
#include "nsTArray.h"





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





class EmbeddedObjCollector : public AccCollector
{
public:
  virtual ~EmbeddedObjCollector() { };

public:
  virtual int32_t GetIndexAt(Accessible* aAccessible);

protected:
  
  EmbeddedObjCollector(Accessible* aRoot) :
    AccCollector(aRoot, filters::GetEmbeddedObject) { }

  virtual void AppendObject(Accessible* aAccessible);

  friend class Accessible;
};

#endif
