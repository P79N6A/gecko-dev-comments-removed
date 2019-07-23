











































#include "SupportsMixin.h"
#include "nsAgg.h"



#ifdef SUPPORT_AGGREGATION

SupportsMixin::SupportsMixin(void* instance, const InterfaceInfo interfaces[], UInt32 interfaceCount, nsISupports* outer)
	: mInstance(instance), mRefCount(0), mInterfaces(interfaces), mInterfaceCount(interfaceCount), mOuter(outer)
{
	if (mOuter != NULL)
		mInner = new Inner(this);
}

SupportsMixin::~SupportsMixin()
{
	if (mRefCount > 0) {
		::DebugStr("\pmRefCount > 0!");
	}
	if (mInner != NULL)
		delete mInner;
}





nsresult SupportsMixin::OuterQueryInterface(const nsIID& aIID, void** aInstancePtr)
{
	
	nsresult result = queryInterface(aIID, aInstancePtr);
	
	if (result != NS_OK && mOuter != NULL)
		return mOuter->QueryInterface(aIID, aInstancePtr);
	else
		return result;
}

nsrefcnt SupportsMixin::OuterAddRef()
{
	nsrefcnt result = addRef();
	if (mOuter != NULL)
		return mOuter->AddRef();
	return result;
}

nsrefcnt SupportsMixin::OuterRelease()
{
	if (mOuter != NULL) {
		nsIOuter* outer = NULL;
		nsISupports* supports = mOuter;
		static NS_DEFINE_IID(kIOuterIID, NS_IOUTER_IID);
		if (mRefCount == 1 && supports->QueryInterface(kIOuterIID, &outer) == NS_OK) {
			outer->ReleaseInner(mInner);
			outer->Release();
		} else
			release();
		return supports->Release();
	} else {
		return release();
	}
}

#else 

SupportsMixin::SupportsMixin(void* instance, const InterfaceInfo interfaces[], UInt32 interfaceCount, nsISupports* )
	: mInstance(instance), mRefCount(0), mInterfaces(interfaces), mInterfaceCount(interfaceCount)
{
}

SupportsMixin::~SupportsMixin()
{
	if (mRefCount > 0) {
		::DebugStr("\pmRefCount > 0!");
	}
}

#endif 






NS_IMETHODIMP SupportsMixin::queryInterface(const nsIID& aIID, void** aInstancePtr)
{
	if (aInstancePtr == NULL) {
		return NS_ERROR_NULL_POINTER;
	}
	
	
	const InterfaceInfo* interfaces = mInterfaces;
	UInt32 count = mInterfaceCount;
	for (UInt32 i = 0; i < count; i++) {
		if (aIID.Equals(interfaces[i].mIID)) {
			*aInstancePtr = (void*) (UInt32(mInstance) + interfaces[i].mOffset);
			addRef();
			return NS_OK;
		}
	}
	
	static NS_DEFINE_IID(kISupportsIID, NS_ISUPPORTS_IID);
	if (aIID.Equals(kISupportsIID)) {
		*aInstancePtr = (void*) mInstance;
		addRef();
		return NS_OK;
	}
	return NS_NOINTERFACE;
}

NS_IMETHODIMP_(nsrefcnt) SupportsMixin::addRef()
{
	return ++mRefCount;
}

NS_IMETHODIMP_(nsrefcnt) SupportsMixin::release()
{
	if (--mRefCount == 0) {
		delete this;
		return 0;
	}
	return mRefCount;
}
