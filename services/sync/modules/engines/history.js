const EXPORTED_SYMBOLS = ['HistoryEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/syncCores.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");

function HistoryEngine(pbeId) {
  this._init(pbeId);
}
HistoryEngine.prototype = {
  get name() { return "history"; },
  get logName() { return "HistEngine"; },
  get serverPrefix() { return "user-data/history/"; },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new HistorySyncCore();
    return this.__core;
  },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new HistoryStore();
    return this.__store;
  },

  __tracker: null,
  get _tracker() {
    if (!this.__tracker)
      this.__tracker = new HistoryTracker();
    return this.__tracker;
  }
};
HistoryEngine.prototype.__proto__ = new Engine();

function HistorySyncCore() {
  this._init();
}
HistorySyncCore.prototype = {
  _logName: "HistSync",

  _itemExists: function HSC__itemExists(GUID) {
    
    return false;
  },

  _commandLike: function HSC_commandLike(a, b) {
    
    
    
    
    return false;
  }
};
HistorySyncCore.prototype.__proto__ = new SyncCore();

function HistoryStore() {
  this._init();
}
HistoryStore.prototype = {
  _logName: "HistStore",

  __hsvc: null,
  get _hsvc() {
    if (!this.__hsvc) {
      this.__hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
                    getService(Ci.nsINavHistoryService);
      this.__hsvc.QueryInterface(Ci.nsIGlobalHistory2);
      this.__hsvc.QueryInterface(Ci.nsIBrowserHistory);
    }
    return this.__hsvc;
  },

  _createCommand: function HistStore__createCommand(command) {
    this._log.debug("  -> creating history entry: " + command.GUID);
    try {
      let uri = Utils.makeURI(command.data.URI);
      this._hsvc.addVisit(uri, command.data.time, null,
                          this._hsvc.TRANSITION_TYPED, false, null);
      this._hsvc.setPageTitle(uri, command.data.title);
    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));
    }
  },

  _removeCommand: function HistStore__removeCommand(command) {
    this._log.trace("  -> NOT removing history entry: " + command.GUID);
    
    
    
    
  },

  _editCommand: function HistStore__editCommand(command) {
    this._log.trace("  -> FIXME: NOT editing history entry: " + command.GUID);
    
  },

  _historyRoot: function HistStore__historyRoot() {
    let query = this._hsvc.getNewQuery(),
        options = this._hsvc.getNewQueryOptions();

    query.minVisits = 1;
    options.maxResults = 1000;
    options.resultType = options.RESULTS_AS_VISIT; 
    options.sortingMode = options.SORT_BY_DATE_DESCENDING;
    options.queryType = options.QUERY_TYPE_HISTORY;

    let root = this._hsvc.executeQuery(query, options).root;
    root.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    return root;
  },

  wrap: function HistStore_wrap() {
    let root = this._historyRoot();
    root.containerOpen = true;
    let items = {};
    for (let i = 0; i < root.childCount; i++) {
      let item = root.getChild(i);
      let guid = item.time + ":" + item.uri
      items[guid] = {parentGUID: '',
			 title: item.title,
			 URI: item.uri,
			 time: item.time
			};
      
    }
    return items;
  },

  wipe: function HistStore_wipe() {
    this._hsvc.removeAllPages();
  }
};
HistoryStore.prototype.__proto__ = new Store();

function HistoryTracker() {
  this._init();
}
HistoryTracker.prototype = {
  _logName: "HistoryTracker",

  
  onBeginUpdateBatch: function HT_onBeginUpdateBatch() {

  },
  onEndUpdateBatch: function HT_onEndUpdateBatch() {

  },
  onPageChanged: function HT_onPageChanged() {

  },
  onTitleChanged: function HT_onTitleChanged() {

  },

  




  onVisit: function HT_onVisit(uri, vid, time, session, referrer, trans) {
    this._score += 1;
  },
  onPageExpired: function HT_onPageExpired(uri, time, entry) {
    this._score += 1;
  },
  onDeleteURI: function HT_onDeleteURI(uri) {
    this._score += 1;
  },
  onClearHistory: function HT_onClearHistory() {
    this._score += 50;
  },

  _init: function HT__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;

    Cc["@mozilla.org/browser/nav-history-service;1"].
    getService(Ci.nsINavHistoryService).
    addObserver(this, false);
  }
}
HistoryTracker.prototype.__proto__ = new Tracker();
