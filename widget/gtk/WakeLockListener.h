






#include <unistd.h>

#ifndef __WakeLockListener_h__
#define __WakeLockListener_h__

#include "nsHashKeys.h"
#include "nsClassHashtable.h"

#include "nsIDOMWakeLockListener.h"

struct DBusConnection;
class WakeLockTopic;





class WakeLockListener final : public nsIDOMMozWakeLockListener
{
public:
  NS_DECL_ISUPPORTS;

  static WakeLockListener* GetSingleton(bool aCreate = true);
  static void Shutdown();

  virtual nsresult Callback(const nsAString& topic,
                            const nsAString& state) override;

private:
  WakeLockListener();
  ~WakeLockListener();

  static WakeLockListener* sSingleton;

  DBusConnection* mConnection;
  
  
  nsClassHashtable<nsStringHashKey, WakeLockTopic> mTopics;
};

#endif 
