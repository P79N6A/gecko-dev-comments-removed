






































#ifndef nsHttp_h__
#define nsHttp_h__

#if defined(MOZ_LOGGING)
#define FORCE_PR_LOG
#endif

#ifdef MOZ_IPC
#include "mozilla/net/NeckoChild.h"
#endif 

#include "plstr.h"
#include "prlog.h"
#include "prtime.h"
#include "nsISupportsUtils.h"
#include "nsPromiseFlatString.h"
#include "nsURLHelper.h"
#include "netCore.h"

#if defined(PR_LOGGING)











extern PRLogModuleInfo *gHttpLog;
#endif

#undef LOG

#define LOG1(args) PR_LOG(gHttpLog, 1, args)
#define LOG2(args) PR_LOG(gHttpLog, 2, args)
#define LOG3(args) PR_LOG(gHttpLog, 3, args)
#define LOG4(args) PR_LOG(gHttpLog, 4, args)
#define LOG(args) LOG4(args)

#define LOG1_ENABLED() PR_LOG_TEST(gHttpLog, 1)
#define LOG2_ENABLED() PR_LOG_TEST(gHttpLog, 2)
#define LOG3_ENABLED() PR_LOG_TEST(gHttpLog, 3)
#define LOG4_ENABLED() PR_LOG_TEST(gHttpLog, 4)
#define LOG_ENABLED() LOG4_ENABLED()


#define NS_HTTP_SEGMENT_SIZE  4096
#define NS_HTTP_SEGMENT_COUNT 16   // 64k maximum
#define NS_HTTP_MAX_ODA_SIZE  (NS_HTTP_SEGMENT_SIZE * 4) // 16k


#define NS_HTTP_VERSION_UNKNOWN  0
#define NS_HTTP_VERSION_0_9      9
#define NS_HTTP_VERSION_1_0     10
#define NS_HTTP_VERSION_1_1     11

typedef PRUint8 nsHttpVersion;





#define NS_HTTP_ALLOW_KEEPALIVE      (1<<0)
#define NS_HTTP_ALLOW_PIPELINING     (1<<1)



#define NS_HTTP_STICKY_CONNECTION    (1<<2)



#define NS_HTTP_REFRESH_DNS          (1<<3)



#define NS_HTTP_LOAD_ANONYMOUS       (1<<4)






#define NS_HTTP_MAX_PIPELINED_REQUESTS 8 

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

    
    static nsHttpAtom ResolveAtom(const char *);
    static nsHttpAtom ResolveAtom(const nsACString &s)
    {
        return ResolveAtom(PromiseFlatCString(s).get());
    }

    
    
    static PRBool IsValidToken(const char *start, const char *end);

    static inline PRBool IsValidToken(const nsCString &s) {
        const char *start = s.get();
        return IsValidToken(start, start + s.Length());
    }

    
    
    
    
    
    static const char *FindToken(const char *input, const char *token,
                                 const char *separators);

    
    
    
    
    
    
    
    
    static PRBool ParseInt64(const char *input, const char **next,
                             PRInt64 *result);

    
    
    static inline PRBool ParseInt64(const char *input, PRInt64 *result) {
        const char *next;
        return ParseInt64(input, &next, result) && *next == '\0';
    }

    
    
    
    
    
    
#define HTTP_ATOM(_name, _value) static nsHttpAtom _name;
#include "nsHttpAtomList.h"
#undef HTTP_ATOM
};





static inline PRUint32
PRTimeToSeconds(PRTime t_usec)
{
    return PRUint32( t_usec / PR_USEC_PER_SEC );
}

#define NowInSeconds() PRTimeToSeconds(PR_Now())


#undef  CLAMP
#define CLAMP(x, low, high)  (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))


#define QVAL_TO_UINT(q) ((unsigned int) ((q + 0.05) * 10.0))

#define HTTP_LWS " \t"
#define HTTP_HEADER_VALUE_SEPS HTTP_LWS ","

#endif 
