



#ifndef nsLayoutStatics_h__
#define nsLayoutStatics_h__

#include "nscore.h"
#include "MainThreadUtils.h"
#include "nsTraceRefcnt.h"
#include "nsDebug.h"





class nsLayoutStatics
{
public:
  
  
  static nsresult Initialize();

  static void AddRef()
  {
    NS_ASSERTION(NS_IsMainThread(),
                 "nsLayoutStatics reference counting must be on main thread");

    NS_ASSERTION(sLayoutStaticRefcnt,
                 "nsLayoutStatics already dropped to zero!");

    ++sLayoutStaticRefcnt;
    NS_LOG_ADDREF(&sLayoutStaticRefcnt, sLayoutStaticRefcnt,
                  "nsLayoutStatics", 1);
  }
  static void Release()
  {
    NS_ASSERTION(NS_IsMainThread(),
                 "nsLayoutStatics reference counting must be on main thread");

    --sLayoutStaticRefcnt;
    NS_LOG_RELEASE(&sLayoutStaticRefcnt, sLayoutStaticRefcnt,
                   "nsLayoutStatics");

    if (!sLayoutStaticRefcnt)
      Shutdown();
  }

private:
  
  nsLayoutStatics();

  static void Shutdown();

  static nsrefcnt sLayoutStaticRefcnt;
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
