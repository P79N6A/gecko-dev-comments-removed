#ifndef __WCHARHXX__
#define __WCHARHXX__

#ifndef GCC
typedef struct {
#else
typedef struct __attribute__ ((packed)) {
#endif
    unsigned char l;
    unsigned char h;
} w_char;


struct replentry {
  char * pattern;
  char * pattern2;
  bool start;
  bool end;
};

#endif
