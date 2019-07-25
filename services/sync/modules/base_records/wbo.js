



































const EXPORTED_SYMBOLS = ['WBORecord', 'RecordManager', 'Records'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/util.js");

function WBORecord(uri) {
  if (uri == null)
    throw "WBOs must have a URI!";

  this.data = {};
  this.payload = {};
  this.uri = uri;
}
WBORecord.prototype = {
  _logName: "Record.WBO",

  
  
  get uri() {
    return Utils.makeURL(this.baseUri.resolve(encodeURI(this.id)));
  },
  set uri(value) {
    if (typeof(value) != "string")
      value = value.spec;
    let parts = value.split('/');
    this.id = parts.pop();
    this.baseUri = Utils.makeURI(parts.join('/') + '/');
  },

  get sortindex() {
    if (this.data.sortindex)
      return this.data.sortindex;
    return 0;
  },

  deserialize: function deserialize(json) {
    this.data = json.constructor.toString() == String ? JSON.parse(json) : json;

    try {
      
      this.payload = JSON.parse(this.payload);
    }
    catch(ex) {}
  },

  toJSON: function toJSON() {
    
    let obj = {};
    for (let [key, val] in Iterator(this.data))
      obj[key] = key == "payload" ? JSON.stringify(val) : val;
    return obj;
  },

  toString: function WBORec_toString() "{ " + [
      "id: " + this.id,
      "index: " + this.sortindex,
      "modified: " + this.modified,
      "payload: " + JSON.stringify(this.payload)
    ].join("\n  ") + " }",
};

Utils.deferGetSet(WBORecord, "data", ["id", "modified", "sortindex", "payload"]);

Utils.lazy(this, 'Records', RecordManager);

function RecordManager() {
  this._log = Log4Moz.repository.getLogger(this._logName);
  this._records = {};
}
RecordManager.prototype = {
  _recordType: WBORecord,
  _logName: "RecordMgr",

  import: function RecordMgr_import(url) {
    this._log.trace("Importing record: " + (url.spec ? url.spec : url));
    try {
      
      this.response = {};
      this.response = new Resource(url).get();

      
      if (!this.response.success)
        return null;

      let record = new this._recordType(url);
      record.deserialize(this.response);

      return this.set(url, record);
    }
    catch(ex) {
      this._log.debug("Failed to import record: " + Utils.exceptionStr(ex));
      return null;
    }
  },

  get: function RecordMgr_get(url) {
    
    let spec = url.spec ? url.spec : url;
    if (spec in this._records)
      return this._records[spec];
    return this.import(url);
  },

  set: function RegordMgr_set(url, record) {
    let spec = url.spec ? url.spec : url;
    return this._records[spec] = record;
  },

  contains: function RegordMgr_contains(url) {
    if ((url.spec || url) in this._records)
      return true;
    return false;
  },

  clearCache: function recordMgr_clearCache() {
    this._records = {};
  },

  del: function RegordMgr_del(url) {
    delete this._records[url];
  }
};
