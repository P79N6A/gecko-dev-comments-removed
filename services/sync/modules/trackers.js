



































const EXPORTED_SYMBOLS = ['Tracker', 'BookmarksTracker'];

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
    onItemVisited: function BMT_onItemChanged() {

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
      super._init();
      Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
      getService(Ci.nsINavBookmarksService).
      addObserver(this, false);
    }
  }
  BookmarksTracker.prototype.__proto__ = new Tracker();
  
