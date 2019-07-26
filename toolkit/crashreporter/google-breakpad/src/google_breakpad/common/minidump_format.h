



























































#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_FORMAT_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_FORMAT_H__

#include <stddef.h>

#include "google_breakpad/common/breakpad_types.h"


#if defined(_MSC_VER)


#pragma warning(push)
#pragma warning(disable:4200)
#endif  






typedef struct {
  uint32_t data1;
  uint16_t data2;
  uint16_t data3;
  uint8_t  data4[8];
} MDGUID;  










#define MD_CONTEXT_IA64  0x00080000  /* CONTEXT_IA64 */

#define MD_CONTEXT_SHX   0x000000c0  /* CONTEXT_SH4 (Super-H, includes SH3) */
#define MD_CONTEXT_MIPS  0x00010000  /* CONTEXT_R4000 (same value as x86?) */
#define MD_CONTEXT_ALPHA 0x00020000  /* CONTEXT_ALPHA */




#define MD_CONTEXT_CPU_MASK 0xffffff00





typedef struct {
  uint32_t context_flags;
} MDRawContextBase;

#include "minidump_cpu_amd64.h"
#include "minidump_cpu_arm.h"
#include "minidump_cpu_ppc.h"
#include "minidump_cpu_ppc64.h"
#include "minidump_cpu_sparc.h"
#include "minidump_cpu_x86.h"






typedef struct {
  uint32_t signature;
  uint32_t struct_version;
  uint32_t file_version_hi;
  uint32_t file_version_lo;
  uint32_t product_version_hi;
  uint32_t product_version_lo;
  uint32_t file_flags_mask;    
  uint32_t file_flags;
  uint32_t file_os;
  uint32_t file_type;
  uint32_t file_subtype;
  uint32_t file_date_hi;
  uint32_t file_date_lo;
} MDVSFixedFileInfo;  


#define MD_VSFIXEDFILEINFO_SIGNATURE 0xfeef04bd
     


#define MD_VSFIXEDFILEINFO_VERSION 0x00010000
     



#define MD_VSFIXEDFILEINFO_FILE_FLAGS_DEBUG        0x00000001
     
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_PRERELEASE   0x00000002
     
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_PATCHED      0x00000004
     
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_PRIVATEBUILD 0x00000008
     
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_INFOINFERRED 0x00000010
     
#define MD_VSFIXEDFILEINFO_FILE_FLAGS_SPECIALBUILD 0x00000020
     


#define MD_VSFIXEDFILEINFO_FILE_OS_UNKNOWN    0          /* VOS_UNKNOWN */
#define MD_VSFIXEDFILEINFO_FILE_OS_DOS        (1 << 16)  /* VOS_DOS */
#define MD_VSFIXEDFILEINFO_FILE_OS_OS216      (2 << 16)  /* VOS_OS216 */
#define MD_VSFIXEDFILEINFO_FILE_OS_OS232      (3 << 16)  /* VOS_OS232 */
#define MD_VSFIXEDFILEINFO_FILE_OS_NT         (4 << 16)  /* VOS_NT */
#define MD_VSFIXEDFILEINFO_FILE_OS_WINCE      (5 << 16)  /* VOS_WINCE */

#define MD_VSFIXEDFILEINFO_FILE_OS__BASE      0          /* VOS__BASE */
#define MD_VSFIXEDFILEINFO_FILE_OS__WINDOWS16 1          /* VOS__WINDOWS16 */
#define MD_VSFIXEDFILEINFO_FILE_OS__PM16      2          /* VOS__PM16 */
#define MD_VSFIXEDFILEINFO_FILE_OS__PM32      3          /* VOS__PM32 */
#define MD_VSFIXEDFILEINFO_FILE_OS__WINDOWS32 4          /* VOS__WINDOWS32 */


#define MD_VSFIXEDFILEINFO_FILE_TYPE_UNKNOWN    0  /* VFT_UNKNOWN */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_APP        1  /* VFT_APP */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_DLL        2  /* VFT_DLL */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_DRV        3  /* VFT_DLL */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_FONT       4  /* VFT_FONT */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_VXD        5  /* VFT_VXD */
#define MD_VSFIXEDFILEINFO_FILE_TYPE_STATIC_LIB 7  /* VFT_STATIC_LIB */


#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_UNKNOWN                0
     

#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_PRINTER            1
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_KEYBOARD           2
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_LANGUAGE           3
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_DISPLAY            4
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_MOUSE              5
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_NETWORK            6
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_SYSTEM             7
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_INSTALLABLE        8
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_SOUND              9
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_COMM              10
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_INPUTMETHOD       11
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_DRV_VERSIONED_PRINTER 12
     

#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_FONT_RASTER            1
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_FONT_VECTOR            2
     
#define MD_VSFIXEDFILEINFO_FILE_SUBTYPE_FONT_TRUETYPE          3
     









typedef uint32_t MDRVA;  

typedef struct {
  uint32_t  data_size;
  MDRVA     rva;
} MDLocationDescriptor;  


typedef struct {
  

  uint64_t             start_of_memory_range;

  MDLocationDescriptor memory;
} MDMemoryDescriptor;  


typedef struct {
  uint32_t  signature;
  uint32_t  version;
  uint32_t  stream_count;
  MDRVA     stream_directory_rva;  

  uint32_t  checksum;              

  uint32_t  time_date_stamp;       
  uint64_t  flags;
} MDRawHeader;  




#define MD_HEADER_SIGNATURE 0x504d444d /* 'PMDM' */
     
#define MD_HEADER_VERSION   0x0000a793 /* 42899 */
     


typedef enum {
  







  MD_NORMAL                            = 0x00000000,
  MD_WITH_DATA_SEGS                    = 0x00000001,
  MD_WITH_FULL_MEMORY                  = 0x00000002,
  MD_WITH_HANDLE_DATA                  = 0x00000004,
  MD_FILTER_MEMORY                     = 0x00000008,
  MD_SCAN_MEMORY                       = 0x00000010,
  MD_WITH_UNLOADED_MODULES             = 0x00000020,
  MD_WITH_INDIRECTLY_REFERENCED_MEMORY = 0x00000040,
  MD_FILTER_MODULE_PATHS               = 0x00000080,
  MD_WITH_PROCESS_THREAD_DATA          = 0x00000100,
  MD_WITH_PRIVATE_READ_WRITE_MEMORY    = 0x00000200,
  MD_WITHOUT_OPTIONAL_DATA             = 0x00000400,
  MD_WITH_FULL_MEMORY_INFO             = 0x00000800,
  MD_WITH_THREAD_INFO                  = 0x00001000,
  MD_WITH_CODE_SEGS                    = 0x00002000,
  MD_WITHOUT_AUXILLIARY_SEGS           = 0x00004000,
  MD_WITH_FULL_AUXILLIARY_STATE        = 0x00008000,
  MD_WITH_PRIVATE_WRITE_COPY_MEMORY    = 0x00010000,
  MD_IGNORE_INACCESSIBLE_MEMORY        = 0x00020000,
  MD_WITH_TOKEN_INFORMATION            = 0x00040000
} MDType;  


typedef struct {
  uint32_t             stream_type;
  MDLocationDescriptor location;
} MDRawDirectory;  


typedef enum {
  MD_UNUSED_STREAM               =  0,
  MD_RESERVED_STREAM_0           =  1,
  MD_RESERVED_STREAM_1           =  2,
  MD_THREAD_LIST_STREAM          =  3,  
  MD_MODULE_LIST_STREAM          =  4,  
  MD_MEMORY_LIST_STREAM          =  5,  
  MD_EXCEPTION_STREAM            =  6,  
  MD_SYSTEM_INFO_STREAM          =  7,  
  MD_THREAD_EX_LIST_STREAM       =  8,
  MD_MEMORY_64_LIST_STREAM       =  9,
  MD_COMMENT_STREAM_A            = 10,
  MD_COMMENT_STREAM_W            = 11,
  MD_HANDLE_DATA_STREAM          = 12,
  MD_FUNCTION_TABLE_STREAM       = 13,
  MD_UNLOADED_MODULE_LIST_STREAM = 14,
  MD_MISC_INFO_STREAM            = 15,  
  MD_MEMORY_INFO_LIST_STREAM     = 16,  
  MD_THREAD_INFO_LIST_STREAM     = 17,
  MD_HANDLE_OPERATION_LIST_STREAM = 18,
  MD_LAST_RESERVED_STREAM        = 0x0000ffff,

  
  MD_BREAKPAD_INFO_STREAM        = 0x47670001,  
  MD_ASSERTION_INFO_STREAM       = 0x47670002,  
  

  MD_LINUX_CPU_INFO              = 0x47670003,  
  MD_LINUX_PROC_STATUS           = 0x47670004,  
  MD_LINUX_LSB_RELEASE           = 0x47670005,  
  MD_LINUX_CMD_LINE              = 0x47670006,  
  MD_LINUX_ENVIRON               = 0x47670007,  
  MD_LINUX_AUXV                  = 0x47670008,  
  MD_LINUX_MAPS                  = 0x47670009,  
  MD_LINUX_DSO_DEBUG             = 0x4767000A   
} MDStreamType;  


typedef struct {
  uint32_t length;     

  uint16_t buffer[1];  
} MDString;  

static const size_t MDString_minsize = offsetof(MDString, buffer[0]);


typedef struct {
  uint32_t             thread_id;
  uint32_t             suspend_count;
  uint32_t             priority_class;
  uint32_t             priority;
  uint64_t             teb;             
  MDMemoryDescriptor   stack;
  MDLocationDescriptor thread_context;  
} MDRawThread;  


typedef struct {
  uint32_t    number_of_threads;
  MDRawThread threads[1];
} MDRawThreadList;  

static const size_t MDRawThreadList_minsize = offsetof(MDRawThreadList,
                                                       threads[0]);


typedef struct {
  uint64_t             base_of_image;
  uint32_t             size_of_image;
  uint32_t             checksum;         
  uint32_t             time_date_stamp;  
  MDRVA                module_name_rva;  
  MDVSFixedFileInfo    version_info;

  

  MDLocationDescriptor cv_record;

  


  MDLocationDescriptor misc_record;

  










  uint32_t             reserved0[2];
  uint32_t             reserved1[2];
} MDRawModule;  






#define MD_MODULE_SIZE 108






typedef struct {
  uint32_t signature;
  uint32_t offset;     
} MDCVHeader;

typedef struct {
  MDCVHeader cv_header;
  uint32_t   signature;         
  uint32_t   age;               
  uint8_t    pdb_file_name[1];  
} MDCVInfoPDB20;

static const size_t MDCVInfoPDB20_minsize = offsetof(MDCVInfoPDB20,
                                                     pdb_file_name[0]);

#define MD_CVINFOPDB20_SIGNATURE 0x3031424e  /* cvHeader.signature = '01BN' */

typedef struct {
  uint32_t  cv_signature;
  MDGUID    signature;         
  uint32_t  age;               
  uint8_t   pdb_file_name[1];  

} MDCVInfoPDB70;

static const size_t MDCVInfoPDB70_minsize = offsetof(MDCVInfoPDB70,
                                                     pdb_file_name[0]);

#define MD_CVINFOPDB70_SIGNATURE 0x53445352  /* cvSignature = 'SDSR' */

typedef struct {
  uint32_t data1[2];
  uint32_t data2;
  uint32_t data3;
  uint32_t data4;
  uint32_t data5[3];
  uint8_t  extra[2];
} MDCVInfoELF;












#define MD_CVINFOCV41_SIGNATURE 0x3930424e  /* '90BN', CodeView 4.10. */
#define MD_CVINFOCV50_SIGNATURE 0x3131424e  /* '11BN', CodeView 5.0,
                                             * MS C7-format (/Z7). */

#define MD_CVINFOUNKNOWN_SIGNATURE 0xffffffff  





typedef struct {
  uint32_t  data_type;    

  uint32_t  length;       
  uint8_t   unicode;      
  uint8_t   reserved[3];
  uint8_t   data[1];
} MDImageDebugMisc;  

static const size_t MDImageDebugMisc_minsize = offsetof(MDImageDebugMisc,
                                                        data[0]);


typedef struct {
  uint32_t    number_of_modules;
  MDRawModule modules[1];
} MDRawModuleList;  

static const size_t MDRawModuleList_minsize = offsetof(MDRawModuleList,
                                                       modules[0]);


typedef struct {
  uint32_t           number_of_memory_ranges;
  MDMemoryDescriptor memory_ranges[1];
} MDRawMemoryList;  

static const size_t MDRawMemoryList_minsize = offsetof(MDRawMemoryList,
                                                       memory_ranges[0]);


#define MD_EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct {
  uint32_t  exception_code;     


  uint32_t  exception_flags;    

  uint64_t  exception_record;   


  uint64_t  exception_address;  


  uint32_t  number_parameters;  

  uint32_t  __align;
  uint64_t  exception_information[MD_EXCEPTION_MAXIMUM_PARAMETERS];
} MDException;  

#include "minidump_exception_win32.h"
#include "minidump_exception_mac.h"
#include "minidump_exception_linux.h"
#include "minidump_exception_solaris.h"

typedef struct {
  uint32_t             thread_id;         


  uint32_t             __align;
  MDException          exception_record;
  MDLocationDescriptor thread_context;    
} MDRawExceptionStream;  


typedef union {
  struct {
    uint32_t vendor_id[3];               
    uint32_t version_information;        
    uint32_t feature_information;        
    uint32_t amd_extended_cpu_features;  
  } x86_cpu_info;
  struct {
    uint64_t processor_features[2];
  } other_cpu_info;
} MDCPUInformation;  


typedef struct {
  

  uint16_t         processor_architecture;
  uint16_t         processor_level;         
  uint16_t         processor_revision;      


  uint8_t          number_of_processors;
  uint8_t          product_type;            

  

  uint32_t         major_version;
  uint32_t         minor_version;
  uint32_t         build_number;
  uint32_t         platform_id;
  MDRVA            csd_version_rva;  







  uint16_t         suite_mask;       
  uint16_t         reserved2;

  MDCPUInformation cpu;
} MDRawSystemInfo;  


typedef enum {
  MD_CPU_ARCHITECTURE_X86       =  0,  
  MD_CPU_ARCHITECTURE_MIPS      =  1,  
  MD_CPU_ARCHITECTURE_ALPHA     =  2,  
  MD_CPU_ARCHITECTURE_PPC       =  3,  
  MD_CPU_ARCHITECTURE_SHX       =  4,  

  MD_CPU_ARCHITECTURE_ARM       =  5,  
  MD_CPU_ARCHITECTURE_IA64      =  6,  
  MD_CPU_ARCHITECTURE_ALPHA64   =  7,  
  MD_CPU_ARCHITECTURE_MSIL      =  8,  

  MD_CPU_ARCHITECTURE_AMD64     =  9,  
  MD_CPU_ARCHITECTURE_X86_WIN64 = 10,
      
  MD_CPU_ARCHITECTURE_SPARC     = 0x8001, 
  MD_CPU_ARCHITECTURE_UNKNOWN   = 0xffff  
} MDCPUArchitecture;


typedef enum {
  MD_OS_WIN32S        = 0,  
  MD_OS_WIN32_WINDOWS = 1,  
  MD_OS_WIN32_NT      = 2,  
  MD_OS_WIN32_CE      = 3,  


  
  MD_OS_UNIX          = 0x8000,  
  MD_OS_MAC_OS_X      = 0x8101,  
  MD_OS_IOS           = 0x8102,  
  MD_OS_LINUX         = 0x8201,  
  MD_OS_SOLARIS       = 0x8202,  
  MD_OS_ANDROID       = 0x8203   
} MDOSPlatform;


typedef struct {
  uint32_t size_of_info;  
  uint32_t flags1;

  

  uint32_t process_id;

  

  uint32_t process_create_time;  
  uint32_t process_user_time;    
  uint32_t process_kernel_time;  

  




  uint32_t processor_max_mhz;
  uint32_t processor_current_mhz;
  uint32_t processor_mhz_limit;
  uint32_t processor_max_idle_state;
  uint32_t processor_current_idle_state;
} MDRawMiscInfo;  

#define MD_MISCINFO_SIZE 24
#define MD_MISCINFO2_SIZE 44



typedef enum {
  MD_MISCINFO_FLAGS1_PROCESS_ID           = 0x00000001,
      
  MD_MISCINFO_FLAGS1_PROCESS_TIMES        = 0x00000002,
      
  MD_MISCINFO_FLAGS1_PROCESSOR_POWER_INFO = 0x00000004
      
} MDMiscInfoFlags1;











typedef struct {
  uint32_t size_of_header;    
  uint32_t size_of_entry;     
  uint64_t number_of_entries;
} MDRawMemoryInfoList;  

typedef struct {
  uint64_t  base_address;           
  uint64_t  allocation_base;        

  uint32_t  allocation_protection;  


  uint32_t  __alignment1;
  uint64_t  region_size;
  uint32_t  state;                  
  uint32_t  protection;             
  uint32_t  type;                   
  uint32_t  __alignment2;
} MDRawMemoryInfo;  


typedef enum {
  MD_MEMORY_STATE_COMMIT   = 0x1000,  
  MD_MEMORY_STATE_RESERVE  = 0x2000,  
  MD_MEMORY_STATE_FREE     = 0x10000  
} MDMemoryState;


typedef enum {
  MD_MEMORY_PROTECT_NOACCESS          = 0x01,  
  MD_MEMORY_PROTECT_READONLY          = 0x02,  
  MD_MEMORY_PROTECT_READWRITE         = 0x04,  
  MD_MEMORY_PROTECT_WRITECOPY         = 0x08,  
  MD_MEMORY_PROTECT_EXECUTE           = 0x10,  
  MD_MEMORY_PROTECT_EXECUTE_READ      = 0x20,  
  MD_MEMORY_PROTECT_EXECUTE_READWRITE = 0x40,  
  MD_MEMORY_PROTECT_EXECUTE_WRITECOPY = 0x80,  
  
  MD_MEMORY_PROTECT_GUARD             = 0x100,  
  MD_MEMORY_PROTECT_NOCACHE           = 0x200,  
  MD_MEMORY_PROTECT_WRITECOMBINE      = 0x400,  
} MDMemoryProtection;


const uint32_t MD_MEMORY_PROTECTION_ACCESS_MASK = 0xFF;


typedef enum {
  MD_MEMORY_TYPE_PRIVATE = 0x20000,   
  MD_MEMORY_TYPE_MAPPED  = 0x40000,   
  MD_MEMORY_TYPE_IMAGE   = 0x1000000  
} MDMemoryType;






typedef struct {
  

  uint32_t validity;

  





  uint32_t dump_thread_id;

  










  uint32_t requesting_thread_id;
} MDRawBreakpadInfo;


typedef enum {
  
  MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID       = 1 << 0,

  
  MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID = 1 << 1
} MDBreakpadInfoValidity;

typedef struct {
  




  uint16_t expression[128];  
  uint16_t function[128];    
  uint16_t file[128];        
  uint32_t line;             
  uint32_t type;
} MDRawAssertionInfo;


typedef enum {
  MD_ASSERTION_INFO_TYPE_UNKNOWN = 0,

  

  MD_ASSERTION_INFO_TYPE_INVALID_PARAMETER,

  

  MD_ASSERTION_INFO_TYPE_PURE_VIRTUAL_CALL
} MDAssertionInfoData;



typedef struct {
  void*     addr;
  MDRVA     name;
  void*     ld;
} MDRawLinkMap;

typedef struct {
  uint32_t  version;
  MDRVA     map;
  uint32_t  dso_count;
  void*     brk;
  void*     ldbase;
  void*     dynamic;
} MDRawDebug;

#if defined(_MSC_VER)
#pragma warning(pop)
#endif  


#endif  
