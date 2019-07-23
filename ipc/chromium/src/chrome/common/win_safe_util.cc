



#include <shlobj.h>
#include <shobjidl.h>
#include <atlcomcli.h>

#include "chrome/common/win_safe_util.h"

#include "base/file_path.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "chrome/common/win_util.h"

namespace win_util {






bool SaferOpenItemViaShell(HWND hwnd, const std::wstring& window_title,
                           const FilePath& full_path,
                           const std::wstring& source_url,
                           bool ask_for_app) {
  ATL::CComPtr<IAttachmentExecute> attachment_services;
  HRESULT hr = attachment_services.CoCreateInstance(CLSID_AttachmentServices);
  if (FAILED(hr)) {
    
    
    if (hr == CO_E_NOTINITIALIZED) {
      NOTREACHED();
      return false;
    }
    return OpenItemViaShell(full_path, ask_for_app);
  }

  
  
  
  static const GUID kClientID = { 0x2676a9a2, 0xd919, 0x4fee,
    { 0x91, 0x87, 0x15, 0x21, 0x0, 0x39, 0x3a, 0xb2 } };

  attachment_services->SetClientGuid(kClientID);

  if (!window_title.empty())
    attachment_services->SetClientTitle(window_title.c_str());

  
  
  
  
  hr = attachment_services->SetLocalPath(full_path.value().c_str());
  if (FAILED(hr))
    return false;
  
  hr = attachment_services->SetSource(source_url.c_str());
  if (FAILED(hr))
    return false;

  
  bool do_prompt;
  hr = attachment_services->CheckPolicy();
  if (S_FALSE == hr) {
    
    do_prompt = true;
  } else if (S_OK == hr) {
    
    do_prompt = false;
  } else {
    
    
    
    
    
    
    
    
    
  }
  if (do_prompt) {
    ATTACHMENT_ACTION action;
    
    
    
    hr = attachment_services->Prompt(hwnd, ATTACHMENT_PROMPT_EXEC, &action);
    if (FAILED(hr) || (ATTACHMENT_ACTION_CANCEL == action))
    {
      
      return false;
    }
  }
  return OpenItemViaShellNoZoneCheck(full_path, ask_for_app);
}

bool SetInternetZoneIdentifier(const FilePath& full_path) {
  const DWORD kShare = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
  std::wstring path = full_path.value() + L":Zone.Identifier";
  HANDLE file = CreateFile(path.c_str(), GENERIC_WRITE, kShare, NULL,
                           OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (INVALID_HANDLE_VALUE == file)
    return false;

  const char kIdentifier[] = "[ZoneTransfer]\nZoneId=3";
  DWORD written = 0;
  BOOL result = WriteFile(file, kIdentifier, arraysize(kIdentifier), &written,
                          NULL);
  CloseHandle(file);

  if (!result || written != arraysize(kIdentifier)) {
    DCHECK(FALSE);
    return false;
  }

  return true;
}

}  
