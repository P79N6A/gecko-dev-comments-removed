






#ifndef __nsQtRemoteService_h__
#define __nsQtRemoteService_h__

#include "nsXRemoteService.h"
#include <X11/Xlib.h>

class RemoteEventHandlerWidget;

class nsQtRemoteService : public nsXRemoteService
{
public:
  
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREMOTESERVICE  

  nsQtRemoteService();

private:
  ~nsQtRemoteService() { };

  virtual void SetDesktopStartupIDOrTimestamp(const nsACString& aDesktopStartupID,
                                              uint32_t aTimestamp);

  void PropertyNotifyEvent(XEvent *evt);
  friend class MozQRemoteEventHandlerWidget;

  QWidget *mServerWindow;
};

#endif 
