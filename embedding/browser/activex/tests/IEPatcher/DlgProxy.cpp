


#include "stdafx.h"
#include "IEPatcher.h"
#include "DlgProxy.h"
#include "IEPatcherDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNCREATE(CIEPatcherDlgAutoProxy, CCmdTarget)

CIEPatcherDlgAutoProxy::CIEPatcherDlgAutoProxy()
{
	EnableAutomation();
	
	
	
	AfxOleLockApp();

	
	
	
	
	ASSERT (AfxGetApp()->m_pMainWnd != NULL);
	ASSERT_VALID (AfxGetApp()->m_pMainWnd);
	ASSERT_KINDOF(CIEPatcherDlg, AfxGetApp()->m_pMainWnd);
	m_pDialog = (CIEPatcherDlg*) AfxGetApp()->m_pMainWnd;
	m_pDialog->m_pAutoProxy = this;
}

CIEPatcherDlgAutoProxy::~CIEPatcherDlgAutoProxy()
{
	
	
	
	AfxOleUnlockApp();
}

void CIEPatcherDlgAutoProxy::OnFinalRelease()
{
	
	
	
	

	CCmdTarget::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CIEPatcherDlgAutoProxy, CCmdTarget)
	
		
	
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CIEPatcherDlgAutoProxy, CCmdTarget)
	
		
	
END_DISPATCH_MAP()






static const IID IID_IIEPatcher =
{ 0xa6031677, 0x3b36, 0x11d2, { 0xb4, 0x4d, 0x0, 0x60, 0x8, 0x19, 0x60, 0x7e } };

BEGIN_INTERFACE_MAP(CIEPatcherDlgAutoProxy, CCmdTarget)
	INTERFACE_PART(CIEPatcherDlgAutoProxy, IID_IIEPatcher, Dispatch)
END_INTERFACE_MAP()



IMPLEMENT_OLECREATE2(CIEPatcherDlgAutoProxy, "IEPatcher.Application", 0xa6031675, 0x3b36, 0x11d2, 0xb4, 0x4d, 0x0, 0x60, 0x8, 0x19, 0x60, 0x7e)



