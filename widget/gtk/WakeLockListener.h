






#include <unistd.h>

#ifndef __WakeLockListener_h__
#define __WakeLockListener_h__

#include "nsHashKeys.h"
#include "nsClassHashtable.h"

#include "nsIDOMWakeLockListener.h"

struct DBusConnection;
class WakeLockTopic;





class WakeLockListener MOZ_FINAL : public nsIDOMMozWakeLockListener
{
public:
  NS_DECL_ISUPPORTS;

  static WakeLockListener* GetSingleton(bool aCreate = true);
  static void Shutdown();

  nsresult Callback(const nsAString& topic, const nsAString& state);

private:
  WakeLockListener();
  ~WakeLockListener();

  static WakeLockListener* sSingleton;

  DBusConnection* mConnection;
  
  
  nsClassHashtable<nsStringHashKey, WakeLockTopic> mTopics;
};

#endif 
