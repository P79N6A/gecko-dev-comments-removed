




































#ifndef nsLayoutStatics_h__
#define nsLayoutStatics_h__

#include "nscore.h"





class nsLayoutStatics
{
public:
  
  
  static nsresult Initialize();

  static void AddRef();
  static void Release();

private:
  
  nsLayoutStatics();

  static void Shutdown();
};

class nsLayoutStaticsRef
{
public:
  nsLayoutStaticsRef()
  {
    nsLayoutStatics::AddRef();
  }
  ~nsLayoutStaticsRef()
  {
    nsLayoutStatics::Release();
  }
};

#endif 
