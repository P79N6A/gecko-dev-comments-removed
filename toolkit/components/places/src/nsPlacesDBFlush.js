






































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/PlacesBackground.jsm");




const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const kPlacesBackgroundShutdown = "places-background-shutdown";

const kSyncPrefName = "syncDBTableIntervalInSecs";
const kDefaultSyncInterval = 120;




function nsPlacesDBFlush()
{
  
  

  this.__defineGetter__("_db", function() {
    delete this._db;
    return this._db = Cc["@mozilla.org/browser/nav-history-service;1"].
                      getService(Ci.nsPIPlacesDatabase).
                      DBConnection;
  });

  this.__defineGetter__("_bh", function() {
    delete this._bh;
    return this._bh = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
                      getService(Ci.nsINavBookmarksService);
  });


  
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

  
  this._bh.addObserver(this, false);

  this._prefs.QueryInterface(Ci.nsIPrefBranch2).addObserver("", this, false);

  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(this, kPlacesBackgroundShutdown, false);

  
  this._timer = this._newTimer();
}

nsPlacesDBFlush.prototype = {
  
  

  observe: function DBFlush_observe(aSubject, aTopic, aData)
  {
    if (aTopic == kPlacesBackgroundShutdown) {
      this._bh.removeObserver(this);
      this._timer.cancel();
      this._timer = null;
      this._syncAll();
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

    
    this._syncAll();
    this._timer = this._newTimer();
  },

  onItemAdded: function() this._syncMozPlaces(),

  onItemChanged: function DBFlush_onItemChanged(aItemId, aProperty,
                                                         aIsAnnotationProperty,
                                                         aValue)
  {
    if (aProperty == "uri")
      this._syncMozPlaces();
  },

  onItemRemoved: function() { },
  onItemVisited: function() { },
  onItemMoved: function() { },

  
  

  notify: function() this._syncAll(),

  
  

  


  _syncMozPlaces: function DBFlush_syncMozPlaces()
  {
    
    if (this._inBatchMode)
      return;

    let self = this;
    PlacesBackground.dispatch({
      run: function() self._doSyncMozX("places")
    }, Ci.nsIEventTarget.DISPATCH_NORMAL);
  },

  


  _syncAll: function DBFlush_syncAll()
  {
    let self = this;
    PlacesBackground.dispatch({
      run: function() {
        
        let ourTransaction = false;
        try {
          this._db.beginTransaction();
          ourTransaction = true;
        }
        catch (e) { }

        try {
          
          
          self._doSyncMozX("places");
          self._doSyncMozX("historyvisits");
        }
        catch (e) {
          if (ourTransaction)
            this._db.rollbackTransaction();
          throw e;
        }

        if (ourTransaction)
          this._db.commitTransaction();
      }
    }, Ci.nsIEventTarget.DISPATCH_NORMAL);
  },

  





  _doSyncMozX: function DBFlush_doSyncMozX(aName)
  {
    
    
   
   this._db.executeSimpleSQL("DELETE FROM moz_" + aName + "_temp");
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
  ])
};




let components = [nsPlacesDBFlush];
function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule(components);
}
