



#include "chrome/common/chrome_plugin_lib.h"

#include "base/command_line.h"
#include "base/hash_tables.h"
#include "base/histogram.h"
#include "base/message_loop.h"
#include "base/path_service.h"
#include "base/perftimer.h"
#include "base/thread.h"
#if defined(OS_WIN)
#include "base/registry.h"
#endif
#include "base/string_util.h"
#include "chrome/common/chrome_counters.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/common/notification_service.h"
#include "chrome/common/chrome_paths.h"
#include "webkit/glue/plugins/plugin_list.h"

using base::TimeDelta;


#if defined(OS_WIN)
const TCHAR ChromePluginLib::kRegistryChromePlugins[] =
    _T("Software\\Google\\Chrome\\Plugins");
static const TCHAR kRegistryLoadOnStartup[] = _T("LoadOnStartup");
static const TCHAR kRegistryPath[] = _T("Path");
#endif

typedef base::hash_map<FilePath, scoped_refptr<ChromePluginLib> >
    PluginMap;


static PluginMap* g_loaded_libs;



static PlatformThreadId g_plugin_thread_id = 0;
static MessageLoop* g_plugin_thread_loop = NULL;

#ifdef GEARS_STATIC_LIB

CPError STDCALL Gears_CP_Initialize(CPID id, const CPBrowserFuncs *bfuncs,
                                    CPPluginFuncs *pfuncs);
#endif

static bool IsSingleProcessMode() {
  
  return CommandLine::ForCurrentProcess()->HasSwitch(switches::kSingleProcess);
}


bool ChromePluginLib::IsInitialized() {
  return (g_loaded_libs != NULL);
}


ChromePluginLib* ChromePluginLib::Create(const FilePath& filename,
                                         const CPBrowserFuncs* bfuncs) {
  
  if (!g_loaded_libs) {
    g_loaded_libs = new PluginMap();
    g_plugin_thread_id = PlatformThread::CurrentId();
    g_plugin_thread_loop = MessageLoop::current();
  }
  DCHECK(IsPluginThread());

  PluginMap::const_iterator iter = g_loaded_libs->find(filename);
  if (iter != g_loaded_libs->end())
    return iter->second;

  scoped_refptr<ChromePluginLib> plugin(new ChromePluginLib(filename));
  if (!plugin->CP_Initialize(bfuncs))
    return NULL;

  (*g_loaded_libs)[filename] = plugin;
  return plugin;
}


ChromePluginLib* ChromePluginLib::Find(const FilePath& filename) {
  if (g_loaded_libs) {
    PluginMap::const_iterator iter = g_loaded_libs->find(filename);
    if (iter != g_loaded_libs->end())
      return iter->second;
  }
  return NULL;
}


void ChromePluginLib::Destroy(const FilePath& filename) {
  DCHECK(g_loaded_libs);
  PluginMap::iterator iter = g_loaded_libs->find(filename);
  if (iter != g_loaded_libs->end()) {
    iter->second->Unload();
    g_loaded_libs->erase(iter);
  }
}


bool ChromePluginLib::IsPluginThread() {
  return PlatformThread::CurrentId() == g_plugin_thread_id;
}


MessageLoop* ChromePluginLib::GetPluginThreadLoop() {
  return g_plugin_thread_loop;
}


void ChromePluginLib::RegisterPluginsWithNPAPI() {
  
  if (IsSingleProcessMode())
    return;

  FilePath path;
  if (!PathService::Get(chrome::FILE_GEARS_PLUGIN, &path))
    return;
  
  
  NPAPI::PluginList::AddExtraPluginPath(path);
}

static void LogPluginLoadTime(const TimeDelta &time) {
  UMA_HISTOGRAM_TIMES("Gears.LoadTime", time);
}


void ChromePluginLib::LoadChromePlugins(const CPBrowserFuncs* bfuncs) {
  static bool loaded = false;
  if (loaded)
    return;
  loaded = true;

  
  if (IsSingleProcessMode())
    return;

  FilePath path;
  if (!PathService::Get(chrome::FILE_GEARS_PLUGIN, &path))
    return;

  PerfTimer timer;
  ChromePluginLib::Create(path, bfuncs);
  LogPluginLoadTime(timer.Elapsed());

  
  
#if 0
  for (RegistryKeyIterator iter(HKEY_CURRENT_USER, kRegistryChromePlugins);
       iter.Valid(); ++iter) {
    
    std::wstring reg_path = kRegistryChromePlugins;
    reg_path.append(L"\\");
    reg_path.append(iter.Name());
    RegKey key(HKEY_CURRENT_USER, reg_path.c_str());

    DWORD is_persistent;
    if (key.ReadValueDW(kRegistryLoadOnStartup, &is_persistent) &&
        is_persistent) {
      std::wstring path;
      if (key.ReadValue(kRegistryPath, &path)) {
        ChromePluginLib::Create(path, bfuncs);
      }
    }
  }
#endif
}


void ChromePluginLib::UnloadAllPlugins() {
  if (g_loaded_libs) {
    PluginMap::iterator it;
    for (PluginMap::iterator it = g_loaded_libs->begin();
         it != g_loaded_libs->end(); ++it) {
      it->second->Unload();
    }
    delete g_loaded_libs;
    g_loaded_libs = NULL;
  }
}

const CPPluginFuncs& ChromePluginLib::functions() const {
  DCHECK(initialized_);
  DCHECK(IsPluginThread());
  return plugin_funcs_;
}

ChromePluginLib::ChromePluginLib(const FilePath& filename)
    : filename_(filename),
#if defined(OS_WIN)
      module_(0),
#endif
      initialized_(false),
      CP_VersionNegotiate_(NULL),
      CP_Initialize_(NULL) {
  memset((void*)&plugin_funcs_, 0, sizeof(plugin_funcs_));
}

ChromePluginLib::~ChromePluginLib() {
}

bool ChromePluginLib::CP_Initialize(const CPBrowserFuncs* bfuncs) {
  LOG(INFO) << "ChromePluginLib::CP_Initialize(" << filename_.value() <<
               "): initialized=" << initialized_;
  if (initialized_)
    return true;

  if (!Load())
    return false;

  if (CP_VersionNegotiate_) {
    uint16 selected_version = 0;
    CPError rv = CP_VersionNegotiate_(CP_VERSION, CP_VERSION,
                                      &selected_version);
    if ( (rv != CPERR_SUCCESS) || (selected_version != CP_VERSION))
      return false;
  }

  plugin_funcs_.size = sizeof(plugin_funcs_);
  CPError rv = CP_Initialize_(cpid(), bfuncs, &plugin_funcs_);
  initialized_ = (rv == CPERR_SUCCESS) &&
      (CP_GET_MAJOR_VERSION(plugin_funcs_.version) == CP_MAJOR_VERSION) &&
      (CP_GET_MINOR_VERSION(plugin_funcs_.version) <= CP_MINOR_VERSION);
  LOG(INFO) << "ChromePluginLib::CP_Initialize(" << filename_.value() <<
               "): initialized=" << initialized_ <<
               "): result=" << rv;

  return initialized_;
}

void ChromePluginLib::CP_Shutdown() {
  DCHECK(initialized_);
  functions().shutdown();
  initialized_ = false;
  memset((void*)&plugin_funcs_, 0, sizeof(plugin_funcs_));
}

int ChromePluginLib::CP_Test(void* param) {
  DCHECK(initialized_);
  if (!CP_Test_)
    return -1;
  return CP_Test_(param);
}

bool ChromePluginLib::Load() {
#if !defined(OS_WIN)
  
  NOTIMPLEMENTED() << " -- gears loading code.";
  return false;
#else
  DCHECK(module_ == 0);
#ifdef GEARS_STATIC_LIB
  FilePath path;
  if (filename_.BaseName().value().find(FILE_PATH_LITERAL("gears")) == 0) {
    CP_Initialize_ = &Gears_CP_Initialize;
    return true;
  }
#endif

  module_ = LoadLibrary(filename_.value().c_str());
  if (module_ == 0)
    return false;

  
  CP_Initialize_ = reinterpret_cast<CP_InitializeFunc>
      (GetProcAddress(module_, "CP_Initialize"));

  if (!CP_Initialize_) {
    FreeLibrary(module_);
    module_ = 0;
    return false;
  }

  
  CP_VersionNegotiate_ = reinterpret_cast<CP_VersionNegotiateFunc>
      (GetProcAddress(module_, "CP_VersionNegotiate"));

  
  CP_Test_ = reinterpret_cast<CP_TestFunc>
      (GetProcAddress(module_, "CP_Test"));

  return true;
#endif
}

void ChromePluginLib::Unload() {
  NotificationService::current()->Notify(
      NotificationType::CHROME_PLUGIN_UNLOADED,
      Source<ChromePluginLib>(this),
      NotificationService::NoDetails());

  if (initialized_)
    CP_Shutdown();

#if defined(OS_WIN)
  if (module_) {
    FreeLibrary(module_);
    module_ = 0;
  }
#endif
}
