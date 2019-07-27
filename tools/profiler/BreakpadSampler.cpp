





#include <string>
#include <stdio.h>
#include <errno.h>
#include <ostream>
#include <fstream>
#include <sstream>


#include "PlatformMacros.h"
#include "GeckoProfiler.h"
#include "platform.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "shared-libraries.h"
#include "mozilla/StackWalk.h"
#include "ProfileEntry.h"
#include "SyncProfile.h"
#include "SaveProfileTask.h"
#include "UnwinderThread2.h"
#include "TableTicker.h"


#include "nsXPCOM.h"
#include "nsXPCOMCID.h"
#include "nsIHttpProtocolHandler.h"
#include "nsServiceManagerUtils.h"
#include "nsIXULRuntime.h"
#include "nsIXULAppInfo.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIObserverService.h"
#include "mozilla/Services.h"


#include "jsfriendapi.h"





UnwMode sUnwindMode      = UnwINVALID;
int     sUnwindInterval  = 0;
int     sUnwindStackScan = 0;
int     sProfileEntries  = 0;

using std::string;
using namespace mozilla;

#if _MSC_VER
 #define snprintf _snprintf
#endif







static
void genProfileEntry(UnwinderThreadBuffer* utb,
                     volatile StackEntry &entry,
                     PseudoStack *stack, void *lastpc)
{
  int lineno = -1;

  
  utb__addEntry( utb, ProfileEntry('h', 'P') );
  
  if (entry.isCpp() && entry.stackAddress() != 0) {
    utb__addEntry( utb, ProfileEntry('S', entry.stackAddress()) );
  }

  
  
  const char* sampleLabel = entry.label();
  if (entry.isCopyLabel()) {
    
    

    utb__addEntry( utb, ProfileEntry('c', "") );
    
    size_t strLen = strlen(sampleLabel) + 1;
    for (size_t j = 0; j < strLen;) {
      
      char text[sizeof(void*)];
      for (size_t pos = 0; pos < sizeof(void*) && j+pos < strLen; pos++) {
        text[pos] = sampleLabel[j+pos];
      }
      j += sizeof(void*)/sizeof(char);
      
      utb__addEntry( utb, ProfileEntry('d', *((void**)(&text[0]))) );
    }
    if (entry.isJs()) {
      if (!entry.pc()) {
        
        MOZ_ASSERT(&entry == &stack->mStack[stack->stackSize() - 1]);
        
        if (lastpc) {
          jsbytecode *jspc = js::ProfilingGetPC(stack->mRuntime, entry.script(),
                                                lastpc);
          if (jspc) {
            lineno = JS_PCToLineNumber(entry.script(), jspc);
          }
        }
      } else {
        lineno = JS_PCToLineNumber(entry.script(), entry.pc());
      }
    } else {
      lineno = entry.line();
    }
  } else {
    utb__addEntry( utb, ProfileEntry('c', sampleLabel) );
    if (entry.isCpp()) {
      lineno = entry.line();
    }
  }
  if (lineno != -1) {
    utb__addEntry( utb, ProfileEntry('n', lineno) );
  }

  
  utb__addEntry( utb, ProfileEntry('h', 'Q') );
}




void genPseudoBacktraceEntries(UnwinderThreadBuffer* utb,
                               PseudoStack *aStack, TickSample *sample)
{
  
  
  
  uint32_t nInStack = aStack->stackSize();
  for (uint32_t i = 0; i < nInStack; i++) {
    genProfileEntry(utb, aStack->mStack[i], aStack, nullptr);
  }
# ifdef ENABLE_SPS_LEAF_DATA
  if (sample) {
    utb__addEntry( utb, ProfileEntry('l', (void*)sample->pc) );
#   ifdef ENABLE_ARM_LR_SAVING
    utb__addEntry( utb, ProfileEntry('L', (void*)sample->lr) );
#   endif
  }
# endif
}


static
void populateBuffer(UnwinderThreadBuffer* utb, TickSample* sample,
                    UTB_RELEASE_FUNC releaseFunction, bool jankOnly)
{
  ThreadProfile& sampledThreadProfile = *sample->threadProfile;
  PseudoStack* stack = sampledThreadProfile.GetPseudoStack();
  stack->updateGeneration(sampledThreadProfile.GetGenerationID());

  

  bool recordSample = true;

  

  if (!sample->isSamplingCurrentThread) {
    
    UWTBufferLinkedList* syncBufs = stack->getLinkedUWTBuffers();
    while (syncBufs && syncBufs->peek()) {
      LinkedUWTBuffer* syncBuf = syncBufs->popHead();
      utb__addEntry(utb, ProfileEntry('B', syncBuf->GetBuffer()));
    }
    
    ProfilerMarkerLinkedList* pendingMarkersList = stack->getPendingMarkers();
    while (pendingMarkersList && pendingMarkersList->peek()) {
      ProfilerMarker* marker = pendingMarkersList->popHead();
      stack->addStoredMarker(marker);
      utb__addEntry( utb, ProfileEntry('m', marker) );
    }
    if (jankOnly) {
      
      
      if (sLastSampledEventGeneration != sCurrentEventGeneration) {
        
        
        
        sampledThreadProfile.erase();
      }
      sLastSampledEventGeneration = sCurrentEventGeneration;

      recordSample = false;
      
      
      if (!sLastTracerEvent.IsNull()) {
        mozilla::TimeDuration delta = sample->timestamp - sLastTracerEvent;
        if (delta.ToMilliseconds() > 100.0) {
            recordSample = true;
        }
      }
    }
  }

  
  
  
  
  
  switch (sUnwindMode) {
    case UnwNATIVE: 
      
      
      
      utb__addEntry( utb, ProfileEntry('h', 'N') );
      break;
    case UnwPSEUDO: 
      
      genPseudoBacktraceEntries(utb, stack, sample);
      break;
    case UnwCOMBINED: 
      utb__addEntry( utb, ProfileEntry('h', 'N') );
      genPseudoBacktraceEntries(utb, stack, sample);
      break;
    case UnwINVALID:
    default:
      MOZ_CRASH();
  }

  if (recordSample) {
    
    utb__addEntry( utb, ProfileEntry('h', 'F') );
  }

  
  if (!sLastTracerEvent.IsNull() && sample) {
    mozilla::TimeDuration delta = sample->timestamp - sLastTracerEvent;
    utb__addEntry( utb, ProfileEntry('r', static_cast<float>(delta.ToMilliseconds())) );
  }

  if (sample) {
    mozilla::TimeDuration delta = sample->timestamp - sStartTime;
    utb__addEntry( utb, ProfileEntry('t', static_cast<float>(delta.ToMilliseconds())) );
  }

  if (sLastFrameNumber != sFrameNumber) {
    utb__addEntry( utb, ProfileEntry('f', sFrameNumber) );
    sLastFrameNumber = sFrameNumber;
  }

  







  



  


  if (sUnwindMode == UnwNATIVE || sUnwindMode == UnwCOMBINED) {
#   if defined(SPS_PLAT_amd64_linux) || defined(SPS_PLAT_arm_android) \
       || defined(SPS_PLAT_x86_linux) || defined(SPS_PLAT_x86_android)
    void* ucV = (void*)sample->context;
#   elif defined(SPS_PLAT_amd64_darwin)
    struct __darwin_mcontext64 mc;
    memset(&mc, 0, sizeof(mc));
    ucontext_t uc;
    memset(&uc, 0, sizeof(uc));
    uc.uc_mcontext = &mc;
    mc.__ss.__rip = (uint64_t)sample->pc;
    mc.__ss.__rsp = (uint64_t)sample->sp;
    mc.__ss.__rbp = (uint64_t)sample->fp;
    void* ucV = (void*)&uc;
#   elif defined(SPS_PLAT_x86_darwin)
    struct __darwin_mcontext32 mc;
    memset(&mc, 0, sizeof(mc));
    ucontext_t uc;
    memset(&uc, 0, sizeof(uc));
    uc.uc_mcontext = &mc;
    mc.__ss.__eip = (uint32_t)sample->pc;
    mc.__ss.__esp = (uint32_t)sample->sp;
    mc.__ss.__ebp = (uint32_t)sample->fp;
    void* ucV = (void*)&uc;
#   elif defined(SPS_OS_windows)
    

    void* ucV = nullptr;
#   else
#     error "Unsupported platform"
#   endif
    releaseFunction(&sampledThreadProfile, utb, ucV);
  } else {
    releaseFunction(&sampledThreadProfile, utb, nullptr);
  }
}

static
void sampleCurrent(TickSample* sample)
{
  
  MOZ_ASSERT(sample->threadProfile);
  LinkedUWTBuffer* syncBuf = utb__acquire_sync_buffer(tlsStackTop.get());
  if (!syncBuf) {
    return;
  }
  SyncProfile* syncProfile = sample->threadProfile->AsSyncProfile();
  MOZ_ASSERT(syncProfile);
  if (!syncProfile->SetUWTBuffer(syncBuf)) {
    utb__release_sync_buffer(syncBuf);
    return;
  }
  UnwinderThreadBuffer* utb = syncBuf->GetBuffer();
  populateBuffer(utb, sample, &utb__finish_sync_buffer, false);
}


void TableTicker::UnwinderTick(TickSample* sample)
{
  if (sample->isSamplingCurrentThread) {
    sampleCurrent(sample);
    return;
  }

  if (!sample->threadProfile) {
    
    sample->threadProfile = GetPrimaryThreadProfile();
  }

  

  UnwinderThreadBuffer* utb = uwt__acquire_empty_buffer();

  


  if (!utb)
    return;

  populateBuffer(utb, sample, &uwt__release_full_buffer, mJankOnly);
}




