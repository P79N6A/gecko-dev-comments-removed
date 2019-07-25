



































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
    hsvc.QueryInterface(Ci.nsPIPlacesDatabase);
    this.__defineGetter__("_hsvc", function() hsvc);
    return hsvc;
  },

  get _anno() {
    let anno = Cc["@mozilla.org/browser/annotation-service;1"].
      getService(Ci.nsIAnnotationService);
    this.__defineGetter__("_ans", function() anno);
    return anno;
  },

  get _histDB_31() {
    return this._hsvc.DBConnection;
  },

  get _histDB_30() {
    let file = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties).
      get("ProfD", Ci.nsIFile);
    file.append("places.sqlite");
    let stor = Cc["@mozilla.org/storage/service;1"].
      getService(Ci.mozIStorageService);
    let db = stor.openDatabase(file);
    this.__defineGetter__("_histDB_30", function() db);
    return db;
  },

  get _db() {
    
    
    
    
      return this._histDB_31;
  },

  get _visitStm() {
    let stmt = this._db.createStatement(
      "SELECT visit_type FROM moz_historyvisits WHERE place_id = ?1");
    this.__defineGetter__("_visitStm", function() stmt);
    return stmt;
  },

  get _pidStm() {
    let stmt = this._db.createStatement(
      "SELECT id FROM moz_places WHERE url = ?1");
    this.__defineGetter__("_pidStm", function() stmt);
    return stmt;
  },

  get _urlStm() {
    let stmt = this._db.createStatement(
      "SELECT url FROM moz_places WHERE id = ?1");
    this.__defineGetter__("_urlStm", function() stmt);
    return stmt;
  },

  get _findPidByAnnoStm() {
    let stmt = this._db.createStatement(
      "SELECT place_id FROM moz_annos WHERE type = ?1 AND content = ?2");
    this.__defineGetter__("_findPidByAnnoStm", function() stmt);
    return stmt;
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

  
  _getVisitType: function HistStore__getVisitType(uri) {
    this._pidStm.bindUTF8StringParameter(0, uri);
    if (this._pidStm.executeStep()) {
      let placeId = this._pidStm.getInt32(0);
      this._visitStm.bindInt32Parameter(0, placeId);
      if (this._visitStm.executeStep())
        return this._visitStm.getInt32(0);
    }
    return null;
  },

  _getGUID: function HistStore__getAnno(uri) {
    try {
      return this._anno.getPageAnnotation(uri, "weave/guid");
    } catch (e) {
      
      
      
    }
    let guid = Utils.makeGUID();
    this._anno.setPageAnnotation(uri, "weave/guid", guid, 0, 0);
    return guid;
  },

  
  _findURLByGUID: function HistStore__findByGUID(guid) {
    this._findPidByAnnoStm.bindUTF8Parameter(0, "weave/guid");
    this._findPidByAnnoStm.bindUTF8Parameter(1, guid);
    if (this._findPidByAnnoStm.executeStep()) {
      let placeId = this._findPidByAnnoStm.getInt32(0);
      this._urlStm.bindInt32Parameter(0, placeId);
      if (this._urlStm.executeStep()) {
        return this._urlStm.getString(0);
      }
    }
    return null;
  },

  wrap: function HistStore_wrap() {
    let query = this._hsvc.getNewQuery(),
        options = this._hsvc.getNewQueryOptions();

    query.minVisits = 1;

    let root = this._hsvc.executeQuery(query, options).root;
    root.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    root.containerOpen = true;

    let items = {};
    for (let i = 0; i < root.childCount; i++) {
      let item = root.getChild(i);
      let guid = this._getGUID(item.uri);
      let vType = this._getVisitType(item.uri);
      items[guid] = {title: item.title,
		     URI: item.uri,
		     transition: vType};
      
    }

    return items;
  },

  wrapItem: function HistStore_wrapItem(id) {
    
    if (this._itemCache)
      return this._itemCache[id];
    let all = this._wrap();
    return all[id];
  },

  wipe: function HistStore_wipe() {
    this._hsvc.removeAllPages();
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
