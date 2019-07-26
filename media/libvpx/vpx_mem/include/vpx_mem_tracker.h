










#ifndef __VPX_MEM_TRACKER_H__
#define __VPX_MEM_TRACKER_H__


#define vpx_mem_tracker_version "2.5.1.1"

#define VPX_MEM_TRACKER_VERSION_CHIEF 2
#define VPX_MEM_TRACKER_VERSION_MAJOR 5
#define VPX_MEM_TRACKER_VERSION_MINOR 1
#define VPX_MEM_TRACKER_VERSION_PATCH 1


#include <stdarg.h>

struct mem_block
{
    size_t addr;
    unsigned int size,
             line;
    char *file;
    struct mem_block *prev,
            * next;

    int padded; 
    
    
    
};

#if defined(__cplusplus)
extern "C" {
#endif












    int vpx_memory_tracker_init(int padding_size, int pad_value);

    



    void vpx_memory_tracker_destroy();

    









    void vpx_memory_tracker_add(size_t addr, unsigned int size,
                                char *file, unsigned int line,
                                int padded);

    












    int vpx_memory_tracker_remove(size_t addr);

    







    struct mem_block *vpx_memory_tracker_find(size_t addr);

    




    void vpx_memory_tracker_dump();

    






    void vpx_memory_tracker_check_integrity(char *file, unsigned int line);

    














    int vpx_memory_tracker_set_log_type(int type, char *option);

    









    int vpx_memory_tracker_set_log_func(void *userdata,
                                        void(*logfunc)(void *userdata,
                                                const char *fmt, va_list args));

    
    typedef void*(* mem_track_malloc_func)(size_t);
    typedef void*(* mem_track_calloc_func)(size_t, size_t);
    typedef void*(* mem_track_realloc_func)(void *, size_t);
    typedef void (* mem_track_free_func)(void *);
    typedef void*(* mem_track_memcpy_func)(void *, const void *, size_t);
    typedef void*(* mem_track_memset_func)(void *, int, size_t);
    typedef void*(* mem_track_memmove_func)(void *, const void *, size_t);

    








    int vpx_memory_tracker_set_functions(mem_track_malloc_func g_malloc_l
                                         , mem_track_calloc_func g_calloc_l
                                         , mem_track_realloc_func g_realloc_l
                                         , mem_track_free_func g_free_l
                                         , mem_track_memcpy_func g_memcpy_l
                                         , mem_track_memset_func g_memset_l
                                         , mem_track_memmove_func g_memmove_l);

#if defined(__cplusplus)
}
#endif

#endif
