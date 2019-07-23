






























#if !defined(AFX_CFORMATOPTIONTAB_H__F7BDB355_9202_440A_8478_165AD3FC2F41__INCLUDED_)
#define AFX_CFORMATOPTIONTAB_H__F7BDB355_9202_440A_8478_165AD3FC2F41__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif 



#include "nsIPrintSettings.h"




class CFormatOptionTab : public CPropertyPage
{
	DECLARE_DYNCREATE(CFormatOptionTab)


public:
	CFormatOptionTab();
	~CFormatOptionTab();
  void GetPaperSizeInfo(short& aType, double& aWidth, double& aHeight);


	
	enum { IDD = IDD_FORMAT_OPTIONS_TAB };
	int		m_PaperSize;
	BOOL	m_BGColors;
	int		m_Scaling;
	CString	m_ScalingText;
	int		m_DoInches;
	BOOL	m_BGImages;
	double	m_PaperHeight;
	double	m_PaperWidth;
	BOOL	m_IsLandScape;
	

  nsCOMPtr<nsIPrintSettings> m_PrintSettings;
  int                        m_PaperSizeInx;


	
	
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    
	


protected:
  void EnableUserDefineControls(BOOL aEnable);
  int  GetPaperSizeIndexFromData(short aType, double aW, double aH);
  int  GetPaperSizeIndex(const CString& aStr);

	
	
	virtual BOOL OnInitDialog();
	afx_msg void OnCustomdrawScale(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKillfocusScaleTxt();
	afx_msg void OnSelchangePaperSizeCbx();
	
	DECLARE_MESSAGE_MAP()

};




#endif
