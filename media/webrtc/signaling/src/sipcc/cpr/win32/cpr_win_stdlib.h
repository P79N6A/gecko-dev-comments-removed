






































#ifndef _CPR_WIN_STDLIB_H_
#define _CPR_WIN_STDLIB_H_


#include <stdlib.h>
#include <crtdbg.h>
#include "string.h"
#include "errno.h"







#define CPR_USE_DIRECT_OS_CALL
#define CPR_USE_CALLOC_FOR_MALLOC




#define ENOTBLK        15 /* Block device required                */
#define ETXTBSY        26 /* Text file busy                       */
#define ENOMSG         35 /* No message of desired type           */
#define ECHRNG         37 /* Channel number out of range          */
#define EIDRM          43 /* Identifier removed                   */
#define EL2NSYNC       44 /* Level 2 not synchronized             */
#define EL3HLT         45 /* Level 3 halted                       */
#define EL3RST         46 /* Level 3 reset                        */
#define ELNRNG         47 /* Link number out of range             */
#define EUNATCH        48 /* Protocol driver not attached         */
#define ENOCSI         49 /* No CSI structure available           */
#define EL2HLT         50 /* Level 2 halted                       */
#define ECANCELED      51 /* Operation canceled                   */
#define ENOTSUP        52 /* Operation not supported              */
#define EBADE          53 /* Invalid exchange                     */
#define EBADR          54 /* Invalid request descriptor           */
#define EXFULL         55 /* Exchange full                        */
#define ENOANO         56 /* No anode                             */
#define EBADRQC        57 /* Invalid request code                 */
#define EBADSLT        58 /* Invalid slot                         */
#define EBFONT         59 /* Bad font file fmt                    */
#define EBADMSG        60 /* Trying to read unreadable message    */
#define EOVERFLOW      61 /* Value too large for defined data type*/
#define ENOSTR         62 /* Device not a stream                  */
#define ENODATA        63 /* No data (for no delay io)            */
#define ETIME          64 /* Timer expired                        */
#define ENOSR          65 /* Out of streams resources             */
#define ENONET         66 /* Machine is not on the network        */
#define ENOPKG         67 /* Package not installed                */
#define EREMOTE        68 /* The object is remote                 */
#define ENOLINK        69 /* The link has been severed            */
#define EADV           70 /* Advertise error                      */
#define ESRMNT         71 /* Srmount error                        */
#define ECOMM          72 /* Communication error on send          */
#define EPROTO         73 /* Protocol error                       */
#define EREMCHG        74 /* Remote address changed               */
#define EBADFD         75 /* fd invalid for this operation        */
#define ENOTUNIQ       76 /* given log. name not unique           */
#define EMULTIHOP      77 /* multihop attempted                   */
#define ELIBACC        78 /* Can't access a needed shared lib.    */
#define ELIBBAD        79 /* Accessing a corrupted shared lib.    */
#define ELIBSCN        80 /* .lib section in a.out corrupted.     */
#define ELIBMAX        81 /* Attempting to link in too many libs. */
#define ELIBEXEC       82 /* Attempting to exec a shared library. */
#define ELOOP          83 /* Symbolic link loop                   */
#define ERESTART       84 /* Restartable system call              */
#define ESTRPIPE       85 /* if FIFO, don't sleep in stream head  */
#define EUSERS         86 /* Too many users (for UFS)             */
#define ENOTSOCK       87 /* Socket operation on non-socket       */
#define EDESTADDRREQ   88 /* Destination address required         */
#define EMSGSIZE       89 /* Message too long                     */
#define EPROTOTYPE     90 /* Protocol wrong type for socket       */
#define ENOPROTOOPT    91 /* Protocol not available               */
#define EPROTONOSUPPORT 100     /* Protocol not supported */
#define ESOCKTNOSUPPORT 101     /* Socket type not supported */
#define EOPNOTSUPP      102     /* Operation not supported on socket */
#define EPFNOSUPPORT    103     /* Protocol family not supported */
#define EAFNOSUPPORT    104     /* Address family not supported by */
                                
#define EADDRINUSE      105     /* Address already in use */
#define EADDRNOTAVAIL   106     /* Can't assign requested address */
#define ENETDOWN        107     /* Network is down */
#define ENETUNREACH     108     /* Network is unreachable */
#define ENETRESET       109     /* Network dropped connection because */
                                
#define ECONNABORTED    110     /* Software caused connection abort */
#define ECONNRESET      111     /* Connection reset by peer */
#define ENOBUFS         112     /* No buffer space available */
#define EISCONN         113     /* Socket is already connected */
#define ENOTCONN        114     /* Socket is not connected */
#define ESHUTDOWN       115     /* Can't send after socket shutdown */
#define ETOOMANYREFS    116     /* Too many references: can't splice */
#define ETIMEDOUT       117     /* Connection timed out */
#define ECONNREFUSED    118     /* Connection refused */
#define EHOSTDOWN       119     /* Host is down */
#define EHOSTUNREACH    120     /* No route to host */
#define EWOULDBLOCK     EAGAIN
#define EALREADY        121     /* operation already in progress */
#define EINPROGRESS     122     /* operation now in progress */
#define EDQUOT          123     /* Disc quota exceeded   */
#define ESTALE          124     /* Stale NFS file handle */

#endif

