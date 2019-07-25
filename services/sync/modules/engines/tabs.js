




































const EXPORTED_SYMBOLS = ['TabEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const TAB_TIME_ATTR = "weave.tabEngine.lastUsed.timeStamp";

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
  displayName: "Tabs",
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

  get _localClientGUID() {
    return Clients.clientID;
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

        
        let lastUsedTimestamp = tab.getAttribute(TAB_TIME_ATTR);

        
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
    if (id == this._localClientGUID) {
      return true;
    } else if (this._remoteClients[id]) {
      return true;
    } else {
      return false;
    }
  },

  createRecord: function TabStore_createRecord(id, cryptoMetaURL) {
    let record;
    if (id == this._localClientGUID) {
      record = this._createLocalClientTabSetRecord();
    } else {
      record = this._remoteClients[id];
    }
    record.id = id;
    record.encryption = cryptoMetaURL;
    return record;
  },

  changeItemID: function TabStore_changeItemId(oldId, newId) {
    if (this._remoteClients[oldId]) {
      let record = this._remoteClients[oldId];
      record.id = newId;
      delete this._remoteClients[oldId];
      this._remoteClients[newId] = record;
    }
  },

  getAllIDs: function TabStore_getAllIds() {
    let items = {};
    items[ this._localClientGUID ] = true;
    for (let id in this._remoteClients) {
      items[id] = true;
    }
    return items;
  },

  wipe: function TabStore_wipe() {
    this._remoteClients = {};
  },

  create: function TabStore_create(record) {
    if (record.id == this._localClientGUID)
      return; 
    this._log.debug("Adding remote tabs from " + record.getClientName());
    this._remoteClients[record.id] = record;
    this._writeToFile();
    
    
    
  },

  update: function TabStore_update(record) {
    if (record.id == this._localClientGUID)
      return; 
    this._log.debug("Updating remote client: " + record.getClientName());
    this._remoteClients[record.id] = record;
    this._writeToFile();
  },

  remove: function TabStore_remove(record) {
    if (record.id == this._localClientGUID)
      return; 
    this._log.trace("Remove called.  Deleting record with id " + record.id);
    delete this._remoteClients[record.id];
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

    
    this.onTabOpened = Utils.bind2(this, this.onTabOpened);
    this.onTabClosed = Utils.bind2(this, this.onTabClosed);
    this.onTabSelected = Utils.bind2(this, this.onTabSelected);

    

    
    var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"]
	       .getService(Ci.nsIWindowWatcher);
    ww.registerNotification(this);

    

    let wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                   .getService(Components.interfaces.nsIWindowMediator);
    let enumerator = wm.getEnumerator("navigator:browser");
    while (enumerator.hasMoreElements()) {
      this._registerListenersForWindow(enumerator.getNext());
    }
  },

  _getBrowser: function TabTracker__getBrowser(window) {
    
    if (typeof window.getBrowser != "function")
      return null;

    
    let browser = window.getBrowser();
    if (browser == null || typeof browser.tabContainer != "object")
      return null;

    return browser;
  },

  _registerListenersForWindow: function TabTracker__registerListen(window) {
    let browser = this._getBrowser(window);
    if (browser == null)
      return;

    
    
    let container = browser.tabContainer;
    container.addEventListener("TabOpen", this.onTabOpened, false);
    container.addEventListener("TabClose", this.onTabClosed, false);
    container.addEventListener("TabSelect", this.onTabSelected, false);
  },

  _unRegisterListenersForWindow: function TabTracker__unregister(window) {
    let browser = this._getBrowser(window);
    if (browser == null)
      return;

    let container = browser.tabContainer;
    container.removeEventListener("TabOpen", this.onTabOpened, false);
    container.removeEventListener("TabClose", this.onTabClosed, false);
    container.removeEventListener("TabSelect", this.onTabSelected, false);
  },

  observe: function TabTracker_observe(aSubject, aTopic, aData) {
    

    let window = aSubject.QueryInterface(Ci.nsIDOMWindow);
    
    if (aTopic == "domwindowopened") {
      this._registerListenersForWindow(window);
    } else if (aTopic == "domwindowclosed") {
      this._unRegisterListenersForWindow(window);
    }
  },

  onTabOpened: function TabTracker_onTabOpened(event) {
    
    this._log.trace("Tab opened.");
    event.target.setAttribute(TAB_TIME_ATTR, event.timeStamp);
    
    this._score += 50;
  },

  onTabClosed: function TabTracker_onTabSelected(event) {
    
    this._score += 10;
  },

  onTabSelected: function TabTracker_onTabSelected(event) {
    
    this._log.trace("Tab selected.");
    
    event.target.setAttribute(TAB_TIME_ATTR, event.timeStamp);
    
    this._score += 10;
  },
  

  get changedIDs() {
    
    let obj = {};
    if (this._score > 0)
      obj[Clients.clientID] = true;
    return obj;
  }
}
