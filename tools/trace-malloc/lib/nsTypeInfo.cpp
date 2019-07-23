


















































#include <typeinfo>
#include <ctype.h>

extern "C" const char* nsGetTypeName(void* ptr);

class IUnknown {
public:
    virtual long QueryInterface() = 0;
    virtual long AddRef() = 0;
    virtual long Release() = 0;
};

#if defined(MACOS)

#include <Processes.h>

class AddressSpace {
public:
    AddressSpace();
    Boolean contains(void* ptr);
private:
    ProcessInfoRec mInfo;
};

AddressSpace::AddressSpace()
{
    ProcessSerialNumber psn = { 0, kCurrentProcess };
    mInfo.processInfoLength = sizeof(mInfo);
    ::GetProcessInformation(&psn, &mInfo);
}

Boolean AddressSpace::contains(void* ptr)
{
    UInt32 start = UInt32(mInfo.processLocation);
    return (UInt32(ptr) >= start && UInt32(ptr) < (start + mInfo.processSize));
}

const char* nsGetTypeName(void* ptr)
{
    
    static AddressSpace space;
	
    
    void** vt = *(void***)ptr;
    if (vt && !(unsigned(vt) & 0x3) && space.contains(vt) && space.contains(*vt)) {
	IUnknown* u = static_cast<IUnknown*>(ptr);
	const char* type = typeid(*u).name();
        
	if (type && (isalnum(type[0]) || type[0] == '_'))
	    return type;
    }
    return "void*";
}

#endif




#if 0

#include <signal.h>
#include <setjmp.h>

static jmp_buf context;

static void handler(int signum)
{
    longjmp(context, signum);
}

#define attempt() setjmp(context)

class Signaller {
public:
    Signaller(int signum);
    ~Signaller();

private:
    typedef void (*handler_t) (int signum);
    int mSignal;
    handler_t mOldHandler;
};

Signaller::Signaller(int signum)
    : mSignal(signum), mOldHandler(signal(signum, &handler))
{
}

Signaller::~Signaller()
{
    signal(mSignal, mOldHandler);
}







extern "C" {
    
    extern void* _pr_faulty_methods;
};

static inline int
sanity_check_vtable_i386(void** vt)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    unsigned char** fp1 = reinterpret_cast<unsigned char**>(vt) + 1;

    
    unsigned char* ip = *fp1;
    if ((unsigned(ip) & 3) != 0)
        return 0;

    
    static unsigned char prologue[] = { 0x55, 0x89, 0xE5 };
    for (unsigned i = 0; i < sizeof(prologue); ++i)
        if (*ip++ != prologue[i])
            return 0;

    
    if (*ip == 0x53 || *ip == 0x56) {
        return 1;
    }

    
    
    if (ip[0] == 0x83 && ip[1] == 0xec     
        && ip[3] == 0x83 && ip[4] == 0x3d  
        && ip[10] == 0x75                  
        ) {
        return 1;
    }

    return 0;
}

static inline int
sanity_check_vtable_ppc(void** vt)
{
    
    return 1;
}

#if defined(__i386)
#  define SANITY_CHECK_VTABLE(vt) (sanity_check_vtable_i386(vt))
#elif defined(PPC)
#  define SANITY_CHECK_VTABLE(vt) (sanity_check_vtable_ppc(vt))
#else
#  define SANITY_CHECK_VTABLE(vt) (1)
#endif

const char* nsGetTypeName(void* ptr)
{
    
    void** vt = *(void***)ptr;
    if (vt && !(unsigned(vt) & 3) && (vt != &_pr_faulty_methods)) {
        Signaller s1(SIGSEGV);
        if (attempt() == 0) {
            if (SANITY_CHECK_VTABLE(vt)) {
                
                IUnknown* u = static_cast<IUnknown*>(ptr);
                const char* type = typeid(*u).name();
                
                while (isdigit(*type)) ++type;
                return type;
            }
        }
    }
    return "void*";
}

#endif

#if defined(linux) || defined(XP_MACOSX)

#define __USE_GNU
#include <dlfcn.h>
#include <ctype.h>
#include <string.h>

const char* nsGetTypeName(void* ptr)
{
#if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION >= 100 
    const int expected_offset = 8;
    const char vtable_sym_start[] = "_ZTV";
    const int vtable_sym_start_length = sizeof(vtable_sym_start) - 1;
#else
    const int expected_offset = 0;
    const char vtable_sym_start[] = "__vt_";
    const int vtable_sym_start_length = sizeof(vtable_sym_start) - 1;
#endif
    void* vt = *(void**)ptr;
    Dl_info info;
    
    
    if ( !dladdr(vt, &info) ||
         ((char*)info.dli_saddr) + expected_offset != vt ||
         !info.dli_sname ||
         strncmp(info.dli_sname, vtable_sym_start, vtable_sym_start_length))
        return "void*";

    
    
    const char* rv = info.dli_sname + vtable_sym_start_length;
    while (*rv && isdigit(*rv))
        ++rv;
    return rv;
}

#endif

#ifdef XP_WIN32
const char* nsGetTypeName(void* ptr)
{
  
    return "void*";
}

#endif 
