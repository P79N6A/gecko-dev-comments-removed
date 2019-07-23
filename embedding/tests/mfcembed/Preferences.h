































#ifndef _PREFERENCES_H_
#define _PREFERENCES_H_

#include "resource.h"

#if _MSC_VER > 1000
#pragma once
#endif 




class CStartupPrefsPage : public CPropertyPage
{
    DECLARE_DYNCREATE(CStartupPrefsPage)


public:
    CStartupPrefsPage();
    ~CStartupPrefsPage();


    
    enum { IDD = IDD_PREFS_START_PAGE };
    CEdit    m_HomePage;
    CString    m_strHomePage;
    int        m_iStartupPage;
    



    
    
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    
    


protected:
    
    
    
    DECLARE_MESSAGE_MAP()

};




class CPreferences : public CPropertySheet
{
    DECLARE_DYNAMIC(CPreferences)


public:
    CPreferences(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);


public:
    CStartupPrefsPage m_startupPage;


public:


    
    
    public:
    virtual BOOL OnInitDialog();
    


public:
    virtual ~CPreferences();

    
protected:
    
        
    
    DECLARE_MESSAGE_MAP()
};




#endif
