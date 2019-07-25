



































const EXPORTED_SYMBOLS = ['Tracker'];

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
