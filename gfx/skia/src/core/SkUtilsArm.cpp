







#include "SkUtilsArm.h"

#if SK_ARM_NEON_IS_DYNAMIC

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>



#ifdef SK_DEBUG
#  define NEON_DEBUG  1
#else
#  define NEON_DEBUG 0
#endif

#if NEON_DEBUG
#  ifdef SK_BUILD_FOR_ANDROID
     
#    include <sys/system_properties.h>
#  endif
#endif



static bool sk_cpu_arm_check_neon(void) {
    bool result = false;

#if NEON_DEBUG
    
#  ifdef SK_BUILD_FOR_ANDROID
    
#   define PROP_NAME  "debug.skia.arm_neon_mode"
    char prop[PROP_VALUE_MAX];
    if (__system_property_get(PROP_NAME, prop) > 0) {
#  else
#   define PROP_NAME   "SKIA_ARM_NEON_MODE"
    
    const char* prop = getenv(PROP_NAME);
    if (prop != NULL) {
#  endif
        SkDebugf("%s: %s", PROP_NAME, prop);
        if (!strcmp(prop, "1")) {
            SkDebugf("Forcing ARM Neon mode to full!\n");
            return true;
        }
        if (!strcmp(prop, "0")) {
            SkDebugf("Disabling ARM NEON mode\n");
            return false;
        }
    }
    SkDebugf("Running dynamic CPU feature detection\n");
#endif


















    char   buffer[4096];

    
    
    result = false;

    do {
        
        int fd = TEMP_FAILURE_RETRY(open("/proc/cpuinfo", O_RDONLY));
        if (fd < 0) {
            SkDebugf("Could not open /proc/cpuinfo: %s\n", strerror(errno));
            break;
        }

        
        
        
        buffer[0] = '\n';
        int size = TEMP_FAILURE_RETRY(read(fd, buffer+1, sizeof(buffer)-2));
        close(fd);

        if (size < 0) {  
            SkDebugf("Could not read /proc/cpuinfo: %s\n", strerror(errno));
            break;
        }

        SkDebugf("START /proc/cpuinfo:\n%.*s\nEND /proc/cpuinfo\n",
                 size, buffer+1);

        
        char* buffer_end = buffer + 1 + size;
        buffer_end[0] = '\n';

        
        
        const char features[] = "\nFeatures\t";
        const size_t features_len = sizeof(features)-1;

        char*  line = (char*) memmem(buffer, buffer_end - buffer,
                                     features, features_len);
        if (line == NULL) {  
            SkDebugf("Could not find a line starting with 'Features'"
              "in /proc/cpuinfo ?\n");
            break;
        }

        line += features_len;  

        
        char* line_end = (char*) memchr(line, '\n', buffer_end - line);
        if (line_end == NULL)
            line_end = buffer_end;

        
        
        
        const char neon[] = " neon";
        const size_t neon_len = sizeof(neon)-1;
        const char* flag = (const char*) memmem(line, line_end - line,
                                                neon, neon_len);
        if (flag == NULL)
            break;

        
        if (flag[neon_len] != ' ' && flag[neon_len] != '\n')
            break;

        
        result = true;

    } while (0);

    if (result) {
        SkDebugf("Device supports ARM NEON instructions!\n");
    } else {
        SkDebugf("Device does NOT support ARM NEON instructions!\n");
    }
    return result;
}

static pthread_once_t  sOnce;
static bool            sHasArmNeon;


void sk_cpu_arm_probe_features(void) {
    sHasArmNeon = sk_cpu_arm_check_neon();
}

bool sk_cpu_arm_has_neon(void) {
    pthread_once(&sOnce, sk_cpu_arm_probe_features);
    return sHasArmNeon;
}

#endif 
