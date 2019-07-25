




































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
  SyncEngine.call(this);

  
  this._resetClient();
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
    SyncEngine.prototype._resetClient.call(this);
    this._store.wipe();
  },

  _syncFinish: function _syncFinish() {
    SyncEngine.prototype._syncFinish.call(this);
    this._tracker.resetChanged();
  },

  








  locallyOpenTabMatchesURL: function TabEngine_localTabMatches(url) {
    return this._store.getAllTabs().some(function(tab) {
      return tab.urlHistory[0] == url;
    });
  }
};


function TabStore() {
  Store.call(this);
}
TabStore.prototype = {
  __proto__: Store.prototype,
  name: "tabs",
  _logName: "Tabs.Store",
  _remoteClients: {},

  get _sessionStore() {
    let sessionStore = Cc["@mozilla.org/browser/sessionstore;1"].
		       getService(Ci.nsISessionStore);
    this.__defineGetter__("_sessionStore", function() { return sessionStore;});
    return this._sessionStore;
  },

  itemExists: function TabStore_itemExists(id) {
    return id == Clients.clientID;
  },


  getAllTabs: function getAllTabs(filter) {
    let filteredUrls = new RegExp(Svc.Prefs.get("engine.tabs.filteredUrls"), "i");

    
    let allTabs = [];
    let wins = Svc.WinMediator.getEnumerator("navigator:browser");
    while (wins.hasMoreElements()) {
      
      let window = wins.getNext();
      let tabs = window.gBrowser && window.gBrowser.tabContainer.childNodes;
      tabs = tabs || window.Browser._tabs;
  
      
      Array.forEach(tabs, function(tab) {
        let browser = tab.linkedBrowser || tab.browser;
        let url = browser.currentURI.spec;

        
        if (filter && filteredUrls.test(url))
          return;

        allTabs.push({
          title: browser.contentTitle || "",
          urlHistory: [url],
          icon: browser.mIconURL || "",
          lastUsed: tab.lastUsed || 0
        });
      });
    }
  
    return allTabs;
  },

  createRecord: function TabStore_createRecord(id, cryptoMetaURL) {
    let record = new TabSetRecord();
    record.clientName = Clients.clientName;

    
    record.tabs = this.getAllTabs(true).sort(function(a, b) {
      return b.lastUsed - a.lastUsed;
    }).slice(0, 25);
    record.tabs.forEach(function(tab) {
      this._log.trace("Wrapping tab: " + JSON.stringify(tab));
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

    
    let roundModify = Math.floor(record.modified / 1000);
    let notifyState = Svc.Prefs.get("notifyTabState");
    
    if (notifyState == null)
      Svc.Prefs.set("notifyTabState", roundModify);
    
    else if (notifyState == 0)
      return;
    
    else if (notifyState != roundModify)
      Svc.Prefs.set("notifyTabState", 0);
  }
};


function TabTracker() {
  Tracker.call(this);

  this.resetChanged();

  
  this.onTab = Utils.bind2(this, this.onTab);

  
  Svc.WinWatcher.registerNotification(this);

  
  let wins = Svc.WinMediator.getEnumerator("navigator:browser");
  while (wins.hasMoreElements())
    this._registerListenersForWindow(wins.getNext());
}
TabTracker.prototype = {
  __proto__: Tracker.prototype,
  name: "tabs",
  _logName: "TabTracker",
  file: "tab_tracker",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

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
    
    if (aTopic == "domwindowopened") {
      let self = this;
      aSubject.addEventListener("load", function onLoad(event) {
        aSubject.removeEventListener("load", onLoad, false);
        
        self._registerListenersForWindow(aSubject);
      }, false);
    }
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
