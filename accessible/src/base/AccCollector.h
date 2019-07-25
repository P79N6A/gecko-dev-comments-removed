




































#ifndef AccCollector_h_
#define AccCollector_h_

#include "filters.h"

#include "nscore.h"
#include "nsTArray.h"





class AccCollector
{
public:
  AccCollector(nsAccessible* aRoot, filters::FilterFuncPtr aFilterFunc);
  virtual ~AccCollector();

  


  PRUint32 Count();

  


  nsAccessible* GetAccessibleAt(PRUint32 aIndex);

  


  virtual PRInt32 GetIndexAt(nsAccessible* aAccessible);

protected:
  


  nsAccessible* EnsureNGetObject(PRUint32 aIndex);

  


  PRInt32 EnsureNGetIndex(nsAccessible* aAccessible);

  


  virtual void AppendObject(nsAccessible* aAccessible);

  filters::FilterFuncPtr mFilterFunc;
  nsAccessible* mRoot;
  PRInt32 mRootChildIdx;

  nsTArray<nsAccessible*> mObjects;

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
  virtual PRInt32 GetIndexAt(nsAccessible* aAccessible);

protected:
  
  EmbeddedObjCollector(nsAccessible* aRoot) :
    AccCollector(aRoot, filters::GetEmbeddedObject) { }

  virtual void AppendObject(nsAccessible* aAccessible);

  friend class nsAccessible;
};

#endif
