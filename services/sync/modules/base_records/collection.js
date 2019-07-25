



































const EXPORTED_SYMBOLS = ['Collection'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/resource.js");

function Collection(uri, recordObj) {
  this._Coll_init(uri);
  this._recordObj = recordObj;
}
Collection.prototype = {
  __proto__: Resource.prototype,
  _logName: "Collection",

  _Coll_init: function Coll_init(uri) {
    this._init(uri);
    this._full = false;
    this._ids = null;
    this._limit = 0;
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
    }
    if (this.full)
      args.push('full=1');
    if (this.sort)
      args.push('sort=' + this.sort);
    if (this.ids != null)
      args.push("ids=" + this.ids);
    if (this.limit > 0)
      args.push("limit=" + this.limit);

    this.uri.query = (args.length > 0)? '?' + args.join('&') : '';
  },

  
  get full() { return this._full; },
  set full(value) {
    this._full = value;
    this._rebuildURL();
  },

  
  get ids() this._ids,
  set ids(value) {
    this._ids = value;
    this._rebuildURL();
  },

  
  get limit() this._limit,
  set limit(value) {
    this._limit = value;
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

  pushData: function Coll_pushData(data) {
    this._data.push(data);
  },

  clearRecords: function Coll_clearRecords() {
    this._data = [];
  },

  set recordHandler(onRecord) {
    
    let coll = this;

    
    coll.setHeader("Accept", "application/newlines");

    this._onProgress = function() {
      let newline;
      while ((newline = this._data.indexOf("\n")) > 0) {
        
        let json = this._data.slice(0, newline);
        this._data = this._data.slice(newline + 1);

        
        let record = new coll._recordObj();
        record.deserialize(json);
        record.baseURI = coll.uri;
        record.id = record.data.id;
        onRecord(record);
      }
    };
  }
};
