






































#ifndef xptcprivate_h___
#define xptcprivate_h___

#include "xptcall.h"
#include "nsAutoPtr.h"

class xptiInterfaceEntry;

#if !defined(__ia64) || (!defined(__hpux) && !defined(__linux__))
#define STUB_ENTRY(n) NS_IMETHOD Stub##n() = 0;
#else
#define STUB_ENTRY(n) NS_IMETHOD Stub##n(PRUint64,PRUint64,PRUint64,PRUint64,PRUint64,PRUint64,PRUint64,PRUint64) = 0;
#endif

#define SENTINEL_ENTRY(n) NS_IMETHOD Sentinel##n() = 0;

class nsIXPTCStubBase : public nsISupports
{
public:
#include "xptcstubsdef.inc"
};

#undef STUB_ENTRY
#undef SENTINEL_ENTRY

#if !defined(__ia64) || (!defined(__hpux) && !defined(__linux__))
#define STUB_ENTRY(n) NS_IMETHOD Stub##n();
#else
#define STUB_ENTRY(n) NS_IMETHOD Stub##n(PRUint64,PRUint64,PRUint64,PRUint64,PRUint64,PRUint64,PRUint64,PRUint64);
#endif

#define SENTINEL_ENTRY(n) NS_IMETHOD Sentinel##n();

class nsXPTCStubBase : public nsIXPTCStubBase
{
public:
    NS_DECL_ISUPPORTS_INHERITED

#include "xptcstubsdef.inc"

    nsXPTCStubBase(nsIXPTCProxy* aOuter, xptiInterfaceEntry *aEntry) :
        mOuter(aOuter), mEntry(aEntry) { }

    nsIXPTCProxy*          mOuter;
    xptiInterfaceEntry*    mEntry;

    ~nsXPTCStubBase() { }
};

#undef STUB_ENTRY
#undef SENTINEL_ENTRY

#endif 
