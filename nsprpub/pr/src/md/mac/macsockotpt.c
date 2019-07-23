





































#define OTUNIXERRORS 1

#include <string.h>

#include <Gestalt.h>
#include <Files.h>
#include <OpenTransport.h>
#include <OSUtils.h>

#define GESTALT_OPEN_TPT_PRESENT        gestaltOpenTptPresentMask
#define GESTALT_OPEN_TPT_TCP_PRESENT    gestaltOpenTptTCPPresentMask

#include <OpenTptInternet.h>    

#if (UNIVERSAL_INTERFACES_VERSION >= 0x0330)

typedef struct OTConfiguration	OTConfiguration;
#endif

#include "primpl.h"

typedef enum SndRcvOpCode {
    kSTREAM_SEND,
    kSTREAM_RECEIVE,
    kDGRAM_SEND,
    kDGRAM_RECEIVE
} SndRcvOpCode;

static struct {
	PRLock *    lock;
	InetSvcRef  serviceRef;
	PRThread *  thread;
	void *      cookie;
} dnsContext;


static pascal void  DNSNotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie);
static pascal void  NotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie);
static pascal void  RawEndpointNotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie);

static PRBool GetState(PRFileDesc *fd, PRBool *readReady, PRBool *writeReady, PRBool *exceptReady);

void
WakeUpNotifiedThread(PRThread *thread, OTResult result);

extern void WaitOnThisThread(PRThread *thread, PRIntervalTime timeout);
extern void DoneWaitingOnThisThread(PRThread *thread);

#if TARGET_CARBON
OTClientContextPtr  clientContext = NULL;

#define INIT_OPEN_TRANSPORT()	InitOpenTransportInContext(kInitOTForExtensionMask, &clientContext)
#define OT_OPEN_INTERNET_SERVICES(config, flags, err)	OTOpenInternetServicesInContext(config, flags, err, clientContext)
#define OT_OPEN_ENDPOINT(config, flags, info, err)		OTOpenEndpointInContext(config, flags, info, err, clientContext)

#else

#define INIT_OPEN_TRANSPORT()	InitOpenTransport()
#define OT_OPEN_INTERNET_SERVICES(config, flags, err)	OTOpenInternetServices(config, flags, err)
#define OT_OPEN_ENDPOINT(config, flags, info, err)		OTOpenEndpoint(config, flags, info, err)
#endif 

static OTNotifyUPP	DNSNotifierRoutineUPP;
static OTNotifyUPP NotifierRoutineUPP;
static OTNotifyUPP RawEndpointNotifierRoutineUPP;

void _MD_InitNetAccess()
{
    OSErr       err;
    OSStatus    errOT;
    PRBool      hasOTTCPIP = PR_FALSE;
    PRBool      hasOT = PR_FALSE;
    long        gestaltResult;

    err = Gestalt(gestaltOpenTpt, &gestaltResult);
    if (err == noErr)
        if (gestaltResult & GESTALT_OPEN_TPT_PRESENT)
            hasOT = PR_TRUE;
    
    if (hasOT)
        if (gestaltResult & GESTALT_OPEN_TPT_TCP_PRESENT)
            hasOTTCPIP = PR_TRUE;
        
    PR_ASSERT(hasOTTCPIP == PR_TRUE);

    DNSNotifierRoutineUPP	=  NewOTNotifyUPP(DNSNotifierRoutine);
    NotifierRoutineUPP		=  NewOTNotifyUPP(NotifierRoutine);
    RawEndpointNotifierRoutineUPP = NewOTNotifyUPP(RawEndpointNotifierRoutine);

    errOT = INIT_OPEN_TRANSPORT();
    PR_ASSERT(err == kOTNoError);

	dnsContext.serviceRef = NULL;
	dnsContext.lock = PR_NewLock();
	PR_ASSERT(dnsContext.lock != NULL);

	dnsContext.thread = _PR_MD_CURRENT_THREAD();
	dnsContext.cookie = NULL;
	

}

static void _MD_FinishInitNetAccess()
{
    OSStatus    errOT;

	if (dnsContext.serviceRef)
		return;
		
    dnsContext.serviceRef = OT_OPEN_INTERNET_SERVICES(kDefaultInternetServicesPath, NULL, &errOT);
    if (errOT != kOTNoError) {
        dnsContext.serviceRef = NULL;
        return;    
    }
    
    PR_ASSERT((dnsContext.serviceRef != NULL) && (errOT == kOTNoError));

    
    errOT = OTInstallNotifier(dnsContext.serviceRef, DNSNotifierRoutineUPP, &dnsContext);
    PR_ASSERT(errOT == kOTNoError);

    
    errOT = OTSetAsynchronous(dnsContext.serviceRef);
    PR_ASSERT(errOT == kOTNoError);
}


static pascal void  DNSNotifierRoutine(void * contextPtr, OTEventCode otEvent, OTResult result, void * cookie)
{
#pragma unused(contextPtr)
    _PRCPU *    cpu    = _PR_MD_CURRENT_CPU(); 
	OSStatus    errOT;

		dnsContext.thread->md.osErrCode = result;
		dnsContext.cookie = cookie;
	
	switch (otEvent) {
		case T_DNRSTRINGTOADDRCOMPLETE:
				if (_PR_MD_GET_INTSOFF()) {
					dnsContext.thread->md.missedIONotify = PR_TRUE;
					cpu->u.missed[cpu->where] |= _PR_MISSED_IO;
				} else {
					DoneWaitingOnThisThread(dnsContext.thread);
				}
				break;
		
        case kOTProviderWillClose:
                errOT = OTSetSynchronous(dnsContext.serviceRef);
                
		
        case kOTProviderIsClosed:
                errOT = OTCloseProvider((ProviderRef)dnsContext.serviceRef);
                dnsContext.serviceRef = nil;

				if (_PR_MD_GET_INTSOFF()) {
					dnsContext.thread->md.missedIONotify = PR_TRUE;
					cpu->u.missed[cpu->where] |= _PR_MISSED_IO;
				} else {
					DoneWaitingOnThisThread(dnsContext.thread);
				}
                break;

        default: 
	            PR_ASSERT(otEvent==NULL);
		
	}
	
	
	SignalIdleSemaphore();
}


static void macsock_map_error(OSStatus err)
{
    _PR_MD_CURRENT_THREAD()->md.osErrCode = err;

    if (IsEError(err) || (err >= EPERM && err <= ELASTERRNO)) {
    switch (IsEError(err) ? OSStatus2E(err) : err) {
        case EBADF:
            PR_SetError(PR_BAD_DESCRIPTOR_ERROR, err);
            break;
        case EADDRNOTAVAIL:
            PR_SetError(PR_ADDRESS_NOT_AVAILABLE_ERROR, err);
            break;
        case EINPROGRESS:
            PR_SetError(PR_IN_PROGRESS_ERROR, err);
            break;
        case EWOULDBLOCK:
        case EAGAIN:
            PR_SetError(PR_WOULD_BLOCK_ERROR, err);
            break;
        case ENOTSOCK:
            PR_SetError(PR_NOT_SOCKET_ERROR, err);
            break;
        case ETIMEDOUT:
            PR_SetError(PR_IO_TIMEOUT_ERROR, err);
            break;
        case ECONNREFUSED:
            PR_SetError(PR_CONNECT_REFUSED_ERROR, err);
            break;
        case ENETUNREACH:
            PR_SetError(PR_NETWORK_UNREACHABLE_ERROR, err);
            break;
        case EADDRINUSE:
            PR_SetError(PR_ADDRESS_IN_USE_ERROR, err);
            break;
        case EFAULT:
            PR_SetError(PR_ACCESS_FAULT_ERROR, err);
            break;
        case EINTR:
            PR_SetError(PR_PENDING_INTERRUPT_ERROR, err);
            break;
        case EINVAL:
            PR_SetError(PR_INVALID_ARGUMENT_ERROR, err);
            break;
        case EIO:
            PR_SetError(PR_IO_ERROR, err);
            break;
        case ENOENT:
            PR_SetError(PR_ADDRESS_NOT_SUPPORTED_ERROR, err);
            break;
        case ENXIO:
            PR_SetError(PR_IO_ERROR, err);
            break;
        case EPROTOTYPE:
            PR_SetError(PR_PROTOCOL_NOT_SUPPORTED_ERROR, err);
            break;
        case EOPNOTSUPP:
            PR_SetError(PR_OPERATION_NOT_SUPPORTED_ERROR, err);
            break;
        default:
            PR_SetError(PR_UNKNOWN_ERROR, err);
            break;
        }
    } else {
    PR_ASSERT(IsXTIError(err));
    switch (err) {
        case kOTNoDataErr:
        case kOTFlowErr:
            PR_SetError(PR_WOULD_BLOCK_ERROR, err);
            break;
        default:
            PR_SetError(PR_UNKNOWN_ERROR, err);
            break;
        }
    }
}

static void PrepareForAsyncCompletion(PRThread * thread, PRInt32 osfd)
{
    thread->io_pending       = PR_TRUE;
    thread->io_fd            = osfd;
    thread->md.osErrCode     = noErr;
}


void
WakeUpNotifiedThread(PRThread *thread, OTResult result)
{
    _PRCPU *      cpu      = _PR_MD_CURRENT_CPU(); 

	if (thread) {
		thread->md.osErrCode = result;
		if (_PR_MD_GET_INTSOFF()) {
			thread->md.missedIONotify = PR_TRUE;
			cpu->u.missed[cpu->where] |= _PR_MISSED_IO;
		} else {
			DoneWaitingOnThisThread(thread);
		}
	}
	
	SignalIdleSemaphore();
}






static pascal void  NotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie)
{
    PRFilePrivate *secret  = (PRFilePrivate *) contextPtr;
    _MDFileDesc * md       = &(secret->md);
    EndpointRef   endpoint = (EndpointRef)secret->md.osfd;
    PRThread *    readThread   = NULL;          
    PRThread *    writeThread  = NULL;
    OSStatus      err;
    OTResult      resultOT;
    TDiscon       discon;

    switch (code)
    {

        case T_LISTEN:        
            
            
            
            
            
            if (md->doListen) {
                readThread = secret->md.misc.thread;
                secret->md.misc.thread    = NULL;
                secret->md.misc.cookie    = cookie;
                break;
            } else {
                
                OTSndDisconnect(endpoint, NULL);
            }
            break;

        case T_CONNECT:      
            
            err = OTRcvConnect(endpoint, NULL);
            PR_ASSERT(err == kOTNoError);

            
            writeThread = secret->md.write.thread;
            secret->md.write.thread    = NULL;
            secret->md.write.cookie    = cookie;            
            break;

        case T_DATA:        
            
            secret->md.readReady = PR_TRUE;

            
            readThread = secret->md.read.thread;
            secret->md.read.thread    = NULL;
            secret->md.read.cookie    = cookie;
            break;

        case T_EXDATA:      
            PR_ASSERT(!"T_EXDATA Not implemented");
            return;

        case T_DISCONNECT:  
            discon.udata.len = 0;
            err = OTRcvDisconnect(endpoint, &discon);
            PR_ASSERT(err == kOTNoError);
            secret->md.exceptReady = PR_TRUE;       

            md->disconnectError = discon.reason;    

            
            result = -3199 - discon.reason; 
            if ((readThread = secret->md.read.thread) != NULL) {
                secret->md.read.thread    = NULL;
                secret->md.read.cookie    = cookie;
            }

            if ((writeThread = secret->md.write.thread) != NULL) {
                secret->md.write.thread    = NULL;
                secret->md.write.cookie    = cookie;
            }
            break;

        case T_ERROR:       
            PR_ASSERT(!"T_ERROR Not implemented");
            return;

        case T_UDERR:       
            (void) OTRcvUDErr((EndpointRef) cookie, NULL);
            break;

        case T_ORDREL:      
            err = OTRcvOrderlyDisconnect(endpoint);
            PR_ASSERT(err == kOTNoError);
            secret->md.readReady      = PR_TRUE;   
            
            secret->md.orderlyDisconnect = PR_TRUE;
            
            readThread = secret->md.read.thread;
            secret->md.read.thread    = NULL;
            secret->md.read.cookie    = cookie;
            break;		

        case T_GODATA:   
            secret->md.writeReady = PR_TRUE;
            resultOT = OTLook(endpoint);        
            PR_ASSERT(resultOT == T_GODATA);
            
            
            writeThread = secret->md.write.thread;
            secret->md.write.thread    = NULL;
            secret->md.write.cookie    = cookie;
            break;

        case T_GOEXDATA: 
            PR_ASSERT(!"T_GOEXDATA Not implemented");
            return;

        case T_REQUEST:  
            PR_ASSERT(!"T_REQUEST Not implemented");
            return;

        case T_REPLY:    
            PR_ASSERT(!"T_REPLY Not implemented");
            return;

        case T_PASSCON:  
            
            
            break;

        case T_RESET:    
            PR_ASSERT(!"T_RESET Not implemented");
            return;
            

        case T_BINDCOMPLETE:
        case T_UNBINDCOMPLETE:
        case T_ACCEPTCOMPLETE:
        case T_OPTMGMTCOMPLETE:
        case T_GETPROTADDRCOMPLETE:
            readThread = secret->md.misc.thread;
            secret->md.misc.thread    = NULL;
            secret->md.misc.cookie    = cookie;
            break;












        





        default:
            
            
            return;
    }

    if (readThread)
        WakeUpNotifiedThread(readThread, result);

    if (writeThread && (writeThread != readThread))
        WakeUpNotifiedThread(writeThread, result);
}


static OSErr CreateSocket(int type, EndpointRef *endpoint)
{
    OSStatus err;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    char *  configName;
    OTConfiguration *config;
    EndpointRef ep;

    
    

    switch (type){
        case SOCK_STREAM:   configName = kTCPName;  break;
        case SOCK_DGRAM:    configName = kUDPName;  break;
    }
    config = OTCreateConfiguration(configName);
    ep = OT_OPEN_ENDPOINT(config, 0, NULL, &err);
    if (err != kOTNoError)
        goto ErrorExit;

    *endpoint = ep;
    PR_ASSERT(*endpoint != NULL);

    return kOTNoError;

ErrorExit:
    return err;
}






PRInt32 _MD_socket(int domain, int type, int protocol)
{
    OSStatus    err;
    EndpointRef endpoint;
    
    _MD_FinishInitNetAccess();

    
    if (domain != AF_INET) {
        err = kEPROTONOSUPPORTErr;
        goto ErrorExit;
    }
    
    
    if ((type != SOCK_STREAM) && (type != SOCK_DGRAM)) {
        err = kEPROTONOSUPPORTErr;
        goto ErrorExit;
    }
    
    
    if (protocol == 0)  {
        if (type == SOCK_DGRAM)
            protocol = IPPROTO_UDP;
        else if (type == SOCK_STREAM)
            protocol = IPPROTO_TCP;
    }
    
    
    if ((type == SOCK_STREAM)  && (protocol != IPPROTO_TCP)) {
        err = kEPROTONOSUPPORTErr;
        goto ErrorExit;
    }
                
    
    if ((type == SOCK_DGRAM)  && (protocol != IPPROTO_UDP)) {
        err = kEPROTONOSUPPORTErr;
        goto ErrorExit;
    }
        
    
    err = CreateSocket(type, &endpoint);
    if (err != kOTNoError)
        goto ErrorExit;

    PR_ASSERT((PRInt32)endpoint != -1);

    return ((PRInt32)endpoint);

ErrorExit:
    macsock_map_error(err);
    return -1;
}





PRInt32 _MD_bind(PRFileDesc *fd, PRNetAddr *addr, PRUint32 addrlen)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    TBind bindReq;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRUint32 retryCount = 0;

    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
        
    if (addr == NULL) {
        err = kEFAULTErr;
        goto ErrorExit;
    }
        






TryAgain:
    
    bindReq.addr.len = addrlen;
        
    bindReq.addr.maxlen = addrlen;
    bindReq.addr.buf = (UInt8*) addr;
    bindReq.qlen = 1;

	PR_Lock(fd->secret->md.miscLock);
    PrepareForAsyncCompletion(me, fd->secret->md.osfd);
	fd->secret->md.misc.thread = me;

    err = OTBind(endpoint, &bindReq, NULL);
    if (err != kOTNoError) {
	    me->io_pending = PR_FALSE;
	    PR_Unlock(fd->secret->md.miscLock);
        goto ErrorExit;
	}

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
	PR_Unlock(fd->secret->md.miscLock);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    return kOTNoError;

ErrorExit:
    if ((err == kOTNoAddressErr) && (++retryCount <= 4)) {
        unsigned long finalTicks;
    
        Delay(100,&finalTicks);
        goto TryAgain;
    }
    macsock_map_error(err);
    return -1;
}




PRInt32 _MD_listen(PRFileDesc *fd, PRIntn backlog)
{
    PRInt32 osfd = fd->secret->md.osfd;
    OSStatus err = 0;
    EndpointRef endpoint = (EndpointRef) osfd;
    TBind bindReq;
    PRNetAddr addr;
    PRThread *me = _PR_MD_CURRENT_THREAD();

	if ((fd == NULL) || (endpoint == NULL)) {
		err = EBADF;
		goto ErrorExit;
	}

    if (backlog == 0)
        backlog = 1;

    if (endpoint == NULL) {
        err = EBADF;
        goto ErrorExit;
    }
        
    addr.inet.family = AF_INET;
    addr.inet.port = addr.inet.ip = 0;

    bindReq.addr.maxlen = PR_NETADDR_SIZE (&addr);
    bindReq.addr.len = 0;
    bindReq.addr.buf = (UInt8*) &addr;
    bindReq.qlen = 0;
    
    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
	fd->secret->md.misc.thread = me; 

    err = OTGetProtAddress(endpoint, &bindReq, NULL);
    if (err != kOTNoError)
        goto ErrorExit;

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
	fd->secret->md.misc.thread = me; 

    err = OTUnbind(endpoint);
    if (err != kOTNoError)
        goto ErrorExit;

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

	
	fd->secret->md.doListen = PR_TRUE;
	
    bindReq.qlen = backlog;
    
    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
	fd->secret->md.misc.thread = me; 

    err = OTBind(endpoint, &bindReq, NULL);
    if (err != kOTNoError)
        goto ErrorExit;

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);

    err = me->md.osErrCode;
    if (err != kOTNoError)
    {
    	
		fd->secret->md.doListen = PR_FALSE;
        goto ErrorExit;
    }

    return kOTNoError;

ErrorExit:
	me->io_pending = PR_FALSE; 
    macsock_map_error(err);
    return -1;
}




PRInt32 _MD_getsockname(PRFileDesc *fd, PRNetAddr *addr, PRUint32 *addrlen)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    TBind bindReq;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
        
    if (addr == NULL) {
        err = kEFAULTErr;
        goto ErrorExit;
    }

    bindReq.addr.len = *addrlen;
    bindReq.addr.maxlen = *addrlen;
    bindReq.addr.buf = (UInt8*) addr;
    bindReq.qlen = 0;
    
	PR_Lock(fd->secret->md.miscLock);
    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
	fd->secret->md.misc.thread = me;

    err = OTGetProtAddress(endpoint, &bindReq, NULL);
    if (err != kOTNoError) {
	    me->io_pending = PR_FALSE;
	    PR_Unlock(fd->secret->md.miscLock);
        goto ErrorExit;
	}

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
	PR_Unlock(fd->secret->md.miscLock);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    *addrlen = PR_NETADDR_SIZE(addr);
    return kOTNoError;

ErrorExit:
    macsock_map_error(err);
    return -1;
}


PRStatus _MD_getsockopt(PRFileDesc *fd, PRInt32 level, PRInt32 optname, char* optval, PRInt32* optlen)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    TOptMgmt cmd;
    TOption *opt;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    unsigned char optionBuffer[kOTOptionHeaderSize + sizeof(PRSocketOptionData)];
    
    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
    
    




    if (level == SOL_SOCKET) {
        if (optname == SO_REUSEADDR)
            level = IPPROTO_IP;
        else if (optname == SO_KEEPALIVE)
            level = INET_TCP;
    }

    opt = (TOption *)&optionBuffer[0];
    opt->len = sizeof(TOption);
    opt->level = level;
    opt->name = optname;
    opt->status = 0;
    
    cmd.opt.len = sizeof(TOption);
    cmd.opt.maxlen = sizeof(optionBuffer);
    cmd.opt.buf = (UInt8*)optionBuffer;
    cmd.flags = T_CURRENT;

	PR_Lock(fd->secret->md.miscLock);
    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
	fd->secret->md.misc.thread = me;

    err = OTOptionManagement(endpoint, &cmd, &cmd);
    if (err != kOTNoError) {
	    me->io_pending = PR_FALSE;
	    PR_Unlock(fd->secret->md.miscLock);
        goto ErrorExit;
	}

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
	PR_Unlock(fd->secret->md.miscLock);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    if (opt->status == T_FAILURE || opt->status == T_NOTSUPPORT){
        err = kEOPNOTSUPPErr;
        goto ErrorExit;
    }

    PR_ASSERT(opt->status == T_SUCCESS);

    switch (optname) {
        case SO_LINGER:
            *((t_linger*)optval) = *((t_linger*)&opt->value);
            *optlen = sizeof(t_linger);
            break;
        case SO_REUSEADDR:
        case TCP_NODELAY:
        case SO_KEEPALIVE:
        case SO_RCVBUF:
        case SO_SNDBUF:
            *((PRIntn*)optval) = *((PRIntn*)&opt->value);
            *optlen = sizeof(PRIntn);
            break;
        case IP_MULTICAST_LOOP:
            *((PRUint8*)optval) = *((PRIntn*)&opt->value);
            *optlen = sizeof(PRUint8);
            break;
        case IP_TTL:
            *((PRUintn*)optval) = *((PRUint8*)&opt->value);
            *optlen = sizeof(PRUintn);
            break;
        case IP_MULTICAST_TTL:
            *((PRUint8*)optval) = *((PRUint8*)&opt->value);
            *optlen = sizeof(PRUint8);
            break;
        case IP_ADD_MEMBERSHIP:
        case IP_DROP_MEMBERSHIP:
            {
            

            *((struct ip_mreq *)optval) = *((struct ip_mreq *)&opt->value);
            *optlen = sizeof(struct ip_mreq);
            break;
            }
        case IP_MULTICAST_IF:
            {
            *((PRUint32*)optval) = *((PRUint32*)&opt->value);
            *optlen = sizeof(PRUint32);
            break;
            }
         
        case TCP_MAXSEG:
            if (level == IPPROTO_TCP) { 
                *((PRIntn*)optval) = *((PRIntn*)&opt->value);
                *optlen = sizeof(PRIntn);
            } else { 
                *((PRUintn*)optval) = *((PRUint8*)&opt->value);
                *optlen = sizeof(PRUintn);
            }
            break;
        default:
            PR_ASSERT(0);
            break;    
    }
    
    return PR_SUCCESS;

ErrorExit:
    macsock_map_error(err);
    return PR_FAILURE;
}


PRStatus _MD_setsockopt(PRFileDesc *fd, PRInt32 level, PRInt32 optname, const char* optval, PRInt32 optlen)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    TOptMgmt cmd;
    TOption *opt;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    unsigned char optionBuffer[kOTOptionHeaderSize + sizeof(PRSocketOptionData) + 1];
    
    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
    
    




    if (level == SOL_SOCKET) {
        if (optname == SO_REUSEADDR)
            level = IPPROTO_IP;
        else if (optname == SO_KEEPALIVE)
            level = INET_TCP;
    }

    opt = (TOption *)&optionBuffer[0];
    opt->len = kOTOptionHeaderSize + optlen;

    
    if (optname == SO_KEEPALIVE) 
        opt->len = kOTOptionHeaderSize + sizeof(t_kpalive);
    if (optname == IP_MULTICAST_TTL || optname == IP_TTL) 
        opt->len = kOTOneByteOptionSize;
    if (optname == IP_TOS && level == IPPROTO_IP)
        opt->len = kOTOneByteOptionSize;

    opt->level = level;
    opt->name = optname;
    opt->status = 0;
    
    cmd.opt.len = opt->len;
    cmd.opt.maxlen = sizeof(optionBuffer);
    cmd.opt.buf = (UInt8*)optionBuffer;
    
    optionBuffer[opt->len] = 0;
    
    cmd.flags = T_NEGOTIATE;

    switch (optname) {
        case SO_LINGER:
            *((t_linger*)&opt->value) = *((t_linger*)optval);
            break;
        case SO_REUSEADDR:
        case TCP_NODELAY:
        case SO_RCVBUF:
        case SO_SNDBUF:
            *((PRIntn*)&opt->value) = *((PRIntn*)optval);
            break;
        case IP_MULTICAST_LOOP:
            if (*optval != 0)
                opt->value[0] = T_YES;
            else
                opt->value[0] = T_NO;
            break;
        case SO_KEEPALIVE:
            {
            t_kpalive *kpalive = (t_kpalive *)&opt->value;
            
            kpalive->kp_onoff = *((long*)optval);
            kpalive->kp_timeout = 10; 
            break;
            }
        case IP_TTL:
            *((unsigned char*)&opt->value) = *((PRUintn*)optval);
            break;
        case IP_MULTICAST_TTL:
            *((unsigned char*)&opt->value) = *optval;
            break;
        case IP_ADD_MEMBERSHIP:
        case IP_DROP_MEMBERSHIP:
            {
            

            *((TIPAddMulticast *)&opt->value) = *((TIPAddMulticast *)optval);
            break;
            }
        case IP_MULTICAST_IF:
            {
            *((PRUint32*)&opt->value) = *((PRUint32*)optval);
            break;
            }
         
        case TCP_MAXSEG:
            if (level == IPPROTO_TCP) { 
                *((PRIntn*)&opt->value) = *((PRIntn*)optval);
            } else { 
                *((unsigned char*)&opt->value) = *((PRUintn*)optval);
            }
            break;
        default:
            PR_ASSERT(0);
            break;    
    }
    
	PR_Lock(fd->secret->md.miscLock);
    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
	fd->secret->md.misc.thread = me;

    err = OTOptionManagement(endpoint, &cmd, &cmd);
    if (err != kOTNoError) {
	    me->io_pending = PR_FALSE;
	    PR_Unlock(fd->secret->md.miscLock);
        goto ErrorExit;
	}

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
	PR_Unlock(fd->secret->md.miscLock);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    if (opt->status == T_FAILURE || opt->status == T_NOTSUPPORT){
        err = kEOPNOTSUPPErr;
        goto ErrorExit;
    }
    
    if (level == IPPROTO_TCP && optname == TCP_MAXSEG && opt->status == T_READONLY) {
        err = kEOPNOTSUPPErr;
        goto ErrorExit;
    }

    PR_ASSERT(opt->status == T_SUCCESS);

    return PR_SUCCESS;

ErrorExit:
    macsock_map_error(err);
    return PR_FAILURE;
}


PRInt32 _MD_socketavailable(PRFileDesc *fd)
{
    PRInt32 osfd = fd->secret->md.osfd;
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) osfd;
    size_t bytes;

    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
    
    bytes = 0;
    
    err = OTCountDataBytes(endpoint, &bytes);
    if ((err == kOTLookErr) ||         
        (err == kOTNoDataErr))        
        err = kOTNoError;
        
    if (err != kOTNoError)
        goto ErrorExit;
        
    return bytes;

ErrorExit:
    macsock_map_error(err);
    return -1;
}


typedef struct RawEndpointAndThread
{
	PRThread *  thread;
	EndpointRef endpoint;
} RawEndpointAndThread;




static pascal void  RawEndpointNotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie)
{
    RawEndpointAndThread *endthr = (RawEndpointAndThread *) contextPtr;
    PRThread *    thread   = endthr->thread;
    EndpointRef * endpoint = endthr->endpoint;
    _PRCPU *      cpu      = _PR_MD_CURRENT_CPU(); 
    OSStatus      err;
    OTResult	  resultOT;

    switch (code)
    {

        case T_LISTEN:        
            PR_ASSERT(!"T_EXDATA not implemented for raw endpoints");
            break;
			
        case T_CONNECT:      
			
            err = OTRcvConnect(endpoint, NULL);
            PR_ASSERT(err == kOTNoError);

			
            break;

        case T_DATA:        
			break;

        case T_EXDATA:      
            PR_ASSERT(!"T_EXDATA Not implemented for raw endpoints");
			return;

        case T_DISCONNECT:  
            err = OTRcvDisconnect(endpoint, NULL);
            PR_ASSERT(err == kOTNoError);
            break;
		
        case T_ERROR:       
            PR_ASSERT(!"T_ERROR Not implemented for raw endpoints");
			return;		
		
        case T_UDERR:       
			(void) OTRcvUDErr((EndpointRef) cookie, NULL);
            break;

        case T_ORDREL:      
            err = OTRcvOrderlyDisconnect(endpoint);
            PR_ASSERT(err == kOTNoError);
            break;		

        case T_GODATA:   
			resultOT = OTLook(endpoint);		
			PR_ASSERT(resultOT == T_GODATA);
			
			
            break;

        case T_GOEXDATA: 
            PR_ASSERT(!"T_GOEXDATA Not implemented");
			return;

        case T_REQUEST:  
            PR_ASSERT(!"T_REQUEST Not implemented");
            return;

        case T_REPLY:    
            PR_ASSERT(!"T_REPLY Not implemented");
            return;

        case T_PASSCON:  
			
			
			break;
            

        case T_BINDCOMPLETE:
        case T_UNBINDCOMPLETE:
        case T_ACCEPTCOMPLETE:
        case T_OPTMGMTCOMPLETE:
        case T_GETPROTADDRCOMPLETE:
            break;

		
        default:
            return;
    }

	if (thread) {
		thread->md.osErrCode = result;
		if (_PR_MD_GET_INTSOFF()) {
			thread->md.asyncNotifyPending = PR_TRUE;
			cpu->u.missed[cpu->where] |= _PR_MISSED_IO;
		} else {
			DoneWaitingOnThisThread(thread);
		}
	}

	SignalIdleSemaphore();
}

PRInt32 _MD_accept(PRFileDesc *fd, PRNetAddr *addr, PRUint32 *addrlen, PRIntervalTime timeout)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    TBind bindReq;
    PRNetAddr bindAddr;
    PRInt32 newosfd = -1;
    TCall call;
    PRNetAddr callAddr;
    RawEndpointAndThread *endthr = NULL;

    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
        
    memset(&call, 0 , sizeof(call));

    if (addr != NULL) {
        call.addr.maxlen = *addrlen;
        call.addr.len = *addrlen;
        call.addr.buf = (UInt8*) addr;
    } else {
        call.addr.maxlen = sizeof(callAddr);
        call.addr.len = sizeof(callAddr);
        call.addr.buf = (UInt8*) &callAddr;
    }

	do {
	    PrepareForAsyncCompletion(me, fd->secret->md.osfd);
	    fd->secret->md.misc.thread = me;
	    
	    
	    err = OTListen (endpoint, &call);
	    if (err == kOTNoError)
	    	break; 
	    else if ((!fd->secret->nonblocking) && (err == kOTNoDataErr)) {
	        WaitOnThisThread(me, timeout);
	        err = me->md.osErrCode;
	        if ((err != kOTNoError) && (err != kOTNoDataErr))
	        	goto ErrorExit;
			
			
			
	    } else {
	    	goto ErrorExit; 
	    }   
	}
	while(1);

    newosfd = _MD_socket(AF_INET, SOCK_STREAM, 0);
    if (newosfd == -1)
        return -1;
            
	
	endthr = (RawEndpointAndThread *) PR_Malloc(sizeof(RawEndpointAndThread));
	endthr->thread = me;
	endthr->endpoint = (EndpointRef) newosfd;
	
	err = OTInstallNotifier((ProviderRef) newosfd, RawEndpointNotifierRoutineUPP, endthr);
    PR_ASSERT(err == kOTNoError);
    
	err = OTSetAsynchronous((EndpointRef) newosfd);
	PR_ASSERT(err == kOTNoError);

    
    bindAddr.inet.family = AF_INET;
    bindAddr.inet.port = bindAddr.inet.ip = 0;

    bindReq.addr.maxlen = PR_NETADDR_SIZE (&bindAddr);
    bindReq.addr.len = 0;
    bindReq.addr.buf = (UInt8*) &bindAddr;
    bindReq.qlen = 0;

    PrepareForAsyncCompletion(me, newosfd);    
    err = OTBind((EndpointRef) newosfd, &bindReq, NULL);
    if (err != kOTNoError)
        goto ErrorExit;

    WaitOnThisThread(me, timeout);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    PrepareForAsyncCompletion(me, newosfd);    

    err = OTAccept (endpoint, (EndpointRef) newosfd, &call);
 	if ((err != kOTNoError) && (err != kOTNoDataErr))
        goto ErrorExit;

    WaitOnThisThread(me, timeout);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    if (addrlen != NULL)
        *addrlen = call.addr.len;

	
	OTRemoveNotifier((EndpointRef) newosfd);
	PR_Free(endthr); 

    return newosfd;

ErrorExit:
	me->io_pending = PR_FALSE; 
    if (newosfd != -1)
        _MD_closesocket(newosfd);
    macsock_map_error(err);
    return -1;
}


PRInt32 _MD_connect(PRFileDesc *fd, PRNetAddr *addr, PRUint32 addrlen, PRIntervalTime timeout)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    TCall sndCall;
    TBind bindReq;
    PRNetAddr bindAddr;

    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
        
    if (addr == NULL) {
        err = kEFAULTErr;
        goto ErrorExit;
    }
    
    

    bindAddr.inet.family = AF_INET;
    bindAddr.inet.port = bindAddr.inet.ip = 0;

    bindReq.addr.maxlen = PR_NETADDR_SIZE (&bindAddr);
    bindReq.addr.len = 0;
    bindReq.addr.buf = (UInt8*) &bindAddr;
    bindReq.qlen = 0;
    
    PR_Lock(fd->secret->md.miscLock);
    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
    fd->secret->md.misc.thread = me;

    err = OTBind(endpoint, &bindReq, NULL);
    if (err != kOTNoError) {
      me->io_pending = PR_FALSE;
      PR_Unlock(fd->secret->md.miscLock);
      goto ErrorExit;
    }

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
    PR_Unlock(fd->secret->md.miscLock);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    memset(&sndCall, 0 , sizeof(sndCall));

    sndCall.addr.maxlen = addrlen;
    sndCall.addr.len = addrlen;
    sndCall.addr.buf = (UInt8*) addr;

    if (!fd->secret->nonblocking) {    
      PrepareForAsyncCompletion(me, fd->secret->md.osfd);
      PR_ASSERT(fd->secret->md.write.thread == NULL);
      fd->secret->md.write.thread = me;
    }

    err = OTConnect (endpoint, &sndCall, NULL);
    if (err == kOTNoError) {
      PR_ASSERT(!"OTConnect returned kOTNoError in async mode!?!");	
    }
    if (fd->secret->nonblocking) {
      if (err == kOTNoDataErr)
      err = EINPROGRESS;
      goto ErrorExit;
    } else {
      if (err != kOTNoError && err != kOTNoDataErr) {
        me->io_pending = PR_FALSE;
        goto ErrorExit;
      }
    }
	
    WaitOnThisThread(me, timeout);

    err = me->md.osErrCode;
    if (err != kOTNoError)
        goto ErrorExit;

    return kOTNoError;

ErrorExit:
    macsock_map_error(err);
    return -1;
}





static PRInt32 SendReceiveStream(PRFileDesc *fd, void *buf, PRInt32 amount, 
                               PRIntn flags, PRIntervalTime timeout, SndRcvOpCode opCode)
{
    OSStatus err;
    OTResult result;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRInt32 bytesLeft = amount;

    PR_ASSERT(flags == 0 ||
        (opCode == kSTREAM_RECEIVE && flags == PR_MSG_PEEK));
    PR_ASSERT(opCode == kSTREAM_SEND || opCode == kSTREAM_RECEIVE);
    
    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
        
    if (buf == NULL) {
        err = kEFAULTErr;
        goto ErrorExit;
    }

    PR_ASSERT(opCode == kSTREAM_SEND ? fd->secret->md.write.thread == NULL :
                                       fd->secret->md.read.thread  == NULL);

    while (bytesLeft > 0)
    {
        Boolean disabledNotifications = OTEnterNotifier(endpoint);
    
        PrepareForAsyncCompletion(me, fd->secret->md.osfd);    

        if (opCode == kSTREAM_SEND) {
        	do {
				fd->secret->md.write.thread = me;
				fd->secret->md.writeReady = PR_FALSE;				
	            result = OTSnd(endpoint, buf, bytesLeft, NULL);
				fd->secret->md.writeReady = (result != kOTFlowErr);
				if (fd->secret->nonblocking)							
					break;
				else {

					
					if (result != kOTFlowErr)
						break;

					
					
                    if (disabledNotifications) {
                        OTLeaveNotifier(endpoint);
                        disabledNotifications = false;
                    }
					WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
					result = me->md.osErrCode;
					if (result != kOTNoError) 
						break;

					
					disabledNotifications = OTEnterNotifier(endpoint);
  			   		PrepareForAsyncCompletion(me, fd->secret->md.osfd);  
				}
			}
			while(1);
        } else {
        	do {
				fd->secret->md.read.thread = me;
				fd->secret->md.readReady = PR_FALSE;				
	            result = OTRcv(endpoint, buf, bytesLeft, NULL);
	            if (fd->secret->nonblocking) {
					fd->secret->md.readReady = (result != kOTNoDataErr);
					break;
				} else {
					if (result != kOTNoDataErr) {
			      		
			      		
			      		
			        	size_t count;
			        	OSErr tmpResult;
			        	tmpResult = OTCountDataBytes(endpoint, &count);
				        fd->secret->md.readReady = ((tmpResult == kOTNoError) && (count > 0));
						break;
				    }

					
					
                    if (disabledNotifications) {
                        OTLeaveNotifier(endpoint);
                        disabledNotifications = false;
                    }
					WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
					result = me->md.osErrCode;
					if (result != kOTNoError) 
						break;

					
					disabledNotifications = OTEnterNotifier(endpoint);
  			   		PrepareForAsyncCompletion(me, fd->secret->md.osfd);  
				}
			}
			
			while(1);
        }

		me->io_pending = PR_FALSE;
		
        if (opCode == kSTREAM_SEND)
            fd->secret->md.write.thread = NULL;
        else
            fd->secret->md.read.thread  = NULL;

        
        if (disabledNotifications)
            OTLeaveNotifier(endpoint);

        if (result > 0) {
            buf = (void *) ( (UInt32) buf + (UInt32)result );
            bytesLeft -= result;
            if (opCode == kSTREAM_RECEIVE) {
                amount = result;
                goto NormalExit;
            }
        } else {
			switch (result) {
				case kOTLookErr:
				    PR_ASSERT(!"call to OTLook() required after all.");
					break;
				
				case kOTFlowErr:
				case kOTNoDataErr:
				case kEAGAINErr:
				case kEWOULDBLOCKErr:
					if (fd->secret->nonblocking) {
					
					    if (bytesLeft == amount) {  
						    err = result;
						    goto ErrorExit;
						}
						
						
						amount -= bytesLeft;
						goto NormalExit;
					}

					WaitOnThisThread(me, timeout);
					err = me->md.osErrCode;
					if (err != kOTNoError)
						goto ErrorExit;				
					break;
					
				case kOTOutStateErr:	
					if (fd->secret->md.orderlyDisconnect) {
						amount = 0;
						goto NormalExit;
					}
					
				default:
					err = result;
					goto ErrorExit;
			}
		}
    }

NormalExit:
    PR_ASSERT(opCode == kSTREAM_SEND ? fd->secret->md.write.thread == NULL :
                                       fd->secret->md.read.thread  == NULL);
    return amount;

ErrorExit:
    PR_ASSERT(opCode == kSTREAM_SEND ? fd->secret->md.write.thread == NULL :
                                       fd->secret->md.read.thread  == NULL);
    macsock_map_error(err);
    return -1;
}


PRInt32 _MD_recv(PRFileDesc *fd, void *buf, PRInt32 amount, 
                               PRIntn flags, PRIntervalTime timeout)
{
    return (SendReceiveStream(fd, buf, amount, flags, timeout, kSTREAM_RECEIVE));
}


PRInt32 _MD_send(PRFileDesc *fd,const void *buf, PRInt32 amount, 
                               PRIntn flags, PRIntervalTime timeout)
{
    return (SendReceiveStream(fd, (void *)buf, amount, flags, timeout, kSTREAM_SEND));
}





static PRInt32 SendReceiveDgram(PRFileDesc *fd, void *buf, PRInt32 amount, 
                               PRIntn flags, PRNetAddr *addr, PRUint32 *addrlen, 
                               PRIntervalTime timeout, SndRcvOpCode opCode)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) fd->secret->md.osfd;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRInt32 bytesLeft = amount;
    TUnitData dgram;

    PR_ASSERT(flags == 0);
    
    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
        
    if (buf == NULL || addr == NULL) {
        err = kEFAULTErr;
        goto ErrorExit;
    }
    
    if (opCode != kDGRAM_SEND && opCode != kDGRAM_RECEIVE) {
        err = kEINVALErr;
        goto ErrorExit;
    }
        
    memset(&dgram, 0 , sizeof(dgram));
    dgram.addr.maxlen = *addrlen;
    dgram.addr.len = *addrlen;
    dgram.addr.buf = (UInt8*) addr;
    dgram.udata.maxlen = amount;
    dgram.udata.len = amount;
    dgram.udata.buf = (UInt8*) buf;    

    while (bytesLeft > 0) {
    
        PrepareForAsyncCompletion(me, fd->secret->md.osfd);    

        if (opCode == kDGRAM_SEND) {
			fd->secret->md.write.thread = me;
			fd->secret->md.writeReady = PR_FALSE;				
            err = OTSndUData(endpoint, &dgram);
			if (err != kOTFlowErr)							
				fd->secret->md.writeReady = PR_TRUE;
		} else {
			fd->secret->md.read.thread = me;
			fd->secret->md.readReady = PR_FALSE;				
            err = OTRcvUData(endpoint, &dgram, NULL);
			if (err != kOTNoDataErr)							
				fd->secret->md.readReady = PR_TRUE;
		}

        if (err == kOTNoError) {
            buf = (void *) ( (UInt32) buf + (UInt32)dgram.udata.len );
            bytesLeft -= dgram.udata.len;
            dgram.udata.buf = (UInt8*) buf;    
            me->io_pending = PR_FALSE;
        } else {
            PR_ASSERT(err == kOTNoDataErr || err == kOTOutStateErr);
            WaitOnThisThread(me, timeout);
            err = me->md.osErrCode;
            if (err != kOTNoError)
                goto ErrorExit;
        }
    }

    if (opCode == kDGRAM_RECEIVE)
        *addrlen = dgram.addr.len;

    return amount;

ErrorExit:
    macsock_map_error(err);
    return -1;
}


PRInt32 _MD_recvfrom(PRFileDesc *fd, void *buf, PRInt32 amount, 
                               PRIntn flags, PRNetAddr *addr, PRUint32 *addrlen,
                               PRIntervalTime timeout)
{
    return (SendReceiveDgram(fd, buf, amount, flags, addr, addrlen,
                            timeout, kDGRAM_RECEIVE));
}


PRInt32 _MD_sendto(PRFileDesc *fd,const void *buf, PRInt32 amount, 
                               PRIntn flags, PRNetAddr *addr, PRUint32 addrlen,
                               PRIntervalTime timeout)
{
    return (SendReceiveDgram(fd, (void *)buf, amount, flags, addr, &addrlen,
                            timeout, kDGRAM_SEND));
}


PRInt32 _MD_closesocket(PRInt32 osfd)
{
    OSStatus err;
    EndpointRef endpoint = (EndpointRef) osfd;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if (endpoint == NULL) {
        err = kEBADFErr;
        goto ErrorExit;
    }
        
    if (me->io_pending && me->io_fd == osfd)
        me->io_pending = PR_FALSE;

    (void) OTSndOrderlyDisconnect(endpoint);
    err = OTCloseProvider(endpoint);
    if (err != kOTNoError)
        goto ErrorExit;

    return kOTNoError;

ErrorExit:
    macsock_map_error(err);
    return -1;
}


PRInt32 _MD_writev(PRFileDesc *fd, const struct PRIOVec *iov, PRInt32 iov_size, PRIntervalTime timeout)
{
#pragma unused (fd, iov, iov_size, timeout)

    PR_ASSERT(0);
    _PR_MD_CURRENT_THREAD()->md.osErrCode = unimpErr;
    return -1;
}




static PRBool GetState(PRFileDesc *fd, PRBool *readReady, PRBool *writeReady, PRBool *exceptReady)
{
    OTResult resultOT;
    
    
    size_t   availableData = 1;
    if (!fd->secret->md.orderlyDisconnect)
        OTCountDataBytes((EndpointRef)fd->secret->md.osfd, &availableData);

    *readReady = fd->secret->md.readReady && (availableData > 0);
    *exceptReady = fd->secret->md.exceptReady;

    resultOT = OTGetEndpointState((EndpointRef)fd->secret->md.osfd);
    switch (resultOT) {
        case T_IDLE:
        case T_UNBND:
            
            
            
            
            
            *readReady = PR_FALSE;
            *writeReady = PR_FALSE;
            break;

        case T_DATAXFER:        
            *writeReady = fd->secret->md.writeReady;
            break;

        case T_INREL:           
            *writeReady = fd->secret->md.writeReady;
            break;

        case T_OUTCON:          
        case T_INCON:           
        case T_OUTREL:          
        default:
            *writeReady = PR_FALSE;
    }
    
    return  *readReady || *writeReady || *exceptReady;
}



static PRInt32 CheckPollDescMethods(PRPollDesc *pds, PRIntn npds, PRInt16 *outReadFlags, PRInt16 *outWriteFlags)
{
    PRInt32     ready = 0;
    PRPollDesc  *pd, *epd;
    PRInt16     *readFlag, *writeFlag;
    
    for (pd = pds, epd = pd + npds, readFlag = outReadFlags, writeFlag = outWriteFlags;
        pd < epd;
        pd++, readFlag++, writeFlag++)
    {
        PRInt16  in_flags_read = 0,  in_flags_write = 0;
        PRInt16 out_flags_read = 0, out_flags_write = 0;

        pd->out_flags = 0;

        if (NULL == pd->fd || pd->in_flags == 0) continue;

        if (pd->in_flags & PR_POLL_READ)
        {
            in_flags_read = (pd->fd->methods->poll)(
                pd->fd, pd->in_flags & ~PR_POLL_WRITE, &out_flags_read);
        }

        if (pd->in_flags & PR_POLL_WRITE)
        {
            in_flags_write = (pd->fd->methods->poll)(
                pd->fd, pd->in_flags & ~PR_POLL_READ, &out_flags_write);
        }

        if ((0 != (in_flags_read & out_flags_read)) ||
            (0 != (in_flags_write & out_flags_write)))
        {
            ready += 1;  
            pd->out_flags = out_flags_read | out_flags_write;
        }
        
        *readFlag = in_flags_read;
        *writeFlag = in_flags_write;
    }

    return ready;
}



static PRInt32 CheckPollDescEndpoints(PRPollDesc *pds, PRIntn npds, const PRInt16 *inReadFlags, const PRInt16 *inWriteFlags)
{
    PRInt32 ready = 0;
    PRPollDesc *pd, *epd;
    const PRInt16   *readFlag, *writeFlag;
    
    for (pd = pds, epd = pd + npds, readFlag = inReadFlags, writeFlag = inWriteFlags;
         pd < epd;
        pd++, readFlag++, writeFlag++)
    {
        PRFileDesc *bottomFD;
        PRBool      readReady, writeReady, exceptReady;
        PRInt16     in_flags_read = *readFlag;
        PRInt16     in_flags_write = *writeFlag;

        if (NULL == pd->fd || pd->in_flags == 0) continue;

        if ((pd->in_flags & ~pd->out_flags) == 0) {
            ready++;
            continue;
        }

        bottomFD = PR_GetIdentitiesLayer(pd->fd, PR_NSPR_IO_LAYER);
        
        if (bottomFD)
        {
            if (_PR_FILEDESC_OPEN == bottomFD->secret->state)
            {
                if (GetState(bottomFD, &readReady, &writeReady, &exceptReady))
                {
                    if (readReady)
                    {
                        if (in_flags_read & PR_POLL_READ)
                            pd->out_flags |= PR_POLL_READ;
                        if (in_flags_write & PR_POLL_READ)
                            pd->out_flags |= PR_POLL_WRITE;
                    }
                    if (writeReady)
                    {
                        if (in_flags_read & PR_POLL_WRITE)
                            pd->out_flags |= PR_POLL_READ;
                        if (in_flags_write & PR_POLL_WRITE)
                            pd->out_flags |= PR_POLL_WRITE;
                    }
                    if (exceptReady && (pd->in_flags & PR_POLL_EXCEPT))
                    {
                        pd->out_flags |= PR_POLL_EXCEPT;
                    }
                }
                if (0 != pd->out_flags) ready++;
            }
            else    
            {
                ready += 1;  
                pd->out_flags = PR_POLL_NVAL;  
            }
        }
    }

    return ready;
}



static PRInt32 CountReadyPollDescs(PRPollDesc *pds, PRIntn npds)
{
    PRInt32 ready = 0;
    PRPollDesc *pd, *epd;
    
    for (pd = pds, epd = pd + npds; pd < epd; pd++)
    {
        if (pd->out_flags)
            ready ++;
    }

    return ready;
}


static void SetDescPollThread(PRPollDesc *pds, PRIntn npds, PRThread* thread)
{
    PRInt32     ready = 0;
    PRPollDesc *pd, *epd;

    for (pd = pds, epd = pd + npds; pd < epd; pd++)
    {   
        if (pd->fd)
        { 
            PRFileDesc *bottomFD = PR_GetIdentitiesLayer(pd->fd, PR_NSPR_IO_LAYER);
            if (bottomFD && (_PR_FILEDESC_OPEN == bottomFD->secret->state))
            {
                if (pd->in_flags & PR_POLL_READ) {
                    PR_ASSERT(thread == NULL || bottomFD->secret->md.read.thread == NULL);
                    bottomFD->secret->md.read.thread = thread;
                }

                if (pd->in_flags & PR_POLL_WRITE) {
                    
                    
                    
                    
                    
                    
                    
                    PR_ASSERT(thread == NULL ||
                        (bottomFD->secret->md.write.thread == NULL ||
                         bottomFD->secret->md.write.thread == thread));
                    bottomFD->secret->md.write.thread = thread;
                }
            }
        }
    }
}


#define DESCRIPTOR_FLAGS_ARRAY_SIZE     32

PRInt32 _MD_poll(PRPollDesc *pds, PRIntn npds, PRIntervalTime timeout)
{
    PRInt16     readFlagsArray[DESCRIPTOR_FLAGS_ARRAY_SIZE];
    PRInt16     writeFlagsArray[DESCRIPTOR_FLAGS_ARRAY_SIZE];
    
    PRInt16     *readFlags  = readFlagsArray;
    PRInt16     *writeFlags = writeFlagsArray;

    PRInt16     *ioFlags = NULL;
    
    PRThread    *thread = _PR_MD_CURRENT_THREAD();
    PRInt32     ready;
    
    if (npds > DESCRIPTOR_FLAGS_ARRAY_SIZE)
    {
        
        
        ioFlags = (PRInt16*)PR_Malloc(sizeof(PRInt16) * npds * 2);
        if (!ioFlags)
        {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            return -1;
        }
        
        readFlags = ioFlags;
        writeFlags = &ioFlags[npds];
    }

    
    
    
    ready = CheckPollDescMethods(pds, npds, readFlags, writeFlags);

    if (!ready && timeout != PR_INTERVAL_NO_WAIT) {
        intn        is;
        

        _PR_INTSOFF(is);
        PR_Lock(thread->md.asyncIOLock);
        PrepareForAsyncCompletion(thread, 0);

        SetDescPollThread(pds, npds, thread);

        (void)CheckPollDescEndpoints(pds, npds, readFlags, writeFlags);

        PR_Unlock(thread->md.asyncIOLock);
        _PR_FAST_INTSON(is);

        ready = CountReadyPollDescs(pds, npds);

        if (ready == 0) {
            WaitOnThisThread(thread, timeout);

            
            
            (void)CheckPollDescMethods(pds, npds, readFlags, writeFlags);
            ready = CheckPollDescEndpoints(pds, npds, readFlags, writeFlags);
        }
        
        thread->io_pending = PR_FALSE;
        SetDescPollThread(pds, npds, NULL);
    }
    else {
        ready = CheckPollDescEndpoints(pds, npds, readFlags, writeFlags);
    }

    if (readFlags != readFlagsArray)
        PR_Free(ioFlags);
    
    return ready;
}


void _MD_initfiledesc(PRFileDesc *fd)
{
	
	
	
	if (fd->methods->file_type == PR_DESC_SOCKET_TCP ||
		fd->methods->file_type == PR_DESC_SOCKET_UDP )
	{
		PR_ASSERT(fd->secret->md.miscLock == NULL);
		fd->secret->md.miscLock = PR_NewLock();
		PR_ASSERT(fd->secret->md.miscLock != NULL);
		fd->secret->md.orderlyDisconnect = PR_FALSE;
		fd->secret->md.readReady = PR_FALSE;		
		fd->secret->md.writeReady = PR_TRUE;		
		fd->secret->md.exceptReady = PR_FALSE;
	}
}


void _MD_freefiledesc(PRFileDesc *fd)
{
	if (fd->secret->md.miscLock)
	{
		PR_ASSERT(fd->methods->file_type == PR_DESC_SOCKET_TCP || fd->methods->file_type == PR_DESC_SOCKET_UDP);
		PR_DestroyLock(fd->secret->md.miscLock);
		fd->secret->md.miscLock = NULL;
	} else {
		PR_ASSERT(fd->methods->file_type != PR_DESC_SOCKET_TCP && PR_DESC_SOCKET_TCP != PR_DESC_SOCKET_UDP);
	}
}



void _MD_makenonblock(PRFileDesc *fd)
{
	
	
	
	
	ProviderRef	endpointRef = (ProviderRef)fd->secret->md.osfd;
	OSStatus	err;
	
	
	
	
	
	
	
	err = OTInstallNotifier(endpointRef, NotifierRoutineUPP, fd->secret);
	PR_ASSERT(err == kOTNoError);
	
	
	err = OTSetAsynchronous(endpointRef);
	PR_ASSERT(err == kOTNoError);
}


void _MD_initfdinheritable(PRFileDesc *fd, PRBool imported)
{
	
	fd->secret->inheritable = _PR_TRI_UNKNOWN;
}


void _MD_queryfdinheritable(PRFileDesc *fd)
{
	
	PR_ASSERT(0);
}


PR_IMPLEMENT(PRInt32) _MD_shutdown(PRFileDesc *fd, PRIntn how)
{
#pragma unused (fd, how)


return (0);
}


PR_IMPLEMENT(PRStatus) 
_MD_getpeername(PRFileDesc *fd, PRNetAddr *addr, PRUint32 *addrlen)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
	EndpointRef ep = (EndpointRef) fd->secret->md.osfd;
	InetAddress inetAddr;
	TBind peerAddr;
	OSErr err;
	
	if (*addrlen < sizeof(InetAddress)) {

		err = (OSErr) kEINVALErr;
		goto ErrorExit;
	}

    peerAddr.addr.maxlen = sizeof(InetAddress);
    peerAddr.addr.len = 0;
    peerAddr.addr.buf = (UInt8*) &inetAddr;
    peerAddr.qlen = 0;

    PrepareForAsyncCompletion(me, fd->secret->md.osfd);    
	fd->secret->md.misc.thread = me; 

	err = OTGetProtAddress(ep, NULL, &peerAddr);
	
    if (err != kOTNoError)
        goto ErrorExit;

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);

    err = me->md.osErrCode;
    if ((err == kOTNoError) && (peerAddr.addr.len < sizeof(InetAddress)))
    	err = kEBADFErr; 
    if (err != kOTNoError)
    	goto ErrorExit;
    	
    
    addr->inet.family = AF_INET;
    addr->inet.port = (PRUint16) inetAddr.fPort;
    addr->inet.ip = (PRUint32) inetAddr.fHost;
    
    *addrlen = PR_NETADDR_SIZE(addr); 
	return PR_SUCCESS;

ErrorExit:
    macsock_map_error(err);
    return PR_FAILURE;
}


PR_IMPLEMENT(unsigned long) inet_addr(const char *cp)
{
    OSStatus err;
    InetHost host;    

    _MD_FinishInitNetAccess();

    err = OTInetStringToHost((char*) cp, &host);
    if (err != kOTNoError)
        return -1;
    
    return host;
}


static char *sAliases[1] = {NULL};
static struct hostent sHostEnt = {NULL, &sAliases[0], AF_INET, sizeof (long), NULL};
static InetHostInfo sHostInfo;
static InetHost *sAddresses[kMaxHostAddrs+1];


PR_IMPLEMENT(struct hostent *) gethostbyname(const char * name)
{
    OSStatus err;
    PRUint32 index;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    	_MD_FinishInitNetAccess();

    me->io_pending       = PR_TRUE;
    me->io_fd            = NULL;
    me->md.osErrCode     = noErr;
	
	PR_Lock(dnsContext.lock);	
	dnsContext.thread = me;		

    err = OTInetStringToAddress(dnsContext.serviceRef, (char *)name, &sHostInfo);
    if (err != kOTNoError) {
        me->io_pending = PR_FALSE;
        me->md.osErrCode = err;
        goto ErrorExit;
    }

    WaitOnThisThread(me, PR_INTERVAL_NO_TIMEOUT);
	PR_Unlock(dnsContext.lock);

    if (me->md.osErrCode != kOTNoError)
        goto ErrorExit;

    sHostEnt.h_name = sHostInfo.name;
    for (index=0; index<kMaxHostAddrs && sHostInfo.addrs[index] != NULL; index++)
        sAddresses[index] = &sHostInfo.addrs[index];
    sAddresses[index] = NULL;    
    sHostEnt.h_addr_list = (char **)sAddresses;

    return (&sHostEnt);

ErrorExit:
    return NULL;
}


PR_IMPLEMENT(struct hostent *) gethostbyaddr(const void *addr, int addrlen, int type)
{
    PR_ASSERT(type == AF_INET);
    PR_ASSERT(addrlen == sizeof(struct in_addr));

    	_MD_FinishInitNetAccess();

    OTInetHostToString((InetHost)addr, sHostInfo.name);
    
    return (gethostbyname(sHostInfo.name));
}


PR_IMPLEMENT(char *) inet_ntoa(struct in_addr addr)
{
    _MD_FinishInitNetAccess();

    OTInetHostToString((InetHost)addr.s_addr, sHostInfo.name);
    
    return sHostInfo.name;
}


PRStatus _MD_gethostname(char *name, int namelen)
{
    OSStatus err;
    InetInterfaceInfo info;

    _MD_FinishInitNetAccess();

    







    
    if (namelen < strlen("123.123.123.123")) {
        err = kEINVALErr;
        goto ErrorExit;
    }

    err = OTInetGetInterfaceInfo(&info, kDefaultInetInterface);
    if (err != kOTNoError)
        goto ErrorExit;
    
    OTInetHostToString(info.fAddress, name);
    
    return PR_SUCCESS;

ErrorExit:
    macsock_map_error(err);
    return PR_FAILURE;
}


#define kIPName        "ip"
static struct protoent sIPProto = {kIPName, NULL, INET_IP};
static struct protoent sTCPProto = {kTCPName, NULL, INET_TCP};
static struct protoent sUDPProto = {kUDPName, NULL, INET_UDP};

PR_IMPLEMENT(struct protoent *) getprotobyname(const char * name)
{
    if (strcmp(name, kIPName) == 0)
        return (&sIPProto);
        
    if (strcmp(name, kTCPName) == 0)
        return (&sTCPProto);
        
    if (strcmp(name, kUDPName) == 0)
        return (&sUDPProto);
        
ErrorExit:
    macsock_map_error(kEINVALErr);
    return NULL;
}


PR_IMPLEMENT(struct protoent *) getprotobynumber(int number)
{
    if (number == INET_IP)
        return (&sIPProto);
        
    if (number == INET_TCP)
        return (&sTCPProto);
        
    if (number == INET_UDP)
        return (&sUDPProto);
        
ErrorExit:
    macsock_map_error(kEINVALErr);
    return NULL;
}


int _MD_mac_get_nonblocking_connect_error(PRFileDesc* fd)
{
    EndpointRef endpoint = (EndpointRef)fd->secret->md.osfd;
    OTResult    resultOT = OTGetEndpointState(endpoint);

    switch (resultOT)    {
        case T_OUTCON:
            macsock_map_error(EINPROGRESS);
            return -1;
            
        case T_DATAXFER:
            return 0;
            
        case T_IDLE:
            macsock_map_error(fd->secret->md.disconnectError);
            fd->secret->md.disconnectError = 0;
            return -1;

        case T_INREL:
            macsock_map_error(ENOTCONN);
            return -1;

        default:
            PR_ASSERT(0);
            return -1;
    }

    return -1;      
}
