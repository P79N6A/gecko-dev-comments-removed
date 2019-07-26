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
Cu.import("resource:///modules/SignInToWebsite.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AboutHome",
                                  "resource:///modules/AboutHome.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ContentClick",
                                  "resource:///modules/ContentClick.jsm");

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

XPCOMUtils.defineLazyModuleGetter(this, "webappsUI",
                                  "resource:///modules/webappsUI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PageThumbs",
                                  "resource://gre/modules/PageThumbs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NewTabUtils",
                                  "resource://gre/modules/NewTabUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "BrowserNewTabPreloader",
                                  "resource:///modules/BrowserNewTabPreloader.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PdfJs",
                                  "resource://pdf.js/PdfJs.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ShumwayUtils",
                                  "resource://shumway/ShumwayUtils.jsm");

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

XPCOMUtils.defineLazyModuleGetter(this, "SessionStore",
                                  "resource:///modules/sessionstore/SessionStore.jsm");

const PREF_PLUGINS_NOTIFYUSER = "plugins.update.notifyUser";
const PREF_PLUGINS_UPDATEURL  = "plugins.update.url";



const BOOKMARKS_BACKUP_IDLE_TIME = 10 * 60;

const BOOKMARKS_BACKUP_INTERVAL = 86400 * 1000;

const BOOKMARKS_BACKUP_MAX_BACKUPS = 10;


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
  _isIdleObserver: false,
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
        
        
        this._setPrefToSaveSession();
        try {
          let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                           getService(Ci.nsIAppStartup);
          appStartup.trackStartupCrashEnd();
        } catch (e) {
          Cu.reportError("Could not end startup crash tracking in quit-application-granted: " + e);
        }
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
        if (this._idleService.idleTime > BOOKMARKS_BACKUP_IDLE_TIME * 1000)
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
        this._onProfileShutdown();
        break;
#ifdef MOZ_SERVICES_HEALTHREPORT
      case "keyword-search":
        
        
        
        
        let reporter = Cc["@mozilla.org/datareporting/service;1"]
                         .getService()
                         .wrappedJSObject
                         .healthReporter;

        if (!reporter) {
          return;
        }

        reporter.onInit().then(function record() {
          try {
            let name = subject.QueryInterface(Ci.nsISearchEngine).name;
            reporter.getProvider("org.mozilla.searches").recordSearch(name,
                                                                      "urlbar");
          } catch (ex) {
            Cu.reportError(ex);
          }
        });
        break;
#endif
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
    if (this._isIdleObserver)
      this._idleService.removeIdleObserver(this, BOOKMARKS_BACKUP_IDLE_TIME);
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

    webappsUI.init();
    PageThumbs.init();
    NewTabUtils.init();
    BrowserNewTabPreloader.init();
    SignInToWebsiteUX.init();
    PdfJs.init();
    ShumwayUtils.init();
    webrtcUI.init();
    AboutHome.init();
    SessionStore.init();

    if (Services.prefs.getBoolPref("browser.tabs.remote"))
      ContentClick.init();

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
    if (lastUse &&
        Date.now() - lastUse >= OFFER_PROFILE_RESET_INTERVAL_MS) {
      this._resetUnusedProfileNotification();
    }

    this._checkForOldBuildUpdates();
  },

  




  _onProfileShutdown: function BG__onProfileShutdown() {
    BrowserNewTabPreloader.uninit();
    webappsUI.uninit();
    SignInToWebsiteUX.uninit();
    webrtcUI.uninit();
    this._dispose();
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

    
    var shell;
    try {
      shell = Components.classes["@mozilla.org/browser/shell-service;1"]
        .getService(Components.interfaces.nsIShellService);
    } catch (e) { }
    if (shell) {
#ifdef DEBUG
      let shouldCheck = false;
#else
      let shouldCheck = shell.shouldCheckDefaultBrowser;
#endif
      let willRecoverSession = false;
      try {
        let ss = Cc["@mozilla.org/browser/sessionstartup;1"].
                 getService(Ci.nsISessionStartup);
        willRecoverSession =
          (ss.sessionType == Ci.nsISessionStartup.RECOVER_SESSION);
      }
      catch (ex) {  }

      let isDefault = shell.isDefaultBrowser(true, false); 
      try {
        
        
        Services.telemetry.getHistogramById("BROWSER_IS_USER_DEFAULT")
                          .add(isDefault);
      }
      catch (ex) {  }

      if (shouldCheck && !isDefault && !willRecoverSession) {
        Services.tm.mainThread.dispatch(function() {
          var win = this.getMostRecentBrowserWindow();
          var brandBundle = win.document.getElementById("bundle_brand");
          var shellBundle = win.document.getElementById("bundle_shell");

          var brandShortName = brandBundle.getString("brandShortName");
          var promptTitle = shellBundle.getString("setDefaultBrowserTitle");
          var promptMessage = shellBundle.getFormattedString("setDefaultBrowserMessage",
                                                             [brandShortName]);
          var checkboxLabel = shellBundle.getFormattedString("setDefaultBrowserDontAsk",
                                                             [brandShortName]);
          var checkEveryTime = { value: shouldCheck };
          var ps = Services.prompt;
          var rv = ps.confirmEx(win, promptTitle, promptMessage,
                                ps.STD_YES_NO_BUTTONS,
                                null, null, null, checkboxLabel, checkEveryTime);
          if (rv == 0) {
            var claimAllTypes = true;
#ifdef XP_WIN
            try {
              
              
              
              
              let version = Cc["@mozilla.org/system-info;1"]
                              .getService(Ci.nsIPropertyBag2)
                              .getProperty("version");
              claimAllTypes = (parseFloat(version) < 6.2);
            } catch (ex) { }
#endif
            shell.setDefaultBrowser(claimAllTypes, false);
          }
          shell.shouldCheckDefaultBrowser = checkEveryTime.value;
        }.bind(this), Ci.nsIThread.DISPATCH_NORMAL);
      }
    }
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

    var mostRecentBrowserWindow;

    
    
    

    var sessionWillBeRestored = Services.prefs.getIntPref("browser.startup.page") == 3 ||
                                Services.prefs.getBoolPref("browser.sessionstore.resume_session_once");
    if (sessionWillBeRestored || !Services.prefs.getBoolPref("browser.warnOnQuit"))
      return;

    
    
    if (!Services.prefs.getBoolPref("browser.showQuitWarning")) {
      if (aQuitType == "lastwindow") {
        
        
        
        
        mostRecentBrowserWindow = Services.wm.getMostRecentWindow("navigator:browser");
        let allTabs = mostRecentBrowserWindow.gBrowser.closingTabsEnum.ALL;
        aCancelQuit.data = !mostRecentBrowserWindow.gBrowser.warnAboutClosingTabs(allTabs)
      }
      return;
    }

    
    if (allWindowsPrivate)
      return;

    var quitBundle = Services.strings.createBundle("chrome://browser/locale/quitDialog.properties");
    var brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");

    var appName = brandBundle.GetStringFromName("brandShortName");
    var quitTitleString = "quitDialogTitle";
    var quitDialogTitle = quitBundle.formatStringFromName(quitTitleString, [appName], 1);

    var message;
    if (windowcount == 1)
      message = quitBundle.formatStringFromName("messageNoWindows",
                                                [appName], 1);
    else
      message = quitBundle.formatStringFromName("message",
                                                [appName], 1);

    var promptService = Services.prompt;

    var flags = promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0 +
                promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_1 +
                promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_2 +
                promptService.BUTTON_POS_0_DEFAULT;

    var neverAsk = {value:false};
    var button0Title = quitBundle.GetStringFromName("saveTitle");
    var button1Title = quitBundle.GetStringFromName("cancelTitle");
    var button2Title = quitBundle.GetStringFromName("quitTitle");
    var neverAskText = quitBundle.GetStringFromName("neverAsk2");

    
    
    mostRecentBrowserWindow = Services.wm.getMostRecentWindow("navigator:browser");

    var buttonChoice =
      promptService.confirmEx(mostRecentBrowserWindow, quitDialogTitle, message,
                              flags, button0Title, button1Title, button2Title,
                              neverAskText, neverAsk);

    switch (buttonChoice) {
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
    
    
    
    
    
    var dbStatus = PlacesUtils.history.databaseStatus;
    var importBookmarks = !aInitialMigrationPerformed &&
                          (dbStatus == PlacesUtils.history.DATABASE_STATUS_CREATE ||
                           dbStatus == PlacesUtils.history.DATABASE_STATUS_CORRUPT);

    
    var importBookmarksHTML = false;
    try {
      importBookmarksHTML =
        Services.prefs.getBoolPref("browser.places.importBookmarksHTML");
      if (importBookmarksHTML)
        importBookmarks = true;
    } catch(ex) {}

    Task.spawn(function() {
      
      
      var restoreDefaultBookmarks = false;
      try {
        restoreDefaultBookmarks =
          Services.prefs.getBoolPref("browser.bookmarks.restore_default_bookmarks");
        if (restoreDefaultBookmarks) {
          
          yield this._backupBookmarks();
          importBookmarks = true;
        }
      } catch(ex) {}

      
      
      if (importBookmarks && !restoreDefaultBookmarks && !importBookmarksHTML) {
        
        var bookmarksBackupFile = yield PlacesBackups.getMostRecent("json");
        if (bookmarksBackupFile) {
          
          yield BookmarkJSONUtils.importFromFile(bookmarksBackupFile, true);
          importBookmarks = false;
        }
        else {
          
          importBookmarks = true;
          var dirService = Cc["@mozilla.org/file/directory_service;1"].
                           getService(Ci.nsIProperties);
          var bookmarksHTMLFile = dirService.get("BMarks", Ci.nsILocalFile);
          if (bookmarksHTMLFile.exists()) {
            
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
        
        
        
        var autoExportHTML = false;
        try {
          autoExportHTML = Services.prefs.getBoolPref("browser.bookmarks.autoExportHTML");
        } catch(ex) {}
        var smartBookmarksVersion = 0;
        try {
          smartBookmarksVersion = Services.prefs.getIntPref("browser.places.smartBookmarksVersion");
        } catch(ex) {}
        if (!autoExportHTML && smartBookmarksVersion != -1)
          Services.prefs.setIntPref("browser.places.smartBookmarksVersion", 0);

        
        var dirService = Cc["@mozilla.org/file/directory_service;1"].
                         getService(Ci.nsIProperties);

        var bookmarksURI = null;
        if (restoreDefaultBookmarks) {
          
          bookmarksURI = NetUtil.newURI("resource:///defaults/profile/bookmarks.html");
        }
        else {
          var bookmarksFile = dirService.get("BMarks", Ci.nsILocalFile);
          if (bookmarksFile.exists())
            bookmarksURI = NetUtil.newURI(bookmarksFile);
        }

        if (bookmarksURI) {
          
          try {
            BookmarkHTMLUtils.importFromURL(bookmarksURI.spec, true).then(null,
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

      
      
      if (!this._isIdleObserver) {
        this._idleService.addIdleObserver(this, BOOKMARKS_BACKUP_IDLE_TIME);
        this._isIdleObserver = true;
      }

      Services.obs.notifyObservers(null, "places-browser-init-complete", "");
    }.bind(this));
  },

  





  _onPlacesShutdown: function BG__onPlacesShutdown() {
    this._sanitizer.onShutdown();
    PageThumbs.uninit();

    if (this._isIdleObserver) {
      this._idleService.removeIdleObserver(this, BOOKMARKS_BACKUP_IDLE_TIME);
      this._isIdleObserver = false;
    }

    let waitingForBackupToComplete = true;
    this._backupBookmarks().then(
      function onSuccess() {
        waitingForBackupToComplete = false;
      },
      function onFailure() {
        Cu.reportError("Unable to backup bookmarks.");
        waitingForBackupToComplete = false;
      }
    );

    
    
    let waitingForHTMLExportToComplete = false;
    
    if (Services.prefs.getBoolPref("browser.bookmarks.autoExportHTML")) {
      
      
      
      
      
      
      waitingForHTMLExportToComplete = true;
      BookmarkHTMLUtils.exportToFile(Services.dirsvc.get("BMarks", Ci.nsIFile)).then(
        function onSuccess() {
          waitingForHTMLExportToComplete = false;
        },
        function onFailure() {
          Cu.reportError("Unable to auto export html.");
          waitingForHTMLExportToComplete = false;
        }
      );
    }

    
    
    let thread = Services.tm.currentThread;
    while (waitingForBackupToComplete || waitingForHTMLExportToComplete) {
      thread.processNextEvent(true);
    }
  },

  


  _backupBookmarks: function BG__backupBookmarks() {
    return Task.spawn(function() {
      let lastBackupFile = yield PlacesBackups.getMostRecentBackup();
      
      
      if (!lastBackupFile ||
          new Date() - PlacesBackups.getDateForFile(lastBackupFile) > BOOKMARKS_BACKUP_INTERVAL) {
        let maxBackups = BOOKMARKS_BACKUP_MAX_BACKUPS;
        try {
          maxBackups = Services.prefs.getIntPref("browser.bookmarks.max_backups");
        }
        catch(ex) {  }

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
    const UI_VERSION = 14;
    const BROWSER_DOCURL = "chrome://browser/content/browser.xul#";
    let currentUIVersion = 0;
    try {
      currentUIVersion = Services.prefs.getIntPref("browser.migration.version");
    } catch(ex) {}
    if (currentUIVersion >= UI_VERSION)
      return;

    this._rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
    this._dataSource = this._rdf.GetDataSource("rdf:local-store");
    this._dirty = false;

    if (currentUIVersion < 2) {
      
      let currentsetResource = this._rdf.GetResource("currentset");
      let toolbarResource = this._rdf.GetResource(BROWSER_DOCURL + "nav-bar");
      let currentset = this._getPersist(toolbarResource, currentsetResource);
      
      if (currentset &&
          currentset.indexOf("bookmarks-menu-button-container") == -1) {
        currentset += ",bookmarks-menu-button-container";
        this._setPersist(toolbarResource, currentsetResource, currentset);
      }
    }

    if (currentUIVersion < 3) {
      
      let currentsetResource = this._rdf.GetResource("currentset");
      let toolbarResource = this._rdf.GetResource(BROWSER_DOCURL + "nav-bar");
      let currentset = this._getPersist(toolbarResource, currentsetResource);
      
      if (currentset &&
          currentset.indexOf("reload-button") != -1 &&
          currentset.indexOf("stop-button") != -1 &&
          currentset.indexOf("urlbar-container") != -1 &&
          currentset.indexOf("urlbar-container,reload-button,stop-button") == -1) {
        currentset = currentset.replace(/(^|,)reload-button($|,)/, "$1$2")
                               .replace(/(^|,)stop-button($|,)/, "$1$2")
                               .replace(/(^|,)urlbar-container($|,)/,
                                        "$1urlbar-container,reload-button,stop-button$2");
        this._setPersist(toolbarResource, currentsetResource, currentset);
      }
    }

    if (currentUIVersion < 4) {
      
      let currentsetResource = this._rdf.GetResource("currentset");
      let toolbarResource = this._rdf.GetResource(BROWSER_DOCURL + "nav-bar");
      let currentset = this._getPersist(toolbarResource, currentsetResource);
      
      if (currentset &&
          currentset.indexOf("home-button") != -1 &&
          currentset.indexOf("bookmarks-menu-button-container") != -1) {
        currentset = currentset.replace(/(^|,)home-button($|,)/, "$1$2")
                               .replace(/(^|,)bookmarks-menu-button-container($|,)/,
                                        "$1home-button,bookmarks-menu-button-container$2");
        this._setPersist(toolbarResource, currentsetResource, currentset);
      }
    }

    if (currentUIVersion < 5) {
      
      
      let toolbarResource = this._rdf.GetResource(BROWSER_DOCURL + "PersonalToolbar");
      let collapsedResource = this._rdf.GetResource("collapsed");
      let collapsed = this._getPersist(toolbarResource, collapsedResource);
      
      
      if (collapsed === null) {
        
        
        let currentsetResource = this._rdf.GetResource("currentset");
        let toolbarIsCustomized = !!this._getPersist(toolbarResource,
                                                     currentsetResource);
        function getToolbarFolderCount() {
          let toolbarFolder =
            PlacesUtils.getFolderContents(PlacesUtils.toolbarFolderId).root;
          let toolbarChildCount = toolbarFolder.childCount;
          toolbarFolder.containerOpen = false;
          return toolbarChildCount;
        }

        if (toolbarIsCustomized || getToolbarFolderCount() > 3) {
          this._setPersist(toolbarResource, collapsedResource, "false");
        }
      }
    }

    if (currentUIVersion < 6) {
      
      let toolboxResource = this._rdf.GetResource(BROWSER_DOCURL + "navigator-toolbox");
      let tabsOnTopResource = this._rdf.GetResource("tabsontop");
      let tabsOnTopAttribute = this._getPersist(toolboxResource, tabsOnTopResource);
      if (tabsOnTopAttribute)
        Services.prefs.setBoolPref("browser.tabs.onTop", tabsOnTopAttribute == "true");
    }

    
    
    

    if (currentUIVersion < 8) {
      
      let uri = Services.prefs.getComplexValue("browser.startup.homepage",
                                               Ci.nsIPrefLocalizedString).data;
      if (uri && /^https?:\/\/(www\.)?google(\.\w{2,3}){1,2}\/firefox\/?$/.test(uri)) {
        Services.prefs.clearUserPref("browser.startup.homepage");
      }
    }

    if (currentUIVersion < 9) {
      
      let currentsetResource = this._rdf.GetResource("currentset");
      let toolbarResource = this._rdf.GetResource(BROWSER_DOCURL + "nav-bar");
      let currentset = this._getPersist(toolbarResource, currentsetResource);

      
      
      
      
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
        this._setPersist(toolbarResource, currentsetResource, currentset);
      }

      Services.prefs.clearUserPref("browser.download.useToolkitUI");
      Services.prefs.clearUserPref("browser.library.useNewDownloadsView");
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
      
      
      let currentsetResource = this._rdf.GetResource("currentset");
      let toolbarResource = this._rdf.GetResource(BROWSER_DOCURL + "nav-bar");
      let currentset = this._getPersist(toolbarResource, currentsetResource);
      
      if (currentset) {
        if (currentset.contains("bookmarks-menu-button-container")) {
          currentset = currentset.replace(/(^|,)bookmarks-menu-button-container($|,)/,
                                          "$1bookmarks-menu-button$2");
          this._setPersist(toolbarResource, currentsetResource, currentset);
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

    if (this._dirty)
      this._dataSource.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();

    delete this._rdf;
    delete this._dataSource;

    
    Services.prefs.setIntPref("browser.migration.version", UI_VERSION);
  },

  _getPersist: function BG__getPersist(aSource, aProperty) {
    var target = this._dataSource.GetTarget(aSource, aProperty, true);
    if (target instanceof Ci.nsIRDFLiteral)
      return target.Value;
    return null;
  },

  _setPersist: function BG__setPersist(aSource, aProperty, aTarget) {
    this._dirty = true;
    try {
      var oldTarget = this._dataSource.GetTarget(aSource, aProperty, true);
      if (oldTarget) {
        if (aTarget)
          this._dataSource.Change(aSource, aProperty, oldTarget, this._rdf.GetLiteral(aTarget));
        else
          this._dataSource.Unassert(aSource, aProperty, oldTarget);
      }
      else {
        this._dataSource.Assert(aSource, aProperty, this._rdf.GetLiteral(aTarget), true);
      }

      
      
      let docURL = aSource.ValueUTF8.split("#")[0];
      let docResource = this._rdf.GetResource(docURL);
      let persistResource = this._rdf.GetResource("http://home.netscape.com/NC-rdf#persist");
      if (!this._dataSource.HasAssertion(docResource, persistResource, aSource, true)) {
        this._dataSource.Assert(docResource, persistResource, aSource, true);
      }
    }
    catch(ex) {}
  },

  
  
  

  sanitize: function BG_sanitize(aParentWindow) {
    this._sanitizer.sanitize(aParentWindow);
  },

  ensurePlacesDefaultQueriesInitialized:
  function BG_ensurePlacesDefaultQueriesInitialized() {
    
    
    
    
    
    
    const SMART_BOOKMARKS_VERSION = 4;
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
            position: toolbarIndex++,
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
            position: menuIndex++,
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
            position: menuIndex++,
            newInVersion: 1
          }
        };

        
        
        
        let smartBookmarkItemIds = PlacesUtils.annotations.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO);
        smartBookmarkItemIds.forEach(function (itemId) {
          let queryId = PlacesUtils.annotations.getItemAnnotation(itemId, SMART_BOOKMARKS_ANNO);
          if (queryId in smartBookmarks) {
            let smartBookmark = smartBookmarks[queryId];
            smartBookmark.itemId = itemId;
            smartBookmark.parent = PlacesUtils.bookmarks.getFolderIdForItem(itemId);
            smartBookmark.position = PlacesUtils.bookmarks.getItemIndex(itemId);
          }
          else {
            
            
            
            PlacesUtils.annotations.removeItemAnnotation(itemId, SMART_BOOKMARKS_ANNO);
          }
        });

        for (let queryId in smartBookmarks) {
          let smartBookmark = smartBookmarks[queryId];

          
          
          
          if (smartBookmarksCurrentVersion > 0 &&
              smartBookmark.newInVersion <= smartBookmarksCurrentVersion &&
              !smartBookmark.itemId)
            continue;

          
          
          if (smartBookmark.itemId) {
            PlacesUtils.bookmarks.removeItem(smartBookmark.itemId);
          }

          
          smartBookmark.itemId =
            PlacesUtils.bookmarks.insertBookmark(smartBookmark.parent,
                                                 smartBookmark.uri,
                                                 smartBookmark.position,
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

    if (aRequest.type == "pointerLock") {
      
      let autoAllow = !mainAction;
      aOptions = {
        removeOnDismissal: autoAllow,
        eventCallback: type => {
          if (type == "removed") {
            browser.removeEventListener("mozfullscreenchange", onFullScreen, true);
            if (autoAllow) {
              aRequest.allow();
            }
          }
        },
      };
    }

    var popup = chromeWin.PopupNotifications.show(browser, aNotificationId, aMessage, aAnchorId,
                                                  mainAction, secondaryActions, aOptions);
    if (aRequest.type == "pointerLock") {
      
      
      
      
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

    var chromeWin = this._getBrowserForRequest(aRequest).ownerDocument.defaultView;
    var link = chromeWin.document.getElementById("geolocation-learnmore-link");
    link.value = browserBundle.GetStringFromName("geolocation.learnMore");
    link.href = Services.urlFormatter.formatURLPref("browser.geolocation.warning.infoURL");

    secHistogram.add(Ci.nsISecurityUITelemetry.WARNING_GEOLOCATION_REQUEST);

    this._showPrompt(aRequest, message, "geo", actions, "geolocation",
                     "geo-notification-icon", null);
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

    const kFeatureKeys = { "geolocation" : "geo",
                           "desktop-notification" : "desktop-notification",
                           "pointerLock" : "pointerLock",
                         };

    
    if (!(request.type in kFeatureKeys)) {
        return;
    }

    var requestingPrincipal = request.principal;
    var requestingURI = requestingPrincipal.URI;

    
    if (!(requestingURI instanceof Ci.nsIStandardURL))
      return;

    var autoAllow = false;
    var permissionKey = kFeatureKeys[request.type];
    var result = Services.perms.testExactPermissionFromPrincipal(requestingPrincipal, permissionKey);

    if (result == Ci.nsIPermissionManager.DENY_ACTION) {
      request.cancel();
      return;
    }

    if (result == Ci.nsIPermissionManager.ALLOW_ACTION) {
      autoAllow = true;
      
      if (request.type != "pointerLock") {
        request.allow();
        return;
      }
    }

    
    switch (request.type) {
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

var components = [BrowserGlue, ContentPermissionPrompt];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
