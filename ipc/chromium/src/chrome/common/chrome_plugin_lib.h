



#ifndef CHROME_COMMON_CHROME_PLUGIN_LIB_H_
#define CHROME_COMMON_CHROME_PLUGIN_LIB_H_

#include <string>

#include "base/basictypes.h"
#include "base/file_path.h"
#include "base/ref_counted.h"
#include "chrome/common/chrome_plugin_api.h"

class MessageLoop;





class ChromePluginLib : public base::RefCounted<ChromePluginLib>  {
 public:
  static bool IsInitialized();
  static ChromePluginLib* Create(const FilePath& filename,
                                 const CPBrowserFuncs* bfuncs);
  static ChromePluginLib* Find(const FilePath& filename);
  static void Destroy(const FilePath& filename);
  static bool IsPluginThread();
  static MessageLoop* GetPluginThreadLoop();

  static ChromePluginLib* FromCPID(CPID id) {
    return reinterpret_cast<ChromePluginLib*>(id);
  }

  
  static void RegisterPluginsWithNPAPI();

  
  
  static void LoadChromePlugins(const CPBrowserFuncs* bfuncs);

  
  static void UnloadAllPlugins();

  
  const bool is_loaded() const { return initialized_; }

  
  const CPPluginFuncs& functions() const;

  CPID cpid() { return reinterpret_cast<CPID>(this); }

  const FilePath& filename() { return filename_; }

  

  
  int CP_Test(void* param);

#if defined(OS_WIN)
  
  static const TCHAR kRegistryChromePlugins[];
#endif  

 private:
  friend class base::RefCounted<ChromePluginLib>;

  ChromePluginLib(const FilePath& filename);
  ~ChromePluginLib();

  
  
  bool CP_Initialize(const CPBrowserFuncs* bfuncs);

  
  void CP_Shutdown();

  
  
  bool Load();

  
  void Unload();

  FilePath filename_;  
#if defined(OS_WIN)
  
  HMODULE module_;  
#endif  
  bool initialized_;  

  
  CP_VersionNegotiateFunc CP_VersionNegotiate_;
  CP_InitializeFunc CP_Initialize_;

  
  CPPluginFuncs plugin_funcs_;

  
  typedef int (STDCALL *CP_TestFunc)(void*);
  CP_TestFunc CP_Test_;

  DISALLOW_COPY_AND_ASSIGN(ChromePluginLib);
};

#endif  
