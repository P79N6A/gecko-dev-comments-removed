




































const EXPORTED_SYMBOLS = ['TabEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/type_records/tabs.js");
Cu.import("resource://weave/engines/clientData.js");

function TabEngine() {
  this._init();
}
TabEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "tabs",
  _displayName: "Tabs",
  description: "Access tabs from other devices via the History menu",
  logName: "Tabs",
  _storeObj: TabStore,
  _trackerObj: TabTracker,
  _recordObj: TabSetRecord,

  
  getAllClients: function TabEngine_getAllClients() {
    return this._store._remoteClients;
  },

  getClientById: function TabEngine_getClientById(id) {
    return this._store._remoteClients[id];
  },

  _resetClient: function TabEngine__resetClient() {
    this.resetLastSync();
    this._store.wipe();
  },

  _syncFinish: function _syncFinish() {
    SyncEngine.prototype._syncFinish.call(this);
    this._tracker.resetChanged();
  },

  








  locallyOpenTabMatchesURL: function TabEngine_localTabMatches(url) {
    
    

    if (Cc["@mozilla.org/browser/sessionstore;1"])  {
      let state = this._store._sessionStore.getBrowserState();
      let session = JSON.parse(state);
      for (let i = 0; i < session.windows.length; i++) {
        let window = session.windows[i];
        for (let j = 0; j < window.tabs.length; j++) {
          let tab = window.tabs[j];
          if (tab.entries.length > 0) {
            let tabUrl = tab.entries[tab.entries.length-1].url;
            if (tabUrl == url) {
              return true;
            }
          }
        }
      }
    } else {
      let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
	.getService(Ci.nsIWindowMediator);
      let browserWindow = wm.getMostRecentWindow("navigator:browser");
      for each (let tab in browserWindow.Browser._tabs ) {
        let tabUrl = tab.browser.contentWindow.location.toString();
        if (tabUrl == url) {
          return true;
        }
      }
    }
    return false;
  }
};


function TabStore() {
  this._TabStore_init();
}
TabStore.prototype = {
  __proto__: Store.prototype,
  name: "tabs",
  _logName: "Tabs.Store",
  _filePath: "meta/tabSets",
  _remoteClients: {},

  _TabStore_init: function TabStore__init() {
    this._init();
    this._readFromFile();
  },

  get _localClientName() {
    return Clients.clientName;
  },

  _writeToFile: function TabStore_writeToFile() {
    let json = {};
    for (let [id, val] in Iterator(this._remoteClients))
      json[id] = val.toJson();

    Utils.jsonSave(this._filePath, this, json);
  },

  _readFromFile: function TabStore_readFromFile() {
    Utils.jsonLoad(this._filePath, this, function(json) {
      for (let [id, val] in Iterator(json)) {
	this._remoteClients[id] = new TabSetRecord();
	this._remoteClients[id].fromJson(val);
	this._remoteClients[id].id = id;
      }
    });
  },

  get _sessionStore() {
    let sessionStore = Cc["@mozilla.org/browser/sessionstore;1"].
		       getService(Ci.nsISessionStore);
    this.__defineGetter__("_sessionStore", function() { return sessionStore;});
    return this._sessionStore;
  },

  get _windowMediator() {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
                 .getService(Ci.nsIWindowMediator);
    this.__defineGetter__("_windowMediator", function() { return wm;});
    return this._windowMediator;
  },

  _createLocalClientTabSetRecord: function TabStore__createLocalTabSet() {
    
    
    let record = new TabSetRecord();
    record.setClientName( this._localClientName );

    if (Cc["@mozilla.org/browser/sessionstore;1"])  {
      this._addFirefoxTabsToRecord(record);
    } else {
      this._addFennecTabsToRecord(record);
    }
    return record;
  },

  _addFirefoxTabsToRecord: function TabStore__addFirefoxTabs(record) {
    
    let enumerator = this._windowMediator.getEnumerator("navigator:browser");
    while (enumerator.hasMoreElements()) {
      let window = enumerator.getNext();
      let tabContainer = window.getBrowser().tabContainer;

      
      for each (let tab in Array.slice(tabContainer.childNodes)) {
        if (!(tab instanceof Ci.nsIDOMNode))
          continue;

        let tabState = JSON.parse(this._sessionStore.getTabState(tab));
	
	if (tabState.entries.length == 0)
	  continue;

        
        let lastUsedTimestamp = tab.lastUsed;

        
	let currentPage = tabState.entries[tabState.entries.length - 1];
	



        
        let urlHistory = [];
	
	for (let i = tabState.entries.length -1; i >= 0; i--) {
          let entry = tabState.entries[i];
	  if (entry && entry.url)
	    urlHistory.push(entry.url);
	  if (urlHistory.length >= 10)
	    break;
	}

        
        this._log.debug("Wrapping a tab with title " + currentPage.title);
        this._log.trace("And timestamp " + lastUsedTimestamp);
        record.addTab(currentPage.title, urlHistory, lastUsedTimestamp);
      }
    }
  },

  _addFennecTabsToRecord: function TabStore__addFennecTabs(record) {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
	       .getService(Ci.nsIWindowMediator);
    let browserWindow = wm.getMostRecentWindow("navigator:browser");
    for each (let tab in browserWindow.Browser._tabs ) {
      let title = tab.browser.contentDocument.title;
      let url = tab.browser.contentWindow.location.toString();
      let urlHistory = [url];

      
      

      
      

      
      let lastUsedTimestamp = "0";

      this._log.debug("Wrapping a tab with title " + title);
      this._log.trace("And timestamp " + lastUsedTimestamp);
      record.addTab(title, urlHistory, lastUsedTimestamp);
      
    }
  },

  itemExists: function TabStore_itemExists(id) {
    return id == Clients.clientID;
  },

  createRecord: function TabStore_createRecord(id, cryptoMetaURL) {
    let record = this._createLocalClientTabSetRecord();
    record.id = id;
    record.encryption = cryptoMetaURL;
    return record;
  },

  getAllIDs: function TabStore_getAllIds() {
    let ids = {};
    ids[Clients.clientID] = true;
    return ids;
  },

  wipe: function TabStore_wipe() {
    this._remoteClients = {};
  },

  create: function TabStore_create(record) {
    this._log.debug("Adding remote tabs from " + record.getClientName());
    this._remoteClients[record.id] = record;
    this._writeToFile();
    
    
    
  }
};


function TabTracker() {
  this._TabTracker_init();
}
TabTracker.prototype = {
  __proto__: Tracker.prototype,
  name: "tabs",
  _logName: "TabTracker",
  file: "tab_tracker",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  _TabTracker_init: function TabTracker__init() {
    this._init();
    this.resetChanged();

    
    this.onTab = Utils.bind2(this, this.onTab);

    
    Svc.WinWatcher.registerNotification(this);

    
    let wins = Svc.WinMediator.getEnumerator("navigator:browser");
    while (wins.hasMoreElements())
      this._registerListenersForWindow(wins.getNext());
  },

  _registerListenersForWindow: function TabTracker__registerListen(window) {
    this._log.trace("Registering tab listeners in new window");

    
    let topics = ["TabOpen", "TabClose", "TabSelect"];
    let onTab = this.onTab;
    let addRem = function(add) topics.forEach(function(topic) {
      window[(add ? "add" : "remove") + "EventListener"](topic, onTab, false);
    });

    
    addRem(true);
    window.addEventListener("unload", function() addRem(false), false);
  },

  observe: function TabTracker_observe(aSubject, aTopic, aData) {
    
    let window = aSubject.QueryInterface(Ci.nsIDOMWindow);
    if (aTopic == "domwindowopened")
      this._registerListenersForWindow(window);
  },

  onTab: function onTab(event) {
    this._log.trace(event.type);
    this.score += 1;
    this._changedIDs[Clients.clientID] = true;

    
    event.originalTarget.lastUsed = Math.floor(Date.now() / 1000);
  },

  get changedIDs() this._changedIDs,

  
  resetChanged: function resetChanged() this._changedIDs = {}
}
