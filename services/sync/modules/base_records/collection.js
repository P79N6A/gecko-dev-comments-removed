



































const EXPORTED_SYMBOLS = ['Collection'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/Observers.js");
Cu.import("resource://weave/Preferences.js");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

function Collection(uri, authenticator) {
  this._Coll_init(uri, authenticator);
}
Collection.prototype = {
  __proto__: Resource.prototype,
  _logName: "Collection",

  _Coll_init: function Coll_init(uri, authenticator) {
    this._init(uri, authenticator);
    this.pushFilter(new JsonFilter());
    this._full = true;
    this._modified = 0;
    this._data = [];
  },

  _rebuildURL: function Coll__rebuildURL() {
    
    this.uri.QueryInterface(Ci.nsIURL);

    let args = [];
    if (this.modified)
      args.push('modified=' + this.modified);
    if (this.full)
      args.push('full=1');

    this.uri.query = (args.length > 0)? '?' + args.join('&') : '';
  },

  
  get full() { return this._full; },
  set full(value) {
    this._full = value;
    this._rebuildURL();
  },

  
  get modified() { return this._modified; },
  set modified(value) {
    this._modified = value;
    this._rebuildURL();
  },

  get iter() {
    if (!this._iter)
      this._iter = new CollectionIterator(this);
    return this._iter;
  },

  get _json() {
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    this.__defineGetter__("_json", function() json);
    return this._json;
  },

  pushRecord: function Coll_pushRecord(onComplete, record) {
    let fn = function(record) {
      let self = yield;
      yield record.filterUpload(self.cb); 
      this._data.push(this._json.decode(record.data)); 
      self.done();
    };
    fn.async(this, onComplete, record);
  },

  clearRecords: function Coll_clearRecords() {
    this._data = [];
  }
};



function CollectionIterator(coll) {
  this._init(coll);
}
CollectionIterator.prototype = {
  _init: function CollIter__init(coll) {
    this._coll = coll;
    this._idx = 0;
  },

  get count() { return this._coll.data.length; },

  next: function CollIter_next(onComplete) {
    let fn = function CollIter__next() {
      let self = yield;

      if (this._idx >= this.count)
        return;
      let item = this._coll.data[this._idx++];
      let wrap = new CryptoWrapper(this._coll.uri.resolve(item.id));
      wrap.data = this._coll._json.encode(item); 
      yield wrap.filterDownload(self.cb); 

      self.done(wrap);
    };
    fn.async(this, onComplete);
  },

  reset: function CollIter_reset() {
    this._idx = 0;
  }
};