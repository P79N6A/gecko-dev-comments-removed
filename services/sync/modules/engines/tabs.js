



































const EXPORTED_SYMBOLS = ['TabEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/syncCores.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/constants.js");

Function.prototype.async = Async.sugar;

function TabEngine(pbeId) {
  this._init(pbeId);
}

TabEngine.prototype = {
  __proto__: new Engine(),

  get name() "tabs",
  get logName() "TabEngine",
  get serverPrefix() "user-data/tabs/",
  get store() this._store,

  get _store() {
    let store = new TabStore();
    this.__defineGetter__("_store", function() store);
    return this._store;
  },
  
  get _core() {
    let core = new TabSyncCore(this._store);
    this.__defineGetter__("_core", function() core);
    return this._core;
  },

  get _tracker() {
    let tracker = new TabTracker(this);
    this.__defineGetter__("_tracker", function() tracker);
    return this._tracker;
  }

};

function TabSyncCore(store) {
  this._store = store;
  this._init();
}
TabSyncCore.prototype = {
  __proto__: new SyncCore(),

  _logName: "TabSync",
  _store: null,

  get _sessionStore() {
    let sessionStore = Cc["@mozilla.org/browser/sessionstore;1"].
		       getService(Ci.nsISessionStore);
    this.__defineGetter__("_sessionStore", function() sessionStore);
    return this._sessionStore;
  },

  _commandLike: function TSC_commandLike(a, b) {
    
    return false;
  }
};

function TabStore() {
  this._virtualTabs = {};
  this._init();
}
TabStore.prototype = {
  __proto__: new Store(),

  _logName: "TabStore",

  get _sessionStore() {
    let sessionStore = Cc["@mozilla.org/browser/sessionstore;1"].
		       getService(Ci.nsISessionStore);
    this.__defineGetter__("_sessionStore", function() sessionStore);
    return this._sessionStore;
  },

  get _windowMediator() {
    let windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].
			 getService(Ci.nsIWindowMediator);
    this.__defineGetter__("_windowMediator", function() windowMediator);
    return this._windowMediator;
  },

  get _os() {
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    this.__defineGetter__("_os", function() os);
    return this._os;
  },

  get _dirSvc() {
    let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties);
    this.__defineGetter__("_dirSvc", function() dirSvc);
    return this._dirSvc;
  },

  










  _virtualTabs: null,

  get virtualTabs() {
    
    
    
    let realTabs = this._wrapRealTabs();
    let virtualTabsChanged = false;
    for (let id in this._virtualTabs) {
      if (id in realTabs) {
        this._log.warn("get virtualTabs: both real and virtual tabs exist for "
                       + id + "; removing virtual one");
        delete this._virtualTabs[id];
        virtualTabsChanged = true;
      }
    }
    if (virtualTabsChanged)
      this._saveVirtualTabs();

    return this._virtualTabs;
  },

  set virtualTabs(newValue) {
    this._virtualTabs = newValue;
    this._saveVirtualTabs();
  },

  
  get _file() {
    let file = this._dirSvc.get("ProfD", Ci.nsILocalFile);
    file.append("weave");
    file.append("store");
    file.append("tabs");
    file.append("virtual.json");
    this.__defineGetter__("_file", function() file);
    return this._file;
  },

  _saveVirtualTabs: function TabStore__saveVirtualTabs() {
    try {
      if (!this._file.exists())
        this._file.create(Ci.nsIFile.NORMAL_FILE_TYPE, PERMS_FILE);
      let out = this._json.encode(this._virtualTabs);
      let [fos] = Utils.open(this._file, ">");
      fos.writeString(out);
      fos.close();
    }
    catch(ex) {
      this._log.warn("could not serialize virtual tabs to disk: " + ex);
    }
  },

  _restoreVirtualTabs: function TabStore__restoreVirtualTabs() {
    try {
      if (this._file.exists()) {
        let [is] = Utils.open(this._file, "<");
        let json = Utils.readStream(is);
        is.close();
        this._virtualTabs = this._json.decode(json);
      }
    }
    catch (ex) {
      this._log.warn("could not parse virtual tabs from disk: " + ex);
    }
  },

  _init: function TabStore__init() {
    this._restoreVirtualTabs();

    this.__proto__.__proto__._init.call(this);
  },

  




  applyCommands: function TabStore_applyCommands(commandList) {
    let self = yield;

    this.__proto__.__proto__.applyCommands.async(this, self.cb, commandList);
    yield;

    this._saveVirtualTabs();

    self.done();
  },

  _itemExists: function TabStore__itemExists(GUID) {
    
    
    
    
    

    
    let tabs = this.wrap();

    
    
    if (GUID in tabs) {
      this._log.trace("_itemExists: " + GUID + " exists");
      return true;
    }

    this._log.trace("_itemExists: " + GUID + " doesn't exist");
    return false;
  },

  _createCommand: function TabStore__createCommand(command) {
    this._log.debug("_createCommand: " + command.GUID);

    if (command.GUID in this._virtualTabs || command.GUID in this._wrapRealTabs())
      throw "trying to create a tab that already exists; id: " + command.GUID;

    
    this._virtualTabs[command.GUID] = command.data;
    this._os.notifyObservers(null, "weave:store:tabs:virtual:created", null);
  },

  _removeCommand: function TabStore__removeCommand(command) {
    this._log.debug("_removeCommand: " + command.GUID);

    
    
    
    if (command.GUID in this._virtualTabs) {
      delete this._virtualTabs[command.GUID];
      this._os.notifyObservers(null, "weave:store:tabs:virtual:removed", null);
    }
  },

  _editCommand: function TabStore__editCommand(command) {
    this._log.debug("_editCommand: " + command.GUID);

    
    
    

    if (this._virtualTabs[command.GUID])
      this._virtualTabs[command.GUID] = command.data;
  },

  








  wrap: function TabStore_wrap() {
    let items;

    let virtualTabs = this._wrapVirtualTabs();
    let realTabs = this._wrapRealTabs();

    
    
    
    
    items = virtualTabs;
    let virtualTabsChanged = false;
    for (let id in realTabs) {
      
      
      
      
      if (this._virtualTabs[id]) {
        this._log.warn("wrap: both real and virtual tabs exist for " + id +
                       "; removing virtual one");
        delete this._virtualTabs[id];
        virtualTabsChanged = true;
      }

      items[id] = realTabs[id];
    }
    if (virtualTabsChanged)
      this._saveVirtualTabs();

    return items;
  },

  _wrapVirtualTabs: function TabStore__wrapVirtualTabs() {
    let items = {};

    for (let id in this._virtualTabs) {
      let virtualTab = this._virtualTabs[id];

      
      
      
      let item = {};
      for (let property in virtualTab)
        if (property[0] != "_")
          item[property] = virtualTab[property];

      items[id] = item;
    }

    return items;
  },

  _wrapRealTabs: function TabStore__wrapRealTabs() {
    let items = {};

    let session = this._json.decode(this._sessionStore.getBrowserState());

    for (let i = 0; i < session.windows.length; i++) {
      let window = session.windows[i];
      
      
      
      let windowID = i + 1;
      this._log.trace("_wrapRealTabs: window " + windowID);
      for (let j = 0; j < window.tabs.length; j++) {
        let tab = window.tabs[j];

	
	
	let currentEntry = tab.entries[tab.index - 1];

	if (!currentEntry || !currentEntry.url) {
	  this._log.warn("_wrapRealTabs: no current entry or no URL, can't " +
                         "identify " + this._json.encode(tab));
	  continue;
	}

	let tabID = currentEntry.url;
        this._log.trace("_wrapRealTabs: tab " + tabID);

        
        
        
        
        for each (let entry in tab.entries)
          delete entry.ID;

	items[tabID] = {
          
          
	  type: "tab",

          
          
          position: j + 1,

	  windowID: windowID,

	  state: tab
	};
      }
    }

    return items;
  },

  wipe: function TabStore_wipe() {
    
    
    this._virtualTabs = {};
    this._saveVirtualTabs();
  },

  _resetGUIDs: function TabStore__resetGUIDs() {
    let self = yield;
    
  }

};

function TabTracker(engine) {
  this._engine = engine;
  this._init();
}
TabTracker.prototype = {
  __proto__: new Tracker(),

  _logName: "TabTracker",

  _engine: null,

  get _json() {
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    this.__defineGetter__("_json", function() json);
    return this._json;
  },

  

























  get score() {
    
    
    let snapshotData = this._engine.snapshot.data;
    let a = {};

    
    let b = this._engine.store.wrap();

    
    
    
    let c = [];

    
    for (id in snapshotData) {
      if (id in b) {
        c.push(this._json.encode(snapshotData[id]) == this._json.encode(b[id]));
        delete b[id];
      }
      else {
        a[id] = snapshotData[id];
      }
    }

    let numShared = c.length;
    let numUnique = [true for (id in a)].length + [true for (id in b)].length;
    let numTotal = numShared + numUnique;

    
    
    
    if (numTotal == 0)
      return 0;

    
    let numChanged = c.filter(function(v) !v).length;

    let fractionSimilar = (numShared - (numChanged / 2)) / numTotal;
    let fractionDissimilar = 1 - fractionSimilar;
    let percentDissimilar = Math.round(fractionDissimilar * 100);

    return percentDissimilar;
  },

  resetScore: function FormsTracker_resetScore() {
    
  }
}
