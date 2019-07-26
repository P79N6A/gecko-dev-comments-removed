




#ifndef MOZ_UNWINDER_THREAD_2_H
#define MOZ_UNWINDER_THREAD_2_H

#include "GeckoProfilerImpl.h"
#include "ProfileEntry.h"

#include "PlatformMacros.h"
#if defined(SPS_OS_android) || defined(SPS_OS_linux)
# include "LulMain.h"
#endif





typedef
  struct _UnwinderThreadBuffer 
  UnwinderThreadBuffer;





void utb__addEntry(UnwinderThreadBuffer* utb,
                   ProfileEntry ent);


void uwt__init();




void uwt__stop();




void uwt__deinit();





void uwt__register_thread_for_profiling(void* stackTop);


void uwt__unregister_thread_for_profiling();












UnwinderThreadBuffer* uwt__acquire_empty_buffer();










void uwt__release_full_buffer(ThreadProfile* aProfile,
                              UnwinderThreadBuffer* utb,
                              void*  ucV);

struct LinkedUWTBuffer;



LinkedUWTBuffer* utb__acquire_sync_buffer(void* stackTop);

void utb__finish_sync_buffer(ThreadProfile* aProfile,
                             UnwinderThreadBuffer* utb,
                             void*  ucV);



void utb__release_sync_buffer(LinkedUWTBuffer* utb);


void utb__end_sync_buffer_unwind(LinkedUWTBuffer* utb);


typedef void (*UTB_RELEASE_FUNC)(ThreadProfile*,UnwinderThreadBuffer*,void*);

#if defined(SPS_OS_android) || defined(SPS_OS_linux)



void read_procmaps(lul::LUL* aLUL);
#endif

#endif 
