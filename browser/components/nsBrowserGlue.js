# -*- indent-tabs-mode: nil -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AboutHome",
                                  "resource:///modules/AboutHome.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ContentClick",
                                  "resource:///modules/ContentClick.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DirectoryLinksProvider",
                                  "resource://gre/modules/DirectoryLinksProvider.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BookmarkHTMLUtils",
                                  "resource://gre/modules/BookmarkHTMLUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BookmarkJSONUtils",
                                  "resource://gre/modules/BookmarkJSONUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "WebappManager",
                                  "resource:///modules/WebappManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageThumbs",
                                  "resource://gre/modules/PageThumbs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
                                  "resource://gre/modules/NewTabUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserNewTabPreloader",
                                  "resource:///modules/BrowserNewTabPreloader.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizationTabPreloader",
                                  "resource:///modules/CustomizationTabPreloader.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PdfJs",
                                  "resource://pdf.js/PdfJs.jsm");

#ifdef NIGHTLY_BUILD
XPCOMUtils.defineLazyModuleGetter(this, "ShumwayUtils",
                                  "resource://shumway/ShumwayUtils.jsm");
#endif

XPCOMUtils.defineLazyModuleGetter(this, "webrtcUI",
                                  "resource:///modules/webrtcUI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "RecentWindow",
                                  "resource:///modules/RecentWindow.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesBackups",
                                  "resource://gre/modules/PlacesBackups.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "RemotePrompt",
                                  "resource:///modules/RemotePrompt.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ContentPrefServiceParent",
                                  "resource://gre/modules/ContentPrefServiceParent.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SessionStore",
                                  "resource:///modules/sessionstore/SessionStore.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserUITelemetry",
                                  "resource:///modules/BrowserUITelemetry.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AsyncShutdown",
                                  "resource://gre/modules/AsyncShutdown.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginManagerParent",
                                  "resource://gre/modules/LoginManagerParent.jsm");

#ifdef NIGHTLY_BUILD
XPCOMUtils.defineLazyModuleGetter(this, "SignInToWebsiteUX",
                                  "resource:///modules/SignInToWebsite.jsm");
#endif

XPCOMUtils.defineLazyModuleGetter(this, "ContentSearch",
                                  "resource:///modules/ContentSearch.jsm");

XPCOMUtils.defineLazyGetter(this, "ShellService", function() {
  try {
    return Cc["@mozilla.org/browser/shell-service;1"].
           getService(Ci.nsIShellService);
  }
  catch(ex) {
    return null;
  }
});

XPCOMUtils.defineLazyModuleGetter(this, "FormValidationHandler",
                                  "resource:///modules/FormValidationHandler.jsm");

const PREF_PLUGINS_NOTIFYUSER = "plugins.update.notifyUser";
const PREF_PLUGINS_UPDATEURL  = "plugins.update.url";


const BOOKMARKS_BACKUP_IDLE_TIME_SEC = 8 * 60;


const BOOKMARKS_BACKUP_MIN_INTERVAL_DAYS = 1;


const BOOKMARKS_BACKUP_MAX_INTERVAL_DAYS = 3;


const BrowserGlueServiceFactory = {
  _instance: null,
  createInstance: function BGSF_createInstance(outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return this._instance == null ?
      this._instance = new BrowserGlue() : this._instance;
  }
};



function BrowserGlue() {
  XPCOMUtils.defineLazyServiceGetter(this, "_idleService",
                                     "@mozilla.org/widget/idleservice;1",
                                     "nsIIdleService");

  XPCOMUtils.defineLazyGetter(this, "_distributionCustomizer", function() {
                                Cu.import("resource:///modules/distribution.js");
                                return new DistributionCustomizer();
                              });

  XPCOMUtils.defineLazyGetter(this, "_sanitizer",
    function() {
      let sanitizerScope = {};
      Services.scriptloader.loadSubScript("chrome://browser/content/sanitize.js", sanitizerScope);
      return sanitizerScope.Sanitizer;
    });

  this._init();
}

#ifndef XP_MACOSX
# OS X has the concept of zero-window sessions and therefore ignores the
# browser-lastwindow-close-* topics.
#define OBSERVE_LASTWINDOW_CLOSE_TOPICS 1
#endif

BrowserGlue.prototype = {
  _saveSession: false,
  _isPlacesInitObserver: false,
  _isPlacesLockedObserver: false,
  _isPlacesShutdownObserver: false,
  _isPlacesDatabaseLocked: false,
  _migrationImportsDefaultBookmarks: false,

  _setPrefToSaveSession: function BG__setPrefToSaveSession(aForce) {
    if (!this._saveSession && !aForce)
      return;

    Services.prefs.setBoolPref("browser.sessionstore.resume_session_once", true);

    
    
    
    Services.prefs.savePrefFile(null);
  },

#ifdef MOZ_SERVICES_SYNC
  _setSyncAutoconnectDelay: function BG__setSyncAutoconnectDelay() {
    
    if (Services.prefs.prefHasUserValue("services.sync.autoconnectDelay")) {
      let prefDelay = Services.prefs.getIntPref("services.sync.autoconnectDelay");

      if (prefDelay > 0)
        return;
    }

    
    const MAX_DELAY = 300;
    let delay = 3;
    let browserEnum = Services.wm.getEnumerator("navigator:browser");
    while (browserEnum.hasMoreElements()) {
      delay += browserEnum.getNext().gBrowser.tabs.length;
    }
    delay = delay <= MAX_DELAY ? delay : MAX_DELAY;

    Cu.import("resource://services-sync/main.js");
    Weave.Service.scheduler.delayedAutoConnect(delay);
  },
#endif

  
  observe: function BG_observe(subject, topic, data) {
    switch (topic) {
      case "prefservice:after-app-defaults":
        this._onAppDefaults();
        break;
      case "final-ui-startup":
        this._finalUIStartup();
        break;
      case "browser-delayed-startup-finished":
        this._onFirstWindowLoaded(subject);
        Services.obs.removeObserver(this, "browser-delayed-startup-finished");
        break;
      case "sessionstore-windows-restored":
        this._onWindowsRestored();
        break;
      case "browser:purge-session-history":
        
        Services.console.logStringMessage(null); 
        Services.console.reset();
        break;
      case "quit-application-requested":
        this._onQuitRequest(subject, data);
        break;
      case "quit-application-granted":
        this._onQuitApplicationGranted();
        break;
#ifdef OBSERVE_LASTWINDOW_CLOSE_TOPICS
      case "browser-lastwindow-close-requested":
        
        
        this._onQuitRequest(subject, "lastwindow");
        break;
      case "browser-lastwindow-close-granted":
        this._setPrefToSaveSession();
        break;
#endif
#ifdef MOZ_SERVICES_SYNC
      case "weave:service:ready":
        this._setSyncAutoconnectDelay();
        break;
      case "weave:engine:clients:display-uri":
        this._onDisplaySyncURI(subject);
        break;
#endif
      case "session-save":
        this._setPrefToSaveSession(true);
        subject.QueryInterface(Ci.nsISupportsPRBool);
        subject.data = true;
        break;
      case "places-init-complete":
        if (!this._migrationImportsDefaultBookmarks)
          this._initPlaces(false);

        Services.obs.removeObserver(this, "places-init-complete");
        this._isPlacesInitObserver = false;
        
        Services.obs.removeObserver(this, "places-database-locked");
        this._isPlacesLockedObserver = false;
        break;
      case "places-database-locked":
        this._isPlacesDatabaseLocked = true;
        
        
        Services.obs.removeObserver(this, "places-database-locked");
        this._isPlacesLockedObserver = false;
        break;
      case "places-shutdown":
        if (this._isPlacesShutdownObserver) {
          Services.obs.removeObserver(this, "places-shutdown");
          this._isPlacesShutdownObserver = false;
        }
        
        this._onPlacesShutdown();
        break;
      case "idle":
        this._backupBookmarks();
        break;
      case "distribution-customization-complete":
        Services.obs.removeObserver(this, "distribution-customization-complete");
        
        delete this._distributionCustomizer;
        break;
      case "browser-glue-test": 
        if (data == "post-update-notification") {
          if (Services.prefs.prefHasUserValue("app.update.postupdate"))
            this._showUpdateNotification();
        }
        else if (data == "force-ui-migration") {
          this._migrateUI();
        }
        else if (data == "force-distribution-customization") {
          this._distributionCustomizer.applyPrefDefaults();
          this._distributionCustomizer.applyCustomizations();
          
        }
        else if (data == "force-places-init") {
          this._initPlaces(false);
        }
        break;
      case "initial-migration-will-import-default-bookmarks":
        this._migrationImportsDefaultBookmarks = true;
        break;
      case "initial-migration-did-import-default-bookmarks":
        this._initPlaces(true);
        break;
      case "handle-xul-text-link":
        let linkHandled = subject.QueryInterface(Ci.nsISupportsPRBool);
        if (!linkHandled.data) {
          let win = this.getMostRecentBrowserWindow();
          if (win) {
            win.openUILinkIn(data, "tab");
            linkHandled.data = true;
          }
        }
        break;
      case "profile-before-change":
         
         
         
        this._dispose();
        break;
      case "keyword-search":
        
        
        
        
        let win = this.getMostRecentBrowserWindow();
        BrowserUITelemetry.countSearchEvent("urlbar", win.gURLBar.value);
#ifdef MOZ_SERVICES_HEALTHREPORT
        let reporter = Cc["@mozilla.org/datareporting/service;1"]
                         .getService()
                         .wrappedJSObject
                         .healthReporter;

        if (!reporter) {
          return;
        }

        reporter.onInit().then(function record() {
          try {
            let engine = subject.QueryInterface(Ci.nsISearchEngine);
            reporter.getProvider("org.mozilla.searches").recordSearch(engine, "urlbar");
          } catch (ex) {
            Cu.reportError(ex);
          }
        });
#endif
        break;
      case "browser-search-engine-modified":
        if (data != "engine-default" && data != "engine-current") {
          break;
        }
        
        
        
        
        
        
        
        let ss = Services.search;
        if (ss.currentEngine.name == ss.defaultEngine.name)
          return;
        if (data == "engine-current")
          ss.defaultEngine = ss.currentEngine;
        else
          ss.currentEngine = ss.defaultEngine;
        break;
      case "browser-search-service":
        if (data != "init-complete")
          return;
        Services.obs.removeObserver(this, "browser-search-service");
        this._syncSearchEngines();
        break;
    }
  },

  _syncSearchEngines: function () {
    
    
    
    
    if (Services.search.isInitialized) {
      Services.search.defaultEngine = Services.search.currentEngine;
    }
  },

  
  _init: function BG__init() {
    let os = Services.obs;
    os.addObserver(this, "prefservice:after-app-defaults", false);
    os.addObserver(this, "final-ui-startup", false);
    os.addObserver(this, "browser-delayed-startup-finished", false);
    os.addObserver(this, "sessionstore-windows-restored", false);
    os.addObserver(this, "browser:purge-session-history", false);
    os.addObserver(this, "quit-application-requested", false);
    os.addObserver(this, "quit-application-granted", false);
#ifdef OBSERVE_LASTWINDOW_CLOSE_TOPICS
    os.addObserver(this, "browser-lastwindow-close-requested", false);
    os.addObserver(this, "browser-lastwindow-close-granted", false);
#endif
#ifdef MOZ_SERVICES_SYNC
    os.addObserver(this, "weave:service:ready", false);
    os.addObserver(this, "weave:engine:clients:display-uri", false);
#endif
    os.addObserver(this, "session-save", false);
    os.addObserver(this, "places-init-complete", false);
    this._isPlacesInitObserver = true;
    os.addObserver(this, "places-database-locked", false);
    this._isPlacesLockedObserver = true;
    os.addObserver(this, "distribution-customization-complete", false);
    os.addObserver(this, "places-shutdown", false);
    this._isPlacesShutdownObserver = true;
    os.addObserver(this, "handle-xul-text-link", false);
    os.addObserver(this, "profile-before-change", false);
#ifdef MOZ_SERVICES_HEALTHREPORT
    os.addObserver(this, "keyword-search", false);
#endif
    os.addObserver(this, "browser-search-engine-modified", false);
    os.addObserver(this, "browser-search-service", false);
  },

  
  _dispose: function BG__dispose() {
    let os = Services.obs;
    os.removeObserver(this, "prefservice:after-app-defaults");
    os.removeObserver(this, "final-ui-startup");
    os.removeObserver(this, "sessionstore-windows-restored");
    os.removeObserver(this, "browser:purge-session-history");
    os.removeObserver(this, "quit-application-requested");
    os.removeObserver(this, "quit-application-granted");
#ifdef OBSERVE_LASTWINDOW_CLOSE_TOPICS
    os.removeObserver(this, "browser-lastwindow-close-requested");
    os.removeObserver(this, "browser-lastwindow-close-granted");
#endif
#ifdef MOZ_SERVICES_SYNC
    os.removeObserver(this, "weave:service:ready");
    os.removeObserver(this, "weave:engine:clients:display-uri");
#endif
    os.removeObserver(this, "session-save");
    if (this._bookmarksBackupIdleTime) {
      this._idleService.removeIdleObserver(this, this._bookmarksBackupIdleTime);
      delete this._bookmarksBackupIdleTime;
    }
    if (this._isPlacesInitObserver)
      os.removeObserver(this, "places-init-complete");
    if (this._isPlacesLockedObserver)
      os.removeObserver(this, "places-database-locked");
    if (this._isPlacesShutdownObserver)
      os.removeObserver(this, "places-shutdown");
    os.removeObserver(this, "handle-xul-text-link");
    os.removeObserver(this, "profile-before-change");
#ifdef MOZ_SERVICES_HEALTHREPORT
    os.removeObserver(this, "keyword-search");
#endif
    os.removeObserver(this, "browser-search-engine-modified");
    try {
      os.removeObserver(this, "browser-search-service");
      
    } catch (ex) {}
  },

  _onAppDefaults: function BG__onAppDefaults() {
    
    
    this._distributionCustomizer.applyPrefDefaults();
  },

  
  
  _finalUIStartup: function BG__finalUIStartup() {
    this._sanitizer.onStartup();
    
    if (Services.appinfo.inSafeMode) {
      Services.ww.openWindow(null, "chrome://browser/content/safeMode.xul", 
                             "_blank", "chrome,centerscreen,modal,resizable=no", null);
    }

    
    
    this._distributionCustomizer.applyCustomizations();

    
    this._migrateUI();

    this._syncSearchEngines();

    WebappManager.init();
    PageThumbs.init();
    NewTabUtils.init();
    DirectoryLinksProvider.init();
    NewTabUtils.links.addProvider(DirectoryLinksProvider);
    BrowserNewTabPreloader.init();
#ifdef NIGHTLY_BUILD
    if (Services.prefs.getBoolPref("dom.identity.enabled")) {
      SignInToWebsiteUX.init();
    }
#endif
    PdfJs.init();
#ifdef NIGHTLY_BUILD
    ShumwayUtils.init();
#endif
    webrtcUI.init();
    AboutHome.init();
    SessionStore.init();
    BrowserUITelemetry.init();
    ContentSearch.init();
    FormValidationHandler.init();

    ContentClick.init();
    RemotePrompt.init();
    ContentPrefServiceParent.init();

    LoginManagerParent.init();

    Services.obs.notifyObservers(null, "browser-ui-startup-complete", "");
  },

  _checkForOldBuildUpdates: function () {
    
    if (Services.prefs.getBoolPref("app.update.enabled") &&
        Services.prefs.getBoolPref("app.update.checkInstallTime")) {

      let buildID = Services.appinfo.appBuildID;
      let today = new Date().getTime();
      let buildDate = new Date(buildID.slice(0,4),     
                               buildID.slice(4,6) - 1, 
                               buildID.slice(6,8),     
                               buildID.slice(8,10),    
                               buildID.slice(10,12),   
                               buildID.slice(12,14))   
      .getTime();

      const millisecondsIn24Hours = 86400000;
      let acceptableAge = Services.prefs.getIntPref("app.update.checkInstallTime.days") * millisecondsIn24Hours;

      if (buildDate + acceptableAge < today) {
        Cc["@mozilla.org/updates/update-service;1"].getService(Ci.nsIApplicationUpdateService).checkForBackgroundUpdates();
      }
    }
  },

  _trackSlowStartup: function () {
    if (Services.startup.interrupted ||
        Services.prefs.getBoolPref("browser.slowStartup.notificationDisabled"))
      return;

    let currentTime = Date.now() - Services.startup.getStartupInfo().process;
    let averageTime = 0;
    let samples = 0;
    try {
      averageTime = Services.prefs.getIntPref("browser.slowStartup.averageTime");
      samples = Services.prefs.getIntPref("browser.slowStartup.samples");
    } catch (e) { }

    let totalTime = (averageTime * samples) + currentTime;
    samples++;
    averageTime = totalTime / samples;

    if (samples >= Services.prefs.getIntPref("browser.slowStartup.maxSamples")) {
      if (averageTime > Services.prefs.getIntPref("browser.slowStartup.timeThreshold"))
        this._showSlowStartupNotification();
      averageTime = 0;
      samples = 0;
    }

    Services.prefs.setIntPref("browser.slowStartup.averageTime", averageTime);
    Services.prefs.setIntPref("browser.slowStartup.samples", samples);
  },

  _showSlowStartupNotification: function () {
    let win = this.getMostRecentBrowserWindow();
    if (!win)
      return;

    let productName = Services.strings
                              .createBundle("chrome://branding/locale/brand.properties")
                              .GetStringFromName("brandFullName");
    let message = win.gNavigatorBundle.getFormattedString("slowStartup.message", [productName]);

    let buttons = [
      {
        label:     win.gNavigatorBundle.getString("slowStartup.helpButton.label"),
        accessKey: win.gNavigatorBundle.getString("slowStartup.helpButton.accesskey"),
        callback: function () {
          win.openUILinkIn("https://support.mozilla.org/kb/reset-firefox-easily-fix-most-problems", "tab");
        }
      },
      {
        label:     win.gNavigatorBundle.getString("slowStartup.disableNotificationButton.label"),
        accessKey: win.gNavigatorBundle.getString("slowStartup.disableNotificationButton.accesskey"),
        callback: function () {
          Services.prefs.setBoolPref("browser.slowStartup.notificationDisabled", true);
        }
      }
    ];

    let nb = win.document.getElementById("global-notificationbox");
    nb.appendNotification(message, "slow-startup",
                          "chrome://browser/skin/slowStartup-16.png",
                          nb.PRIORITY_INFO_LOW, buttons);
  },

  


  _resetUnusedProfileNotification: function () {
    let win = this.getMostRecentBrowserWindow();
    if (!win)
      return;

    Cu.import("resource://gre/modules/ResetProfile.jsm");
    if (!ResetProfile.resetSupported())
      return;

    let productName = Services.strings
                              .createBundle("chrome://branding/locale/brand.properties")
                              .GetStringFromName("brandShortName");
    let resetBundle = Services.strings
                              .createBundle("chrome://global/locale/resetProfile.properties");

    let message = resetBundle.formatStringFromName("resetUnusedProfile.message", [productName], 1);
    let buttons = [
      {
        label:     resetBundle.formatStringFromName("resetProfile.resetButton.label", [productName], 1),
        accessKey: resetBundle.GetStringFromName("resetProfile.resetButton.accesskey"),
        callback: function () {
          ResetProfile.openConfirmationDialog(win);
        }
      },
    ];

    let nb = win.document.getElementById("global-notificationbox");
    nb.appendNotification(message, "reset-unused-profile",
                          "chrome://global/skin/icons/question-16.png",
                          nb.PRIORITY_INFO_LOW, buttons);
  },

  _firstWindowTelemetry: function(aWindow) {
#ifdef XP_WIN
    let SCALING_PROBE_NAME = "DISPLAY_SCALING_MSWIN";
#elifdef XP_MACOSX
    let SCALING_PROBE_NAME = "DISPLAY_SCALING_OSX";
#elifdef XP_LINUX
    let SCALING_PROBE_NAME = "DISPLAY_SCALING_LINUX";
#else
    let SCALING_PROBE_NAME = "";
#endif
    if (SCALING_PROBE_NAME) {
      let scaling = aWindow.devicePixelRatio * 100;
      Services.telemetry.getHistogramById(SCALING_PROBE_NAME).add(scaling);
    }
  },

  
  _onFirstWindowLoaded: function BG__onFirstWindowLoaded(aWindow) {
#ifdef XP_WIN
    
    const WINTASKBAR_CONTRACTID = "@mozilla.org/windows-taskbar;1";
    if (WINTASKBAR_CONTRACTID in Cc &&
        Cc[WINTASKBAR_CONTRACTID].getService(Ci.nsIWinTaskbar).available) {
      let temp = {};
      Cu.import("resource:///modules/WindowsJumpLists.jsm", temp);
      temp.WinTaskbarJumpList.startup();
    }
#endif

    this._trackSlowStartup();

    
    const OFFER_PROFILE_RESET_INTERVAL_MS = 60 * 24 * 60 * 60 * 1000;
    let lastUse = Services.appinfo.replacedLockTime;
    let disableResetPrompt = false;
    try {
      disableResetPrompt = Services.prefs.getBoolPref("browser.disableResetPrompt");
    } catch(e) {}
    if (!disableResetPrompt && lastUse &&
        Date.now() - lastUse >= OFFER_PROFILE_RESET_INTERVAL_MS) {
      this._resetUnusedProfileNotification();
    }

    this._checkForOldBuildUpdates();

    this._firstWindowTelemetry(aWindow);
  },

  


  _onQuitApplicationGranted: function () {
    
    
    this._setPrefToSaveSession();

    
    
    try {
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                         .getService(Ci.nsIAppStartup);
      appStartup.trackStartupCrashEnd();
    } catch (e) {
      Cu.reportError("Could not end startup crash tracking in quit-application-granted: " + e);
    }

    BrowserNewTabPreloader.uninit();
    CustomizationTabPreloader.uninit();
    WebappManager.uninit();
#ifdef NIGHTLY_BUILD
    if (Services.prefs.getBoolPref("dom.identity.enabled")) {
      SignInToWebsiteUX.uninit();
    }
#endif
    webrtcUI.uninit();
    FormValidationHandler.uninit();
  },

  
  _onWindowsRestored: function BG__onWindowsRestored() {
    
    if (Services.prefs.prefHasUserValue("app.update.postupdate"))
      this._showUpdateNotification();

    
    
    if (this._isPlacesDatabaseLocked) {
      this._showPlacesLockedNotificationBox();
    }

    
    
    if (Services.prefs.getBoolPref(PREF_PLUGINS_NOTIFYUSER))
      this._showPluginUpdatePage();

    
    
    let changedIDs = AddonManager.getStartupChanges(AddonManager.STARTUP_CHANGE_INSTALLED);
    if (changedIDs.length > 0) {
      let win = this.getMostRecentBrowserWindow();
      AddonManager.getAddonsByIDs(changedIDs, function(aAddons) {
        aAddons.forEach(function(aAddon) {
          
          if (!aAddon.userDisabled || !(aAddon.permissions & AddonManager.PERM_CAN_ENABLE))
            return;

          win.openUILinkIn("about:newaddon?id=" + aAddon.id, "tab");
        })
      });
    }

    
    if (ShellService) {
#ifdef DEBUG
      let shouldCheck = false;
#else
      let shouldCheck = ShellService.shouldCheckDefaultBrowser;
#endif
      let willRecoverSession = false;
      try {
        let ss = Cc["@mozilla.org/browser/sessionstartup;1"].
                 getService(Ci.nsISessionStartup);
        willRecoverSession =
          (ss.sessionType == Ci.nsISessionStartup.RECOVER_SESSION);
      }
      catch (ex) {  }

      
      let isDefault = ShellService.isDefaultBrowser(true, false);
      try {
        
        
        Services.telemetry.getHistogramById("BROWSER_IS_USER_DEFAULT")
                          .add(isDefault);
      }
      catch (ex) {  }

      if (shouldCheck && !isDefault && !willRecoverSession) {
        Services.tm.mainThread.dispatch(function() {
          DefaultBrowserCheck.prompt(this.getMostRecentBrowserWindow());
        }.bind(this), Ci.nsIThread.DISPATCH_NORMAL);
      }
    }

#ifdef E10S_TESTING_ONLY
    E10SUINotification.checkStatus();
#endif
  },

  _onQuitRequest: function BG__onQuitRequest(aCancelQuit, aQuitType) {
    
    if ((aCancelQuit instanceof Ci.nsISupportsPRBool) && aCancelQuit.data)
      return;

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    if (aQuitType == "restart")
      return;

    var windowcount = 0;
    var pagecount = 0;
    var browserEnum = Services.wm.getEnumerator("navigator:browser");
    let allWindowsPrivate = true;
    while (browserEnum.hasMoreElements()) {
      
      windowcount++;

      var browser = browserEnum.getNext();
      if (!PrivateBrowsingUtils.isWindowPrivate(browser))
        allWindowsPrivate = false;
      var tabbrowser = browser.document.getElementById("content");
      if (tabbrowser)
        pagecount += tabbrowser.browsers.length - tabbrowser._numPinnedTabs;
    }

    this._saveSession = false;
    if (pagecount < 2)
      return;

    if (!aQuitType)
      aQuitType = "quit";

    
    
    

    var sessionWillBeRestored = Services.prefs.getIntPref("browser.startup.page") == 3 ||
                                Services.prefs.getBoolPref("browser.sessionstore.resume_session_once");
    if (sessionWillBeRestored || !Services.prefs.getBoolPref("browser.warnOnQuit"))
      return;

    let win = Services.wm.getMostRecentWindow("navigator:browser");

    
    
    if (!Services.prefs.getBoolPref("browser.showQuitWarning")) {
      if (aQuitType == "lastwindow") {
        
        
        
        
        aCancelQuit.data =
          !win.gBrowser.warnAboutClosingTabs(win.gBrowser.closingTabsEnum.ALL);
      }
      return;
    }

    let prompt = Services.prompt;
    let quitBundle = Services.strings.createBundle("chrome://browser/locale/quitDialog.properties");
    let brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");

    let appName = brandBundle.GetStringFromName("brandShortName");
    let quitDialogTitle = quitBundle.formatStringFromName("quitDialogTitle",
                                                          [appName], 1);
    let neverAskText = quitBundle.GetStringFromName("neverAsk2");
    let neverAsk = {value: false};

    let choice;
    if (allWindowsPrivate) {
      let text = quitBundle.formatStringFromName("messagePrivate", [appName], 1);
      let flags = prompt.BUTTON_TITLE_IS_STRING * prompt.BUTTON_POS_0 +
                  prompt.BUTTON_TITLE_IS_STRING * prompt.BUTTON_POS_1 +
                  prompt.BUTTON_POS_0_DEFAULT;
      choice = prompt.confirmEx(win, quitDialogTitle, text, flags,
                                quitBundle.GetStringFromName("quitTitle"),
                                quitBundle.GetStringFromName("cancelTitle"),
                                null,
                                neverAskText, neverAsk);

      
      
      if (choice == 0) {
        choice = 2;
      }
    } else {
      let text = quitBundle.formatStringFromName(
        windowcount == 1 ? "messageNoWindows" : "message", [appName], 1);
      let flags = prompt.BUTTON_TITLE_IS_STRING * prompt.BUTTON_POS_0 +
                  prompt.BUTTON_TITLE_IS_STRING * prompt.BUTTON_POS_1 +
                  prompt.BUTTON_TITLE_IS_STRING * prompt.BUTTON_POS_2 +
                  prompt.BUTTON_POS_0_DEFAULT;
      choice = prompt.confirmEx(win, quitDialogTitle, text, flags,
                                quitBundle.GetStringFromName("saveTitle"),
                                quitBundle.GetStringFromName("cancelTitle"),
                                quitBundle.GetStringFromName("quitTitle"),
                                neverAskText, neverAsk);
    }

    switch (choice) {
    case 2: 
      if (neverAsk.value)
        Services.prefs.setBoolPref("browser.showQuitWarning", false);
      break;
    case 1: 
      aCancelQuit.QueryInterface(Ci.nsISupportsPRBool);
      aCancelQuit.data = true;
      break;
    case 0: 
      this._saveSession = true;
      if (neverAsk.value) {
        
        Services.prefs.setIntPref("browser.startup.page", 3);
      }
      break;
    }
  },

  _showUpdateNotification: function BG__showUpdateNotification() {
    Services.prefs.clearUserPref("app.update.postupdate");

    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    try {
      
      var update = um.getUpdateAt(0).QueryInterface(Ci.nsIPropertyBag);
    }
    catch (e) {
      
      Cu.reportError("Unable to find update: " + e);
      return;
    }

    var actions = update.getProperty("actions");
    if (!actions || actions.indexOf("silent") != -1)
      return;

    var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
                    getService(Ci.nsIURLFormatter);
    var browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
    var brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");
    var appName = brandBundle.GetStringFromName("brandShortName");

    function getNotifyString(aPropData) {
      var propValue = update.getProperty(aPropData.propName);
      if (!propValue) {
        if (aPropData.prefName)
          propValue = formatter.formatURLPref(aPropData.prefName);
        else if (aPropData.stringParams)
          propValue = browserBundle.formatStringFromName(aPropData.stringName,
                                                         aPropData.stringParams,
                                                         aPropData.stringParams.length);
        else
          propValue = browserBundle.GetStringFromName(aPropData.stringName);
      }
      return propValue;
    }

    if (actions.indexOf("showNotification") != -1) {
      let text = getNotifyString({propName: "notificationText",
                                  stringName: "puNotifyText",
                                  stringParams: [appName]});
      let url = getNotifyString({propName: "notificationURL",
                                 prefName: "startup.homepage_override_url"});
      let label = getNotifyString({propName: "notificationButtonLabel",
                                   stringName: "pu.notifyButton.label"});
      let key = getNotifyString({propName: "notificationButtonAccessKey",
                                 stringName: "pu.notifyButton.accesskey"});

      let win = this.getMostRecentBrowserWindow();
      let notifyBox = win.gBrowser.getNotificationBox();

      let buttons = [
                      {
                        label:     label,
                        accessKey: key,
                        popup:     null,
                        callback: function(aNotificationBar, aButton) {
                          win.openUILinkIn(url, "tab");
                        }
                      }
                    ];

      let notification = notifyBox.appendNotification(text, "post-update-notification",
                                                      null, notifyBox.PRIORITY_INFO_LOW,
                                                      buttons);
      notification.persistence = -1; 
    }

    if (actions.indexOf("showAlert") == -1)
      return;

    let notifier;
    try {
      notifier = Cc["@mozilla.org/alerts-service;1"].
                 getService(Ci.nsIAlertsService);
    }
    catch (e) {
      
      return;
    }

    let title = getNotifyString({propName: "alertTitle",
                                 stringName: "puAlertTitle",
                                 stringParams: [appName]});
    let text = getNotifyString({propName: "alertText",
                                stringName: "puAlertText",
                                stringParams: [appName]});
    let url = getNotifyString({propName: "alertURL",
                               prefName: "startup.homepage_override_url"});

    var self = this;
    function clickCallback(subject, topic, data) {
      
      if (topic != "alertclickcallback")
        return;
      let win = self.getMostRecentBrowserWindow();
      win.openUILinkIn(data, "tab");
    }

    try {
      
      
      notifier.showAlertNotification(null, title, text,
                                     true, url, clickCallback);
    }
    catch (e) {
    }
  },

  _showPluginUpdatePage: function BG__showPluginUpdatePage() {
    Services.prefs.setBoolPref(PREF_PLUGINS_NOTIFYUSER, false);

    var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
                    getService(Ci.nsIURLFormatter);
    var updateUrl = formatter.formatURLPref(PREF_PLUGINS_UPDATEURL);

    var win = this.getMostRecentBrowserWindow();
    win.openUILinkIn(updateUrl, "tab");
  },

  



















  _initPlaces: function BG__initPlaces(aInitialMigrationPerformed) {
    
    
    
    
    
    let dbStatus = PlacesUtils.history.databaseStatus;
    let importBookmarks = !aInitialMigrationPerformed &&
                          (dbStatus == PlacesUtils.history.DATABASE_STATUS_CREATE ||
                           dbStatus == PlacesUtils.history.DATABASE_STATUS_CORRUPT);

    
    let importBookmarksHTML = false;
    try {
      importBookmarksHTML =
        Services.prefs.getBoolPref("browser.places.importBookmarksHTML");
      if (importBookmarksHTML)
        importBookmarks = true;
    } catch(ex) {}

    
    let autoExportHTML = false;
    try {
      autoExportHTML = Services.prefs.getBoolPref("browser.bookmarks.autoExportHTML");
    } catch (ex) {} 
    if (autoExportHTML) {
      
      
      AsyncShutdown.profileChangeTeardown.addBlocker(
        "Places: export bookmarks.html",
        () => BookmarkHTMLUtils.exportToFile(BookmarkHTMLUtils.defaultPath));
    }

    Task.spawn(function() {
      
      
      let restoreDefaultBookmarks = false;
      try {
        restoreDefaultBookmarks =
          Services.prefs.getBoolPref("browser.bookmarks.restore_default_bookmarks");
        if (restoreDefaultBookmarks) {
          
          yield this._backupBookmarks();
          importBookmarks = true;
        }
      } catch(ex) {}

      
      
      let lastBackupFile;

      
      
      if (importBookmarks && !restoreDefaultBookmarks && !importBookmarksHTML) {
        
        lastBackupFile = yield PlacesBackups.getMostRecentBackup();
        if (lastBackupFile) {
          
          yield BookmarkJSONUtils.importFromFile(lastBackupFile, true);
          importBookmarks = false;
        }
        else {
          
          importBookmarks = true;
          if (yield OS.File.exists(BookmarkHTMLUtils.defaultPath)) {
            
            importBookmarksHTML = true;
          }
          else {
            
            restoreDefaultBookmarks = true;
          }
        }
      }

      
      
      
      
      
      if (!importBookmarks) {
        
        
        this._distributionCustomizer.applyBookmarks();
        this.ensurePlacesDefaultQueriesInitialized();
      }
      else {
        
        
        
        let smartBookmarksVersion = 0;
        try {
          smartBookmarksVersion = Services.prefs.getIntPref("browser.places.smartBookmarksVersion");
        } catch(ex) {}
        if (!autoExportHTML && smartBookmarksVersion != -1)
          Services.prefs.setIntPref("browser.places.smartBookmarksVersion", 0);

        let bookmarksUrl = null;
        if (restoreDefaultBookmarks) {
          
          bookmarksUrl = "resource:///defaults/profile/bookmarks.html";
        }
        else if (yield OS.File.exists(BookmarkHTMLUtils.defaultPath)) {
          bookmarksUrl = OS.Path.toFileURI(BookmarkHTMLUtils.defaultPath);
        }

        if (bookmarksUrl) {
          
          try {
            BookmarkHTMLUtils.importFromURL(bookmarksUrl, true).then(null,
              function onFailure() {
                Cu.reportError("Bookmarks.html file could be corrupt.");
              }
            ).then(
              function onComplete() {
                
                
                this._distributionCustomizer.applyBookmarks();
                
                
                this.ensurePlacesDefaultQueriesInitialized();
              }.bind(this)
            );
          } catch (err) {
            Cu.reportError("Bookmarks.html file could be corrupt. " + err);
          }
        }
        else {
          Cu.reportError("Unable to find bookmarks.html file.");
        }

        
        if (importBookmarksHTML)
          Services.prefs.setBoolPref("browser.places.importBookmarksHTML", false);
        if (restoreDefaultBookmarks)
          Services.prefs.setBoolPref("browser.bookmarks.restore_default_bookmarks",
                                     false);
      }

      
      if (!this._bookmarksBackupIdleTime) {
        this._bookmarksBackupIdleTime = BOOKMARKS_BACKUP_IDLE_TIME_SEC;

        
        
        if (lastBackupFile === undefined)
          lastBackupFile = yield PlacesBackups.getMostRecentBackup();
        if (!lastBackupFile) {
            this._bookmarksBackupIdleTime /= 2;
        }
        else {
          let lastBackupTime = PlacesBackups.getDateForFile(lastBackupFile);
          let profileLastUse = Services.appinfo.replacedLockTime || Date.now();

          
          
          
          if (profileLastUse > lastBackupTime) {
            let backupAge = Math.round((profileLastUse - lastBackupTime) / 86400000);
            
            try {
              Services.telemetry
                      .getHistogramById("PLACES_BACKUPS_DAYSFROMLAST")
                      .add(backupAge);
            } catch (ex) {
              Components.utils.reportError("Unable to report telemetry.");
            }

            if (backupAge > BOOKMARKS_BACKUP_MAX_INTERVAL_DAYS)
              this._bookmarksBackupIdleTime /= 2;
          }
        }
        this._idleService.addIdleObserver(this, this._bookmarksBackupIdleTime);
      }

      Services.obs.notifyObservers(null, "places-browser-init-complete", "");
    }.bind(this));
  },

  




  _onPlacesShutdown: function BG__onPlacesShutdown() {
    this._sanitizer.onShutdown();
    PageThumbs.uninit();

    if (this._bookmarksBackupIdleTime) {
      this._idleService.removeIdleObserver(this, this._bookmarksBackupIdleTime);
      delete this._bookmarksBackupIdleTime;
    }
  },

  


  _backupBookmarks: function BG__backupBookmarks() {
    return Task.spawn(function() {
      let lastBackupFile = yield PlacesBackups.getMostRecentBackup();
      
      
      if (!lastBackupFile ||
          new Date() - PlacesBackups.getDateForFile(lastBackupFile) > BOOKMARKS_BACKUP_MIN_INTERVAL_DAYS * 86400000) {
        let maxBackups = Services.prefs.getIntPref("browser.bookmarks.max_backups");
        yield PlacesBackups.create(maxBackups);
      }
    });
  },

  


  _showPlacesLockedNotificationBox: function BG__showPlacesLockedNotificationBox() {
    var brandBundle  = Services.strings.createBundle("chrome://branding/locale/brand.properties");
    var applicationName = brandBundle.GetStringFromName("brandShortName");
    var placesBundle = Services.strings.createBundle("chrome://browser/locale/places/places.properties");
    var title = placesBundle.GetStringFromName("lockPrompt.title");
    var text = placesBundle.formatStringFromName("lockPrompt.text", [applicationName], 1);
    var buttonText = placesBundle.GetStringFromName("lockPromptInfoButton.label");
    var accessKey = placesBundle.GetStringFromName("lockPromptInfoButton.accessKey");

    var helpTopic = "places-locked";
    var url = Cc["@mozilla.org/toolkit/URLFormatterService;1"].
              getService(Components.interfaces.nsIURLFormatter).
              formatURLPref("app.support.baseURL");
    url += helpTopic;

    var win = this.getMostRecentBrowserWindow();

    var buttons = [
                    {
                      label:     buttonText,
                      accessKey: accessKey,
                      popup:     null,
                      callback:  function(aNotificationBar, aButton) {
                        win.openUILinkIn(url, "tab");
                      }
                    }
                  ];

    var notifyBox = win.gBrowser.getNotificationBox();
    var notification = notifyBox.appendNotification(text, title, null,
                                                    notifyBox.PRIORITY_CRITICAL_MEDIUM,
                                                    buttons);
    notification.persistence = -1; 
  },

  _migrateUI: function BG__migrateUI() {
    const UI_VERSION = 23;
    const BROWSER_DOCURL = "chrome://browser/content/browser.xul";
    let currentUIVersion = 0;
    try {
      currentUIVersion = Services.prefs.getIntPref("browser.migration.version");
    } catch(ex) {}
    if (currentUIVersion >= UI_VERSION)
      return;

    let xulStore = Cc["@mozilla.org/xul/xulstore;1"].getService(Ci.nsIXULStore);

    if (currentUIVersion < 2) {
      
      let currentset = xulStore.getValue(BROWSER_DOCURL, "nav-bar", "currentset");
      
      if (currentset &&
          currentset.indexOf("bookmarks-menu-button-container") == -1) {
        currentset += ",bookmarks-menu-button-container";
        xulStore.setValue(BROWSER_DOCURL, "nav-bar", "currentset", currentset);
      }
    }

    if (currentUIVersion < 3) {
      
      let currentset = xulStore.getValue(BROWSER_DOCURL, "nav-bar", "currentset");
      
      if (currentset &&
          currentset.indexOf("reload-button") != -1 &&
          currentset.indexOf("stop-button") != -1 &&
          currentset.indexOf("urlbar-container") != -1 &&
          currentset.indexOf("urlbar-container,reload-button,stop-button") == -1) {
        currentset = currentset.replace(/(^|,)reload-button($|,)/, "$1$2")
                               .replace(/(^|,)stop-button($|,)/, "$1$2")
                               .replace(/(^|,)urlbar-container($|,)/,
                                        "$1urlbar-container,reload-button,stop-button$2");
        xulStore.setValue(BROWSER_DOCURL, "nav-bar", "currentset", currentset);
      }
    }

    if (currentUIVersion < 4) {
      
      let currentset = xulStore.getValue(BROWSER_DOCURL, "nav-bar", "currentset");
      
      if (currentset &&
          currentset.indexOf("home-button") != -1 &&
          currentset.indexOf("bookmarks-menu-button-container") != -1) {
        currentset = currentset.replace(/(^|,)home-button($|,)/, "$1$2")
                               .replace(/(^|,)bookmarks-menu-button-container($|,)/,
                                        "$1home-button,bookmarks-menu-button-container$2");
        xulStore.setValue(BROWSER_DOCURL, "nav-bar", "currentset", currentset);
      }
    }

    if (currentUIVersion < 5) {
      
      
      
      
      
      if (!xulStore.hasValue(BROWSER_DOCURL, "PersonalToolbar", "collapsed")) {
        
        
        let toolbarIsCustomized = xulStore.hasValue(BROWSER_DOCURL,
                                                    "PersonalToolbar", "currentset");
        let getToolbarFolderCount = function () {
          let toolbarFolder =
            PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId).root;
          let toolbarChildCount = toolbarFolder.childCount;
          toolbarFolder.containerOpen = false;
          return toolbarChildCount;
        };

        if (toolbarIsCustomized || getToolbarFolderCount() > 3) {
          xulStore.setValue(BROWSER_DOCURL, "PersonalToolbar", "collapsed", "false");
        }
      }
    }

    if (currentUIVersion < 8) {
      
      let uri = Services.prefs.getComplexValue("browser.startup.homepage",
                                               Ci.nsIPrefLocalizedString).data;
      if (uri && /^https?:\/\/(www\.)?google(\.\w{2,3}){1,2}\/firefox\/?$/.test(uri)) {
        Services.prefs.clearUserPref("browser.startup.homepage");
      }
    }

    if (currentUIVersion < 9) {
      
      let currentset = xulStore.getValue(BROWSER_DOCURL, "nav-bar", "currentset");

      
      
      
      
      if (currentset &&
          currentset.indexOf("downloads-button") == -1) {
        
        
        
        if (currentset.indexOf("search-container") != -1) {
          currentset = currentset.replace(/(^|,)search-container($|,)/,
                                          "$1search-container,downloads-button$2")
        } else if (currentset.indexOf("home-button") != -1) {
          currentset = currentset.replace(/(^|,)home-button($|,)/,
                                          "$1downloads-button,home-button$2")
        } else {
          currentset = currentset.replace(/(^|,)window-controls($|,)/,
                                          "$1downloads-button,window-controls$2")
        }
        xulStore.setValue(BROWSER_DOCURL, "nav-bar", "currentset", currentset);
      }
    }

#ifdef XP_WIN
    if (currentUIVersion < 10) {
      
      
      
      
      let sm = Cc["@mozilla.org/gfx/screenmanager;1"].getService(Ci.nsIScreenManager);
      if (sm.systemDefaultScale > 1.0) {
        let cps2 = Cc["@mozilla.org/content-pref/service;1"].
                   getService(Ci.nsIContentPrefService2);
        cps2.removeByName("browser.content.full-zoom", null);
      }
    }
#endif

    if (currentUIVersion < 11) {
      Services.prefs.clearUserPref("dom.disable_window_move_resize");
      Services.prefs.clearUserPref("dom.disable_window_flip");
      Services.prefs.clearUserPref("dom.event.contextmenu.enabled");
      Services.prefs.clearUserPref("javascript.enabled");
      Services.prefs.clearUserPref("permissions.default.image");
    }

    if (currentUIVersion < 12) {
      
      
      let currentset = xulStore.getValue(BROWSER_DOCURL, "nav-bar", "currentset");
      
      if (currentset) {
        if (currentset.contains("bookmarks-menu-button-container")) {
          currentset = currentset.replace(/(^|,)bookmarks-menu-button-container($|,)/,
                                          "$1bookmarks-menu-button$2");
          xulStore.setValue(BROWSER_DOCURL, "nav-bar", "currentset", currentset);
        }
      }
    }

    if (currentUIVersion < 13) {
      try {
        if (Services.prefs.getBoolPref("plugins.hide_infobar_for_missing_plugin"))
          Services.prefs.setBoolPref("plugins.notifyMissingFlash", false);
      }
      catch (ex) {}
    }

    if (currentUIVersion < 14) {
      
      let path = OS.Path.join(OS.Constants.Path.profileDir,
                              "chromeappsstore.sqlite");
      OS.File.remove(path);
    }

    if (currentUIVersion < 16) {
      let isCollapsed = xulStore.getValue(BROWSER_DOCURL, "nav-bar", "collapsed");
      if (isCollapsed == "true") {
        xulStore.setValue(BROWSER_DOCURL, "nav-bar", "collapsed", "false");
      }
    }

    
    
    if (currentUIVersion < 17) {
      let currentset = xulStore.getValue(BROWSER_DOCURL, "nav-bar", "currentset");
      
      if (currentset) {
        if (!currentset.contains("bookmarks-menu-button")) {
          
          
          if (currentset.contains("downloads-button")) {
            currentset = currentset.replace(/(^|,)downloads-button($|,)/,
                                            "$1bookmarks-menu-button,downloads-button$2");
          } else if (currentset.contains("home-button")) {
            currentset = currentset.replace(/(^|,)home-button($|,)/,
                                            "$1bookmarks-menu-button,home-button$2");
          } else {
            
            currentset = currentset.replace(/(^|,)window-controls($|,)/,
                                            "$1bookmarks-menu-button,window-controls$2")
          }
          xulStore.setValue(BROWSER_DOCURL, "nav-bar", "currentset", currentset);
        }
      }
    }

    if (currentUIVersion < 18) {
      
      let toolbars = ["navigator-toolbox", "nav-bar", "PersonalToolbar",
                      "addon-bar", "TabsToolbar", "toolbar-menubar"];
      for (let resourceName of ["mode", "iconsize"]) {
        for (let toolbarId of toolbars) {
          xulStore.removeValue(BROWSER_DOCURL, toolbarId, resourceName);
        }
      }
    }

    if (currentUIVersion < 19) {
      let detector = null;    
      try {
        detector = Services.prefs.getComplexValue("intl.charset.detector",
                                                  Ci.nsIPrefLocalizedString).data;
      } catch (ex) {}
      if (!(detector == "" ||
            detector == "ja_parallel_state_machine" ||
            detector == "ruprob" ||
            detector == "ukprob")) {
        
        
        Services.prefs.clearUserPref("intl.charset.detector");
      }
    }

    if (currentUIVersion < 20) {
      
      xulStore.removeValue(BROWSER_DOCURL, "TabsToolbar", "collapsed");
    }

    if (currentUIVersion < 21) {
      
      
      xulStore.removeValue(BROWSER_DOCURL, "bookmarks-menu-button", "class");
    }

    if (currentUIVersion < 22) {
      
      Services.prefs.clearUserPref("browser.syncPromoViewsLeft");
      Services.prefs.clearUserPref("browser.syncPromoViewsLeftMap");
    }

    if (currentUIVersion < 23) {
      const kSelectedEnginePref = "browser.search.selectedEngine";
      if (Services.prefs.prefHasUserValue(kSelectedEnginePref)) {
        try {
          let name = Services.prefs.getComplexValue(kSelectedEnginePref,
                                                    Ci.nsIPrefLocalizedString).data;
          Services.search.currentEngine = Services.search.getEngineByName(name);
        } catch (ex) {}
      }
    }

    
    Services.prefs.setIntPref("browser.migration.version", UI_VERSION);
  },

  
  
  

  sanitize: function BG_sanitize(aParentWindow) {
    this._sanitizer.sanitize(aParentWindow);
  },

  ensurePlacesDefaultQueriesInitialized:
  function BG_ensurePlacesDefaultQueriesInitialized() {
    
    
    
    
    
    
    const SMART_BOOKMARKS_VERSION = 7;
    const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";
    const SMART_BOOKMARKS_PREF = "browser.places.smartBookmarksVersion";

    
    const MAX_RESULTS = 10;

    
    let smartBookmarksCurrentVersion = 0;
    try {
      smartBookmarksCurrentVersion = Services.prefs.getIntPref(SMART_BOOKMARKS_PREF);
    } catch(ex) {}

    
    if (smartBookmarksCurrentVersion == -1 ||
        smartBookmarksCurrentVersion >= SMART_BOOKMARKS_VERSION) {
      return;
    }

    let batch = {
      runBatched: function BG_EPDQI_runBatched() {
        let menuIndex = 0;
        let toolbarIndex = 0;
        let bundle = Services.strings.createBundle("chrome://browser/locale/places/places.properties");

        let smartBookmarks = {
          MostVisited: {
            title: bundle.GetStringFromName("mostVisitedTitle"),
            uri: NetUtil.newURI("place:sort=" +
                                Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING +
                                "&maxResults=" + MAX_RESULTS),
            parent: PlacesUtils.toolbarFolderId,
            get position() { return toolbarIndex++; },
            newInVersion: 1
          },
          RecentlyBookmarked: {
            title: bundle.GetStringFromName("recentlyBookmarkedTitle"),
            uri: NetUtil.newURI("place:folder=BOOKMARKS_MENU" +
                                "&folder=UNFILED_BOOKMARKS" +
                                "&folder=TOOLBAR" +
                                "&queryType=" +
                                Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS +
                                "&sort=" +
                                Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING +
                                "&maxResults=" + MAX_RESULTS +
                                "&excludeQueries=1"),
            parent: PlacesUtils.bookmarksMenuFolderId,
            get position() { return menuIndex++; },
            newInVersion: 1
          },
          RecentTags: {
            title: bundle.GetStringFromName("recentTagsTitle"),
            uri: NetUtil.newURI("place:"+
                                "type=" +
                                Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY +
                                "&sort=" +
                                Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_DESCENDING +
                                "&maxResults=" + MAX_RESULTS),
            parent: PlacesUtils.bookmarksMenuFolderId,
            get position() { return menuIndex++; },
            newInVersion: 1
          },
        };

        if (Services.metro && Services.metro.supported) {
          smartBookmarks.Windows8Touch = {
            title: PlacesUtils.getString("windows8TouchTitle"),
            get uri() {
              let metroBookmarksRoot = PlacesUtils.annotations.getItemsWithAnnotation('metro/bookmarksRoot', {});
              if (metroBookmarksRoot.length > 0) {
                return NetUtil.newURI("place:folder=" +
                                      metroBookmarksRoot[0] +
                                      "&queryType=" +
                                      Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS +
                                      "&sort=" +
                                      Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING +
                                      "&maxResults=" + MAX_RESULTS +
                                      "&excludeQueries=1")
              }
              return null;
            },
            parent: PlacesUtils.bookmarksMenuFolderId,
            get position() { return menuIndex++; },
            newInVersion: 7
          };
        }

        
        
        
        let smartBookmarkItemIds = PlacesUtils.annotations.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO);
        smartBookmarkItemIds.forEach(function (itemId) {
          let queryId = PlacesUtils.annotations.getItemAnnotation(itemId, SMART_BOOKMARKS_ANNO);
          if (queryId in smartBookmarks) {
            let smartBookmark = smartBookmarks[queryId];
            if (!smartBookmark.uri) {
              PlacesUtils.bookmarks.removeItem(itemId);
              return;
            }
            smartBookmark.itemId = itemId;
            smartBookmark.parent = PlacesUtils.bookmarks.getFolderIdForItem(itemId);
            smartBookmark.updatedPosition = PlacesUtils.bookmarks.getItemIndex(itemId);
          }
          else {
            
            
            
            PlacesUtils.annotations.removeItemAnnotation(itemId, SMART_BOOKMARKS_ANNO);
          }
        });

        for (let queryId in smartBookmarks) {
          let smartBookmark = smartBookmarks[queryId];

          
          
          
          if (smartBookmarksCurrentVersion > 0 &&
              smartBookmark.newInVersion <= smartBookmarksCurrentVersion &&
              !smartBookmark.itemId || !smartBookmark.uri)
            continue;

          
          
          if (smartBookmark.itemId) {
            PlacesUtils.bookmarks.removeItem(smartBookmark.itemId);
          }

          
          smartBookmark.itemId =
            PlacesUtils.bookmarks.insertBookmark(smartBookmark.parent,
                                                 smartBookmark.uri,
                                                 smartBookmark.updatedPosition || smartBookmark.position,
                                                 smartBookmark.title);
          PlacesUtils.annotations.setItemAnnotation(smartBookmark.itemId,
                                                    SMART_BOOKMARKS_ANNO,
                                                    queryId, 0,
                                                    PlacesUtils.annotations.EXPIRE_NEVER);
        }

        
        
        if (smartBookmarksCurrentVersion == 0 &&
            smartBookmarkItemIds.length == 0) {
          let id = PlacesUtils.bookmarks.getIdForItemAt(PlacesUtils.bookmarksMenuFolderId,
                                                        menuIndex);
          
          if (id != -1 &&
              PlacesUtils.bookmarks.getItemType(id) != PlacesUtils.bookmarks.TYPE_SEPARATOR) {
            PlacesUtils.bookmarks.insertSeparator(PlacesUtils.bookmarksMenuFolderId,
                                                  menuIndex);
          }
        }
      }
    };

    try {
      PlacesUtils.bookmarks.runInBatchMode(batch, null);
    }
    catch(ex) {
      Components.utils.reportError(ex);
    }
    finally {
      Services.prefs.setIntPref(SMART_BOOKMARKS_PREF, SMART_BOOKMARKS_VERSION);
      Services.prefs.savePrefFile(null);
    }
  },

  
  getMostRecentBrowserWindow: function BG_getMostRecentBrowserWindow() {
    return RecentWindow.getMostRecentBrowserWindow();
  },

#ifdef MOZ_SERVICES_SYNC
  










  _onDisplaySyncURI: function _onDisplaySyncURI(data) {
    try {
      let tabbrowser = RecentWindow.getMostRecentBrowserWindow({private: false}).gBrowser;

      
      tabbrowser.addTab(data.wrappedJSObject.object.uri);
    } catch (ex) {
      Cu.reportError("Error displaying tab received by Sync: " + ex);
    }
  },
#endif

  
  classID:          Components.ID("{eab9012e-5f74-4cbc-b2b5-a590235513cc}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIBrowserGlue]),

  
  _xpcom_factory: BrowserGlueServiceFactory,
}

function ContentPermissionPrompt() {}

ContentPermissionPrompt.prototype = {
  classID:          Components.ID("{d8903bf6-68d5-4e97-bcd1-e4d3012f721a}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPermissionPrompt]),

  _getBrowserForRequest: function (aRequest) {
    
    let browser = aRequest.element;
    if (!browser) {
      
      browser = aRequest.window.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIWebNavigation)
                                  .QueryInterface(Ci.nsIDocShell)
                                  .chromeEventHandler;
    }
    return browser;
  },

  













  _showPrompt: function CPP_showPrompt(aRequest, aMessage, aPermission, aActions,
                                       aNotificationId, aAnchorId, aOptions) {
    function onFullScreen() {
      popup.remove();
    }

    var browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");

    var browser = this._getBrowserForRequest(aRequest);
    var chromeWin = browser.ownerDocument.defaultView;
    var requestPrincipal = aRequest.principal;

    
    var popupNotificationActions = [];
    for (var i = 0; i < aActions.length; i++) {
      let promptAction = aActions[i];

      
      if (PrivateBrowsingUtils.isWindowPrivate(chromeWin) &&
          promptAction.expireType != Ci.nsIPermissionManager.EXPIRE_SESSION &&
          promptAction.action) {
        continue;
      }

      var action = {
        label: browserBundle.GetStringFromName(promptAction.stringId),
        accessKey: browserBundle.GetStringFromName(promptAction.stringId + ".accesskey"),
        callback: function() {
          if (promptAction.callback) {
            promptAction.callback();
          }

          
          if (promptAction.action) {
            Services.perms.addFromPrincipal(requestPrincipal, aPermission,
                                            promptAction.action, promptAction.expireType);
          }

          
          if (!promptAction.action || promptAction.action == Ci.nsIPermissionManager.ALLOW_ACTION) {
            aRequest.allow();
          } else {
            aRequest.cancel();
          }
        },
      };

      popupNotificationActions.push(action);
    }

    var mainAction = popupNotificationActions.length ?
                       popupNotificationActions[0] : null;
    var secondaryActions = popupNotificationActions.splice(1);

    
    let types = aRequest.types.QueryInterface(Ci.nsIArray);
    if (types.length != 1) {
      aRequest.cancel();
      return;
    }

    let perm = types.queryElementAt(0, Ci.nsIContentPermissionType);

    if (perm.type == "pointerLock") {
      
      let autoAllow = !mainAction;

      if (!aOptions)
        aOptions = {};

      aOptions.removeOnDismissal = autoAllow;
      aOptions.eventCallback = type => {
        if (type == "removed") {
          browser.removeEventListener("mozfullscreenchange", onFullScreen, true);
          if (autoAllow) {
            aRequest.allow();
          }
        }
      }

    }

    var popup = chromeWin.PopupNotifications.show(browser, aNotificationId, aMessage, aAnchorId,
                                                  mainAction, secondaryActions, aOptions);
    if (perm.type == "pointerLock") {
      
      
      
      
      browser.addEventListener("mozfullscreenchange", onFullScreen, true);
    }
  },

  _promptGeo : function(aRequest) {
    var secHistogram = Services.telemetry.getHistogramById("SECURITY_UI");
    var browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
    var requestingURI = aRequest.principal.URI;

    var message;

    
    var actions = [{
      stringId: "geolocation.shareLocation",
      action: null,
      expireType: null,
      callback: function() {
        secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_GEOLOCATION_REQUEST_SHARE_LOCATION);
      },
    }];

    if (requestingURI.schemeIs("file")) {
      message = browserBundle.formatStringFromName("geolocation.shareWithFile",
                                                   [requestingURI.path], 1);
    } else {
      message = browserBundle.formatStringFromName("geolocation.shareWithSite",
                                                   [requestingURI.host], 1);
      
      actions.push({
        stringId: "geolocation.alwaysShareLocation",
        action: Ci.nsIPermissionManager.ALLOW_ACTION,
        expireType: null,
        callback: function() {
          secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_GEOLOCATION_REQUEST_ALWAYS_SHARE);
        },
      });

      
      actions.push({
        stringId: "geolocation.neverShareLocation",
        action: Ci.nsIPermissionManager.DENY_ACTION,
        expireType: null,
        callback: function() {
          secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_GEOLOCATION_REQUEST_NEVER_SHARE);
        },
      });
    }

    var options = {
                    learnMoreURL: Services.urlFormatter.formatURLPref("browser.geolocation.warning.infoURL"),
                  };

    secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_GEOLOCATION_REQUEST);

    this._showPrompt(aRequest, message, "geo", actions, "geolocation",
                     "geo-notification-icon", options);
  },

  _promptWebNotifications : function(aRequest) {
    var browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
    var requestingURI = aRequest.principal.URI;

    var message = browserBundle.formatStringFromName("webNotifications.showFromSite",
                                                     [requestingURI.host], 1);

    var actions = [
      {
        stringId: "webNotifications.showForSession",
        action: Ci.nsIPermissionManager.ALLOW_ACTION,
        expireType: Ci.nsIPermissionManager.EXPIRE_SESSION,
        callback: function() {},
      },
      {
        stringId: "webNotifications.alwaysShow",
        action: Ci.nsIPermissionManager.ALLOW_ACTION,
        expireType: null,
        callback: function() {},
      },
      {
        stringId: "webNotifications.neverShow",
        action: Ci.nsIPermissionManager.DENY_ACTION,
        expireType: null,
        callback: function() {},
      },
    ];

    this._showPrompt(aRequest, message, "desktop-notification", actions,
                     "web-notifications",
                     "web-notifications-notification-icon", null);
  },

  _promptPointerLock: function CPP_promtPointerLock(aRequest, autoAllow) {

    let browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
    let requestingURI = aRequest.principal.URI;

    let originString = requestingURI.schemeIs("file") ? requestingURI.path : requestingURI.host;
    let message = browserBundle.formatStringFromName(autoAllow ?
                                  "pointerLock.autoLock.title2" : "pointerLock.title2",
                                  [originString], 1);
    
    
    let actions = [];
    if (!autoAllow) {
      actions = [
        {
          stringId: "pointerLock.allow2",
          action: null,
          expireType: null,
          callback: function() {},
        },
        {
          stringId: "pointerLock.alwaysAllow",
          action: Ci.nsIPermissionManager.ALLOW_ACTION,
          expireType: null,
          callback: function() {},
        },
        {
          stringId: "pointerLock.neverAllow",
          action: Ci.nsIPermissionManager.DENY_ACTION,
          expireType: null,
          callback: function() {},
        },
      ];
    }

    this._showPrompt(aRequest, message, "pointerLock", actions, "pointerLock",
                     "pointerLock-notification-icon", null);
  },

  prompt: function CPP_prompt(request) {

    
    let types = request.types.QueryInterface(Ci.nsIArray);
    if (types.length != 1) {
      request.cancel();
      return;
    }
    let perm = types.queryElementAt(0, Ci.nsIContentPermissionType);

    const kFeatureKeys = { "geolocation" : "geo",
                           "desktop-notification" : "desktop-notification",
                           "pointerLock" : "pointerLock",
                         };

    
    if (!(perm.type in kFeatureKeys)) {
        return;
    }

    var requestingPrincipal = request.principal;
    var requestingURI = requestingPrincipal.URI;

    
    if (!(requestingURI instanceof Ci.nsIStandardURL))
      return;

    var autoAllow = false;
    var permissionKey = kFeatureKeys[perm.type];
    var result = Services.perms.testExactPermissionFromPrincipal(requestingPrincipal, permissionKey);

    if (result == Ci.nsIPermissionManager.DENY_ACTION) {
      request.cancel();
      return;
    }

    if (result == Ci.nsIPermissionManager.ALLOW_ACTION) {
      autoAllow = true;
      
      if (perm.type != "pointerLock") {
        request.allow();
        return;
      }
    }

    var browser = this._getBrowserForRequest(request);
    var chromeWin = browser.ownerDocument.defaultView;
    if (!chromeWin.PopupNotifications)
      
      
      return;

    
    switch (perm.type) {
    case "geolocation":
      this._promptGeo(request);
      break;
    case "desktop-notification":
      this._promptWebNotifications(request);
      break;
    case "pointerLock":
      this._promptPointerLock(request, autoAllow);
      break;
    }
  },

};

let DefaultBrowserCheck = {
  get OPTIONPOPUP() { return "defaultBrowserNotificationPopup" },

  closePrompt: function(aNode) {
    if (this._notification) {
      this._notification.close();
    }
  },

  setAsDefault: function() {
    let claimAllTypes = true;
#ifdef XP_WIN
    try {
      
      
      
      
      let version = Services.sysinfo.getProperty("version");
      claimAllTypes = (parseFloat(version) < 6.2);
    } catch (ex) { }
#endif
    ShellService.setDefaultBrowser(claimAllTypes, false);
    this.closePrompt();
  },

  _createPopup: function(win, bundle) {
    let doc = win.document;
    let popup = doc.createElement("menupopup");
    popup.id = this.OPTIONPOPUP;

    let notNowItem = doc.createElement("menuitem");
    notNowItem.id = "defaultBrowserNotNow";
    let label = bundle.getString("setDefaultBrowserNotNow.label");
    notNowItem.setAttribute("label", label);
    let accesskey = bundle.getString("setDefaultBrowserNotNow.accesskey");
    notNowItem.setAttribute("accesskey", accesskey);
    popup.appendChild(notNowItem);

    let neverItem = doc.createElement("menuitem");
    neverItem.id = "defaultBrowserNever";
    let label = bundle.getString("setDefaultBrowserNever.label");
    neverItem.setAttribute("label", label);
    let accesskey = bundle.getString("setDefaultBrowserNever.accesskey");
    neverItem.setAttribute("accesskey", accesskey);
    popup.appendChild(neverItem);

    popup.addEventListener("command", this);

    let popupset = doc.getElementById("mainPopupSet");
    popupset.appendChild(popup);
  },

  handleEvent: function(event) {
    if (event.type == "command") {
      if (event.target.id == "defaultBrowserNever") {
        ShellService.shouldCheckDefaultBrowser = false;
      }
      this.closePrompt();
    }
  },

  prompt: function(win) {
    let brandBundle = win.document.getElementById("bundle_brand");
    let shellBundle = win.document.getElementById("bundle_shell");

    let brandShortName = brandBundle.getString("brandShortName");
    let promptMessage = shellBundle.getFormattedString("setDefaultBrowserMessage2",
                                                       [brandShortName]);

    let confirmMessage = shellBundle.getFormattedString("setDefaultBrowserConfirm.label",
                                                        [brandShortName]);
    let confirmKey = shellBundle.getString("setDefaultBrowserConfirm.accesskey");

    let optionsMessage = shellBundle.getString("setDefaultBrowserOptions.label");
    let optionsKey = shellBundle.getString("setDefaultBrowserOptions.accesskey");

    let selectedBrowser = win.gBrowser.selectedBrowser;
    let notificationBox = win.document.getElementById("high-priority-global-notificationbox");

    this._createPopup(win, shellBundle);

    let buttons = [
      {
        label: confirmMessage,
        accessKey: confirmKey,
        callback: this.setAsDefault.bind(this)
      },
      {
        label: optionsMessage,
        accessKey: optionsKey,
        popup: this.OPTIONPOPUP
      }
    ];


    let iconPixels = win.devicePixelRatio > 1 ? "32" : "16";
    let iconURL = "chrome://branding/content/icon" + iconPixels + ".png";
    const priority = notificationBox.PRIORITY_INFO_HIGH;
    let callback = this._onNotificationEvent.bind(this);
    this._notification = notificationBox.appendNotification(promptMessage, "default-browser",
                                                            iconURL, priority, buttons,
                                                            callback);
    this._notification.persistence = -1;
  },

  _onNotificationEvent: function(eventType) {
    if (eventType == "removed") {
      let doc = this._notification.ownerDocument;
      let popup = doc.getElementById(this.OPTIONPOPUP);
      popup.removeEventListener("command", this);
      popup.remove();
      delete this._notification;
    }
  },
};

#ifdef E10S_TESTING_ONLY
let E10SUINotification = {
  
  
  CURRENT_NOTICE_COUNT: 0,

  checkStatus: function() {
    if (Services.appinfo.browserTabsRemoteAutostart) {
      let notice = 0;
      try {
        notice = Services.prefs.getIntPref("browser.displayedE10SNotice");
      } catch(e) {}
      let activationNoticeShown = notice >= this.CURRENT_NOTICE_COUNT;

      if (!activationNoticeShown) {
        this._showE10sActivatedNotice();
      }
    } else {
      let e10sPromptShownCount = 0;
      try {
        e10sPromptShownCount = Services.prefs.getIntPref("browser.displayedE10SPrompt");
      } catch(e) {}

      if (!Services.appinfo.inSafeMode && e10sPromptShownCount < 5) {
        Services.tm.mainThread.dispatch(() => {
          try {
            this._showE10SPrompt();
            Services.prefs.setIntPref("browser.displayedE10SPrompt", e10sPromptShownCount + 1);
          } catch (ex) {
            Cu.reportError("Failed to show e10s prompt: " + ex);
          }
        }, Ci.nsIThread.DISPATCH_NORMAL);
      }
    }
  },

  _showE10sActivatedNotice: function() {
    let win = RecentWindow.getMostRecentBrowserWindow();
    if (!win)
      return;

    Services.prefs.setIntPref("browser.displayedE10SNotice", this.CURRENT_NOTICE_COUNT);

    let nb = win.document.getElementById("high-priority-global-notificationbox");
    let message = "Thanks for helping to test multiprocess Firefox (e10s). Some functions might not work yet."
    let buttons = [
      {
        label: "Learn More",
        accessKey: "L",
        callback: function () {
          win.openUILinkIn("https://wiki.mozilla.org/Electrolysis", "tab");
        }
      }
    ];
    nb.appendNotification(message, "e10s-activated-noticed",
                          null, nb.PRIORITY_WARNING_MEDIUM, buttons);

  },

  _showE10SPrompt: function BG__showE10SPrompt() {
    let win = RecentWindow.getMostRecentBrowserWindow();
    if (!win)
      return;

    let browser = win.gBrowser.selectedBrowser;

    let promptMessage = "Would you like to help us test multiprocess Nightly (e10s)? You can also enable e10s in Nightly preferences.";
    let mainAction = {
      label: "Enable and Restart",
      accessKey: "E",
      callback: function () {
        Services.prefs.setBoolPref("browser.tabs.remote.autostart", true);
        Services.prefs.setBoolPref("browser.enabledE10SFromPrompt", true);
        
        let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
        Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");
        if (cancelQuit.data)
          return; 
        Services.startup.quit(Services.startup.eAttemptQuit | Services.startup.eRestart);
      }
    };
    let secondaryActions = [
      {
        label: "No thanks",
        accessKey: "N",
        callback: function () {
          Services.prefs.setIntPref("browser.displayedE10SPrompt", 5);
        }
      }
    ];
    let options = {
      popupIconURL: "chrome://browser/skin/e10s-64@2x.png",
      learnMoreURL: "https://wiki.mozilla.org/Electrolysis",
      persistWhileVisible: true
    };

    win.PopupNotifications.show(browser, "enable_e10s", promptMessage, null, mainAction, secondaryActions, options);
  },
};
#endif

var components = [BrowserGlue, ContentPermissionPrompt];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
