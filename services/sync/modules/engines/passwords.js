



































const EXPORTED_SYMBOLS = ['PasswordEngine'];

const Cu = Components.utils;

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/syncCores.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");

Function.prototype.async = Async.sugar;

function PasswordEngine() {
  this._init();
}
PasswordEngine.prototype = {
  get name() { return "passwords"; },
  get displayName() { return "Saved Passwords"; },
  get logName() { return "PasswordEngine"; },
  get serverPrefix() { return "user-data/passwords/"; },

  __store: null,
  get _store() {
    if (!this.__store)
      this.__store = new PasswordStore();
    return this.__store;
  },

  __core: null,
  get _core() {
    if (!this.__core)
      this.__core = new PasswordSyncCore(this._store);
    return this.__core;
  }
};
PasswordEngine.prototype.__proto__ = new Engine();

function PasswordSyncCore(store) {
  this._store = store;
  this._init();
}
PasswordSyncCore.prototype = {
  _logName: "PasswordSync",
  _store: null,

  _commandLike: function PSC_commandLike(a, b) {
    
    return false;
  }
};
PasswordSyncCore.prototype.__proto__ = new SyncCore();

function PasswordStore() {
  this._init();
}
PasswordStore.prototype = {
  _logName: "PasswordStore",
  _lookup: null,

  __loginManager: null,
  get _loginManager() {
    if (!this.__loginManager)
      this.__loginManager = Utils.getLoginManager();
    return this.__loginManager;
  },

  __nsLoginInfo: null,
  get _nsLoginInfo() {
    if (!this.__nsLoginInfo)
      this.__nsLoginInfo = Utils.makeNewLoginInfo();
    return this.__nsLoginInfo;
  },

  








   _hashLoginInfo: function PasswordStore__hashLoginInfo(aLogin) {
    var loginKey = aLogin.hostname      + ":" +
                   aLogin.formSubmitURL + ":" +
                   aLogin.httpRealm     + ":" +
                   aLogin.username      + ":" +
                   aLogin.password      + ":" +
                   aLogin.usernameField + ":" +
                   aLogin.passwordField;

    return Utils.sha1(loginKey);
  },

  _createCommand: function PasswordStore__createCommand(command) {
    this._log.info("PasswordStore got createCommand: " + command );

    var login = new this._nsLoginInfo(command.data.hostname,
                                      command.data.formSubmitURL,
                                      command.data.httpRealm,
                                      command.data.username,
                                      command.data.password,
                                      command.data.usernameField,
                                      command.data.passwordField);

    this._loginManager.addLogin(login);
  },

  _removeCommand: function PasswordStore__removeCommand(command) {
    this._log.info("PasswordStore got removeCommand: " + command );

    if (command.GUID in this._lookup) {
      var data  = this._lookup[command.GUID];
      var login = new this._nsLoginInfo(data.hostname,
                                        data.formSubmitURL,
                                        data.httpRealm,
                                        data.username,
                                        data.password,
                                        data.usernameField,
                                        data.passwordField);
      this._loginManager.removeLogin(login);
    } else {
      this._log.warn("Invalid GUID for remove, ignoring request!");
    }
  },

  _editCommand: function PasswordStore__editCommand(command) {
    this._log.info("PasswordStore got editCommand: " + command );
    throw "Password syncs are expected to only be create/remove!";
  },

  wrap: function PasswordStore_wrap() {
    
    var items = {};
    var logins = this._loginManager.getAllLogins({});

    for (var i = 0; i < logins.length; i++) {
      var login = logins[i];

      var key = this._hashLoginInfo(login);

      items[key] = { hostname      : login.hostname,
                     formSubmitURL : login.formSubmitURL,
                     httpRealm     : login.httpRealm,
                     username      : login.username,
                     password      : login.password,
                     usernameField : login.usernameField,
                     passwordField : login.passwordField };
    }

    this._lookup = items;
    return items;
  },

  wipe: function PasswordStore_wipe() {
    this._loginManager.removeAllLogins();
  },

  _resetGUIDs: function PasswordStore__resetGUIDs() {
    let self = yield;
    
  }
};
PasswordStore.prototype.__proto__ = new Store();

function PasswordTracker() {
  this._init();
}
PasswordTracker.prototype = {
  _logName: "PasswordTracker",

  __loginManager : null,
  get _loginManager() {
    if (!this.__loginManager)
      this.__loginManager = Cc["@mozilla.org/login-manager;1"].
                            getService(Ci.nsILoginManager);
    return this.__loginManager;
  },

  










  _loginCount: 0,
  get score() {
    var count = this._loginManager.countLogins("", "", "");
    var score = (Math.abs(this._loginCount - count) * 15) + 30;

    if (score >= 100)
      return 100;
    else
      return score;
  }, 

  resetScore: function PasswordTracker_resetScore() {
    this._loginCount = this._loginManager.countLogins("", "", "");
  },

  _init: function PasswordTracker__init() {
    this._log = Log4Moz.Service.getLogger("Service."  + this._logName);
    this._loginCount = this._loginManager.countLogins("", "", "");
  }
};
PasswordTracker.prototype.__proto__ = new Tracker();
