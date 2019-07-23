






















































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const nsPlacesExpirationFactory = {
  _instance: null,
  createInstance: function(aOuter, aIID) {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return this._instance === null ? this._instance = new nsPlacesExpiration() :
                                     this._instance;
  },
  lockFactory: function (aDoLock) {},
  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIFactory
  ]),
};




const MAX_INT64 = 9223372036854775807;

const TOPIC_SHUTDOWN = "places-shutdown";
const TOPIC_PREF_CHANGED = "nsPref:changed";
const TOPIC_DEBUG_START_EXPIRATION = "places-debug-start-expiration";
const TOPIC_EXPIRATION_FINISHED = "places-expiration-finished";
const TOPIC_IDLE_BEGIN = "idle";
const TOPIC_IDLE_END = "back";


const PREF_BRANCH = "places.history.expiration.";








const PREF_MAX_URIS = "max_pages";
const PREF_MAX_URIS_NOTSET = -1; 



const PREF_READONLY_CALCULATED_MAX_URIS = "transient_current_max_pages";


const PREF_INTERVAL_SECONDS = "interval_seconds";
const PREF_INTERVAL_SECONDS_NOTSET = 3 * 60;




const PREF_DATABASE_CACHE_PER_MEMORY_PERCENTAGE =
  "places.history.cache_per_memory_percentage";
const PREF_DATABASE_CACHE_PER_MEMORY_PERCENTAGE_NOTSET = 6;



const MIN_URIS = 1000;




const EXPIRE_LIMIT_PER_STEP = 6;

const EXPIRE_LIMIT_PER_LARGE_STEP_MULTIPLIER = 10;











const EXPIRE_AGGRESSIVITY_MULTIPLIER = 3;









const URIENTRY_AVG_SIZE_MIN = 2000;
const URIENTRY_AVG_SIZE_MAX = 4000;




const IDLE_TIMEOUT_SECONDS = 5 * 60;




const SHUTDOWN_WITH_RECENT_CLEARHISTORY_TIMEOUT_SECONDS = 10;

const USECS_PER_DAY = 86400000000;
const ANNOS_EXPIRE_POLICIES = [
  { bind: "expire_days",
    type: Ci.nsIAnnotationService.EXPIRE_DAYS,
    time: 7 * USECS_PER_DAY },
  { bind: "expire_weeks",
    type: Ci.nsIAnnotationService.EXPIRE_WEEKS,
    time: 30 * USECS_PER_DAY },
  { bind: "expire_months",
    type: Ci.nsIAnnotationService.EXPIRE_MONTHS,
    time: 180 * USECS_PER_DAY },
];






const LIMIT = {
  SMALL: 0,
  LARGE: 1,
  UNLIMITED: 2,
  DEBUG: 3,
};


const STATUS = {
  CLEAN: 0,
  DIRTY: 1,
  UNKNOWN: 2,
};


const ACTION = {
  TIMED: 1 << 0,
  CLEAR_HISTORY: 1 << 1,
  SHUTDOWN: 1 << 2,
  CLEAN_SHUTDOWN: 1 << 3,
  IDLE: 1 << 4,
  DEBUG: 1 << 5,
};


const EXPIRATION_QUERIES = {

  
  
  QUERY_FIND_VISITS_TO_EXPIRE: {
    sql: "SELECT IFNULL(h_t.url, h.url) AS url, v.visit_date AS visit_date, " +
                "0 AS whole_entry, :limit_visits AS expected_results " +
         "FROM moz_historyvisits_temp v " +
         "LEFT JOIN moz_places_temp AS h_t ON h_t.id = v.place_id " +
         "LEFT JOIN moz_places AS h ON h.id = v.place_id " +
         "WHERE ((SELECT count(*) FROM moz_places_temp) + " +
                "(SELECT count(*) FROM moz_places)) > :max_uris " +
         "UNION ALL " +
         "SELECT IFNULL(h_t.url, h.url) AS url, v.visit_date AS visit_date, " +
                "0 AS whole_entry, :limit_uris AS expected_results " +
         "FROM moz_historyvisits v " +
         "LEFT JOIN moz_places_temp AS h_t ON h_t.id = v.place_id " +
         "LEFT JOIN moz_places AS h ON h.id = v.place_id " +
         "WHERE ((SELECT count(*) FROM moz_places_temp) + " +
                "(SELECT count(*) FROM moz_places)) > :max_uris " +
         "ORDER BY v.visit_date ASC " +
         "LIMIT :limit_visits",
    actions: ACTION.TIMED | ACTION.SHUTDOWN | ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_VISITS: {
    sql: "DELETE FROM moz_historyvisits_view WHERE id IN ( " +
           "SELECT id FROM ( " +
             "SELECT v.id, v.visit_date " +
             "FROM moz_historyvisits_temp v " +
             "UNION ALL " +
             "SELECT v.id, v.visit_date " +
             "FROM moz_historyvisits v " +
             "ORDER BY v.visit_date ASC " +
             "LIMIT :limit_visits " +
            " ) " +
          ") " +
        " AND ((SELECT count(*) FROM moz_places_temp) + " +
              "(SELECT count(*) FROM moz_places))  > :max_uris",
    actions: ACTION.TIMED | ACTION.SHUTDOWN | ACTION.IDLE | ACTION.DEBUG
  },

  
  
  
  QUERY_FIND_URIS_TO_EXPIRE: {
    sql: "SELECT h.url, h.last_visit_date AS visit_date, 1 AS whole_entry, " +
                ":limit_uris AS expected_results " +
         "FROM moz_places_temp h " +
         "LEFT JOIN moz_historyvisits v ON h.id = v.place_id " +
         "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id " +
         "LEFT JOIN moz_bookmarks b ON h.id = b.fk " +
         "WHERE v.id IS NULL " +
           "AND v_t.id IS NULL " +
           "AND b.id IS NULL " +
           "AND h.id <= :last_place_id " +
           "AND SUBSTR(h.url, 1, 6) <> 'place:' " +
         "UNION ALL " +
         "SELECT h.url, h.last_visit_date AS visit_date, 1 AS whole_entry, " +
               ":limit_uris AS expected_results " +
         "FROM moz_places h " +
         "LEFT JOIN moz_historyvisits v ON h.id = v.place_id " +
         "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id " +
         "LEFT JOIN moz_bookmarks b ON h.id = b.fk " +
         "WHERE v.id IS NULL " +
           "AND v_t.id IS NULL " +
           "AND b.id IS NULL " +
           "AND h.id <= :last_place_id " +
           "AND SUBSTR(h.url, 1, 6) <> 'place:' " +
         "LIMIT :limit_uris",
    actions: ACTION.TIMED | ACTION.SHUTDOWN | ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_URIS: {
    sql: "DELETE FROM moz_places_view WHERE id IN ( " +
           "SELECT h.id " +
           "FROM moz_places_temp h " +
           "LEFT JOIN moz_historyvisits v ON h.id = v.place_id " +
           "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id " +
           "LEFT JOIN moz_bookmarks b ON h.id = b.fk " +
           "WHERE v.id IS NULL " +
             "AND v_t.id IS NULL " +
             "AND b.id IS NULL " +
             "AND h.id <= :last_place_id " +
             "AND SUBSTR(h.url, 1, 6) <> 'place:' " +
           "UNION ALL " +
           "SELECT h.id " +
           "FROM moz_places h " +
           "LEFT JOIN moz_historyvisits v ON h.id = v.place_id " +
           "LEFT JOIN moz_historyvisits_temp v_t ON h.id = v_t.place_id " +
           "LEFT JOIN moz_bookmarks b ON h.id = b.fk " +
           "WHERE v.id IS NULL " +
             "AND v_t.id IS NULL " +
             "AND b.id IS NULL " +
             "AND h.id <= :last_place_id " +
             "AND SUBSTR(h.url, 1, 6) <> 'place:' " +
           "LIMIT :limit_uris " +
         ")",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_FAVICONS: {
    sql: "DELETE FROM moz_favicons WHERE id IN ( " +
           "SELECT f.id FROM moz_favicons f " +
           "LEFT JOIN moz_places h ON f.id = h.favicon_id " +
           "LEFT JOIN moz_places_temp h_t ON f.id = h_t.favicon_id " +
           "WHERE h.favicon_id IS NULL " +
             "AND h_t.favicon_id IS NULL " +
           "LIMIT :limit_favicons " +
         ")",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS: {
    sql: "DELETE FROM moz_annos WHERE id in ( " +
           "SELECT a.id FROM moz_annos a " +
           "LEFT JOIN moz_places h ON a.place_id = h.id " +
           "LEFT JOIN moz_places_temp h_t ON a.place_id = h_t.id " +
           "LEFT JOIN moz_historyvisits v ON a.place_id = v.place_id " +
           "LEFT JOIN moz_historyvisits_temp v_t ON a.place_id = v_t.place_id " +
           "WHERE (h.id IS NULL AND h_t.id IS NULL) " +
              "OR (v.id IS NULL AND v_t.id IS NULL AND a.expiration <> :expire_never) " +
           "LIMIT :limit_annos " +
         ")",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS_WITH_POLICY: {
    sql: "DELETE FROM moz_annos " +
         "WHERE (expiration = :expire_days " +
           "AND :expire_days_time > MAX(lastModified, dateAdded)) " +
            "OR (expiration = :expire_weeks " +
           "AND :expire_weeks_time > MAX(lastModified, dateAdded)) " +
            "OR (expiration = :expire_months " +
           "AND :expire_months_time > MAX(lastModified, dateAdded))",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ITEMS_ANNOS_WITH_POLICY: {
    sql: "DELETE FROM moz_items_annos " +
         "WHERE (expiration = :expire_days " +
           "AND :expire_days_time > MAX(lastModified, dateAdded)) " +
            "OR (expiration = :expire_weeks " +
           "AND :expire_weeks_time > MAX(lastModified, dateAdded)) " +
            "OR (expiration = :expire_months " +
           "AND :expire_months_time > MAX(lastModified, dateAdded))",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS_WITH_HISTORY: {
    sql: "DELETE FROM moz_annos " +
         "WHERE expiration = :expire_with_history " +
           "AND NOT EXISTS (SELECT id FROM moz_historyvisits_temp " +
                           "WHERE place_id = moz_annos.place_id LIMIT 1) " +
           "AND NOT EXISTS (SELECT id FROM moz_historyvisits " +
                           "WHERE place_id = moz_annos.place_id LIMIT 1)",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ITEMS_ANNOS: {
    sql: "DELETE FROM moz_items_annos WHERE id IN ( " +
           "SELECT a.id FROM moz_items_annos a " +
           "LEFT JOIN moz_bookmarks b ON a.item_id = b.id " +
           "WHERE b.id IS NULL " +
           "LIMIT :limit_annos " +
         ")",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNO_ATTRIBUTES: {
    sql: "DELETE FROM moz_anno_attributes WHERE id IN ( " +
           "SELECT n.id FROM moz_anno_attributes n " +
           "LEFT JOIN moz_annos a ON n.id = a.anno_attribute_id " +
           "LEFT JOIN moz_items_annos t ON n.id = t.anno_attribute_id " +
           "WHERE a.anno_attribute_id IS NULL " +
             "AND t.anno_attribute_id IS NULL " +
           "LIMIT :limit_annos" +
         ")",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_INPUTHISTORY: {
    sql: "DELETE FROM moz_inputhistory WHERE place_id IN ( " +
           "SELECT i.place_id FROM moz_inputhistory i " +
           "LEFT JOIN moz_places h ON h.id = i.place_id " +
           "LEFT JOIN moz_places_temp h_t ON h_t.id = i.place_id " +
           "WHERE h.id IS NULL " +
             "AND h_t.id IS NULL " +
           "LIMIT :limit_inputhistory " +
         ")",
    actions: ACTION.TIMED | ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN |
             ACTION.IDLE | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS_SESSION: {
    sql: "DELETE FROM moz_annos WHERE expiration = :expire_session",
    actions: ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN | ACTION.CLEAN_SHUTDOWN |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ITEMS_ANNOS_SESSION: {
    sql: "DELETE FROM moz_items_annos WHERE expiration = :expire_session",
    actions: ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN | ACTION.CLEAN_SHUTDOWN |
             ACTION.DEBUG
  },

};




function nsPlacesExpiration()
{
  
  

  XPCOMUtils.defineLazyGetter(this, "_db", function() {
    return Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsPIPlacesDatabase).
           DBConnection;
  });
  XPCOMUtils.defineLazyServiceGetter(this, "_ios",
                                     "@mozilla.org/network/io-service;1",
                                     "nsIIOService");
  XPCOMUtils.defineLazyServiceGetter(this, "_hsn",
                                     "@mozilla.org/browser/nav-history-service;1",
                                     "nsPIPlacesHistoryListenersNotifier");
  XPCOMUtils.defineLazyServiceGetter(this, "_sys",
                                     "@mozilla.org/system-info;1",
                                     "nsIPropertyBag2");
  XPCOMUtils.defineLazyServiceGetter(this, "_idle",
                                     "@mozilla.org/widget/idleservice;1",
                                     "nsIIdleService");

  this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                     getService(Ci.nsIPrefService).
                     getBranch(PREF_BRANCH).
                     QueryInterface(Ci.nsIPrefBranch2);
  this._loadPrefs();

  
  this._prefBranch.addObserver("", this, false);

  
  this._os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
  this._os.addObserver(this, TOPIC_SHUTDOWN, false);
  this._os.addObserver(this, TOPIC_DEBUG_START_EXPIRATION, false);

  
  this._newTimer();
}

nsPlacesExpiration.prototype = {

  
  

  observe: function PEX_observe(aSubject, aTopic, aData)
  {
    if (aTopic == TOPIC_SHUTDOWN) {
      this._shuttingDown = true;
      this._os.removeObserver(this, TOPIC_SHUTDOWN);
      this._os.removeObserver(this, TOPIC_DEBUG_START_EXPIRATION);

      this._prefBranch.removeObserver("", this);

      if (this._isIdleObserver)
        this._idle.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);

      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }

      
      
      let hasRecentClearHistory =
        Date.now() - this._lastClearHistoryTime <
          SHUTDOWN_WITH_RECENT_CLEARHISTORY_TIMEOUT_SECONDS * 1000;
      let action = hasRecentClearHistory ? ACTION.CLEAN_SHUTDOWN
                                         : ACTION.SHUTDOWN;
      this._expireWithActionAndLimit(action, LIMIT.LARGE);

      this._finalizeInternalStatements();
    }
    else if (aTopic == TOPIC_PREF_CHANGED) {
      this._loadPrefs();

      if (aData == PREF_INTERVAL_SECONDS) {
        
        this._newTimer();
      }
    }
    else if (aTopic == TOPIC_DEBUG_START_EXPIRATION) {
      this._debugLimit = aData;
      this._expireWithActionAndLimit(ACTION.DEBUG, LIMIT.DEBUG);
    }
    else if (aTopic == TOPIC_IDLE_BEGIN) {
      
      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }
      this._expireWithActionAndLimit(ACTION.IDLE, LIMIT.LARGE);
    }
    else if (aTopic == TOPIC_IDLE_END) {
      
      if (!this._timer)
        this._newTimer();
    }
  },

  
  

  _inBatchMode: false,
  onBeginUpdateBatch: function PEX_onBeginUpdateBatch()
  {
    this._inBatchMode = true;

    
    if (this._timer) {
      this._timer.cancel();
      this._timer = null;
    }
  },

  onEndUpdateBatch: function PEX_onEndUpdateBatch()
  {
    this._inBatchMode = false;

    
    if (!this._timer)
      this._newTimer();
  },

  _lastClearHistoryTime: 0,
  onClearHistory: function PEX_onClearHistory() {
    this._lastClearHistoryTime = Date.now();
    
    this._expireWithActionAndLimit(ACTION.CLEAR_HISTORY, LIMIT.UNLIMITED);
  },

  onVisit: function() {},
  onTitleChanged: function() {},
  onBeforeDeleteURI: function() {},
  onDeleteURI: function() {},
  onPageChanged: function() {},
  onDeleteVisits: function() {},

  
  

  notify: function PEX_timerCallback()
  {
    
    
    
    if (!this._isIdleObserver) {
      this._idle.addIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._isIdleObserver = true;
    }

    this._expireWithActionAndLimit(ACTION.TIMED, LIMIT.SMALL);
  },

  
  

  handleResult: function PEX_handleResult(aResultSet)
  {
    if (!("_expiredResults" in this))
      this._expiredResults = {};

    let row;
    while (row = aResultSet.getNextRow()) {

      this._expectedResultsCount = row.getResultByName("expected_results");

      let url = row.getResultByName("url");
      let wholeEntry = row.getResultByName("whole_entry");
      let visitDate = row.getResultByName("visit_date");

      if (url in this._expiredResults) {
        this._expiredResults[url].wholeEntry |= wholeEntry;
        this._expiredResults[url].visitDate =
          Math.max(this._expiredResults[url].visitDate, visitDate);
      }
      else {
        this._expiredResults[url] = {
          visitDate: visitDate,
          wholeEntry: wholeEntry,
        };
      }
    }
  },

  handleError: function PEX_handleError(aError)
  {
    Components.utils.reportError("Async statement execution returned with '" +
                                 aError.result + "', '" + aError.message + "'");
  },

  handleCompletion: function PEX_handleCompletion(aReason)
  {
    if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
      
      
      
      this._status = STATUS.UNKNOWN;
      if ("_expectedResultsCount" in this && "_expiredResults" in this) {
        let isClean = this._expiredResults.length < this._expectedResultsCount;
        let status = isClean ? STATUS.CLEAN : STATUS.DIRTY;
        
        if (this._status != status) {
          this._newTimer();
          this._status = status;
        }
      }
      delete this._expectedResultsCount;

      
      
      if ("_expiredResults" in this) {
        if (!this._shuttingDown) {
          
          for (let expiredUrl in this._expiredResults) {
            let entry = this._expiredResults[expiredUrl];
            this._hsn.notifyOnPageExpired(
              this._ios.newURI(expiredUrl, null, null),
              entry.visitDate,
              entry.wholeEntry);
          }
        }
        delete this._expiredResults;
      }

      
      this._os.notifyObservers(null, TOPIC_EXPIRATION_FINISHED, null);
    }
  },

  
  

  _urisLimit: PREF_MAX_URIS_NOTSET,
  _interval: PREF_INTERVAL_SECONDS_NOTSET,
  _status: STATUS.UNKNOWN,
  _isIdleObserver: false,
  _shuttingDown: false,

  _loadPrefs: function PEX__loadPrefs() {
    
    try {
      
      
      this._urisLimit = this._prefBranch.getIntPref(PREF_MAX_URIS);
    }
    catch(e) {}

    if (this._urisLimit < 0) {
      
      
      let memsize = this._sys.getProperty("memsize"); 
      let cpucount = this._sys.getProperty("cpucount"); 
      const AVG_SIZE_PER_URIENTRY = cpucount > 1 ? URIENTRY_AVG_SIZE_MIN
                                                 : URIENTRY_AVG_SIZE_MAX;
      
      
      let cache_percentage = PREF_DATABASE_CACHE_PER_MEMORY_PERCENTAGE_NOTSET;
      try {
        let prefs = Cc["@mozilla.org/preferences-service;1"].
                    getService(Ci.nsIPrefBranch);
        cache_percentage =
          prefs.getIntPref(PREF_DATABASE_CACHE_PER_MEMORY_PERCENTAGE);
        if (cache_percentage < 0) {
          cache_percentage = 0;
        }
        else if (cache_percentage > 50) {
          cache_percentage = 50;
        }
      }
      catch(e) {}
      let cachesize = memsize * cache_percentage / 100;
      this._urisLimit = Math.max(MIN_URIS,
                                 parseInt(cachesize / AVG_SIZE_PER_URIENTRY));
    }
    
    this._prefBranch.setIntPref(PREF_READONLY_CALCULATED_MAX_URIS,
                                this._urisLimit);

    
    try {
      
      
      this._interval = this._prefBranch.getIntPref(PREF_INTERVAL_SECONDS);
    }
    catch (e) {}
    if (this._interval <= 0)
      this._interval = PREF_INTERVAL_SECONDS_NOTSET;
  },

  





  _expireWithActionAndLimit:
  function PEX__expireWithActionAndLimit(aAction, aLimit)
  {
    
    if (this._inBatchMode)
      return;

    let boundStatements = [];
    for (let queryType in EXPIRATION_QUERIES) {
      if (EXPIRATION_QUERIES[queryType].actions & aAction)
        boundStatements.push(this._getBoundStatement(queryType, aLimit));
    }

    
    this._db.executeAsync(boundStatements, boundStatements.length, this);
  },

  



  _finalizeInternalStatements: function PEX__finalizeInternalStatements()
  {
    for each (let stmt in this._cachedStatements) {
      stmt.finalize();
    }
  },

  








  _cachedStatements: {},
  _getBoundStatement: function PEX__getBoundStatement(aQueryType, aLimit)
  {
    
    let stmt = this._cachedStatements[aQueryType];
    if (stmt === undefined) {
      stmt = this._cachedStatements[aQueryType] =
        this._db.createStatement(EXPIRATION_QUERIES[aQueryType].sql);
    }

    let baseLimit;
    switch (aLimit) {
      case LIMIT.UNLIMITED:
        baseLimit = -1;
        break;
      case LIMIT.SMALL:
        baseLimit = EXPIRE_LIMIT_PER_STEP;
        break;
      case LIMIT.LARGE:
        baseLimit = EXPIRE_LIMIT_PER_STEP * EXPIRE_LIMIT_PER_LARGE_STEP_MULTIPLIER;
        break;
      case LIMIT.DEBUG:
        baseLimit = this._debugLimit;
        break;
    }
    if (this._status == STATUS.DIRTY && aLimit != LIMIT.DEBUG)
      baseLimit *= EXPIRE_AGGRESSIVITY_MULTIPLIER;

    
    let params = stmt.params;
    switch (aQueryType) {
      case "QUERY_FIND_VISITS_TO_EXPIRE":
      case "QUERY_EXPIRE_VISITS":
        params.max_uris = this._urisLimit;
        params.limit_visits = baseLimit;
        break;
      case "QUERY_FIND_URIS_TO_EXPIRE":
      case "QUERY_EXPIRE_URIS":
        
        
        
        
        
        let max_place_id = MAX_INT64;
        let maxPlaceIdStmt = this._db.createStatement(
          "SELECT MAX(IFNULL((SELECT MAX(id) FROM moz_places_temp), 0), " +
                     "IFNULL((SELECT MAX(id) FROM moz_places), 0) " +
                    ") AS max_place_id");
        try {
          maxPlaceIdStmt.executeStep();
          max_place_id = maxPlaceIdStmt.getInt64(0);
        }
        catch(e) {}
        finally {
          maxPlaceIdStmt.finalize();
        }

        params.limit_uris = baseLimit;
        params.last_place_id = max_place_id;
        break;
      case "QUERY_EXPIRE_FAVICONS":
        params.limit_favicons = baseLimit;
        break;
      case "QUERY_EXPIRE_ANNOS":
        params.expire_never = Ci.nsIAnnotationService.EXPIRE_NEVER;
        params.limit_annos = baseLimit;
        break;
      case "QUERY_EXPIRE_ANNOS_WITH_POLICY":
      case "QUERY_EXPIRE_ITEMS_ANNOS_WITH_POLICY":
        let microNow = Date.now() * 1000;
        ANNOS_EXPIRE_POLICIES.forEach(function(policy) {
          params[policy.bind] = policy.type;
          params[policy.bind + "_time"] = microNow - policy.time;
        });
        break;
      case "QUERY_EXPIRE_ANNOS_WITH_HISTORY":
        params.expire_with_history = Ci.nsIAnnotationService.EXPIRE_WITH_HISTORY;
        break;
      case "QUERY_EXPIRE_ITEMS_ANNOS":
        params.limit_annos = baseLimit;
        break;
      case "QUERY_EXPIRE_ANNO_ATTRIBUTES":
        params.limit_annos = baseLimit;
        break;
      case "QUERY_EXPIRE_INPUTHISTORY":
        params.limit_inputhistory = baseLimit;
        break;
      case "QUERY_EXPIRE_ANNOS_SESSION":
      case "QUERY_EXPIRE_ITEMS_ANNOS_SESSION":
        params.expire_session = Ci.nsIAnnotationService.EXPIRE_SESSION;
        break;
    }

    return stmt;
  },

  




  _newTimer: function PEX__newTimer()
  {
    if (this._timer)
      this._timer.cancel();
    let interval = (this._status == STATUS.CLEAN) ?
      this._interval * EXPIRE_AGGRESSIVITY_MULTIPLIER : this._interval;

    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, interval * 1000,
                           Ci.nsITimer.TYPE_REPEATING_SLACK);
    return this._timer = timer;
  },

  
  

  classDescription: "Used to expire obsolete data from Places",
  classID: Components.ID("705a423f-2f69-42f3-b9fe-1517e0dee56f"),
  contractID: "@mozilla.org/places/expiration;1",

  
  
  _xpcom_categories: [
    { category: "history-observers" },
  ],

  _xpcom_factory: nsPlacesExpirationFactory,

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsINavHistoryObserver,
    Ci.nsITimerCallback,
    Ci.mozIStorageStatementCallback,
  ])
};




let components = [nsPlacesExpiration];
function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}
