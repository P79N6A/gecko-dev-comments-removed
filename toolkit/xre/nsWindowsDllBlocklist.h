





































#ifndef IN_WINDOWS_DLL_BLOCKLIST
#error This file should only be included by nsWindowsDllBlocklist.cpp
#endif

#define ALL_VERSIONS   ((unsigned long long)-1LL)

struct DllBlockInfo {
  
  
  const char *name;

  
  
  
  
  
  
  
  
  unsigned long long maxVersion;
};

static DllBlockInfo sWindowsDllBlocklist[] = {
  
  
  
  

  
  { "mozdllblockingtest.dll", ALL_VERSIONS },
  { "mozdllblockingtest_versioned.dll", 0x0000000400000000ULL },

  { NULL, 0 }
};
