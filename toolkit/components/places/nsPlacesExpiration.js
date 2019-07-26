





















const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");





const TOPIC_SHUTDOWN = "places-will-close-connection";
const TOPIC_PREF_CHANGED = "nsPref:changed";
const TOPIC_DEBUG_START_EXPIRATION = "places-debug-start-expiration";
const TOPIC_EXPIRATION_FINISHED = "places-expiration-finished";
const TOPIC_IDLE_BEGIN = "idle";
const TOPIC_IDLE_END = "back";
const TOPIC_IDLE_DAILY = "idle-daily";


const PREF_BRANCH = "places.history.expiration.";








const PREF_MAX_URIS = "max_pages";
const PREF_MAX_URIS_NOTSET = -1; 



const PREF_READONLY_CALCULATED_MAX_URIS = "transient_current_max_pages";


const PREF_INTERVAL_SECONDS = "interval_seconds";
const PREF_INTERVAL_SECONDS_NOTSET = 3 * 60;




const DATABASE_TO_MEMORY_PERC = 4;


const DATABASE_TO_DISK_PERC = 2;


const DATABASE_MAX_SIZE = 167772160; 

const MEMSIZE_FALLBACK_BYTES = 268435456; 

const DISKSIZE_FALLBACK_BYTES = 268435456; 




const EXPIRE_LIMIT_PER_STEP = 6;

const EXPIRE_LIMIT_PER_LARGE_STEP_MULTIPLIER = 10;











const EXPIRE_AGGRESSIVITY_MULTIPLIER = 3;






const URIENTRY_AVG_SIZE = 1600;




const IDLE_TIMEOUT_SECONDS = 5 * 60;




const SHUTDOWN_WITH_RECENT_CLEARHISTORY_TIMEOUT_SECONDS = 10;



const ANALYZE_PAGES_THRESHOLD = 100;



const OVERLIMIT_PAGES_THRESHOLD = 1000;

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
  TIMED:           1 << 0, 
  TIMED_OVERLIMIT: 1 << 1, 
  TIMED_ANALYZE:   1 << 2, 
  CLEAR_HISTORY:   1 << 3, 
  SHUTDOWN_DIRTY:  1 << 4, 
  IDLE_DIRTY:      1 << 5, 
  IDLE_DAILY:      1 << 6, 
  DEBUG:           1 << 7, 
};


const EXPIRATION_QUERIES = {

  
  
  
  
  QUERY_FIND_VISITS_TO_EXPIRE: {
    sql: "INSERT INTO expiration_notify "
       +   "(v_id, url, guid, visit_date, expected_results) "
       + "SELECT v.id, h.url, h.guid, v.visit_date, :limit_visits "
       + "FROM moz_historyvisits v "
       + "JOIN moz_places h ON h.id = v.place_id "
       + "WHERE (SELECT COUNT(*) FROM moz_places) > :max_uris "
       + "AND visit_date < strftime('%s','now','localtime','start of day','-7 days','utc') * 1000000 "
       + "ORDER BY v.visit_date ASC "
       + "LIMIT :limit_visits",
    actions: ACTION.TIMED_OVERLIMIT | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_VISITS: {
    sql: "DELETE FROM moz_historyvisits WHERE id IN ( "
       +   "SELECT v_id FROM expiration_notify WHERE v_id NOTNULL "
       + ")",
    actions: ACTION.TIMED_OVERLIMIT | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  
  
  
  
  
  
  QUERY_FIND_URIS_TO_EXPIRE: {
    sql: "INSERT INTO expiration_notify "
       +   "(p_id, url, guid, visit_date, expected_results) "
       + "SELECT h.id, h.url, h.guid, h.last_visit_date, :limit_uris "
       + "FROM moz_places h "
       + "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
       + "LEFT JOIN moz_bookmarks b ON h.id = b.fk "
       + "WHERE h.last_visit_date IS NULL "
       +   "AND v.id IS NULL "
       +   "AND b.id IS NULL "
       +   "AND frecency <> -1 "
       + "LIMIT :limit_uris",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.SHUTDOWN_DIRTY |
             ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_URIS: {
    sql: "DELETE FROM moz_places WHERE id IN ( "
       +   "SELECT p_id FROM expiration_notify WHERE p_id NOTNULL "
       + ")",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.SHUTDOWN_DIRTY |
             ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },

  
  QUERY_SILENT_EXPIRE_ORPHAN_URIS: {
    sql: "DELETE FROM moz_places WHERE id IN ( "
       +   "SELECT h.id "
       +   "FROM moz_places h "
       +   "LEFT JOIN moz_historyvisits v ON h.id = v.place_id "
       +   "LEFT JOIN moz_bookmarks b ON h.id = b.fk "
       +   "WHERE h.last_visit_date IS NULL "
       +     "AND v.id IS NULL "
       +     "AND b.id IS NULL "
       +   "LIMIT :limit_uris "
       + ")",
    actions: ACTION.CLEAR_HISTORY
  },

  
  QUERY_EXPIRE_FAVICONS: {
    sql: "DELETE FROM moz_favicons WHERE id IN ( "
       +   "SELECT f.id FROM moz_favicons f "
       +   "LEFT JOIN moz_places h ON f.id = h.favicon_id "
       +   "WHERE h.favicon_id IS NULL "
       +   "LIMIT :limit_favicons "
       + ")",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.CLEAR_HISTORY |
             ACTION.SHUTDOWN_DIRTY | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS: {
    sql: "DELETE FROM moz_annos WHERE id in ( "
       +   "SELECT a.id FROM moz_annos a "
       +   "LEFT JOIN moz_places h ON a.place_id = h.id "
       +   "WHERE h.id IS NULL "
       +   "LIMIT :limit_annos "
       + ")",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.CLEAR_HISTORY |
             ACTION.SHUTDOWN_DIRTY | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS_WITH_POLICY: {
    sql: "DELETE FROM moz_annos "
       + "WHERE (expiration = :expire_days "
       +   "AND :expire_days_time > MAX(lastModified, dateAdded)) "
       +    "OR (expiration = :expire_weeks "
       +   "AND :expire_weeks_time > MAX(lastModified, dateAdded)) "
       +    "OR (expiration = :expire_months "
       +   "AND :expire_months_time > MAX(lastModified, dateAdded))",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.CLEAR_HISTORY |
             ACTION.SHUTDOWN_DIRTY | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ITEMS_ANNOS_WITH_POLICY: {
    sql: "DELETE FROM moz_items_annos "
       + "WHERE (expiration = :expire_days "
       +   "AND :expire_days_time > MAX(lastModified, dateAdded)) "
       +    "OR (expiration = :expire_weeks "
       +   "AND :expire_weeks_time > MAX(lastModified, dateAdded)) "
       +    "OR (expiration = :expire_months "
       +   "AND :expire_months_time > MAX(lastModified, dateAdded))",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.CLEAR_HISTORY |
             ACTION.SHUTDOWN_DIRTY | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS_WITH_HISTORY: {
    sql: "DELETE FROM moz_annos "
       + "WHERE expiration = :expire_with_history "
       +   "AND NOT EXISTS (SELECT id FROM moz_historyvisits "
       +                   "WHERE place_id = moz_annos.place_id LIMIT 1)",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.CLEAR_HISTORY |
             ACTION.SHUTDOWN_DIRTY | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ITEMS_ANNOS: {
    sql: "DELETE FROM moz_items_annos WHERE id IN ( "
       +   "SELECT a.id FROM moz_items_annos a "
       +   "LEFT JOIN moz_bookmarks b ON a.item_id = b.id "
       +   "WHERE b.id IS NULL "
       +   "LIMIT :limit_annos "
       + ")",
    actions: ACTION.CLEAR_HISTORY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNO_ATTRIBUTES: {
    sql: "DELETE FROM moz_anno_attributes WHERE id IN ( "
       +   "SELECT n.id FROM moz_anno_attributes n "
       +   "LEFT JOIN moz_annos a ON n.id = a.anno_attribute_id "
       +   "LEFT JOIN moz_items_annos t ON n.id = t.anno_attribute_id "
       +   "WHERE a.anno_attribute_id IS NULL "
       +     "AND t.anno_attribute_id IS NULL "
       +   "LIMIT :limit_annos"
       + ")",
    actions: ACTION.CLEAR_HISTORY | ACTION.SHUTDOWN_DIRTY | ACTION.IDLE_DIRTY |
             ACTION.IDLE_DAILY | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_INPUTHISTORY: {
    sql: "DELETE FROM moz_inputhistory WHERE place_id IN ( "
       +   "SELECT i.place_id FROM moz_inputhistory i "
       +   "LEFT JOIN moz_places h ON h.id = i.place_id "
       +   "WHERE h.id IS NULL "
       +   "LIMIT :limit_inputhistory "
       + ")",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.CLEAR_HISTORY |
             ACTION.SHUTDOWN_DIRTY | ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY |
             ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ANNOS_SESSION: {
    sql: "DELETE FROM moz_annos WHERE expiration = :expire_session",
    actions: ACTION.CLEAR_HISTORY | ACTION.DEBUG
  },

  
  QUERY_EXPIRE_ITEMS_ANNOS_SESSION: {
    sql: "DELETE FROM moz_items_annos WHERE expiration = :expire_session",
    actions: ACTION.CLEAR_HISTORY | ACTION.DEBUG
  },

  
  
  
  QUERY_SELECT_NOTIFICATIONS: {
    sql: "SELECT url, guid, MAX(visit_date) AS visit_date, "
       +        "MAX(IFNULL(MIN(p_id, 1), MIN(v_id, 0))) AS whole_entry, "
       +        "expected_results "
       + "FROM expiration_notify "
       + "GROUP BY url",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.SHUTDOWN_DIRTY |
             ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },

  
  QUERY_DELETE_NOTIFICATIONS: {
    sql: "DELETE FROM expiration_notify",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.SHUTDOWN_DIRTY |
             ACTION.IDLE_DIRTY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },

  
  
  
  
  

  QUERY_ANALYZE_MOZ_PLACES: {
    sql: "ANALYZE moz_places",
    actions: ACTION.TIMED_OVERLIMIT | ACTION.TIMED_ANALYZE |
             ACTION.CLEAR_HISTORY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },
  QUERY_ANALYZE_MOZ_BOOKMARKS: {
    sql: "ANALYZE moz_bookmarks",
    actions: ACTION.TIMED_ANALYZE | ACTION.IDLE_DAILY | ACTION.DEBUG
  },
  QUERY_ANALYZE_MOZ_HISTORYVISITS: {
    sql: "ANALYZE moz_historyvisits",
    actions: ACTION.TIMED_OVERLIMIT | ACTION.TIMED_ANALYZE |
             ACTION.CLEAR_HISTORY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },
  QUERY_ANALYZE_MOZ_INPUTHISTORY: {
    sql: "ANALYZE moz_inputhistory",
    actions: ACTION.TIMED | ACTION.TIMED_OVERLIMIT | ACTION.TIMED_ANALYZE |
             ACTION.CLEAR_HISTORY | ACTION.IDLE_DAILY | ACTION.DEBUG
  },
};




function nsPlacesExpiration()
{
  
  

  XPCOMUtils.defineLazyGetter(this, "_db", function () {
    let db = Cc["@mozilla.org/browser/nav-history-service;1"].
             getService(Ci.nsPIPlacesDatabase).
             DBConnection;

    
    let stmt = db.createAsyncStatement(
      "CREATE TEMP TABLE expiration_notify ( "
    + "  id INTEGER PRIMARY KEY "
    + ", v_id INTEGER "
    + ", p_id INTEGER "
    + ", url TEXT NOT NULL "
    + ", guid TEXT NOT NULL "
    + ", visit_date INTEGER "
    + ", expected_results INTEGER NOT NULL "
    + ") ");
    stmt.executeAsync();
    stmt.finalize();

    return db;
  });

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
                     getBranch(PREF_BRANCH);
  this._loadPrefs();

  
  this._prefBranch.addObserver("", this, false);

  
  Services.obs.addObserver(this, TOPIC_SHUTDOWN, false);
  Services.obs.addObserver(this, TOPIC_DEBUG_START_EXPIRATION, false);
  Services.obs.addObserver(this, TOPIC_IDLE_DAILY, false);

  
  this._newTimer();
}

nsPlacesExpiration.prototype = {

  
  

  observe: function PEX_observe(aSubject, aTopic, aData)
  {
    if (aTopic == TOPIC_SHUTDOWN) {
      this._shuttingDown = true;
      Services.obs.removeObserver(this, TOPIC_SHUTDOWN);
      Services.obs.removeObserver(this, TOPIC_DEBUG_START_EXPIRATION);
      Services.obs.removeObserver(this, TOPIC_IDLE_DAILY);

      this._prefBranch.removeObserver("", this);

      this.expireOnIdle = false;

      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }

      
      
      let hasRecentClearHistory =
        Date.now() - this._lastClearHistoryTime <
          SHUTDOWN_WITH_RECENT_CLEARHISTORY_TIMEOUT_SECONDS * 1000;
      if (!hasRecentClearHistory && this.status == STATUS.DIRTY) {
        this._expireWithActionAndLimit(ACTION.SHUTDOWN_DIRTY, LIMIT.LARGE);
      }

      this._finalizeInternalStatements();
    }
    else if (aTopic == TOPIC_PREF_CHANGED) {
      this._loadPrefs();

      if (aData == PREF_INTERVAL_SECONDS) {
        
        this._newTimer();
      }
    }
    else if (aTopic == TOPIC_DEBUG_START_EXPIRATION) {
      
      
      let limit = parseInt(aData);
      if (limit == -1) {
        
        
        
        this._expireWithActionAndLimit(ACTION.DEBUG, LIMIT.UNLIMITED);
      }
      else if (limit > 0) {
        
        
        this._debugLimit = limit;
        this._expireWithActionAndLimit(ACTION.DEBUG, LIMIT.DEBUG);
      }
      else {
        
        
        
        
        this._debugLimit = -1;
        this._expireWithActionAndLimit(ACTION.DEBUG, LIMIT.DEBUG);
      }
    }
    else if (aTopic == TOPIC_IDLE_BEGIN) {
      
      
      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }
      if (this.expireOnIdle)
        this._expireWithActionAndLimit(ACTION.IDLE_DIRTY, LIMIT.LARGE);
    }
    else if (aTopic == TOPIC_IDLE_END) {
      
      if (!this._timer)
        this._newTimer();
    }
    else if (aTopic == TOPIC_IDLE_DAILY) {
      this._expireWithActionAndLimit(ACTION.IDLE_DAILY, LIMIT.LARGE);
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
    
    this.status = STATUS.CLEAN;
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
    
    this._getPagesStats((function onPagesCount(aPagesCount, aStatsCount) {
      let overLimitPages = aPagesCount - this._urisLimit;
      this._overLimit = overLimitPages > 0;

      let action = this._overLimit ? ACTION.TIMED_OVERLIMIT : ACTION.TIMED;
      
      
      if (Math.abs(aPagesCount - aStatsCount) >= ANALYZE_PAGES_THRESHOLD) {
        action = action | ACTION.TIMED_ANALYZE;
      }

      
      let limit = overLimitPages > OVERLIMIT_PAGES_THRESHOLD ? LIMIT.LARGE
                                                             : LIMIT.SMALL;

      this._expireWithActionAndLimit(action, limit);
    }).bind(this));
  },

  
  

  handleResult: function PEX_handleResult(aResultSet)
  {
    
    if (this._shuttingDown)
      return;

    let row;
    while (row = aResultSet.getNextRow()) {
      if (!("_expectedResultsCount" in this))
        this._expectedResultsCount = row.getResultByName("expected_results");
      if (this._expectedResultsCount > 0)
        this._expectedResultsCount--;

      let uri = Services.io.newURI(row.getResultByName("url"), null, null);
      let guid = row.getResultByName("guid");
      let visitDate = row.getResultByName("visit_date");
      let wholeEntry = row.getResultByName("whole_entry");
      
      this._hsn.notifyOnPageExpired(uri, visitDate, wholeEntry, guid,
                                    Ci.nsINavHistoryObserver.REASON_EXPIRED);
    }
  },

  handleError: function PEX_handleError(aError)
  {
    Cu.reportError("Async statement execution returned with '" +
                   aError.result + "', '" + aError.message + "'");
  },

  
  _telemetrySteps: 1,
  handleCompletion: function PEX_handleCompletion(aReason)
  {
    if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
      if ("_expectedResultsCount" in this) {
        
        
        
        let oldStatus = this.status;
        this.status = this._expectedResultsCount == 0 ? STATUS.DIRTY
                                                      : STATUS.CLEAN;

        
        if (this.status == STATUS.DIRTY) {
          this._telemetrySteps++;
        }
        else {
          
          
          if (oldStatus == STATUS.DIRTY) {
            try {
              Services.telemetry
                      .getHistogramById("PLACES_EXPIRATION_STEPS_TO_CLEAN2")
                      .add(this._telemetrySteps);
            } catch (ex) {
              Components.utils.reportError("Unable to report telemetry.");
            }
          }
          this._telemetrySteps = 1;
        }

        delete this._expectedResultsCount;
      }

      
      Services.obs.notifyObservers(null, TOPIC_EXPIRATION_FINISHED, null);
    }
  },

  
  

  _urisLimit: PREF_MAX_URIS_NOTSET,
  _interval: PREF_INTERVAL_SECONDS_NOTSET,
  _shuttingDown: false,

  _status: STATUS.UNKNOWN,
  set status(aNewStatus) {
    if (aNewStatus != this._status) {
      
      this._status = aNewStatus;
      this._newTimer();
      
      
      this.expireOnIdle = aNewStatus == STATUS.DIRTY;
    }
    return aNewStatus;
  },
  get status() this._status,

  _isIdleObserver: false,
  _expireOnIdle: false,
  set expireOnIdle(aExpireOnIdle) {
    
    
    if (!this._isIdleObserver && !this._shuttingDown) {
      this._idle.addIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._isIdleObserver = true;
    }
    else if (this._isIdleObserver && this._shuttingDown) {
      this._idle.removeIdleObserver(this, IDLE_TIMEOUT_SECONDS);
      this._isIdleObserver = false;
    }

    
    
    
    if (this._debugLimit !== undefined)
      this._expireOnIdle = false;
    else
      this._expireOnIdle = aExpireOnIdle;
    return this._expireOnIdle;
  },
  get expireOnIdle() this._expireOnIdle,

  _loadPrefs: function PEX__loadPrefs() {
    
    try {
      
      
      this._urisLimit = this._prefBranch.getIntPref(PREF_MAX_URIS);
    }
    catch(e) {}

    if (this._urisLimit < 0) {
      
      
      
      

      let memSizeBytes = MEMSIZE_FALLBACK_BYTES;
      try {
        
         memSizeBytes = this._sys.getProperty("memsize");
      } catch (ex) {}
      if (memSizeBytes <= 0) {
        memsize = MEMSIZE_FALLBACK_BYTES;
      }

      let diskAvailableBytes = DISKSIZE_FALLBACK_BYTES;
      try {
        
        let dbFile = this._db.databaseFile;
        dbFile.QueryInterface(Ci.nsILocalFile);
        diskAvailableBytes = dbFile.diskSpaceAvailable;
      } catch (ex) {}
      if (diskAvailableBytes <= 0) {
        diskAvailableBytes = DISKSIZE_FALLBACK_BYTES;
      }

      let optimalDatabaseSize = Math.min(
        memSizeBytes * DATABASE_TO_MEMORY_PERC / 100,
        diskAvailableBytes * DATABASE_TO_DISK_PERC / 100,
        DATABASE_MAX_SIZE
      );

      this._urisLimit = Math.ceil(optimalDatabaseSize / URIENTRY_AVG_SIZE);
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

  






  _getPagesStats: function PEX__getPagesStats(aCallback) {
    if (!this._cachedStatements["LIMIT_COUNT"]) {
      this._cachedStatements["LIMIT_COUNT"] = this._db.createAsyncStatement(
        "SELECT (SELECT COUNT(*) FROM moz_places), "
      +        "(SELECT SUBSTR(stat,1,LENGTH(stat)-2) FROM sqlite_stat1 "
      +         "WHERE idx = 'moz_places_url_uniqueindex')"
      );
    }
    this._cachedStatements["LIMIT_COUNT"].executeAsync({
      _pagesCount: 0,
      _statsCount: 0,
      handleResult: function(aResults) {
        let row = aResults.getNextRow();
        this._pagesCount = row.getResultByIndex(0);
        this._statsCount = row.getResultByIndex(1);
      },
      handleCompletion: function (aReason) {
        if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
          aCallback(this._pagesCount, this._statsCount);
        }
      },
      handleError: function(aError) {
        Cu.reportError("Async statement execution returned with '" +
                       aError.result + "', '" + aError.message + "'");
      }
    });
  },

  








  _expireWithActionAndLimit:
  function PEX__expireWithActionAndLimit(aAction, aLimit)
  {
    
    if (this._inBatchMode)
      return;
    
    if (this._shuttingDown && aAction != ACTION.SHUTDOWN_DIRTY) {
      return;
    }

    let boundStatements = [];
    for (let queryType in EXPIRATION_QUERIES) {
      if (EXPIRATION_QUERIES[queryType].actions & aAction)
        boundStatements.push(this._getBoundStatement(queryType, aLimit, aAction));
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
  _getBoundStatement: function PEX__getBoundStatement(aQueryType, aLimit, aAction)
  {
    
    let stmt = this._cachedStatements[aQueryType];
    if (stmt === undefined) {
      stmt = this._cachedStatements[aQueryType] =
        this._db.createAsyncStatement(EXPIRATION_QUERIES[aQueryType].sql);
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
    if (this.status == STATUS.DIRTY && aAction != ACTION.DEBUG &&
        baseLimit > 0) {
      baseLimit *= EXPIRE_AGGRESSIVITY_MULTIPLIER;
    }

    
    let params = stmt.params;
    switch (aQueryType) {
      case "QUERY_FIND_VISITS_TO_EXPIRE":
        params.max_uris = this._urisLimit;
        
        
        params.limit_visits =
          aLimit == LIMIT.DEBUG && baseLimit == -1 ? 0 : baseLimit;
        break;
      case "QUERY_FIND_URIS_TO_EXPIRE":
        params.limit_uris = baseLimit;
        break;
      case "QUERY_SILENT_EXPIRE_ORPHAN_URIS":
        params.limit_uris = baseLimit;
        break;
      case "QUERY_EXPIRE_FAVICONS":
        params.limit_favicons = baseLimit;
        break;
      case "QUERY_EXPIRE_ANNOS":
        
        params.limit_annos = baseLimit * EXPIRE_AGGRESSIVITY_MULTIPLIER;
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
    if (this._shuttingDown)
      return;
    let interval = this.status != STATUS.DIRTY ?
      this._interval * EXPIRE_AGGRESSIVITY_MULTIPLIER : this._interval;

    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, interval * 1000,
                           Ci.nsITimer.TYPE_REPEATING_SLACK);
    return this._timer = timer;
  },

  
  

  classID: Components.ID("705a423f-2f69-42f3-b9fe-1517e0dee56f"),

  _xpcom_factory: XPCOMUtils.generateSingletonFactory(nsPlacesExpiration),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver
  , Ci.nsINavHistoryObserver
  , Ci.nsITimerCallback
  , Ci.mozIStorageStatementCallback
  ])
};




let components = [nsPlacesExpiration];
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
