



#include "chrome/common/chrome_constants.h"

#include "base/file_path.h"

#define FPL FILE_PATH_LITERAL

namespace chrome {



#if defined(OS_WIN)
const wchar_t kBrowserProcessExecutableName[] = L"chrome.exe";
#elif defined(OS_LINUX)
const wchar_t kBrowserProcessExecutableName[] = L"chrome";
#elif defined(OS_MACOSX)
const wchar_t kBrowserProcessExecutableName[] =
#if defined(GOOGLE_CHROME_BUILD)
    L"Chrome";
#else
    L"Chromium";
#endif  
#endif  
#if defined(OS_WIN)
const wchar_t kBrowserProcessExecutablePath[] = L"chrome.exe";
#elif defined(OS_LINUX)
const wchar_t kBrowserProcessExecutablePath[] = L"chrome";
#elif defined(OS_MACOSX)
const wchar_t kBrowserProcessExecutablePath[] =
#if defined(GOOGLE_CHROME_BUILD)
    L"Chrome.app/Contents/MacOS/Chrome";
#else
    L"Chromium.app/Contents/MacOS/Chromium";
#endif  
#endif  
#if defined(GOOGLE_CHROME_BUILD)
const wchar_t kBrowserAppName[] = L"Chrome";
const char    kStatsFilename[] = "ChromeStats2";
#else
const wchar_t kBrowserAppName[] = L"Chromium";
const char    kStatsFilename[] = "ChromiumStats2";
#endif
const wchar_t kExternalTabWindowClass[] = L"Chrome_ExternalTabContainer";
const wchar_t kMessageWindowClass[] = L"Chrome_MessageWindow";
const wchar_t kCrashReportLog[] = L"Reported Crashes.txt";
const wchar_t kTestingInterfaceDLL[] = L"testing_interface.dll";
const wchar_t kNotSignedInProfile[] = L"Default";
const wchar_t kNotSignedInID[] = L"not-signed-in";
const wchar_t kBrowserResourcesDll[] = L"chrome.dll";
const FilePath::CharType kExtensionFileExtension[] = FPL("crx");


const FilePath::CharType kArchivedHistoryFilename[] = FPL("Archived History");
const FilePath::CharType kCacheDirname[] = FPL("Cache");
const FilePath::CharType kMediaCacheDirname[] = FPL("Media Cache");
const FilePath::CharType kOffTheRecordMediaCacheDirname[] =
    FPL("Incognito Media Cache");
const wchar_t kChromePluginDataDirname[] = L"Plugin Data";
const FilePath::CharType kCookieFilename[] = FPL("Cookies");
const FilePath::CharType kHistoryFilename[] = FPL("History");
const FilePath::CharType kLocalStateFilename[] = FPL("Local State");
const FilePath::CharType kPreferencesFilename[] = FPL("Preferences");
const FilePath::CharType kSafeBrowsingFilename[] = FPL("Safe Browsing");


const FilePath::CharType kSingletonSocketFilename[] = FPL("SingletonSocket");
const FilePath::CharType kThumbnailsFilename[] = FPL("Thumbnails");
const wchar_t kUserDataDirname[] = L"User Data";
const FilePath::CharType kUserScriptsDirname[] = FPL("User Scripts");
const FilePath::CharType kWebDataFilename[] = FPL("Web Data");
const FilePath::CharType kBookmarksFileName[] = FPL("Bookmarks");
const FilePath::CharType kHistoryBookmarksFileName[] =
    FPL("Bookmarks From History");
const FilePath::CharType kCustomDictionaryFileName[] =
    FPL("Custom Dictionary.txt");


const unsigned int kMaxRendererProcessCount = 42;
const int kStatsMaxThreads = 32;
const int kStatsMaxCounters = 300;





#ifndef NDEBUG
const bool kRecordModeEnabled = true;
#else
const bool kRecordModeEnabled = false;
#endif

}  
