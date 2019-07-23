






















#ifndef GSSAPI_H_
#define GSSAPI_H_





#define _GSSAPI_H_






#define _GSSAPI_GENERIC_H_
#define _GSSAPI_KRB5_H_





#ifndef GSS_CALLCONV
#if defined(_WIN32)
#define GSS_CALLCONV __stdcall
#define GSS_CALLCONV_C __cdecl
#else
#define GSS_CALLCONV 
#define GSS_CALLCONV_C
#endif
#endif 

#ifdef GSS_USE_FUNCTION_POINTERS
#ifdef _WIN32
#undef GSS_CALLCONV
#define GSS_CALLCONV
#define GSS_FUNC(f) (__stdcall *f##_type)
#else
#define GSS_FUNC(f) (*f##_type)
#endif
#define GSS_MAKE_TYPEDEF typedef
#else
#define GSS_FUNC(f) f
#define GSS_MAKE_TYPEDEF
#endif




#include <stddef.h>





#ifndef SIZEOF_LONG
#undef SIZEOF_LONG 
#endif
#ifndef SIZEOF_SHORT
#undef SIZEOF_SHORT
#endif

#ifndef EXTERN_C_BEGIN
#ifdef __cplusplus
#define EXTERN_C_BEGIN extern "C" {
#define EXTERN_C_END }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif
#endif

EXTERN_C_BEGIN












typedef void * gss_name_t ;
typedef void * gss_ctx_id_t ;
typedef void * gss_cred_id_t ;
 







#if SIZEOF_LONG == 4
typedef unsigned long gss_uint32;
#elif SIZEOF_SHORT == 4
typedef unsigned short gss_uint32;
#else
typedef unsigned int gss_uint32;
#endif

#ifdef OM_STRING






#if sizeof(gss_uint32) != sizeof(OM_uint32)
#error Incompatible definition of OM_uint32 from xom.h
#endif

typedef OM_object_identifier gss_OID_desc, *gss_OID;

#else 




typedef gss_uint32 OM_uint32;
typedef struct gss_OID_desc_struct {
  OM_uint32 length;
  void *elements;
} gss_OID_desc, *gss_OID;

#endif 

typedef struct gss_OID_set_desc_struct  {
  size_t     count;
  gss_OID    elements;
} gss_OID_set_desc, *gss_OID_set;





typedef OM_uint32 gss_qop_t;

typedef int gss_cred_usage_t;


typedef struct gss_buffer_desc_struct {
  size_t length;
  void *value;
} gss_buffer_desc, *gss_buffer_t;

typedef struct gss_channel_bindings_struct {
  OM_uint32 initiator_addrtype;
  gss_buffer_desc initiator_address;
  OM_uint32 acceptor_addrtype;
  gss_buffer_desc acceptor_address;
  gss_buffer_desc application_data;
} *gss_channel_bindings_t;





#define GSS_C_DELEG_FLAG 1
#define GSS_C_MUTUAL_FLAG 2
#define GSS_C_REPLAY_FLAG 4
#define GSS_C_SEQUENCE_FLAG 8
#define GSS_C_CONF_FLAG 16
#define GSS_C_INTEG_FLAG 32
#define GSS_C_ANON_FLAG 64
#define GSS_C_PROT_READY_FLAG 128
#define GSS_C_TRANS_FLAG 256




#define GSS_C_BOTH 0
#define GSS_C_INITIATE 1
#define GSS_C_ACCEPT 2




#define GSS_C_GSS_CODE 1
#define GSS_C_MECH_CODE 2




#define GSS_C_AF_UNSPEC     0
#define GSS_C_AF_LOCAL      1
#define GSS_C_AF_INET       2
#define GSS_C_AF_IMPLINK    3
#define GSS_C_AF_PUP        4
#define GSS_C_AF_CHAOS      5
#define GSS_C_AF_NS         6
#define GSS_C_AF_NBS        7
#define GSS_C_AF_ECMA       8
#define GSS_C_AF_DATAKIT    9
#define GSS_C_AF_CCITT      10
#define GSS_C_AF_SNA        11
#define GSS_C_AF_DECnet     12
#define GSS_C_AF_DLI        13
#define GSS_C_AF_LAT        14
#define GSS_C_AF_HYLINK     15
#define GSS_C_AF_APPLETALK  16
#define GSS_C_AF_BSC        17
#define GSS_C_AF_DSS        18
#define GSS_C_AF_OSI        19
#define GSS_C_AF_X25        21

#define GSS_C_AF_NULLADDR   255




#define GSS_C_NO_NAME ((gss_name_t) 0)
#define GSS_C_NO_BUFFER ((gss_buffer_t) 0)
#define GSS_C_NO_OID ((gss_OID) 0)
#define GSS_C_NO_OID_SET ((gss_OID_set) 0)
#define GSS_C_NO_CONTEXT ((gss_ctx_id_t) 0)
#define GSS_C_NO_CREDENTIAL ((gss_cred_id_t) 0)
#define GSS_C_NO_CHANNEL_BINDINGS ((gss_channel_bindings_t) 0)
#define GSS_C_EMPTY_BUFFER {0, NULL}





#define GSS_C_NULL_OID GSS_C_NO_OID
#define GSS_C_NULL_OID_SET GSS_C_NO_OID_SET










#define GSS_C_QOP_DEFAULT 0





#define GSS_C_INDEFINITE 0xfffffffful












extern gss_OID GSS_C_NT_USER_NAME;












extern gss_OID GSS_C_NT_MACHINE_UID_NAME;












extern gss_OID GSS_C_NT_STRING_UID_NAME;


















extern gss_OID GSS_C_NT_HOSTBASED_SERVICE_X;












extern gss_OID GSS_C_NT_HOSTBASED_SERVICE;












extern gss_OID GSS_C_NT_ANONYMOUS;











extern gss_OID GSS_C_NT_EXPORT_NAME;



#define GSS_S_COMPLETE 0




#define GSS_C_CALLING_ERROR_OFFSET 24
#define GSS_C_ROUTINE_ERROR_OFFSET 16
#define GSS_C_SUPPLEMENTARY_OFFSET 0
#define GSS_C_CALLING_ERROR_MASK 0377ul
#define GSS_C_ROUTINE_ERROR_MASK 0377ul
#define GSS_C_SUPPLEMENTARY_MASK 0177777ul







#define GSS_CALLING_ERROR(x) \
(x & (GSS_C_CALLING_ERROR_MASK << GSS_C_CALLING_ERROR_OFFSET))
#define GSS_ROUTINE_ERROR(x) \
     (x & (GSS_C_ROUTINE_ERROR_MASK << GSS_C_ROUTINE_ERROR_OFFSET))
#define GSS_SUPPLEMENTARY_INFO(x) \
     (x & (GSS_C_SUPPLEMENTARY_MASK << GSS_C_SUPPLEMENTARY_OFFSET))
#define GSS_ERROR(x) \
     (x & ((GSS_C_CALLING_ERROR_MASK << GSS_C_CALLING_ERROR_OFFSET) | \
           (GSS_C_ROUTINE_ERROR_MASK << GSS_C_ROUTINE_ERROR_OFFSET)))








#define GSS_S_CALL_INACCESSIBLE_READ \
     (1ul << GSS_C_CALLING_ERROR_OFFSET)
#define GSS_S_CALL_INACCESSIBLE_WRITE \
     (2ul << GSS_C_CALLING_ERROR_OFFSET)
#define GSS_S_CALL_BAD_STRUCTURE \
     (3ul << GSS_C_CALLING_ERROR_OFFSET)




#define GSS_S_BAD_MECH (1ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_BAD_NAME (2ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_BAD_NAMETYPE (3ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_BAD_BINDINGS (4ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_BAD_STATUS (5ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_BAD_SIG (6ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_BAD_MIC GSS_S_BAD_SIG
#define GSS_S_NO_CRED (7ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_NO_CONTEXT (8ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_DEFECTIVE_TOKEN (9ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_DEFECTIVE_CREDENTIAL (10ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_CREDENTIALS_EXPIRED (11ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_CONTEXT_EXPIRED (12ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_FAILURE (13ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_BAD_QOP (14ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_UNAUTHORIZED (15ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_UNAVAILABLE (16ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_DUPLICATE_ELEMENT (17ul << GSS_C_ROUTINE_ERROR_OFFSET)
#define GSS_S_NAME_NOT_MN (18ul << GSS_C_ROUTINE_ERROR_OFFSET)




#define GSS_S_CONTINUE_NEEDED (1ul << (GSS_C_SUPPLEMENTARY_OFFSET + 0))
#define GSS_S_DUPLICATE_TOKEN (1ul << (GSS_C_SUPPLEMENTARY_OFFSET + 1))
#define GSS_S_OLD_TOKEN (1ul << (GSS_C_SUPPLEMENTARY_OFFSET + 2))
#define GSS_S_UNSEQ_TOKEN (1ul << (GSS_C_SUPPLEMENTARY_OFFSET + 3))
#define GSS_S_GAP_TOKEN (1ul << (GSS_C_SUPPLEMENTARY_OFFSET + 4))





GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_acquire_cred)
(OM_uint32 *,             
 const gss_name_t,        
 OM_uint32,               
 const gss_OID_set,       
 gss_cred_usage_t,        
 gss_cred_id_t *,         
 gss_OID_set *,           
 OM_uint32 *              
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_release_cred)
(OM_uint32 *,             
 gss_cred_id_t *          
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_init_sec_context)
(OM_uint32 *,             
 const gss_cred_id_t,     
 gss_ctx_id_t *,          
 const gss_name_t,        
 const gss_OID,           
 OM_uint32,               
 OM_uint32,               
 const gss_channel_bindings_t, 
 const gss_buffer_t,      
 gss_OID *,               
 gss_buffer_t,            
 OM_uint32 *,             
 OM_uint32 *              
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_accept_sec_context)
(OM_uint32 *,             
 gss_ctx_id_t *,          
 const gss_cred_id_t,     
 const gss_buffer_t,      
 const gss_channel_bindings_t, 
 gss_name_t *,            
 gss_OID *,               
 gss_buffer_t,            
 OM_uint32 *,             
 OM_uint32 *,             
 gss_cred_id_t *          
              );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_process_context_token)
(OM_uint32 *,             
 const gss_ctx_id_t,      
 const gss_buffer_t       
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_delete_sec_context)
(OM_uint32 *,             
 gss_ctx_id_t *,          
 gss_buffer_t             
 );

GSS_MAKE_TYPEDEF
OM_uint32
GSS_CALLCONV GSS_FUNC(gss_context_time)
(OM_uint32 *,             
 const gss_ctx_id_t,      
 OM_uint32 *              
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_get_mic)
(OM_uint32 *,             
 const gss_ctx_id_t,      
 gss_qop_t,               
 const gss_buffer_t,      
 gss_buffer_t             
 );


GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_verify_mic)
(OM_uint32 *,             
 const gss_ctx_id_t,      
 const gss_buffer_t,      
 const gss_buffer_t,      
 gss_qop_t *              
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_wrap)
(OM_uint32 *,             
 const gss_ctx_id_t,      
 int,                     
 gss_qop_t,               
 const gss_buffer_t,      
 int *,                   
 gss_buffer_t             
 );


GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_unwrap)
(OM_uint32 *,             
 const gss_ctx_id_t,      
 const gss_buffer_t,      
 gss_buffer_t,            
 int *,                   
 gss_qop_t *              
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_display_status)
(OM_uint32 *,             
 OM_uint32,               
 int,                     
 const gss_OID,           
 OM_uint32 *,             
 gss_buffer_t             
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_indicate_mechs)
(OM_uint32 *,             
 gss_OID_set *            
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_compare_name)
(OM_uint32 *,             
 const gss_name_t,        
 const gss_name_t,        
 int *                    
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_display_name)
(OM_uint32 *,             
 const gss_name_t,        
 gss_buffer_t,            
 gss_OID *                
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_import_name)
(OM_uint32 *,             
 const gss_buffer_t,      
 const gss_OID,           
 gss_name_t *             
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_export_name)
(OM_uint32  *,            
 const gss_name_t,        
 gss_buffer_t             
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_release_name)
(OM_uint32 *,             
 gss_name_t *             
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_release_buffer)
(OM_uint32 *,             
 gss_buffer_t             
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_release_oid_set)
(OM_uint32 *,             
 gss_OID_set *            
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_inquire_cred)
(OM_uint32 *,             
 const gss_cred_id_t,     
 gss_name_t *,            
 OM_uint32 *,             
 gss_cred_usage_t *,      
 gss_OID_set *            
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_inquire_context)
(OM_uint32 *,             
 const gss_ctx_id_t,      
 gss_name_t *,            
 gss_name_t *,            
 OM_uint32 *,             
 gss_OID *,               
 OM_uint32 *,             
 int *,                   
 int *                    
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_wrap_size_limit) 
(OM_uint32 *,             
 const gss_ctx_id_t,      
 int,                     
 gss_qop_t,               
 OM_uint32,               
 OM_uint32 *              
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_add_cred) 
(OM_uint32 *,             
 const gss_cred_id_t,     
 const gss_name_t,        
 const gss_OID,           
 gss_cred_usage_t,        
 OM_uint32,               
 OM_uint32,               
 gss_cred_id_t *,         
 gss_OID_set *,           
 OM_uint32 *,             
 OM_uint32 *              
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_inquire_cred_by_mech) 
(OM_uint32 *,             
 const gss_cred_id_t,     
 const gss_OID,           
 gss_name_t *,            
 OM_uint32 *,             
 OM_uint32 *,             
 gss_cred_usage_t *       
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_export_sec_context)
(OM_uint32 *,             
 gss_ctx_id_t *,          
 gss_buffer_t             
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_import_sec_context)
(OM_uint32 *,             
 const gss_buffer_t,      
 gss_ctx_id_t *           
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_create_empty_oid_set)
(OM_uint32 *,             
 gss_OID_set *            
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_add_oid_set_member)
(OM_uint32 *,             
 const gss_OID,           
 gss_OID_set *            
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_test_oid_set_member)
(OM_uint32 *,             
 const gss_OID,           
 const gss_OID_set,       
 int *                    
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_inquire_names_for_mech)
(OM_uint32 *,             
 const gss_OID,           
 gss_OID_set *            
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_inquire_mechs_for_name)
(OM_uint32 *,             
 const gss_name_t,        
 gss_OID_set *            
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_canonicalize_name)
(OM_uint32 *,             
 const gss_name_t,        
 const gss_OID,           
 gss_name_t *             
 );

GSS_MAKE_TYPEDEF
OM_uint32 
GSS_CALLCONV GSS_FUNC(gss_duplicate_name)
(OM_uint32 *,             
 const gss_name_t,        
 gss_name_t *             
 );

   










   GSS_MAKE_TYPEDEF
   OM_uint32 
   GSS_CALLCONV GSS_FUNC(gss_sign)
              (OM_uint32 *,        
               gss_ctx_id_t,       
               int,                
               gss_buffer_t,       
               gss_buffer_t        
              );


   GSS_MAKE_TYPEDEF
   OM_uint32 
   GSS_CALLCONV GSS_FUNC(gss_verify)
              (OM_uint32 *,        
               gss_ctx_id_t,       
               gss_buffer_t,       
               gss_buffer_t,       
               int *               
              );

   GSS_MAKE_TYPEDEF
   OM_uint32
   GSS_CALLCONV GSS_FUNC(gss_seal)
              (OM_uint32 *,        
               gss_ctx_id_t,       
               int,                
               int,                
               gss_buffer_t,       
               int *,              
               gss_buffer_t        
              );


   GSS_MAKE_TYPEDEF
   OM_uint32 
   GSS_CALLCONV GSS_FUNC(gss_unseal)
              (OM_uint32 *,        
               gss_ctx_id_t,       
               gss_buffer_t,       
               gss_buffer_t,       
               int *,              
               int *               
              );



EXTERN_C_END

#endif 

