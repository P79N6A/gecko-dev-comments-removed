





#include "mozilla/ipc/IOThreadChild.h"
#include "mozilla/plugins/PluginProcessChild.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"
#include "nsDebugImpl.h"

#if defined(XP_MACOSX)
#include "nsCocoaFeatures.h"


extern "C" CGError CGSSetDebugOptions(int options);
#endif

#ifdef XP_WIN
#include <objbase.h>
bool ShouldProtectPluginCurrentDirectory(char16ptr_t pluginFilePath);
#if defined(MOZ_SANDBOX)
#define TARGET_SANDBOX_EXPORTS
#include "mozilla/sandboxTarget.h"
#endif
#endif

using mozilla::ipc::IOThreadChild;

#ifdef OS_WIN
#include "nsSetDllDirectory.h"
#include <algorithm>

namespace {

std::size_t caseInsensitiveFind(std::string aHaystack, std::string aNeedle) {
    std::transform(aHaystack.begin(), aHaystack.end(), aHaystack.begin(), ::tolower);
    std::transform(aNeedle.begin(), aNeedle.end(), aNeedle.begin(), ::tolower);
    return aHaystack.find(aNeedle);
}

}
#endif

namespace mozilla {
namespace plugins {


bool
PluginProcessChild::Init()
{
    nsDebugImpl::SetMultiprocessMode("NPAPI");

#if defined(XP_MACOSX)
    
    
    
    
    
    
    nsCString interpose(PR_GetEnv("DYLD_INSERT_LIBRARIES"));
    if (!interpose.IsEmpty()) {
        
        
        
        int32_t lastSeparatorPos = interpose.RFind(":");
        int32_t lastTriggerPos = interpose.RFind("libplugin_child_interpose.dylib");
        bool needsReset = false;
        if (lastTriggerPos != -1) {
            if (lastSeparatorPos == -1) {
                interpose.Truncate();
                needsReset = true;
            } else if (lastTriggerPos > lastSeparatorPos) {
                interpose.SetLength(lastSeparatorPos);
                needsReset = true;
            }
        }
        if (needsReset) {
            nsCString setInterpose("DYLD_INSERT_LIBRARIES=");
            if (!interpose.IsEmpty()) {
                setInterpose.Append(interpose);
            }
            
            char* setInterposePtr = strdup(setInterpose.get());
            PR_SetEnv(setInterposePtr);
        }
    }
#endif

#ifdef XP_WIN
    
    
    ::OleInitialize(nullptr);
#endif

    
    
    message_loop()->set_exception_restoration(true);

    std::string pluginFilename;

#if defined(OS_POSIX)
    
    
    
    std::vector<std::string> values = CommandLine::ForCurrentProcess()->argv();
    NS_ABORT_IF_FALSE(values.size() >= 2, "not enough args");

    pluginFilename = UnmungePluginDsoPath(values[1]);

#elif defined(OS_WIN)
    std::vector<std::wstring> values =
        CommandLine::ForCurrentProcess()->GetLooseValues();
    NS_ABORT_IF_FALSE(values.size() >= 1, "not enough loose args");

    if (ShouldProtectPluginCurrentDirectory(values[0].c_str())) {
        SanitizeEnvironmentVariables();
        SetDllDirectory(L"");
    }

    pluginFilename = WideToUTF8(values[0]);

#if defined(MOZ_SANDBOX)
    
    
    
    mozilla::SandboxTarget::Instance()->StartSandbox();
#endif
#else
#  error Sorry
#endif

    if (NS_FAILED(nsRegion::InitStatic())) {
      NS_ERROR("Could not initialize nsRegion");
      return false;
    }

    bool retval = mPlugin.InitForChrome(pluginFilename, ParentHandle(),
                                        IOThreadChild::message_loop(),
                                        IOThreadChild::channel());
#if defined(XP_MACOSX)
    if (nsCocoaFeatures::OnYosemiteOrLater()) {
      
      
      
      
      
      
      CGSSetDebugOptions(0x80000007);
    }
#endif
    return retval;
}

void
PluginProcessChild::CleanUp()
{
#ifdef XP_WIN
    ::OleUninitialize();
#endif
    nsRegion::ShutdownStatic();
}

} 
} 
