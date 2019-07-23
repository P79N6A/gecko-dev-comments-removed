





































#ifndef WatchTask_h__
#define WatchTask_h__


#ifndef XP_MACOSX
#include <Retrace.h>
#endif
#include <Quickdraw.h>
#include "prtypes.h"
#include "gfxCore.h"









class nsWatchTask
{
public:
  nsWatchTask ( ) ;
  ~nsWatchTask ( ) ;

    
    
    
  NS_GFX void Start ( ) ;
  
    
  NS_GFX void EventLoopReached ( ) ;
  
    
    
  void Suspend ( ) { mSuspended = PR_TRUE; };
  void Resume ( ) { mSuspended = PR_FALSE; };
  
  static NS_GFX nsWatchTask& GetTask ( ) ;
  
private:

  enum { 
    kRepeatInterval = 10,       
    kTicksToShowWatch = 45,     
    kStepsInAnimation = 12
  };
  
    
  static pascal void DoWatchTask(nsWatchTask* theTaskPtr) ;
  
#if !TARGET_CARBON
  VBLTask mTask;            
#endif
  long mChecksum;           
  void* mSelf;              
  long mTicks;              
  Cursor mWatchCursor;      
  PRPackedBool mBusy;       
  PRPackedBool mSuspended;  
  PRPackedBool mInstallSucceeded;     
  short mAnimation;         
  
};


#endif
