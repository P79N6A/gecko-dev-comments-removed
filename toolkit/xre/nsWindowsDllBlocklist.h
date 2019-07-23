





































#ifndef IN_WINDOWS_DLL_BLOCKLIST
#error This file should only be included by nsWindowsDllBlocklist.cpp
#endif

#define ALL_VERSIONS   ((unsigned long long)-1LL)



#define MAKE_VERSION(a,b,c,d)\
  ((a##ULL << 48) + (b##ULL << 32) + (c##ULL << 16) + d##ULL)

struct DllBlockInfo {
  
  
  const char *name;

  
  
  
  
  
  
  
  
  unsigned long long maxVersion;
};

static DllBlockInfo sWindowsDllBlocklist[] = {
  
  
  
  
  
  
  { "npffaddon.dll", ALL_VERSIONS},

  
  {"avgrsstx.dll", MAKE_VERSION(8,5,0,401)},
  
  
  {"calc.dll", MAKE_VERSION(1,0,0,1)},

  
  {"hook.dll", ALL_VERSIONS},

  
  { "mozdllblockingtest.dll", ALL_VERSIONS },
  { "mozdllblockingtest_versioned.dll", 0x0000000400000000ULL },

  { NULL, 0 }
};
