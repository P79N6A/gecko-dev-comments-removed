





#ifndef __SYNCPROFILE_H
#define __SYNCPROFILE_H

#include "ProfileEntry.h"

class SyncProfile : public ThreadProfile
{
public:
  SyncProfile(ThreadInfo* aInfo, int aEntrySize);
  ~SyncProfile();

  virtual void EndUnwind();
  virtual SyncProfile* AsSyncProfile() { return this; }

private:
  friend class ProfilerBacktrace;

  enum OwnerState
  {
    REFERENCED,       
    OWNED,            
    OWNER_DESTROYING, 
    ORPHANED          
  };

  bool ShouldDestroy();

  OwnerState mOwnerState;
};

#endif 

