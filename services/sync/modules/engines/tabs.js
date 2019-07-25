





































const EXPORTED_SYMBOLS = ['TabEngine', 'TabSetRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const TABS_TTL = 604800; 

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/ext/Preferences.js");






const PBPrefs = new Preferences("browser.privatebrowsing.");


function TabSetRecord(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
TabSetRecord.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Record.Tabs",
  ttl: TABS_TTL
};

Utils.deferGetSet(TabSetRecord, "cleartext", ["clientName", "tabs"]);


function TabEngine() {
  SyncEngine.call(this, "Tabs");

  
  this._resetClient();
}
TabEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: TabStore,
  _trackerObj: TabTracker,
  _recordObj: TabSetRecord,

  getChangedIDs: function getChangedIDs() {
    
    let changedIDs = {};
    if (this._tracker.modified)
      changedIDs[Clients.localID] = 0;
    return changedIDs;
  },

  
  getAllClients: function TabEngine_getAllClients() {
    return this._store._remoteClients;
  },

  getClientById: function TabEngine_getClientById(id) {
    return this._store._remoteClients[id];
  },

  _resetClient: function TabEngine__resetClient() {
    SyncEngine.prototype._resetClient.call(this);
    this._store.wipe();
    this._tracker.modified = true;
  },

  removeClientData: function removeClientData() {
    new Resource(this.engineURL + "/" + Clients.localID).delete();
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

  createRecord: function createRecord(id, collection) {
    let record = new TabSetRecord(collection, id);
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
  Svc.Obs.add("weave:engine:start-tracking", this);
  Svc.Obs.add("weave:engine:stop-tracking", this);

  
  this.onTab = Utils.bind2(this, this.onTab);
  this._unregisterListeners = Utils.bind2(this, this._unregisterListeners);
}
TabTracker.prototype = {
  __proto__: Tracker.prototype,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  loadChangedIDs: function loadChangedIDs() {
    
  },

  clearChangedIDs: function clearChangedIDs() {
    this.modified = false;
  },

  _topics: ["pageshow", "TabOpen", "TabClose", "TabSelect"],
  _registerListenersForWindow: function registerListenersFW(window) {
    this._log.trace("Registering tab listeners in window");
    for each (let topic in this._topics) {
      window.addEventListener(topic, this.onTab, false);
    }
    window.addEventListener("unload", this._unregisterListeners, false);
  },

  _unregisterListeners: function unregisterListeners(event) {
    this._unregisterListenersForWindow(event.target);
  },

  _unregisterListenersForWindow: function unregisterListenersFW(window) {
    this._log.trace("Removing tab listeners in window");
    window.removeEventListener("unload", this._unregisterListeners, false);
    for each (let topic in this._topics) {
      window.removeEventListener(topic, this.onTab, false);
    }
  },

  _enabled: false,
  observe: function TabTracker_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "weave:engine:start-tracking":
        if (!this._enabled) {
          Svc.Obs.add("private-browsing", this);
          Svc.Obs.add("domwindowopened", this);
          let wins = Svc.WinMediator.getEnumerator("navigator:browser");
          while (wins.hasMoreElements())
            this._registerListenersForWindow(wins.getNext());
          this._enabled = true;
        }
        break;
      case "weave:engine:stop-tracking":
        if (this._enabled) {
          Svc.Obs.remove("private-browsing", this);
          Svc.Obs.remove("domwindowopened", this);
          let wins = Svc.WinMediator.getEnumerator("navigator:browser");
          while (wins.hasMoreElements())
            this._unregisterListenersForWindow(wins.getNext());
          this._enabled = false;
        }
        return;
      case "domwindowopened":
        
        let self = this;
        aSubject.addEventListener("load", function onLoad(event) {
          aSubject.removeEventListener("load", onLoad, false);
          
          self._registerListenersForWindow(aSubject);
        }, false);
        break;
      case "private-browsing":
        if (aData == "enter" && !PBPrefs.get("autostart"))
          this.modified = false;
    }
  },

  onTab: function onTab(event) {
    if (Svc.Private.privateBrowsingEnabled && !PBPrefs.get("autostart")) {
      this._log.trace("Ignoring tab event from private browsing.");
      return;
    }

    this._log.trace("onTab event: " + event.type);
    this.modified = true;

    
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
