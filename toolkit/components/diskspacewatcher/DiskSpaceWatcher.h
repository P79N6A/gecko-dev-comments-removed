



#ifndef __DISKSPACEWATCHER_H__

#include "nsIDiskSpaceWatcher.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"

class DiskSpaceWatcher MOZ_FINAL : public nsIDiskSpaceWatcher,
                                   public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDISKSPACEWATCHER
  NS_DECL_NSIOBSERVER

  static already_AddRefed<DiskSpaceWatcher>
  FactoryCreate();

  static void UpdateState(bool aIsDiskFull, uint64_t aFreeSpace);

private:
  DiskSpaceWatcher();
  ~DiskSpaceWatcher();

  static uint64_t sFreeSpace;
  static bool     sIsDiskFull;
};

#endif 
