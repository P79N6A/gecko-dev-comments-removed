




































#ifndef __PROFILEW_H__
#define __PROFILEW_H__

#include "profile.h"

class ProfileWin : public Profile
{
  HKEY hKey;

public:
  ProfileWin();
  ~ProfileWin();

  BOOL getBool(char * key, BOOL * value);
  BOOL setBool(char * key, BOOL value);

  BOOL getString(char * key, char * string, int size);
  BOOL setString(char * key, char * string);

  BOOL getSizeAndPosition(int *width, int *height, int *x, int *y);
  BOOL setSizeAndPosition(int width, int height, int x, int y);
};

#define NPSPY_REG_SUBKEY "Software\\Netscape\\SpyPlugin"

#define NPSPY_REG_KEY_ONTOP        "AlwaysOnTop"
#define NPSPY_REG_KEY_LOGTOWINDOW  "LogToWindow"
#define NPSPY_REG_KEY_LOGTOCONSOLE "LogToConsole"
#define NPSPY_REG_KEY_LOGTOFILE    "LogToFile"
#define NPSPY_REG_KEY_SPALID       "ShutdownPluginsAfterDestroy"
#define NPSPY_REG_KEY_WIDTH        "width"
#define NPSPY_REG_KEY_HEIGHT       "height"
#define NPSPY_REG_KEY_X            "x"
#define NPSPY_REG_KEY_Y            "y"
#define NPSPY_REG_KEY_LOGFILENAME  "LogFileName"

#endif

