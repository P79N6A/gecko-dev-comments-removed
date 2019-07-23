




































#include "nspr.h"
#include "prio.h"
#include "prinit.h"
#include "prprf.h"
#include "obsolete/probslet.h"

#include "plerror.h"

static PRFileDesc *err = NULL;
static PRBool failed = PR_FALSE;

static void Failed(const char *msg1, const char *msg2)
{
    if (NULL != msg1) PR_fprintf(err, "%s ", msg1);
    PL_FPrintError(err, msg2);
    failed = PR_TRUE;
}  

static PRSockOption Incr(PRSockOption *option)
{
    PRIntn val = ((PRIntn)*option) + 1;
    *option = (PRSockOption)val;
    return (PRSockOption)val;
}  

int main(int argc, char **argv)
{
    PRStatus rv;
    PRFileDesc *udp = PR_NewUDPSocket();
    PRFileDesc *tcp = PR_NewTCPSocket();
    const char *tag[] =
    {
        "PR_SockOpt_Nonblocking",     
        "PR_SockOpt_Linger",          
        "PR_SockOpt_Reuseaddr",       
        "PR_SockOpt_Keepalive",       
        "PR_SockOpt_RecvBufferSize",  
        "PR_SockOpt_SendBufferSize",  

        "PR_SockOpt_IpTimeToLive",    
        "PR_SockOpt_IpTypeOfService", 

        "PR_SockOpt_AddMember",       
        "PR_SockOpt_DropMember",      
        "PR_SockOpt_McastInterface",  
        "PR_SockOpt_McastTimeToLive", 
        "PR_SockOpt_McastLoopback",   

        "PR_SockOpt_NoDelay",         
        "PR_SockOpt_MaxSegment",      
        "PR_SockOpt_Broadcast",       
        "PR_SockOpt_Last"
    };

    err = PR_GetSpecialFD(PR_StandardError);
    PR_STDIO_INIT();

    if (NULL == udp) Failed("PR_NewUDPSocket()", NULL);
    else if (NULL == tcp) Failed("PR_NewTCPSocket()", NULL);
    else
    {
        PRSockOption option;
        PRUint32 segment = 1024;
        PRNetAddr addr;

        rv = PR_InitializeNetAddr(PR_IpAddrAny, 0, &addr);
        if (PR_FAILURE == rv) Failed("PR_InitializeNetAddr()", NULL);
        rv = PR_Bind(udp, &addr);
        if (PR_FAILURE == rv) Failed("PR_Bind()", NULL);
        for(option = PR_SockOpt_Linger; option < PR_SockOpt_Last; Incr(&option))
        {
            PRSocketOptionData data;
            PRFileDesc *fd = tcp;
            data.option = option;
            switch (option)
            {
                case PR_SockOpt_Nonblocking:
                    data.value.non_blocking = PR_TRUE;
                    break;    
#ifndef SYMBIAN
                case PR_SockOpt_Linger:
                    data.value.linger.polarity = PR_TRUE;
                    data.value.linger.linger = PR_SecondsToInterval(2);          
                    break;    
#endif
                case PR_SockOpt_Reuseaddr:
                    data.value.reuse_addr = PR_TRUE;      
                    break;    
                case PR_SockOpt_Keepalive:       
                    data.value.keep_alive = PR_TRUE;      
                    break;    
                case PR_SockOpt_RecvBufferSize:
                    data.value.recv_buffer_size = segment;  
                    break;    
                case PR_SockOpt_SendBufferSize:  
                    data.value.send_buffer_size = segment;  
                    break;    
#ifndef SYMBIAN
                case PR_SockOpt_IpTimeToLive:
                    data.value.ip_ttl = 64;  
                    break;    
                case PR_SockOpt_IpTypeOfService:
                    data.value.tos = 0; 
                    break;    
                case PR_SockOpt_McastTimeToLive:
                    fd = udp; 
                    data.value.mcast_ttl = 4; 
                    break;    
                case PR_SockOpt_McastLoopback:
                    fd = udp; 
                    data.value.mcast_loopback = PR_TRUE; 
                    break;    
#endif
                case PR_SockOpt_NoDelay:
                    data.value.no_delay = PR_TRUE;         
                    break;    
#ifndef WIN32
                case PR_SockOpt_MaxSegment:
                    data.value.max_segment = segment;      
                    break;    
#endif
#ifndef SYMBIAN
                case PR_SockOpt_Broadcast:
                    fd = udp; 
                    data.value.broadcast = PR_TRUE;         
                    break;    
#endif
                default: continue;
            }

			


            if (option != PR_SockOpt_MaxSegment) {
#ifdef WIN32
            	if (option != PR_SockOpt_McastLoopback)
#endif
				{
            		rv = PR_SetSocketOption(fd, &data);
            		if (PR_FAILURE == rv)
							Failed("PR_SetSocketOption()", tag[option]);
				}
			}

            rv = PR_GetSocketOption(fd, &data);
            if (PR_FAILURE == rv) Failed("PR_GetSocketOption()", tag[option]);
        }
        PR_Close(udp);
        PR_Close(tcp);
    }
    PR_fprintf(err, "%s\n", (failed) ? "FAILED" : "PASSED");
    return (failed) ? 1 : 0;
}  



