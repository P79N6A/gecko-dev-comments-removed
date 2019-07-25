



































const EXPORTED_SYMBOLS = ['Collection'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");

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

  pushData: function Coll_pushData(data) {
    this._data.push(data);
  },

  clearRecords: function Coll_clearRecords() {
    this._data = [];
  },

  set recordHandler(onRecord) {
    
    let coll = this;

    this._onProgress = function() {
      
      if (this._data == "[]")
        return;

      do {
        
        
        let start = this._data[0];
        if (start == "[" || start == "," || start == "]")
          this._data = this._data.slice(1);

        
        let json = "";
        let braces = 1;
        let ignore = false;
        let escaped = false;
        let length = this._data.length;

        
        for (let i = 1; i < length; i++) {
          let char = this._data[i];

          
          if (char == '"') {
            if (!ignore)
              ignore = true;
            
            else if (!escaped)
              ignore = false;
          }

          
          if (ignore) {
            escaped = char == "\\" ? !escaped : false;

            
            continue;
          }

          
          if (char == "{")
            braces++;
          
          else if (char == "}" && --braces == 0) {
            
            json = this._data.slice(0, i + 1);
            this._data = this._data.slice(i + 1);

            
            break;
          }
        }

        
        if (json.length == 0)
          break;

        
        let record = new coll._recordObj();
        record.deserialize(json);
        record.baseURI = coll.uri;
        record.id = record.data.id;
        onRecord(record);

      
      } while (true);

      
      
      Cu.forceGC();
    };
  }
};
