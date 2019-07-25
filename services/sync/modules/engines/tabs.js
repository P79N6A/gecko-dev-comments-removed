



































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
  __proto__: new FileEngine(),

  get name() "tabs",
  get displayName() { return "Tabs"; },
  get logName() "TabEngine",
  get serverPrefix() "user-data/tabs/",

  get virtualTabs() {
    let virtualTabs = {};
    let realTabs = this._store.wrap();

    for (let profileId in this._file.data) {
      let tabset = this._file.data[profileId];
      for (let guid in tabset) {
        if (!(guid in realTabs) && !(guid in virtualTabs)) {
          virtualTabs[guid] = tabset[guid];
          virtualTabs[guid].profileId = profileId;
        }
      }
    }
    return virtualTabs;
  },

  get _store() {
    let store = new TabStore();
    this.__defineGetter__("_store", function() store);
    return this._store;
  },

  get _tracker() {
    let tracker = new TabTracker(this);
    this.__defineGetter__("_tracker", function() tracker);
    return this._tracker;
  }

};

function TabStore() {
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

  _createCommand: function TabStore__createCommand(command) {
    this._log.debug("_createCommand: " + command.GUID);

    if (command.GUID in this._virtualTabs || command.GUID in this._wrapRealTabs())
      throw "trying to create a tab that already exists; id: " + command.GUID;

    
    
    if (!this.validateVirtualTab(command.data)) {
      this._log.warn("could not create command " + command.GUID + "; invalid");
      return;
    }

    
    this._virtualTabs[command.GUID] = command.data;
    this._os.notifyObservers(null, "weave:store:tabs:virtual:created", null);
  },

  


  wrap: function TabStore_wrap() {
    let items = {};

    let session = this._json.decode(this._sessionStore.getBrowserState());

    for (let i = 0; i < session.windows.length; i++) {
      let window = session.windows[i];
      
      
      
      let windowID = i + 1;

      for (let j = 0; j < window.tabs.length; j++) {
        let tab = window.tabs[j];

	
	
	let currentEntry = tab.entries[tab.index - 1];
	if (!currentEntry || !currentEntry.url) {
	  this._log.warn("_wrapRealTabs: no current entry or no URL, can't " +
                         "identify " + this._json.encode(tab));
	  continue;
	}

	let tabID = currentEntry.url;

        
        
        tab.entries = tab.entries.slice(tab.entries.length - 10);

        
        
        
        
        for (let k = 0; k < tab.entries.length; k++) {
            delete tab.entries[k].ID;
        }

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
