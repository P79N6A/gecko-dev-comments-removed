















































#include "PyXPCOM_std.h"

PyXPCOM_GatewayWeakReference::PyXPCOM_GatewayWeakReference( PyG_Base *base )
{
	m_pBase = base;

#ifdef NS_BUILD_REFCNT_LOGGING
	
	strncpy(refcntLogRepr, m_pBase->refcntLogRepr, sizeof(refcntLogRepr));
	refcntLogRepr[sizeof(refcntLogRepr)-1] = '\0';
	char *dest = refcntLogRepr + ((strlen(refcntLogRepr) > 36) ? 36 : strlen(refcntLogRepr));
	strcpy(dest, "(WR)");
#endif 
}

PyXPCOM_GatewayWeakReference::~PyXPCOM_GatewayWeakReference()
{
	
	
	
	
	m_pBase = NULL;
}

nsrefcnt
PyXPCOM_GatewayWeakReference::AddRef(void)
{
	nsrefcnt cnt = (nsrefcnt) PR_AtomicIncrement((PRInt32*)&mRefCnt);
#ifdef NS_BUILD_REFCNT_LOGGING
	NS_LOG_ADDREF(this, cnt, refcntLogRepr, sizeof(*this));
#endif
	return cnt;
}

nsrefcnt
PyXPCOM_GatewayWeakReference::Release(void)
{
	nsrefcnt cnt = (nsrefcnt) PR_AtomicDecrement((PRInt32*)&mRefCnt);
#ifdef NS_BUILD_REFCNT_LOGGING
	NS_LOG_RELEASE(this, cnt, refcntLogRepr);
#endif
	if ( cnt == 0 )
		delete this;
	return cnt;
}

NS_IMPL_THREADSAFE_QUERY_INTERFACE1(PyXPCOM_GatewayWeakReference, nsIWeakReference)

NS_IMETHODIMP
PyXPCOM_GatewayWeakReference::QueryReferent(REFNSIID iid, void * *ret)
{
	{ 
	
	
	
	CEnterLeaveXPCOMFramework _celf;
	if (m_pBase == NULL)
		return NS_ERROR_NULL_POINTER;
	m_pBase->AddRef(); 
	} 
	nsresult nr = m_pBase->QueryInterface(iid, ret);
	m_pBase->Release();
	return nr;
}
