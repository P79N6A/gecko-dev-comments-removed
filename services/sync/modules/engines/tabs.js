




































const EXPORTED_SYMBOLS = ['TabEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/stores.js");
Cu.import("resource://services-sync/trackers.js");
Cu.import("resource://services-sync/type_records/tabs.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/ext/Preferences.js");






const PBPrefs = new Preferences("browser.privatebrowsing.");

function TabEngine() {
  SyncEngine.call(this, "Tabs");

  
  this._resetClient();
}
TabEngine.prototype = {
  __proto__: SyncEngine.prototype,
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

  








  locallyOpenTabMatchesURL: function TabEngine_localTabMatches(url) {
    return this._store.getAllTabs().some(function(tab) {
      return tab.urlHistory[0] == url;
    });
  }
};


function TabStore(name) {
  Store.call(this, name);
}
TabStore.prototype = {
  __proto__: Store.prototype,

  itemExists: function TabStore_itemExists(id) {
    return id == Clients.localID;
  },

  getAllTabs: function getAllTabs(filter) {
    let filteredUrls = new RegExp(Svc.Prefs.get("engine.tabs.filteredUrls"), "i");

    let allTabs = [];

    let currentState = JSON.parse(Svc.Session.getBrowserState());
    currentState.windows.forEach(function(window) {
      window.tabs.forEach(function(tab) {
        
        if (!tab.entries.length)
          return;
        
        
        let entry = tab.entries[tab.index - 1];

        
        
        if (!entry.url || filter && filteredUrls.test(entry.url))
          return;

        
        
        
        
        allTabs.push({
          title: entry.title || "",
          urlHistory: [entry.url],
          icon: tab.attributes && tab.attributes.image || "",
          lastUsed: tab.extData && tab.extData.weaveLastUsed || 0
        });
      });
    });

    return allTabs;
  },

  createRecord: function createRecord(guid) {
    let record = new TabSetRecord();
    record.clientName = Clients.localName;

    
    if (Svc.Private.privateBrowsingEnabled && !PBPrefs.get("autostart")) {
      record.tabs = [];
      return record;
    }

    
    let tabs = this.getAllTabs(true).sort(function(a, b) {
      return b.lastUsed - a.lastUsed;
    });

    
    
    let size = JSON.stringify(tabs).length;
    let origLength = tabs.length;
    const MAX_TAB_SIZE = 20000;
    if (size > MAX_TAB_SIZE) {
      
      let cutoff = Math.ceil(tabs.length * MAX_TAB_SIZE / size);
      tabs = tabs.slice(0, cutoff + 1);

      
      while (JSON.stringify(tabs).length > MAX_TAB_SIZE)
        tabs.pop();
    }

    this._log.trace("Created tabs " + tabs.length + " of " + origLength);
    tabs.forEach(function(tab) {
      this._log.trace("Wrapping tab: " + JSON.stringify(tab));
    }, this);

    record.tabs = tabs;
    return record;
  },

  getAllIDs: function TabStore_getAllIds() {
    
    let ids = {};
    if (Svc.Private.privateBrowsingEnabled && !PBPrefs.get("autostart"))
      return ids;

    ids[Clients.localID] = true;
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
  },

  update: function update(record) {
    this._log.trace("Ignoring tab updates as local ones win");
  }
};


function TabTracker(name) {
  Tracker.call(this, name);

  Svc.Obs.add("private-browsing", this);

  
  this.onTab = Utils.bind2(this, this.onTab);

  
  Svc.WinWatcher.registerNotification(this);

  
  let wins = Svc.WinMediator.getEnumerator("navigator:browser");
  while (wins.hasMoreElements())
    this._registerListenersForWindow(wins.getNext());
}
TabTracker.prototype = {
  __proto__: Tracker.prototype,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  _registerListenersForWindow: function TabTracker__registerListen(window) {
    this._log.trace("Registering tab listeners in new window");

    
    let topics = ["pageshow", "TabOpen", "TabClose", "TabSelect"];
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
    else if (aTopic == "private-browsing" && aData == "enter"
             && !PBPrefs.get("autostart"))
      this.clearChangedIDs();
  },

  onTab: function onTab(event) {
    if (Svc.Private.privateBrowsingEnabled && !PBPrefs.get("autostart")) {
      this._log.trace("Ignoring tab event from private browsing.");
      return;
    }

    this._log.trace("onTab event: " + event.type);
    this.addChangedID(Clients.localID);

    
    let chance = .1;

    
    if (event.type != "pageshow") {
      chance = 1;

      
      Svc.Session.setTabValue(event.originalTarget, "weaveLastUsed",
                              Math.floor(Date.now() / 1000));
    }

    
    if (Math.random() < chance)
      this.score++;
  },
}
