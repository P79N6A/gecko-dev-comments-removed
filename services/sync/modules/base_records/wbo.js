



































const EXPORTED_SYMBOLS = ['WBORecord'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

function WBORecord(uri, authenticator) {
  this._WBORec_init(uri, authenticator);
}
WBORecord.prototype = {
  __proto__: Resource.prototype,
  _logName: "Record.WBO",

  _WBORec_init: function WBORec_init(uri, authenticator) {
    this._init(uri, authenticator);
    this.pushFilter(new WBOFilter());
    this.pushFilter(new JsonFilter());
    this.data = {
      
      payload: {}
    };
  },

  
  get id() {
    if (this.data.id)
      return decodeURI(this.data.id);
    let foo = this.uri.spec.split('/');
    return decodeURI(foo[foo.length-1]);
  },

  get parentid() this.data.parentid,
  set parentid(value) {
    this.data.parentid = value;
  },

  get modified() this.data.modified,
  set modified(value) {
    this.data.modified = value;
  },

  get encoding() this.data.encoding,
  set encoding(value) {
    this.data.encoding = value;
  },

  get payload() this.data.payload,
  set payload(value) {
    this.data.payload = value;
  }

  
  
};


let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);

function WBOFilter() {}
WBOFilter.prototype = {
  beforePUT: function(data, wbo) {
    let self = yield;
    let foo = wbo.uri.spec.split('/');
    data.id = decodeURI(foo[foo.length-1]);
    data.payload = json.encode([data.payload]);
    self.done(data);
  },
  afterGET: function(data, wbo) {
    let self = yield;
    let foo = wbo.uri.spec.split('/');
    data.id = decodeURI(foo[foo.length-1]);
    data.payload = json.decode(data.payload)[0];
    self.done(data);
  }
};
