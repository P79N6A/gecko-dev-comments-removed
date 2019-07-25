



































const EXPORTED_SYMBOLS = ['Tracker',
                          'TabTracker'];

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
