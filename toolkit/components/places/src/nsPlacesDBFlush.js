







































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const kQuitApplication = "quit-application";
const kSyncFinished = "places-sync-finished";

const kSyncPrefName = "places.syncDBTableIntervalInSecs";
const kDefaultSyncInterval = 120;
const kExpireDaysPrefName = "browser.history_expire_days";
const kDefaultExpireDays = 90;


const kMSPerDay = 86400000;


const kMaxExpire = 24;


const kQuerySyncPlacesId = 0;
const kQuerySyncHistoryVisitsId = 1;
const kQuerySelectExpireVisitsId = 2;
const kQueryExpireVisitsId = 3;




function nsPlacesDBFlush()
{
  this._prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);

  
  try {
    
    
    this._syncInterval = this._prefs.getIntPref(kSyncPrefName);
    if (this._syncInterval <= 0)
      this._syncInterval = kDefaultSyncInterval;
  }
  catch (e) {
    
    this._syncInterval = kDefaultSyncInterval;
  }

  
  try {
    
    
    this._expireDays = this._prefs.getIntPref(kExpireDaysPrefName);
    if (this._expireDays <= 0)
      this._expireDays = kDefaultExpireDays;
  }
  catch (e) {
    
    this._expireDays = kDefaultExpireDays;
  }

  
  this._os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
  this._os.addObserver(this, kQuitApplication, false);

  let (pb2 = this._prefs.QueryInterface(Ci.nsIPrefBranch2)) {
    pb2.addObserver(kSyncPrefName, this, false);
    pb2.addObserver(kExpireDaysPrefName, this, false);
  }

  
  this._timer = this._newTimer();

  
  

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

  XPCOMUtils.defineLazyServiceGetter(this, "_bs",
                                     "@mozilla.org/browser/nav-bookmarks-service;1",
                                     "nsINavBookmarksService");
}

nsPlacesDBFlush.prototype = {
  
  

  observe: function DBFlush_observe(aSubject, aTopic, aData)
  {
    if (aTopic == kQuitApplication) {
      this._os.removeObserver(this, kQuitApplication);
      let (pb2 = this._prefs.QueryInterface(Ci.nsIPrefBranch2)) {
        pb2.removeObserver(kSyncPrefName, this);
        pb2.removeObserver(kExpireDaysPrefName, this);
      }
      this._timer.cancel();
      this._timer = null;
      
      
      
      
      let tm = Cc["@mozilla.org/thread-manager;1"].
          getService(Ci.nsIThreadManager);
      tm.mainThread.dispatch({
        _self: this,
        run: function() {
          let pip = Cc["@mozilla.org/browser/nav-history-service;1"].
                    getService(Ci.nsPIPlacesDatabase);
          pip.commitPendingChanges();
          this._self._flushWithQueries([kQuerySyncPlacesId, kQuerySyncHistoryVisitsId]);
          
          
          pip.finalizeInternalStatements();
          this._self._finalizeInternalStatements();
          this._self._db.close();
        }
      }, Ci.nsIThread.DISPATCH_NORMAL);

    }
    else if (aTopic == "nsPref:changed" && aData == kSyncPrefName) {
      
      this._syncInterval = this._prefs.getIntPref(kSyncPrefName);
      if (this._syncInterval <= 0)
        this._syncInterval = kDefaultSyncInterval;

      
      
      if (!this._timer)
        return;

      this._timer.cancel();
      this._timer = this._newTimer();
    }
    else if (aTopic == "nsPref:changed" && aData == kExpireDaysPrefName) {
      
      this._expireDays = this._prefs.getIntPref(kExpireDaysPrefName);
      if (this._expireDays <= 0)
        this._expireDays = kDefaultExpireDays;
    }
  },

  
  

  onBeginUpdateBatch: function DBFlush_onBeginUpdateBatch()
  {
    this._inBatchMode = true;

    
    this._timer.cancel();
    this._timer = null;
  },

  onEndUpdateBatch: function DBFlush_onEndUpdateBatch()
  {
    this._inBatchMode = false;

    
    this._timer = this._newTimer();

    
    this._flushWithQueries([kQuerySyncPlacesId, kQuerySyncHistoryVisitsId]);
  },

  onItemAdded: function(aItemId, aParentId, aIndex)
  {
    
    
    if (!this._inBatchMode &&
        this._bs.getItemType(aItemId) == this._bs.TYPE_BOOKMARK)
      this._flushWithQueries([kQuerySyncPlacesId]);
  },

  onItemChanged: function DBFlush_onItemChanged(aItemId, aProperty,
                                                aIsAnnotationProperty, aValue)
  {
    if (!this._inBatchMode && aProperty == "uri")
      this._flushWithQueries([kQuerySyncPlacesId]);
  },

  onBeforeItemRemoved: function() { },
  onItemRemoved: function() { },
  onItemVisited: function() { },
  onItemMoved: function() { },

  
  

  
  
  

  
  
  
  
  onVisit: function(aURI, aVisitID, aTime, aSessionID, aReferringID, aTransitionType) { },
  onTitleChanged: function(aURI, aPageTitle) { },
  onBeforeDeleteURI: function(aURI) { },
  onDeleteURI: function(aURI) { },
  onClearHistory: function() { },
  onPageChanged: function(aURI, aWhat, aValue) { },
  onPageExpired: function(aURI, aVisitTime, aWholeEntry) { },

  
  

  notify: function DBFlush_timerCallback()
  {
    let queries = [
      kQuerySelectExpireVisitsId,
      kQueryExpireVisitsId,
      kQuerySyncPlacesId,
      kQuerySyncHistoryVisitsId,
    ];
    this._flushWithQueries(queries);
  },

  
  

  handleResult: function DBFlush_handleResult(aResultSet)
  {
    
    if (!this._expiredResults)
      this._expiredResults = [];

    let row;
    while (row = aResultSet.getNextRow()) {
      if (row.getResultByName("hidden"))
        continue;

      this._expiredResults.push({
        uri: this._ios.newURI(row.getResultByName("url"), null, null),
        visitDate: row.getResultByName("visit_date"),
        wholeEntry: (row.getResultByName("visit_count") == 1)
      });
    }
  },

  handleError: function DBFlush_handleError(aError)
  {
    Cu.reportError("Async statement execution returned with '" +
                   aError.result + "', '" + aError.message + "'");
  },

  handleCompletion: function DBFlush_handleCompletion(aReason)
  {
    if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
      
      if (this._expiredResults) {
        while (this._expiredResults.length) {
          let visit = this._expiredResults.shift();
          this._hsn.notifyOnPageExpired(visit.uri, visit.visitDate,
                                        visit.wholeEntry);
        }

        
        delete this._expiredResults;
      }

      
      this._os.notifyObservers(null, kSyncFinished, null);
    }
  },

  
  
  _syncInterval: kDefaultSyncInterval,

  





  _flushWithQueries: function DBFlush_flushWithQueries(aQueryNames)
  {
    
    if (this._inBatchMode)
      return;

    let statements = [];
    for (let i = 0; i < aQueryNames.length; i++)
      statements.push(this._getQuery(aQueryNames[i]));

    
    this._db.executeAsync(statements, statements.length, this);
  },

  



  _finalizeInternalStatements: function DBFlush_finalizeInternalStatements()
  {
    for each (let stmt in this._cachedStatements)
      if (stmt instanceof Ci.mozIStorageStatement)
        stmt.finalize();
  },

  









  _cachedStatements: [],
  _getQuery: function DBFlush_getQuery(aQueryType)
  {
    
    if (aQueryType in this._cachedStatements) {
      let stmt = this._cachedStatements[aQueryType];

      
      let params = stmt.params;
      switch (aQueryType) {
        case kQuerySyncHistoryVisitsId:
        case kQuerySyncPlacesId:
          params.transition_type = Ci.nsINavHistoryService.TRANSITION_EMBED;
          break;
        case kQuerySelectExpireVisitsId:
        case kQueryExpireVisitsId:
          params.visit_date = (Date.now() - (this._expireDays * kMSPerDay)) * 1000;
          params.max_expire = kMaxExpire;
          break;
      }

      return stmt;
    }

    switch(aQueryType) {
      case kQuerySyncHistoryVisitsId:
        
        
        this._cachedStatements[aQueryType] = this._db.createStatement(
          "DELETE FROM moz_historyvisits_temp " +
          "WHERE visit_type <> :transition_type"
        );
        break;

      case kQuerySyncPlacesId:
        
        
        
        this._cachedStatements[aQueryType] = this._db.createStatement(
          "DELETE FROM moz_places_temp " +
          "WHERE id IN ( " +
            "SELECT id FROM moz_places_temp h " +
            "WHERE h.hidden <> 1 OR NOT EXISTS ( " +
              "SELECT id FROM moz_historyvisits_temp " +
              "WHERE place_id = h.id AND visit_type = :transition_type " +
              "LIMIT 1 " +
            ") " +
          ")"
        );
        break;

      case kQuerySelectExpireVisitsId:
        
        
        this._cachedStatements[aQueryType] = this._db.createStatement(
          "SELECT h.url, v.visit_date, h.hidden, h.visit_count " +
          "FROM moz_places h " +
          "JOIN moz_historyvisits v ON h.id = v.place_id " +
          "WHERE v.visit_date < :visit_date " +
          "ORDER BY v.visit_date ASC " +
          "LIMIT :max_expire"
        );
        break;

      case kQueryExpireVisitsId:
        
        this._cachedStatements[aQueryType] = this._db.createStatement(
          "DELETE FROM moz_historyvisits " +
          "WHERE id IN ( " +
            "SELECT id " +
            "FROM moz_historyvisits " +
            "WHERE visit_date < :visit_date " +
            "ORDER BY visit_date ASC " +
            "LIMIT :max_expire " +
          ")"
        );
        break;

      default:
        throw "Unexpected statement!";
    }

    
    
    return this._getQuery(aQueryType);
  },

  




  _newTimer: function DBFlush_newTimer()
  {
    let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    timer.initWithCallback(this, this._syncInterval * 1000,
                           Ci.nsITimer.TYPE_REPEATING_SLACK);
    return timer;
  },

  
  

  classDescription: "Used to synchronize the temporary and permanent tables of Places",
  classID: Components.ID("c1751cfc-e8f1-4ade-b0bb-f74edfb8ef6a"),
  contractID: "@mozilla.org/places/sync;1",

  
  
  _xpcom_categories: [
    { category: "bookmark-observers" },
    { category: "history-observers" },
  ],

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsINavBookmarkObserver,
    Ci.nsINavHistoryObserver,
    Ci.nsITimerCallback,
    Ci.mozIStorageStatementCallback,
  ])
};




let components = [nsPlacesDBFlush];
function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}
