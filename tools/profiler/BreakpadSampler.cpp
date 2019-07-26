





#include <string>
#include <stdio.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#if defined(ANDROID)
# include "android-signal-defs.h"
#endif


#include "PlatformMacros.h"
#include "GeckoProfilerImpl.h"
#include "platform.h"
#include "nsXULAppAPI.h"
#include "nsThreadUtils.h"
#include "prenv.h"
#include "shared-libraries.h"
#include "mozilla/StackWalk.h"
#include "ProfileEntry.h"
#include "SaveProfileTask.h"
#include "UnwinderThread2.h"
#include "TableTicker.h"


#include "JSObjectBuilder.h"
#include "nsIJSRuntimeService.h"


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


#include "jsdbgapi.h"





UnwMode sUnwindMode     = UnwINVALID;
int     sUnwindInterval = 0;

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
  
  if (entry.stackAddress() != 0) {
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
    if (entry.js()) {
      if (!entry.pc()) {
        
        MOZ_ASSERT(&entry == &stack->mStack[stack->stackSize() - 1]);
        
        if (lastpc) {
          jsbytecode *jspc = js::ProfilingGetPC(stack->mRuntime, entry.script(),
                                                lastpc);
          if (jspc) {
            lineno = JS_PCToLineNumber(NULL, entry.script(), jspc);
          }
        }
      } else {
        lineno = JS_PCToLineNumber(NULL, entry.script(), entry.pc());
      }
    } else {
      lineno = entry.line();
    }
  } else {
    utb__addEntry( utb, ProfileEntry('c', sampleLabel) );
    lineno = entry.line();
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


void BreakpadSampler::Tick(TickSample* sample)
{
  if (!sample->threadProfile) {
    
    sample->threadProfile = GetPrimaryThreadProfile();
  }

  ThreadProfile& currThreadProfile = *sample->threadProfile;

  

  UnwinderThreadBuffer* utb = uwt__acquire_empty_buffer();

  


  if (!utb)
    return;

  


  
  PseudoStack* stack = currThreadProfile.GetPseudoStack();
  for (int i = 0; stack->getMarker(i) != NULL; i++) {
    utb__addEntry( utb, ProfileEntry('m', stack->getMarker(i)) );
  }
  stack->mQueueClearMarker = true;

  bool recordSample = true;
  if (mJankOnly) {
    
    
    if (sLastSampledEventGeneration != sCurrentEventGeneration) {
      
      
      
      currThreadProfile.erase();
    }
    sLastSampledEventGeneration = sCurrentEventGeneration;

    recordSample = false;
    
    
    if (!sLastTracerEvent.IsNull()) {
      TimeDuration delta = sample->timestamp - sLastTracerEvent;
      if (delta.ToMilliseconds() > 100.0) {
          recordSample = true;
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
    TimeDuration delta = sample->timestamp - sLastTracerEvent;
    utb__addEntry( utb, ProfileEntry('r', delta.ToMilliseconds()) );
  }

  if (sample) {
    TimeDuration delta = sample->timestamp - mStartTime;
    utb__addEntry( utb, ProfileEntry('t', delta.ToMilliseconds()) );
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
    

    void* ucV = NULL;
#   else
#     error "Unsupported platform"
#   endif
    uwt__release_full_buffer(&currThreadProfile, utb, ucV);
  } else {
    uwt__release_full_buffer(&currThreadProfile, utb, NULL);
  }
}




