



































const EXPORTED_SYMBOLS = ['Weave'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/wrap.js");
Cu.import("resource://weave/crypto.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/dav.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;


let Weave = {};
Components.utils.import("resource://weave/constants.js", Weave);
Components.utils.import("resource://weave/util.js", Weave);
Components.utils.import("resource://weave/async.js", Weave);
Components.utils.import("resource://weave/crypto.js", Weave);
Components.utils.import("resource://weave/identity.js", Weave);
Components.utils.import("resource://weave/dav.js", Weave);
Components.utils.import("resource://weave/stores.js", Weave);
Components.utils.import("resource://weave/syncCores.js", Weave);
Components.utils.import("resource://weave/engines.js", Weave);
Components.utils.import("resource://weave/service.js", Weave);
Utils.lazy(Weave, 'Service', WeaveSvc);






function WeaveSvc() {
  this._initLogs();
  this._log.info("Weave Sync Service Initializing");

  Utils.prefs.addObserver("", this, false);

  if (!this.enabled) {
    this._log.info("Weave Sync disabled");
    return;
  }

  this._setSchedule(this.schedule);
}
WeaveSvc.prototype = {

  _notify: Wrap.notify,
  _lock: Wrap.lock,
  _localLock: Wrap.localLock,
  _osPrefix: "weave:service:",

  __os: null,
  get _os() {
    if (!this.__os)
      this.__os = Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService);
    return this.__os;
  },

  __dirSvc: null,
  get _dirSvc() {
    if (!this.__dirSvc)
      this.__dirSvc = Cc["@mozilla.org/file/directory_service;1"].
        getService(Ci.nsIProperties);
    return this.__dirSvc;
  },

  __dav: null,
  get _dav() {
    if (!this.__dav)
      this.__dav = new DAVCollection();
    return this.__dav;
  },

  

  __bmkEngine: null,
  get _bmkEngine() {
    if (!this.__bmkEngine)
      this.__bmkEngine = new BookmarksEngine(this._dav, this._cryptoId);
    return this.__bmkEngine;
  },

  __histEngine: null,
  get _histEngine() {
    if (!this.__histEngine)
      this.__histEngine = new HistoryEngine(this._dav, this._cryptoId);
    return this.__histEngine;
  },

  
  _log: null,

  
  _scheduleTimer: null,

  __mozId: null,
  get _mozId() {
    if (this.__mozId === null)
      this.__mozId = new Identity('Mozilla Services Password', this.username);
    return this.__mozId;
  },

  __cryptoId: null,
  get _cryptoId() {
    if (this.__cryptoId === null)
      this.__cryptoId = new Identity('Mozilla Services Encryption Passphrase',
				     this.username);
    return this.__cryptoId;
  },

  get username() {
    return Utils.prefs.getCharPref("username");
  },
  set username(value) {
    if (value)
      Utils.prefs.setCharPref("username", value);
    else
      Utils.prefs.clearUserPref("username");

    
    this._mozId.username = value;
    this._cryptoId.username = value;
  },

  get password() { return this._mozId.password; },
  set password(value) { this._mozId.password = value; },

  get passphrase() { return this._cryptoId.password; },
  set passphrase(value) { this._cryptoId.password = value; },

  get userPath() { return this._mozId.userHash; },

  get currentUser() {
    if (this._dav.loggedIn)
      return this.username;
    return null;
  },

  get enabled() {
    return Utils.prefs.getBoolPref("enabled");
  },

  get schedule() {
    if (!this.enabled)
      return 0; 
    return Utils.prefs.getIntPref("schedule");
  },

  _setSchedule: function Weave__setSchedule(schedule) {
    switch (this.schedule) {
    case 0:
      this._disableSchedule();
      break;
    case 1:
      this._enableSchedule();
      break;
    default:
      this._log.info("Invalid Weave scheduler setting: " + schedule);
      break;
    }
  },

  _enableSchedule: function WeaveSync__enableSchedule() {
    if (this._scheduleTimer) {
      this._scheduleTimer.cancel();
      this._scheduleTimer = null;
    }
    this._scheduleTimer = Cc["@mozilla.org/timer;1"].
      createInstance(Ci.nsITimer);
    let listener = new Utils.EventListener(Utils.bind2(this, this._onSchedule));
    this._scheduleTimer.initWithCallback(listener, 1800000, 
                                         this._scheduleTimer.TYPE_REPEATING_SLACK);
    this._log.info("Weave scheduler enabled");
  },

  _disableSchedule: function WeaveSync__disableSchedule() {
    if (this._scheduleTimer) {
      this._scheduleTimer.cancel();
      this._scheduleTimer = null;
    }
    this._log.info("Weave scheduler disabled");
  },

  _onSchedule: function WeaveSync__onSchedule() {
    if (this.enabled) {
      this._log.info("Running scheduled sync");
      this.sync();
    }
  },

  _initLogs: function WeaveSync__initLogs() {
    this._log = Log4Moz.Service.getLogger("Service.Main");
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.service.main")];

    let formatter = Log4Moz.Service.newFormatter("basic");
    let root = Log4Moz.Service.rootLogger;
    root.level = Log4Moz.Level[Utils.prefs.getCharPref("log.rootLogger")];

    let capp = Log4Moz.Service.newAppender("console", formatter);
    capp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.console")];
    root.addAppender(capp);

    let dapp = Log4Moz.Service.newAppender("dump", formatter);
    dapp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.dump")];
    root.addAppender(dapp);

    let brief = this._dirSvc.get("ProfD", Ci.nsIFile);
    brief.QueryInterface(Ci.nsILocalFile);
    brief.append("weave");
    brief.append("logs");
    brief.append("brief-log.txt");
    if (!brief.exists())
      brief.create(brief.NORMAL_FILE_TYPE, PERMS_FILE);

    let verbose = brief.parent.clone();
    verbose.append("verbose-log.txt");
    if (!verbose.exists())
      verbose.create(verbose.NORMAL_FILE_TYPE, PERMS_FILE);

    let fapp = Log4Moz.Service.newFileAppender("rotating", brief, formatter);
    fapp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.briefLog")];
    root.addAppender(fapp);
    let vapp = Log4Moz.Service.newFileAppender("rotating", verbose, formatter);
    vapp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.debugLog")];
    root.addAppender(vapp);
  },

  _createUserDir: function WeaveSync__createUserDir(serverURL) {
    let self = yield;

    this._log.debug("Attempting to create user directory");

    this._dav.baseURL = serverURL;
    this._dav.MKCOL("user/" + this.userPath, self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create user directory";

    this._log.debug("Successfully created user directory.  Re-attempting login.");
    this._dav.baseURL = serverURL + "user/" + this.userPath + "/";
    this._dav.login.async(this._dav, self.cb, this.username, this.password);
    let success = yield;
    if (!success)
      throw "Created user directory, but login still failed.  Aborting.";
  },

  _uploadVersion: function WeaveSync__uploadVersion() {
    let self = yield;

    this._dav.MKCOL("meta", self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create meta information directory";

    this._dav.PUT("meta/version", STORAGE_FORMAT_VERSION, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload server version file");
  },

  
  _versionCheck: function WeaveSync__versionCheck() {
    let self = yield;

    this._dav.GET("meta/version", self.cb);
    let ret = yield;

    if (!Utils.checkStatus(ret.status)) {
      this._log.info("Server has no version file.  Wiping server data.");
      this._serverWipe.async(this, self.cb);
      yield;
      this._uploadVersion.async(this, self.cb);
      yield;

    } else if (ret.responseText < STORAGE_FORMAT_VERSION) {
      this._log.info("Server version too low.  Wiping server data.");
      this._serverWipe.async(this, self.cb);
      yield;
      this._uploadVersion.async(this, self.cb);
      yield;

    } else if (ret.responseText > STORAGE_FORMAT_VERSION) {
      
    }
  },

  _generateKeys: function WeaveSync__generateKeys() {
    let self = yield;

    this._log.debug("Generating new RSA key");
    Crypto.RSAkeygen.async(Crypto, self.cb, this._cryptoId);
    let [privkey, pubkey] = yield;

    this._cryptoId.privkey = privkey;
    this._cryptoId.pubkey = pubkey;

    this._dav.MKCOL("private/", self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create private key directory";

    this._dav.MKCOL("public/", self.cb);
    ret = yield;
    if (!ret)
      throw "Could not create public key directory";

    this._dav.PUT("private/privkey", privkey, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload private key");

    this._dav.PUT("public/pubkey", pubkey, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload public key");
  },

  _login: function WeaveSync__login(password, passphrase) {
    let self = yield;

    try {
      if (this._dav.locked)
        throw "Login failed: could not acquire lock";
      this._dav.allowLock = false;
  
      
      
      this._mozId.setTempPassword(password);
      this._cryptoId.setTempPassword(passphrase);

      this._log.debug("Logging in");
      this._os.notifyObservers(null, "weave:service-login:start", "");

      if (!this.username)
        throw "No username set, login failed";
      if (!this.password)
        throw "No password given or found in password manager";

      let serverURL = Utils.prefs.getCharPref("serverURL");
      this._dav.baseURL = serverURL + "user/" + this.userPath + "/";
      this._log.info("Using server URL: " + this._dav.baseURL);

      this._dav.login.async(this._dav, self.cb, this.username, this.password);
      let success = yield;

      if (!success) {
        
        this._createUserDir.async(this, self.cb, serverURL);
        yield;
      }

      this._versionCheck.async(this, self.cb);
      yield;

      this._dav.GET("private/privkey", self.cb);
      let keyResp = yield;
      Utils.ensureStatus(keyResp.status,
                         "Could not get private key from server", [[200,300],404]);

      if (keyResp.status != 404) {
        this._cryptoId.privkey = keyResp.responseText;
        Crypto.RSAkeydecrypt.async(Crypto, self.cb, this._cryptoId);
        this._cryptoId.pubkey = yield;

      } else {
        this._generateKeys.async(this, self.cb);
        yield;
      }

      this._passphrase = null;
      this._dav.allowLock = true;
      this._os.notifyObservers(null, "weave:service-login:success", "");
      self.done(true);

    } catch (e) {
      this._dav.allowLock = true;
      this._os.notifyObservers(null, "weave:service-login:error", "");
      throw e;
    }
  },

  _serverWipe: function WeaveSync__serverWipe() {
    let self = yield;

    let cb = function Weave_serverWipe() {
      let innerSelf = yield;

      this._dav.listFiles.async(this._dav, self.cb);
      let names = yield;

      for (let i = 0; i < names.length; i++) {
        this._dav.DELETE(names[i], self.cb);
        let resp = yield;
        this._log.debug(resp.status);
      }
    };

    
    
    this._notify("server-wipe", cb).async(this, self.cb);
    yield;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupports]),

  

  observe: function WeaveSync__observe(subject, topic, data) {
    if (topic != "nsPref:changed")
      return;

    switch (data) {
    case "enabled": 
    case "schedule":
      this._setSchedule(this.schedule);
      break;
    }
  },

  

  login: function WeaveSync_login(onComplete, password, passphrase) {
    this._login.async(this, onComplete, password, passphrase);
  },

  logout: function WeaveSync_logout() {
    this._log.info("Logging out");
    this._dav.logout();
    this._mozId.setTempPassword(null); 
    this._cryptoId.setTempPassword(null); 
    this._os.notifyObservers(null, "weave:service-logout:success", "");
  },

  resetLock: function WeaveSync_resetLock(onComplete) {
    this._resetLock.async(this, onComplete);
  },
  _resetLock: function WeaveSync__resetLock() {
    let self = yield;

    let cb = function Weave_resetLock() {
      let innerSelf = yield;
      this._dav.forceUnlock.async(this._dav, innerSelf.cb);
      yield;
    };

    this._notify("reset-server-lock", cb).async(this, self.cb);
    yield;
  },


  

  sync: function WeaveSync_sync(onComplete) {
    this._sync.async(this, onComplete);
  },
  _sync: function WeaveSync__sync() {
    let self = yield;

    let cb = function Weave_sync() {
      let innerSelf = yield;

      let engineCb = function WeaveSvc_syncEngine(engine) {
        let innerInnerSelf = yield;
        engine.sync(innerInnerSelf.cb);
        yield;
      };

      if (Utils.prefs.getBoolPref("bookmarks")) {
        this._notify(this._bmkEngine.name + ":sync",
                     engineCb, this._bmkEngine).async(this, innerSelf.cb);
        yield;
        this._bmkEngine.syncMounts(innerSelf.cb);
        yield;
      }
      if (Utils.prefs.getBoolPref("history")) {
        this._notify(this._histEngine.name + ":sync",
                     engineCb, this._histEngine).async(this, innerSelf.cb);
        yield;
      }
    };

    this._lock(this._notify("sync", cb)).async(this, self.cb);
    yield;
  },

  resetServer: function WeaveSync_resetServer(onComplete) {
    this._resetServer.async(this, onComplete);
  },
  _resetServer: function WeaveSync__resetServer() {
    let self = yield;

    let cb = function Weave_resetServer() {
      let innerSelf = yield;
      this._bmkEngine.resetServer(innerSelf.cb);
      yield;
      this._histEngine.resetServer(innerSelf.cb);
      yield;
    };

    this._lock(this._notify("reset-server",cb)).async(this, self.cb);
    yield;
  },

  resetClient: function WeaveSync_resetClient(onComplete) {
    this._resetClient.async(this, onComplete);
  },
  _resetClient: function WeaveSync__resetClient() {
    let self = yield;

    let cb = function Weave_resetClient() {
      let innerSelf = yield;
      this._bmkEngine.resetClient(innerSelf.cb);
      yield;
      this._histEngine.resetClient(innerSelf.cb);
      yield;
    };

    this._localLock(this._notify("reset-client", cb)).async(this, self.cb);
    yield;
  },

  shareBookmarks: function WeaveSync_shareBookmarks(onComplete, username) {
    this._shareBookmarks.async(this, onComplete, username);
  },
  _shareBookmarks: function WeaveSync__shareBookmarks(username) {
    let self = yield;

    let cb = function Weave_shareBookmarks() {
      let innerSelf = yield;
      this._bmkEngine.share(innerSelf.cb, username);
      let ret = yield;
      innerSelf.done(ret);
    };

    this._lock(this._notify("share-bookmarks", cb)).async(this, self.cb);
    let ret = yield;
    self.done(ret);
  }

};
