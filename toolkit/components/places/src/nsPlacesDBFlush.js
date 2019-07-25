










































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;




const kTopicShutdown = "places-teardown";
const kSyncFinished = "places-sync-finished";
const kDebugStopSync = "places-debug-stop-sync";
const kDebugStartSync = "places-debug-start-sync";

const kSyncPrefName = "places.syncDBTableIntervalInSecs";
const kDefaultSyncInterval = 120;


const kQuerySyncPlacesId = 0;
const kQuerySyncHistoryVisitsId = 1;




Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PlacesUtils.jsm");




function nsPlacesDBFlush()
{
  
  try {
    
    
    this._syncInterval = Services.prefs.getIntPref(kSyncPrefName);
    if (this._syncInterval <= 0)
      this._syncInterval = kDefaultSyncInterval;
  }
  catch (e) {
    
    this._syncInterval = kDefaultSyncInterval;
  }

  
  Services.obs.addObserver(this, kTopicShutdown, false);
  Services.obs.addObserver(this, kDebugStopSync, false);
  Services.obs.addObserver(this, kDebugStartSync, false);

  let (pb2 = Services.prefs.QueryInterface(Ci.nsIPrefBranch2))
    pb2.addObserver(kSyncPrefName, this, false);

  
  this._timer = this._newTimer();

  
  

  XPCOMUtils.defineLazyGetter(this, "_db", function() {
    return PlacesUtils.history.QueryInterface(Ci.nsPIPlacesDatabase)
                              .DBConnection;
  });
}

nsPlacesDBFlush.prototype = {
  
  

  observe: function DBFlush_observe(aSubject, aTopic, aData)
  {
    if (aTopic == kTopicShutdown) {
      Services.obs.removeObserver(this, kTopicShutdown);
      Services.obs.removeObserver(this, kDebugStopSync);
      Services.obs.removeObserver(this, kDebugStartSync);

      let (pb2 = Services.prefs.QueryInterface(Ci.nsIPrefBranch2))
        pb2.removeObserver(kSyncPrefName, this);

      if (this._timer) {
        this._timer.cancel();
        this._timer = null;
      }

      
      
      
      
      Services.tm.mainThread.dispatch({
        _self: this,
        run: function() {
          
          this._self._flushWithQueries([kQuerySyncPlacesId, kQuerySyncHistoryVisitsId]);

          
          
          this._self._finalizeInternalStatements();
          this._self._db.asyncClose();
        }
      }, Ci.nsIThread.DISPATCH_NORMAL);
    }
    else if (aTopic == "nsPref:changed" && aData == kSyncPrefName) {
      
      this._syncInterval = Services.prefs.getIntPref(kSyncPrefName);
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
    
    
    if (!this._inBatchMode && aItemType == PlacesUtils.bookmarks.TYPE_BOOKMARK)
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
      
      Services.obs.notifyObservers(null, kSyncFinished, null);
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
      if (stmt instanceof Ci.mozIStorageAsyncStatement)
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
        
        
        
        
        this._cachedStatements[aQueryType] = this._db.createAsyncStatement(
          "DELETE FROM moz_historyvisits_temp " +
          "WHERE visit_type <> :transition_type"
        );
        break;

      case kQuerySyncPlacesId:
        
        
        
        this._cachedStatements[aQueryType] = this._db.createAsyncStatement(
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

  
  

  classID: Components.ID("c1751cfc-e8f1-4ade-b0bb-f74edfb8ef6a"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsINavBookmarkObserver,
    Ci.nsINavHistoryObserver,
    Ci.nsITimerCallback,
    Ci.mozIStorageStatementCallback,
  ])
};




let components = [nsPlacesDBFlush];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
