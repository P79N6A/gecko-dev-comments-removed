










































#include <stdio.h>
#include <stdlib.h>

#if defined(CROSS_COMPILE) && !defined(FORCE_BIG_ENDIAN) && !defined(FORCE_LITTLE_ENDIAN)
#include <prtypes.h>
#endif



int main(int argc, char **argv)
{
    printf("#ifndef js_cpucfg___\n");
    printf("#define js_cpucfg___\n\n");

    printf("/* AUTOMATICALLY GENERATED - DO NOT EDIT */\n\n");

#ifdef CROSS_COMPILE
#if defined(__APPLE__) && !defined(FORCE_BIG_ENDIAN) && !defined(FORCE_LITTLE_ENDIAN)
    




    printf("#ifdef __LITTLE_ENDIAN__\n");
    printf("#define IS_LITTLE_ENDIAN 1\n");
    printf("#undef  IS_BIG_ENDIAN\n");
    printf("#else\n");
    printf("#undef  IS_LITTLE_ENDIAN\n");
    printf("#define IS_BIG_ENDIAN 1\n");
    printf("#endif\n\n");
#elif defined(IS_LITTLE_ENDIAN) || defined(FORCE_LITTLE_ENDIAN)
    printf("#define IS_LITTLE_ENDIAN 1\n");
    printf("#undef  IS_BIG_ENDIAN\n\n");
#elif defined(IS_BIG_ENDIAN) || defined(FORCE_BIG_ENDIAN)
    printf("#undef  IS_LITTLE_ENDIAN\n");
    printf("#define IS_BIG_ENDIAN 1\n\n");
#else
#error "Endianess not defined."
#endif

#else

    




    {
        int big_endian = 0, little_endian = 0, ntests = 0;

        if (sizeof(short) == 2) {
            





            volatile static union {
                short i;
                char c[2];
            } u;

            u.i = 0x0102;
            big_endian += (u.c[0] == 0x01 && u.c[1] == 0x02);
            little_endian += (u.c[0] == 0x02 && u.c[1] == 0x01);
            ntests++;
        }

        if (sizeof(int) == 4) {
            
            volatile static union {
                int i;
                char c[4];
            } u;

            u.i = 0x01020304;
            big_endian += (u.c[0] == 0x01 && u.c[1] == 0x02 &&
                           u.c[2] == 0x03 && u.c[3] == 0x04);
            little_endian += (u.c[0] == 0x04 && u.c[1] == 0x03 &&
                              u.c[2] == 0x02 && u.c[3] == 0x01);
            ntests++;
        }

        if (sizeof(long) == 8) {
            
            volatile static union {
                long i;
                char c[8];
            } u;

            



            u.i = 0x01020304;
            u.i <<= 16, u.i <<= 16;
            u.i |= 0x05060708;
            big_endian += (u.c[0] == 0x01 && u.c[1] == 0x02 &&
                           u.c[2] == 0x03 && u.c[3] == 0x04 &&
                           u.c[4] == 0x05 && u.c[5] == 0x06 &&
                           u.c[6] == 0x07 && u.c[7] == 0x08);
            little_endian += (u.c[0] == 0x08 && u.c[1] == 0x07 &&
                              u.c[2] == 0x06 && u.c[3] == 0x05 &&
                              u.c[4] == 0x04 && u.c[5] == 0x03 &&
                              u.c[6] == 0x02 && u.c[7] == 0x01);
            ntests++;
        }

        if (big_endian && big_endian == ntests) {
            printf("#undef  IS_LITTLE_ENDIAN\n");
            printf("#define IS_BIG_ENDIAN 1\n\n");
        } else if (little_endian && little_endian == ntests) {
            printf("#define IS_LITTLE_ENDIAN 1\n");
            printf("#undef  IS_BIG_ENDIAN\n\n");
        } else {
            fprintf(stderr, "%s: unknown byte order"
                    "(big_endian=%d, little_endian=%d, ntests=%d)!\n",
                    argv[0], big_endian, little_endian, ntests);
            return EXIT_FAILURE;
        }
    }

#endif 

    
    
    
    
    printf("#ifdef __hppa\n"
           "# define JS_STACK_GROWTH_DIRECTION (1)\n"
           "#else\n"
           "# define JS_STACK_GROWTH_DIRECTION (-1)\n"
           "#endif\n");

    printf("#endif /* js_cpucfg___ */\n");

    return EXIT_SUCCESS;
}

