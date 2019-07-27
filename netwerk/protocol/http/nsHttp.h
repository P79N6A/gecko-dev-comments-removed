





#ifndef nsHttp_h__
#define nsHttp_h__

#include <stdint.h>
#include "prtime.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsError.h"


#define NS_HTTP_VERSION_UNKNOWN  0
#define NS_HTTP_VERSION_0_9      9
#define NS_HTTP_VERSION_1_0     10
#define NS_HTTP_VERSION_1_1     11
#define NS_HTTP_VERSION_2_0     20

namespace mozilla {

class Mutex;

namespace net {
    enum {
        SPDY_VERSION_2_REMOVED = 2,
        SPDY_VERSION_3 = 3,
        SPDY_VERSION_31 = 4,
        HTTP_VERSION_2 = 5,

        
        
        
        
        
        
        
        HTTP2_VERSION_DRAFT14 = 30
    };

typedef uint8_t nsHttpVersion;

#define NS_HTTP2_DRAFT_VERSION HTTP2_VERSION_DRAFT14
#define NS_HTTP2_DRAFT_TOKEN "h2-14"





#define NS_HTTP_ALLOW_KEEPALIVE      (1<<0)
#define NS_HTTP_ALLOW_PIPELINING     (1<<1)



#define NS_HTTP_STICKY_CONNECTION    (1<<2)



#define NS_HTTP_REFRESH_DNS          (1<<3)



#define NS_HTTP_LOAD_ANONYMOUS       (1<<4)


#define NS_HTTP_TIMING_ENABLED       (1<<5)



#define NS_HTTP_LOAD_AS_BLOCKING     (1<<6)




#define NS_HTTP_DISALLOW_SPDY        (1<<7)



#define NS_HTTP_LOAD_UNBLOCKED       (1<<8)



#define NS_HTTP_ALLOW_RSA_FALSESTART (1<<9)





#define NS_HTTP_DEFAULT_PORT  80
#define NS_HTTPS_DEFAULT_PORT 443

#define NS_HTTP_HEADER_SEPS ", \t"





struct nsHttpAtom
{
    operator const char *() const { return _val; }
    const char *get() const { return _val; }

    void operator=(const char *v) { _val = v; }
    void operator=(const nsHttpAtom &a) { _val = a._val; }

    
    const char *_val;
};

struct nsHttp
{
    static nsresult CreateAtomTable();
    static void DestroyAtomTable();

    
    
    
    static Mutex *GetLock();

    
    static nsHttpAtom ResolveAtom(const char *);
    static nsHttpAtom ResolveAtom(const nsACString &s)
    {
        return ResolveAtom(PromiseFlatCString(s).get());
    }

    
    
    static bool IsValidToken(const char *start, const char *end);

    static inline bool IsValidToken(const nsACString &s) {
        return IsValidToken(s.BeginReading(), s.EndReading());
    }

    
    
    
    static bool IsReasonableHeaderValue(const nsACString &s);

    
    
    
    
    
    static const char *FindToken(const char *input, const char *token,
                                 const char *separators);

    
    
    
    
    
    
    
    
    static bool ParseInt64(const char *input, const char **next,
                             int64_t *result);

    
    
    static inline bool ParseInt64(const char *input, int64_t *result) {
        const char *next;
        return ParseInt64(input, &next, result) && *next == '\0';
    }

    
    static bool IsPermanentRedirect(uint32_t httpStatus);

    
    
    
    
    
    
#define HTTP_ATOM(_name, _value) static nsHttpAtom _name;
#include "nsHttpAtomList.h"
#undef HTTP_ATOM
};





static inline uint32_t
PRTimeToSeconds(PRTime t_usec)
{
    return uint32_t( t_usec / PR_USEC_PER_SEC );
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())


#define QVAL_TO_UINT(q) ((unsigned int) ((q + 0.005) * 100.0))

#define HTTP_LWS " \t"
#define HTTP_HEADER_VALUE_SEPS HTTP_LWS ","

void EnsureBuffer(nsAutoArrayPtr<char> &buf, uint32_t newSize,
                  uint32_t preserve, uint32_t &objSize);
void EnsureBuffer(nsAutoArrayPtr<uint8_t> &buf, uint32_t newSize,
                  uint32_t preserve, uint32_t &objSize);

} 
} 

#endif 
