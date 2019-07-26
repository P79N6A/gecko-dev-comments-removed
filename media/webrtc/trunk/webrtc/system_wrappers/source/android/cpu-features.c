









#include <sys/system_properties.h>
#ifdef __arm__
#include <machine/cpu-features.h>
#endif
#include <pthread.h>
#include "cpu-features.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

static  pthread_once_t     g_once;
static  AndroidCpuFamily   g_cpuFamily;
static  uint64_t           g_cpuFeatures;
static  int                g_cpuCount;

static const int  android_cpufeatures_debug = 0;

#ifdef __arm__
#  define DEFAULT_CPU_FAMILY  ANDROID_CPU_FAMILY_ARM
#elif defined __i386__
#  define DEFAULT_CPU_FAMILY  ANDROID_CPU_FAMILY_X86
#else
#  define DEFAULT_CPU_FAMILY  ANDROID_CPU_FAMILY_UNKNOWN
#endif

#define  D(...) \
    do { \
        if (android_cpufeatures_debug) { \
            printf(__VA_ARGS__); fflush(stdout); \
        } \
    } while (0)

#ifdef __i386__
static __inline__ void x86_cpuid(int func, int values[4])
{
    int a, b, c, d;
    
    
    __asm__ __volatile__ ( \
      "push %%ebx\n"
      "cpuid\n" \
      "mov %1, %%ebx\n"
      "pop %%ebx\n"
      : "=a" (a), "=r" (b), "=c" (c), "=d" (d) \
      : "a" (func) \
    );
    values[0] = a;
    values[1] = b;
    values[2] = c;
    values[3] = d;
}
#endif






static int
read_file(const char*  pathname, char*  buffer, size_t  buffsize)
{
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







static char*
extract_cpuinfo_field(char* buffer, int buflen, const char* field)
{
    int  fieldlen = strlen(field);
    char* bufend = buffer + buflen;
    char* result = NULL;
    int len, ignore;
    const char *p, *q;

    

    p = buffer;
    bufend = buffer + buflen;
    for (;;) {
        p = memmem(p, bufend-p, field, fieldlen);
        if (p == NULL)
            goto EXIT;

        if (p == buffer || p[-1] == '\n')
            break;

        p += fieldlen;
    }

    
    p += fieldlen;
    p  = memchr(p, ':', bufend-p);
    if (p == NULL || p[1] != ' ')
        goto EXIT;

    
    p += 2;
    q = memchr(p, '\n', bufend-p);
    if (q == NULL)
        q = bufend;

    
    len = q-p;
    result = malloc(len+1);
    if (result == NULL)
        goto EXIT;

    memcpy(result, p, len);
    result[len] = '\0';

EXIT:
    return result;
}



static int
count_cpuinfo_field(char* buffer, int buflen, const char* field)
{
    int fieldlen = strlen(field);
    const char* p = buffer;
    const char* bufend = buffer + buflen;
    const char* q;
    int count = 0;

    for (;;) {
        const char* q;

        p = memmem(p, bufend-p, field, fieldlen);
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





static int
has_list_item(const char* list, const char* item)
{
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

        if (itemlen == q-p && !memcmp(p, item, itemlen))
            return 1;

        
        p = q;
    }
    return 0;
}


static void
android_cpuInit(void)
{
    char cpuinfo[4096];
    int  cpuinfo_len;

    g_cpuFamily   = DEFAULT_CPU_FAMILY;
    g_cpuFeatures = 0;
    g_cpuCount    = 1;

    cpuinfo_len = read_file("/proc/cpuinfo", cpuinfo, sizeof cpuinfo);
    D("cpuinfo_len is (%d):\n%.*s\n", cpuinfo_len,
      cpuinfo_len >= 0 ? cpuinfo_len : 0, cpuinfo);

    if (cpuinfo_len < 0)   {
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

#ifdef __ARM_ARCH__
    {
        char*  features = NULL;
        char*  architecture = NULL;

        







        char* cpuArch = extract_cpuinfo_field(cpuinfo, cpuinfo_len, "CPU architecture");

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
                g_cpuFeatures |= ANDROID_CPU_ARM_FEATURE_ARMv7;
            }

            
            if (archNumber >= 6) {
                g_cpuFeatures |= ANDROID_CPU_ARM_FEATURE_LDREX_STREX;
            }

            free(cpuArch);
        }

        
        char* cpuFeatures = extract_cpuinfo_field(cpuinfo, cpuinfo_len, "Features");

        if (cpuFeatures != NULL) {

            D("found cpuFeatures = '%s'\n", cpuFeatures);

            if (has_list_item(cpuFeatures, "vfpv3"))
                g_cpuFeatures |= ANDROID_CPU_ARM_FEATURE_VFPv3;

            else if (has_list_item(cpuFeatures, "vfpv3d16"))
                g_cpuFeatures |= ANDROID_CPU_ARM_FEATURE_VFPv3;

            if (has_list_item(cpuFeatures, "neon")) {
                




                g_cpuFeatures |= ANDROID_CPU_ARM_FEATURE_NEON |
                                 ANDROID_CPU_ARM_FEATURE_VFPv3;
            }
            free(cpuFeatures);
        }
    }
#endif 

#ifdef __i386__
    g_cpuFamily = ANDROID_CPU_FAMILY_X86;

    int regs[4];


#define VENDOR_INTEL_b  0x756e6547
#define VENDOR_INTEL_c  0x6c65746e
#define VENDOR_INTEL_d  0x49656e69

    x86_cpuid(0, regs);
    int vendorIsIntel = (regs[1] == VENDOR_INTEL_b &&
                         regs[2] == VENDOR_INTEL_c &&
                         regs[3] == VENDOR_INTEL_d);

    x86_cpuid(1, regs);
    if ((regs[2] & (1 << 9)) != 0) {
        g_cpuFeatures |= ANDROID_CPU_X86_FEATURE_SSSE3;
    }
    if ((regs[2] & (1 << 23)) != 0) {
        g_cpuFeatures |= ANDROID_CPU_X86_FEATURE_POPCNT;
    }
    if (vendorIsIntel && (regs[2] & (1 << 22)) != 0) {
        g_cpuFeatures |= ANDROID_CPU_X86_FEATURE_MOVBE;
    }
#endif
}


AndroidCpuFamily
android_getCpuFamily(void)
{
    pthread_once(&g_once, android_cpuInit);
    return g_cpuFamily;
}


uint64_t
android_getCpuFeatures(void)
{
    pthread_once(&g_once, android_cpuInit);
    return g_cpuFeatures;
}


int
android_getCpuCount(void)
{
    pthread_once(&g_once, android_cpuInit);
    return g_cpuCount;
}
