














#include "dlfcn.h"
#include <pthread.h>
#include <stdio.h>
#include "linker.h"
#include "linker_format.h"



#define DL_SUCCESS                    0
#define DL_ERR_CANNOT_LOAD_LIBRARY    1
#define DL_ERR_INVALID_LIBRARY_HANDLE 2
#define DL_ERR_BAD_SYMBOL_NAME        3
#define DL_ERR_SYMBOL_NOT_FOUND       4
#define DL_ERR_SYMBOL_NOT_GLOBAL      5

static char dl_err_buf[1024];
static const char *dl_err_str;

static const char *dl_errors[] = {
    [DL_ERR_CANNOT_LOAD_LIBRARY] = "Cannot load library",
    [DL_ERR_INVALID_LIBRARY_HANDLE] = "Invalid library handle",
    [DL_ERR_BAD_SYMBOL_NAME] = "Invalid symbol name",
    [DL_ERR_SYMBOL_NOT_FOUND] = "Symbol not found",
    [DL_ERR_SYMBOL_NOT_GLOBAL] = "Symbol is not global",
};

#define likely(expr)   __builtin_expect (expr, 1)
#define unlikely(expr) __builtin_expect (expr, 0)

static pthread_mutex_t dl_lock = PTHREAD_MUTEX_INITIALIZER;

static void set_dlerror(int err)
{
    format_buffer(dl_err_buf, sizeof(dl_err_buf), "%s: %s", dl_errors[err],
             linker_get_error());
    dl_err_str = (const char *)&dl_err_buf[0];
}

void *__wrap_dlopen(const char *filename, int flag)
{
    soinfo *ret;

    pthread_mutex_lock(&dl_lock);
    ret = find_library(filename);
    if (unlikely(ret == NULL)) {
        set_dlerror(DL_ERR_CANNOT_LOAD_LIBRARY);
    } else {
        ret->refcount++;
    }
    pthread_mutex_unlock(&dl_lock);
    return ret;
}

void *moz_mapped_dlopen(const char *filename, int flag,
                        int fd, void *mem, unsigned int len, unsigned int offset)
{
    soinfo *ret;

    pthread_mutex_lock(&dl_lock);
    ret = find_mapped_library(filename, fd, mem, len, offset);
    if (unlikely(ret == NULL)) {
        set_dlerror(DL_ERR_CANNOT_LOAD_LIBRARY);
    } else {
        ret->refcount++;
    }
    pthread_mutex_unlock(&dl_lock);
    return ret;
}

const char *__wrap_dlerror(void)
{
    const char *tmp = dl_err_str;
    dl_err_str = NULL;
    return (const char *)tmp;
}

void *__wrap_dlsym(void *handle, const char *symbol)
{
    soinfo *found;
    Elf32_Sym *sym;
    unsigned bind;

    pthread_mutex_lock(&dl_lock);

    if(unlikely(handle == 0)) { 
        set_dlerror(DL_ERR_INVALID_LIBRARY_HANDLE);
        goto err;
    }
    if(unlikely(symbol == 0)) {
        set_dlerror(DL_ERR_BAD_SYMBOL_NAME);
        goto err;
    }

    if(handle == RTLD_DEFAULT) {
        sym = lookup(symbol, &found, NULL);
    } else if(handle == RTLD_NEXT) {
        void *ret_addr = __builtin_return_address(0);
        soinfo *si = find_containing_library(ret_addr);

        sym = NULL;
        if(si && si->next) {
            sym = lookup(symbol, &found, si->next);
        }
    } else {
        found = (soinfo*)handle;
        sym = lookup_in_library(found, symbol);
    }

    if(likely(sym != 0)) {
        bind = ELF32_ST_BIND(sym->st_info);

        if(likely((bind == STB_GLOBAL) && (sym->st_shndx != 0))) {
            unsigned ret = sym->st_value + found->base;
            pthread_mutex_unlock(&dl_lock);
            return (void*)ret;
        }

        set_dlerror(DL_ERR_SYMBOL_NOT_GLOBAL);
    }
    else
        set_dlerror(DL_ERR_SYMBOL_NOT_FOUND);

err:
    pthread_mutex_unlock(&dl_lock);
    return 0;
}

int __wrap_dladdr(void *addr, Dl_info *info)
{
    int ret = 0;

    pthread_mutex_lock(&dl_lock);

    
    soinfo *si = find_containing_library(addr);

    if(si) {
        memset(info, 0, sizeof(Dl_info));

        info->dli_fname = si->name;
        info->dli_fbase = (void*)si->base;

        
        Elf32_Sym *sym = find_containing_symbol(addr, si);

        if(sym != NULL) {
            info->dli_sname = si->strtab + sym->st_name;
            info->dli_saddr = (void*)(si->base + sym->st_value);
        }

        ret = 1;
    }

    pthread_mutex_unlock(&dl_lock);

    return ret;
}

int __wrap_dlclose(void *handle)
{
    pthread_mutex_lock(&dl_lock);
    (void)unload_library((soinfo*)handle);
    pthread_mutex_unlock(&dl_lock);
    return 0;
}

#if defined(ANDROID_ARM_LINKER)


#define ANDROID_LIBDL_STRTAB \
                      "__wrap_dlopen\0__wrap_dlclose\0__wrap_dlsym\0__wrap_dlerror\0__wrap_dladdr\0dl_unwind_find_exidx\0"

#elif defined(ANDROID_X86_LINKER)


#define ANDROID_LIBDL_STRTAB \
                      "__wrap_dlopen\0__wrap_dlclose\0__wrap_dlsym\0__wrap_dlerror\0__wrap_dladdr\0dl_iterate_phdr\0"

#elif defined(ANDROID_SH_LINKER)


#define ANDROID_LIBDL_STRTAB \
                      "__wrap_dlopen\0__wrap_dlclose\0__wrap_dlsym\0__wrap_dlerror\0__wrap_dladdr\0dl_iterate_phdr\0"

#else 
#error Unsupported architecture. Only ARM and x86 are presently supported.
#endif


static Elf32_Sym libdl_symtab[] = {
      
      
      
      
    { st_name: sizeof(ANDROID_LIBDL_STRTAB) - 1,
    },
    { st_name: 0,   
      st_value: (Elf32_Addr) &__wrap_dlopen,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
    { st_name: 14,
      st_value: (Elf32_Addr) &__wrap_dlclose,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
    { st_name: 22,
      st_value: (Elf32_Addr) &__wrap_dlsym,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
    { st_name: 28,
      st_value: (Elf32_Addr) &__wrap_dlerror,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
    { st_name: 36,
      st_value: (Elf32_Addr) &__wrap_dladdr,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
#ifdef ANDROID_ARM_LINKER
    { st_name: 43,
      st_value: (Elf32_Addr) &dl_unwind_find_exidx,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
#elif defined(ANDROID_X86_LINKER)
    { st_name: 43,
      st_value: (Elf32_Addr) &dl_iterate_phdr,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
#elif defined(ANDROID_SH_LINKER)
    { st_name: 43,
      st_value: (Elf32_Addr) &dl_iterate_phdr,
      st_info: STB_GLOBAL << 4,
      st_shndx: 1,
    },
#endif
};




















static unsigned libdl_buckets[1] = { 1 };
static unsigned libdl_chains[7] = { 0, 2, 3, 4, 5, 6, 0 };

soinfo libdl_info = {
    name: "libdl.so",
    flags: FLAG_LINKED,

    strtab: ANDROID_LIBDL_STRTAB,
    symtab: libdl_symtab,

    nbucket: 1,
    nchain: 7,
    bucket: libdl_buckets,
    chain: libdl_chains,
};
    
