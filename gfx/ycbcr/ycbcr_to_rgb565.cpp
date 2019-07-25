





































#include "ycbcr_to_rgb565.h"



#if !defined (HAVE_ARM_NEON)

int have_ycbcr_to_rgb565 ()
{
    return 0;
}

#else

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <elf.h>

#ifdef ANDROID

int have_ycbcr_to_rgb565 ()
{
    static int have_ycbcr_to_rgb565_initialized = 0;
    static int arm_has_neon = 0;
    if (!have_ycbcr_to_rgb565_initialized)
    {
        have_ycbcr_to_rgb565_initialized = 1;

        char buf[1024];
        const char* ver_token = "CPU architecture: ";
        FILE* f = fopen("/proc/cpuinfo", "r");
        if (!f) {
	        return 0;
        }

        fread(buf, sizeof(char), 1024, f);
        arm_has_neon = strstr(buf, "neon") != NULL;
        fclose(f);
    }
    return arm_has_neon;
}

#else

int have_ycbcr_to_rgb565 ()
{
    static int have_ycbcr_to_rgb565_initialized = 0;
    static int arm_has_neon = 0;
    if (!have_ycbcr_to_rgb565_initialized)
    {
        have_ycbcr_to_rgb565_initialized = 1;
        int fd;
        Elf32_auxv_t aux;

        fd = open ("/proc/self/auxv", O_RDONLY);
        if (fd >= 0)
        {
            while (read (fd, &aux, sizeof(Elf32_auxv_t)) == sizeof(Elf32_auxv_t))
            {
                if (aux.a_type == AT_HWCAP)
                {
                    uint32_t hwcap = aux.a_un.a_val;
                    arm_has_neon = (hwcap & 4096) != 0;
                    break;
                }
            }
            close (fd);
         }
    }

    return arm_has_neon;
}

#endif 

#endif 

