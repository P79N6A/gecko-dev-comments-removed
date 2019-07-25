



































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
Cu.import("resource://weave/constants.js", Weave);
Cu.import("resource://weave/util.js", Weave);
Cu.import("resource://weave/async.js", Weave);
Cu.import("resource://weave/crypto.js", Weave);
Cu.import("resource://weave/identity.js", Weave);
Cu.import("resource://weave/dav.js", Weave);
Cu.import("resource://weave/stores.js", Weave);
Cu.import("resource://weave/syncCores.js", Weave);
Cu.import("resource://weave/engines.js", Weave);
Cu.import("resource://weave/service.js", Weave);
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

  

  __bmkEngine: null,
  get _bmkEngine() {
    if (!this.__bmkEngine)
      this.__bmkEngine = new BookmarksEngine(DAV, this._cryptoId);
    return this.__bmkEngine;
  },

  __histEngine: null,
  get _histEngine() {
    if (!this.__histEngine)
      this.__histEngine = new HistoryEngine(DAV, this._cryptoId);
    return this.__histEngine;
  },

  __cookieEngine: null,
  get _cookieEngine() {
    if (!this.__cookieEngine)
      this.__cookieEngine = new CookieEngine(DAV, this._cryptoId);
    return this.__cookieEngine;
  },


  
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
    if (DAV.loggedIn)
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

    this._briefApp = Log4Moz.Service.newFileAppender("rotating", brief, formatter);
    this._briefApp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.briefLog")];
    root.addAppender(this._briefApp);
    this._debugApp = Log4Moz.Service.newFileAppender("rotating", verbose, formatter);
    this._debugApp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.debugLog")];
    root.addAppender(this._debugApp);
  },

  clearLogs: function WeaveSvc_clearLogs() {
    this._briefApp.clear();
    this._debugApp.clear();
  },

  _uploadVersion: function WeaveSync__uploadVersion() {
    let self = yield;

    DAV.MKCOL("meta", self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create meta information directory";

    DAV.PUT("meta/version", STORAGE_FORMAT_VERSION, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload server version file");
  },

  
  _versionCheck: function WeaveSync__versionCheck() {
    let self = yield;

    DAV.GET("meta/version", self.cb);
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

  _checkUserDir: function WeaveSvc__checkUserDir() {
    let self = yield;

    this._log.trace("Checking user directory exists");

    let serverURL = Utils.prefs.getCharPref("serverURL");
    if (serverURL[serverURL.length-1] != '/')
      serverURL = serverURL + '/';

    DAV.baseURL = serverURL;
    DAV.MKCOL("user/" + this.userPath, self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create user directory";

    DAV.baseURL = serverURL + "user/" + this.userPath + "/";
    this._log.info("Using server URL: " + DAV.baseURL);
  },

  _keyCheck: function WeaveSvc__keyCheck() {
    let self = yield;

    DAV.GET("private/privkey", self.cb);
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
  },

  _generateKeys: function WeaveSync__generateKeys() {
    let self = yield;

    this._log.debug("Generating new RSA key");
    Crypto.RSAkeygen.async(Crypto, self.cb, this._cryptoId);
    let [privkey, pubkey] = yield;

    this._cryptoId.privkey = privkey;
    this._cryptoId.pubkey = pubkey;

    DAV.MKCOL("private/", self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create private key directory";

    DAV.MKCOL("public/", self.cb);
    ret = yield;
    if (!ret)
      throw "Could not create public key directory";

    DAV.PUT("private/privkey", privkey, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload private key");

    DAV.PUT("public/pubkey", pubkey, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload public key");
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
    this._localLock(this._notify("login", this._login,
                                 password, passphrase)).async(this, onComplete);
  },
  _login: function WeaveSync__login(password, passphrase) {
    let self = yield;

    
    
    this._mozId.setTempPassword(password);
    this._cryptoId.setTempPassword(passphrase);

    this._log.debug("Logging in");

    if (!this.username)
      throw "No username set, login failed";
    if (!this.password)
      throw "No password given or found in password manager";

    this._checkUserDir.async(this, self.cb);
    yield;

    DAV.login.async(DAV, self.cb, this.username, this.password);
    let success = yield;
    if (!success)
      throw "Login failed";

    this._versionCheck.async(this, self.cb);
    yield;

    this._keyCheck.async(this, self.cb);
    yield;

    self.done(true);
  },

  logout: function WeaveSync_logout() {
    this._log.info("Logging out");
    DAV.logout();
    this._mozId.setTempPassword(null); 
    this._cryptoId.setTempPassword(null); 
    this._os.notifyObservers(null, "weave:service:logout:success", "");
  },

  resetLock: function WeaveSvc_resetLock(onComplete) {
    this._notify("reset-server-lock", this._resetLock).async(this, onComplete);
  },
  _resetLock: function WeaveSvc__resetLock() {
    let self = yield;
    DAV.forceUnlock.async(DAV, self.cb);
    yield;
  },

  serverWipe: function WeaveSvc_serverWipe(onComplete) {
    let cb = function WeaveSvc_serverWipeCb() {
      let self = yield;
      this._serverWipe.async(this, self.cb);
      yield;
      this.logout();
      self.done();
    };
    this._lock(this._notify("server-wipe", cb)).async(this, onComplete);
  },
  _serverWipe: function WeaveSvc__serverWipe() {
    let self = yield;

    DAV.listFiles.async(DAV, self.cb);
    let names = yield;

    for (let i = 0; i < names.length; i++) {
      DAV.DELETE(names[i], self.cb);
      let resp = yield;
      this._log.debug(resp.status);
    }
  },

  

  sync: function WeaveSync_sync(onComplete) {
    this._lock(this._notify("sync", this._sync)).async(this, onComplete);
  },
  _sync: function WeaveSync__sync() {
    let self = yield;

    if (!DAV.loggedIn)
      throw "Can't sync: Not logged in";

    this._versionCheck.async(this, self.cb);
    yield;

    this._keyCheck.async(this, self.cb);
    yield;

    if (Utils.prefs.getBoolPref("bookmarks")) {
      this._notify(this._bmkEngine.name + ":sync",
                   this._syncEngine, this._bmkEngine).async(this, self.cb);
      yield;
      this._bmkEngine.syncMounts(self.cb); 
      yield;
    }
    if (Utils.prefs.getBoolPref("history")) {
      this._notify(this._histEngine.name + ":sync",
                   this._syncEngine, this._histEngine).async(this, self.cb);
      yield;
    }
    if (this._prefs.getBoolPref("cookies")) {
	this._notify(this._cookieEngine.name + ":sync",
                    this._syncEngine, this._cookieEngine).async(this, self.cb);
      yield;
    }
  },
  _syncEngine: function WeaveSvc__syncEngine(engine) {
    let self = yield;
    engine.sync(self.cb);
    yield;
  },

  resetServer: function WeaveSync_resetServer(onComplete) {
    this._lock(this._notify("reset-server",
                            this._resetServer)).async(this, onComplete);
  },
  _resetServer: function WeaveSync__resetServer() {
    let self = yield;

    if (!DAV.loggedIn)
      throw "Can't reset server: Not logged in";

    this._bmkEngine.resetServer(self.cb);
    yield;
    this._histEngine.resetServer(self.cb);
    yield;
  },

  resetClient: function WeaveSync_resetClient(onComplete) {
    this._localLock(this._notify("reset-client",
                                 this._resetClient)).async(this, onComplete);
  },
  _resetClient: function WeaveSync__resetClient() {
    let self = yield;
    this._bmkEngine.resetClient(self.cb);
    yield;
    this._histEngine.resetClient(self.cb);
    yield;
  },

  shareBookmarks: function WeaveSync_shareBookmarks(onComplete, username) {
    this._lock(this._notify("share-bookmarks",
                            this._shareBookmarks,
                            username)).async(this, onComplete);
  },
  _shareBookmarks: function WeaveSync__shareBookmarks(username) {
    let self = yield;
    this._bmkEngine.share(self.cb, username);
    let ret = yield;
    self.done(ret);
  }

};
