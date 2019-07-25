





































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

const CACHE_MAX_GROUP_ENTRIES = 100;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");





function electrolify(service) {
  
  
  
  service.wrappedJSObject = service;

  var appInfo = Cc["@mozilla.org/xre/app-info;1"];
  if (!appInfo || appInfo.getService(Ci.nsIXULRuntime).processType ==
      Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
    

    
    
    service.receiveMessage = function(aMessage) {
      var json = aMessage.json;
      
      
      
      
      
      
      
      
      
      
      
      
      
      const NAME_WHITELIST = ["browser.upload.lastDir", "spellcheck.lang"];
      if (NAME_WHITELIST.indexOf(json.name) == -1)
        return { succeeded: false };

      switch (aMessage.name) {
        case "ContentPref:getPref":
          return { succeeded: true,
                   value: service.getPref(json.group, json.name, json.value) };

        case "ContentPref:setPref":
          service.setPref(json.group, json.name, json.value);
          return { succeeded: true };
      }
    };
  } else {
    

    service._dbInit = function(){}; 

    service.messageManager = Cc["@mozilla.org/childprocessmessagemanager;1"].
                             getService(Ci.nsISyncMessageSender);

    
    [
      ['getPref', ['group', 'name'], ['_parseGroupParam']],
      ['setPref', ['group', 'name', 'value'], ['_parseGroupParam']],
    ].forEach(function(data) {
      var method = data[0];
      var params = data[1];
      var parsers = data[2];
      service[method] = function __remoted__() {
        var json = {};
        for (var i = 0; i < params.length; i++) {
          if (params[i]) {
            json[params[i]] = arguments[i];
            if (parsers[i])
              json[params[i]] = this[parsers[i]](json[params[i]]);
          }
        }
        var ret = service.messageManager.sendSyncMessage('ContentPref:' + method, json)[0];
        if (!ret.succeeded)
          throw "ContentPrefs remoting failed to pass whitelist";
        return ret.value;
      };
    });
  }
}

function ContentPrefService() {
  electrolify(this);

  
  
  
  
  this._dbInit();

  
  this._inPrivateBrowsing = false;
  try { 
    var pbs = Cc["@mozilla.org/privatebrowsing;1"].
                getService(Ci.nsIPrivateBrowsingService);
    this._inPrivateBrowsing = pbs.privateBrowsingEnabled;
  } catch (e) {}
  this._observerSvc.addObserver(this, "private-browsing", false);

  
  this._observerSvc.addObserver(this, "xpcom-shutdown", false);
}

var inMemoryPrefsProto = {
  getPref: function(aName, aGroup) {
    aGroup = aGroup || "__GlobalPrefs__";

    if (this._prefCache[aGroup] && this._prefCache[aGroup].hasOwnProperty(aName)) {
      let value = this._prefCache[aGroup][aName];
      return [true, value];
    }
    return [false, undefined];
  },

  setPref: function(aName, aValue, aGroup) {
    if (typeof aValue == "boolean")
      aValue = aValue ? 1 : 0;
    else if (aValue === undefined)
      aValue = null;

    this.cachePref(aName, aValue, aGroup);
  },

  removePref: function(aName, aGroup) {
    aGroup = aGroup || "__GlobalPrefs__";

    if (this._prefCache[aGroup].hasOwnProperty(aName)) {
      delete this._prefCache[aGroup][aName];
      if (Object.keys(this._prefCache[aGroup]).length == 0) {
        
        delete this._prefCache[aGroup];
      }
    }
  },

  invalidate: function(aKeepGlobal) {
    if (!aKeepGlobal) {
      this._prefCache = {};
      return;
    }

    if (this._prefCache.hasOwnProperty("__GlobalPrefs__")) {
      let globals = this._prefCache["__GlobalPrefs__"];
      this._prefCache = {"__GlobalPrefs__": globals};
    } else {
      this._prefCache = {};
    }
  }
};

ContentPrefService.prototype = {
  
  

  classID:          Components.ID("{e6a3f533-4ffa-4615-8eb4-d4e72d883fa7}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIContentPrefService,
                                           Ci.nsIFrameMessageListener]),


  
  

  
  __observerSvc: null,
  get _observerSvc() {
    if (!this.__observerSvc)
      this.__observerSvc = Cc["@mozilla.org/observer-service;1"].
                           getService(Ci.nsIObserverService);
    return this.__observerSvc;
  },

  
  __consoleSvc: null,
  get _consoleSvc() {
    if (!this.__consoleSvc)
      this.__consoleSvc = Cc["@mozilla.org/consoleservice;1"].
                          getService(Ci.nsIConsoleService);
    return this.__consoleSvc;
  },

  
  __prefSvc: null,
  get _prefSvc() {
    if (!this.__prefSvc)
      this.__prefSvc = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefBranch2);
    return this.__prefSvc;
  },


  
  

  _destroy: function ContentPrefService__destroy() {
    this._observerSvc.removeObserver(this, "xpcom-shutdown");
    this._observerSvc.removeObserver(this, "private-browsing");

    
    
    if (this.__stmtSelectPrefID) {
      this.__stmtSelectPrefID.finalize();
      this.__stmtSelectPrefID = null;
    }
    if (this.__stmtSelectGlobalPrefID) {
      this.__stmtSelectGlobalPrefID.finalize();
      this.__stmtSelectGlobalPrefID = null;
    }
    if (this.__stmtInsertPref) {
      this.__stmtInsertPref.finalize();
      this.__stmtInsertPref = null;
    }
    if (this.__stmtInsertGroup) {
      this.__stmtInsertGroup.finalize();
      this.__stmtInsertGroup = null;
    }
    if (this.__stmtInsertSetting) {
      this.__stmtInsertSetting.finalize();
      this.__stmtInsertSetting = null;
    }
    if (this.__stmtSelectGroupID) {
      this.__stmtSelectGroupID.finalize();
      this.__stmtSelectGroupID = null;
    }
    if (this.__stmtSelectSettingID) {
      this.__stmtSelectSettingID.finalize();
      this.__stmtSelectSettingID = null;
    }
    if (this.__stmtSelectPref) {
      this.__stmtSelectPref.finalize();
      this.__stmtSelectPref = null;
    }
    if (this.__stmtSelectGlobalPref) {
      this.__stmtSelectGlobalPref.finalize();
      this.__stmtSelectGlobalPref = null;
    }
    if (this.__stmtSelectPrefsByName) {
      this.__stmtSelectPrefsByName.finalize();
      this.__stmtSelectPrefsByName = null;
    }
    if (this.__stmtDeleteSettingIfUnused) {
      this.__stmtDeleteSettingIfUnused.finalize();
      this.__stmtDeleteSettingIfUnused = null;
    }
    if(this.__stmtSelectPrefs) {
      this.__stmtSelectPrefs.finalize();
      this.__stmtSelectPrefs = null;
    }
    if(this.__stmtDeleteGroupIfUnused) {
      this.__stmtDeleteGroupIfUnused.finalize();
      this.__stmtDeleteGroupIfUnused = null;
    }
    if (this.__stmtDeletePref) {
      this.__stmtDeletePref.finalize();
      this.__stmtDeletePref = null;
    }
    if (this.__stmtUpdatePref) {
      this.__stmtUpdatePref.finalize();
      this.__stmtUpdatePref = null;
    }

    
    
    
    
    for (var i in this) {
      try { this[i] = null }
      
      catch(ex) {}
    }
  },


  
  

  observe: function ContentPrefService_observe(subject, topic, data) {
    switch (topic) {
      case "xpcom-shutdown":
        this._destroy();
        break;
      case "private-browsing":
        switch (data) {
          case "enter":
            this._inPrivateBrowsing = true;
            break;
          case "exit":
            this._inPrivateBrowsing = false;
            this._privModeStorage.invalidate();
            break;
        }
        break;
    }
  },


  
  

  _cache: Object.create(inMemoryPrefsProto, {
    _prefCache: { 
      value: {}, configurable: true, writable: true, enumerable: true
    },

    cachePref: { value:
      function(aName, aValue, aGroup) {
        aGroup = aGroup || "__GlobalPrefs__";

        if (!this._prefCache[aGroup]) {
          this._possiblyCleanCache();
          this._prefCache[aGroup] = {};
        }

        this._prefCache[aGroup][aName] = aValue;
      },
    },

    _possiblyCleanCache: { value:
      function() {
        let groupCount = Object.keys(this._prefCache).length;

        if (groupCount >= CACHE_MAX_GROUP_ENTRIES) {
          
          for (let entry in this._prefCache) {
            delete this._prefCache[entry];
            groupCount--;

            if (groupCount < CACHE_MAX_GROUP_ENTRIES / 2)
              break;
          }
        }
      }
    }
  }),

  
  
  _privModeStorage: Object.create(inMemoryPrefsProto, {
    _prefCache: { 
      value: {}, configurable: true, writable: true, enumerable: true
    },

    cachePref: { value: 
      function(aName, aValue, aGroup) {
        aGroup = aGroup || "__GlobalPrefs__";

        if (!this._prefCache[aGroup]) {
          this._prefCache[aGroup] = {};
        }

        this._prefCache[aGroup][aName] = aValue;
      }
    },

    getPrefs: { value: 
      function(aGroup) {
        aGroup = aGroup || "__GlobalPrefs__";
        if (this._prefCache[aGroup]) {
          return [true, this._prefCache[aGroup]];
        }
        return [false, undefined];
      }
    },

    groupsForName: { value: 
      function(aName) {
        var res = [];
        for (let entry in this._prefCache) {
          if (this._prefCache[entry]) {
            if (entry === "__GlobalPrefs__") {
              entry = null;
            }
            res.push(entry);
          }
        }
        return res;
      }
    }
  }),

  
  

  getPref: function ContentPrefService_getPref(aGroup, aName, aCallback) {
    if (!aName)
      throw Components.Exception("aName cannot be null or an empty string",
                                 Cr.NS_ERROR_ILLEGAL_VALUE);

    var group = this._parseGroupParam(aGroup);

    if (this._inPrivateBrowsing) {
      let [haspref, value] = this._privModeStorage.getPref(aName, group);
      if (haspref) {
        if (aCallback) {
          this._scheduleCallback(function(){aCallback.onResult(value);});
          return;
        }
        return value;
      }
      
      
    }

    if (group == null)
      return this._selectGlobalPref(aName, aCallback);
    return this._selectPref(group, aName, aCallback);
  },

  setPref: function ContentPrefService_setPref(aGroup, aName, aValue) {
    
    var currentValue = this.getPref(aGroup, aName);
    if (typeof currentValue != "undefined") {
      if (currentValue == aValue)
        return;
    }

    var group = this._parseGroupParam(aGroup);

    if (this._inPrivateBrowsing) {
      this._privModeStorage.setPref(aName, aValue, group);
      this._notifyPrefSet(group, aName, aValue);
      return;
    }

    var settingID = this._selectSettingID(aName) || this._insertSetting(aName);
    var groupID, prefID;
    if (group == null) {
      groupID = null;
      prefID = this._selectGlobalPrefID(settingID);
    }
    else {
      groupID = this._selectGroupID(group) || this._insertGroup(group);
      prefID = this._selectPrefID(groupID, settingID);
    }

    
    if (prefID)
      this._updatePref(prefID, aValue);
    else
      this._insertPref(groupID, settingID, aValue);

    this._cache.setPref(aName, aValue, group);

    this._notifyPrefSet(group, aName, aValue);
  },

  hasPref: function ContentPrefService_hasPref(aGroup, aName) {
    
    
    return (typeof this.getPref(aGroup, aName) != "undefined");
  },

  hasCachedPref: function ContentPrefService_hasCachedPref(aGroup, aName) {
    if (!aName)
      throw Components.Exception("aName cannot be null or an empty string",
                                 Cr.NS_ERROR_ILLEGAL_VALUE);

    let group = this._parseGroupParam(aGroup);
    let storage = this._inPrivateBrowsing? this._privModeStorage: this._cache;
    let [cached,] = storage.getPref(aName, group);
    return cached;
  },

  removePref: function ContentPrefService_removePref(aGroup, aName) {
    
    if (!this.hasPref(aGroup, aName))
      return;

    var group = this._parseGroupParam(aGroup);

    if (this._inPrivateBrowsing) {
      this._privModeStorage.removePref(aName, group);
      this._notifyPrefRemoved(group, aName);
      return;
    }

    var settingID = this._selectSettingID(aName);
    var groupID, prefID;
    if (group == null) {
      groupID = null;
      prefID = this._selectGlobalPrefID(settingID);
    }
    else {
      groupID = this._selectGroupID(group);
      prefID = this._selectPrefID(groupID, settingID);
    }

    this._deletePref(prefID);

    
    this._deleteSettingIfUnused(settingID);
    if (groupID)
      this._deleteGroupIfUnused(groupID);

    this._cache.removePref(aName, group);
    this._notifyPrefRemoved(group, aName);
  },

  removeGroupedPrefs: function ContentPrefService_removeGroupedPrefs() {
    
    if (this._inPrivateBrowsing) {
        
        this._privModeStorage.invalidate(true);
    }
    this._cache.invalidate(true);
    this._dbConnection.beginTransaction();
    try {
      this._dbConnection.executeSimpleSQL("DELETE FROM prefs WHERE groupID IS NOT NULL");
      this._dbConnection.executeSimpleSQL("DELETE FROM groups");
      this._dbConnection.executeSimpleSQL(
        "DELETE FROM settings " +
        "WHERE id NOT IN (SELECT DISTINCT settingID FROM prefs)"
      );
      this._dbConnection.commitTransaction();
    }
    catch(ex) {
      this._dbConnection.rollbackTransaction();
      throw ex;
    }
  },

  removePrefsByName: function ContentPrefService_removePrefsByName(aName) {
    if (!aName)
      throw Components.Exception("aName cannot be null or an empty string",
                                 Cr.NS_ERROR_ILLEGAL_VALUE);

    if (this._inPrivateBrowsing) {
      let groupNames = this._privModeStorage.groupsForName(aName);
      for (var i = 0; i < groupNames.length; i++) {
        let groupName = groupNames[i];
        this._privModeStorage.removePref(aName, groupName);
        this._notifyPrefRemoved(groupName, aName);
      }
    }

    var settingID = this._selectSettingID(aName);
    if (!settingID)
      return;
    
    var selectGroupsStmt = this._dbCreateStatement(
      "SELECT groups.id AS groupID, groups.name AS groupName " +
      "FROM prefs " +
      "JOIN groups ON prefs.groupID = groups.id " +
      "WHERE prefs.settingID = :setting "
    );
    
    var groupNames = [];
    var groupIDs = [];
    try {
      selectGroupsStmt.params.setting = settingID;
    
      while (selectGroupsStmt.executeStep()) {
        groupIDs.push(selectGroupsStmt.row["groupID"]);
        groupNames.push(selectGroupsStmt.row["groupName"]);
      }
    }
    finally {
      selectGroupsStmt.reset();
    }
    
    if (this.hasPref(null, aName)) {
      groupNames.push(null);
    }

    this._dbConnection.executeSimpleSQL("DELETE FROM prefs WHERE settingID = " + settingID);
    this._dbConnection.executeSimpleSQL("DELETE FROM settings WHERE id = " + settingID);

    for (var i = 0; i < groupNames.length; i++) {
      this._cache.removePref(aName, groupNames[i]);
      if (groupNames[i]) 
        this._deleteGroupIfUnused(groupIDs[i]);
      if (!this._inPrivateBrowsing) {
        this._notifyPrefRemoved(groupNames[i], aName);
      }
    }
  },

  getPrefs: function ContentPrefService_getPrefs(aGroup) {
    var group = this._parseGroupParam(aGroup);
    if (this._inPrivateBrowsing) {
        let prefs = Cc["@mozilla.org/hash-property-bag;1"].
                    createInstance(Ci.nsIWritablePropertyBag);
        let [hasbranch,properties] = this._privModeStorage.getPrefs(group);
        for (let entry in properties) {
          if (properties.hasOwnProperty(entry)) {
            prefs.setProperty(entry, properties[entry]);
          }
        }
        return prefs;
    }

    if (group == null)
      return this._selectGlobalPrefs();
    return this._selectPrefs(group);
  },

  getPrefsByName: function ContentPrefService_getPrefsByName(aName) {
    if (!aName)
      throw Components.Exception("aName cannot be null or an empty string",
                                 Cr.NS_ERROR_ILLEGAL_VALUE);

    if (this._inPrivateBrowsing) {
      let prefs = Cc["@mozilla.org/hash-property-bag;1"].
                  createInstance(Ci.nsIWritablePropertyBag);
      let groupNames = this._privModeStorage.groupsForName(aName);
      for (var i = 0; i < groupNames.length; i++) {
        let groupName = groupNames[i];
        prefs.setProperty(groupName,
                          this._privModeStorage.getPref(aName, groupName)[1]);
      }
      return prefs;
    }

    return this._selectPrefsByName(aName);
  },

  
  _inPrivateBrowsing: false,

  
  _observers: {},

  
  _genericObservers: [],

  addObserver: function ContentPrefService_addObserver(aName, aObserver) {
    var observers;
    if (aName) {
      if (!this._observers[aName])
        this._observers[aName] = [];
      observers = this._observers[aName];
    }
    else
      observers = this._genericObservers;

    if (observers.indexOf(aObserver) == -1)
      observers.push(aObserver);
  },

  removeObserver: function ContentPrefService_removeObserver(aName, aObserver) {
    var observers;
    if (aName) {
      if (!this._observers[aName])
        return;
      observers = this._observers[aName];
    }
    else
      observers = this._genericObservers;

    if (observers.indexOf(aObserver) != -1)
      observers.splice(observers.indexOf(aObserver), 1);
  },

  






  _getObservers: function ContentPrefService__getObservers(aName) {
    var observers = [];

    if (aName && this._observers[aName])
      observers = observers.concat(this._observers[aName]);
    observers = observers.concat(this._genericObservers);

    return observers;
  },
  
  _notifyPrefRemoved: function ContentPrefService__notifyPrefRemoved(aGroup, aName) {
    for each (var observer in this._getObservers(aName)) {
      try {
        observer.onContentPrefRemoved(aGroup, aName);
      }
      catch(ex) {
        Cu.reportError(ex);
      }
    }
  },

  _notifyPrefSet: function ContentPrefService__notifyPrefSet(aGroup, aName, aValue) {
    for each (var observer in this._getObservers(aName)) {
      try {
        observer.onContentPrefSet(aGroup, aName, aValue);
      }
      catch(ex) {
        Cu.reportError(ex);
      }
    }
  },

  _grouper: null,
  get grouper() {
    if (!this._grouper)
      this._grouper = Cc["@mozilla.org/content-pref/hostname-grouper;1"].
                      getService(Ci.nsIContentURIGrouper);
    return this._grouper;
  },

  get DBConnection() {
    return this._dbConnection;
  },


  
  

  __stmtSelectPref: null,
  get _stmtSelectPref() {
    if (!this.__stmtSelectPref)
      this.__stmtSelectPref = this._dbCreateStatement(
        "SELECT prefs.value AS value " +
        "FROM prefs " +
        "JOIN groups ON prefs.groupID = groups.id " +
        "JOIN settings ON prefs.settingID = settings.id " +
        "WHERE groups.name = :group " +
        "AND settings.name = :setting"
      );

    return this.__stmtSelectPref;
  },

  _scheduleCallback: function(func) {
    let tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
    tm.mainThread.dispatch(func, Ci.nsIThread.DISPATCH_NORMAL);
  },

  _selectPref: function ContentPrefService__selectPref(aGroup, aSetting, aCallback) {
    let [cached, value] = this._cache.getPref(aSetting, aGroup);
    if (cached) {
      if (aCallback) {
        this._scheduleCallback(function(){aCallback.onResult(value);});
        return;
      }
      return value;
    }

    try {
      this._stmtSelectPref.params.group = aGroup;
      this._stmtSelectPref.params.setting = aSetting;

      if (aCallback) {
        let cache = this._cache;
        new AsyncStatement(this._stmtSelectPref).execute({onResult: function(aResult) {
          cache.cachePref(aSetting, aResult, aGroup);
          aCallback.onResult(aResult);
        }});
      }
      else {
        if (this._stmtSelectPref.executeStep()) {
          value = this._stmtSelectPref.row["value"];
        }
        this._cache.cachePref(aSetting, value, aGroup);
      }
    }
    finally {
      this._stmtSelectPref.reset();
    }

    return value;
  },

  __stmtSelectGlobalPref: null,
  get _stmtSelectGlobalPref() {
    if (!this.__stmtSelectGlobalPref)
      this.__stmtSelectGlobalPref = this._dbCreateStatement(
        "SELECT prefs.value AS value " +
        "FROM prefs " +
        "JOIN settings ON prefs.settingID = settings.id " +
        "WHERE prefs.groupID IS NULL " +
        "AND settings.name = :name"
      );

    return this.__stmtSelectGlobalPref;
  },

  _selectGlobalPref: function ContentPrefService__selectGlobalPref(aName, aCallback) {
    let [cached, value] = this._cache.getPref(aName, null);
    if (cached) {
      if (aCallback) {
        this._scheduleCallback(function(){aCallback.onResult(value);});
        return;
      }
      return value;
    }

    try {
      this._stmtSelectGlobalPref.params.name = aName;

      if (aCallback) {
        let cache = this._cache;
        new AsyncStatement(this._stmtSelectGlobalPref).execute({onResult: function(aResult) {
          cache.cachePref(aName, aResult);
          aCallback.onResult(aResult);
        }});
      }
      else {
        if (this._stmtSelectGlobalPref.executeStep()) {
          value = this._stmtSelectGlobalPref.row["value"];
        }
        this._cache.cachePref(aName, value);
      }
    }
    finally {
      this._stmtSelectGlobalPref.reset();
    }

    return value;
  },

  __stmtSelectGroupID: null,
  get _stmtSelectGroupID() {
    if (!this.__stmtSelectGroupID)
      this.__stmtSelectGroupID = this._dbCreateStatement(
        "SELECT groups.id AS id " +
        "FROM groups " +
        "WHERE groups.name = :name "
      );

    return this.__stmtSelectGroupID;
  },

  _selectGroupID: function ContentPrefService__selectGroupID(aName) {
    var id;

    try {
      this._stmtSelectGroupID.params.name = aName;

      if (this._stmtSelectGroupID.executeStep())
        id = this._stmtSelectGroupID.row["id"];
    }
    finally {
      this._stmtSelectGroupID.reset();
    }

    return id;
  },

  __stmtInsertGroup: null,
  get _stmtInsertGroup() {
    if (!this.__stmtInsertGroup)
      this.__stmtInsertGroup = this._dbCreateStatement(
        "INSERT INTO groups (name) VALUES (:name)"
      );

    return this.__stmtInsertGroup;
  },

  _insertGroup: function ContentPrefService__insertGroup(aName) {
    this._stmtInsertGroup.params.name = aName;
    this._stmtInsertGroup.execute();
    return this._dbConnection.lastInsertRowID;
  },

  __stmtSelectSettingID: null,
  get _stmtSelectSettingID() {
    if (!this.__stmtSelectSettingID)
      this.__stmtSelectSettingID = this._dbCreateStatement(
        "SELECT id FROM settings WHERE name = :name"
      );

    return this.__stmtSelectSettingID;
  },

  _selectSettingID: function ContentPrefService__selectSettingID(aName) {
    var id;

    try {
      this._stmtSelectSettingID.params.name = aName;

      if (this._stmtSelectSettingID.executeStep())
        id = this._stmtSelectSettingID.row["id"];
    }
    finally {
      this._stmtSelectSettingID.reset();
    }

    return id;
  },

  __stmtInsertSetting: null,
  get _stmtInsertSetting() {
    if (!this.__stmtInsertSetting)
      this.__stmtInsertSetting = this._dbCreateStatement(
        "INSERT INTO settings (name) VALUES (:name)"
      );

    return this.__stmtInsertSetting;
  },

  _insertSetting: function ContentPrefService__insertSetting(aName) {
    this._stmtInsertSetting.params.name = aName;
    this._stmtInsertSetting.execute();
    return this._dbConnection.lastInsertRowID;
  },

  __stmtSelectPrefID: null,
  get _stmtSelectPrefID() {
    if (!this.__stmtSelectPrefID)
      this.__stmtSelectPrefID = this._dbCreateStatement(
        "SELECT id FROM prefs WHERE groupID = :groupID AND settingID = :settingID"
      );

    return this.__stmtSelectPrefID;
  },

  _selectPrefID: function ContentPrefService__selectPrefID(aGroupID, aSettingID) {
    var id;

    try {
      this._stmtSelectPrefID.params.groupID = aGroupID;
      this._stmtSelectPrefID.params.settingID = aSettingID;

      if (this._stmtSelectPrefID.executeStep())
        id = this._stmtSelectPrefID.row["id"];
    }
    finally {
      this._stmtSelectPrefID.reset();
    }

    return id;
  },

  __stmtSelectGlobalPrefID: null,
  get _stmtSelectGlobalPrefID() {
    if (!this.__stmtSelectGlobalPrefID)
      this.__stmtSelectGlobalPrefID = this._dbCreateStatement(
        "SELECT id FROM prefs WHERE groupID IS NULL AND settingID = :settingID"
      );

    return this.__stmtSelectGlobalPrefID;
  },

  _selectGlobalPrefID: function ContentPrefService__selectGlobalPrefID(aSettingID) {
    var id;

    try {
      this._stmtSelectGlobalPrefID.params.settingID = aSettingID;

      if (this._stmtSelectGlobalPrefID.executeStep())
        id = this._stmtSelectGlobalPrefID.row["id"];
    }
    finally {
      this._stmtSelectGlobalPrefID.reset();
    }

    return id;
  },

  __stmtInsertPref: null,
  get _stmtInsertPref() {
    if (!this.__stmtInsertPref)
      this.__stmtInsertPref = this._dbCreateStatement(
        "INSERT INTO prefs (groupID, settingID, value) " +
        "VALUES (:groupID, :settingID, :value)"
      );

    return this.__stmtInsertPref;
  },

  _insertPref: function ContentPrefService__insertPref(aGroupID, aSettingID, aValue) {
    this._stmtInsertPref.params.groupID = aGroupID;
    this._stmtInsertPref.params.settingID = aSettingID;
    this._stmtInsertPref.params.value = aValue;
    this._stmtInsertPref.execute();
    return this._dbConnection.lastInsertRowID;
  },

  __stmtUpdatePref: null,
  get _stmtUpdatePref() {
    if (!this.__stmtUpdatePref)
      this.__stmtUpdatePref = this._dbCreateStatement(
        "UPDATE prefs SET value = :value WHERE id = :id"
      );

    return this.__stmtUpdatePref;
  },

  _updatePref: function ContentPrefService__updatePref(aPrefID, aValue) {
    this._stmtUpdatePref.params.id = aPrefID;
    this._stmtUpdatePref.params.value = aValue;
    this._stmtUpdatePref.execute();
  },

  __stmtDeletePref: null,
  get _stmtDeletePref() {
    if (!this.__stmtDeletePref)
      this.__stmtDeletePref = this._dbCreateStatement(
        "DELETE FROM prefs WHERE id = :id"
      );

    return this.__stmtDeletePref;
  },

  _deletePref: function ContentPrefService__deletePref(aPrefID) {
    this._stmtDeletePref.params.id = aPrefID;
    this._stmtDeletePref.execute();
  },

  __stmtDeleteSettingIfUnused: null,
  get _stmtDeleteSettingIfUnused() {
    if (!this.__stmtDeleteSettingIfUnused)
      this.__stmtDeleteSettingIfUnused = this._dbCreateStatement(
        "DELETE FROM settings WHERE id = :id " +
        "AND id NOT IN (SELECT DISTINCT settingID FROM prefs)"
      );

    return this.__stmtDeleteSettingIfUnused;
  },

  _deleteSettingIfUnused: function ContentPrefService__deleteSettingIfUnused(aSettingID) {
    this._stmtDeleteSettingIfUnused.params.id = aSettingID;
    this._stmtDeleteSettingIfUnused.execute();
  },

  __stmtDeleteGroupIfUnused: null,
  get _stmtDeleteGroupIfUnused() {
    if (!this.__stmtDeleteGroupIfUnused)
      this.__stmtDeleteGroupIfUnused = this._dbCreateStatement(
        "DELETE FROM groups WHERE id = :id " +
        "AND id NOT IN (SELECT DISTINCT groupID FROM prefs)"
      );

    return this.__stmtDeleteGroupIfUnused;
  },

  _deleteGroupIfUnused: function ContentPrefService__deleteGroupIfUnused(aGroupID) {
    this._stmtDeleteGroupIfUnused.params.id = aGroupID;
    this._stmtDeleteGroupIfUnused.execute();
  },

  __stmtSelectPrefs: null,
  get _stmtSelectPrefs() {
    if (!this.__stmtSelectPrefs)
      this.__stmtSelectPrefs = this._dbCreateStatement(
        "SELECT settings.name AS name, prefs.value AS value " +
        "FROM prefs " +
        "JOIN groups ON prefs.groupID = groups.id " +
        "JOIN settings ON prefs.settingID = settings.id " +
        "WHERE groups.name = :group "
      );

    return this.__stmtSelectPrefs;
  },

  _selectPrefs: function ContentPrefService__selectPrefs(aGroup) {
    var prefs = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag);

    try {
      this._stmtSelectPrefs.params.group = aGroup;

      while (this._stmtSelectPrefs.executeStep())
        prefs.setProperty(this._stmtSelectPrefs.row["name"],
                          this._stmtSelectPrefs.row["value"]);
    }
    finally {
      this._stmtSelectPrefs.reset();
    }

    return prefs;
  },

  __stmtSelectGlobalPrefs: null,
  get _stmtSelectGlobalPrefs() {
    if (!this.__stmtSelectGlobalPrefs)
      this.__stmtSelectGlobalPrefs = this._dbCreateStatement(
        "SELECT settings.name AS name, prefs.value AS value " +
        "FROM prefs " +
        "JOIN settings ON prefs.settingID = settings.id " +
        "WHERE prefs.groupID IS NULL"
      );

    return this.__stmtSelectGlobalPrefs;
  },

  _selectGlobalPrefs: function ContentPrefService__selectGlobalPrefs() {
    var prefs = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag);

    try {
      while (this._stmtSelectGlobalPrefs.executeStep())
        prefs.setProperty(this._stmtSelectGlobalPrefs.row["name"],
                          this._stmtSelectGlobalPrefs.row["value"]);
    }
    finally {
      this._stmtSelectGlobalPrefs.reset();
    }

    return prefs;
  },

  __stmtSelectPrefsByName: null,
  get _stmtSelectPrefsByName() {
    if (!this.__stmtSelectPrefsByName)
      this.__stmtSelectPrefsByName = this._dbCreateStatement(
        "SELECT groups.name AS groupName, prefs.value AS value " +
        "FROM prefs " +
        "JOIN groups ON prefs.groupID = groups.id " +
        "JOIN settings ON prefs.settingID = settings.id " +
        "WHERE settings.name = :setting "
      );

    return this.__stmtSelectPrefsByName;
  },

  _selectPrefsByName: function ContentPrefService__selectPrefsByName(aName) {
    var prefs = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag);

    try {
      this._stmtSelectPrefsByName.params.setting = aName;

      while (this._stmtSelectPrefsByName.executeStep())
        prefs.setProperty(this._stmtSelectPrefsByName.row["groupName"],
                          this._stmtSelectPrefsByName.row["value"]);
    }
    finally {
      this._stmtSelectPrefsByName.reset();
    }
    
    var global = this._selectGlobalPref(aName);
    if (typeof global != "undefined") {
      prefs.setProperty(null, global);
    }

    return prefs;
  },


  
  

  _dbVersion: 3,

  _dbSchema: {
    tables: {
      groups:     "id           INTEGER PRIMARY KEY, \
                   name         TEXT NOT NULL",
  
      settings:   "id           INTEGER PRIMARY KEY, \
                   name         TEXT NOT NULL",
  
      prefs:      "id           INTEGER PRIMARY KEY, \
                   groupID      INTEGER REFERENCES groups(id), \
                   settingID    INTEGER NOT NULL REFERENCES settings(id), \
                   value        BLOB"
    },
    indices: {
      groups_idx: {
        table: "groups",
        columns: ["name"]
      },
      settings_idx: {
        table: "settings",
        columns: ["name"]
      },
      prefs_idx: {
        table: "prefs",
        columns: ["groupID", "settingID"]
      }
    }
  },

  _dbConnection: null,

  _dbCreateStatement: function ContentPrefService__dbCreateStatement(aSQLString) {
    try {
      var statement = this._dbConnection.createStatement(aSQLString);
    }
    catch(ex) {
      Cu.reportError("error creating statement " + aSQLString + ": " +
                     this._dbConnection.lastError + " - " +
                     this._dbConnection.lastErrorString);
      throw ex;
    }

    return statement;
  },

  
  
  
  

  _dbInit: function ContentPrefService__dbInit() {
    var dirService = Cc["@mozilla.org/file/directory_service;1"].
                     getService(Ci.nsIProperties);
    var dbFile = dirService.get("ProfD", Ci.nsIFile);
    dbFile.append("content-prefs.sqlite");

    var dbService = Cc["@mozilla.org/storage/service;1"].
                    getService(Ci.mozIStorageService);

    var dbConnection;

    if (!dbFile.exists())
      dbConnection = this._dbCreate(dbService, dbFile);
    else {
      try {
        dbConnection = dbService.openDatabase(dbFile);
      }
      
      
      catch (e if e.result == Cr.NS_ERROR_FILE_CORRUPTED) {
        dbConnection = this._dbBackUpAndRecreate(dbService, dbFile,
                                                 dbConnection);
      }

      
      var version = dbConnection.schemaVersion;

      
      
      if (version != this._dbVersion) {
        try {
          this._dbMigrate(dbConnection, version, this._dbVersion);
        }
        catch(ex) {
          Cu.reportError("error migrating DB: " + ex + "; backing up and recreating");
          dbConnection = this._dbBackUpAndRecreate(dbService, dbFile, dbConnection);
        }
      }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    if (!this._prefSvc.prefHasUserValue("toolkit.storage.synchronous"))
      dbConnection.executeSimpleSQL("PRAGMA synchronous = OFF");

    this._dbConnection = dbConnection;
  },

  _dbCreate: function ContentPrefService__dbCreate(aDBService, aDBFile) {
    var dbConnection = aDBService.openDatabase(aDBFile);

    try {
      this._dbCreateSchema(dbConnection);
      dbConnection.schemaVersion = this._dbVersion;
    }
    catch(ex) {
      
      
      
      dbConnection.close();
      aDBFile.remove(false);
      throw ex;
    }

    return dbConnection;
  },

  _dbCreateSchema: function ContentPrefService__dbCreateSchema(aDBConnection) {
    this._dbCreateTables(aDBConnection);
    this._dbCreateIndices(aDBConnection);
  },

  _dbCreateTables: function ContentPrefService__dbCreateTables(aDBConnection) {
    for (let name in this._dbSchema.tables)
      aDBConnection.createTable(name, this._dbSchema.tables[name]);
  },

  _dbCreateIndices: function ContentPrefService__dbCreateIndices(aDBConnection) {
    for (let name in this._dbSchema.indices) {
      let index = this._dbSchema.indices[name];
      let statement = "CREATE INDEX IF NOT EXISTS " + name + " ON " + index.table +
                      "(" + index.columns.join(", ") + ")";
      aDBConnection.executeSimpleSQL(statement);
    }
  },

  _dbBackUpAndRecreate: function ContentPrefService__dbBackUpAndRecreate(aDBService,
                                                                         aDBFile,
                                                                         aDBConnection) {
    aDBService.backupDatabaseFile(aDBFile, "content-prefs.sqlite.corrupt");

    
    
    
    try { aDBConnection.close() } catch(ex) {}

    aDBFile.remove(false);

    let dbConnection = this._dbCreate(aDBService, aDBFile);

    return dbConnection;
  },

  _dbMigrate: function ContentPrefService__dbMigrate(aDBConnection, aOldVersion, aNewVersion) {
    if (this["_dbMigrate" + aOldVersion + "To" + aNewVersion]) {
      aDBConnection.beginTransaction();
      try {
        this["_dbMigrate" + aOldVersion + "To" + aNewVersion](aDBConnection);
        aDBConnection.schemaVersion = aNewVersion;
        aDBConnection.commitTransaction();
      }
      catch(ex) {
        aDBConnection.rollbackTransaction();
        throw ex;
      }
    }
    else
      throw("no migrator function from version " + aOldVersion +
            " to version " + aNewVersion);
  },

  






  _dbMigrate0To3: function ContentPrefService___dbMigrate0To3(aDBConnection) {
    this._dbCreateSchema(aDBConnection);
  },

  _dbMigrate1To3: function ContentPrefService___dbMigrate1To3(aDBConnection) {
    aDBConnection.executeSimpleSQL("ALTER TABLE groups RENAME TO groupsOld");
    aDBConnection.createTable("groups", this._dbSchema.tables.groups);
    aDBConnection.executeSimpleSQL(
      "INSERT INTO groups (id, name) " +
      "SELECT id, name FROM groupsOld"
    );

    aDBConnection.executeSimpleSQL("DROP TABLE groupers");
    aDBConnection.executeSimpleSQL("DROP TABLE groupsOld");

    this._dbCreateIndices(aDBConnection);
  },

  _dbMigrate2To3: function ContentPrefService__dbMigrate2To3(aDBConnection) {
    this._dbCreateIndices(aDBConnection);
  },

  _parseGroupParam: function ContentPrefService__parseGroupParam(aGroup) {
    if (aGroup == null)
      return null;
    if (aGroup.constructor.name == "String")
      return aGroup.toString();
    if (aGroup instanceof Ci.nsIURI)
      return this.grouper.group(aGroup);

    throw Components.Exception("aGroup is not a string, nsIURI or null",
                               Cr.NS_ERROR_ILLEGAL_VALUE);
  },
};


function HostnameGrouper() {}

HostnameGrouper.prototype = {
  
  
  
  classID:          Components.ID("{8df290ae-dcaa-4c11-98a5-2429a4dc97bb}"),
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsIContentURIGrouper]),

  
  

  group: function HostnameGrouper_group(aURI) {
    var group;

    try {
      
      
      
      

      group = aURI.host;
      if (!group)
        throw("can't derive group from host; no host in URI");
    }
    catch(ex) {
      
      
      
      
      
      
      
      
      

      

      try {
        var url = aURI.QueryInterface(Ci.nsIURL);
        group = aURI.prePath + url.filePath;
      }
      catch(ex) {
        group = aURI.spec;
      }
    }

    return group;
  }
};

function AsyncStatement(aStatement) {
  this.stmt = aStatement;
}

AsyncStatement.prototype = {
  execute: function AsyncStmt_execute(aCallback) {
    let stmt = this.stmt;
    stmt.executeAsync({
      _callback: aCallback,
      _hadResult: false,
      handleResult: function(aResult) {
        this._hadResult = true;
        if (this._callback) {
          let row = aResult.getNextRow();
          this._callback.onResult(row.getResultByName("value"));
        }
      },
      handleCompletion: function(aReason) {
        if (!this._hadResult && this._callback &&
            aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED)
          this._callback.onResult(undefined);
      },
      handleError: function(aError) {}
    });
  }
};




var components = [ContentPrefService, HostnameGrouper];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
