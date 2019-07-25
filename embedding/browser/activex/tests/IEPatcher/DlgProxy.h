


#if !defined(AFX_DLGPROXY_H__A603167E_3B36_11D2_B44D_00600819607E__INCLUDED_)
#define AFX_DLGPROXY_H__A603167E_3B36_11D2_B44D_00600819607E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif 

class CIEPatcherDlg;




class CIEPatcherDlgAutoProxy : public CCmdTarget
{
	DECLARE_DYNCREATE(CIEPatcherDlgAutoProxy)

	CIEPatcherDlgAutoProxy();           


public:
	CIEPatcherDlg* m_pDialog;


public:


	
	
	public:
	virtual void OnFinalRelease();
	


protected:
	virtual ~CIEPatcherDlgAutoProxy();

	
	
		
	

	DECLARE_MESSAGE_MAP()
	DECLARE_OLECREATE(CIEPatcherDlgAutoProxy)

	
	
		
	
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};






#endif
