



#ifndef _CPR_ERRNO_H_
#define _CPR_ERRNO_H_

#include "cpr_types.h"

__BEGIN_DECLS

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_errno.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_errno.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_errno.h"
#endif





typedef enum
{
    
    CPR_EPERM = 1,      
    CPR_ENOENT,         
    CPR_ESRCH,          
    CPR_EINTR,          
     
    CPR_EIO,            
     
    CPR_ENXIO,          
    CPR_E2BIG,          
    CPR_ENOEXEC,        
    CPR_EBADF,          
    CPR_ECHILD,         
    CPR_EAGAIN,         
     
    CPR_EWOULDBLOCK = CPR_EAGAIN,
     
    CPR_ENOMEM,         
     
    CPR_EACCES,         
     
    CPR_EFAULT,         
     
    CPR_ENOTBLK,        
    CPR_EBUSY,          
    CPR_EEXIST,         
    CPR_EXDEV,          
    CPR_ENODEV,         
    CPR_ENOTDIR,        
    CPR_EISDIR,         
    CPR_EINVAL,         
     
     
    CPR_ENFILE,         
    CPR_EMFILE,         
     
    CPR_ENOTTY,         
    CPR_ETXTBSY,        
    CPR_EFBIG,          
    CPR_ENOSPC,         
    CPR_ESPIPE,         
    CPR_EROFS,          
    CPR_EMLINK,         
    CPR_EPIPE,          
    CPR_EDOM,           
    CPR_ERANGE,         

    
    CPR_ENOTSOCK,       
    CPR_EDESTADDRREQ,   
    CPR_EMSGSIZE,       
    CPR_EPROTOTYPE,     
    CPR_ENOPROTOOPT,    
    CPR_EPROTONOSUPPORT,
    CPR_ESOCKTNOSUPPORT,
    CPR_EOPNOTSUPP,     
    CPR_EPFNOSUPPORT,   
    CPR_EAFNOSUPPORT,   
    CPR_EADDRINUSE,     
    CPR_EADDRNOTAVAIL,  
    CPR_ENETDOWN,       
    CPR_ENETUNREACH,    
    CPR_ENETRESET,      
    CPR_ECONNABORTED,   
    CPR_ECONNRESET,     
    CPR_ENOBUFS,        
    CPR_EISCONN,        
    CPR_ENOTCONN,       
    CPR_ESHUTDOWN,      
    CPR_ETOOMANYREFS,   
    CPR_ETIMEDOUT,      
    CPR_ECONNREFUSED,   
    CPR_EHOSTDOWN,      
    CPR_EHOSTUNREACH,   
    CPR_EALREADY,       
    CPR_EINPROGRESS,    

    
    

    CPR_ENOMSG,         
    CPR_EIDRM,          
    CPR_ECHRNG,         
    CPR_EL2NSYNC,       
    CPR_EL3HLT,         
    CPR_EL3RST,         
    CPR_ELNRNG,         
    CPR_EUNATCH,        
    CPR_ENOCSI,         
    CPR_EL2HLT,         
    CPR_ENOLCK,         
    CPR_EDEADLK,        
    CPR_ECANCELED,      
    CPR_ENOTSUP,        
    CPR_EDQUOT,         

    
    CPR_EBADE,          
    CPR_EBADR,          
    CPR_EXFULL,         
    CPR_ENOANO,         
    CPR_EBADRQC,        
    CPR_EBADSLT,        
    CPR_EDEADLOCK,      
    CPR_EBFONT,         

    
    CPR_ENOSTR,         
    CPR_ENODATA,        
    CPR_ETIME,          
    CPR_ENOSR,          
    CPR_ENONET,         
    CPR_ENOPKG,         
    CPR_EREMOTE,        
    CPR_ENOLINK,        
    CPR_EADV,           
    CPR_ESRMNT,         
    CPR_ECOMM,          
    CPR_EPROTO,         
    CPR_EMULTIHOP,      
    CPR_EBADMSG,        
    CPR_ENAMETOOLONG,   
    CPR_EOVERFLOW,      
    CPR_ENOTUNIQ,       
    CPR_EBADFD,         
    CPR_EREMCHG,        

    
    CPR_ELIBACC,        
    CPR_ELIBBAD,        
    CPR_ELIBSCN,        
    CPR_ELIBMAX,        
    CPR_ELIBEXEC,       
    CPR_EILSEQ,         
    CPR_ENOSYS,         
    CPR_ELOOP,          
    CPR_ERESTART,       
    CPR_ESTRPIPE,       
    CPR_ENOTEMPTY,      
    CPR_EUSERS,         

    CPR_ESTALE,         

    
    CPR_ECLOSED,        


    CPR_EINIT,

   CPR_UNKNOWN_ERR, 
   CPR_ERRNO_MAX
} cpr_errno_e;

__END_DECLS

#endif
