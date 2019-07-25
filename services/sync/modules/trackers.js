



































const EXPORTED_SYMBOLS = ['Tracker', 'BookmarksTracker', 'HistoryTracker',
                          'FormsTracker', 'CookieTracker', 'TabTracker'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;















function Tracker() {
  this._init();
}
Tracker.prototype = {
  _logName: "Tracker",
  _score: 0,

  _init: function T__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;
  },

  


  get score() {
    if (this._score >= 100)
      return 100;
    else
      return this._score;
  },

  


  resetScore: function T_resetScore() {
    this._score = 0;
  }
};












function BookmarksTracker() {
  this._init();
}
BookmarksTracker.prototype = {
  _logName: "BMTracker",

  
  onBeginUpdateBatch: function BMT_onBeginUpdateBatch() {

  },
  onEndUpdateBatch: function BMT_onEndUpdateBatch() {

  },
  onItemVisited: function BMT_onItemVisited() {

  },

  



  onItemAdded: function BMT_onEndUpdateBatch() {
    this._score += 4;
  },
  onItemRemoved: function BMT_onItemRemoved() {
    this._score += 4;
  },
  
  onItemChanged: function BMT_onItemChanged() {
    this._score += 2;
  },

  _init: function BMT__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;

    Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
    getService(Ci.nsINavBookmarksService).
    addObserver(this, false);
  }
}
BookmarksTracker.prototype.__proto__ = new Tracker();

function HistoryTracker() {
  this._init();
}
HistoryTracker.prototype = {
  _logName: "HistoryTracker",

  
  onBeginUpdateBatch: function HT_onBeginUpdateBatch() {

  },
  onEndUpdateBatch: function HT_onEndUpdateBatch() {

  },
  onPageChanged: function HT_onPageChanged() {

  },
  onTitleChanged: function HT_onTitleChanged() {

  },

  




  onVisit: function HT_onVisit(uri, vid, time, session, referrer, trans) {
    this._score += 1;
  },
  onPageExpired: function HT_onPageExpired(uri, time, entry) {
    this._score += 1;
  },
  onDeleteURI: function HT_onDeleteURI(uri) {
    this._score += 1;
  },
  onClearHistory: function HT_onClearHistory() {
    this._score += 50;
  },

  _init: function HT__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;

    Cc["@mozilla.org/browser/nav-history-service;1"].
    getService(Ci.nsINavHistoryService).
    addObserver(this, false);
  }
}
HistoryTracker.prototype.__proto__ = new Tracker();

function CookieTracker() {
  this._init();
}
CookieTracker.prototype = {
  _logName: "CookieTracker",

  _init: function CT__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;
    


    let observerService = Cc["@mozilla.org/observer-service;1"].
            getService(Ci.nsIObserverService);
    observerService.addObserver( this, 'cookie-changed', false );
  },

  
  observe: function ( aSubject, aTopic, aData ) {
    



    var newCookie = aSubject.QueryInterface( Ci.nsICookie2 );
    if ( newCookie ) {
      if ( !newCookie.isSession ) {
	

	this._score += 10;
      }
    }
  }
}
CookieTracker.prototype.__proto__ = new Tracker();

function FormsTracker() {
  this._init();
}
FormsTracker.prototype = {
  _logName: "FormsTracker",

  __formDB: null,
  get _formDB() {
    if (!this.__formDB) {
      var file = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties).
      get("ProfD", Ci.nsIFile);
      file.append("formhistory.sqlite");
      var stor = Cc["@mozilla.org/storage/service;1"].
      getService(Ci.mozIStorageService);
      this.__formDB = stor.openDatabase(file);
    }

    return this.__formDB;
  },

  












  _rowCount: 0,
  get score() {
    var stmnt = this._formDB.createStatement("SELECT COUNT(fieldname) FROM moz_formhistory");
    stmnt.executeStep();
    var count = stmnt.getInt32(0);
    stmnt.reset();

    this._score = Math.abs(this._rowCount - count) * 2;

    if (this._score >= 100)
      return 100;
    else
      return this._score;
  },

  resetScore: function FormsTracker_resetScore() {
    var stmnt = this._formDB.createStatement("SELECT COUNT(fieldname) FROM moz_formhistory");
    stmnt.executeStep();
    this._rowCount = stmnt.getInt32(0);
    stmnt.reset();
    this._score = 0;
  },

  _init: function FormsTracker__init() {
    this._log = Log4Moz.Service.getLogger("Service." + this._logName);
    this._score = 0;

    var stmnt = this._formDB.createStatement("SELECT COUNT(fieldname) FROM moz_formhistory");
    stmnt.executeStep();
    this._rowCount = stmnt.getInt32(0);
    stmnt.reset();
  }
}
FormsTracker.prototype.__proto__ = new Tracker();

function TabTracker(engine) {
  this._engine = engine;
  this._init();
}
TabTracker.prototype = {
  __proto__: new Tracker(),

  _logName: "TabTracker",

  _engine: null,

  get _json() {
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    this.__defineGetter__("_json", function() json);
    return this._json;
  },

  

























  get score() {
    
    
    let snapshotData = this._engine.snapshot.data;
    let a = {};

    
    let b = this._engine.store.wrap();

    
    
    
    let c = [];

    
    for (id in snapshotData) {
      if (id in b) {
        c.push(this._json.encode(snapshotData[id]) == this._json.encode(b[id]));
        delete b[id];
      }
      else {
        a[id] = snapshotData[id];
      }
    }

    let numShared = c.length;
    let numUnique = [true for (id in a)].length + [true for (id in b)].length;
    let numTotal = numShared + numUnique;

    
    
    
    if (numTotal == 0)
      return 0;

    
    let numChanged = c.filter(function(v) v).length;

    let fractionSimilar = (numShared - (numChanged / 2)) / numTotal;
    let fractionDissimilar = 1 - fractionSimilar;
    let percentDissimilar = Math.round(fractionDissimilar * 100);

    return percentDissimilar;
  },

  resetScore: function FormsTracker_resetScore() {
    
  }
}
