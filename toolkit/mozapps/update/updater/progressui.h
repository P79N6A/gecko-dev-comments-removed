





































#ifndef PROGRESSUI_H__
#define PROGRESSUI_H__

#include "updatedefines.h"

#if defined(XP_WIN)
  typedef WCHAR NS_tchar;
  #define NS_main wmain
#else
  typedef char NS_tchar;
  #define NS_main main
#endif


int InitProgressUI(int *argc, NS_tchar ***argv);

#if defined(XP_WIN)
  
  int ShowProgressUI(bool indeterminate = false, bool initUIStrings = true);
  int InitProgressUIStrings();
#else
  
  int ShowProgressUI();
#endif

void QuitProgressUI();


void UpdateProgressUI(float progress);

#endif  
