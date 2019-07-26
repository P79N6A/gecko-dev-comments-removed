




Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");





const Cc = Components.classes;
const Ci = Components.interfaces;


const IDLE_TIMEOUT_SECONDS = 5 * 60;


const PREF_TASKBAR_BRANCH    = "browser.taskbar.lists.";
const PREF_TASKBAR_ENABLED   = "enabled";
const PREF_TASKBAR_ITEMCOUNT = "maxListItemCount";
const PREF_TASKBAR_FREQUENT  = "frequent.enabled";
const PREF_TASKBAR_RECENT    = "recent.enabled";
const PREF_TASKBAR_TASKS     = "tasks.enabled";
const PREF_TASKBAR_REFRESH   = "refreshInSeconds";


const LIST_TYPE = {
  FREQUENT: 0
, RECENT: 1
}





this.EXPORTED_SYMBOLS = [
  "WinTaskbarJumpList",
];





XPCOMUtils.defineLazyGetter(this, "_prefs", function() {
  return Services.prefs.getBranch(PREF_TASKBAR_BRANCH);
});

XPCOMUtils.defineLazyGetter(this, "_stringBundle", function() {
  return Services.strings
                 .createBundle("chrome://browser/locale/taskbar.properties");
});

XPCOMUtils.defineLazyGetter(this, "PlacesUtils", function() {
  Components.utils.import("resource://gre/modules/PlacesUtils.jsm");
  return PlacesUtils;
});

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Components.utils.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

XPCOMUtils.defineLazyServiceGetter(this, "_idle",
                                   "@mozilla.org/widget/idleservice;1",
                                   "nsIIdleService");

XPCOMUtils.defineLazyServiceGetter(this, "_taskbarService",
                                   "@mozilla.org/windows-taskbar;1",
                                   "nsIWinTaskbar");

XPCOMUtils.defineLazyServiceGetter(this, "_winShellService",
                                   "@mozilla.org/browser/shell-service;1",
                                   "nsIWindowsShellService");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");





function _getString(name) {
  return _stringBundle.GetStringFromName(name);
}




var tasksCfg = [
  










  
  {
    get title()       _getString("taskbar.tasks.newTab.label"),
    get description() _getString("taskbar.tasks.newTab.description"),
    args:             "-new-tab about:blank",
    iconIndex:        3, 
    open:             true,
    close:            true, 
                            
                            
  },

  
  {
    get title()       _getString("taskbar.tasks.newWindow.label"),
    get description() _getString("taskbar.tasks.newWindow.description"),
    args:             "-browser",
    iconIndex:        2, 
    open:             true,
    close:            true, 
                            
  },

  
  {
    get title()       _getString("taskbar.tasks.newPrivateWindow.label"),
    get description() _getString("taskbar.tasks.newPrivateWindow.description"),
    args:             "-private-window",
    iconIndex:        4, 
    open:             true,
    close:            true, 
                            
  },
];




this.WinTaskbarJumpList =
{
  _builder: null,
  _tasks: null,
  _shuttingDown: false,

  

 

  startup: function WTBJL_startup() {
    
    if (!this._initTaskbar())
      return;

    
    
    
    try {
      
      this._shortcutMaintenance();
    } catch (ex) {
    }

    
    this._tasks = tasksCfg;

    
    this._refreshPrefs();

    
    this._initObs();

    
    this._updateTimer();
  },

  update: function WTBJL_update() {
    
    if (!this._enabled)
      return;

    
    this._buildList();
  },

  _shutdown: function WTBJL__shutdown() {
    this._shuttingDown = true;

    
    
    
    if (!PlacesUtils.history.hasHistoryEntries) {
      this.update();
    }

    this._free();
  },

  _shortcutMaintenance: function WTBJL__maintenace() {
    _winShellService.shortcutMaintenance();
  },

  









  _pendingStatements: {},
  _hasPendingStatements: function WTBJL__hasPendingStatements() {
    return Object.keys(this._pendingStatements).length > 0;
  },

  _buildList: function WTBJL__buildList() {
    if (this._hasPendingStatements()) {
      
      
      
      for (let listType in this._pendingStatements) {
        this._pendingStatements[listType].cancel();
        delete this._pendingStatements[listType];
      }
      this._builder.abortListBuild();
    }

    
    if (!this._showFrequent && !this._showRecent && !this._showTasks) {
      
      this._deleteActiveJumpList();
      return;
    }

    if (!this._startBuild())
      return;

    if (this._showTasks)
      this._buildTasks();

    
    if (this._showFrequent)
      this._buildFrequent();

    if (this._showRecent)
      this._buildRecent();

    this._commitBuild();
  },

  

 

  _startBuild: function WTBJL__startBuild() {
    var removedItems = Cc["@mozilla.org/array;1"].
                       createInstance(Ci.nsIMutableArray);
    this._builder.abortListBuild();
    if (this._builder.initListBuild(removedItems)) { 
      
      this._clearHistory(removedItems);
      return true;
    }
    return false;
  },

  _commitBuild: function WTBJL__commitBuild() {
    if (!this._hasPendingStatements() && !this._builder.commitListBuild()) {
      this._builder.abortListBuild();
    }
  },

  _buildTasks: function WTBJL__buildTasks() {
    var items = Cc["@mozilla.org/array;1"].
                createInstance(Ci.nsIMutableArray);
    this._tasks.forEach(function (task) {
      if ((this._shuttingDown && !task.close) || (!this._shuttingDown && !task.open))
        return;
      var item = this._getHandlerAppItem(task.title, task.description,
                                         task.args, task.iconIndex, null);
      items.appendElement(item, false);
    }, this);
    
    if (items.length > 0)
      this._builder.addListToBuild(this._builder.JUMPLIST_CATEGORY_TASKS, items);
  },

  _buildCustom: function WTBJL__buildCustom(title, items) {
    if (items.length > 0)
      this._builder.addListToBuild(this._builder.JUMPLIST_CATEGORY_CUSTOMLIST, items, title);
  },

  _buildFrequent: function WTBJL__buildFrequent() {
    
    if (!PlacesUtils.history.hasHistoryEntries) {
      return;
    }

    
    
    
    

    var items = Cc["@mozilla.org/array;1"].
                createInstance(Ci.nsIMutableArray);
    
    
    this._frequentHashList = [];

    this._pendingStatements[LIST_TYPE.FREQUENT] = this._getHistoryResults(
      Ci.nsINavHistoryQueryOptions.SORT_BY_VISITCOUNT_DESCENDING,
      this._maxItemCount,
      function (aResult) {
        if (!aResult) {
          delete this._pendingStatements[LIST_TYPE.FREQUENT];
          
          this._buildCustom(_getString("taskbar.frequent.label"), items);
          this._commitBuild();
          return;
        }

        let title = aResult.title || aResult.uri;
        let faviconPageUri = Services.io.newURI(aResult.uri, null, null);
        let shortcut = this._getHandlerAppItem(title, title, aResult.uri, 1, 
                                               faviconPageUri);
        items.appendElement(shortcut, false);
        this._frequentHashList.push(aResult.uri);
      },
      this
    );
  },

  _buildRecent: function WTBJL__buildRecent() {
    
    if (!PlacesUtils.history.hasHistoryEntries) {
      return;
    }

    var items = Cc["@mozilla.org/array;1"].
                createInstance(Ci.nsIMutableArray);
    
    
    var count = 0;

    this._pendingStatements[LIST_TYPE.RECENT] = this._getHistoryResults(
      Ci.nsINavHistoryQueryOptions.SORT_BY_DATE_DESCENDING,
      this._maxItemCount * 2,
      function (aResult) {
        if (!aResult) {
          
          this._buildCustom(_getString("taskbar.recent.label"), items);
          delete this._pendingStatements[LIST_TYPE.RECENT];
          this._commitBuild();
          return;
        }

        if (count >= this._maxItemCount) {
          return;
        }

        
        if (this._frequentHashList &&
            this._frequentHashList.indexOf(aResult.uri) != -1) {
          return;
        }

        let title = aResult.title || aResult.uri;
        let faviconPageUri = Services.io.newURI(aResult.uri, null, null);
        let shortcut = this._getHandlerAppItem(title, title, aResult.uri, 1,
                                               faviconPageUri);
        items.appendElement(shortcut, false);
        count++;
      },
      this
    );
  },

  _deleteActiveJumpList: function WTBJL__deleteAJL() {
    this._builder.deleteActiveList();
  },

  



  _getHandlerAppItem: function WTBJL__getHandlerAppItem(name, description, 
                                                        args, iconIndex, 
                                                        faviconPageUri) {
    var file = Services.dirsvc.get("XREExeF", Ci.nsILocalFile);

    var handlerApp = Cc["@mozilla.org/uriloader/local-handler-app;1"].
                     createInstance(Ci.nsILocalHandlerApp);
    handlerApp.executable = file;
    
    if (name && name.length != 0)
      handlerApp.name = name;
    handlerApp.detailedDescription = description;
    handlerApp.appendParameter(args);

    var item = Cc["@mozilla.org/windows-jumplistshortcut;1"].
               createInstance(Ci.nsIJumpListShortcut);
    item.app = handlerApp;
    item.iconIndex = iconIndex;
    item.faviconPageUri = faviconPageUri;
    return item;
  },

  _getSeparatorItem: function WTBJL__getSeparatorItem() {
    var item = Cc["@mozilla.org/windows-jumplistseparator;1"].
               createInstance(Ci.nsIJumpListSeparator);
    return item;
  },

  



  _getHistoryResults:
  function WTBLJL__getHistoryResults(aSortingMode, aLimit, aCallback, aScope) {
    var options = PlacesUtils.history.getNewQueryOptions();
    options.maxResults = aLimit;
    options.sortingMode = aSortingMode;
    var query = PlacesUtils.history.getNewQuery();

    
    return PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                              .asyncExecuteLegacyQueries([query], 1, options, {
      handleResult: function (aResultSet) {
        for (let row; (row = aResultSet.getNextRow());) {
          try {
            aCallback.call(aScope,
                           { uri: row.getResultByIndex(1)
                           , title: row.getResultByIndex(2)
                           });
          } catch (e) {}
        }
      },
      handleError: function (aError) {
        Components.utils.reportError(
          "Async execution error (" + aError.result + "): " + aError.message);
      },
      handleCompletion: function (aReason) {
        aCallback.call(WinTaskbarJumpList, null);
      },
    });
  },

  _clearHistory: function WTBJL__clearHistory(items) {
    if (!items)
      return;
    var URIsToRemove = [];
    var e = items.enumerate();
    while (e.hasMoreElements()) {
      let oldItem = e.getNext().QueryInterface(Ci.nsIJumpListShortcut);
      if (oldItem) {
        try { 
          let uriSpec = oldItem.app.getParameter(0);
          URIsToRemove.push(NetUtil.newURI(uriSpec));
        } catch (err) { }
      }
    }
    if (URIsToRemove.length > 0) {
      PlacesUtils.bhistory.removePages(URIsToRemove, URIsToRemove.length, true);
    }
  },

  

 

  _refreshPrefs: function WTBJL__refreshPrefs() {
    this._enabled = _prefs.getBoolPref(PREF_TASKBAR_ENABLED);
    this._showFrequent = _prefs.getBoolPref(PREF_TASKBAR_FREQUENT);
    this._showRecent = _prefs.getBoolPref(PREF_TASKBAR_RECENT);
    this._showTasks = _prefs.getBoolPref(PREF_TASKBAR_TASKS);
    this._maxItemCount = _prefs.getIntPref(PREF_TASKBAR_ITEMCOUNT);
  },

  

 

  _initTaskbar: function WTBJL__initTaskbar() {
    this._builder = _taskbarService.createJumpListBuilder();
    if (!this._builder || !this._builder.available)
      return false;

    return true;
  },

  _initObs: function WTBJL__initObs() {
    
    
    
    Services.obs.addObserver(this, "profile-before-change", false);
    Services.obs.addObserver(this, "browser:purge-session-history", false);
    _prefs.addObserver("", this, false);
  },
 
  _freeObs: function WTBJL__freeObs() {
    Services.obs.removeObserver(this, "profile-before-change");
    Services.obs.removeObserver(this, "browser:purge-session-history");
    _prefs.removeObserver("", this);
  },

  _updateTimer: function WTBJL__updateTimer() {
    if (this._enabled && !this._shuttingDown && !this._timer) {
      this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this._timer.initWithCallback(this,
                                   _prefs.getIntPref(PREF_TASKBAR_REFRESH)*1000,
                                   this._timer.TYPE_REPEATING_SLACK);
    }
    else if ((!this._enabled || this._shuttingDown) && this._timer) {
      this._timer.cancel();
      delete this._timer;
    }
  },

  _hasIdleObserver: false,
  _updateIdleObserver: function WTBJL__updateIdleObserver() {
    if (this._enabled && !this._shuttingDown && !this._hasIdleObserver) {
      _idle.addIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._hasIdleObserver = true;
    }
    else if ((!this._enabled || this._shuttingDown) && this._hasIdleObserver) {
      _idle.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._hasIdleObserver = false;
    }
  },

  _free: function WTBJL__free() {
    this._freeObs();
    this._updateTimer();
    this._updateIdleObserver();
    delete this._builder;
  },

  



  notify: function WTBJL_notify(aTimer) {
    
    this._updateIdleObserver();
    this.update();
  },

  observe: function WTBJL_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed":
        if (this._enabled == true && !_prefs.getBoolPref(PREF_TASKBAR_ENABLED))
          this._deleteActiveJumpList();
        this._refreshPrefs();
        this._updateTimer();
        this._updateIdleObserver();
        this.update();
      break;

      case "profile-before-change":
        this._shutdown();
      break;

      case "browser:purge-session-history":
        this.update();
      break;
      case "idle":
        if (this._timer) {
          this._timer.cancel();
          delete this._timer;
        }
      break;

      case "back":
        this._updateTimer();
      break;
    }
  },
};
