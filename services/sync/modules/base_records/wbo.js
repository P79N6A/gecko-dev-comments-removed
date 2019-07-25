



































const EXPORTED_SYMBOLS = ['WBORecord', 'RecordManager', 'Records'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

function WBORecord(uri) {
  this._WBORec_init(uri);
}
WBORecord.prototype = {
  _logName: "Record.WBO",

  _WBORec_init: function WBORec_init(uri) {
    this.data = {
      payload: {}
    };
    if (uri)
      this.uri = uri;
  },

  get id() { return this.data.id; },
  set id(value) { this.data.id = value; },

  
  
  get uri() {
    return Utils.makeURI(this.baseUri.resolve(encodeURI(this.id)));
  },
  set uri(value) {
    if (typeof(value) != "string")
      value = value.spec;
    let foo = value.split('/');
    this.data.id = foo.pop();
    this.baseUri = Utils.makeURI(foo.join('/') + '/');
  },

  get parentid() { return this.data.parentid; },
  set parentid(value) {
    this.data.parentid = value;
  },

  get modified() {
    if (typeof(this.data.modified) == "string")
      this.data.modified = parseInt(this.data.modified);
    return this.data.modified;
  },
  set modified(value) {
    this.data.modified = value;
  },

  get depth() {
    if (this.data.depth)
      return this.data.depth;
    return 0;
  },
  set depth(value) {
    this.data.depth = value;
  },

  get sortindex() {
    if (this.data.sortindex)
      return this.data.sortindex;
    return 0;
  },
  set sortindex(value) {
    this.data.sortindex = value;
  },

  get payload() { return this.data.payload; },
  set payload(value) {
    this.data.payload = value;
  },

  
  
  serialize: function WBORec_serialize() {
    this.payload = JSON.stringify([this.payload]);
    let ret = JSON.stringify(this.data);
    this.payload = JSON.parse(this.payload)[0];
    return ret;
  },

  deserialize: function WBORec_deserialize(json) {
    this.data = JSON.parse(json);
    this.payload = JSON.parse(this.payload)[0];
  },

  toString: function WBORec_toString() {
    return "{ id: " + this.id + "\n" +
      "  parent: " + this.parentid + "\n" +
      "  depth: " + this.depth + ", index: " + this.sortindex + "\n" +
      "  modified: " + this.modified + "\n" +
      "  payload: " + JSON.stringify(this.payload) + " }";
  }
};

Utils.lazy(this, 'Records', RecordManager);

function RecordManager() {
  this._init();
}
RecordManager.prototype = {
  _recordType: WBORecord,
  _logName: "RecordMgr",

  _init: function RegordMgr__init() {
    this._log = Log4Moz.repository.getLogger(this._logName);
    this._records = {};
    this._aliases = {};
  },

  import: function RegordMgr_import(onComplete, url) {
    let fn = function RegordMgr__import(url) {
      let self = yield;
      let record;

      this._log.trace("Importing record: " + (url.spec? url.spec : url));

      try {
        this.lastResource = new Resource(url);
        yield this.lastResource.get(self.cb);

        record = new this._recordType();
	record.deserialize(this.lastResource.data);
	record.uri = url; 

        this.set(url, record);
      } catch (e) {
        this._log.debug("Failed to import record: " + e);
        record = null;
      }
      self.done(record);
    };
    fn.async(this, onComplete, url);
  },

  get: function RegordMgr_get(onComplete, url) {
    let fn = function RegordMgr__get(url) {
      let self = yield;

      let record = null;
      let spec = url.spec? url.spec : url;
      




      if (url in this._aliases)
	url = this._aliases[url];
      if (spec in this._records)
	record = this._records[spec];

      if (!record)
	record = yield this.import(self.cb, url);

      self.done(record);
    };
    fn.async(this, onComplete, url);
  },

  set: function RegordMgr_set(url, record) {
    let spec = url.spec ? url.spec : url;
    this._records[spec] = record;
  },

  contains: function RegordMgr_contains(url) {
    let record = null;
    if (url in this._aliases)
      url = this._aliases[url];
    if (url in this._records)
      return true;
    return false;
  },

  clearCache: function recordMgr_clearCache() {
    this._records = {};
  },

  del: function RegordMgr_del(url) {
    delete this._records[url];
  },
  getAlias: function RegordMgr_getAlias(alias) {
    return this._aliases[alias];
  },
  setAlias: function RegordMgr_setAlias(url, alias) {
    this._aliases[alias] = url;
  },
  delAlias: function RegordMgr_delAlias(alias) {
    delete this._aliases[alias];
  }
};
