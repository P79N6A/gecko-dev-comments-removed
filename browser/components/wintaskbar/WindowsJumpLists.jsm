





































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





const Cc = Components.classes;
const Ci = Components.interfaces;


const PREF_TASKBAR_BRANCH    = "browser.taskbar.lists.";
const PREF_TASKBAR_ENABLED   = "enabled";
const PREF_TASKBAR_ITEMCOUNT = "maxListItemCount";
const PREF_TASKBAR_FREQUENT  = "frequent.enabled";
const PREF_TASKBAR_RECENT    = "recent.enabled";
const PREF_TASKBAR_TASKS     = "tasks.enabled";
const PREF_TASKBAR_REFRESH   = "refreshInSeconds";





let EXPORTED_SYMBOLS = [
  "WinTaskbarJumpList",
];





XPCOMUtils.defineLazyGetter(this, "_prefs", function() {
  return Cc["@mozilla.org/preferences-service;1"]
           .getService(Ci.nsIPrefService)
           .getBranch(PREF_TASKBAR_BRANCH)
           .QueryInterface(Ci.nsIPrefBranch2);
});

XPCOMUtils.defineLazyGetter(this, "_stringBundle", function() {
  return Cc["@mozilla.org/intl/stringbundle;1"]
           .getService(Ci.nsIStringBundleService)
           .createBundle("chrome://browser/locale/taskbar.properties");
});

XPCOMUtils.defineLazyServiceGetter(this, "_taskbarService",
                                   "@mozilla.org/windows-taskbar;1",
                                   "nsIWinTaskbar");

XPCOMUtils.defineLazyServiceGetter(this, "_navHistoryService",
                                   "@mozilla.org/browser/nav-history-service;1",
                                   "nsINavHistoryService");

XPCOMUtils.defineLazyServiceGetter(this, "_observerService",
                                   "@mozilla.org/observer-service;1",
                                   "nsIObserverService");

XPCOMUtils.defineLazyServiceGetter(this, "_directoryService",
                                   "@mozilla.org/file/directory_service;1",
                                   "nsIProperties");

XPCOMUtils.defineLazyServiceGetter(this, "_ioService",
                                   "@mozilla.org/network/io-service;1",
                                   "nsIIOService");

XPCOMUtils.defineLazyServiceGetter(this, "_winShellService",
                                   "@mozilla.org/browser/shell-service;1",
                                   "nsIWindowsShellService");

XPCOMUtils.defineLazyServiceGetter(this, "_privateBrowsingSvc",
                                   "@mozilla.org/privatebrowsing;1",
                                   "nsIPrivateBrowsingService");





function _getString(name) {
  return _stringBundle.GetStringFromName(name);
}




var tasksCfg = [
  










  
  {
    get title()       _getString("taskbar.tasks.newTab.label"),
    get description() _getString("taskbar.tasks.newTab.description"),
    args:             "-new-tab about:blank",
    iconIndex:        0, 
    open:             true,
    close:            false, 
  },

  
  {
    get title()       _getString("taskbar.tasks.newWindow.label"),
    get description() _getString("taskbar.tasks.newWindow.description"),
    args:             "-browser",
    iconIndex:        0, 
    open:             true,
    close:            false, 
  },

  
  {
    get title() {
      if (_privateBrowsingSvc.privateBrowsingEnabled)
        return _getString("taskbar.tasks.exitPrivacyMode.label");
      else
        return _getString("taskbar.tasks.enterPrivacyMode.label");
    },
    get description() {
      if (_privateBrowsingSvc.privateBrowsingEnabled)
        return _getString("taskbar.tasks.exitPrivacyMode.description");
      else
        return _getString("taskbar.tasks.enterPrivacyMode.description");
    },
    args:             "-private-toggle",
    iconIndex:        0, 
    get open() {
      
      return !_privateBrowsingSvc.autoStarted;
    },
    get close() {
      
      return !_privateBrowsingSvc.autoStarted;
    },
  },
];




var WinTaskbarJumpList =
{
  _builder: null,
  _tasks: null,
  _shuttingDown: false,

  

 

  startup: function WTBJL_startup() {
    
    if (!this._initTaskbar())
      return;

    
    
    
    this._shortcutMaintenance();

    
    this._tasks = tasksCfg;

    
    this._refreshPrefs();
    
    
    this._initObs();

    
    this._initTimer();
  },

  update: function WTBJL_update() {
    
    if (!this._enabled)
      return;

    
    this._buildList();
  },

  _shutdown: function WTBJL__shutdown() {
    this._shuttingDown = true;
    this.update();
    this._free();
  },

  _shortcutMaintenance: function WTBJL__maintenace() {
    _winShellService.shortcutMaintenance();
  },

  

 

  _buildList: function WTBJL__buildList() {
    
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
    if (!this._builder.commitListBuild())
      this._builder.abortListBuild();
  },

  _buildTasks: function WTBJL__buildTasks() {
    var items = Cc["@mozilla.org/array;1"].
                createInstance(Ci.nsIMutableArray);
    this._tasks.forEach(function (task) {
      if ((this._shuttingDown && !task.close) || (!this._shuttingDown && !task.open))
        return;
      var item = this._getHandlerAppItem(task.title, task.description,
                                         task.args, task.iconIndex);
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
    
    
    
    

    var items = Cc["@mozilla.org/array;1"].
                createInstance(Ci.nsIMutableArray);
    var list = this._getNavFrequent(this._maxItemCount);

    if (!list || list.length == 0)
      return;

    
    
    this._frequentHashList = [];

    list.forEach(function (entry) {
      let shortcut = this._getHandlerAppItem(entry.title, entry.title, entry.uri, 1);
      items.appendElement(shortcut, false);
      this._frequentHashList.push(entry.uri);
    }, this);
    this._buildCustom(_getString("taskbar.frequent.label"), items);
  },

  _buildRecent: function WTBJL__buildRecent() {
    var items = Cc["@mozilla.org/array;1"].
                createInstance(Ci.nsIMutableArray);
    var list = this._getNavRecent(this._maxItemCount*2);
    
    if (!list || list.length == 0)
      return;

    let count = 0;
    for (let idx = 0; idx < list.length; idx++) {
      if (count >= this._maxItemCount)
        break;
      let entry = list[idx];
      
      
      if (this._frequentHashList &&
          this._frequentHashList.indexOf(entry.uri) != -1)
        continue;
      let shortcut = this._getHandlerAppItem(entry.title, entry.title, entry.uri, 1);
      items.appendElement(shortcut, false);
      count++;
    }
    this._buildCustom(_getString("taskbar.recent.label"), items);
  },

  _deleteActiveJumpList: function WTBJL__deleteAJL() {
    this._builder.deleteActiveList();
  },

  



  _getHandlerAppItem: function WTBJL__getHandlerAppItem(name, description, args, icon) {
    var file = _directoryService.get("XCurProcD", Ci.nsILocalFile);

    
    file.append("firefox.exe");

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
    item.iconIndex = icon;
    return item;
  },

  _getSeparatorItem: function WTBJL__getSeparatorItem() {
    var item = Cc["@mozilla.org/windows-jumplistseparator;1"].
               createInstance(Ci.nsIJumpListSeparator);
    return item;
  },

  

 

  _getNavFrequent: function WTBJL__getNavFrequent(depth) {
    var options = _navHistoryService.getNewQueryOptions();
    var query = _navHistoryService.getNewQuery();
    
    query.beginTimeReference = query.TIME_RELATIVE_NOW;
    query.beginTime = -24 * 30 * 60 * 60 * 1000000; 
    query.endTimeReference = query.TIME_RELATIVE_NOW;

    options.maxResults = depth;
    options.queryType = options.QUERY_TYPE_HISTORY;
    options.sortingMode = options.SORT_BY_VISITCOUNT_DESCENDING;
    options.resultType = options.RESULT_TYPE_URI;

    var result = _navHistoryService.executeQuery(query, options);

    var list = [];

    var rootNode = result.root;
    rootNode.containerOpen = true;

    for (let idx = 0; idx < rootNode.childCount; idx++) {
      let node = rootNode.getChild(idx);
      list.push({uri: node.uri, title: node.title});
    }
    rootNode.containerOpen = false;

    return list;
  },
  
  _getNavRecent: function WTBJL__getNavRecent(depth) {
    var options = _navHistoryService.getNewQueryOptions();
    var query = _navHistoryService.getNewQuery();
    
    query.beginTimeReference = query.TIME_RELATIVE_NOW;
    query.beginTime = -48 * 60 * 60 * 1000000; 
    query.endTimeReference = query.TIME_RELATIVE_NOW;

    options.maxResults = depth;
    options.queryType = options.QUERY_TYPE_HISTORY;
    options.sortingMode = options.SORT_BY_LASTMODIFIED_DESCENDING;
    options.resultType = options.RESULT_TYPE_URI;

    var result = _navHistoryService.executeQuery(query, options);

    var list = [];

    var rootNode = result.root;
    rootNode.containerOpen = true;

    for (var idx = 0; idx < rootNode.childCount; idx++) {
      var node = rootNode.getChild(idx);
      list.push({uri: node.uri, title: node.title});
    }
    rootNode.containerOpen = false;

    return list;
  },
  
  _clearHistory: function WTBJL__clearHistory(items) {
    if (!items)
      return;
    var enum = items.enumerate();
    while (enum.hasMoreElements()) {
      let oldItem = enum.getNext().QueryInterface(Ci.nsIJumpListShortcut);
      if (oldItem) {
        try { 
          let uriSpec = oldItem.app.getParameter(0);
          _navHistoryService.QueryInterface(Ci.nsIBrowserHistory).removePage(
            _ioService.newURI(uriSpec, null, null));
        } catch (err) { }
      }
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
    _observerService.addObserver(this, "private-browsing", false);
    _observerService.addObserver(this, "quit-application-granted", false);
    _observerService.addObserver(this, "browser:purge-session-history", false);
    _prefs.addObserver("", this, false);
  },
 
  _freeObs: function WTBJL__freeObs() {
    _observerService.removeObserver(this, "private-browsing");
    _observerService.removeObserver(this, "quit-application-granted");
    _observerService.removeObserver(this, "browser:purge-session-history");
    _prefs.removeObserver("", this);
  },

  _initTimer: function WTBJL__initTimer(aTimer) {
    this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._timer.initWithCallback(this,
                                 _prefs.getIntPref(PREF_TASKBAR_REFRESH)*1000,
                                 this._timer.TYPE_REPEATING_SLACK);
  },

  _free: function WTBJL__free() {
    this._freeObs();
    delete this._builder;
    delete this._timer;
  },

  



  notify: function WTBJL_notify(aTimer) {
    this.update();
  },

  observe: function WTBJL_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed":
        if (this._enabled == true && !_prefs.getBoolPref(PREF_TASKBAR_ENABLED))
          this._deleteActiveJumpList();
        this._refreshPrefs();
        this.update();
      break;

      case "quit-application-granted":
        this._shutdown();
      break;

      case "browser:purge-session-history":
        this.update();
      break;

      case "private-browsing":
        this.update();
      break;
    }
  },
};
