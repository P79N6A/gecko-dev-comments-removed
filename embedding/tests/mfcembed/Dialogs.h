






























#ifndef _DIALOGS_H_
#define _DIALOGS_H_

#include "resource.h"

class CBrowserView;

class CFindDialog : public CFindReplaceDialog    
{
public:
    CFindDialog(CString& csSearchStr, PRBool bMatchCase,
                PRBool bMatchWholeWord, PRBool bWrapAround,
                PRBool bSearchBackwards, CBrowserView* pOwner);
    BOOL WrapAround();
    BOOL SearchBackwards();

private:
    CString m_csSearchStr;
    PRBool m_bMatchCase;
    PRBool m_bMatchWholeWord;
    PRBool m_bWrapAround;
    PRBool m_bSearchBackwards;
    CBrowserView* m_pOwner;

protected:
    virtual BOOL OnInitDialog();
    virtual void PostNcDestroy();
};




class CLinkPropertiesDlg : public CDialog
{
public:
	CLinkPropertiesDlg(CWnd* pParent = NULL);

	enum { IDD = IDD_DIALOG_LINK_PROPERTIES };
	CString	m_LinkText;
	CString	m_LinkLocation;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
};

#endif
