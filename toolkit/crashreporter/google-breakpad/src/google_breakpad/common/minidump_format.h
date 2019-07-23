





































































 

#ifndef GOOGLE_BREAKPAD_COMMON_MINIDUMP_FORMAT_H__
#define GOOGLE_BREAKPAD_COMMON_MINIDUMP_FORMAT_H__

#include <stddef.h>

#include "google_breakpad/common/breakpad_types.h"


#if defined(_MSC_VER)


#pragma warning(push)
#pragma warning(disable:4200)
#endif  







typedef struct {
  u_int32_t data1;
  u_int16_t data2;
  u_int16_t data3;
  u_int8_t  data4[8];
} MDGUID;  







#define MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE 80
     

typedef struct {
  u_int32_t control_word;
  u_int32_t status_word;
  u_int32_t tag_word;
  u_int32_t error_offset;
  u_int32_t error_selector;
  u_int32_t data_offset;
  u_int32_t data_selector;

  

  u_int8_t  register_area[MD_FLOATINGSAVEAREA_X86_REGISTERAREA_SIZE];
  u_int32_t cr0_npx_state;
} MDFloatingSaveAreaX86;  


#define MD_CONTEXT_X86_EXTENDED_REGISTERS_SIZE 512
     

typedef struct {
  

  u_int32_t             context_flags;

  
  u_int32_t             dr0;
  u_int32_t             dr1;
  u_int32_t             dr2;
  u_int32_t             dr3;
  u_int32_t             dr6;
  u_int32_t             dr7;

  
  MDFloatingSaveAreaX86 float_save;

  
  u_int32_t             gs; 
  u_int32_t             fs;
  u_int32_t             es;
  u_int32_t             ds;
  
  u_int32_t             edi;
  u_int32_t             esi;
  u_int32_t             ebx;
  u_int32_t             edx;
  u_int32_t             ecx;
  u_int32_t             eax;

  
  u_int32_t             ebp;
  u_int32_t             eip;
  u_int32_t             cs;      
  u_int32_t             eflags;  
  u_int32_t             esp;
  u_int32_t             ss;

  




  u_int8_t              extended_registers[
                         MD_CONTEXT_X86_EXTENDED_REGISTERS_SIZE];
} MDRawContextX86;  




#define MD_CONTEXT_X86                    0x00010000
     
#define MD_CONTEXT_X86_CONTROL            (MD_CONTEXT_X86 | 0x00000001)
     
#define MD_CONTEXT_X86_INTEGER            (MD_CONTEXT_X86 | 0x00000002)
     
#define MD_CONTEXT_X86_SEGMENTS           (MD_CONTEXT_X86 | 0x00000004)
     
#define MD_CONTEXT_X86_FLOATING_POINT     (MD_CONTEXT_X86 | 0x00000008)
     
#define MD_CONTEXT_X86_DEBUG_REGISTERS    (MD_CONTEXT_X86 | 0x00000010)
     
#define MD_CONTEXT_X86_EXTENDED_REGISTERS (MD_CONTEXT_X86 | 0x00000020)
     

#define MD_CONTEXT_X86_FULL              (MD_CONTEXT_X86_CONTROL | \
                                          MD_CONTEXT_X86_INTEGER | \
                                          MD_CONTEXT_X86_SEGMENTS)
     

#define MD_CONTEXT_X86_ALL               (MD_CONTEXT_X86_FULL | \
                                          MD_CONTEXT_X86_FLOATING_POINT | \
                                          MD_CONTEXT_X86_DEBUG_REGISTERS | \
                                          MD_CONTEXT_X86_EXTENDED_REGISTERS)
     





#define MD_CONTEXT_IA64  0x00080000  /* CONTEXT_IA64 */
#define MD_CONTEXT_AMD64 0x00100000  /* CONTEXT_AMD64 */

#define MD_CONTEXT_SHX   0x000000c0  /* CONTEXT_SH4 (Super-H, includes SH3) */
#define MD_CONTEXT_ARM   0x00000040  /* CONTEXT_ARM (0x40 bit set in SHx?) */
#define MD_CONTEXT_MIPS  0x00010000  /* CONTEXT_R4000 (same value as x86?) */
#define MD_CONTEXT_ALPHA 0x00020000  /* CONTEXT_ALPHA */

#define MD_CONTEXT_CPU_MASK 0xffffffc0











typedef struct {
  u_int32_t context_flags;
} MDRawContextBase;


#define MD_FLOATINGSAVEAREA_PPC_FPR_COUNT 32

typedef struct {
  

  u_int64_t fpregs[MD_FLOATINGSAVEAREA_PPC_FPR_COUNT];
  u_int32_t fpscr_pad;
  u_int32_t fpscr;      
} MDFloatingSaveAreaPPC;  


#define MD_VECTORSAVEAREA_PPC_VR_COUNT 32

typedef struct {
  

  u_int128_t save_vr[MD_VECTORSAVEAREA_PPC_VR_COUNT];
  u_int128_t save_vscr;  
  u_int32_t  save_pad5[4];
  u_int32_t  save_vrvalid;  
  u_int32_t  save_pad6[7];
} MDVectorSaveAreaPPC;  


#define MD_CONTEXT_PPC_GPR_COUNT 32

typedef struct {
  


  u_int32_t             context_flags;

  u_int32_t             srr0;    

  u_int32_t             srr1;    

  

  u_int32_t             gpr[MD_CONTEXT_PPC_GPR_COUNT];
  u_int32_t             cr;      
  u_int32_t             xer;     
  u_int32_t             lr;      
  u_int32_t             ctr;     
  u_int32_t             mq;      
  u_int32_t             vrsave;  

  


  MDFloatingSaveAreaPPC float_save;
  MDVectorSaveAreaPPC   vector_save;
} MDRawContextPPC;  





#define MD_CONTEXT_PPC                0x20000000
#define MD_CONTEXT_PPC_BASE           (MD_CONTEXT_PPC | 0x00000001)
#define MD_CONTEXT_PPC_FLOATING_POINT (MD_CONTEXT_PPC | 0x00000008)
#define MD_CONTEXT_PPC_VECTOR         (MD_CONTEXT_PPC | 0x00000020)

#define MD_CONTEXT_PPC_FULL           MD_CONTEXT_PPC_BASE
#define MD_CONTEXT_PPC_ALL            (MD_CONTEXT_PPC_FULL | \
                                       MD_CONTEXT_PPC_FLOATING_POINT | \
                                       MD_CONTEXT_PPC_VECTOR)







typedef struct {
  u_int32_t signature;
  u_int32_t struct_version;
  u_int32_t file_version_hi;
  u_int32_t file_version_lo;
  u_int32_t product_version_hi;
  u_int32_t product_version_lo;
  u_int32_t file_flags_mask;    
  u_int32_t file_flags;
  u_int32_t file_os;
  u_int32_t file_type;
  u_int32_t file_subtype;
  u_int32_t file_date_hi;
  u_int32_t file_date_lo;
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
     









typedef u_int32_t MDRVA;  


typedef struct {
  u_int32_t data_size;
  MDRVA     rva;
} MDLocationDescriptor;  


typedef struct {
  

  u_int64_t            start_of_memory_range;

  MDLocationDescriptor memory;
} MDMemoryDescriptor;  


typedef struct {
  u_int32_t signature;
  u_int32_t version;
  u_int32_t stream_count;
  MDRVA     stream_directory_rva;  

  u_int32_t checksum;              

  u_int32_t time_date_stamp;       
  u_int64_t flags;
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
  MD_WITH_CODE_SEGS                    = 0x00002000
} MDType;  


typedef struct {
  u_int32_t            stream_type;
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
  MD_LAST_RESERVED_STREAM        = 0x0000ffff,

  
  MD_BREAKPAD_INFO_STREAM          = 0x47670001,  
  MD_ASSERTION_INFO_STREAM       = 0x47670002   
} MDStreamType;  


typedef struct {
  u_int32_t length;     

  u_int16_t buffer[1];  
} MDString;  

static const size_t MDString_minsize = offsetof(MDString, buffer[0]);


typedef struct {
  u_int32_t            thread_id;
  u_int32_t            suspend_count;
  u_int32_t            priority_class;
  u_int32_t            priority;
  u_int64_t            teb;             
  MDMemoryDescriptor   stack;
  MDLocationDescriptor thread_context;  
} MDRawThread;  


typedef struct {
  u_int32_t   number_of_threads;
  MDRawThread threads[1];
} MDRawThreadList;  

static const size_t MDRawThreadList_minsize = offsetof(MDRawThreadList,
                                                       threads[0]);


typedef struct {
  u_int64_t            base_of_image;
  u_int32_t            size_of_image;
  u_int32_t            checksum;         
  u_int32_t            time_date_stamp;  
  MDRVA                module_name_rva;  
  MDVSFixedFileInfo    version_info;

  

  MDLocationDescriptor cv_record;

  


  MDLocationDescriptor misc_record;

  










  u_int32_t            reserved0[2];
  u_int32_t            reserved1[2];
} MDRawModule;  






#define MD_MODULE_SIZE 108






typedef struct {
  u_int32_t signature;
  u_int32_t offset;     
} MDCVHeader;

typedef struct {
  MDCVHeader cv_header;
  u_int32_t  signature;         
  u_int32_t  age;               
  u_int8_t   pdb_file_name[1];  
} MDCVInfoPDB20;

static const size_t MDCVInfoPDB20_minsize = offsetof(MDCVInfoPDB20,
                                                     pdb_file_name[0]);

#define MD_CVINFOPDB20_SIGNATURE 0x3031424e  /* cvHeader.signature = '01BN' */

typedef struct {
  u_int32_t cv_signature;
  MDGUID    signature;         
  u_int32_t age;               
  u_int8_t  pdb_file_name[1];  

} MDCVInfoPDB70;

static const size_t MDCVInfoPDB70_minsize = offsetof(MDCVInfoPDB70,
                                                     pdb_file_name[0]);

#define MD_CVINFOPDB70_SIGNATURE 0x53445352  /* cvSignature = 'SDSR' */












#define MD_CVINFOCV41_SIGNATURE 0x3930424e  /* '90BN', CodeView 4.10. */
#define MD_CVINFOCV50_SIGNATURE 0x3131424e  /* '11BN', CodeView 5.0,
                                             * MS C7-format (/Z7). */

#define MD_CVINFOUNKNOWN_SIGNATURE 0xffffffff  





typedef struct {
  u_int32_t data_type;    

  u_int32_t length;       
  u_int8_t  unicode;      
  u_int8_t  reserved[3];
  u_int8_t  data[1];
} MDImageDebugMisc;  

static const size_t MDImageDebugMisc_minsize = offsetof(MDImageDebugMisc,
                                                        data[0]);


typedef struct {
  u_int32_t   number_of_modules;
  MDRawModule modules[1];
} MDRawModuleList;  

static const size_t MDRawModuleList_minsize = offsetof(MDRawModuleList,
                                                       modules[0]);


typedef struct {
  u_int32_t          number_of_memory_ranges;
  MDMemoryDescriptor memory_ranges[1];
} MDRawMemoryList;  

static const size_t MDRawMemoryList_minsize = offsetof(MDRawMemoryList,
                                                       memory_ranges[0]);


#define MD_EXCEPTION_MAXIMUM_PARAMETERS 15

typedef struct {
  u_int32_t exception_code;     


  u_int32_t exception_flags;    

  u_int64_t exception_record;   


  u_int64_t exception_address;  


  u_int32_t number_parameters;  

  u_int32_t __align;
  u_int64_t exception_information[MD_EXCEPTION_MAXIMUM_PARAMETERS];
} MDException;  




typedef enum {
  MD_EXCEPTION_CODE_WIN_CONTROL_C                = 0x40010005,
      
  MD_EXCEPTION_CODE_WIN_GUARD_PAGE_VIOLATION     = 0x80000001,
      
  MD_EXCEPTION_CODE_WIN_DATATYPE_MISALIGNMENT    = 0x80000002,
      
  MD_EXCEPTION_CODE_WIN_BREAKPOINT               = 0x80000003,
      
  MD_EXCEPTION_CODE_WIN_SINGLE_STEP              = 0x80000004,
      
  MD_EXCEPTION_CODE_WIN_ACCESS_VIOLATION         = 0xc0000005,
      
  MD_EXCEPTION_CODE_WIN_IN_PAGE_ERROR            = 0xc0000006,
      
  MD_EXCEPTION_CODE_WIN_INVALID_HANDLE           = 0xc0000008,
      
  MD_EXCEPTION_CODE_WIN_ILLEGAL_INSTRUCTION      = 0xc000001d,
      
  MD_EXCEPTION_CODE_WIN_NONCONTINUABLE_EXCEPTION = 0xc0000025,
      
  MD_EXCEPTION_CODE_WIN_INVALID_DISPOSITION      = 0xc0000026,
      
  MD_EXCEPTION_CODE_WIN_ARRAY_BOUNDS_EXCEEDED    = 0xc000008c,
      
  MD_EXCEPTION_CODE_WIN_FLOAT_DENORMAL_OPERAND   = 0xc000008d,
      
  MD_EXCEPTION_CODE_WIN_FLOAT_DIVIDE_BY_ZERO     = 0xc000008e,
      
  MD_EXCEPTION_CODE_WIN_FLOAT_INEXACT_RESULT     = 0xc000008f,
      
  MD_EXCEPTION_CODE_WIN_FLOAT_INVALID_OPERATION  = 0xc0000090,
      
  MD_EXCEPTION_CODE_WIN_FLOAT_OVERFLOW           = 0xc0000091,
      
  MD_EXCEPTION_CODE_WIN_FLOAT_STACK_CHECK        = 0xc0000092,
      
  MD_EXCEPTION_CODE_WIN_FLOAT_UNDERFLOW          = 0xc0000093,
      
  MD_EXCEPTION_CODE_WIN_INTEGER_DIVIDE_BY_ZERO   = 0xc0000094,
      
  MD_EXCEPTION_CODE_WIN_INTEGER_OVERFLOW         = 0xc0000095,
      
  MD_EXCEPTION_CODE_WIN_PRIVILEGED_INSTRUCTION   = 0xc0000096,
      
  MD_EXCEPTION_CODE_WIN_STACK_OVERFLOW           = 0xc00000fd,
      
  MD_EXCEPTION_CODE_WIN_POSSIBLE_DEADLOCK        = 0xc0000194
      
} MDExceptionCodeWin;




typedef enum {
  

  MD_EXCEPTION_MAC_BAD_ACCESS      = 1,  
      
  MD_EXCEPTION_MAC_BAD_INSTRUCTION = 2,  
      
  MD_EXCEPTION_MAC_ARITHMETIC      = 3,  
      
  MD_EXCEPTION_MAC_EMULATION       = 4,  
      
  MD_EXCEPTION_MAC_SOFTWARE        = 5,
      
  MD_EXCEPTION_MAC_BREAKPOINT      = 6,  
      
  MD_EXCEPTION_MAC_SYSCALL         = 7,
      
  MD_EXCEPTION_MAC_MACH_SYSCALL    = 8,
      
  MD_EXCEPTION_MAC_RPC_ALERT       = 9
      
} MDExceptionMac;




typedef enum {
  

  MD_EXCEPTION_CODE_MAC_INVALID_ADDRESS    =  1,
      
  MD_EXCEPTION_CODE_MAC_PROTECTION_FAILURE =  2,
      
  MD_EXCEPTION_CODE_MAC_NO_ACCESS          =  8,
      
  MD_EXCEPTION_CODE_MAC_MEMORY_FAILURE     =  9,
      
  MD_EXCEPTION_CODE_MAC_MEMORY_ERROR       = 10,
      

  
  MD_EXCEPTION_CODE_MAC_BAD_SYSCALL = 0x00010000,  
  MD_EXCEPTION_CODE_MAC_BAD_PIPE    = 0x00010001,  
  MD_EXCEPTION_CODE_MAC_ABORT       = 0x00010002,  

  
  MD_EXCEPTION_CODE_MAC_PPC_VM_PROT_READ = 0x0101,
      
  MD_EXCEPTION_CODE_MAC_PPC_BADSPACE     = 0x0102,
      
  MD_EXCEPTION_CODE_MAC_PPC_UNALIGNED    = 0x0103,
      

  
  MD_EXCEPTION_CODE_MAC_PPC_INVALID_SYSCALL           = 1,
      
  MD_EXCEPTION_CODE_MAC_PPC_UNIMPLEMENTED_INSTRUCTION = 2,
      
  MD_EXCEPTION_CODE_MAC_PPC_PRIVILEGED_INSTRUCTION    = 3,
      
  MD_EXCEPTION_CODE_MAC_PPC_PRIVILEGED_REGISTER       = 4,
      
  MD_EXCEPTION_CODE_MAC_PPC_TRACE                     = 5,
      
  MD_EXCEPTION_CODE_MAC_PPC_PERFORMANCE_MONITOR       = 6,
      

  
  MD_EXCEPTION_CODE_MAC_PPC_OVERFLOW           = 1,
      
  MD_EXCEPTION_CODE_MAC_PPC_ZERO_DIVIDE        = 2,
      
  MD_EXCEPTION_CODE_MAC_PPC_FLOAT_INEXACT      = 3,
      
  MD_EXCEPTION_CODE_MAC_PPC_FLOAT_ZERO_DIVIDE  = 4,
      
  MD_EXCEPTION_CODE_MAC_PPC_FLOAT_UNDERFLOW    = 5,
      
  MD_EXCEPTION_CODE_MAC_PPC_FLOAT_OVERFLOW     = 6,
      
  MD_EXCEPTION_CODE_MAC_PPC_FLOAT_NOT_A_NUMBER = 7,
      

  
  MD_EXCEPTION_CODE_MAC_PPC_NO_EMULATION   = 8,
      
  MD_EXCEPTION_CODE_MAC_PPC_ALTIVEC_ASSIST = 9,
      

  
  MD_EXCEPTION_CODE_MAC_PPC_TRAP    = 0x00000001,  
  MD_EXCEPTION_CODE_MAC_PPC_MIGRATE = 0x00010100,  

  
  MD_EXCEPTION_CODE_MAC_PPC_BREAKPOINT = 1,  

  

  MD_EXCEPTION_CODE_MAC_X86_INVALID_OPERATION = 1,  

  
  MD_EXCEPTION_CODE_MAC_X86_DIV       = 1,  
  MD_EXCEPTION_CODE_MAC_X86_INTO      = 2,  
  MD_EXCEPTION_CODE_MAC_X86_NOEXT     = 3,  
  MD_EXCEPTION_CODE_MAC_X86_EXTOVR    = 4,  
  MD_EXCEPTION_CODE_MAC_X86_EXTERR    = 5,  
  MD_EXCEPTION_CODE_MAC_X86_EMERR     = 6,  
  MD_EXCEPTION_CODE_MAC_X86_BOUND     = 7,  
  MD_EXCEPTION_CODE_MAC_X86_SSEEXTERR = 8,  

  
  MD_EXCEPTION_CODE_MAC_X86_SGL = 1,  
  MD_EXCEPTION_CODE_MAC_X86_BPT = 2,  

  



  
  
  
  
  
  
  
  
  
  
  MD_EXCEPTION_CODE_MAC_X86_INVALID_TASK_STATE_SEGMENT = 10,
      
  MD_EXCEPTION_CODE_MAC_X86_SEGMENT_NOT_PRESENT        = 11,
      
  MD_EXCEPTION_CODE_MAC_X86_STACK_FAULT                = 12,
      
  MD_EXCEPTION_CODE_MAC_X86_GENERAL_PROTECTION_FAULT   = 13,
      
  
  
  MD_EXCEPTION_CODE_MAC_X86_ALIGNMENT_FAULT            = 17
      
  
  
} MDExceptionCodeMac;



typedef enum {
  MD_EXCEPTION_CODE_LIN_SIGHUP = 1,      
  MD_EXCEPTION_CODE_LIN_SIGINT = 2,      
  MD_EXCEPTION_CODE_LIN_SIGQUIT = 3,     
  MD_EXCEPTION_CODE_LIN_SIGILL = 4,      
  MD_EXCEPTION_CODE_LIN_SIGTRAP = 5,     
  MD_EXCEPTION_CODE_LIN_SIGABRT = 6,     
  MD_EXCEPTION_CODE_LIN_SIGBUS = 7,      
  MD_EXCEPTION_CODE_LIN_SIGFPE = 8,      
  MD_EXCEPTION_CODE_LIN_SIGKILL = 9,     
  MD_EXCEPTION_CODE_LIN_SIGUSR1 = 10,    
  MD_EXCEPTION_CODE_LIN_SIGSEGV = 11,    
  MD_EXCEPTION_CODE_LIN_SIGUSR2 = 12,    
  MD_EXCEPTION_CODE_LIN_SIGPIPE = 13,    
  MD_EXCEPTION_CODE_LIN_SIGALRM = 14,    
  MD_EXCEPTION_CODE_LIN_SIGTERM = 15,    
  MD_EXCEPTION_CODE_LIN_SIGSTKFLT = 16,  
  MD_EXCEPTION_CODE_LIN_SIGCHLD = 17,    
  MD_EXCEPTION_CODE_LIN_SIGCONT = 18,    
  MD_EXCEPTION_CODE_LIN_SIGSTOP = 19,    
  MD_EXCEPTION_CODE_LIN_SIGTSTP = 20,    
  MD_EXCEPTION_CODE_LIN_SIGTTIN = 21,    
  MD_EXCEPTION_CODE_LIN_SIGTTOU = 22,    
  MD_EXCEPTION_CODE_LIN_SIGURG = 23,
    
  MD_EXCEPTION_CODE_LIN_SIGXCPU = 24,    
  MD_EXCEPTION_CODE_LIN_SIGXFSZ = 25,
    
  MD_EXCEPTION_CODE_LIN_SIGVTALRM = 26,  
  MD_EXCEPTION_CODE_LIN_SIGPROF = 27,    
  MD_EXCEPTION_CODE_LIN_SIGWINCH = 28,   
  MD_EXCEPTION_CODE_LIN_SIGIO = 29,      
  MD_EXCEPTION_CODE_LIN_SIGPWR = 30,     
  MD_EXCEPTION_CODE_LIN_SIGSYS = 31      
} MDExceptionCodeLinux;

typedef struct {
  u_int32_t            thread_id;         


  u_int32_t            __align;
  MDException          exception_record;
  MDLocationDescriptor thread_context;    
} MDRawExceptionStream;  


typedef union {
  struct {
    u_int32_t vendor_id[3];               
    u_int32_t version_information;        
    u_int32_t feature_information;        
    u_int32_t amd_extended_cpu_features;  
  } x86_cpu_info;
  struct {
    u_int64_t processor_features[2];
  } other_cpu_info;
} MDCPUInformation;  


typedef struct {
  

  u_int16_t        processor_architecture;
  u_int16_t        processor_level;         
  u_int16_t        processor_revision;      


  u_int8_t         number_of_processors;
  u_int8_t         product_type;            

  

  u_int32_t        major_version;
  u_int32_t        minor_version;
  u_int32_t        build_number;
  u_int32_t        platform_id;
  MDRVA            csd_version_rva;  







  u_int16_t        suite_mask;       
  u_int16_t        reserved2;

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
      
  MD_CPU_ARCHITECTURE_UNKNOWN   = 0xffff  
} MDCPUArchitecture;


typedef enum {
  MD_OS_WIN32S        = 0,  
  MD_OS_WIN32_WINDOWS = 1,  
  MD_OS_WIN32_NT      = 2,  
  MD_OS_WIN32_CE      = 3,  


  
  MD_OS_UNIX          = 0x8000,  
  MD_OS_MAC_OS_X      = 0x8101,  
  MD_OS_LINUX         = 0x8201   
} MDOSPlatform;


typedef struct {
  u_int32_t size_of_info;  
  u_int32_t flags1;

  

  u_int32_t process_id;

  

  u_int32_t process_create_time;  
  u_int32_t process_user_time;    
  u_int32_t process_kernel_time;  

  




  u_int32_t processor_max_mhz;
  u_int32_t processor_current_mhz;
  u_int32_t processor_mhz_limit;
  u_int32_t processor_max_idle_state;
  u_int32_t processor_current_idle_state;
} MDRawMiscInfo;  

#define MD_MISCINFO_SIZE 24
#define MD_MISCINFO2_SIZE 44



typedef enum {
  MD_MISCINFO_FLAGS1_PROCESS_ID           = 0x00000001,
      
  MD_MISCINFO_FLAGS1_PROCESS_TIMES        = 0x00000002,
      
  MD_MISCINFO_FLAGS1_PROCESSOR_POWER_INFO = 0x00000004
      
} MDMiscInfoFlags1;







typedef struct {
  

  u_int32_t validity;

  





  u_int32_t dump_thread_id;

  










  u_int32_t requesting_thread_id;
} MDRawBreakpadInfo;


typedef enum {
  
  MD_BREAKPAD_INFO_VALID_DUMP_THREAD_ID       = 1 << 0,

  
  MD_BREAKPAD_INFO_VALID_REQUESTING_THREAD_ID = 1 << 1
} MDBreakpadInfoValidity;

typedef struct {
  




  u_int16_t expression[128];  
  u_int16_t function[128];    
  u_int16_t file[128];        
  u_int32_t line;             
  u_int32_t type;
} MDRawAssertionInfo;


typedef enum {
  MD_ASSERTION_INFO_TYPE_UNKNOWN = 0,

  

  MD_ASSERTION_INFO_TYPE_INVALID_PARAMETER,

  

  MD_ASSERTION_INFO_TYPE_PURE_VIRTUAL_CALL
} MDAssertionInfoData;

#if defined(_MSC_VER)
#pragma warning(pop)
#endif  


#endif  
