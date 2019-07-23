





































#ifndef PROPERTYDLG_H
#define PROPERTYDLG_H

class CPPageDlg : public CDialogImpl<CPPageDlg>
{
public:
    enum { IDD = IDD_PPAGE_LINK };

    nsString mType;
    nsString mURL;

    BEGIN_MSG_MAP(CPPageLinkDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    END_MSG_MAP()

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,  LPARAM lParam, BOOL& bHandled);
};


class CPropertyDlg : public CDialogImpl<CPropertyDlg>
{
public:
    enum { IDD = IDD_PROPERTIES };

    CPPageDlg *mPPage;

    BEGIN_MSG_MAP(CPropertyDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDCLOSE, OnClose)
    END_MSG_MAP()

    CPropertyDlg();

    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam,  LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnClose(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

    HRESULT AddPage(CPPageDlg *pPage);
};


#endif