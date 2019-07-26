





#ifndef __SYNCPROFILE_H
#define __SYNCPROFILE_H

#include "ProfileEntry.h"

struct LinkedUWTBuffer;

class SyncProfile : public ThreadProfile
{
public:
  SyncProfile(const char* aName, int aEntrySize, PseudoStack *aStack,
              Thread::tid_t aThreadId, bool aIsMainThread);
  ~SyncProfile();

  bool SetUWTBuffer(LinkedUWTBuffer* aBuff);
  LinkedUWTBuffer* GetUWTBuffer() { return mUtb; }

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

  OwnerState        mOwnerState;
  LinkedUWTBuffer*  mUtb;
};

#endif 

