



#include "chrome/common/chrome_switches.h"

#include "base/base_switches.h"

namespace switches {





const wchar_t kDisableHangMonitor[]            = L"disable-hang-monitor";


const wchar_t kDisableMetrics[]                = L"disable-metrics";






const wchar_t kMetricsRecordingOnly[]          = L"metrics-recording-only";


const wchar_t kBrowserAssertTest[]             = L"assert-test";


const wchar_t kRendererAssertTest[]            = L"renderer-assert-test";


const wchar_t kBrowserCrashTest[]              = L"crash-test";


const wchar_t kRendererCrashTest[]             = L"renderer-crash-test";


const wchar_t kRendererStartupDialog[]         = L"renderer-startup-dialog";


const wchar_t kPluginStartupDialog[]           = L"plugin-startup-dialog";




const wchar_t kPluginLauncher[]                = L"plugin-launcher";



const wchar_t kProcessChannelID[]              = L"channel";



const wchar_t kTestingChannelID[]              = L"testing-channel";




const wchar_t kHomePage[]                      = L"homepage";


const wchar_t kRendererProcess[]               = L"renderer";


const wchar_t kBrowserSubprocessPath[]         = L"browser-subprocess-path";


const wchar_t kPluginProcess[]                 = L"plugin";


const wchar_t kWorkerProcess[]                 = L"worker";


const wchar_t kSingleProcess[]                 = L"single-process";





const wchar_t kProcessPerTab[]                 = L"process-per-tab";





const wchar_t kProcessPerSite[]                = L"process-per-site";


const wchar_t kInProcessPlugins[]              = L"in-process-plugins";


const wchar_t kNoSandbox[]                     = L"no-sandbox";


const wchar_t kSafePlugins[]                   = L"safe-plugins";



const wchar_t kTrustedPlugins[]                = L"trusted-plugins";


const wchar_t kTestSandbox[]                   = L"test-sandbox";



const wchar_t kUserDataDir[]                   = L"user-data-dir";



const wchar_t kPluginDataDir[]                 = L"plugin-data-dir";



const wchar_t kDiskCacheDir[]                  = L"disk-cache-dir";



const wchar_t kEnableUserDataDirProfiles[]     = L"enable-udd-profiles";


const wchar_t kParentProfile[]                 = L"parent-profile";


const wchar_t kApp[]                           = L"app";





const wchar_t kDomAutomationController[]       = L"dom-automation";


const wchar_t kPluginPath[]                    = L"plugin-path";


const wchar_t kUserAgent[]                     = L"user-agent";


const wchar_t kJavaScriptFlags[]               = L"js-flags";




const wchar_t kCountry[]                       = L"country";



const wchar_t kLang[]                          = L"lang";




const wchar_t kDebugChildren[]                 = L"debug-children";




const wchar_t kWaitForDebuggerChildren[]       = L"wait-for-debugger-children";



const wchar_t kLogFilterPrefix[]               = L"log-filter-prefix";



const wchar_t kEnableLogging[]                 = L"enable-logging";



const wchar_t kDisableLogging[]                = L"disable-logging";



const wchar_t kLoggingLevel[]                  = L"log-level";


const wchar_t kLogPluginMessages[]             = L"log-plugin-messages";




const wchar_t kDumpHistogramsOnExit[]          = L"dump-histograms-on-exit";


const wchar_t kRemoteShellPort[]               = L"remote-shell-port";


const wchar_t kUninstall[]                     = L"uninstall";


const wchar_t kOmniBoxPopupCount[]             = L"omnibox-popup-count";



const wchar_t kAutomationClientChannelID[]     = L"automation-channel";





const wchar_t kRestoreLastSession[]            = L"restore-last-session";




const wchar_t kRecordMode[]                    = L"record-mode";
const wchar_t kPlaybackMode[]                  = L"playback-mode";


const wchar_t kNoEvents[]                      = L"no-events";






const wchar_t kNoJsRandomness[]              = L"no-js-randomness";





const wchar_t kHideIcons[]                     = L"hide-icons";

const wchar_t kShowIcons[]                     = L"show-icons";


const wchar_t kMakeDefaultBrowser[]            = L"make-default-browser";



const wchar_t kProxyServer[]                   = L"proxy-server";



const wchar_t kWinHttpProxyResolver[]               = L"winhttp-proxy-resolver";



extern const wchar_t kDnsLogDetails[]          = L"dns-log-details";
extern const wchar_t kDnsPrefetchDisable[]     = L"dns-prefetch-disable";


const wchar_t kDebugPrint[]                    = L"debug-print";





const wchar_t kAllowAllActiveX[]               = L"allow-all-activex";


const wchar_t kDisableDevTools[]               = L"disable-dev-tools";



const wchar_t kAlwaysEnableDevTools[]          = L"always-enable-dev-tools";



const wchar_t kTabCountToLoadOnSessionRestore[] =
    L"tab-count-to-load-on-session-restore";



const wchar_t kMemoryProfiling[]               = L"memory-profile";





const wchar_t kMemoryModel[]                   = L"memory-model";



const wchar_t kEnableFileCookies[]             = L"enable-file-cookies";


const wchar_t kStartMaximized[]                = L"start-maximized";





const wchar_t kEnableWatchdog[]                = L"enable-watchdog";



const wchar_t kFirstRun[]                      = L"first-run";




const wchar_t kNoFirstRun[]                    = L"no-first-run";




const wchar_t kMessageLoopHistogrammer[]       = L"message-loop-histogrammer";



const wchar_t kImport[]                        = L"import";




const wchar_t kSilentDumpOnDCHECK[]            = L"silent-dump-on-dcheck";




const wchar_t kDisablePromptOnRepost[]         = L"disable-prompt-on-repost";


const wchar_t kDisablePopupBlocking[]          = L"disable-popup-blocking";


const wchar_t kDisableJavaScript[]             = L"disable-javascript";


const wchar_t kDisableWebSecurity[]            = L"disable-web-security";


const wchar_t kDisableJava[]                   = L"disable-java";


const wchar_t kDisablePlugins[]                = L"disable-plugins";


const wchar_t kDisableImages[]                 = L"disable-images";


const wchar_t kUseLowFragHeapCrt[]             = L"use-lf-heap";

#ifndef NDEBUG

const wchar_t kGearsPluginPathOverride[]       = L"gears-plugin-path";
#endif


const wchar_t kEnableFastback[]                = L"enable-fastback";


const wchar_t kJavaScriptDebuggerPath[]        = L"javascript-debugger-path";

const wchar_t kDisableP13n[]                   = L"disable-p13n";






const wchar_t kSdchFilter[]                    = L"enable-sdch";


const wchar_t kEnableUserScripts[]             = L"enable-user-scripts";


const wchar_t kEnableExtensions[]              = L"enable-extensions";



const wchar_t kInstallExtension[]              = L"install-extension";


const wchar_t kLoadExtension[]                 = L"load-extension";


const wchar_t kLoadPlugin[]                    = L"load-plugin";


const wchar_t kUserScriptsDir[]                = L"user-scripts-dir";


const wchar_t kIncognito[]                     = L"incognito";



const wchar_t kEnableRendererAccessibility[] = L"enable-renderer-accessibility";


const wchar_t kTestName[]                      = L"test-name";



const wchar_t kRendererCmdPrefix[]             = L"renderer-cmd-prefix";


const wchar_t kNewFtp[]                        = L"new-ftp";



const wchar_t kIPCUseFIFO[]                    = L"ipc-use-fifo";



const wchar_t kEnableOutOfProcessDevTools[]    = L"enable-oop-devtools";


const wchar_t kEnableWebWorkers[]              = L"enable-web-workers";


const wchar_t kWebWorkerProcessPerCore[]       = L"web-worker-process-per-core";



const wchar_t kWebWorkerShareProcesses[]       = L"web-worker-share-processes";


const wchar_t kViewsGtk[]                      = L"views-gtk";


const wchar_t kBookmarkMenu[]                  = L"bookmark-menu";


const wchar_t kEnableStatsTable[]              = L"enable-stats-table";


const wchar_t kEnableOmnibox2[]                = L"enable-omnibox2";







const wchar_t kDisableAudio[]                  = L"disable-audio";






const wchar_t kSimpleDataSource[]              = L"simple-data-source";

}  
