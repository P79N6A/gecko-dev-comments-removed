



#include "cpr_errno.h"
#include <errno.h>







int8_t errno_table[] =
{
    CPR_EPERM,
    CPR_ENOENT,
    CPR_ESRCH,
    CPR_EINTR,
    CPR_EIO,
    CPR_ENXIO,
    CPR_E2BIG,
    CPR_ENOEXEC,
    CPR_EBADF,
    CPR_ECHILD,
    CPR_EDEADLK, 
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
    CPR_EAGAIN,
    CPR_EINPROGRESS,
    CPR_EALREADY,
    CPR_ENOTSOCK,
    CPR_EDESTADDRREQ,
    CPR_EMSGSIZE,
    CPR_EPROTOTYPE,
    CPR_ENOPROTOOPT,
    CPR_EPROTONOSUPPORT,
    CPR_ESOCKTNOSUPPORT,
    CPR_ENOTSUP,
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
    CPR_ELOOP,
    CPR_ENAMETOOLONG,
    CPR_EHOSTDOWN,
    CPR_EHOSTUNREACH,
    CPR_ENOTEMPTY,
    CPR_UNKNOWN_ERR,            
    CPR_EUSERS,
    CPR_EDQUOT,
    CPR_ESTALE,
    CPR_EREMOTE,
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_ENOLCK,
    CPR_ENOSYS,
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_EOVERFLOW,
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_UNKNOWN_ERR,            
    CPR_ECANCELED,
    CPR_EIDRM,
    CPR_ENOMSG,
    CPR_EILSEQ,
    CPR_UNKNOWN_ERR,            
    CPR_EBADMSG,
    CPR_EMULTIHOP,
    CPR_ENODATA,
    CPR_ENOLINK,
    CPR_ENOSR,
    CPR_ENOSTR,
    CPR_EPROTO,
    CPR_ETIME,
    CPR_EOPNOTSUPP,
    CPR_UNKNOWN_ERR
}; 












int16_t
cprTranslateErrno (void)
{
    int16_t e = (int16_t) errno;

    


    if ( e < 1 || e > (int16_t) (sizeof(errno_table)/sizeof(errno_table[0])) )
	{
        return CPR_UNKNOWN_ERR;
    }
    return (int16_t) errno_table[e - 1];
}

