







































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const kTopicShutdown = "places-shutdown";
const kSyncFinished = "places-sync-finished";
const kDebugStopSync = "places-debug-stop-sync";
const kDebugStartSync = "places-debug-start-sync";

const kSyncPrefName = "places.syncDBTableIntervalInSecs";
const kDefaultSyncInterval = 120;


const kQuerySyncPlacesId = 0;
const kQuerySyncHistoryVisitsId = 1;




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

  
  this._os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
  this._os.addObserver(this, kTopicShutdown, false);
  this._os.addObserver(this, kDebugStopSync, false);
  this._os.addObserver(this, kDebugStartSync, false);

  let (pb2 = this._prefs.QueryInterface(Ci.nsIPrefBranch2))
    pb2.addObserver(kSyncPrefName, this, false);

  
  this._timer = this._newTimer();

  
  

  XPCOMUtils.defineLazyGetter(this, "_db", function() {
    return Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsPIPlacesDatabase).
           DBConnection;
  });

  XPCOMUtils.defineLazyServiceGetter(this, "_ios",
                                     "@mozilla.org/network/io-service;1",
                                     "nsIIOService");

  XPCOMUtils.defineLazyServiceGetter(this, "_bs",
                                     "@mozilla.org/browser/nav-bookmarks-service;1",
                                     "nsINavBookmarksService");
}

nsPlacesDBFlush.prototype = {
  
  

  observe: function DBFlush_observe(aSubject, aTopic, aData)
  {
    if (aTopic == kTopicShutdown) {
      this._os.removeObserver(this, kTopicShutdown);
      this._os.removeObserver(this, kDebugStopSync);
      this._os.removeObserver(this, kDebugStartSync);

      let (pb2 = this._prefs.QueryInterface(Ci.nsIPrefBranch2))
        pb2.removeObserver(kSyncPrefName, this);

      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }

      
      
      
      
      let tm = Cc["@mozilla.org/thread-manager;1"].
          getService(Ci.nsIThreadManager);
      tm.mainThread.dispatch({
        _self: this,
        run: function() {
          
          this._self._flushWithQueries([kQuerySyncPlacesId, kQuerySyncHistoryVisitsId]);

          
          
          this._self._finalizeInternalStatements();
          this._self._db.asyncClose();
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
    else if (aTopic == kDebugStopSync) {
      this._syncStopped = true;
    }
    else if (aTopic == kDebugStartSync) {
      if (_syncStopped in this)
        delete this._syncStopped;
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

  onItemAdded: function(aItemId, aParentId, aIndex, aItemType)
  {
    
    
    if (!this._inBatchMode && aItemType == this._bs.TYPE_BOOKMARK)
      this._flushWithQueries([kQuerySyncPlacesId]);
  },

  onItemChanged: function DBFlush_onItemChanged(aItemId, aProperty,
                                                aIsAnnotationProperty,
                                                aNewValue, aLastModified,
                                                aItemType)
  {
    if (!this._inBatchMode && aProperty == "uri")
      this._flushWithQueries([kQuerySyncPlacesId]);
  },

  onBeforeItemRemoved: function() { },
  onItemRemoved: function() { },
  onItemVisited: function() { },
  onItemMoved: function() { },

  
  

  
  
  

  
  
  
  
  onVisit: function() { },
  onTitleChanged: function() { },
  onBeforeDeleteURI: function() { },
  onDeleteURI: function() { },
  onClearHistory: function() { },
  onPageChanged: function() { },
  onDeleteVisits: function() { },

  
  

  notify: function DBFlush_timerCallback()
  {
    let queries = [
      kQuerySyncPlacesId,
      kQuerySyncHistoryVisitsId,
    ];
    this._flushWithQueries(queries);
  },

  
  

  handleResult: function DBFlush_handleResult(aResultSet)
  {
  },

  handleError: function DBFlush_handleError(aError)
  {
    Cu.reportError("Async statement execution returned with '" +
                   aError.result + "', '" + aError.message + "'");
  },

  handleCompletion: function DBFlush_handleCompletion(aReason)
  {
    if (aReason == Ci.mozIStorageStatementCallback.REASON_FINISHED) {
      
      this._os.notifyObservers(null, kSyncFinished, null);
    }
  },

  
  
  _syncInterval: kDefaultSyncInterval,

  





  _flushWithQueries: function DBFlush_flushWithQueries(aQueryNames)
  {
    
    if (this._inBatchMode || this._syncStopped)
      return;

    let statements = [];
    for (let i = 0; i < aQueryNames.length; i++)
      statements.push(this._getQuery(aQueryNames[i]));

    
    this._db.executeAsync(statements, statements.length, this);
  },

  



  _finalizeInternalStatements: function DBFlush_finalizeInternalStatements()
  {
    this._cachedStatements.forEach(function(stmt) {
      if (stmt instanceof Ci.mozIStorageStatement)
        stmt.finalize();
    });
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
