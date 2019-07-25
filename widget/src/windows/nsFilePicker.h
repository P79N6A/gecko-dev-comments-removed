








































#ifndef nsFilePicker_h__
#define nsFilePicker_h__

#include <windows.h>

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
#define _WIN32_WINNT_bak _WIN32_WINNT
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#define _WIN32_IE_bak _WIN32_IE
#undef _WIN32_IE
#define _WIN32_IE _WIN32_IE_IE70
#endif
#endif

#include "nsILocalFile.h"
#include "nsISimpleEnumerator.h"
#include "nsCOMArray.h"
#include "nsAutoPtr.h"
#include "nsICharsetConverterManager.h"
#include "nsBaseFilePicker.h"
#include "nsString.h"
#include "nsdefs.h"
#include <commdlg.h>
#include <shobjidl.h>





class nsFilePicker : public nsBaseFilePicker
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
                     , public IFileDialogEvents
#endif
{
public:
  nsFilePicker(); 
  virtual ~nsFilePicker();

  NS_DECL_ISUPPORTS
  
#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  
  STDMETHODIMP QueryInterface(REFIID refiid, void** ppvResult);
#endif

  
  NS_IMETHOD GetDefaultString(nsAString& aDefaultString);
  NS_IMETHOD SetDefaultString(const nsAString& aDefaultString);
  NS_IMETHOD GetDefaultExtension(nsAString& aDefaultExtension);
  NS_IMETHOD SetDefaultExtension(const nsAString& aDefaultExtension);
  NS_IMETHOD GetFilterIndex(PRInt32 *aFilterIndex);
  NS_IMETHOD SetFilterIndex(PRInt32 aFilterIndex);
  NS_IMETHOD GetFile(nsILocalFile * *aFile);
  NS_IMETHOD GetFileURL(nsIURI * *aFileURL);
  NS_IMETHOD GetFiles(nsISimpleEnumerator **aFiles);
  NS_IMETHOD Show(PRInt16 *aReturnVal); 
  NS_IMETHOD ShowW(PRInt16 *aReturnVal); 
  NS_IMETHOD AppendFilter(const nsAString& aTitle, const nsAString& aFilter);

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
  
  HRESULT STDMETHODCALLTYPE OnFileOk(IFileDialog *pfd);
  HRESULT STDMETHODCALLTYPE OnFolderChanging(IFileDialog *pfd, IShellItem *psiFolder);
  HRESULT STDMETHODCALLTYPE OnFolderChange(IFileDialog *pfd);
  HRESULT STDMETHODCALLTYPE OnSelectionChange(IFileDialog *pfd);
  HRESULT STDMETHODCALLTYPE OnShareViolation(IFileDialog *pfd, IShellItem *psi, FDE_SHAREVIOLATION_RESPONSE *pResponse);
  HRESULT STDMETHODCALLTYPE OnTypeChange(IFileDialog *pfd);
  HRESULT STDMETHODCALLTYPE OnOverwrite(IFileDialog *pfd, IShellItem *psi, FDE_OVERWRITE_RESPONSE *pResponse);
#endif

protected:
  enum PickerType {
    PICKER_TYPE_OPEN,
    PICKER_TYPE_SAVE,
  };

  
  virtual void InitNative(nsIWidget *aParent,
                          const nsAString& aTitle,
                          PRInt16 aMode);
  static void GetQualifiedPath(const PRUnichar *aInPath, nsString &aOutPath);
  void GetFilterListArray(nsString& aFilterList);
  bool FilePickerWrapper(OPENFILENAMEW* ofn, PickerType aType);
  bool ShowFolderPicker(const nsString& aInitialDir);
  bool ShowXPFolderPicker(const nsString& aInitialDir);
  bool ShowFilePicker(const nsString& aInitialDir);
  bool ShowXPFilePicker(const nsString& aInitialDir);
  void RememberLastUsedDirectory();
  bool IsPrivacyModeEnabled();
  bool IsDefaultPathLink();
  bool IsDefaultPathHtml();

  nsCOMPtr<nsIWidget>    mParentWidget;
  nsString               mTitle;
  PRInt16                mMode;
  nsCString              mFile;
  nsString               mDefault;
  nsString               mDefaultExtension;
  nsString               mFilterList;
  PRInt16                mSelectedType;
  nsCOMArray<nsILocalFile> mFiles;
  static char            mLastUsedDirectory[];
  nsString               mUnicodeFile;
  static PRUnichar      *mLastUsedUnicodeDirectory;
  DWORD                  mFDECookie;
};

#if MOZ_WINSDK_TARGETVER >= MOZ_NTDDI_LONGHORN
#if defined(_WIN32_WINNT_bak)
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_bak
#undef _WIN32_IE
#define _WIN32_IE _WIN32_IE_bak
#endif
#endif

#endif
