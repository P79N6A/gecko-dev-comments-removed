





































#ifndef PROGRESSUI_H__
#define PROGRESSUI_H__

#if defined(XP_WIN)
  typedef unsigned short NS_tchar;
  #define NS_main wmain
#else
  typedef char NS_tchar;
  #define NS_main main
#endif


int InitProgressUI(int *argc, NS_tchar ***argv);


int ShowProgressUI();


void QuitProgressUI();


void UpdateProgressUI(float progress);

#endif  
