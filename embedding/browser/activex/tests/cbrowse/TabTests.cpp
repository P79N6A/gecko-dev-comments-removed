


#include "stdafx.h"
#include "cbrowse.h"
#include "TabTests.h"
#include "CBrowseDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




IMPLEMENT_DYNCREATE(CTabTests, CPropertyPage)

CTabTests::CTabTests() : CPropertyPage(CTabTests::IDD, CTabTests::IDD)
{
	
	m_szTestDescription = _T("");
	
}


CTabTests::~CTabTests()
{
}


void CTabTests::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	
	DDX_Control(pDX, IDC_RUNTEST, m_btnRunTest);
	DDX_Control(pDX, IDC_TESTLIST, m_tcTests);
	DDX_Text(pDX, IDC_TESTDESCRIPTION, m_szTestDescription);
	
}


BEGIN_MESSAGE_MAP(CTabTests, CPropertyPage)
	
	ON_BN_CLICKED(IDC_RUNTEST, OnRunTest)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TESTLIST, OnSelchangedTestlist)
	ON_NOTIFY(NM_DBLCLK, IDC_TESTLIST, OnDblclkTestlist)
	
END_MESSAGE_MAP()




void CTabTests::OnRunTest() 
{
	m_pBrowseDlg->OnRunTest();	
}


void CTabTests::OnSelchangedTestlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;

	BOOL bItemSelected = FALSE;
	m_szTestDescription.Empty();

	HTREEITEM hItem = m_tcTests.GetNextItem(NULL, TVGN_FIRSTVISIBLE);
	while (hItem)
	{
		UINT nState;

		nState = m_tcTests.GetItemState(hItem, TVIS_SELECTED);
		if (nState & TVIS_SELECTED)
		{
			bItemSelected = TRUE;
			if (m_tcTests.ItemHasChildren(hItem))
			{
				TestSet *pTestSet = (TestSet *) m_tcTests.GetItemData(hItem);
				m_szTestDescription = pTestSet->szDesc;
			}
			else
			{
				Test *pTest = (Test *) m_tcTests.GetItemData(hItem);
				m_szTestDescription = pTest->szDesc;
			}
		}

		hItem = m_tcTests.GetNextItem(hItem, TVGN_NEXTVISIBLE);
	}

	UpdateData(FALSE);
	m_btnRunTest.EnableWindow(bItemSelected);

	*pResult = 0;
}


void CTabTests::OnDblclkTestlist(NMHDR* pNMHDR, LRESULT* pResult) 
{
	OnRunTest();
	*pResult = 0;
}


BOOL CTabTests::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
	m_pBrowseDlg->PopulateTests();
	
	return TRUE;  
	              
}
