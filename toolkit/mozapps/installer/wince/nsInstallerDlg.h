





































#pragma once




class nsInstallerDlg : public nsExtractorProgress
{
public:
  static nsInstallerDlg* GetInstance();
  void Init(HINSTANCE hInst);
  int DoModal();
  virtual void Progress(int n); 
  INT_PTR DlgMain(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
  const WCHAR* GetExtractPath() { return m_sInstallPath; }

private:
  nsInstallerDlg();
  BOOL OnInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam);
  void FindMemCards();
  BOOL OnBtnExtract();
  LRESULT SendMessageToControl(int nCtlID, UINT Msg, WPARAM wParam = 0, LPARAM lParam = 0);
  void SetControlWindowText(int nCtlID, const WCHAR *sText);

  BOOL PostExtract();
  BOOL StoreInstallPath();
  BOOL CreateShortcut();
  BOOL MoveSetupStrings();
  BOOL SilentFirstRun();

  BOOL GetInstallPath(WCHAR *sPath);
  void RunUninstall();

  void AddErrorMsg(WCHAR* sErr);

  static const int c_nMaxErrorLen = 2048;

  HINSTANCE m_hInst;
  HWND m_hDlg;

  WCHAR m_sProgramFiles[MAX_PATH];
  WCHAR m_sExtractPath[MAX_PATH];
  WCHAR m_sInstallPath[MAX_PATH];
  WCHAR m_sErrorMsg[c_nMaxErrorLen];
};
