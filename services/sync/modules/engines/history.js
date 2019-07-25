



































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
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

function HistoryEngine() {
  this._init();
}
HistoryEngine.prototype = {
  __proto__: SyncEngine.prototype,
  get _super() SyncEngine.prototype,

  get name() "history",
  get displayName() "History",
  get logName() "History",
  get serverPrefix() "user-data/history/",

  get _store() {
    let store = new HistoryStore();
    this.__defineGetter__("_store", function() store);
    return store;
  },

  get _tracker() {
    let tracker = new HistoryTracker();
    this.__defineGetter__("_tracker", function() tracker);
    return tracker;
  }
};

function HistoryStore() {
  this._init();
}
HistoryStore.prototype = {
  __proto__: Store.prototype,
  _logName: "HistStore",
  _lookup: null,

  get _hsvc() {
    let hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
      getService(Ci.nsINavHistoryService);
    hsvc.QueryInterface(Ci.nsIGlobalHistory2);
    hsvc.QueryInterface(Ci.nsIBrowserHistory);
    this.__defineGetter__("_hsvc", function() hsvc);
    return hsvc;
  },

  get _histDB() {
    let file = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties).
      get("ProfD", Ci.nsIFile);
    file.append("places.sqlite");
    let stor = Cc["@mozilla.org/storage/service;1"].
      getService(Ci.mozIStorageService);
    let db = stor.openDatabase(file);
    this.__defineGetter__("_histDB", function() db);
    return db;
  },

  _itemExists: function HistStore__itemExists(GUID) {
    
    return false;
  },

  _createCommand: function HistStore__createCommand(command) {
    this._log.debug("  -> creating history entry: " + command.GUID);
    try {
      let uri = Utils.makeURI(command.data.URI);
      let redirect = false;
      if (command.data.transition == 5 || command.data.transition == 6)
        redirect = true;

      this._hsvc.addVisit(uri, command.data.time, null,
                          command.data.transition, redirect, 0);
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

  

  _getVisitType: function HistStore__getVisitType(uri) {
    let visitStmnt = this._histDB.createStatement("SELECT visit_type FROM moz_historyvisits WHERE place_id = ?1");
    let pidStmnt = this._histDB.createStatement("SELECT id FROM moz_places WHERE url = ?1");
    
    pidStmnt.bindUTF8StringParameter(0, uri);
    
    let placeID = null;
    if (pidStmnt.executeStep()) {
      placeID = pidStmnt.getInt32(0);
    }

    if (placeID) {
      visitStmnt.bindInt32Parameter(0, placeID);
      if (visitStmnt.executeStep())
        return visitStmnt.getInt32(0);
    }
    return null;
  },
  
  wrap: function HistStore_wrap() {
    let root = this._historyRoot();
    root.containerOpen = true;
    let items = {};
    for (let i = 0; i < root.childCount; i++) {
      let item = root.getChild(i);
      let guid = item.time + ":" + item.uri;
      let vType = this._getVisitType(item.uri);
      items[guid] = {parentid: '',
		     title: item.title,
		     URI: item.uri,
		     time: item.time,
		     transition: vType};
    }
    
    this._lookup = items;
    return items;
  },

  wipe: function HistStore_wipe() {
    this._hsvc.removeAllPages();
  },

  _resetGUIDs: function FormStore__resetGUIDs() {
    let self = yield;
    
  }
};

function HistoryTracker() {
  this._init();
}
HistoryTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "HistoryTracker",

  get _hsvc() {
    let hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
      getService(Ci.nsINavHistoryService);
    this.__defineGetter__("_hsvc", function() hsvc);
    return hsvc;
  },

  _init: function HT__init() {
    this.__proto__.__proto__._init.call(this);
    this._hsvc.addObserver(this, false);
  },

  
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
  }
};
