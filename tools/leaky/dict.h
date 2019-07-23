



































#ifndef __dict_h_
#define __dict_h_

#include <sys/types.h>
#include "libmalloc.h"



struct MallocDict {
  MallocDict(int buckets);

  void rewind(void);
  malloc_log_entry* next(void);

  malloc_log_entry** find(u_long addr);
  void add(u_long addr, malloc_log_entry *log);
  void remove(u_long addr);

  struct MallocDictEntry {
    u_long addr;
    malloc_log_entry* logEntry;
    MallocDictEntry* next;
  } **buckets;

  int numBuckets;

  int iterNextBucket;
  MallocDictEntry* iterNextEntry;
};

#endif 
