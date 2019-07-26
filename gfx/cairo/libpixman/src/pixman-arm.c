




















#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pixman-private.h"

typedef enum
{
    ARM_V7		= (1 << 0),
    ARM_V6		= (1 << 1),
    ARM_VFP		= (1 << 2),
    ARM_NEON		= (1 << 3),
    ARM_IWMMXT		= (1 << 4)
} arm_cpu_features_t;

#if defined(USE_ARM_SIMD) || defined(USE_ARM_NEON) || defined(USE_ARM_IWMMXT)

#if defined(_MSC_VER)


#include <windows.h>

extern int pixman_msvc_try_arm_neon_op ();
extern int pixman_msvc_try_arm_simd_op ();

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;

    __try
    {
	pixman_msvc_try_arm_simd_op ();
	features |= ARM_V6;
    }
    __except (GetExceptionCode () == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
    }

    __try
    {
	pixman_msvc_try_arm_neon_op ();
	features |= ARM_NEON;
    }
    __except (GetExceptionCode () == EXCEPTION_ILLEGAL_INSTRUCTION)
    {
    }

    return features;
}

#elif defined(__APPLE__) && defined(TARGET_OS_IPHONE) 

#include "TargetConditionals.h"

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;

    features |= ARM_V6;

    





#if defined(__ARM_NEON__)
    features |= ARM_NEON;
#endif

    return features;
}

#elif defined(__ANDROID__) || defined(ANDROID) 

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;
    char buf[1024];
    char* pos;
    const char* ver_token = "CPU architecture: ";
    FILE* f = fopen("/proc/cpuinfo", "r");
    if (!f) {
	return features;
    }

    fread(buf, sizeof(char), sizeof(buf), f);
    fclose(f);
    pos = strstr(buf, ver_token);
    if (pos) {
	char vchar = *(pos + strlen(ver_token));
	if (vchar >= '0' && vchar <= '9') {
	    int ver = vchar - '0';
	    if (ver >= 7)
		features |= ARM_V7;
	}
    }
    if (strstr(buf, "neon") != NULL)
	features |= ARM_NEON;
    if (strstr(buf, "vfp") != NULL)
	features |= ARM_VFP;

    return features;
}

#elif defined (__linux__) 

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <elf.h>

static arm_cpu_features_t
detect_cpu_features (void)
{
    arm_cpu_features_t features = 0;
    Elf32_auxv_t aux;
    int fd;

    fd = open ("/proc/self/auxv", O_RDONLY);
    if (fd >= 0)
    {
	while (read (fd, &aux, sizeof(Elf32_auxv_t)) == sizeof(Elf32_auxv_t))
	{
	    if (aux.a_type == AT_HWCAP)
	    {
		uint32_t hwcap = aux.a_un.a_val;

		


		if ((hwcap & 64) != 0)
		    features |= ARM_VFP;
		if ((hwcap & 512) != 0)
		    features |= ARM_IWMMXT;
		
		if ((hwcap & 4096) != 0)
		    features |= ARM_NEON;
	    }
	    else if (aux.a_type == AT_PLATFORM)
	    {
		const char *plat = (const char*) aux.a_un.a_val;

		if (strncmp (plat, "v7l", 3) == 0)
		    features |= (ARM_V7 | ARM_V6);
		else if (strncmp (plat, "v6l", 3) == 0)
		    features |= ARM_V6;
	    }
	}
	close (fd);
    }

    return features;
}

#else 

static arm_cpu_features_t
detect_cpu_features (void)
{
    return 0;
}

#endif 

static pixman_bool_t
have_feature (arm_cpu_features_t feature)
{
    static pixman_bool_t initialized;
    static arm_cpu_features_t features;

    if (!initialized)
    {
	features = detect_cpu_features();
	initialized = TRUE;
    }

    return (features & feature) == feature;
}

#endif 

pixman_implementation_t *
_pixman_arm_get_implementations (pixman_implementation_t *imp)
{
#ifdef USE_ARM_SIMD
    if (!_pixman_disabled ("arm-simd") && have_feature (ARM_V6))
	imp = _pixman_implementation_create_arm_simd (imp);
#endif

#ifdef USE_ARM_IWMMXT
    if (!_pixman_disabled ("arm-iwmmxt") && have_feature (ARM_IWMMXT))
	imp = _pixman_implementation_create_mmx (imp);
#endif

#ifdef USE_ARM_NEON
    if (!_pixman_disabled ("arm-neon") && have_feature (ARM_NEON))
	imp = _pixman_implementation_create_arm_neon (imp);
#endif

    return imp;
}
