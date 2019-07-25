







































#include "nsCOMPtr.h"
#include "nsGUIEvent.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsWindow.h"
#include "nsIServiceManager.h"
#include "nsIPlatformCharset.h"
#include "nsICharsetConverterManager.h"
#include "nsIPrivateBrowsingService.h"
#include "nsFilePicker.h"
#include "nsILocalFile.h"
#include "nsIURL.h"
#include "nsIStringBundle.h"
#include "nsEnumeratorUtils.h"
#include "nsCRT.h"
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>


#include <commdlg.h>
#include <cderr.h>

#include "nsString.h"
#include "nsToolkit.h"

NS_IMPL_ISUPPORTS1(nsFilePicker, nsIFilePicker)

PRUnichar *nsFilePicker::mLastUsedUnicodeDirectory;
char nsFilePicker::mLastUsedDirectory[MAX_PATH+1] = { 0 };

#define MAX_EXTENSION_LENGTH 10

nsFilePicker::nsFilePicker()
{
  mSelectedType   = 1;
}

nsFilePicker::~nsFilePicker()
{
  if (mLastUsedUnicodeDirectory) {
    NS_Free(mLastUsedUnicodeDirectory);
    mLastUsedUnicodeDirectory = nsnull;
  }
}


int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  if (uMsg == BFFM_INITIALIZED)
  {
    PRUnichar * filePath = (PRUnichar *) lpData;
    if (filePath)
      ::SendMessageW(hwnd, BFFM_SETSELECTIONW,
                     TRUE ,
                     lpData);
  }
  return 0;
}

static void EnsureWindowVisible(HWND hwnd) 
{
  
  
  HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
  if (!monitor) {
    
    HWND parentHwnd = GetParent(hwnd);
    RECT parentRect;
    GetWindowRect(parentHwnd, &parentRect);
    BOOL b = SetWindowPos(hwnd, NULL, 
                          parentRect.left, 
                          parentRect.top, 0, 0, 
                          SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
  }
}


static UINT_PTR CALLBACK FilePickerHook(HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam) 
{
  if (msg == WM_NOTIFY) {
    LPOFNOTIFYW lpofn = (LPOFNOTIFYW) lParam;
    if (!lpofn || !lpofn->lpOFN) {
      return 0;
    }
    
    if (CDN_INITDONE == lpofn->hdr.code) {
      
      
      
      PostMessage(hwnd, MOZ_WM_ENSUREVISIBLE, 0, 0);
    }
  } else if (msg == MOZ_WM_ENSUREVISIBLE) {
    EnsureWindowVisible(GetParent(hwnd));
  }
  return 0;
}




static UINT_PTR CALLBACK MultiFilePickerHook(HWND hwnd, UINT msg,
                                             WPARAM wParam, LPARAM lParam)
{
  switch (msg) {
    case WM_INITDIALOG:
      {
        
        
        
        HWND comboBox = FindWindowEx(GetParent(hwnd), NULL, 
                                     L"ComboBoxEx32", NULL );
        if(comboBox)
          SendMessage(comboBox, CB_LIMITTEXT, 0, 0);
      }
      break;
    case WM_NOTIFY:
      {
        LPOFNOTIFYW lpofn = (LPOFNOTIFYW) lParam;
        if (!lpofn || !lpofn->lpOFN) {
          return 0;
        }
        
        
        if (lpofn->hdr.code == CDN_SELCHANGE) {
          HWND parentHWND = GetParent(hwnd);

          
          UINT newBufLength = 0; 
          int requiredBufLength = CommDlg_OpenSave_GetSpecW(parentHWND, 
                                                            NULL, 0);
          if(requiredBufLength >= 0)
            newBufLength += requiredBufLength;
          else
            newBufLength += MAX_PATH;

          
          
          
          
          requiredBufLength = CommDlg_OpenSave_GetFolderPathW(parentHWND, 
                                                              NULL, 0);
          if(requiredBufLength >= 0)
            newBufLength += requiredBufLength;
          else
            newBufLength += MAX_PATH;

          
          if (newBufLength > lpofn->lpOFN->nMaxFile) {
            if (lpofn->lpOFN->lpstrFile)
              delete[] lpofn->lpOFN->lpstrFile;

            
            
            
            newBufLength += FILE_BUFFER_SIZE;

            PRUnichar* filesBuffer = new PRUnichar[newBufLength];
            ZeroMemory(filesBuffer, newBufLength * sizeof(PRUnichar));

            lpofn->lpOFN->lpstrFile = filesBuffer;
            lpofn->lpOFN->nMaxFile  = newBufLength;
          }
        }
      }
      break;
  }

  return FilePickerHook(hwnd, msg, wParam, lParam);
}

NS_IMETHODIMP nsFilePicker::ShowW(PRInt16 *aReturnVal)
{
  NS_ENSURE_ARG_POINTER(aReturnVal);

  
  if (mParentWidget) {
    nsIWidget *tmp = mParentWidget;
    nsWindow *parent = static_cast<nsWindow *>(tmp);
    parent->SuppressBlurEvents(true);
  }

  bool result = false;
  nsAutoArrayPtr<PRUnichar> fileBuffer(new PRUnichar[FILE_BUFFER_SIZE+1]);
            
  wcsncpy(fileBuffer,  mDefault.get(), FILE_BUFFER_SIZE);
  fileBuffer[FILE_BUFFER_SIZE] = '\0'; 

  NS_NAMED_LITERAL_STRING(htmExt, "html");
  nsAutoString initialDir;
  if (mDisplayDirectory)
    mDisplayDirectory->GetPath(initialDir);

  
  if(initialDir.IsEmpty()) {
    
    initialDir = mLastUsedUnicodeDirectory;
  }

  mUnicodeFile.Truncate();

  if (mMode == modeGetFolder) {
    PRUnichar dirBuffer[MAX_PATH+1];
    wcsncpy(dirBuffer, initialDir.get(), MAX_PATH);

    BROWSEINFOW browserInfo;
    browserInfo.hwndOwner      = (HWND)
      (mParentWidget.get() ? mParentWidget->GetNativeData(NS_NATIVE_TMP_WINDOW) : 0); 
    browserInfo.pidlRoot       = nsnull;
    browserInfo.pszDisplayName = (LPWSTR)dirBuffer;
    browserInfo.lpszTitle      = mTitle.get();
    browserInfo.ulFlags        = BIF_USENEWUI | BIF_RETURNONLYFSDIRS;
    if (initialDir.Length()) {
      
      
      browserInfo.lParam       = (LPARAM) initialDir.get();
      browserInfo.lpfn         = &BrowseCallbackProc;
    } else {
    browserInfo.lParam         = nsnull;
      browserInfo.lpfn         = nsnull;
    }
    browserInfo.iImage         = nsnull;

    LPITEMIDLIST list = ::SHBrowseForFolderW(&browserInfo);
    if (list != NULL) {
      result = ::SHGetPathFromIDListW(list, (LPWSTR)fileBuffer);
      if (result) {
          mUnicodeFile.Assign(fileBuffer);
      }
  
      
      CoTaskMemFree(list);
    }
  } else {

    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    nsString filterBuffer = mFilterList;
                                  
    if (!initialDir.IsEmpty()) {
      ofn.lpstrInitialDir = initialDir.get();
    }
    
    ofn.lpstrTitle   = (LPCWSTR)mTitle.get();
    ofn.lpstrFilter  = (LPCWSTR)filterBuffer.get();
    ofn.nFilterIndex = mSelectedType;
    ofn.hwndOwner    = (HWND) (mParentWidget.get() ? mParentWidget->GetNativeData(NS_NATIVE_TMP_WINDOW) : 0); 
    ofn.lpstrFile    = fileBuffer;
    ofn.nMaxFile     = FILE_BUFFER_SIZE;
    ofn.Flags = OFN_SHAREAWARE | OFN_LONGNAMES | OFN_OVERWRITEPROMPT |
                OFN_HIDEREADONLY | OFN_PATHMUSTEXIST | OFN_ENABLESIZING | 
                OFN_EXPLORER;

    
    
    
    
    if (nsWindow::GetWindowsVersion() < VISTA_VERSION) {
      ofn.lpfnHook = FilePickerHook;
      ofn.Flags |= OFN_ENABLEHOOK;
    }

    
    nsCOMPtr<nsIPrivateBrowsingService> pbs =
      do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
    bool privacyModeEnabled = false;
    if (pbs) {
      pbs->GetPrivateBrowsingEnabled(&privacyModeEnabled);
    }
    if (privacyModeEnabled || !mAddToRecentDocs) {
      ofn.Flags |= OFN_DONTADDTORECENT;
    }

    if (!mDefaultExtension.IsEmpty()) {
      ofn.lpstrDefExt = mDefaultExtension.get();
    }
    else {
      
      
      
      PRInt32 extIndex = mDefault.RFind(".");
      if ( extIndex >= 0) {
        nsAutoString ext;
        mDefault.Right(ext, mDefault.Length() - extIndex);
        
        

        if ( ext.LowerCaseEqualsLiteral(".htm")  ||
             ext.LowerCaseEqualsLiteral(".html") ||
             ext.LowerCaseEqualsLiteral(".shtml") ) {
          
          
          
          
          ofn.lpstrDefExt = htmExt.get();
        }
      }
    }

    
    
    
    
    
    
    AutoRestoreWorkingPath restoreWorkingPath;
    
    
    if (!restoreWorkingPath.HasWorkingPath()) {
      ofn.Flags |= OFN_NOCHANGEDIR;
    }
    
    MOZ_SEH_TRY {
      if (mMode == modeOpen) {
        
        ofn.Flags |= OFN_FILEMUSTEXIST;
        result = ::GetOpenFileNameW(&ofn);
      }
      else if (mMode == modeOpenMultiple) {
        ofn.Flags |= OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT;

        
        
        
        
        
        
        
        
        
        
        if (nsWindow::GetWindowsVersion() < VISTA_VERSION) {
          ofn.lpfnHook = MultiFilePickerHook;
          fileBuffer.forget();
          result = ::GetOpenFileNameW(&ofn);
          fileBuffer = ofn.lpstrFile;
        }
        else {
          result = ::GetOpenFileNameW(&ofn);
        }
      }
      else if (mMode == modeSave) {
        ofn.Flags |= OFN_NOREADONLYRETURN;

        
        
        NS_ConvertUTF16toUTF8 ext(mDefault);
        ext.Trim(" .", false, true); 
        ToLowerCase(ext);
        if (StringEndsWith(ext, NS_LITERAL_CSTRING(".lnk")) ||
            StringEndsWith(ext, NS_LITERAL_CSTRING(".pif")) ||
            StringEndsWith(ext, NS_LITERAL_CSTRING(".url")))
          ofn.Flags |= OFN_NODEREFERENCELINKS;

        result = ::GetSaveFileNameW(&ofn);
        if (!result) {
          
          if (::GetLastError() == ERROR_INVALID_PARAMETER 
              || ::CommDlgExtendedError() == FNERR_INVALIDFILENAME
              ) {
            
            
            ofn.lpstrFile[0] = 0;
            result = ::GetSaveFileNameW(&ofn);
          }
        }
      } 
      else {
        NS_ERROR("unsupported mode"); 
      }
    }
    MOZ_SEH_EXCEPT(true) {
      MessageBoxW(ofn.hwndOwner,
                  0,
                  L"The filepicker was unexpectedly closed by Windows.",
                  MB_ICONERROR);
      result = false;
    }

    if (result) {
      
      mSelectedType = (PRInt16)ofn.nFilterIndex;

      
      mFiles.Clear();

      
      if (mMode == modeOpenMultiple) {
        
        
        
        
        
        
        
        PRUnichar *current = fileBuffer;
        
        nsAutoString dirName(current);
        
        
        if (current[dirName.Length() - 1] != '\\')
          dirName.Append((PRUnichar)'\\');
        
        nsresult rv;
        while (current && *current && *(current + nsCRT::strlen(current) + 1)) {
          current = current + nsCRT::strlen(current) + 1;
          
          nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1", &rv);
          NS_ENSURE_SUCCESS(rv,rv);

          
          nsAutoString path;
          if (PathIsRelativeW(current)) {
            path = dirName + nsDependentString(current);
          } else {
            path = current;
          }

          nsAutoString canonicalizedPath;
          GetQualifiedPath(path.get(), canonicalizedPath);
          
          rv = file->InitWithPath(canonicalizedPath);
          NS_ENSURE_SUCCESS(rv,rv);
          
          rv = mFiles.AppendObject(file);
          NS_ENSURE_SUCCESS(rv,rv);
        }
        
        
        
        
        
        
        if (current && *current && (current == fileBuffer)) {
          nsCOMPtr<nsILocalFile> file = do_CreateInstance("@mozilla.org/file/local;1", &rv);
          NS_ENSURE_SUCCESS(rv,rv);
          
          nsAutoString canonicalizedPath;
          GetQualifiedPath(current, canonicalizedPath);
          rv = file->InitWithPath(canonicalizedPath);
          NS_ENSURE_SUCCESS(rv,rv);
          
          rv = mFiles.AppendObject(file);
          NS_ENSURE_SUCCESS(rv,rv);
        }
      } else {
        GetQualifiedPath(fileBuffer, mUnicodeFile);
      }
    }
    if (ofn.hwndOwner) {
      ::DestroyWindow(ofn.hwndOwner);
    }
  }

  if (result) {
    PRInt16 returnOKorReplace = returnOK;

    
    nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

    
    file->InitWithPath(mUnicodeFile);
    nsCOMPtr<nsIFile> dir;
    if (NS_SUCCEEDED(file->GetParent(getter_AddRefs(dir)))) {
      mDisplayDirectory = do_QueryInterface(dir);
      if (mDisplayDirectory) {
        if (mLastUsedUnicodeDirectory) {
          NS_Free(mLastUsedUnicodeDirectory);
          mLastUsedUnicodeDirectory = nsnull;
        }

        nsAutoString newDir;
        mDisplayDirectory->GetPath(newDir);
        if(!newDir.IsEmpty())
          mLastUsedUnicodeDirectory = ToNewUnicode(newDir);
      }
    }

    if (mMode == modeSave) {
      
      
      bool exists = false;
      file->Exists(&exists);

      if (exists)
        returnOKorReplace = returnReplace;
    }
    *aReturnVal = returnOKorReplace;
  }
  else {
    *aReturnVal = returnCancel;
  }
  if (mParentWidget) {
    nsIWidget *tmp = mParentWidget;
    nsWindow *parent = static_cast<nsWindow *>(tmp);
    parent->SuppressBlurEvents(false);
  }

  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::Show(PRInt16 *aReturnVal)
{
  return ShowW(aReturnVal);
}

NS_IMETHODIMP nsFilePicker::GetFile(nsILocalFile **aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  *aFile = nsnull;

  if (mUnicodeFile.IsEmpty())
      return NS_OK;

  nsCOMPtr<nsILocalFile> file(do_CreateInstance("@mozilla.org/file/local;1"));
    
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  file->InitWithPath(mUnicodeFile);

  NS_ADDREF(*aFile = file);

  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::GetFileURL(nsIURI **aFileURL)
{
  *aFileURL = nsnull;
  nsCOMPtr<nsILocalFile> file;
  nsresult rv = GetFile(getter_AddRefs(file));
  if (!file)
    return rv;

  return NS_NewFileURI(aFileURL, file);
}

NS_IMETHODIMP nsFilePicker::GetFiles(nsISimpleEnumerator **aFiles)
{
  NS_ENSURE_ARG_POINTER(aFiles);
  return NS_NewArrayEnumerator(aFiles, mFiles);
}


NS_IMETHODIMP nsFilePicker::SetDefaultString(const nsAString& aString)
{
  mDefault = aString;

  
  PRInt32 nameLength;
  PRInt32 nameIndex = mDefault.RFind("\\");
  if (nameIndex == kNotFound)
    nameIndex = 0;
  else
    nameIndex ++;
  nameLength = mDefault.Length() - nameIndex;
  
  if (nameLength > MAX_PATH) {
    PRInt32 extIndex = mDefault.RFind(".");
    if (extIndex == kNotFound)
      extIndex = mDefault.Length();

    
    PRInt32 charsToRemove = nameLength - MAX_PATH;
    if (extIndex - nameIndex >= charsToRemove) {
      mDefault.Cut(extIndex - charsToRemove, charsToRemove);
    }
  }

  
  
  mDefault.ReplaceChar(FILE_ILLEGAL_CHARACTERS, '-');

  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::GetDefaultString(nsAString& aString)
{
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsFilePicker::GetDefaultExtension(nsAString& aExtension)
{
  aExtension = mDefaultExtension;
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::SetDefaultExtension(const nsAString& aExtension)
{
  mDefaultExtension = aExtension;
  return NS_OK;
}


NS_IMETHODIMP nsFilePicker::GetFilterIndex(PRInt32 *aFilterIndex)
{
  
  *aFilterIndex = mSelectedType - 1;
  return NS_OK;
}

NS_IMETHODIMP nsFilePicker::SetFilterIndex(PRInt32 aFilterIndex)
{
  
  mSelectedType = aFilterIndex + 1;
  return NS_OK;
}

void nsFilePicker::InitNative(nsIWidget *aParent,
                              const nsAString& aTitle,
                              PRInt16 aMode)
{
  mParentWidget = aParent;
  mTitle.Assign(aTitle);
  mMode = aMode;
}

void 
nsFilePicker::GetQualifiedPath(const PRUnichar *aInPath, nsString &aOutPath)
{
  
  
  
  PRUnichar qualifiedFileBuffer[MAX_PATH];
  if (PathSearchAndQualifyW(aInPath, qualifiedFileBuffer, MAX_PATH)) {
    aOutPath.Assign(qualifiedFileBuffer);
  } else {
    aOutPath.Assign(aInPath);
  }
}

NS_IMETHODIMP
nsFilePicker::AppendFilter(const nsAString& aTitle, const nsAString& aFilter)
{
  mFilterList.Append(aTitle);
  mFilterList.Append(PRUnichar('\0'));

  if (aFilter.EqualsLiteral("..apps"))
    mFilterList.AppendLiteral("*.exe;*.com");
  else
  {
    nsAutoString filter(aFilter);
    filter.StripWhitespace();
    if (filter.EqualsLiteral("*"))
      filter.AppendLiteral(".*");
    mFilterList.Append(filter);
  }

  mFilterList.Append(PRUnichar('\0'));

  return NS_OK;
}

AutoRestoreWorkingPath::AutoRestoreWorkingPath() 
{
  DWORD bufferLength = GetCurrentDirectoryW(0, NULL);
  mWorkingPath = new PRUnichar[bufferLength];
  if (GetCurrentDirectoryW(bufferLength, mWorkingPath) == 0) {
    mWorkingPath = NULL;
  }
}

AutoRestoreWorkingPath::~AutoRestoreWorkingPath()
{
  if (HasWorkingPath()) {
    ::SetCurrentDirectoryW(mWorkingPath);
  }
}

