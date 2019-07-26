















#ifndef ANDROID_INCLUDE_HARDWARE_HARDWARE_H
#define ANDROID_INCLUDE_HARDWARE_HARDWARE_H

#include <stdint.h>
#include <sys/cdefs.h>

#include <cutils/native_handle.h>
#include <system/graphics.h>

__BEGIN_DECLS





#define MAKE_TAG_CONSTANT(A,B,C,D) (((A) << 24) | ((B) << 16) | ((C) << 8) | (D))

#define HARDWARE_MODULE_TAG MAKE_TAG_CONSTANT('H', 'W', 'M', 'T')
#define HARDWARE_DEVICE_TAG MAKE_TAG_CONSTANT('H', 'W', 'D', 'T')

#define HARDWARE_MAKE_API_VERSION(maj,min) \
            ((((maj) & 0xff) << 8) | ((min) & 0xff))

#define HARDWARE_MAKE_API_VERSION_2(maj,min,hdr) \
            ((((maj) & 0xff) << 24) | (((min) & 0xff) << 16) | ((hdr) & 0xffff))
#define HARDWARE_API_VERSION_2_MAJ_MIN_MASK 0xffff0000
#define HARDWARE_API_VERSION_2_HEADER_MASK  0x0000ffff













#define HARDWARE_HAL_API_VERSION HARDWARE_MAKE_API_VERSION(1, 0)










#define HARDWARE_MODULE_API_VERSION(maj,min) HARDWARE_MAKE_API_VERSION(maj,min)
#define HARDWARE_MODULE_API_VERSION_2(maj,min,hdr) HARDWARE_MAKE_API_VERSION_2(maj,min,hdr)




#define HARDWARE_DEVICE_API_VERSION(maj,min) HARDWARE_MAKE_API_VERSION(maj,min)
#define HARDWARE_DEVICE_API_VERSION_2(maj,min,hdr) HARDWARE_MAKE_API_VERSION_2(maj,min,hdr)

struct hw_module_t;
struct hw_module_methods_t;
struct hw_device_t;






typedef struct hw_module_t {
    
    uint32_t tag;

    




















    uint16_t module_api_version;
#define version_major module_api_version
    





    









    uint16_t hal_api_version;
#define version_minor hal_api_version

    
    const char *id;

    
    const char *name;

    
    const char *author;

    
    struct hw_module_methods_t* methods;

    
    void* dso;

    
    uint32_t reserved[32-7];

} hw_module_t;

typedef struct hw_module_methods_t {
    
    int (*open)(const struct hw_module_t* module, const char* id,
            struct hw_device_t** device);

} hw_module_methods_t;





typedef struct hw_device_t {
    
    uint32_t tag;

    















    uint32_t version;

    
    struct hw_module_t* module;

    
    uint32_t reserved[12];

    
    int (*close)(struct hw_device_t* device);

} hw_device_t;




#define HAL_MODULE_INFO_SYM         HMI




#define HAL_MODULE_INFO_SYM_AS_STR  "HMI"






int hw_get_module(const char *id, const struct hw_module_t **module);













int hw_get_module_by_class(const char *class_id, const char *inst,
                           const struct hw_module_t **module);

__END_DECLS

#endif  
