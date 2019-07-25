




































const EXPORTED_SYMBOLS = ['TabEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
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

  _writeToFile: function TabStore_writeToFile() {
    Utils.jsonSave(this._filePath, this, this._remoteClients);
  },

  _readFromFile: function TabStore_readFromFile() {
    Utils.jsonLoad(this._filePath, this, function(json) {
      this._remoteClients = json;
    });
  },

  get _sessionStore() {
    let sessionStore = Cc["@mozilla.org/browser/sessionstore;1"].
		       getService(Ci.nsISessionStore);
    this.__defineGetter__("_sessionStore", function() { return sessionStore;});
    return this._sessionStore;
  },

  itemExists: function TabStore_itemExists(id) {
    return id == Clients.clientID;
  },

  createRecord: function TabStore_createRecord(id, cryptoMetaURL) {
    let record = new TabSetRecord();
    record.clientName = Clients.clientName;

    
    let allTabs = [];
    let wins = Svc.WinMediator.getEnumerator("navigator:browser");
    while (wins.hasMoreElements()) {
      
      let window = wins.getNext();
      let tabs = window.gBrowser && window.gBrowser.tabContainer.childNodes;
      tabs = tabs || window.Browser._tabs;
      Array.forEach(tabs, function(tab) {
        allTabs.push(tab);
      });
    }

    
    let tabData = allTabs.map(function(tab) {
      let browser = tab.linkedBrowser || tab.browser;
      return {
        title: browser.contentTitle || "",
        urlHistory: [browser.currentURI.spec],
        icon: browser.mIconURL || "",
        lastUsed: tab.lastUsed || 0
      };
    }).sort(function(a, b) b.lastUsed - a.lastUsed);

    
    record.tabs = tabData.slice(0, 25);
    record.tabs.forEach(function(tab) {
      this._log.debug("Wrapping tab: " + JSON.stringify(tab));
    }, this);

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
    this._log.debug("Adding remote tabs from " + record.clientName);
    this._remoteClients[record.id] = record.cleartext;
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
