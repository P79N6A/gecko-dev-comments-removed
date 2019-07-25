





































Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");




XPCOMUtils.defineLazyGetter(this, "PluralForm", function() {
  Cu.import("resource://gre/modules/PluralForm.jsm");
  return PluralForm;
});

XPCOMUtils.defineLazyGetter(this, "PlacesUtils", function() {
  Cu.import("resource://gre/modules/PlacesUtils.jsm");
  return PlacesUtils;
});




Cu.import("resource://gre/modules/Geometry.jsm");





XPCOMUtils.defineLazyGetter(this, "CommonUI", function() {
  let CommonUI = {};
  Services.scriptloader.loadSubScript("chrome://browser/content/common-ui.js", CommonUI);
  return CommonUI;
});

[
  ["FullScreenVideo"],
  ["BadgeHandlers"],
  ["ContextHelper"],
  ["FormHelperUI"],
  ["FindHelperUI"],
  ["NewTabPopup"],
  ["PageActions"],
  ["BrowserSearch"],
  ["CharsetMenu"]
].forEach(function (aObject) {
  XPCOMUtils.defineLazyGetter(window, aObject, function() {
    return CommonUI[aObject];
  });
});




[
  ["AlertsHelper", "chrome://browser/content/AlertsHelper.js"],
  ["AnimatedZoom", "chrome://browser/content/AnimatedZoom.js"],
  ["AppMenu", "chrome://browser/content/AppMenu.js"],
  ["AwesomePanel", "chrome://browser/content/AwesomePanel.js"],
  ["BookmarkHelper", "chrome://browser/content/BookmarkHelper.js"],
  ["BookmarkPopup", "chrome://browser/content/BookmarkPopup.js"],
  ["CommandUpdater", "chrome://browser/content/commandUtil.js"],
  ["ContextCommands", "chrome://browser/content/ContextCommands.js"],
  ["ConsoleView", "chrome://browser/content/console.js"],
  ["DownloadsView", "chrome://browser/content/downloads.js"],
  ["ExtensionsView", "chrome://browser/content/extensions.js"],
  ["MenuListHelperUI", "chrome://browser/content/MenuListHelperUI.js"],
  ["OfflineApps", "chrome://browser/content/OfflineApps.js"],
  ["IndexedDB", "chrome://browser/content/IndexedDB.js"],
  ["PreferencesView", "chrome://browser/content/preferences.js"],
  ["Sanitizer", "chrome://browser/content/sanitize.js"],
  ["SelectHelperUI", "chrome://browser/content/SelectHelperUI.js"],
  ["SharingUI", "chrome://browser/content/SharingUI.js"],
#ifdef MOZ_SERVICES_SYNC
  ["WeaveGlue", "chrome://browser/content/sync.js"],
#endif
  ["SSLExceptions", "chrome://browser/content/exceptions.js"]
].forEach(function (aScript) {
  let [name, script] = aScript;
  XPCOMUtils.defineLazyGetter(window, name, function() {
    let sandbox = {};
    Services.scriptloader.loadSubScript(script, sandbox);
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

XPCOMUtils.defineLazyServiceGetter(window, "gHistSvc", "@mozilla.org/browser/nav-history-service;1", "nsINavHistoryService", "nsIBrowserHistory");
XPCOMUtils.defineLazyServiceGetter(window, "gURIFixup", "@mozilla.org/docshell/urifixup;1", "nsIURIFixup");
XPCOMUtils.defineLazyServiceGetter(window, "gFaviconService", "@mozilla.org/browser/favicon-service;1", "nsIFaviconService");
XPCOMUtils.defineLazyServiceGetter(window, "gFocusManager", "@mozilla.org/focus-manager;1", "nsIFocusManager");
