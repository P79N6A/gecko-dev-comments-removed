






































#ifndef mozilla_storage_VacuumManager_h__
#define mozilla_storage_VacuumManager_h__

#include "nsCOMPtr.h"
#include "nsIObserver.h"
#include "mozIStorageStatementCallback.h"
#include "mozIStorageVacuumParticipant.h"
#include "nsCategoryCache.h"

namespace mozilla {
namespace storage {

class VacuumManager : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  VacuumManager();

  


  static VacuumManager * getSingleton();

private:
  ~VacuumManager();

  static VacuumManager *gVacuumManager;

  
  nsCategoryCache<mozIStorageVacuumParticipant> mParticipants;
};

} 
} 

#endif
