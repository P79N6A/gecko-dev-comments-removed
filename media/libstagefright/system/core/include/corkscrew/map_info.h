

















#ifndef _CORKSCREW_MAP_INFO_H
#define _CORKSCREW_MAP_INFO_H

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct map_info {
    struct map_info* next;
    uintptr_t start;
    uintptr_t end;
    bool is_readable;
    bool is_writable;
    bool is_executable;
    void* data; 
    char name[];
} map_info_t;


map_info_t* load_map_info_list(pid_t tid);


void free_map_info_list(map_info_t* milist);


const map_info_t* find_map_info(const map_info_t* milist, uintptr_t addr);


bool is_readable_map(const map_info_t* milist, uintptr_t addr);

bool is_writable_map(const map_info_t* milist, uintptr_t addr);

bool is_executable_map(const map_info_t* milist, uintptr_t addr);




map_info_t* acquire_my_map_info_list();



void release_my_map_info_list(map_info_t* milist);



void flush_my_map_info_list();

#ifdef __cplusplus
}
#endif

#endif
