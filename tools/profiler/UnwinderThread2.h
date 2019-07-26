




#ifndef MOZ_UNWINDER_THREAD_2_H
#define MOZ_UNWINDER_THREAD_2_H

#include "GeckoProfilerImpl.h"
#include "ProfileEntry.h"





typedef
  struct _UnwinderThreadBuffer 
  UnwinderThreadBuffer;





void utb__addEntry(UnwinderThreadBuffer* utb,
                   ProfileEntry ent);


void uwt__init();


void uwt__deinit();




void uwt__register_thread_for_profiling(void* stackTop);








UnwinderThreadBuffer* uwt__acquire_empty_buffer();










void uwt__release_full_buffer(ThreadProfile* aProfile,
                              UnwinderThreadBuffer* utb,
                              void*  ucV);

#endif 
