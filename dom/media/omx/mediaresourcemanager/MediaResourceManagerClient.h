





#ifndef ANDROID_MEDIARESOURCEMANAGERCLIENT_H
#define ANDROID_MEDIARESOURCEMANAGERCLIENT_H

#include "IMediaResourceManagerClient.h"
#include "IMediaResourceManagerDeathNotifier.h"

namespace android {

class MediaResourceManagerClient: public BnMediaResourceManagerClient,
                                  public virtual IMediaResourceManagerDeathNotifier
{
public:
  
  enum State {
    CLIENT_STATE_WAIT_FOR_RESOURCE,
    CLIENT_STATE_RESOURCE_ASSIGNED,
    CLIENT_STATE_SHUTDOWN
  };

  struct EventListener : public virtual RefBase {
    
    virtual void statusChanged(int event) = 0;
  };

  MediaResourceManagerClient(const wp<EventListener>& listener);

  
  void            died();

  
  virtual void statusChanged(int event);

private:
  wp<EventListener> mEventListener;
};

}; 

#endif
