



































const EXPORTED_SYMBOLS = ['HistoryEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

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
    
    
    
    
      return this._hsvc.DBConnection;
  },

  get _visitStm() {
    let stmt = this._db.createStatement(
      "SELECT visit_type AS type FROM moz_historyvisits WHERE place_id = :placeid");
    this.__defineGetter__("_visitStm", function() stmt);
    return stmt;
  },

  get _pidStm() {
    let stmt = this._db.createStatement(
      "SELECT id FROM moz_places WHERE url = :url");
    this.__defineGetter__("_pidStm", function() stmt);
    return stmt;
  },

  get _urlStm() {
    let stmt = this._db.createStatement(
      "SELECT url FROM moz_places WHERE id = :id");
    this.__defineGetter__("_urlStm", function() stmt);
    return stmt;
  },

  get _annoAttrIdStm() {
    let stmt = this._db.createStatement(
      "SELECT id from moz_anno_attributes WHERE name = :name");
    this.__defineGetter__("_annoAttrIdStm", function() stmt);
    return stmt;
  },

  get _findPidByAnnoStm() {
    let stmt = this._db.createStatement(
      "SELECT place_id AS id FROM moz_annos WHERE anno_attribute_id = :attr AND content = :content");
    this.__defineGetter__("_findPidByAnnoStm", function() stmt);
    return stmt;
  },

  create: function HistStore_create(record) {
    this._log.debug("  -> creating history entry: " + record.id);
    let uri = Utils.makeURI(record.cleartext.URI);
    let redirect = false;
    if (record.cleartext.transition == 5 || record.cleartext.transition == 6)
      redirect = true;

    this._hsvc.addVisit(uri, record.cleartext.time, null,
                        record.cleartext.transition, redirect, 0);
    this._hsvc.setPageTitle(uri, record.cleartext.title);
  },

  remove: function HistStore_remove(record) {
    this._log.trace("  -> NOT removing history entry: " + record.id);
    
  },

  update: function HistStore_update(record) {
    this._log.trace("  -> FIXME: NOT editing history entry: " + record.id);
    
  },

  
  _getVisitType: function HistStore__getVisitType(uri) {
    if (typeof(uri) != "string")
      uri = uri.spec;
    try {
      this._pidStm.params.url = uri;
      if (this._pidStm.step()) {
        let placeId = this._pidStm.row.id;
        this._visitStm.params.placeid = placeId;
        if (this._visitStm.step())
          return this._visitStm.row.type;
      }
    } finally {
      this._pidStm.reset();
      this._visitStm.reset();
    }
    return null;
  },

  _getGUID: function HistStore__getGUID(uri) {
    if (typeof(uri) == "string")
      uri = Utils.makeURI(uri);
    try {
      return this._anno.getPageAnnotation(uri, "weave/guid");
    } catch (e) {
      
      
      
    }
    let guid = Utils.makeGUID();
    this._anno.setPageAnnotation(uri, "weave/guid", guid, 0, this._anno.EXPIRE_WITH_HISTORY);
    return guid;
  },

  
  _findURLByGUID: function HistStore__findByGUID(guid) {
    try {
      this._annoAttrIdStm.params.name = "weave/guid";
      if (!this._annoAttrIdStm.step())
        return null;
      let annoId = this._annoAttrIdStm.row.id;

      this._findPidByAnnoStm.params.attr = annoId;
      this._findPidByAnnoStm.params.content = guid;
      if (!this._findPidByAnnoStm.step())
        return null;
      let placeId = this._findPidByAnnoStm.row.id;

      this._urlStm.params.id = placeId;
      if (!this._urlStm.step())
        return null;

      return this._urlStm.row.url;
    } finally {
      this._annoAttrIdStm.reset();
      this._findPidByAnnoStm.reset();
      this._urlStm.reset();
    }
  },

  changeItemID: function HStore_changeItemID(oldID, newID) {
    let uri = Utils.makeURI(this._findURLByGUID(oldID));
    this._anno.setPageAnnotation(uri, "weave/guid", newID, 0, 0);
  },

  
  getAllIDs: function BStore_getAllIDs() {
    let all = this.wrap();
    return all;
  },

  wrap: function HistStore_wrap() {
    let query = this._hsvc.getNewQuery(),
        options = this._hsvc.getNewQueryOptions();

    query.minVisits = 1;
    options.maxResults = 150;
    options.sortingMode = options.SORT_BY_DATE_DESCENDING;
    options.queryType = options.QUERY_TYPE_HISTORY;

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

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver]),

  get _hsvc() {
    let hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
      getService(Ci.nsINavHistoryService);
    this.__defineGetter__("_hsvc", function() hsvc);
    return hsvc;
  },

  
  get _store() {
    let store = new HistoryStore();
    this.__defineGetter__("_store", function() store);
    return store;
  },

  _init: function HT__init() {
    this.__proto__.__proto__._init.call(this);
    this._hsvc.addObserver(this, false);
  },

  onBeginUpdateBatch: function HT_onBeginUpdateBatch() {},
  onEndUpdateBatch: function HT_onEndUpdateBatch() {},
  onPageChanged: function HT_onPageChanged() {},
  onTitleChanged: function HT_onTitleChanged() {},

  


  _upScore: function BMT__upScore() {
    if (!this.enabled)
      return;
    this._score += 1;
  },

  onVisit: function HT_onVisit(uri, vid, time, session, referrer, trans) {
    this._log.trace("onVisit: " + uri.spec);
    this.addChangedID(this._store._getGUID(uri));
    this._upScore();
  },
  onPageExpired: function HT_onPageExpired(uri, time, entry) {
    this._log.trace("onPageExpired: " + uri.spec);
    this.addChangedID(this._store._getGUID(uri)); 
    this._upScore();
  },
  onDeleteURI: function HT_onDeleteURI(uri) {
    this._log.trace("onDeleteURI: " + uri.spec);
    this.addChangedID(this._store._getGUID(uri)); 
    this._upScore();
  },
  onClearHistory: function HT_onClearHistory() {
    this._log.trace("onClearHistory");
    this._score += 50;
  }
};
