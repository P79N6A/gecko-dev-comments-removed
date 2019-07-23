





































#ifndef READSTRINGS_H__
#define READSTRINGS_H__

#define MAX_TEXT_LEN 200

struct StringTable {
  char title[MAX_TEXT_LEN];
  char info[MAX_TEXT_LEN];
};




int ReadStrings(const char *path, StringTable *results);

#endif  
