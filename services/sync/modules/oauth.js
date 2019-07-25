



































const EXPORTED_SYMBOLS = ['OAuth'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/engines.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'OAuth', OAuthSvc);

function OAuthSvc() {
  this._init();
}
OAuthSvc.prototype = {
  _logName: "OAuth",
  _keyring: null,
  _consKey: null,
  _bulkID: null,
  _rsaKey: null,
  _token: null,
  _cback: null,
  _uid: null,
  _pwd: null,
  _pas: null,
  _cb1: null,
  _cb2: null,
  
  _init: function OAuth__init() {
    this._log = Log4Moz.repository.getLogger("Service." + this._logName);
    this._log.level = "Debug";
    this._log.info("OAuth Module Initialized");
  },

  setToken: function OAuth_setToken(token, cback) {
    this._token = token;
    this._cback = cback;
  },
  
  setUser: function OAuth_setUser(username, password, passphrase) {
    this._uid = username;
    this._pwd = password;
    this._pas = passphrase;
  },
  
  validate: function OAuth_getName(obj, cb) {
    if (!this._token || !this._uid || !this._pwd)
      cb(obj, false);

    var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
              createInstance(Ci.nsIXMLHttpRequest);
    var key = btoa(this._uid + ":" + this._pwd);
    
    req.onreadystatechange = function(e) {
      if (req.readyState == 4) {
        if (req.status == 200) {
          var fields = req.responseText.split(',');
          cb(obj, fields[0], fields[1], fields[2]);
        } else {
          cb(obj, req.responseText);
        }
      }
    };
    req.open('GET', 'https://services.mozilla.com/api/oauth/info.php?token=' + this._token + '&key=' + key);
    req.send(null);
  },
  
  finalize: function OAuth_finalize(cb1, cb2, bundle) {
    this._cb1 = cb1;
    this._cb2 = cb2;
    this._bundle = bundle;

    var bmkEngine = Engines.get('bookmarks');
    var bmkRstore = bmkEngine._remote;

    this._keyring = bmkRstore.keys;
    this._keyring.getKeyAndIV(Utils.bind2(this, this._gotBulkKey), ID.get('WeaveID'));
  },
  
  _gotBulkKey: function OAuth_gotBulkKey() {
    let consID = new Identity();
    consID.pubkey = this._rsaKey;
    consID.username = this._consKey;
    this._cb1(this._bundle);
    this._log.info("Updating keyring for 3rd party access");
    this._keyring.setKey(Utils.bind2(this, this._done), ID.get('WeaveID'), consID);
  },
  
  _done: function OAuth__done() {
    var cb = this._cb2;
    var bu = this._bundle;

    if (!this._token || !this._uid || !this._pwd)
      cb(this._bundle, false);
    
    var req = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
              createInstance(Ci.nsIXMLHttpRequest);
    var key = btoa(this._uid + ":" + this._pwd);
    
    req.onreadystatechange = function(e) {
      if (req.readyState == 4) {
        if (req.status == 200 && req.responseText == "1") {
          cb(bu, true);
        } else {
          cb(bu, false);
        }
      }
    };
    req.open('GET', 'https://services.mozilla.com/api/oauth/update.php?token=' + this._token + '&key=' + key);
    req.send(null);
  }
};
