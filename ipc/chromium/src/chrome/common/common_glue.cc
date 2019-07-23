



#include "app/l10n_util.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/string16.h"
#include "base/string_util.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/plugin/npobject_util.h"
#include "googleurl/src/url_util.h"
#include "webkit/glue/webkit_glue.h"

namespace webkit_glue {

bool GetExeDirectory(std::wstring *path) {
  return PathService::Get(base::DIR_EXE, path);
}

bool GetApplicationDirectory(std::wstring *path) {
  return PathService::Get(chrome::DIR_APP, path);
}

bool IsPluginRunningInRendererProcess() {
  return !IsPluginProcess();
}

std::wstring GetWebKitLocale() {
  
  
  
  const CommandLine& parsed_command_line = *CommandLine::ForCurrentProcess();
  const std::wstring& lang =
      parsed_command_line.GetSwitchValue(switches::kLang);
  DCHECK(!lang.empty() ||
      (!parsed_command_line.HasSwitch(switches::kRendererProcess) &&
       !parsed_command_line.HasSwitch(switches::kPluginProcess)));
  return lang;
}

string16 GetLocalizedString(int message_id) {
  return WideToUTF16(l10n_util::GetString(message_id));
}

}  
