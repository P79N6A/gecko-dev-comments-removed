





#include "imgINotificationObserver.h"
#include "nsCOMPtr.h"
#include "nsCycleCollectionParticipant.h"

class imgIScriptedNotificationObserver;

namespace mozilla {
namespace image {

class ScriptedNotificationObserver : public imgINotificationObserver
{
public:
  ScriptedNotificationObserver(imgIScriptedNotificationObserver* aInner);
  virtual ~ScriptedNotificationObserver() {}

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER
  NS_DECL_CYCLE_COLLECTION_CLASS(ScriptedNotificationObserver)

private:
  nsCOMPtr<imgIScriptedNotificationObserver> mInner;
};

}}
