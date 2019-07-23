


#include "stdafx.h"
#include "RegMozCtl.h"
#include "RegMozCtlDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




CRegMozCtlDlg::CRegMozCtlDlg(CWnd* pParent )
	: CDialog(CRegMozCtlDlg::IDD, pParent)
{
	
	m_szMozillaDir = _T("");
	
	
	m_hIcon = AfxGetApp()->LoadIcon(IDI_MOZILLA);

	GetCurrentDirectory(1024, m_szMozillaDir.GetBuffer(1024));
	m_szMozillaDir.ReleaseBuffer();
}

void CRegMozCtlDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_TASKLIST, m_cTaskList);
	DDX_Text(pDX, IDC_MOZILLADIR, m_szMozillaDir);
	
}

BEGIN_MESSAGE_MAP(CRegMozCtlDlg, CDialog)
	
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_REGISTER, OnRegister)
	ON_BN_CLICKED(IDC_UNREGISTER, OnUnregister)
	ON_BN_CLICKED(IDC_PICKDIR, OnPickDir)
	
END_MESSAGE_MAP()




BOOL CRegMozCtlDlg::OnInitDialog()
{
	
	
	SetIcon(m_hIcon, TRUE);			
	SetIcon(m_hIcon, FALSE);		
	
	
	TCHAR szValue[1024];
	DWORD dwSize = sizeof(szValue) / sizeof(szValue[0]);
	CRegKey cKey;
	cKey.Create(HKEY_LOCAL_MACHINE, MOZ_CONTROL_REG_KEY);
	
	memset(szValue, 0, sizeof(szValue));
	if (cKey.QueryValue(szValue, MOZ_CONTROL_REG_VALUE_BIN_DIR_PATH, &dwSize) == ERROR_SUCCESS)
	{
		m_szMozillaDir = CString(szValue);
	}

    CDialog::OnInitDialog();

    CRegTask::PopulateTasks(m_cTaskMgr);

	






	


	struct ColumnData
	{
		TCHAR *sColumnTitle;
		int nPercentageWidth;
		int nFormat;
	};

	ColumnData aColData[] =
	{
		{ _T("Task"),	70, LVCFMT_LEFT },
		{ _T("Status"),	30, LVCFMT_LEFT }
	};

	
	

	CRect rcList;
	m_cTaskList.GetClientRect(&rcList);

	int nColTotal = sizeof(aColData) / sizeof(aColData[0]);
	int nWidthRemaining = rcList.Width();
	
	for (int nCol = 0; nCol < nColTotal; nCol++)
	{
		ColumnData *pData = &aColData[nCol];

		int nColWidth = (rcList.Width() * pData->nPercentageWidth) / 100;
		if (nCol == nColTotal - 1)
		{
			nColWidth = nWidthRemaining;
		}
		else if (nColWidth > nWidthRemaining)
		{
			nColWidth = nWidthRemaining;
		}

		m_cTaskList.InsertColumn(nCol, pData->sColumnTitle, pData->nFormat, nColWidth);

		nWidthRemaining -= nColWidth;
		if (nColWidth <= 0)
		{
			break;
		}
	}

    for (int i = 0; i < m_cTaskMgr.GetTaskCount(); i++)
    {
        CRegTask *pTask = m_cTaskMgr.GetTask(i);
        m_cTaskList.InsertItem(i, pTask->GetDescription());
    }

	return TRUE;  
}





void CRegMozCtlDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}



HCURSOR CRegMozCtlDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CRegMozCtlDlg::OnRegister() 
{
	RegisterMozillaControl(TRUE);
}

void CRegMozCtlDlg::OnUnregister() 
{
	RegisterMozillaControl(FALSE);
}

void CRegMozCtlDlg::RegisterMozillaControl(BOOL bRegister)
{
	UpdateData();

    m_cTaskMgr.SetValue(c_szValueBinDirPath, m_szMozillaDir);

    for (int i = 0; i < m_cTaskMgr.GetTaskCount(); i++)
    {
        CRegTask *pTask = m_cTaskMgr.GetTask(i);
        pTask->DoTask();
    }

    AfxMessageBox(bRegister ? _T("Register completed") : _T("Unregister completed"));
}

void CRegMozCtlDlg::OnPickDir() 
{
	BROWSEINFO bi;
	TCHAR szFolder[MAX_PATH + 1];

	memset(szFolder, 0, sizeof(szFolder));

	memset(&bi, 0, sizeof(bi));
	bi.hwndOwner = GetSafeHwnd();
	bi.pidlRoot = NULL;
	bi.pszDisplayName = szFolder;
	bi.lpszTitle = _T("Pick a folder to scan");

	
	LPITEMIDLIST pItemList = SHBrowseForFolder(&bi);
	if (pItemList)
	{
		IMalloc *pShellAllocator = NULL;
		
		SHGetMalloc(&pShellAllocator);
		if (pShellAllocator)
		{
			char szPath[MAX_PATH + 1];

			if (SHGetPathFromIDList(pItemList, szPath))
			{
				
				int nPathSize = strlen(szPath);
				if (nPathSize > 0)
				{
					if (szPath[nPathSize - 1] == '\\')
					{
						szPath[nPathSize - 1] = '\0';
					}
				}

				m_szMozillaDir = CString(szPath);

				UpdateData(FALSE);
			}

			pShellAllocator->Free(pItemList);
			pShellAllocator->Release();
		}
	}
}


CString CRegMozCtlDlg::GetSystemPath()
{
	
	return _T("");
}

BOOL CRegMozCtlDlg::SetSystemPath(const CString &szNewPath)
{
	
	return TRUE;
}
