



#include <stdint.h>
#include <elf.h>


#undef Elf_Ehdr
#undef Elf_Addr

#if defined(__LP64__)
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Addr Elf64_Addr
#else
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Addr Elf32_Addr
#endif

extern __attribute__((visibility("hidden"))) void original_init(int argc, char **argv, char **env);

extern __attribute__((visibility("hidden"))) Elf32_Rel relhack[];
extern __attribute__((visibility("hidden"))) Elf_Ehdr elf_header;

int init(int argc, char **argv, char **env)
{
    Elf32_Rel *rel;
    Elf_Addr *ptr, *start;
    for (rel = relhack; rel->r_offset; rel++) {
        start = (Elf_Addr *)((intptr_t)&elf_header + rel->r_offset);
        for (ptr = start; ptr < &start[rel->r_info]; ptr++)
            *ptr += (intptr_t)&elf_header;
    }

#ifndef NOINIT
    original_init(argc, argv, env);
#endif
    
    
    return 0;
}
