





#ifndef CHROME_COMMON_GEARS_API_H__
#define CHROME_COMMON_GEARS_API_H__

#include "chrome/common/chrome_plugin_api.h"

#ifdef __cplusplus
extern "C" {
#endif




typedef enum {
  
  
  
  GEARSPLUGINCOMMAND_SHOW_SETTINGS = 0,

  
  
  GEARSPLUGINCOMMAND_CREATE_SHORTCUT = 1,

  
  
  GEARSPLUGINCOMMAND_GET_SHORTCUT_LIST = 2,
} GearsPluginCommand;




typedef enum {
  
  
  
  GEARSBROWSERCOMMAND_CREATE_SHORTCUT_DONE = 1,

  
  
  GEARSBROWSERCOMMAND_NOTIFY_SHORTCUTS_CHANGED = 3,
} GearsBrowserCommand;


typedef struct _GearsShortcutIcon {
  const char* size;  
  const char* url;  
  int width;  
  int height;  
} GearsShortcutIcon;


typedef struct _GearsShortcutData {
  const char* name;  
  const char* url;  
  const char* description;  
  GearsShortcutIcon icons[4];  
} GearsShortcutData;






typedef struct _GearsShortcutData2 {
  const char* name;  
  const char* url;  
  const char* description;  
  GearsShortcutIcon icons[4];  
  const char* orig_name;  
} GearsShortcutData2;


typedef struct _GearsShortcutList {
  
  
  
  GearsShortcutData* shortcuts;  
  uint32 num_shortcuts;  
} GearsShortcutList;


typedef struct _GearsCreateShortcutResult {
  GearsShortcutData2* shortcut;  
                                 
  CPError result;  
} GearsCreateShortcutResult;

#ifdef __cplusplus
} 
#endif

#endif
