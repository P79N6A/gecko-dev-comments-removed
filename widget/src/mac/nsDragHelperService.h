




































#ifndef nsDragHelperService_h__
#define nsDragHelperService_h__

#include "nsCOMPtr.h"
#include "nsIDragService.h"

#include <Carbon/Carbon.h>

#include "nsIDragHelperService.h"










#define NS_DRAGHELPERSERVICE_CID      \
{ 0x75993200, 0xf3b2, 0x11d5, { 0xa3, 0x84, 0xc7, 0x05, 0x4d, 0x07, 0xd6, 0xfc } }


class nsDragHelperService : public nsIDragHelperService
{
public:
  nsDragHelperService();
  virtual ~nsDragHelperService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDRAGHELPERSERVICE

protected:

    
  PRBool DoDragAction(PRUint32      aMessage,
                      DragReference aDragRef,
                      nsIEventSink* aSink);

    
    
  void SetDragActionBasedOnModifiers ( short inModifiers ) ;

    
  void SetDragOverTimer(nsIEventSink* aSink, DragRef aDragRef);
  static void DragOverTimerHandler(EventLoopTimerRef aTimer, void* aUserData);
  
    
    
    
    
    
  nsCOMPtr<nsIDragService> mDragService;

    
  EventLoopTimerRef      mDragOverTimer;
  nsCOMPtr<nsIEventSink> mDragOverSink;
  DragRef                mDragOverDragRef;
}; 


#endif 
