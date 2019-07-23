







































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const kQuitApplication = "quit-application";
const kSyncFinished = "places-sync-finished";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const kDefaultSyncInterval = 120;




function nsPlacesDBFlush()
{
  this._prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefService).
                getBranch("places.");

  
  try {
    
    
    this._syncInterval = this._prefs.getIntPref(kSyncPrefName);
    if (this._syncInterval <= 0)
      this._syncInterval = kDefaultSyncInterval;
  }
  catch (e) {
    
    this._syncInterval = kDefaultSyncInterval;
  }

  
  this._bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
             getService(Ci.nsINavBookmarksService);
  this._bs.addObserver(this, false);

  this._os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
  this._os.addObserver(this, kQuitApplication, false);

  this._prefs.QueryInterface(Ci.nsIPrefBranch2)
             .addObserver("", this, false);

  
  this._timer = this._newTimer();

  
  

  this.__defineGetter__("_db", function() {
    delete this._db;
    return this._db = Cc["@mozilla.org/browser/nav-history-service;1"].
                      getService(Ci.nsPIPlacesDatabase).
                      DBConnection;
  });

}

nsPlacesDBFlush.prototype = {
  
  

  observe: function DBFlush_observe(aSubject, aTopic, aData)
  {
    if (aTopic == kQuitApplication) {
      this._bs.removeObserver(this);
      this._os.removeObserver(this, kQuitApplication);
      this._prefs.QueryInterface(Ci.nsIPrefBranch2).removeObserver("", this);
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
          this._self._syncTables(["places", "historyvisits"]);
          
          
          pip.finalizeInternalStatements();
          this._self._cachedStatements.forEach(function(stmt) stmt.finalize());
          this._self._db.close();
        }
      }, Ci.nsIThread.DISPATCH_NORMAL);

    }
    else if (aTopic == "nsPref:changed" && aData == kSyncPrefName) {
      
      this._syncInterval = aSubject.getIntPref(kSyncPrefName);
      if (this._syncInterval <= 0)
        this._syncInterval = kDefaultSyncInterval;

      
      
      if (!this._timer)
        return;

      this._timer.cancel();
      this._timer = this._newTimer();
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

    
    this._syncTables(["places", "historyvisits"]);
  },

  onItemAdded: function(aItemId, aParentId, aIndex)
  {
    
    if (!this._inBatchMode &&
        this._bs.getItemType(aItemId) == this._bs.TYPE_BOOKMARK)
      this._syncTables(["places"]);
  },

  onItemChanged: function DBFlush_onItemChanged(aItemId, aProperty,
                                                         aIsAnnotationProperty,
                                                         aValue)
  {
    if (!this._inBatchMode && aProperty == "uri")
      this._syncTables(["places"]);
  },

  onItemRemoved: function() { },
  onItemVisited: function() { },
  onItemMoved: function() { },

  
  

  notify: function() this._syncTables(["places", "historyvisits"]),

  
  

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

  




  _syncTables: function DBFlush_syncTables(aTableNames)
  {
    
    if (this._inBatchMode)
      return;

    let statements = [];
    for (let i = 0; i < aTableNames.length; i++)
      statements.push(this._getSyncTableStatement(aTableNames[i]));

    
    this._db.executeAsync(statements, statements.length, this);
  },

  








  _cachedStatements: [],
  _getSyncTableStatement: function DBFlush_getSyncTableStatement(aTableName)
  {
    
    if (aTableName in this._cachedStatements)
      return this._cachedStatements[aTableName];

    
    
    
    let condition = "";
    switch(aTableName) {
      case "historyvisits":
        
        
        condition = "WHERE visit_type <> " + Ci.nsINavHistoryService.TRANSITION_EMBED;
        break;
      case "places":
        
        
        
        condition = "WHERE id IN (SELECT id FROM moz_places_temp h " +
                                  "WHERE h.hidden <> 1 OR NOT EXISTS ( " +
                                    "SELECT id FROM moz_historyvisits_temp " +
                                    "WHERE place_id = h.id AND visit_type = " +
                                    Ci.nsINavHistoryService.TRANSITION_EMBED +
                                    " LIMIT 1) " +
                                  ")";
        break;
    }

    let sql = "DELETE FROM moz_" + aTableName + "_temp " + condition;
    return this._cachedStatements[aTableName] = this._db.createStatement(sql);
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
  _xpcom_categories: [{
    category: "profile-after-change",
  }],

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsINavBookmarkObserver,
    Ci.nsITimerCallback,
    Ci.mozIStorageStatementCallback,
  ])
};




let components = [nsPlacesDBFlush];
function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}
