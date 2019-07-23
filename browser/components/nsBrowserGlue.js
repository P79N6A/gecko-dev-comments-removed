# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the Browser Search Service.
#
# The Initial Developer of the Original Code is
# Giorgio Maone
# Portions created by the Initial Developer are Copyright (C) 2005
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Giorgio Maone <g.maone@informaction.com>
#   Seth Spitzer <sspitzer@mozilla.com>
#   Asaf Romano <mano@mozilla.com>
#   Marco Bonardo <mak77@bonardo.net>
#   Dietrich Ayala <dietrich@mozilla.com>
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
#   Nils Maier <maierman@web.de>
#   Robert Strong <robert.bugzilla@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

const XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const PREF_EM_NEW_ADDONS_LIST = "extensions.newAddons";
const PREF_PLUGINS_NOTIFYUSER = "plugins.update.notifyUser";
const PREF_PLUGINS_UPDATEURL  = "plugins.update.url";



const BOOKMARKS_BACKUP_IDLE_TIME = 15 * 60;

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

  _setPrefToSaveSession: function BG__setPrefToSaveSession(aForce) {
    if (!this._saveSession && !aForce)
      return;

    Services.prefs.setBoolPref("browser.sessionstore.resume_session_once", true);

    
    
    
    Services.prefs.savePrefFile(null);
  },

  
  observe: function BG_observe(subject, topic, data) {
    switch (topic) {
      case "xpcom-shutdown":
        this._dispose();
        break;
      case "prefservice:after-app-defaults":
        this._onAppDefaults();
        break;
      case "final-ui-startup":
        this._onProfileStartup();
        break;
      case "sessionstore-windows-restored":
        this._onBrowserStartup();
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
        break;
#ifdef OBSERVE_LASTWINDOW_CLOSE_TOPICS
      case "browser-lastwindow-close-requested":
        
        
        this._onQuitRequest(subject, "lastwindow");
        break;
      case "browser-lastwindow-close-granted":
        this._setPrefToSaveSession();
        break;
#endif
      case "session-save":
        this._setPrefToSaveSession(true);
        subject.QueryInterface(Ci.nsISupportsPRBool);
        subject.data = true;
        break;
      case "places-init-complete":
        this._initPlaces();
        Services.obs.removeObserver(this, "places-init-complete");
        this._isPlacesInitObserver = false;
        
        Services.obs.removeObserver(this, "places-database-locked");
        this._isPlacesLockedObserver = false;

        
        
        this._distributionCustomizer.applyBookmarks();
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
        
        
        this._onProfileShutdown();
        break;
      case "idle":
        if (this._idleService.idleTime > BOOKMARKS_BACKUP_IDLE_TIME * 1000)
          this._backupBookmarks();
        break;
      case "distribution-customization-complete":
        Services.obs.removeObserver(this, "distribution-customization-complete");
        
        delete this._distributionCustomizer;
        break;
      case "bookmarks-restore-success":
      case "bookmarks-restore-failed":
        Services.obs.removeObserver(this, "bookmarks-restore-success");
        Services.obs.removeObserver(this, "bookmarks-restore-failed");
        if (topic == "bookmarks-restore-success" && data == "html-initial")
          this.ensurePlacesDefaultQueriesInitialized();
        break;
      case "browser-glue-test": 
        if (data == "post-update-notification") {
          if (Services.prefs.prefHasUserValue("app.update.postupdate"))
            this._showUpdateNotification();
          break;
        }
        break;
    }
  }, 

  
  _init: function BG__init() {
    let os = Services.obs;
    os.addObserver(this, "xpcom-shutdown", false);
    os.addObserver(this, "prefservice:after-app-defaults", false);
    os.addObserver(this, "final-ui-startup", false);
    os.addObserver(this, "sessionstore-windows-restored", false);
    os.addObserver(this, "browser:purge-session-history", false);
    os.addObserver(this, "quit-application-requested", false);
    os.addObserver(this, "quit-application-granted", false);
#ifdef OBSERVE_LASTWINDOW_CLOSE_TOPICS
    os.addObserver(this, "browser-lastwindow-close-requested", false);
    os.addObserver(this, "browser-lastwindow-close-granted", false);
#endif
    os.addObserver(this, "session-save", false);
    os.addObserver(this, "places-init-complete", false);
    this._isPlacesInitObserver = true;
    os.addObserver(this, "places-database-locked", false);
    this._isPlacesLockedObserver = true;
    os.addObserver(this, "distribution-customization-complete", false);
    os.addObserver(this, "places-shutdown", false);
    this._isPlacesShutdownObserver = true;
  },

  
  _dispose: function BG__dispose() {
    let os = Services.obs;
    os.removeObserver(this, "xpcom-shutdown");
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
    os.removeObserver(this, "session-save");
    if (this._isIdleObserver)
      this._idleService.removeIdleObserver(this, BOOKMARKS_BACKUP_IDLE_TIME);
    if (this._isPlacesInitObserver)
      os.removeObserver(this, "places-init-complete");
    if (this._isPlacesLockedObserver)
      os.removeObserver(this, "places-database-locked");
    if (this._isPlacesShutdownObserver)
      os.removeObserver(this, "places-shutdown");
  },

  _onAppDefaults: function BG__onAppDefaults() {
    
    
    this._distributionCustomizer.applyPrefDefaults();
  },

  
  _onProfileStartup: function BG__onProfileStartup() {
    this._sanitizer.onStartup();
    
    if (Services.appinfo.inSafeMode) {
      Services.ww.openWindow(null, "chrome://browser/content/safeMode.xul", 
                             "_blank", "chrome,centerscreen,modal,resizable=no", null);
    }

    
    
    this._distributionCustomizer.applyCustomizations();

    
    this._migrateUI();

    
    
    
    if (!Services.io.manageOfflineStatus) {
      
      try {
        Services.io.offline = Services.prefs.getBoolPref("browser.offline");
      }
      catch (e) {
        Services.io.offline = false;
      }
    }

    Services.obs.notifyObservers(null, "browser-ui-startup-complete", "");
  },

  
  _onProfileShutdown: function BG__onProfileShutdown() {
#ifdef WINCE
    
    try {
      let um = Cc["@mozilla.org/updates/update-manager;1"].
               getService(Ci.nsIUpdateManager);
      if (um.activeUpdate && um.activeUpdate.state == "pending") {
        let cacheService = Cc["@mozilla.org/network/cache-service;1"].
                           getService(Ci.nsICacheService);
        cacheService.evictEntries(Ci.nsICache.STORE_ANYWHERE);
      }
    } catch (e) { }
#endif
    this._shutdownPlaces();
    this._sanitizer.onShutdown();
  },

  
  _onBrowserStartup: function BG__onBrowserStartup() {
    
    if (this._shouldShowRights())
      this._showRightsNotification();

    
    if (Services.prefs.prefHasUserValue("app.update.postupdate"))
      this._showUpdateNotification();

    
    if (Services.prefs.prefHasUserValue(PREF_EM_NEW_ADDONS_LIST)) {
      var args = Cc["@mozilla.org/supports-array;1"].
                 createInstance(Ci.nsISupportsArray);
      var str = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      str.data = "";
      args.AppendElement(str);
      var str = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      str.data = Services.prefs.getCharPref(PREF_EM_NEW_ADDONS_LIST);
      args.AppendElement(str);
      const EMURL = "chrome://mozapps/content/extensions/extensions.xul";
      const EMFEATURES = "chrome,menubar,extra-chrome,toolbar,dialog=no,resizable";
      Services.ww.openWindow(null, EMURL, "_blank", EMFEATURES, args);
      Services.prefs.clearUserPref(PREF_EM_NEW_ADDONS_LIST);
    }

    
    
    
    
    
    if (this._isPlacesDatabaseLocked) {
      this._showPlacesLockedNotificationBox();
    }

    
    
    if (Services.prefs.getBoolPref(PREF_PLUGINS_NOTIFYUSER))
      this._showPluginUpdatePage();

#ifdef XP_WIN
#ifndef WINCE
    
    const WINTASKBAR_CONTRACTID = "@mozilla.org/windows-taskbar;1";
    if (WINTASKBAR_CONTRACTID in Cc &&
        Cc[WINTASKBAR_CONTRACTID].getService(Ci.nsIWinTaskbar).available) {
      let temp = {};
      Cu.import("resource://gre/modules/WindowsJumpLists.jsm", temp);
      temp.WinTaskbarJumpList.startup();
    }
#endif
#endif
  },

  _onQuitRequest: function BG__onQuitRequest(aCancelQuit, aQuitType) {
    
    if ((aCancelQuit instanceof Ci.nsISupportsPRBool) && aCancelQuit.data)
      return;

    var windowcount = 0;
    var pagecount = 0;
    var browserEnum = Services.wm.getEnumerator("navigator:browser");
    while (browserEnum.hasMoreElements()) {
      windowcount++;

      var browser = browserEnum.getNext();
      var tabbrowser = browser.document.getElementById("content");
      if (tabbrowser)
        pagecount += tabbrowser.browsers.length;
    }

    this._saveSession = false;
    if (pagecount < 2)
      return;

    if (aQuitType != "restart")
      aQuitType = "quit";

    var showPrompt = true;
    try {
      
      
      

      var sessionWillBeSaved = Services.prefs.getIntPref("browser.startup.page") == 3 ||
                               Services.prefs.getBoolPref("browser.sessionstore.resume_session_once");
      if (sessionWillBeSaved || !Services.prefs.getBoolPref("browser.warnOnQuit"))
        showPrompt = false;
      else if (aQuitType == "restart")
        showPrompt = Services.prefs.getBoolPref("browser.warnOnRestart");
      else
        showPrompt = Services.prefs.getBoolPref("browser.tabs.warnOnClose");
    } catch (ex) {}

    
    var inPrivateBrowsing = Cc["@mozilla.org/privatebrowsing;1"].
                            getService(Ci.nsIPrivateBrowsingService).
                            privateBrowsingEnabled;
    if (!showPrompt || inPrivateBrowsing)
      return false;

    var quitBundle = Services.strings.createBundle("chrome://browser/locale/quitDialog.properties");
    var brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");

    var appName = brandBundle.GetStringFromName("brandShortName");
    var quitDialogTitle = quitBundle.formatStringFromName(aQuitType + "DialogTitle",
                                                          [appName], 1);

    var message;
    if (aQuitType == "restart")
      message = quitBundle.formatStringFromName("messageRestart",
                                                [appName], 1);
    else if (windowcount == 1)
      message = quitBundle.formatStringFromName("messageNoWindows",
                                                [appName], 1);
    else
      message = quitBundle.formatStringFromName("message",
                                                [appName], 1);

    var promptService = Services.prompt;

    var flags = promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0 +
                promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_1 +
                promptService.BUTTON_POS_0_DEFAULT;

    var neverAsk = {value:false};
    var button0Title, button2Title;
    var button1Title = quitBundle.GetStringFromName("cancelTitle");
    var neverAskText = quitBundle.GetStringFromName("neverAsk");

    if (aQuitType == "restart")
      button0Title = quitBundle.GetStringFromName("restartTitle");
    else {
      flags += promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_2;
      button0Title = quitBundle.GetStringFromName("saveTitle");
      button2Title = quitBundle.GetStringFromName("quitTitle");
    }

    var mostRecentBrowserWindow = Services.wm.getMostRecentWindow("navigator:browser");
    var buttonChoice =
      promptService.confirmEx(mostRecentBrowserWindow, quitDialogTitle, message,
                              flags, button0Title, button1Title, button2Title,
                              neverAskText, neverAsk);

    switch (buttonChoice) {
    case 2: 
      if (neverAsk.value)
        Services.prefs.setBoolPref("browser.tabs.warnOnClose", false);
      break;
    case 1: 
      aCancelQuit.QueryInterface(Ci.nsISupportsPRBool);
      aCancelQuit.data = true;
      break;
    case 0: 
      this._saveSession = true;
      if (neverAsk.value) {
        if (aQuitType == "restart")
          Services.prefs.setBoolPref("browser.warnOnRestart", false);
        else {
          
          Services.prefs.setIntPref("browser.startup.page", 3);
        }
      }
      break;
    }
  },

  







  _shouldShowRights: function BG__shouldShowRights() {
    
    
    try {
      return !Services.prefs.getBoolPref("browser.rights.override");
    } catch (e) { }
    
    try {
      return !Services.prefs.getBoolPref("browser.EULA.override");
    } catch (e) { }

#ifndef OFFICIAL_BUILD
    
    return false;
#endif

    
    var currentVersion = Services.prefs.getIntPref("browser.rights.version");
    try {
      return !Services.prefs.getBoolPref("browser.rights." + currentVersion + ".shown");
    } catch (e) { }

    
    
    try {
      return !Services.prefs.getBoolPref("browser.EULA." + currentVersion + ".accepted");
    } catch (e) { }

    
    return true;
  },

  _showRightsNotification: function BG__showRightsNotification() {
    
    var win = this.getMostRecentBrowserWindow();
    var browser = win.gBrowser; 
    var notifyBox = browser.getNotificationBox();

    var brandBundle  = Services.strings.createBundle("chrome://branding/locale/brand.properties");
    var rightsBundle = Services.strings.createBundle("chrome://global/locale/aboutRights.properties");

    var buttonLabel      = rightsBundle.GetStringFromName("buttonLabel");
    var buttonAccessKey  = rightsBundle.GetStringFromName("buttonAccessKey");
    var productName      = brandBundle.GetStringFromName("brandFullName");
    var notifyRightsText = rightsBundle.formatStringFromName("notifyRightsText", [productName], 1);
    
    var buttons = [
                    {
                      label:     buttonLabel,
                      accessKey: buttonAccessKey,
                      popup:     null,
                      callback: function(aNotificationBar, aButton) {
                        browser.selectedTab = browser.addTab("about:rights");
                      }
                    }
                  ];

    
    var currentVersion = Services.prefs.getIntPref("browser.rights.version");
    Services.prefs.setBoolPref("browser.rights." + currentVersion + ".shown", true);

    var box = notifyBox.appendNotification(notifyRightsText, "about-rights", null, notifyBox.PRIORITY_INFO_LOW, buttons);
    box.persistence = 3; 
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
      let browser = win.gBrowser; 
      let notifyBox = browser.getNotificationBox();

      let buttons = [
                      {
                        label:     label,
                        accessKey: key,
                        popup:     null,
                        callback: function(aNotificationBar, aButton) {
                          browser.selectedTab = browser.addTab(url);
                        }
                      }
                    ];

      let box = notifyBox.appendNotification(text, "post-update-notification",
                                             null, notifyBox.PRIORITY_INFO_LOW,
                                             buttons);
      box.persistence = 3;
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
      let browser = win.gBrowser;
      browser.selectedTab = browser.addTab(data);
    }

    try {
      
      
      notifier.showAlertNotification("post-update-notification", title, text,
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
    var browser = win.gBrowser;
    browser.selectedTab = browser.addTab(updateUrl);
  },

  



















  _initPlaces: function BG__initPlaces() {
    
    
    
    var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);

    
    
    var databaseStatus = histsvc.databaseStatus;
    var importBookmarks = databaseStatus == histsvc.DATABASE_STATUS_CREATE ||
                          databaseStatus == histsvc.DATABASE_STATUS_CORRUPT;

    if (databaseStatus == histsvc.DATABASE_STATUS_CREATE) {
      
      
      
      
      var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                  getService(Ci.nsINavBookmarksService);
      if (bmsvc.getIdForItemAt(bmsvc.bookmarksMenuFolder, 0) != -1 ||
          bmsvc.getIdForItemAt(bmsvc.toolbarFolder, 0) != -1)
        importBookmarks = false;
    }

    
    var importBookmarksHTML = false;
    try {
      importBookmarksHTML =
        Services.prefs.getBoolPref("browser.places.importBookmarksHTML");
      if (importBookmarksHTML)
        importBookmarks = true;
    } catch(ex) {}

    
    
    var restoreDefaultBookmarks = false;
    try {
      restoreDefaultBookmarks =
        Services.prefs.getBoolPref("browser.bookmarks.restore_default_bookmarks");
      if (restoreDefaultBookmarks) {
        
        this._backupBookmarks();
        importBookmarks = true;
      }
    } catch(ex) {}

    
    
    if (importBookmarks && !restoreDefaultBookmarks && !importBookmarksHTML) {
      
      Cu.import("resource://gre/modules/PlacesUtils.jsm");
      var bookmarksBackupFile = PlacesUtils.backups.getMostRecent("json");
      if (bookmarksBackupFile) {
        
        PlacesUtils.restoreBookmarksFromJSONFile(bookmarksBackupFile);
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

      var bookmarksFile = null;
      if (restoreDefaultBookmarks) {
        
        bookmarksFile = dirService.get("profDef", Ci.nsILocalFile);
        bookmarksFile.append("bookmarks.html");
      }
      else
        bookmarksFile = dirService.get("BMarks", Ci.nsILocalFile);

      if (bookmarksFile.exists()) {
        
        
        Services.obs.addObserver(this, "bookmarks-restore-success", false);
        Services.obs.addObserver(this, "bookmarks-restore-failed", false);

        
        try {
          var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].
                         getService(Ci.nsIPlacesImportExportService);
          importer.importHTMLFromFile(bookmarksFile, true );
        } catch (err) {
          
          Cu.reportError("Bookmarks.html file could be corrupt. " + err);
          Services.obs.removeObserver(this, "bookmarks-restore-success");
          Services.obs.removeObserver(this, "bookmarks-restore-failed");
        }
      }
      else
        Cu.reportError("Unable to find bookmarks.html file.");

      
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
  },

  







  _shutdownPlaces: function BG__shutdownPlaces() {
    if (this._isIdleObserver) {
      this._idleService.removeIdleObserver(this, BOOKMARKS_BACKUP_IDLE_TIME);
      this._isIdleObserver = false;
    }
    this._backupBookmarks();

    
    
    var autoExportHTML = false;
    try {
      autoExportHTML = Services.prefs.getBoolPref("browser.bookmarks.autoExportHTML");
    } catch(ex) {  }

    if (autoExportHTML) {
      Cc["@mozilla.org/browser/places/import-export-service;1"].
        getService(Ci.nsIPlacesImportExportService).
        backupBookmarksFile();
    }
  },

  


  _backupBookmarks: function BG__backupBookmarks() {
    Cu.import("resource://gre/modules/PlacesUtils.jsm");

    let lastBackupFile = PlacesUtils.backups.getMostRecent();

    
    
    if (!lastBackupFile ||
        new Date() - PlacesUtils.backups.getDateForFile(lastBackupFile) > BOOKMARKS_BACKUP_INTERVAL) {
      let maxBackups = BOOKMARKS_BACKUP_MAX_BACKUPS;
      try {
        maxBackups = Services.prefs.getIntPref("browser.bookmarks.max_backups");
      }
      catch(ex) {  }

      PlacesUtils.backups.create(maxBackups); 
    }
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

    var browser = this.getMostRecentBrowserWindow().gBrowser;

    var buttons = [
                    {
                      label:     buttonText,
                      accessKey: accessKey,
                      popup:     null,
                      callback:  function(aNotificationBar, aButton) {
                        browser.selectedTab = browser.addTab(url);
                      }
                    }
                  ];

    var notifyBox = browser.getNotificationBox();
    var box = notifyBox.appendNotification(text, title, null,
                                           notifyBox.PRIORITY_CRITICAL_MEDIUM,
                                           buttons);
    box.persistence = -1; 
  },

  _migrateUI: function BG__migrateUI() {
    var migration = 0;
    try {
      migration = Services.prefs.getIntPref("browser.migration.version");
    } catch(ex) {}

    if (migration == 0) {
      

      
      this._rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
      this._dataSource = this._rdf.GetDataSource("rdf:local-store");
      this._dirty = false;

      let currentsetResource = this._rdf.GetResource("currentset");
      let toolbars = ["nav-bar", "toolbar-menubar", "PersonalToolbar"];
      for (let i = 0; i < toolbars.length; i++) {
        let toolbar = this._rdf.GetResource("chrome://browser/content/browser.xul#" + toolbars[i]);
        let currentset = this._getPersist(toolbar, currentsetResource);
        if (!currentset) {
          
          if (i == 0)
            
            break;
          continue;
        }
        if (/(?:^|,)unified-back-forward-button(?:$|,)/.test(currentset))
          
          break;
        if (/(?:^|,)back-button(?:$|,)/.test(currentset)) {
          let newset = currentset.replace(/(^|,)back-button($|,)/,
                                          "$1unified-back-forward-button,back-button$2")
          this._setPersist(toolbar, currentsetResource, newset);
          
          break;
        }
      }

      
      if (this._dirty)
        this._dataSource.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();

      
      this._rdf = null;
      this._dataSource = null;

      
      Services.prefs.setIntPref("browser.migration.version", 1);
    }
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
    }
    catch(ex) {}
  },

  
  
  
  
  sanitize: function BG_sanitize(aParentWindow) {
    this._sanitizer.sanitize(aParentWindow);
  },

  ensurePlacesDefaultQueriesInitialized:
  function BG_ensurePlacesDefaultQueriesInitialized() {
    
    
    
    
    
    
    const SMART_BOOKMARKS_VERSION = 2;
    const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";
    const SMART_BOOKMARKS_PREF = "browser.places.smartBookmarksVersion";

    
    const MAX_RESULTS = 10;

    
    
    var smartBookmarksCurrentVersion = 0;
    try {
      smartBookmarksCurrentVersion = Services.prefs.getIntPref(SMART_BOOKMARKS_PREF);
    } catch(ex) {  }

    
    if (smartBookmarksCurrentVersion == -1 ||
        smartBookmarksCurrentVersion >= SMART_BOOKMARKS_VERSION)
      return;

    var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                getService(Ci.nsINavBookmarksService);
    var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].
                  getService(Ci.nsIAnnotationService);

    var callback = {
      _uri: function BG_EPDQI__uri(aSpec) {
        return Services.io.newURI(aSpec, null, null);
      },

      runBatched: function BG_EPDQI_runBatched() {
        var smartBookmarks = [];
        var bookmarksMenuIndex = 0;
        var bookmarksToolbarIndex = 0;

        var placesBundle = Services.strings.createBundle("chrome://browser/locale/places/places.properties");

        
        var smart = {queryId: "MostVisited", 
                     itemId: null,
                     title: placesBundle.GetStringFromName("mostVisitedTitle"),
                     uri: this._uri("place:redirectsMode=" +
                                    Ci.nsINavHistoryQueryOptions.REDIRECTS_MODE_TARGET +
                                    "&sort=" +
                                    Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING +
                                    "&maxResults=" + MAX_RESULTS),
                     parent: bmsvc.toolbarFolder,
                     position: bookmarksToolbarIndex++,
                     newInVersion: 1 };
        smartBookmarks.push(smart);

        
        smart = {queryId: "RecentlyBookmarked", 
                 itemId: null,
                 title: placesBundle.GetStringFromName("recentlyBookmarkedTitle"),
                 uri: this._uri("place:folder=BOOKMARKS_MENU" +
                                "&folder=UNFILED_BOOKMARKS" +
                                "&folder=TOOLBAR" +
                                "&queryType=" +
                                Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS +
                                "&sort=" +
                                Ci.nsINavHistoryQueryOptions.SORT_BY_DATEADDED_DESCENDING +
                                "&excludeItemIfParentHasAnnotation=livemark%2FfeedURI" +
                                "&maxResults=" + MAX_RESULTS +
                                "&excludeQueries=1"),
                 parent: bmsvc.bookmarksMenuFolder,
                 position: bookmarksMenuIndex++,
                 newInVersion: 1 };
        smartBookmarks.push(smart);

        
        smart = {queryId: "RecentTags", 
                 itemId: null,
                 title: placesBundle.GetStringFromName("recentTagsTitle"),
                 uri: this._uri("place:"+
                    "type=" +
                    Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY +
                    "&sort=" +
                    Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_DESCENDING +
                    "&maxResults=" + MAX_RESULTS),
                 parent: bmsvc.bookmarksMenuFolder,
                 position: bookmarksMenuIndex++,
                 newInVersion: 1 };
        smartBookmarks.push(smart);

        var smartBookmarkItemIds = annosvc.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO);
        
        
        
        for each(var itemId in smartBookmarkItemIds) {
          var queryId = annosvc.getItemAnnotation(itemId, SMART_BOOKMARKS_ANNO);
          for (var i = 0; i < smartBookmarks.length; i++){
            if (smartBookmarks[i].queryId == queryId) {
              smartBookmarks[i].found = true;
              smartBookmarks[i].itemId = itemId;
              smartBookmarks[i].parent = bmsvc.getFolderIdForItem(itemId);
              smartBookmarks[i].position = bmsvc.getItemIndex(itemId);
              
              bmsvc.removeItem(itemId);
              break;
            }
            
            
            
            if (i == smartBookmarks.length - 1)
              annosvc.removeItemAnnotation(itemId, SMART_BOOKMARKS_ANNO);
          }
        }

        
        for each(var smartBookmark in smartBookmarks) {
          
          
          
          if (smartBookmarksCurrentVersion > 0 &&
              smartBookmark.newInVersion <= smartBookmarksCurrentVersion &&
              !smartBookmark.found)
            continue;

          smartBookmark.itemId = bmsvc.insertBookmark(smartBookmark.parent,
                                                      smartBookmark.uri,
                                                      smartBookmark.position,
                                                      smartBookmark.title);
          annosvc.setItemAnnotation(smartBookmark.itemId,
                                    SMART_BOOKMARKS_ANNO, smartBookmark.queryId,
                                    0, annosvc.EXPIRE_NEVER);
        }
        
        
        
        if (smartBookmarksCurrentVersion == 0 &&
            smartBookmarkItemIds.length == 0) {
          let id = bmsvc.getIdForItemAt(bmsvc.bookmarksMenuFolder,
                                        bookmarksMenuIndex);
          
          if (id != -1 && bmsvc.getItemType(id) != bmsvc.TYPE_SEPARATOR)
            bmsvc.insertSeparator(bmsvc.bookmarksMenuFolder, bookmarksMenuIndex);
        }
      }
    };

    try {
      bmsvc.runInBatchMode(callback, null);
    }
    catch(ex) {
      Components.utils.reportError(ex);
    }
    finally {
      Services.prefs.setIntPref(SMART_BOOKMARKS_PREF, SMART_BOOKMARKS_VERSION);
      Services.prefs.savePrefFile(null);
    }
  },

#ifndef XP_WIN
#define BROKEN_WM_Z_ORDER
#endif

  
  getMostRecentBrowserWindow: function BG_getMostRecentBrowserWindow() {
    function isFullBrowserWindow(win) {
      return !win.closed &&
             !win.document.documentElement.getAttribute("chromehidden");
    }

#ifdef BROKEN_WM_Z_ORDER
    var win = Services.wm.getMostRecentWindow("navigator:browser");

    
    if (win && !isFullBrowserWindow(win)) {
      win = null;
      let windowList = Services.wm.getEnumerator("navigator:browser");
      
      while (windowList.hasMoreElements()) {
        let nextWin = windowList.getNext();
        if (isFullBrowserWindow(nextWin))
          win = nextWin;
      }
    }
    return win;
#else
    var windowList = Services.wm.getZOrderDOMWindowEnumerator("navigator:browser", true);
    while (windowList.hasMoreElements()) {
      let win = windowList.getNext();
      if (isFullBrowserWindow(win))
        return win;
    }
    return null;
#endif
  },


  
  classDescription: "Firefox Browser Glue Service",
  classID:          Components.ID("{eab9012e-5f74-4cbc-b2b5-a590235513cc}"),
  contractID:       "@mozilla.org/browser/browserglue;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsIBrowserGlue]),

  
  _xpcom_factory: BrowserGlueServiceFactory,

  
  _xpcom_categories: [
    
    { category: "app-startup", service: true }
  ]
}

function GeolocationPrompt() {}

GeolocationPrompt.prototype = {
  classDescription: "Geolocation Prompting Component",
  classID:          Components.ID("{C6E8C44D-9F39-4AF7-BCC0-76E38A8310F5}"),
  contractID:       "@mozilla.org/geolocation/prompt;1",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIGeolocationPrompt]),
 
  prompt: function GP_prompt(request) {
    var result = Services.perms.testExactPermission(request.requestingURI, "geo");

    if (result == Ci.nsIPermissionManager.ALLOW_ACTION) {
      request.allow();
      return;
    }
    
    if (result == Ci.nsIPermissionManager.DENY_ACTION) {
      request.cancel();
      return;
    }

    function setPagePermission(uri, allow) {
      if (allow == true)
        Services.perms.add(uri, "geo", Ci.nsIPermissionManager.ALLOW_ACTION);
      else
        Services.perms.add(uri, "geo", Ci.nsIPermissionManager.DENY_ACTION);
    }

    function getChromeWindow(aWindow) {
      var chromeWin = aWindow 
        .QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIWebNavigation)
        .QueryInterface(Ci.nsIDocShellTreeItem)
        .rootTreeItem
        .QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindow)
        .QueryInterface(Ci.nsIDOMChromeWindow);
      return chromeWin;
    }

    var requestingWindow = request.requestingWindow.top;
    var chromeWindowObject = getChromeWindow(requestingWindow).wrappedJSObject;
    var tabbrowser = chromeWindowObject.gBrowser;
    var browser = tabbrowser.getBrowserForDocument(requestingWindow.document);
    var notificationBox = tabbrowser.getNotificationBox(browser);

    var notification = notificationBox.getNotificationWithValue("geolocation");
    if (!notification) {
      var browserBundle = Services.strings.createBundle("chrome:

      var buttons = [{
              label: browserBundle.GetStringFromName("geolocation.shareLocation"),
              accessKey: browserBundle.GetStringFromName("geolocation.shareLocation.accesskey"),
              callback: function(notification) {
                  var elements = notification.getElementsByClassName("rememberChoice");
                  if (elements.length && elements[0].checked)
                      setPagePermission(request.requestingURI, true);
                  request.allow(); 
              },
          },
          {
              label: browserBundle.GetStringFromName("geolocation.dontShareLocation"),
              accessKey: browserBundle.GetStringFromName("geolocation.dontShareLocation.accesskey"),
              callback: function(notification) {
                  var elements = notification.getElementsByClassName("rememberChoice");
                  if (elements.length && elements[0].checked)
                      setPagePermission(request.requestingURI, false);
                  request.cancel();
              },
          }];
      
      var message;

      
      if (request.requestingURI.schemeIs("file")) {
        message = browserBundle.formatStringFromName("geolocation.fileWantsToKnow",
                                                     [request.requestingURI.path], 1);
      } else {
        message = browserBundle.formatStringFromName("geolocation.siteWantsToKnow",
                                                     [request.requestingURI.host], 1);
      }

      var newBar = notificationBox.appendNotification(message,
                                                      "geolocation",
                                                      "chrome:
                                                      notificationBox.PRIORITY_INFO_HIGH,
                                                      buttons);

      
      
      
      
      function geolocation_hacks_to_notification () {

        
        var inPrivateBrowsing = Cc["@mozilla.org/privatebrowsing;1"].
                                getService(Ci.nsIPrivateBrowsingService).
                                privateBrowsingEnabled;

        
        var host;
        try {
            host = request.requestingURI.host;
        } catch (ex) {}

        if (!inPrivateBrowsing && host) {
          var checkbox = newBar.ownerDocument.createElementNS(XULNS, "checkbox");
          checkbox.className = "rememberChoice";
          checkbox.setAttribute("label", browserBundle.GetStringFromName("geolocation.remember"));
          checkbox.setAttribute("accesskey", browserBundle.GetStringFromName("geolocation.remember.accesskey"));
          newBar.appendChild(checkbox);
        }

        var link = newBar.ownerDocument.createElementNS(XULNS, "label");
        link.className = "text-link";
        link.setAttribute("value", browserBundle.GetStringFromName("geolocation.learnMore"));

        var formatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"].getService(Ci.nsIURLFormatter);
        link.href = formatter.formatURLPref("browser.geolocation.warning.infoURL");

        var description = newBar.ownerDocument.getAnonymousElementByAttribute(newBar, "anonid", "messageText");
        description.appendChild(link);
      };

      chromeWindowObject.setTimeout(geolocation_hacks_to_notification, 0);

    }
  },
};



function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([BrowserGlue, GeolocationPrompt]);
}
