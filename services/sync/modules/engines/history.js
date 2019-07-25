



































const EXPORTED_SYMBOLS = ['HistoryEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const GUID_ANNO = "weave/guid";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/base_records/collection.js");
Cu.import("resource://weave/type_records/history.js");


function setGUID(uri, guid) {
  if (arguments.length == 1)
    guid = Utils.makeGUID();
  Utils.anno(uri, GUID_ANNO, guid, "WITH_HISTORY");
  return guid;
}
function GUIDForUri(uri, create) {
  try {
    
    return Utils.anno(uri, GUID_ANNO);
  }
  catch (ex) {
    
    if (create)
      return setGUID(uri);
  }
}

function HistoryEngine() {
  this._init();
}
HistoryEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "history",
  _displayName: "History",
  description: "All the sites you've been to.  Take your awesomebar with you!",
  logName: "History",
  _recordObj: HistoryRec,
  _storeObj: HistoryStore,
  _trackerObj: HistoryTracker,

  _sync: Utils.batchSync("History", SyncEngine),

  _findDupe: function _findDupe(item) {
    return GUIDForUri(item.histUri);
  }
};

function HistoryStore() {
  this._init();
}
HistoryStore.prototype = {
  __proto__: Store.prototype,
  name: "history",
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

  get _db() {
    return this._hsvc.DBConnection;
  },

  get _visitStm() {
    this._log.trace("Creating SQL statement: _visitStm");
    let stm = this._db.createStatement(
      "SELECT visit_type type, visit_date date " +
      "FROM moz_historyvisits_view " +
      "WHERE place_id = (" +
        "SELECT id " +
        "FROM moz_places_view " +
        "WHERE url = :url) " +
      "ORDER BY date DESC LIMIT 10");
    this.__defineGetter__("_visitStm", function() stm);
    return stm;
  },

  get _urlStm() {
    this._log.trace("Creating SQL statement: _urlStm");
    let stm = this._db.createStatement(
      "SELECT url, title, frecency " +
      "FROM moz_places_view " +
      "WHERE id = (" +
        "SELECT place_id " +
        "FROM moz_annos " +
        "WHERE content = :guid AND anno_attribute_id = (" +
          "SELECT id " +
          "FROM moz_anno_attributes " +
          "WHERE name = '" + GUID_ANNO + "'))");
    this.__defineGetter__("_urlStm", function() stm);
    return stm;
  },

  
  _getVisits: function HistStore__getVisits(uri) {
    let visits = [];
    if (typeof(uri) != "string")
      uri = uri.spec;

    try {
      this._visitStm.params.url = uri;
      while (this._visitStm.step()) {
        visits.push({date: this._visitStm.row.date,
                     type: this._visitStm.row.type});
      }
    } finally {
      this._visitStm.reset();
    }

    return visits;
  },

  
  _findURLByGUID: function HistStore__findURLByGUID(guid) {
    try {
      this._urlStm.params.guid = guid;
      if (!this._urlStm.step())
        return null;

      return {
        url: this._urlStm.row.url,
        title: this._urlStm.row.title,
        frecency: this._urlStm.row.frecency
      };
    }
    finally {
      this._urlStm.reset();
    }
  },

  changeItemID: function HStore_changeItemID(oldID, newID) {
    setGUID(this._findURLByGUID(oldID).url, newID);
  },


  getAllIDs: function HistStore_getAllIDs() {
    let query = this._hsvc.getNewQuery(),
        options = this._hsvc.getNewQueryOptions();

    query.minVisits = 1;
    options.maxResults = 1000;
    options.sortingMode = options.SORT_BY_DATE_DESCENDING;
    options.queryType = options.QUERY_TYPE_HISTORY;

    let root = this._hsvc.executeQuery(query, options).root;
    root.QueryInterface(Ci.nsINavHistoryQueryResultNode);
    root.containerOpen = true;

    let items = {};
    for (let i = 0; i < root.childCount; i++) {
      let item = root.getChild(i);
      let guid = GUIDForUri(item.uri, true);
      items[guid] = item.uri;
    }
    return items;
  },

  create: function HistStore_create(record) {
    
    this.update(record);
    setGUID(record.histUri, record.id);
  },

  remove: function HistStore_remove(record) {
    
    
  },

  update: function HistStore_update(record) {
    this._log.trace("  -> processing history entry: " + record.histUri);

    let uri = Utils.makeURI(record.histUri);
    if (!uri) {
      this._log.warn("Attempted to process invalid URI, skipping");
      throw "invalid URI in record";
    }
    let curvisits = [];
    if (this.urlExists(uri))
      curvisits = this._getVisits(record.histUri);

    
    for each (let {date, type} in record.visits)
      if (curvisits.every(function(cur) cur.date != date))
        Svc.History.addVisit(uri, date, null, type, type == 5 || type == 6, 0);

    this._hsvc.setPageTitle(uri, record.title);
  },

  itemExists: function HistStore_itemExists(id) {
    if (this._findURLByGUID(id))
      return true;
    return false;
  },

  urlExists: function HistStore_urlExists(url) {
    if (typeof(url) == "string")
      url = Utils.makeURI(url);
    
    return url ? this._hsvc.isVisited(url) : false;
  },

  createRecord: function HistStore_createRecord(guid, cryptoMetaURL) {
    let foo = this._findURLByGUID(guid);
    let record = new HistoryRec();
    record.id = guid;
    if (foo) {
      record.histUri = foo.url;
      record.title = foo.title;
      record.sortindex = foo.frecency;
      record.visits = this._getVisits(record.histUri);
      record.encryption = cryptoMetaURL;
    }
    else
      record.deleted = true;
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
  name: "history",
  _logName: "HistoryTracker",
  file: "history",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver]),

  get _hsvc() {
    let hsvc = Cc["@mozilla.org/browser/nav-history-service;1"].
      getService(Ci.nsINavHistoryService);
    this.__defineGetter__("_hsvc", function() hsvc);
    return hsvc;
  },

  _init: function HT__init() {
    Tracker.prototype._init.call(this);
    this._hsvc.addObserver(this, false);
  },

  onBeginUpdateBatch: function HT_onBeginUpdateBatch() {},
  onEndUpdateBatch: function HT_onEndUpdateBatch() {},
  onPageChanged: function HT_onPageChanged() {},
  onTitleChanged: function HT_onTitleChanged() {},

  


  _upScore: function BMT__upScore() {
    this.score += 1;
  },

  onVisit: function HT_onVisit(uri, vid, time, session, referrer, trans) {
    if (this.ignoreAll)
      return;
    this._log.trace("onVisit: " + uri.spec);
    if (this.addChangedID(GUIDForUri(uri, true)))
      this._upScore();
  },
  onPageExpired: function HT_onPageExpired(uri, time, entry) {
  },
  onDeleteURI: function HT_onDeleteURI(uri) {
  },
  onClearHistory: function HT_onClearHistory() {
    this._log.trace("onClearHistory");
    this.score += 500;
  }
};
