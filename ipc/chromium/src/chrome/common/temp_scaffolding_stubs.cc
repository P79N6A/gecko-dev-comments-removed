



#include "temp_scaffolding_stubs.h"

#include "build/build_config.h"

#include <vector>

#include "base/logging.h"
#include "chrome/browser/automation/automation_provider.h"
#include "chrome/browser/download/download_request_dialog_delegate.h"
#include "chrome/browser/download/download_request_manager.h"
#include "chrome/browser/first_run.h"
#include "chrome/browser/hung_renderer_dialog.h"
#include "chrome/browser/memory_details.h"
#include "chrome/browser/rlz/rlz.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/task_manager.h"
#include "chrome/common/process_watcher.h"

#if defined(OS_MACOSX)
#include "chrome/browser/download/download_item_model.h"
#include "chrome/browser/download/download_shelf.h"
#endif

class TabContents;



void AutomationProvider::GetActiveWindow(int* handle) { NOTIMPLEMENTED(); }

void AutomationProvider::ActivateWindow(int handle) { NOTIMPLEMENTED(); }

void AutomationProvider::SetWindowVisible(int handle, bool visible,
                                          bool* result) { NOTIMPLEMENTED(); }

void AutomationProvider::GetFocusedViewID(int handle, int* view_id) {
  NOTIMPLEMENTED();
}

#if defined(OS_MACOSX)
void AutomationProvider::GetAutocompleteEditForBrowser(
    int browser_handle,
    bool* success,
    int* autocomplete_edit_handle) {
  *success = false;
  NOTIMPLEMENTED();
}
#endif

void AutomationProvider::GetBrowserForWindow(int window_handle,
                                             bool* success,
                                             int* browser_handle) {
  *success = false;
  NOTIMPLEMENTED();
}

void AutomationProvider::GetSecurityState(int handle, bool* success,
                                          SecurityStyle* security_style,
                                          int* ssl_cert_status,
                                          int* mixed_content_status) {
  *success = false;
  NOTIMPLEMENTED();
}

void AutomationProvider::GetPageType(int handle, bool* success,
                                     NavigationEntry::PageType* page_type) {
  *success = false;
  NOTIMPLEMENTED();
}

void AutomationProvider::ActionOnSSLBlockingPage(int handle, bool proceed,
                                                 IPC::Message* reply_message) {
  NOTIMPLEMENTED();
}

void AutomationProvider::PrintNow(int tab_handle,
                                  IPC::Message* reply_message) {
  NOTIMPLEMENTED();
}

#if defined(OS_MACOSX)
void AutomationProvider::GetAutocompleteEditText(int autocomplete_edit_handle,
                                                 bool* success,
                                                 std::wstring* text) {
  *success = false;
  NOTIMPLEMENTED();
}

void AutomationProvider::SetAutocompleteEditText(int autocomplete_edit_handle,
                                                 const std::wstring& text,
                                                 bool* success) {
  *success = false;
  NOTIMPLEMENTED();
}

void AutomationProvider::AutocompleteEditGetMatches(
    int autocomplete_edit_handle,
    bool* success,
    std::vector<AutocompleteMatchData>* matches) {
  *success = false;
  NOTIMPLEMENTED();
}

void AutomationProvider::AutocompleteEditIsQueryInProgress(
    int autocomplete_edit_handle,
    bool* success,
    bool* query_in_progress) {
  *success = false;
  NOTIMPLEMENTED();
}

void AutomationProvider::OnMessageFromExternalHost(
    int handle, const std::string& message, const std::string& origin,
    const std::string& target) {
  NOTIMPLEMENTED();
}
#endif  



#if defined(OS_LINUX)
bool ShellIntegration::SetAsDefaultBrowser() {
  NOTIMPLEMENTED();
  return true;
}

bool ShellIntegration::IsDefaultBrowser() {
  NOTIMPLEMENTED();
  return true;
}
#endif




bool FirstRun::IsChromeFirstRun() {
  NOTIMPLEMENTED();
  return false;
}


bool FirstRun::ProcessMasterPreferences(const FilePath& user_data_dir,
                                        const FilePath& master_prefs_path,
                                        int* preference_details) {
  NOTIMPLEMENTED();
  return false;
}


int FirstRun::ImportNow(Profile* profile, const CommandLine& cmdline) {
  NOTIMPLEMENTED();
  return 0;
}


bool Upgrade::IsBrowserAlreadyRunning() {
  
  return false;
}


bool Upgrade::RelaunchChromeBrowser(const CommandLine& command_line) {
  
  return true;
}


bool Upgrade::SwapNewChromeExeIfPresent() {
  
  return true;
}

void OpenFirstRunDialog(Profile* profile, ProcessSingleton* process_singleton) {
  NOTIMPLEMENTED();
}



void InstallJankometer(const CommandLine&) {
  
}

void UninstallJankometer() {
  
}



void RLZTracker::CleanupRlz() {
  
}

bool RLZTracker::GetAccessPointRlz(AccessPoint point, std::wstring* rlz) {
  
  return false;
}

bool RLZTracker::RecordProductEvent(Product product, AccessPoint point,
                                    Event event) {
  
  return false;
}



#if defined(OS_MACOSX)

class DownloadShelfMac : public DownloadShelf {
 public:
  explicit DownloadShelfMac(TabContents* tab_contents)
      : DownloadShelf(tab_contents) { }
  virtual void AddDownload(BaseDownloadItemModel* download_model) { }
  virtual bool IsShowing() const { return false; }
};


DownloadShelf* DownloadShelf::Create(TabContents* tab_contents) {
  return new DownloadShelfMac(tab_contents);
}

#endif



void RunRepostFormWarningDialog(NavigationController*) {
}

LoginHandler* CreateLoginPrompt(net::AuthChallengeInfo* auth_info,
                                URLRequest* request,
                                MessageLoop* ui_loop) {
  NOTIMPLEMENTED();
  return NULL;
}



MemoryDetails::MemoryDetails() {
  NOTIMPLEMENTED();
}

void MemoryDetails::StartFetch() {
  NOTIMPLEMENTED();
}

#if defined(OS_MACOSX)
InfoBar* ConfirmInfoBarDelegate::CreateInfoBar() {
  NOTIMPLEMENTED();
  return NULL;
}

InfoBar* AlertInfoBarDelegate::CreateInfoBar() {
  NOTIMPLEMENTED();
  return NULL;
}

InfoBar* LinkInfoBarDelegate::CreateInfoBar() {
  NOTIMPLEMENTED();
  return NULL;
}
#endif

DownloadRequestDialogDelegate* DownloadRequestDialogDelegate::Create(
    TabContents* tab,
    DownloadRequestManager::TabDownloadState* host) {
  NOTIMPLEMENTED();
  return NULL;
}

views::Window* CreateInputWindow(gfx::NativeWindow parent_hwnd,
                                 InputWindowDelegate* delegate) {
  NOTIMPLEMENTED();
  return new views::Window();
}

namespace download_util {

void DragDownload(const DownloadItem* download, SkBitmap* icon) {
  NOTIMPLEMENTED();
}

}  

void WindowSizer::GetBrowserWindowBounds(const std::wstring& app_name,
                                         const gfx::Rect& specified_bounds,
                                         Browser* browser,
                                         gfx::Rect* window_bounds,
                                         bool* maximized) {
  
  
  
  *window_bounds = specified_bounds;
  if (specified_bounds.IsEmpty()) {
    *window_bounds = gfx::Rect(0, 0, 1024, 768);
  }
}

void TaskManager::AddResource(Resource* resource) {
  NOTIMPLEMENTED();
}

void TaskManager::RemoveResource(Resource* resource) {
  NOTIMPLEMENTED();
}

void HungRendererDialog::HideForTabContents(TabContents*) {
  NOTIMPLEMENTED();
}

void HungRendererDialog::ShowForTabContents(TabContents*) {
  NOTIMPLEMENTED();
}
