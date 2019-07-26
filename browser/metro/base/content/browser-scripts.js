




Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");





XPCOMUtils.defineLazyModuleGetter(this, "Downloads",
                                  "resource://gre/modules/Downloads.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FormHistory",
                                  "resource://gre/modules/FormHistory.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageThumbs",
                                  "resource://gre/modules/PageThumbs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PluralForm",
                                  "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PdfJs",
                                  "resource://pdf.js/PdfJs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadUtils",
                                  "resource://gre/modules/DownloadUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
                                  "resource://gre/modules/NewTabUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");





XPCOMUtils.defineLazyServiceGetter(this, "StyleSheetSvc",
                                   "@mozilla.org/content/style-sheet-service;1",
                                   "nsIStyleSheetService");
XPCOMUtils.defineLazyServiceGetter(window, "gHistSvc",
                                   "@mozilla.org/browser/nav-history-service;1",
                                   "nsINavHistoryService",
                                   "nsIBrowserHistory");
XPCOMUtils.defineLazyServiceGetter(window, "gURIFixup",
                                   "@mozilla.org/docshell/urifixup;1",
                                   "nsIURIFixup");
XPCOMUtils.defineLazyServiceGetter(window, "gFaviconService",
                                   "@mozilla.org/browser/favicon-service;1",
                                   "nsIFaviconService");
XPCOMUtils.defineLazyServiceGetter(window, "gFocusManager",
                                   "@mozilla.org/focus-manager;1",
                                   "nsIFocusManager");
#ifdef MOZ_CRASHREPORTER
XPCOMUtils.defineLazyServiceGetter(this, "CrashReporter",
                                   "@mozilla.org/xre/app-info;1",
                                   "nsICrashReporter");
#endif






Cu.import("resource://gre/modules/Geometry.jsm");



let ScriptContexts = {};
[
  ["ContentAreaObserver", "chrome://browser/content/ContentAreaObserver.js"],
  ["WebProgress", "chrome://browser/content/WebProgress.js"],
  ["FindHelperUI", "chrome://browser/content/helperui/FindHelperUI.js"],
  ["FormHelperUI", "chrome://browser/content/helperui/FormHelperUI.js"],
  ["BrowserTouchHandler", "chrome://browser/content/BrowserTouchHandler.js"],
  ["AlertsHelper", "chrome://browser/content/helperui/AlertsHelper.js"],
  ["AutofillMenuUI", "chrome://browser/content/helperui/MenuUI.js"],
  ["ContextMenuUI", "chrome://browser/content/helperui/MenuUI.js"],
  ["MenuControlUI", "chrome://browser/content/helperui/MenuUI.js"],
  ["MenuPopup", "chrome://browser/content/helperui/MenuUI.js"],
  ["IndexedDB", "chrome://browser/content/helperui/IndexedDB.js"],
  ["OfflineApps", "chrome://browser/content/helperui/OfflineApps.js"],
  ["SelectHelperUI", "chrome://browser/content/helperui/SelectHelperUI.js"],
  ["SelectionHelperUI", "chrome://browser/content/helperui/SelectionHelperUI.js"],
  ["SelectionPrototype", "chrome://browser/content/library/SelectionPrototype.js"],
  ["ChromeSelectionHandler", "chrome://browser/content/helperui/ChromeSelectionHandler.js"],
  ["AnimatedZoom", "chrome://browser/content/AnimatedZoom.js"],
  ["CommandUpdater", "chrome://browser/content/commandUtil.js"],
  ["ContextCommands", "chrome://browser/content/ContextCommands.js"],
  ["Bookmarks", "chrome://browser/content/bookmarks.js"],
  ["MetroDownloadsView", "chrome://browser/content/downloads.js"],
  ["ConsolePanelView", "chrome://browser/content/console.js"],
  ["Site", "chrome://browser/content/Site.js"],
  ["TopSites", "chrome://browser/content/TopSites.js"],
  ["Sanitizer", "chrome://browser/content/sanitize.js"],
  ["SanitizeUI", "chrome://browser/content/sanitizeUI.js"],
  ["SSLExceptions", "chrome://browser/content/exceptions.js"],
  ["ItemPinHelper", "chrome://browser/content/helperui/ItemPinHelper.js"],
  ["NavButtonSlider", "chrome://browser/content/NavButtonSlider.js"],
  ["ContextUI", "chrome://browser/content/ContextUI.js"],
  ["FlyoutPanelsUI", "chrome://browser/content/flyoutpanels/FlyoutPanelsUI.js"],
  ["APZCObserver", "chrome://browser/content/apzc.js"],
].forEach(function (aScript) {
  let [name, script] = aScript;
  XPCOMUtils.defineLazyGetter(window, name, function() {
    let sandbox;
    if (script in ScriptContexts) {
      sandbox = ScriptContexts[script];
    } else {
      sandbox = ScriptContexts[script] = {};
      Services.scriptloader.loadSubScript(script, sandbox);
    }
    return sandbox[name];
  });
});
#ifdef MOZ_SERVICES_SYNC
XPCOMUtils.defineLazyGetter(this, "Weave", function() {
  Components.utils.import("resource://services-sync/main.js");
  return Weave;
});
#endif




XPCOMUtils.defineLazyGetter(this, "GlobalOverlay", function() {
  let GlobalOverlay = {};
  Services.scriptloader.loadSubScript("chrome://global/content/globalOverlay.js", GlobalOverlay);
  return GlobalOverlay;
});

XPCOMUtils.defineLazyGetter(this, "ContentAreaUtils", function() {
  let ContentAreaUtils = {};
  Services.scriptloader.loadSubScript("chrome://global/content/contentAreaUtils.js", ContentAreaUtils);
  return ContentAreaUtils;
});

XPCOMUtils.defineLazyGetter(this, "ZoomManager", function() {
  let sandbox = {};
  Services.scriptloader.loadSubScript("chrome://global/content/viewZoomOverlay.js", sandbox);
  return sandbox.ZoomManager;
});
