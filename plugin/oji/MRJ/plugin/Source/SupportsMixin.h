

















































#pragma once

#include "nsISupports.h"



struct InterfaceInfo {
	nsID mIID;			
	UInt32 mOffset;		
};

class SupportsMixin {
public:
	
	nsresult queryInterface(const nsIID& aIID, void** aInstancePtr);
	nsrefcnt addRef(void);
	nsrefcnt release(void);

protected:
	SupportsMixin(void* instance, const InterfaceInfo interfaces[], UInt32 interfaceCount, nsISupports* outer = NULL);
	virtual ~SupportsMixin();

#ifdef SUPPORT_AGGREGATION
	NS_METHOD OuterQueryInterface(REFNSIID aIID, void** aInstancePtr);
	NS_METHOD_(nsrefcnt) OuterAddRef(void);
	NS_METHOD_(nsrefcnt) OuterRelease(void);
#endif

private:
	void* mInstance;
	nsrefcnt mRefCount;
	const InterfaceInfo* mInterfaces;
	UInt32 mInterfaceCount;

#ifdef SUPPORT_AGGREGATION
	nsISupports* mOuter;
	
	class Inner : public nsISupports {
	public:
		NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) { return mSupports->queryInterface(aIID, aInstancePtr); }
		NS_IMETHOD_(nsrefcnt) AddRef(void) { return mSupports->addRef(); }
		NS_IMETHOD_(nsrefcnt) Release(void) { return mSupports->release(); }
		
		Inner(SupportsMixin* supports) : mSupports(supports) {}
		
	private:
		SupportsMixin* mSupports;
	};

	Inner* mInner;
#endif
};

#ifdef SUPPORT_AGGREGATION

#define DECL_SUPPORTS_MIXIN \
	NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) { return OuterQueryInterface(aIID, aInstancePtr); } \
	NS_IMETHOD_(nsrefcnt) AddRef(void) { return OuterAddRef(); } \
	NS_IMETHOD_(nsrefcnt) Release(void) { return OuterRelease(); }

#else

#define DECL_SUPPORTS_MIXIN \
	NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr) { return queryInterface(aIID, aInstancePtr); } \
	NS_IMETHOD_(nsrefcnt) AddRef(void) { return addRef(); } \
	NS_IMETHOD_(nsrefcnt) Release(void) { return release(); }

#endif

#define INTERFACE_OFFSET(leafType, interfaceType) \
	UInt32((interfaceType*) ((leafType*)0))
