



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

  


  PRUint32 Count();

  


  Accessible* GetAccessibleAt(PRUint32 aIndex);

  


  virtual PRInt32 GetIndexAt(Accessible* aAccessible);

protected:
  


  Accessible* EnsureNGetObject(PRUint32 aIndex);

  


  PRInt32 EnsureNGetIndex(Accessible* aAccessible);

  


  virtual void AppendObject(Accessible* aAccessible);

  filters::FilterFuncPtr mFilterFunc;
  Accessible* mRoot;
  PRUint32 mRootChildIdx;

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
  virtual PRInt32 GetIndexAt(Accessible* aAccessible);

protected:
  
  EmbeddedObjCollector(Accessible* aRoot) :
    AccCollector(aRoot, filters::GetEmbeddedObject) { }

  virtual void AppendObject(Accessible* aAccessible);

  friend class Accessible;
};

#endif
