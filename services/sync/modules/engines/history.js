



































const EXPORTED_SYMBOLS = ['HistoryEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/type_records/history.js");

Function.prototype.async = Async.sugar;

function HistoryEngine() {
  this._init();
}
HistoryEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "history",
  displayName: "History",
  logName: "History",
  _storeObj: HistoryStore,
  _trackerObj: HistoryTracker
};

function HistoryStore() {
  this._init();
}
HistoryStore.prototype = {
  __proto__: Store.prototype,
  _logName: "HistStore",

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

  _fetchRow: function HistStore__fetchRow(stm, params, retparams) {
    try {
      for (let key in params) {
        stm.params[key] = params[key];
      }
      if (!stm.step())
        return null;
      if (retparams.length == 1)
        return stm.row[retparams[0]];
      let ret = {};
      for each (let key in retparams) {
        ret[key] = stm.row[key];
      }
      return ret;
    } finally {
      stm.reset();
    }
  },

  get _visitStm() {
    this._log.trace("Creating SQL statement: _visitStm");
    let stm = this._db.createStatement(
      "SELECT * FROM ( " +
        "SELECT visit_type AS type, visit_date AS date " +
        "FROM moz_historyvisits_temp " +
        "WHERE place_id = :placeid " +
        "ORDER BY visit_date DESC " +
        "LIMIT 10 " +
      ") " +
      "UNION ALL " +
      "SELECT * FROM ( " +
        "SELECT visit_type AS type, visit_date AS date " +
        "FROM moz_historyvisits " +
        "WHERE place_id = :placeid " +
        "AND id NOT IN (SELECT id FROM moz_historyvisits_temp) " +
        "ORDER BY visit_date DESC " +
        "LIMIT 10 " +
      ") " +
      "ORDER BY 2 DESC LIMIT 10"); 
    this.__defineGetter__("_visitStm", function() stm);
    return stm;
  },

  get _pidStm() {
    this._log.trace("Creating SQL statement: _pidStm");
    let stm = this._db.createStatement(
      "SELECT * FROM " + 
        "(SELECT id FROM moz_places_temp WHERE url = :url LIMIT 1) " +
      "UNION ALL " +
      "SELECT * FROM ( " +
        "SELECT id FROM moz_places WHERE url = :url " +
        "AND id NOT IN (SELECT id from moz_places_temp) " +
        "LIMIT 1 " +
      ") " +
      "LIMIT 1");
    this.__defineGetter__("_pidStm", function() stm);
    return stm;
  },

  get _urlStm() {
    this._log.trace("Creating SQL statement: _urlStm");
    let stm = this._db.createStatement(
      "SELECT * FROM " +
        "(SELECT url,title FROM moz_places_temp WHERE id = :id LIMIT 1) " +
      "UNION ALL " +
      "SELECT * FROM ( " +
        "SELECT url,title FROM moz_places WHERE id = :id " +
        "AND id NOT IN (SELECT id from moz_places_temp) " +
        "LIMIT 1 " +
      ") " +
      "LIMIT 1");
    this.__defineGetter__("_urlStm", function() stm);
    return stm;
  },

  get _annoAttrIdStm() {
    this._log.trace("Creating SQL statement: _annoAttrIdStm");
    let stm = this._db.createStatement(
      "SELECT id from moz_anno_attributes WHERE name = :name");
    this.__defineGetter__("_annoAttrIdStm", function() stm);
    return stm;
  },

  get _findPidByAnnoStm() {
    this._log.trace("Creating SQL statement: _findPidByAnnoStm");
    let stm = this._db.createStatement(
      "SELECT place_id AS id FROM moz_annos " +
        "WHERE anno_attribute_id = :attr AND content = :content");
    this.__defineGetter__("_findPidByAnnoStm", function() stm);
    return stm;
  },

  
  _getVisits: function HistStore__getVisits(uri) {
    let visits = [];
    if (typeof(uri) != "string")
      uri = uri.spec;

    let placeid = this._fetchRow(this._pidStm, {url: uri}, ['id']);
    if (!placeid) {
      this._log.debug("Could not find place ID for history URL: " + placeid);
      return visits;
    }

    try {
      this._visitStm.params.placeid = placeid;
      while (this._visitStm.step()) {
        visits.push({date: this._visitStm.row.date,
                     type: this._visitStm.row.type});
      }
    } finally {
      this._visitStm.reset();
    }

    return visits;
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

  
  _findURLByGUID: function HistStore__findURLByGUID(guid) {
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

      return {url: this._urlStm.row.url,
              title: this._urlStm.row.title};
    } finally {
      this._annoAttrIdStm.reset();
      this._findPidByAnnoStm.reset();
      this._urlStm.reset();
    }
  },

  changeItemID: function HStore_changeItemID(oldID, newID) {
    try {
      let uri = Utils.makeURI(this._findURLByGUID(oldID).uri);
      this._anno.setPageAnnotation(uri, "weave/guid", newID, 0, 0);
    } catch (e) {
      this._log.debug("Could not change item ID: " + e);
    }
  },


  getAllIDs: function HistStore_getAllIDs() {
    let query = this._hsvc.getNewQuery(),
        options = this._hsvc.getNewQueryOptions();

    query.minVisits = 1;
    options.maxResults = 100;
    options.sortingMode = options.SORT_BY_DATE_DESCENDING;
    options.queryType = options.QUERY_TYPE_HISTORY;

    let root = this._hsvc.executeQuery(query, options).root;
    root.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    root.containerOpen = true;

    let items = {};
    for (let i = 0; i < root.childCount; i++) {
      let item = root.getChild(i);
      let guid = this._getGUID(item.uri);
      items[guid] = item.uri;
    }
    return items;
  },

  create: function HistStore_create(record) {
    this._log.debug("  -> creating history entry: " + record.cleartext.uri);

    let uri = Utils.makeURI(record.cleartext.uri);
    let visit;
    while ((visit = record.cleartext.visits.pop())) {
      this._log.debug("     visit " + visit.date);
      this._hsvc.addVisit(uri, visit.date, null, visit.type,
                          (visit.type == 5 || visit.type == 6), 0);
    }
    this._hsvc.setPageTitle(uri, record.cleartext.title);

    let guid = this._getGUID(record.cleartext.uri);
    if (guid != record.id)
      this.changeItemID(guid, record.id);
  },

  remove: function HistStore_remove(record) {
    this._log.trace("  -> NOT removing history entry: " + record.id);
    
  },

  
  update: function HistStore_update(record) {
    this._log.trace("  -> editing history entry: " + record.cleartext.uri);
    let uri = Utils.makeURI(record.cleartext.uri);
    let visit;
    while ((visit = record.cleartext.visits.pop())) {
      this._log.debug("     visit " + visit.date);
      this._hsvc.addVisit(uri, visit.date, null, visit.type,
                          (visit.type == 5 || visit.type == 6), 0);
    }
    this._hsvc.setPageTitle(uri, record.cleartext.title);
  },

  itemExists: function HistStore_itemExists(id) {
    if (this._findURLByGUID(id))
      return true;
    return false;
  },

  createRecord: function HistStore_createRecord(guid) {
    let foo = this._findURLByGUID(guid);
    let record = new HistoryRec();
    if (foo) {
      record.histUri = foo.url;
      record.title = foo.title;
      record.visits = this._getVisits(record.histUri);
    } else {
      record.cleartext = null; 
    }
    this.cache.put(guid, record);
    return record;
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

    
    
    let store = new HistoryStore();
    let all = store.getAllIDs();
    this._all = {};
    for (let guid in all) {
      this._all[all[guid]] = guid;
    }

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
    this.addChangedID(this._all[uri]);
    this._upScore();
  },
  onClearHistory: function HT_onClearHistory() {
    this._log.trace("onClearHistory");
    this._score += 50;
  }
};
