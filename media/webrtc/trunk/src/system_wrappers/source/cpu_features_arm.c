













#include "cpu_features_wrapper.h"

#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>


typedef enum {
  CPU_FAMILY_UNKNOWN = 0,
  CPU_FAMILY_ARM,
  CPU_FAMILY_X86,
  CPU_FAMILY_MAX  
} CpuFamily;

static pthread_once_t g_once;
static CpuFamily g_cpuFamily;
static uint64_t g_cpuFeatures;
static int g_cpuCount;

static const int cpufeatures_debug = 0;

#ifdef __arm__
#  define DEFAULT_CPU_FAMILY  CPU_FAMILY_ARM
#elif defined __i386__
#  define DEFAULT_CPU_FAMILY  CPU_FAMILY_X86
#else
#  define DEFAULT_CPU_FAMILY  CPU_FAMILY_UNKNOWN
#endif

#define  D(...) \
  do { \
    if (cpufeatures_debug) { \
      printf(__VA_ARGS__); fflush(stdout); \
    } \
  } while (0)






static int read_file(const char*  pathname, char*  buffer, size_t  buffsize) {
  int  fd, len;

  fd = open(pathname, O_RDONLY);
  if (fd < 0)
    return -1;

  do {
    len = read(fd, buffer, buffsize);
  } while (len < 0 && errno == EINTR);

  close(fd);

  return len;
}







static char* extract_cpuinfo_field(char* buffer, int buflen, const char* field) {
  int  fieldlen = strlen(field);
  char* bufend = buffer + buflen;
  char* result = NULL;
  int len, ignore;
  const char* p, *q;

  

  p = buffer;
  bufend = buffer + buflen;
  for (;;) {
    p = memmem(p, bufend - p, field, fieldlen);
    if (p == NULL)
      goto EXIT;

    if (p == buffer || p[-1] == '\n')
      break;

    p += fieldlen;
  }

  
  p += fieldlen;
  p  = memchr(p, ':', bufend - p);
  if (p == NULL || p[1] != ' ')
    goto EXIT;

  
  p += 2;
  q = memchr(p, '\n', bufend - p);
  if (q == NULL)
    q = bufend;

  
  len = q - p;
  result = malloc(len + 1);
  if (result == NULL)
    goto EXIT;

  memcpy(result, p, len);
  result[len] = '\0';

EXIT:
  return result;
}



static int count_cpuinfo_field(char* buffer, int buflen, const char* field) {
  int fieldlen = strlen(field);
  const char* p = buffer;
  const char* bufend = buffer + buflen;
  const char* q;
  int count = 0;

  for (;;) {
    const char* q;

    p = memmem(p, bufend - p, field, fieldlen);
    if (p == NULL)
      break;

    
    if (p > buffer && p[-1] != '\n') {
      p += fieldlen;
      continue;
    }


    
    q = p + fieldlen;
    while (q < bufend && (*q == ' ' || *q == '\t'))
      q++;

    
    if (q < bufend && *q == ':') {
      count += 1;
      q ++;
    }
    p = q;
  }

  return count;
}


#define STRLEN_CONST(x)  ((sizeof(x)-1)





static int has_list_item(const char* list, const char* item) {
  const char*  p = list;
  int itemlen = strlen(item);

  if (list == NULL)
    return 0;

  while (*p) {
    const char*  q;

    
    while (*p == ' ' || *p == '\t')
      p++;

    
    q = p;
    while (*q && *q != ' ' && *q != '\t')
      q++;

    if (itemlen == q - p && !memcmp(p, item, itemlen))
      return 1;

    
    p = q;
  }
  return 0;
}


static void cpuInit(void) {
  char cpuinfo[4096];
  int  cpuinfo_len;

  g_cpuFamily   = DEFAULT_CPU_FAMILY;
  g_cpuFeatures = 0;
  g_cpuCount    = 1;

  cpuinfo_len = read_file("/proc/cpuinfo", cpuinfo, sizeof cpuinfo);
  D("cpuinfo_len is (%d):\n%.*s\n", cpuinfo_len,
    cpuinfo_len >= 0 ? cpuinfo_len : 0, cpuinfo);

  if (cpuinfo_len < 0) { 
    return;
  }

  
  g_cpuCount = count_cpuinfo_field(cpuinfo, cpuinfo_len, "processor");
  if (g_cpuCount == 0) {
    g_cpuCount = count_cpuinfo_field(cpuinfo, cpuinfo_len, "Processor");
    if (g_cpuCount == 0) {
      g_cpuCount = 1;
    }
  }

  D("found cpuCount = %d\n", g_cpuCount);

#ifdef __arm__
  {
    char*  features = NULL;
    char*  architecture = NULL;

    







    char* cpuArch = extract_cpuinfo_field(cpuinfo, cpuinfo_len,
                                          "CPU architecture");

    if (cpuArch != NULL) {
      char*  end;
      long   archNumber;
      int    hasARMv7 = 0;

      D("found cpuArch = '%s'\n", cpuArch);

      
      archNumber = strtol(cpuArch, &end, 10);

      



      if (end > cpuArch && archNumber >= 7) {
        hasARMv7 = 1;
      }

      







      if (hasARMv7) {
        char* cpuProc = extract_cpuinfo_field(cpuinfo, cpuinfo_len,
                                              "Processor");
        if (cpuProc != NULL) {
          D("found cpuProc = '%s'\n", cpuProc);
          if (has_list_item(cpuProc, "(v6l)")) {
            D("CPU processor and architecture mismatch!!\n");
            hasARMv7 = 0;
          }
          free(cpuProc);
        }
      }

      if (hasARMv7) {
        g_cpuFeatures |= kCPUFeatureARMv7;
      }

      
      if (archNumber >= 6) {
        g_cpuFeatures |= kCPUFeatureLDREXSTREX;
      }

      free(cpuArch);
    }

    
    char* cpuFeatures = extract_cpuinfo_field(cpuinfo, cpuinfo_len,
                                              "Features");

    if (cpuFeatures != NULL) {

      D("found cpuFeatures = '%s'\n", cpuFeatures);

      if (has_list_item(cpuFeatures, "vfpv3"))
        g_cpuFeatures |= kCPUFeatureVFPv3;

      else if (has_list_item(cpuFeatures, "vfpv3d16"))
        g_cpuFeatures |= kCPUFeatureVFPv3;

      if (has_list_item(cpuFeatures, "neon")) {
        




        g_cpuFeatures |= kCPUFeatureNEON |
                         kCPUFeatureVFPv3;
      }
      free(cpuFeatures);
    }
  }
#endif  

#ifdef __i386__
  g_cpuFamily = CPU_FAMILY_X86;
#endif
}


uint64_t WebRtc_GetCPUFeaturesARM(void) {
  pthread_once(&g_once, cpuInit);
  return g_cpuFeatures;
}
