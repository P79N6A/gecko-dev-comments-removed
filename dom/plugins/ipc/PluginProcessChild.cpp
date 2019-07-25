






































#include "mozilla/ipc/IOThreadChild.h"
#include "mozilla/plugins/PluginProcessChild.h"

#include "prlink.h"

#include "base/command_line.h"
#include "base/string_util.h"
#include "chrome/common/chrome_switches.h"

#ifdef XP_WIN
#include <objbase.h>
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
#if defined(XP_MACOSX)
    
    
    
    
    
    
    nsCString interpose(PR_GetEnv("DYLD_INSERT_LIBRARIES"));
    if (!interpose.IsEmpty()) {
        
        
        
        PRInt32 lastSeparatorPos = interpose.RFind(":");
        PRInt32 lastTriggerPos = interpose.RFind("libplugin_child_interpose.dylib");
        PRBool needsReset = PR_FALSE;
        if (lastTriggerPos != -1) {
            if (lastSeparatorPos == -1) {
                interpose.Truncate();
                needsReset = PR_TRUE;
            } else if (lastTriggerPos > lastSeparatorPos) {
                interpose.SetLength(lastSeparatorPos);
                needsReset = PR_TRUE;
            }
        }
        if (needsReset) {
            nsCString setInterpose("DYLD_INSERT_LIBRARIES=");
            if (!interpose.IsEmpty()) {
                setInterpose.Append(interpose);
            }
            
            char* setInterposePtr = strdup(PromiseFlatCString(setInterpose).get());
            PR_SetEnv(setInterposePtr);
        }
    }
#endif

#ifdef XP_WIN
    
    
    ::OleInitialize(NULL);
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

    pluginFilename = WideToUTF8(values[0]);

    bool protectCurrentDirectory = true;
    
    const std::string shockwaveDirectorPluginFilename("\\np32dsw.dll");
    std::size_t index = caseInsensitiveFind(pluginFilename, shockwaveDirectorPluginFilename);
    if (index != std::string::npos &&
        index + shockwaveDirectorPluginFilename.length() == pluginFilename.length()) {
        protectCurrentDirectory = false;
    }
    if (protectCurrentDirectory) {
        NS_SetDllDirectory(L"");
    }

#else
#  error Sorry
#endif

    if (NS_FAILED(nsRegion::InitStatic())) {
      NS_ERROR("Could not initialize nsRegion");
      return false;
    }

    mPlugin.Init(pluginFilename, ParentHandle(),
                 IOThreadChild::message_loop(),
                 IOThreadChild::channel());

    return true;
}

void
PluginProcessChild::CleanUp()
{
#ifdef XP_WIN
    ::OleUninitialize();
#endif
    nsRegion::ShutdownStatic();
}


void
PluginProcessChild::AppendNotesToCrashReport(const nsCString& aNotes)
{
    AssertPluginThread();

    PluginProcessChild* p = PluginProcessChild::current();
    if (p) {
        p->mPlugin.SendAppendNotesToCrashReport(aNotes);
    }
}

} 
} 
