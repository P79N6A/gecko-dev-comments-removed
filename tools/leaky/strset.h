




































#ifndef __strset_h_
#define __strset_h_

struct StrSet {
  StrSet();

  void add(const char* string);
  int contains(const char* string);
  bool IsEmpty() const { return 0 == numstrings; }

  char** strings;
  int numstrings;
};

#endif 
