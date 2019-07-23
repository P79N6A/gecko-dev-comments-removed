





































#ifndef IN_WINDOWS_DLL_BLOCKLIST
#error This file should only be included by nsWindowsDllBlocklist.cpp
#endif

#define ALL_VERSIONS   ((unsigned long long)-1LL)

struct DllBlockInfo {
  
  
  const char *name;

  
  
  
  
  
  
  
  
  unsigned long long maxVersion;
};

static DllBlockInfo sWindowsDllBlocklist[] = {
  
  
  
  
  { NULL, 0 }
};
