



this.EXPORTED_SYMBOLS = ['TabEngine', 'TabSetRecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const TABS_TTL = 604800; 

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/constants.js");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");

this.TabSetRecord = function TabSetRecord(collection, id) {
  CryptoWrapper.call(this, collection, id);
}
TabSetRecord.prototype = {
  __proto__: CryptoWrapper.prototype,
  _logName: "Sync.Record.Tabs",
  ttl: TABS_TTL
};

Utils.deferGetSet(TabSetRecord, "cleartext", ["clientName", "tabs"]);


this.TabEngine = function TabEngine(service) {
  SyncEngine.call(this, "Tabs", service);

  
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
      changedIDs[this.service.clientsEngine.localID] = 0;
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
    let url = this.engineURL + "/" + this.service.clientsEngine.localID;
    this.service.resource(url).delete();
  },

  


  getOpenURLs: function () {
    let urls = new Set();
    for (let entry of this._store.getAllTabs()) {
      urls.add(entry.urlHistory[0]);
    }
    return urls;
  }
};


function TabStore(name, engine) {
  Store.call(this, name, engine);
}
TabStore.prototype = {
  __proto__: Store.prototype,

  itemExists: function TabStore_itemExists(id) {
    return id == this.engine.service.clientsEngine.localID;
  },

  getWindowEnumerator: function () {
    return Services.wm.getEnumerator("navigator:browser");
  },

  shouldSkipWindow: function (win) {
    return win.closed ||
           PrivateBrowsingUtils.isWindowPrivate(win);
  },

  getTabState: function (tab) {
    return JSON.parse(Svc.Session.getTabState(tab));
  },

  getAllTabs: function (filter) {
    let filteredUrls = new RegExp(Svc.Prefs.get("engine.tabs.filteredUrls"), "i");

    let allTabs = [];

    let winEnum = this.getWindowEnumerator();
    while (winEnum.hasMoreElements()) {
      let win = winEnum.getNext();
      if (this.shouldSkipWindow(win)) {
        continue;
      }

      for (let tab of win.gBrowser.tabs) {
        tabState = this.getTabState(tab);

        
        if (!tabState || !tabState.entries.length) {
          continue;
        }

        
        
        let entry = tabState.entries[tabState.index - 1];

        
        
        if (!entry.url || filter && filteredUrls.test(entry.url)) {
          continue;
        }

        
        
        allTabs.push({
          title: entry.title || "",
          urlHistory: [entry.url],
          icon: tabState.attributes && tabState.attributes.image || "",
          lastUsed: Math.floor((tabState.lastAccessed || 0) / 1000)
        });
      }
    }

    return allTabs;
  },

  createRecord: function createRecord(id, collection) {
    let record = new TabSetRecord(collection, id);
    record.clientName = this.engine.service.clientsEngine.localName;

    
    let tabs = this.getAllTabs(true).sort(function (a, b) {
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
    tabs.forEach(function (tab) {
      this._log.trace("Wrapping tab: " + JSON.stringify(tab));
    }, this);

    record.tabs = tabs;
    return record;
  },

  getAllIDs: function TabStore_getAllIds() {
    
    
    let ids = {};
    let allWindowsArePrivate = false;
    let wins = Services.wm.getEnumerator("navigator:browser");
    while (wins.hasMoreElements()) {
      if (PrivateBrowsingUtils.isWindowPrivate(wins.getNext())) {
        
        allWindowsArePrivate = true;
      } else {
        
        allWindowsArePrivate = false;
        break;
      }
    }

    if (allWindowsArePrivate &&
        !PrivateBrowsingUtils.permanentPrivateBrowsing) {
      return ids;
    }

    ids[this.engine.service.clientsEngine.localID] = true;
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


function TabTracker(name, engine) {
  Tracker.call(this, name, engine);
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

  startTracking: function () {
    Svc.Obs.add("domwindowopened", this);
    let wins = Services.wm.getEnumerator("navigator:browser");
    while (wins.hasMoreElements()) {
      this._registerListenersForWindow(wins.getNext());
    }
  },

  stopTracking: function () {
    Svc.Obs.remove("domwindowopened", this);
    let wins = Services.wm.getEnumerator("navigator:browser");
    while (wins.hasMoreElements()) {
      this._unregisterListenersForWindow(wins.getNext());
    }
  },

  observe: function (subject, topic, data) {
    Tracker.prototype.observe.call(this, subject, topic, data);

    switch (topic) {
      case "domwindowopened":
        let onLoad = () => {
          subject.removeEventListener("load", onLoad, false);
          
          this._registerListenersForWindow(subject);
        };

        
        subject.addEventListener("load", onLoad, false);
        break;
    }
  },

  onTab: function onTab(event) {
    if (event.originalTarget.linkedBrowser) {
      let browser = event.originalTarget.linkedBrowser;
      if (PrivateBrowsingUtils.isBrowserPrivate(browser) &&
          !PrivateBrowsingUtils.permanentPrivateBrowsing) {
        this._log.trace("Ignoring tab event from private browsing.");
        return;
      }
    }

    this._log.trace("onTab event: " + event.type);
    this.modified = true;

    
    
    
    if (event.type != "pageshow" || Math.random() < .1)
      this.score += SCORE_INCREMENT_SMALL;
  },
}
