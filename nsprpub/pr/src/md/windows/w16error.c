







































 
#include "prerror.h"
#include <errno.h>
#include <winsock.h>


void _PR_MD_map_error( int err )
{

    switch ( err )
    {
        case  ENOENT:   
			PR_SetError(PR_FILE_NOT_FOUND_ERROR, err);
            break;
        case  E2BIG:    
            PR_SetError( PR_INVALID_ARGUMENT_ERROR, err );
            break;
        case  ENOEXEC:  
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EBADF:    
            PR_SetError( PR_BAD_DESCRIPTOR_ERROR, err );
            break;
        case  ENOMEM:   
            PR_SetError( PR_OUT_OF_MEMORY_ERROR, err );
            break;
        case  EACCES:   
            PR_SetError( PR_NO_ACCESS_RIGHTS_ERROR, err );
            break;
        case  EEXIST:   
        
 
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EXDEV:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EINVAL:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENFILE:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EMFILE:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENOSPC:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        
        case  EDOM:     
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ERANGE:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        
        case  EDEADLK:      
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EINTR:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ECHILD:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        
        case  EAGAIN:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EBUSY:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EFBIG:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EIO:      
            PR_SetError( PR_IO_ERROR, err );
            break;
        case  EISDIR:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENOTDIR:  
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EMLINK:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENOTBLK:  
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENOTTY:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENXIO:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EPERM:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EPIPE:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EROFS:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ESPIPE:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ESRCH:    
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ETXTBSY:  
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  EFAULT:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENAMETOOLONG: 
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENODEV:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENOLCK:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENOSYS:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
        case  ENOTEMPTY:    
        
        case  EILSEQ:   
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
            
        
        case WSAEACCES:
            PR_SetError(PR_NO_ACCESS_RIGHTS_ERROR, err);
            break;
        case WSAEADDRINUSE:
            PR_SetError(PR_ADDRESS_IN_USE_ERROR, err);
            break;
        case WSAEADDRNOTAVAIL:
            PR_SetError(PR_ADDRESS_NOT_AVAILABLE_ERROR, err);
            break;
        case WSAEAFNOSUPPORT:
            PR_SetError(PR_ADDRESS_NOT_SUPPORTED_ERROR, err);
            break;
        case WSAEBADF:
            PR_SetError(PR_BAD_DESCRIPTOR_ERROR, err);
            break;
        case WSAECONNREFUSED:
            PR_SetError(PR_CONNECT_REFUSED_ERROR, err);
            break;
        case WSAEFAULT:
            PR_SetError(PR_ACCESS_FAULT_ERROR, err);
            break;
        case WSAEINVAL:
            PR_SetError(PR_BUFFER_OVERFLOW_ERROR, err);
            break;
        case WSAEISCONN:
            PR_SetError(PR_IS_CONNECTED_ERROR, err);
            break;
        case WSAEMFILE:
            PR_SetError(PR_PROC_DESC_TABLE_FULL_ERROR, err);
            break;
        case WSAENETDOWN:
        case WSAENETUNREACH:
            PR_SetError(PR_NETWORK_UNREACHABLE_ERROR, err);
            break;
        case WSAENOBUFS:
            PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, err);
            break;
        case WSAENOPROTOOPT:
        case WSAEMSGSIZE:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, err);
            break;
        case WSAENOTCONN:
            PR_SetError(PR_NOT_CONNECTED_ERROR, err);
            break;
        case WSAENOTSOCK:
            PR_SetError(PR_NOT_SOCKET_ERROR, err);
            break;
        case WSAEOPNOTSUPP:
            PR_SetError(PR_NOT_TCP_SOCKET_ERROR, err);
            break;
        case WSAEPROTONOSUPPORT:
            PR_SetError(PR_PROTOCOL_NOT_SUPPORTED_ERROR, err);
            break;
        case WSAETIMEDOUT:
            PR_SetError(PR_IO_TIMEOUT_ERROR, err);
            break;
        case WSAEINTR:
            PR_SetError(PR_PENDING_INTERRUPT_ERROR, err );
            break;
        case WSASYSNOTREADY:
        case WSAVERNOTSUPPORTED:
            PR_SetError(PR_PROTOCOL_NOT_SUPPORTED_ERROR, err);
            break;
		case WSAEWOULDBLOCK:
			PR_SetError(PR_WOULD_BLOCK_ERROR, err);
			break;
            
        default:
            PR_SetError( PR_UNKNOWN_ERROR, err );
            break;
    }
    return;
} 


