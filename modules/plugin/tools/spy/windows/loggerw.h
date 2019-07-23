




































#ifndef __LOGGRERW_H__
#define __LOGGRERW_H__

#include "logger.h"

class LoggerWin : public Logger
{
public:
  HWND hWnd;
  int width;
  int height;
  int x;
  int y;
  BOOL bSaveSettings;

  LoggerWin();
  ~LoggerWin();

  BOOL platformInit();
  void platformShut();
  void dumpStringToMainWindow(char * string);

  void onDestroyWindow();
  void onClear();
};

#endif
