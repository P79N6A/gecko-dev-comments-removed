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
# The Original Code is Private Browsing.
#
# The Initial Developer of the Original Code is
# Ehsan Akhgari.
# Portions created by the Initial Developer are Copyright (C) 2008
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#  Ehsan Akhgari <ehsan.akhgari@gmail.com> (Original Author)
#  Simon BÃ¼nzli <zeniko@gmail.com>
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

#ifndef XP_WIN
#define BROKEN_WM_Z_ORDER
#endif










String.prototype.hasRootDomain = function hasRootDomain(aDomain)
{
  let index = this.indexOf(aDomain);
  
  if (index == -1)
    return false;

  
  if (this == aDomain)
    return true;

  
  
  
  let prevChar = this[index - 1];
  return (index == (this.length - aDomain.length)) &&
         (prevChar == "." || prevChar == "/");
}




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const STATE_IDLE = 0;
const STATE_TRANSITION_STARTED = 1;
const STATE_WAITING_FOR_RESTORE = 2;
const STATE_RESTORE_FINISHED = 3;




function PrivateBrowsingService() {
  this._obs = Cc["@mozilla.org/observer-service;1"].
              getService(Ci.nsIObserverService);
  this._obs.addObserver(this, "profile-after-change", true);
  this._obs.addObserver(this, "quit-application-granted", true);
  this._obs.addObserver(this, "private-browsing", true);
  this._obs.addObserver(this, "command-line-startup", true);
  this._obs.addObserver(this, "sessionstore-browser-state-restored", true);

  
  this._windowsToClose = [];
}

PrivateBrowsingService.prototype = {
  
  get _prefs() {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    this.__defineGetter__("_prefs", function() prefs);
    return this._prefs;
  },

  
  _inPrivateBrowsing: false,

  
  _savedBrowserState: null,

  
  _quitting: false,

  
  _saveSession: true,

  
  _currentStatus: STATE_IDLE,

  
  _autoStarted: false,

  
  _viewSrcURLs: [],

  
  _lastChangedByCommandLine: false,

  
  classID: Components.ID("{c31f4883-839b-45f6-82ad-a6a9bc5ad599}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrivateBrowsingService, 
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsICommandLineHandler]),

  _unload: function PBS__destroy() {
    
    this._quitting = true;
    if (this._inPrivateBrowsing)
      this.privateBrowsingEnabled = false;
  },

  _onBeforePrivateBrowsingModeChange: function PBS__onBeforePrivateBrowsingModeChange() {
    
    if (!this._autoStarted) {
      let ss = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);
      let blankState = JSON.stringify({
        "windows": [{
          "tabs": [{
            "entries": [{
              "url": "about:blank"
            }]
          }],
          "_closedTabs": []
        }]
      });

      if (this._inPrivateBrowsing) {
        
        if (this._saveSession && !this._savedBrowserState) {
          if (this._getBrowserWindow())
            this._savedBrowserState = ss.getBrowserState();
          else 
            this._savedBrowserState = blankState;
        }
      }

      this._closePageInfoWindows();

      
      let viewSrcWindowsEnum = Cc["@mozilla.org/appshell/window-mediator;1"].
                               getService(Ci.nsIWindowMediator).
                               getEnumerator("navigator:view-source");
      while (viewSrcWindowsEnum.hasMoreElements()) {
        let win = viewSrcWindowsEnum.getNext();
        if (this._inPrivateBrowsing) {
          let plainURL = win.gBrowser.currentURI.spec;
          if (plainURL.indexOf("view-source:") == 0) {
            plainURL = plainURL.substr(12);
            this._viewSrcURLs.push(plainURL);
          }
        }
        win.close();
      }

      if (!this._quitting && this._saveSession) {
        let browserWindow = this._getBrowserWindow();

        
        
        if (browserWindow) {
          
          ss.setBrowserState(blankState);

          
          
          
          browserWindow = this._getBrowserWindow();
          let browser = browserWindow.gBrowser;

          
          
          browser.addTab();
          browser.getBrowserForTab(browser.tabContainer.firstChild).stop();
          browser.removeTab(browser.tabContainer.firstChild);
          browserWindow.getInterface(Ci.nsIWebNavigation)
                       .QueryInterface(Ci.nsIDocShellTreeItem)
                       .treeOwner
                       .QueryInterface(Ci.nsIInterfaceRequestor)
                       .getInterface(Ci.nsIXULWindow)
                       .docShell.contentViewer.resetCloseWindow();
        }
      }
    }
    else
      this._saveSession = false;
  },

  _onAfterPrivateBrowsingModeChange: function PBS__onAfterPrivateBrowsingModeChange() {
    
    
    if (!this._autoStarted && this._saveSession) {
      let ss = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);
      
      
      if (!this._inPrivateBrowsing) {
        this._currentStatus = STATE_WAITING_FOR_RESTORE;
        if (!this._getBrowserWindow()) {
          ss.init(null);
        }
        ss.setBrowserState(this._savedBrowserState);
        this._savedBrowserState = null;

        this._closePageInfoWindows();

        
        let windowWatcher = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                            getService(Ci.nsIWindowWatcher);
        this._viewSrcURLs.forEach(function(uri) {
          let args = Cc["@mozilla.org/supports-array;1"].
                     createInstance(Ci.nsISupportsArray);
          let str = Cc["@mozilla.org/supports-string;1"].
                    createInstance(Ci.nsISupportsString);
          str.data = uri;
          args.AppendElement(str);
          args.AppendElement(null); 
          args.AppendElement(null); 
          args.AppendElement(null); 
          let forcedCharset = Cc["@mozilla.org/supports-PRBool;1"].
                              createInstance(Ci.nsISupportsPRBool);
          forcedCharset.data = false;
          args.AppendElement(forcedCharset);
          windowWatcher.openWindow(null, "chrome://global/content/viewSource.xul",
            "_blank", "all,dialog=no", args);
        });
        this._viewSrcURLs = [];
      }
      else {
        
        
        let privateBrowsingState = {
          "windows": [{
            "tabs": [{
              "entries": [{
                "url": "about:privatebrowsing"
              }]
            }],
            "_closedTabs": []
          }]
        };
        
        this._currentStatus = STATE_WAITING_FOR_RESTORE;
        if (!this._getBrowserWindow()) {
          ss.init(null);
        }
        ss.setBrowserState(JSON.stringify(privateBrowsingState));
      }
    }
  },

  _notifyIfTransitionComplete: function PBS__notifyIfTransitionComplete() {
    switch (this._currentStatus) {
      case STATE_TRANSITION_STARTED:
        
      case STATE_RESTORE_FINISHED:
        
        this._currentStatus = STATE_IDLE;
        this._obs.notifyObservers(null, "private-browsing-transition-complete", "");
        break;
      case STATE_WAITING_FOR_RESTORE:
        
        break;
      case STATE_IDLE:
        
        break;
      default:
        
        Cu.reportError("Unexpected private browsing status reached: " +
                       this._currentStatus);
        break;
    }
  },

  _canEnterPrivateBrowsingMode: function PBS__canEnterPrivateBrowsingMode() {
    let cancelEnter = Cc["@mozilla.org/supports-PRBool;1"].
                      createInstance(Ci.nsISupportsPRBool);
    cancelEnter.data = false;
    this._obs.notifyObservers(cancelEnter, "private-browsing-cancel-vote", "enter");
    return !cancelEnter.data;
  },

  _canLeavePrivateBrowsingMode: function PBS__canLeavePrivateBrowsingMode() {
    let cancelLeave = Cc["@mozilla.org/supports-PRBool;1"].
                      createInstance(Ci.nsISupportsPRBool);
    cancelLeave.data = false;
    this._obs.notifyObservers(cancelLeave, "private-browsing-cancel-vote", "exit");
    return !cancelLeave.data;
  },

  _getBrowserWindow: function PBS__getBrowserWindow() {
    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);

    var win = wm.getMostRecentWindow("navigator:browser");

    

    if (!win)
      return null;
    if (!win.closed)
      return win;

#ifdef BROKEN_WM_Z_ORDER
    win = null;
    var windowsEnum = wm.getEnumerator("navigator:browser");
    
    while (windowsEnum.hasMoreElements()) {
      let nextWin = windowsEnum.getNext();
      if (!nextWin.closed)
        win = nextWin;
    }
    return win;
#else
    var windowsEnum = wm.getZOrderDOMWindowEnumerator("navigator:browser", true);
    while (windowsEnum.hasMoreElements()) {
      win = windowsEnum.getNext();
      if (!win.closed)
        return win;
    }
    return null;
#endif
  },

  _ensureCanCloseWindows: function PBS__ensureCanCloseWindows() {
    
    this._saveSession = true;
    try {
      if (this._prefs.getBoolPref("browser.privatebrowsing.keep_current_session")) {
        this._saveSession = false;
        return;
      }
    } catch (ex) {}

    let windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
                         getService(Ci.nsIWindowMediator);
    let windowsEnum = windowMediator.getEnumerator("navigator:browser");

    while (windowsEnum.hasMoreElements()) {
      let win = windowsEnum.getNext();
      if (win.closed)
        continue;
      let xulWin = win.QueryInterface(Ci.nsIInterfaceRequestor).
                   getInterface(Ci.nsIWebNavigation).
                   QueryInterface(Ci.nsIDocShellTreeItem).
                   treeOwner.QueryInterface(Ci.nsIInterfaceRequestor).
                   getInterface(Ci.nsIXULWindow);
      if (xulWin.docShell.contentViewer.permitUnload(true))
        this._windowsToClose.push(xulWin);
      else
        throw Cr.NS_ERROR_ABORT;
    }
  },

  _closePageInfoWindows: function PBS__closePageInfoWindows() {
    let pageInfoEnum = Cc["@mozilla.org/appshell/window-mediator;1"].
                       getService(Ci.nsIWindowMediator).
                       getEnumerator("Browser:page-info");
    while (pageInfoEnum.hasMoreElements()) {
      let win = pageInfoEnum.getNext();
      win.close();
    }
  },

  

  observe: function PBS_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "profile-after-change":
        
        
        
        
        if (!this._autoStarted) {
          this._autoStarted = this._prefs.getBoolPref("browser.privatebrowsing.autostart");
          if (this._autoStarted)
            this.privateBrowsingEnabled = true;
        }
        this._obs.removeObserver(this, "profile-after-change");
        break;
      case "quit-application-granted":
        this._unload();
        break;
      case "private-browsing":
        
        let sdr = Cc["@mozilla.org/security/sdr;1"].
                  getService(Ci.nsISecretDecoderRing);
        sdr.logoutAndTeardown();
    
        
        let authMgr = Cc['@mozilla.org/network/http-auth-manager;1'].
                      getService(Ci.nsIHttpAuthManager);
        authMgr.clearAll();

        try {
          this._prefs.deleteBranch("geo.wifi.access_token.");
        } catch (ex) {}

        if (!this._inPrivateBrowsing) {
          
          let consoleService = Cc["@mozilla.org/consoleservice;1"].
                               getService(Ci.nsIConsoleService);
          consoleService.logStringMessage(null); 
          consoleService.reset();
        }
        break;
      case "command-line-startup":
        this._obs.removeObserver(this, "command-line-startup");
        aSubject.QueryInterface(Ci.nsICommandLine);
        if (aSubject.findFlag("private", false) >= 0) {
          this.privateBrowsingEnabled = true;
          this._autoStarted = true;
          this._lastChangedByCommandLine = true;
        }
        else if (aSubject.findFlag("private-toggle", false) >= 0) {
          this._lastChangedByCommandLine = true;
        }
        break;
      case "sessionstore-browser-state-restored":
        if (this._currentStatus == STATE_WAITING_FOR_RESTORE) {
          this._currentStatus = STATE_RESTORE_FINISHED;
          this._notifyIfTransitionComplete();
        }
        break;
    }
  },

  

  handle: function PBS_handle(aCmdLine) {
    if (aCmdLine.handleFlag("private", false))
      aCmdLine.preventDefault = true; 
    else if (aCmdLine.handleFlag("private-toggle", false)) {
      if (this._autoStarted) {
        throw Cr.NS_ERROR_ABORT;
      }
      this.privateBrowsingEnabled = !this.privateBrowsingEnabled;
      this._lastChangedByCommandLine = true;
    }
  },

  get helpInfo() {
    return "  -private           Enable private browsing mode.\n" +
           "  -private-toggle    Toggle private browsing mode.\n";
  },

  

  


  get privateBrowsingEnabled() {
    return this._inPrivateBrowsing;
  },

  


  set privateBrowsingEnabled(val) {
    
    
    
    
    
    if (this._currentStatus != STATE_IDLE)
      throw Cr.NS_ERROR_FAILURE;

    try {
      this._currentStatus = STATE_TRANSITION_STARTED;

      if (val != this._inPrivateBrowsing) {
        if (val) {
          if (!this._canEnterPrivateBrowsingMode())
            return;
        }
        else {
          if (!this._canLeavePrivateBrowsingMode())
            return;
        }

        this._ensureCanCloseWindows();

        this._autoStarted = this._prefs.getBoolPref("browser.privatebrowsing.autostart");
        this._inPrivateBrowsing = val != false;

        let data = val ? "enter" : "exit";

        let quitting = Cc["@mozilla.org/supports-PRBool;1"].
                       createInstance(Ci.nsISupportsPRBool);
        quitting.data = this._quitting;

        
        this._obs.notifyObservers(quitting, "private-browsing-change-granted", data);

        
        this._onBeforePrivateBrowsingModeChange();

        this._obs.notifyObservers(quitting, "private-browsing", data);

        
        this._onAfterPrivateBrowsingModeChange();
      }
    } catch (ex) {
      
      
      for (let i = 0; i < this._windowsToClose.length; i++)
        this._windowsToClose[i].docShell.contentViewer.resetCloseWindow();
      
      if (ex != Cr.NS_ERROR_ABORT)
        Cu.reportError("Exception thrown while processing the " +
          "private browsing mode change request: " + ex.toString());
    } finally {
      this._windowsToClose = [];
      this._notifyIfTransitionComplete();
      this._lastChangedByCommandLine = false;
    }
  },

  


  get autoStarted() {
    return this._inPrivateBrowsing && this._autoStarted;
  },

  


  get lastChangedByCommandLine() {
    return this._lastChangedByCommandLine;
  },

  removeDataFromDomain: function PBS_removeDataFromDomain(aDomain)
  {

    
    try {
        this._prefs.deleteBranch("geo.wifi.access_token.");
    } catch (e) {}
    
    
    let (bh = Cc["@mozilla.org/browser/global-history;2"].
              getService(Ci.nsIBrowserHistory)) {
      bh.removePagesFromHost(aDomain, true);
    }

    
    let (cs = Cc["@mozilla.org/network/cache-service;1"].
              getService(Ci.nsICacheService)) {
      
      
      try {
        cs.evictEntries(Ci.nsICache.STORE_ANYWHERE);
      } catch (ex) {
        Cu.reportError("Exception thrown while clearing the cache: " +
          ex.toString());
      }
    }

    
    let (imageCache = Cc["@mozilla.org/image/cache;1"].
                      getService(Ci.imgICache)) {
      try {
        imageCache.clearCache(false); 
      } catch (ex) {
        Cu.reportError("Exception thrown while clearing the image cache: " +
          ex.toString());
      }
    }

    
    let (cm = Cc["@mozilla.org/cookiemanager;1"].
              getService(Ci.nsICookieManager2)) {
      let enumerator = cm.getCookiesFromHost(aDomain);
      while (enumerator.hasMoreElements()) {
        let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie);
        cm.remove(cookie.host, cookie.name, cookie.path, false);
      }
    }

    
    let (ph = Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost)) {
      const phInterface = Ci.nsIPluginHost_MOZILLA_2_0_BRANCH;
      const FLAG_CLEAR_ALL = phInterface.FLAG_CLEAR_ALL;
      ph.QueryInterface(phInterface);

      let tags = ph.getPluginTags();
      for (let i = 0; i < tags.length; i++) {
        try {
          ph.clearSiteData(tags[i], aDomain, FLAG_CLEAR_ALL, -1);
        } catch (e) {
          
        }
      }
    }

    
    let (dm = Cc["@mozilla.org/download-manager;1"].
              getService(Ci.nsIDownloadManager)) {
      
      let enumerator = dm.activeDownloads;
      while (enumerator.hasMoreElements()) {
        let dl = enumerator.getNext().QueryInterface(Ci.nsIDownload);
        if (dl.source.host.hasRootDomain(aDomain)) {
          dm.cancelDownload(dl.id);
          dm.removeDownload(dl.id);
        }
      }

      
      let db = dm.DBConnection;
      
      
      
      let stmt = db.createStatement(
        "DELETE FROM moz_downloads " +
        "WHERE source LIKE ?1 ESCAPE '/' " +
        "AND state NOT IN (?2, ?3, ?4)"
      );
      let pattern = stmt.escapeStringForLIKE(aDomain, "/");
      stmt.bindStringParameter(0, "%" + pattern + "%");
      stmt.bindInt32Parameter(1, Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING);
      stmt.bindInt32Parameter(2, Ci.nsIDownloadManager.DOWNLOAD_PAUSED);
      stmt.bindInt32Parameter(3, Ci.nsIDownloadManager.DOWNLOAD_QUEUED);
      try {
        stmt.execute();
      }
      finally {
        stmt.finalize();
      }

      
      
      let os = Cc["@mozilla.org/observer-service;1"].
               getService(Ci.nsIObserverService);
      os.notifyObservers(null, "download-manager-remove-download", null);
    }

    
    let (lm = Cc["@mozilla.org/login-manager;1"].
              getService(Ci.nsILoginManager)) {
      
      try {
        let logins = lm.getAllLogins();
        for (let i = 0; i < logins.length; i++)
          if (logins[i].hostname.hasRootDomain(aDomain))
            lm.removeLogin(logins[i]);
      }
      
      
      catch (ex if ex.message.indexOf("User canceled Master Password entry") != -1) { }

      
      let disabledHosts = lm.getAllDisabledHosts();
      for (let i = 0; i < disabledHosts.length; i++)
        if (disabledHosts[i].hasRootDomain(aDomain))
          lm.setLoginSavingEnabled(disabledHosts, true);
    }

    
    let (pm = Cc["@mozilla.org/permissionmanager;1"].
              getService(Ci.nsIPermissionManager)) {
      
      let enumerator = pm.enumerator;
      while (enumerator.hasMoreElements()) {
        let perm = enumerator.getNext().QueryInterface(Ci.nsIPermission);
        if (perm.host.hasRootDomain(aDomain))
          pm.remove(perm.host, perm.type);
      }
    }

    
    let (cp = Cc["@mozilla.org/content-pref/service;1"].
              getService(Ci.nsIContentPrefService)) {
      let db = cp.DBConnection;
      
      let names = [];
      let stmt = db.createStatement(
        "SELECT name " +
        "FROM groups " +
        "WHERE name LIKE ?1 ESCAPE '/'"
      );
      let pattern = stmt.escapeStringForLIKE(aDomain, "/");
      stmt.bindStringParameter(0, "%" + pattern);
      try {
        while (stmt.executeStep())
          if (stmt.getString(0).hasRootDomain(aDomain))
            names.push(stmt.getString(0));
      }
      finally {
        stmt.finalize();
      }

      
      for (let i = 0; i < names.length; i++) {
        let uri = names[i];
        let enumerator = cp.getPrefs(uri).enumerator;
        while (enumerator.hasMoreElements()) {
          let pref = enumerator.getNext().QueryInterface(Ci.nsIProperty);
          cp.removePref(uri, pref.name);
        }
      }
    }

    
    let (idbm = Cc["@mozilla.org/dom/indexeddb/manager;1"].
                getService(Ci.nsIIndexedDatabaseManager)) {
      
      let caUtils = {};
      let scriptLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].
                         getService(Ci.mozIJSSubScriptLoader);
      scriptLoader.loadSubScript("chrome://global/content/contentAreaUtils.js",
                                 caUtils);
      let httpURI = caUtils.makeURI("http://" + aDomain);
      let httpsURI = caUtils.makeURI("https://" + aDomain);
      idbm.clearDatabasesForURI(httpURI);
      idbm.clearDatabasesForURI(httpsURI);
    }

    
    this._obs.notifyObservers(null, "browser:purge-domain-data", aDomain);
  }
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([PrivateBrowsingService]);
