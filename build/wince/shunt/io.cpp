







































#include "mozce_internal.h"

extern "C" {
#if 0
}
#endif


MOZCE_SHUNT_API int mozce_chmod(const char* inFilename, int inMode)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("mozce_chmod called\n");
#endif
    
    int retval = -1;
    
    if(NULL != inFilename)
    {
        unsigned short buffer[MAX_PATH];
        
        int convRes = a2w_buffer(inFilename, -1, buffer, sizeof(buffer) / sizeof(unsigned short));
        if(0 != convRes)
        {
            DWORD attribs = 0;
            
            attribs = GetFileAttributesW(buffer);
            if(0 != attribs)
            {
                if(0 != (_S_IWRITE & inMode))
                {
                    attribs |= FILE_ATTRIBUTE_READONLY;
                }
                else
                {
                    attribs &= ~FILE_ATTRIBUTE_READONLY;
                }
                
                BOOL setRes = SetFileAttributesW(buffer, attribs);
                if(FALSE != setRes)
                {
                    retval = 0;
                }
            }
        }
    }
    
    return retval;
}


MOZCE_SHUNT_API int mozce_isatty(int inHandle)
{
    MOZCE_PRECHECK

#ifdef DEBUG
    mozce_printf("-- mozce_isatty called\n");
#endif
    
    int retval = 0;
    
    return retval;
}






static struct protoent sProtos[] = {
    { "tcp",    NULL,   IPPROTO_TCP },
    { "udp",    NULL,   IPPROTO_UDP },
    { "ip",     NULL,   IPPROTO_IP },
    { "icmp",   NULL,   IPPROTO_ICMP },
    { "ggp",    NULL,   IPPROTO_GGP },
    { "pup",    NULL,   IPPROTO_PUP },
    { "idp",    NULL,   IPPROTO_IDP },
    { "nd",     NULL,   IPPROTO_ND },
    { "raw",    NULL,   IPPROTO_RAW }
};

#define MAX_PROTOS (sizeof(sProtos) / sizeof(struct protoent))






MOZCE_SHUNT_API struct protoent* mozce_getprotobyname(const char* inName)
{
    struct protoent* retval = NULL;

    if(NULL != inName)
    {
        unsigned uLoop;

        for(uLoop = 0; uLoop < MAX_PROTOS; uLoop++)
        {
            if(0 == _stricmp(inName, sProtos[uLoop].p_name))
            {
                retval = &sProtos[uLoop];
                break;
            }
        }
    }

    return retval;
}






MOZCE_SHUNT_API struct protoent* mozce_getprotobynumber(int inNumber)
{
    struct protoent* retval = NULL;
    unsigned uLoop;
    
    for(uLoop = 0; uLoop < MAX_PROTOS; uLoop++)
    {
        if(inNumber == sProtos[uLoop].p_proto)
        {
            retval = &sProtos[uLoop];
            break;
        }
    }

    return retval;

}
#if 0
{
#endif
} 

