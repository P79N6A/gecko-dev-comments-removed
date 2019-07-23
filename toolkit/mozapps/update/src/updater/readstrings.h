





































#ifndef READSTRINGS_H__
#define READSTRINGS_H__

#define MAX_TEXT_LEN 200

#if defined(XP_WIN) || defined(XP_OS2)
# include <windows.h>
  typedef WCHAR NS_tchar;
#else
  typedef char NS_tchar;
#endif

struct StringTable {
  char title[MAX_TEXT_LEN];
  char info[MAX_TEXT_LEN];
};




int ReadStrings(const NS_tchar *path, StringTable *results);

#endif  
