






#define LOG_TAG "MediaResourceManagerClient"

#include <utils/Log.h>

#include "MediaResourceManagerClient.h"

namespace android {

MediaResourceManagerClient::MediaResourceManagerClient(const wp<EventListener>& listener)
  : mEventListener(listener)
{
}

void MediaResourceManagerClient::statusChanged(int event)
{
  if (mEventListener != NULL) {
    sp<EventListener> listener = mEventListener.promote();
    if (listener != NULL) {
      listener->statusChanged(event);
    }
  }
}

void MediaResourceManagerClient::died()
{
  sp<EventListener> listener = mEventListener.promote();
  if (listener != NULL) {
    listener->statusChanged(CLIENT_STATE_SHUTDOWN);
  }
}

}; 

