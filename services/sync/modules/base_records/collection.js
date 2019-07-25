



































const EXPORTED_SYMBOLS = ['Collection'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

Function.prototype.async = Async.sugar;

function Collection(uri, recordObj) {
  this._Coll_init(uri);
  this._recordObj = recordObj;
}
Collection.prototype = {
  __proto__: Resource.prototype,
  _logName: "Collection",

  _Coll_init: function Coll_init(uri) {
    this._init(uri);
    this.pushFilter(new JsonFilter());
    this._full = true;
    this._older = 0;
    this._newer = 0;
    this._data = [];
  },

  _rebuildURL: function Coll__rebuildURL() {
    
    this.uri.QueryInterface(Ci.nsIURL);

    let args = [];
    if (this.older)
      args.push('older=' + this.older);
    else if (this.newer) {
      args.push('newer=' + this.newer);
      args.push('modified=' + this.newer); 
    }
    if (this.full)
      args.push('full=1');
    if (this.sort)
      args.push('sort=' + this.sort);

    this.uri.query = (args.length > 0)? '?' + args.join('&') : '';
  },

  
  get full() { return this._full; },
  set full(value) {
    this._full = value;
    this._rebuildURL();
  },

  
  get older() { return this._older; },
  set older(value) {
    this._older = value;
    this._rebuildURL();
  },

  
  get newer() { return this._newer; },
  set newer(value) {
    this._newer = value;
    this._rebuildURL();
  },

  
  
  
  
  
  get sort() { return this._sort; },
  set sort(value) {
    this._sort = value;
    this._rebuildURL();
  },

  get iter() {
    if (!this._iter)
      this._iter = new CollectionIterator(this);
    return this._iter;
  },

  pushData: function Coll_pushData(data) {
    this._data.push(data);
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
      let record = new this._coll._recordObj();
      record.deserialize(Svc.Json.encode(item)); 
      record.baseUri = this._coll.uri;

      self.done(record);
    };
    fn.async(this, onComplete);
  },

  reset: function CollIter_reset() {
    this._idx = 0;
  }
};