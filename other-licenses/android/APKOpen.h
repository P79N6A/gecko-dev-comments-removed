



































#ifndef APKOpen_h
#define APKOpen_h

struct mapping_info {
  char * name;
  char * file_id;
  uintptr_t base;
  size_t len;
  size_t offset;
};

extern struct mapping_info * lib_mapping;

#endif 
