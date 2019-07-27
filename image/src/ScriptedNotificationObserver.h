





#ifndef ScriptedNotificationObserver_h
#define ScriptedNotificationObserver_h

#include "imgINotificationObserver.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"

class imgIScriptedNotificationObserver;

namespace mozilla {
namespace image {

class ScriptedNotificationObserver : public imgINotificationObserver
{
public:
  explicit ScriptedNotificationObserver(imgIScriptedNotificationObserver* aInner);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER
  NS_DECL_CYCLE_COLLECTION_CLASS(ScriptedNotificationObserver)

private:
  virtual ~ScriptedNotificationObserver() {}
  nsCOMPtr<imgIScriptedNotificationObserver> mInner;
};

}}

#endif
