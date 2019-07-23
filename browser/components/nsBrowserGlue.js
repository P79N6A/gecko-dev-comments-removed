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

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/distribution.js");

const PREF_EM_NEW_ADDONS_LIST = "extensions.newAddons";



const BOOKMARKS_ARCHIVE_IDLE_TIME = 60 * 60;


const BOOKMARKS_ARCHIVE_INTERVAL = 86400 * 1000;


const BrowserGlueServiceFactory = {
  _instance: null,
  createInstance: function (outer, iid) 
  {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return this._instance == null ?
      this._instance = new BrowserGlue() : this._instance;
  }
};



function BrowserGlue() {
  this._init();
}

BrowserGlue.prototype = {
  _saveSession: false,

  _setPrefToSaveSession: function()
  {
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    prefBranch.setBoolPref("browser.sessionstore.resume_session_once", true);
  },

  
  observe: function(subject, topic, data) 
  {
    switch(topic) {
      case "xpcom-shutdown":
        this._dispose();
        break;
      case "quit-application": 
        this._onProfileShutdown();
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
        
        const cs = Cc["@mozilla.org/consoleservice;1"].
                   getService(Ci.nsIConsoleService);
        cs.logStringMessage(null); 
        cs.reset();
        break;
      case "quit-application-requested":
        this._onQuitRequest(subject, data);
        break;
      case "quit-application-granted":
        if (this._saveSession) {
          this._setPrefToSaveSession();
        }
        break;
      case "session-save":
        this._setPrefToSaveSession();
        subject.QueryInterface(Ci.nsISupportsPRBool);
        subject.data = true;
        break;
      case "idle":
        if (this.idleService.idleTime > BOOKMARKS_ARCHIVE_IDLE_TIME * 1000) {
          
          this._archiveBookmarks();
        }
        break;
    }
  }, 

  
  _init: function() 
  {
    
    const osvr = Cc['@mozilla.org/observer-service;1'].
                 getService(Ci.nsIObserverService);
    osvr.addObserver(this, "quit-application", false);
    osvr.addObserver(this, "xpcom-shutdown", false);
    osvr.addObserver(this, "prefservice:after-app-defaults", false);
    osvr.addObserver(this, "final-ui-startup", false);
    osvr.addObserver(this, "sessionstore-windows-restored", false);
    osvr.addObserver(this, "browser:purge-session-history", false);
    osvr.addObserver(this, "quit-application-requested", false);
    osvr.addObserver(this, "quit-application-granted", false);
    osvr.addObserver(this, "session-save", false);
  },

  
  _dispose: function() 
  {
    
    const osvr = Cc['@mozilla.org/observer-service;1'].
                 getService(Ci.nsIObserverService);
    osvr.removeObserver(this, "quit-application");
    osvr.removeObserver(this, "xpcom-shutdown");
    osvr.removeObserver(this, "prefservice:after-app-defaults");
    osvr.removeObserver(this, "final-ui-startup");
    osvr.removeObserver(this, "sessionstore-windows-restored");
    osvr.removeObserver(this, "browser:purge-session-history");
    osvr.removeObserver(this, "quit-application-requested");
    osvr.removeObserver(this, "quit-application-granted");
    osvr.removeObserver(this, "session-save");
  },

  _onAppDefaults: function()
  {
    
    
    var distro = new DistributionCustomizer();
    distro.applyPrefDefaults();
  },

  
  _onProfileStartup: function() 
  {
    
    try {
      var mustDisplayEULA = true;
      var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch);
      var EULAVersion = prefBranch.getIntPref("browser.EULA.version");
      mustDisplayEULA = !prefBranch.getBoolPref("browser.EULA." + EULAVersion + ".accepted");
    } catch(ex) {
    }

    if (mustDisplayEULA) {
      var ww2 = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                getService(Ci.nsIWindowWatcher);
      ww2.openWindow(null, "chrome://browser/content/EULA.xul", 
                     "_blank", "chrome,centerscreen,modal,resizable=yes", null);
    }

    this.Sanitizer.onStartup();
    
    var app = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo).
              QueryInterface(Ci.nsIXULRuntime);
    if (app.inSafeMode) {
      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.openWindow(null, "chrome://browser/content/safeMode.xul", 
                    "_blank", "chrome,centerscreen,modal,resizable=no", null);
    }

    
    this._initPlaces();

    
    
    var distro = new DistributionCustomizer();
    distro.applyCustomizations();

    
    this._migrateUI();
  },

  
  _onProfileShutdown: function() 
  {
    this._shutdownPlaces();
    this.idleService.removeIdleObserver(this, BOOKMARKS_ARCHIVE_IDLE_TIME);
    this.Sanitizer.onShutdown();
  },

  
  _onBrowserStartup: function()
  {
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    
    if (prefBranch.prefHasUserValue(PREF_EM_NEW_ADDONS_LIST)) {
      var args = Cc["@mozilla.org/supports-array;1"].
                 createInstance(Ci.nsISupportsArray);
      var str = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      str.data = "";
      args.AppendElement(str);
      var str = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      str.data = prefBranch.getCharPref(PREF_EM_NEW_ADDONS_LIST);
      args.AppendElement(str);
      const EMURL = "chrome://mozapps/content/extensions/extensions.xul";
      const EMFEATURES = "chrome,menubar,extra-chrome,toolbar,dialog=no,resizable";
      var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
               getService(Ci.nsIWindowWatcher);
      ww.openWindow(null, EMURL, "_blank", EMFEATURES, args);
      prefBranch.clearUserPref(PREF_EM_NEW_ADDONS_LIST);
    }
  },

  _onQuitRequest: function(aCancelQuit, aQuitType)
  {
    
    if ((aCancelQuit instanceof Ci.nsISupportsPRBool) && aCancelQuit.data)
      return;

    var wm = Cc["@mozilla.org/appshell/window-mediator;1"].
             getService(Ci.nsIWindowMediator);
    var windowcount = 0;
    var pagecount = 0;
    var browserEnum = wm.getEnumerator("navigator:browser");
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

    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);
    var showPrompt = true;
    try {
      
      
      
      if (prefBranch.getBoolPref("browser.warnOnQuit") == false)
        showPrompt = false;
      else if (aQuitType == "restart")
        showPrompt = prefBranch.getBoolPref("browser.warnOnRestart");
      else
        showPrompt = prefBranch.getBoolPref("browser.tabs.warnOnClose");
    } catch (ex) {}

    var buttonChoice = 0;
    if (showPrompt) {
      var bundleService = Cc["@mozilla.org/intl/stringbundle;1"].
                          getService(Ci.nsIStringBundleService);
      var quitBundle = bundleService.createBundle("chrome://browser/locale/quitDialog.properties");
      var brandBundle = bundleService.createBundle("chrome://branding/locale/brand.properties");

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

      var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"].
                          getService(Ci.nsIPromptService);

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

      buttonChoice = promptService.confirmEx(null, quitDialogTitle, message,
                                   flags, button0Title, button1Title, button2Title,
                                   neverAskText, neverAsk);

      switch (buttonChoice) {
      case 2:
        if (neverAsk.value)
          prefBranch.setBoolPref("browser.tabs.warnOnClose", false);
        break;
      case 1:
        aCancelQuit.QueryInterface(Ci.nsISupportsPRBool);
        aCancelQuit.data = true;
        break;
      case 0:
        this._saveSession = true;
        if (neverAsk.value) {
          if (aQuitType == "restart")
            prefBranch.setBoolPref("browser.warnOnRestart", false);
          else {
            
            prefBranch.setBoolPref("browser.tabs.warnOnClose", false);
            
            prefBranch.setIntPref("browser.startup.page", 3);
          }
        }
        break;
      }
    }
  },

  
  get Sanitizer() 
  {
    if(typeof(Sanitizer) != "function") { 
      Cc["@mozilla.org/moz/jssubscript-loader;1"].
      getService(Ci.mozIJSSubScriptLoader).
      loadSubScript("chrome://browser/content/sanitize.js", null);
    }
    return Sanitizer;
  },

  _idleService: null,
  get idleService() {
    if (!this._idleService)
      this._idleService = Cc["@mozilla.org/widget/idleservice;1"].
                          getService(Ci.nsIIdleService);
    return this._idleService;
  },

  

















  _initPlaces: function bg__initPlaces() {
    
    
    
    
    var histsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                  getService(Ci.nsINavHistoryService);

    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);

    var importBookmarks = false;
    var restoreDefaultBookmarks = false;
    try {
      restoreDefaultBookmarks = prefBranch.getBoolPref("browser.bookmarks.restore_default_bookmarks");
    } catch(ex) {}

    if (restoreDefaultBookmarks) {
      
      this._archiveBookmarks();
      
      importBookmarks = true;
    }
    else {
      try {
        importBookmarks = prefBranch.getBoolPref("browser.places.importBookmarksHTML");
      } catch(ex) {}
    }

    if (!importBookmarks) {
      
      
      this.ensurePlacesDefaultQueriesInitialized();
    }
    else {
      
      Cu.import("resource://gre/modules/utils.js");
      var bookmarksFile = PlacesUtils.getMostRecentBackup();

      if (!restoreDefaultBookmarks &&
          bookmarksFile && bookmarksFile.leafName.match("\.json$")) {
        
        PlacesUtils.restoreBookmarksFromJSONFile(bookmarksFile);
      }
      else {
        

        
        prefBranch.setIntPref("browser.places.smartBookmarksVersion", 0);

        var dirService = Cc["@mozilla.org/file/directory_service;1"].
                         getService(Ci.nsIProperties);

        if (restoreDefaultBookmarks) {
          
          var bookmarksFileName = "bookmarks.html";
          var bookmarksFile = dirService.get("profDef", Ci.nsILocalFile);
          bookmarksFile.append(bookmarksFileName);
        }
        else
          var bookmarksFile = dirService.get("BMarks", Ci.nsILocalFile);

        
        try {
          var importer = Cc["@mozilla.org/browser/places/import-export-service;1"].
                         getService(Ci.nsIPlacesImportExportService);
          importer.importHTMLFromFile(bookmarksFile, true );
        } finally {
          prefBranch.setBoolPref("browser.places.importBookmarksHTML", false);
          if (restoreDefaultBookmarks)
            prefBranch.setBoolPref("browser.bookmarks.restore_default_bookmarks",
                                   false);
        }
      }
    }

    
    
    this.idleService.addIdleObserver(this, BOOKMARKS_ARCHIVE_IDLE_TIME);
  },

  







  _shutdownPlaces: function bg__shutdownPlaces() {
    
    this._archiveBookmarks();

    
    
    var prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    var autoExportHTML = false;
    try {
      autoExportHTML = prefs.getBoolPref("browser.bookmarks.autoExportHTML");
    } catch(ex) {
      Components.utils.reportError(ex);
    }

    if (autoExportHTML) {
      Cc["@mozilla.org/browser/places/import-export-service;1"].
        getService(Ci.nsIPlacesImportExportService).
        backupBookmarksFile();
    }
  },

  


  _archiveBookmarks: function nsBrowserGlue__archiveBookmarks() {
    Cu.import("resource://gre/modules/utils.js");

    var lastBackup = PlacesUtils.getMostRecentBackup();

    
    
    if (!lastBackup ||
        Date.now() - lastBackup.lastModifiedTime > BOOKMARKS_ARCHIVE_INTERVAL) {
      var maxBackups = 5;
      var prefs = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefBranch);
      try {
        maxBackups = prefs.getIntPref("browser.bookmarks.max_backups");
      } catch(ex) {}

      PlacesUtils.archiveBookmarksFile(maxBackups, false );
    }
  },

  _migrateUI: function bg__migrateUI() {
    var prefBranch = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

    var migration = 0;
    try {
      migration = prefBranch.getIntPref("browser.migration.version");
    } catch(ex) {}

    if (migration == 0) {
      

      
      this._rdf = Cc["@mozilla.org/rdf/rdf-service;1"].getService(Ci.nsIRDFService);
      this._dataSource = this._rdf.GetDataSource("rdf:local-store");
      this._dirty = false;

      var currentSet = this._rdf.GetResource("currentset");

      
      var navBar = this._rdf.GetResource("chrome://browser/content/browser.xul#nav-bar");
      var target = this._getPersist(navBar, currentSet);
      if (target && !/(?:^|,)unified-back-forward-button(?:$|,)/.test(target))
        this._setPersist(navBar, currentSet, "unified-back-forward-button," + target);

      
      if (this._dirty)
        this._dataSource.QueryInterface(Ci.nsIRDFRemoteDataSource).Flush();

      
      this._rdf = null;
      this._dataSource = null;

      
      prefBranch.setIntPref("browser.migration.version", 1);
    }
  },

  _getPersist: function bg__getPersist(aSource, aProperty) {
    var target = this._dataSource.GetTarget(aSource, aProperty, true);
    if (target instanceof Ci.nsIRDFLiteral)
      return target.Value;
    return null;
  },

  _setPersist: function bg__setPersist(aSource, aProperty, aTarget) {
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

  
  
  
  
  sanitize: function(aParentWindow) 
  {
    this.Sanitizer.sanitize(aParentWindow);
  },

  ensurePlacesDefaultQueriesInitialized: function() {
    const SMART_BOOKMARKS_VERSION = 1;
    const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";
    const SMART_BOOKMARKS_PREF = "browser.places.smartBookmarksVersion";

    
    const MAX_RESULTS = 10;

    var prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefBranch);

    
    
    var smartBookmarksCurrentVersion = 0;
    try {
      smartBookmarksCurrentVersion = prefBranch.getIntPref(SMART_BOOKMARKS_PREF);
    } catch(ex) {}

    
    if (smartBookmarksCurrentVersion == -1 ||
        smartBookmarksCurrentVersion >= SMART_BOOKMARKS_VERSION)
      return;

    var bmsvc = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                getService(Ci.nsINavBookmarksService);
    var annosvc = Cc["@mozilla.org/browser/annotation-service;1"].
                  getService(Ci.nsIAnnotationService);

    var callback = {
      _placesBundle: Cc["@mozilla.org/intl/stringbundle;1"].
                     getService(Ci.nsIStringBundleService).
                     createBundle("chrome://browser/locale/places/places.properties"),

      _uri: function(aSpec) {
        return Cc["@mozilla.org/network/io-service;1"].
               getService(Ci.nsIIOService).
               newURI(aSpec, null, null);
      },

      runBatched: function() {
        var smartBookmarks = [];
        var bookmarksMenuIndex = 0;
        var bookmarksToolbarIndex = 0;

        
        var smart = {queryId: "MostVisited", 
                     itemId: null,
                     title: this._placesBundle.GetStringFromName("mostVisitedTitle"),
                     uri: this._uri("place:queryType=" +
                                    Ci.nsINavHistoryQueryOptions.QUERY_TYPE_HISTORY +
                                    "&sort=" +
                                    Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING +
                                    "&maxResults=" + MAX_RESULTS),
                     parent: bmsvc.toolbarFolder,
                     position: bookmarksToolbarIndex++};
        smartBookmarks.push(smart);

        
        smart = {queryId: "RecentlyBookmarked", 
                 itemId: null,
                 title: this._placesBundle.GetStringFromName("recentlyBookmarkedTitle"),
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
                 position: bookmarksMenuIndex++};
        smartBookmarks.push(smart);

        
        smart = {queryId: "RecentTags", 
                 itemId: null,
                 title: this._placesBundle.GetStringFromName("recentTagsTitle"),
                 uri: this._uri("place:"+
                    "type=" +
                    Ci.nsINavHistoryQueryOptions.RESULTS_AS_TAG_QUERY +
                    "&sort=" +
                    Ci.nsINavHistoryQueryOptions.SORT_BY_LASTMODIFIED_DESCENDING +
                    "&maxResults=" + MAX_RESULTS),
                 parent: bmsvc.bookmarksMenuFolder,
                 position: bookmarksMenuIndex++};
        smartBookmarks.push(smart);

        var smartBookmarkItemIds = annosvc.getItemsWithAnnotation(SMART_BOOKMARKS_ANNO, {});
        
        for each(var itemId in smartBookmarkItemIds) {
          var queryId = annosvc.getItemAnnotation(itemId, SMART_BOOKMARKS_ANNO);
          for (var i = 0; i < smartBookmarks.length; i++){
            if (smartBookmarks[i].queryId == queryId) {
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
          smartBookmark.itemId = bmsvc.insertBookmark(smartBookmark.parent,
                                                      smartBookmark.uri,
                                                      smartBookmark.position,
                                                      smartBookmark.title);
          annosvc.setItemAnnotation(smartBookmark.itemId,
                                    SMART_BOOKMARKS_ANNO, smartBookmark.queryId,
                                    0, annosvc.EXPIRE_NEVER);
        }
        
        
        
        if (smartBookmarkItemIds.length == 0)
          bmsvc.insertSeparator(bmsvc.bookmarksMenuFolder, bookmarksMenuIndex);
      }
    };

    try {
      bmsvc.runInBatchMode(callback, null);
    }
    catch(ex) {
      Components.utils.reportError(ex);
    }
    finally {
      prefBranch.setIntPref(SMART_BOOKMARKS_PREF, SMART_BOOKMARKS_VERSION);
      prefBranch.QueryInterface(Ci.nsIPrefService).savePrefFile(null);
    }
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


function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([BrowserGlue]);
}
