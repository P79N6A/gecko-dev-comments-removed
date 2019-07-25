




































const EXPORTED_SYMBOLS = ['Weave'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;












const SCHEDULED_SYNC_INTERVAL = 60 * 1000 * 5; 




const INITIAL_THRESHOLD = 75;



const THRESHOLD_DECREMENT_STEP = 25;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/wrap.js");
Cu.import("resource://weave/faultTolerance.js");
Cu.import("resource://weave/auth.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/base_records/keys.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/oauth.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/clientData.js");

Function.prototype.async = Async.sugar;


let Weave = {};
Cu.import("resource://weave/constants.js", Weave);
Cu.import("resource://weave/util.js", Weave);
Cu.import("resource://weave/async.js", Weave);
Cu.import("resource://weave/faultTolerance.js", Weave);
Cu.import("resource://weave/auth.js", Weave);
Cu.import("resource://weave/resource.js", Weave);
Cu.import("resource://weave/base_records/keys.js", Weave);
Cu.import("resource://weave/notifications.js", Weave);
Cu.import("resource://weave/identity.js", Weave);
Cu.import("resource://weave/clientData.js", Weave);
Cu.import("resource://weave/stores.js", Weave);
Cu.import("resource://weave/syncCores.js", Weave);
Cu.import("resource://weave/engines.js", Weave);
Cu.import("resource://weave/oauth.js", Weave);
Cu.import("resource://weave/service.js", Weave); 

Utils.lazy(Weave, 'Service', WeaveSvc);






function WeaveSvc() {}
WeaveSvc.prototype = {

  _notify: Wrap.notify,
  _localLock: Wrap.localLock,
  _catchAll: Wrap.catchAll,
  _osPrefix: "weave:service:",
  _cancelRequested: false,
  _isQuitting: false,
  _loggedIn: false,
  _syncInProgress: false,

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

  __json: null,
  get _json() {
    if (!this.__json)
      this.__json = Cc["@mozilla.org/dom/json;1"].
        createInstance(Ci.nsIJSON);
    return this.__json;
  },

  
  _keyPair: {},

  
  _scheduleTimer: null,

  get username() {
    return Utils.prefs.getCharPref("username");
  },
  set username(value) {
    if (value)
      Utils.prefs.setCharPref("username", value);
    else
      Utils.prefs.clearUserPref("username");

    
    ID.get('WeaveID').username = value;
    ID.get('WeaveCryptoID').username = value;

    
    this._genKeyURLs();
  },

  get password() { return ID.get('WeaveID').password; },
  set password(value) { ID.get('WeaveID').password = value; },

  get passphrase() { return ID.get('WeaveCryptoID').password; },
  set passphrase(value) { ID.get('WeaveCryptoID').password = value; },

  
  set onGetPassword(value) {
    ID.get('WeaveID').onGetPassword = value;
  },
  set onGetPassphrase(value) {
    ID.get('WeaveCryptoID').onGetPassword = value;
  },

  get baseURL() {
    let url = Utils.prefs.getCharPref("serverURL");
    if (url && url[url.length-1] != '/')
      url = url + '/';
    return url;
  },
  set baseURL(value) {
    Utils.prefs.setCharPref("serverURL", value);
    this._genKeyURLs();
  },

  get userPath() { return ID.get('WeaveID').username; },

  get isLoggedIn() this._loggedIn,

  get isQuitting() this._isQuitting,
  set isQuitting(value) { this._isQuitting = value; },

  get cancelRequested() this._cancelRequested,
  set cancelRequested(value) { this._cancelRequested = value; },

  get enabled() Utils.prefs.getBoolPref("enabled"),

  get schedule() {
    if (!this.enabled)
      return 0; 
    return Utils.prefs.getIntPref("schedule");
  },

  onWindowOpened: function Weave__onWindowOpened() {
  },

  get locked() this._locked,
  lock: function Svc_lock() {
    if (this._locked)
      return false;
    this._locked = true;
    return true;
  },
  unlock: function Svc_unlock() {
    this._locked = false;
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
      this._log.warn("Invalid Weave scheduler setting: " + schedule);
      break;
    }
  },

  _enableSchedule: function WeaveSvc__enableSchedule() {
    if (this._scheduleTimer) {
      this._scheduleTimer.cancel();
      this._scheduleTimer = null;
    }
    this._scheduleTimer = Cc["@mozilla.org/timer;1"].
      createInstance(Ci.nsITimer);
    let listener = new Utils.EventListener(Utils.bind2(this, this._onSchedule));
    this._scheduleTimer.initWithCallback(listener, SCHEDULED_SYNC_INTERVAL,
                                         this._scheduleTimer.TYPE_REPEATING_SLACK);
    this._log.config("Weave scheduler enabled");
  },

  _disableSchedule: function WeaveSvc__disableSchedule() {
    if (this._scheduleTimer) {
      this._scheduleTimer.cancel();
      this._scheduleTimer = null;
    }
    this._log.config("Weave scheduler disabled");
  },

  _onSchedule: function WeaveSvc__onSchedule() {
    if (this.enabled) {
      if (this.locked) {
        this._log.info("Skipping scheduled sync; local operation in progress");
      } else {
        this._log.info("Running scheduled sync");
        this._notify("sync", "",
                     this._catchAll(this._localLock(this._sync))).async(this);
      }
    }
  },

  _genKeyURLs: function WeaveSvc__genKeyURLs() {
    let url = this.baseURL + this.username;
    PubKeys.defaultKeyUri = url + "/keys/pubkey";
    PrivKeys.defaultKeyUri = url + "/keys/privkey";
  },

  
  
  
  _onStartup: function WeaveSvc__onStartup() {
    let self = yield;
    this._initLogs();
    this._log.info("Weave Service Initializing");

    Utils.prefs.addObserver("", this, false);
    this._os.addObserver(this, "quit-application", true);
    FaultTolerance.Service; 

    if (!this.enabled)
      this._log.info("Weave Sync disabled");

    this._setSchedule(this.schedule);

    
    ID.set('WeaveID', new Identity('Mozilla Services Password', this.username));
    Auth.defaultAuthenticator = new BasicAuthenticator(ID.get('WeaveID'));

    ID.set('WeaveCryptoID',
           new Identity('Mozilla Services Encryption Passphrase', this.username));

    this._genKeyURLs();

    if (Utils.prefs.getBoolPref("autoconnect") &&
        this.username && this.username != 'nobody')
      try {
        yield this.login(self.cb);
        yield this.sync(self.cb);
      } catch (e) {}
    self.done();
  },
  onStartup: function WeaveSvc_onStartup() {
    this._onStartup.async(this);
  },

  _initLogs: function WeaveSvc__initLogs() {
    this._log = Log4Moz.repository.getLogger("Service.Main");
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.service.main")];

    let formatter = new Log4Moz.BasicFormatter();
    let root = Log4Moz.repository.rootLogger;
    root.level = Log4Moz.Level[Utils.prefs.getCharPref("log.rootLogger")];

    let capp = new Log4Moz.ConsoleAppender(formatter);
    capp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.console")];
    root.addAppender(capp);

    let dapp = new Log4Moz.DumpAppender(formatter);
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

    this._briefApp = new Log4Moz.RotatingFileAppender(brief, formatter);
    this._briefApp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.briefLog")];
    root.addAppender(this._briefApp);
    this._debugApp = new Log4Moz.RotatingFileAppender(verbose, formatter);
    this._debugApp.level = Log4Moz.Level[Utils.prefs.getCharPref("log.appender.debugLog")];
    root.addAppender(this._debugApp);
  },

  clearLogs: function WeaveSvc_clearLogs() {
    this._briefApp.clear();
    this._debugApp.clear();
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  

  observe: function WeaveSvc__observe(subject, topic, data) {
    switch (topic) {
      case "nsPref:changed":
        switch (data) {
          case "enabled": 
          case "schedule":
            this._setSchedule(this.schedule);
            break;
        }
        break;
      case "quit-application":
        this._onQuitApplication();
        break;
    }
  },

  _onQuitApplication: function WeaveSvc__onQuitApplication() {



























  },

  

  _verifyLogin: function WeaveSvc__verifyLogin(username, password) {
    let self = yield;
    this._log.debug("Verifying login for user " + username);
    let res = new Resource(this.baseURL + username);
    yield res.get(self.cb);
  },
  verifyLogin: function WeaveSvc_verifyLogin(onComplete, username, password) {
    this._localLock(this._notify("verify-login", "", this._verifyLogin,
                                 username, password)).async(this, onComplete);
  },

  _verifyPassphrase: function WeaveSvc__verifyPassphrase(username, password,
                                                         passphrase) {
    let self = yield;

    this._log.debug("Verifying passphrase");

    this.username = username;
    ID.get('WeaveID').setTempPassword(password);

    let id = new Identity('Passphrase Verification', username);
    id.setTempPassword(passphrase);

    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    let privkey = yield PrivKeys.get(self.cb, pubkey.PrivKeyUri);

    
  },
  verifyPassphrase: function WeaveSvc_verifyPassphrase(onComplete, username,
                                                       password, passphrase) {
    this._localLock(this._notify("verify-passphrase", "", this._verifyPassphrase,
                                 username, password, passphrase)).
      async(this, onComplete);
  },

  _login: function WeaveSvc__login(username, password, passphrase) {
    let self = yield;

    if (typeof(username) != 'undefined')
      this.username = username;
    if (typeof(password) != 'undefined')
      ID.get('WeaveID').setTempPassword(password);
    if (typeof(passphrase) != 'undefined')
      ID.get('WeaveCryptoID').setTempPassword(passphrase);

    if (!this.username)
      throw "No username set, login failed";
    if (!this.password)
      throw "No password given or found in password manager";

    this._log.debug("Logging in user " + this.username);

    yield this._verifyLogin.async(this, self.cb, this.username, this.password);

    this._loggedIn = true;
    self.done(true);
  },
  login: function WeaveSvc_login(onComplete, username, password, passphrase) {
    this._localLock(
      this._notify("login", "",
                   this._catchAll(this._login, username, password, passphrase))).
      async(this, onComplete);
  },

  logout: function WeaveSvc_logout() {
    this._log.info("Logging out");
    this._disableSchedule();
    this._loggedIn = false;
    this._keyPair = {};
    ID.get('WeaveID').setTempPassword(null); 
    ID.get('WeaveCryptoID').setTempPassword(null); 
    this._os.notifyObservers(null, "weave:service:logout:success", "");
  },

  serverWipe: function WeaveSvc_serverWipe(onComplete) {
    let cb = function WeaveSvc_serverWipeCb() {
      let self = yield;
      this._log.error("Server wipe not supported");
      this.logout();
    };
    this._notify("server-wipe", "", this._localLock(cb)).async(this, onComplete);
  },

  
  
  _remoteSetup: function WeaveSvc__remoteSetup() {
    let self = yield;
    let ret = false; 

    
    
    
    let needKeys = true;
    let pubkey = yield PubKeys.getDefaultKey(self.cb);
    if (pubkey) {
      
      let privkey = yield PrivKeys.get(self.cb, pubkey.privateKeyUri);
      if (privkey) {
        needKeys = false;
        ret = true;
      }
    }

    if (needKeys) {
      let pass = yield ID.get('WeaveCryptoID').getPassword(self.cb);
      if (pass) {
        let keys = PubKeys.createKeypair(pass, PubKeys.defaultKeyUri,
                                         PrivKeys.defaultKeyUri);
        try {
          yield keys.pubkey.put(self.cb);
          yield keys.privkey.put(self.cb);
          ret = true;
        } catch (e) {
          this._log.error("Could not upload keys: " + Utils.exceptionStr(e));
          this._log.error(keys.pubkey.lastRequest.responseText);
        }
      } else {
        this._log.warn("Could not get encryption passphrase");
      }
    }

    self.done(ret);
  },

  

  _sync: function WeaveSvc__sync() {
    let self = yield;

    if (!(yield this._remoteSetup.async(this, self.cb))) {
      throw "aborting sync, remote setup failed";
    }
    
    
    

    let engines = Engines.getAll();
    for each (let engine in engines) {
      if (this.cancelRequested)
        continue;

      if (!engine.enabled)
        continue;

      this._log.debug("Syncing engine " + engine.name);
      yield this._notify(engine.name + "-engine:sync", "",
                         this._syncEngine, engine).async(this, self.cb);
    }

    if (this._syncError) {
      this._syncError = false;
      throw "Some engines did not sync correctly";
    }

    this._log.debug("Sync complete");
  },
  sync: function WeaveSvc_sync(onComplete) {
    this._notify("sync", "",
                 this._catchAll(this._localLock(this._sync))).async(this, onComplete);
  },

  
  
  
  _syncThresholds: {},

  _syncAsNeeded: function WeaveSvc__syncAsNeeded() {
    let self = yield;

    let engines = Engines.getAll();
    for each (let engine in engines) {
      if (!engine.enabled)
        continue;

      if (!(engine.name in this._syncThresholds))
        this._syncThresholds[engine.name] = INITIAL_THRESHOLD;

      let score = engine.score;
      if (score >= this._syncThresholds[engine.name]) {
        this._log.debug(engine.name + " score " + score +
                        " reaches threshold " +
                        this._syncThresholds[engine.name] + "; syncing");
        this._notify(engine.name + "-engine:sync", "",
                     this._syncEngine, engine).async(this, self.cb);
        yield;

        
        
        
        
        
        
        
        this._syncThresholds[engine.name] = INITIAL_THRESHOLD;
      }
      else {
        this._log.debug(engine.name + " score " + score +
                        " does not reach threshold " +
                        this._syncThresholds[engine.name] + "; not syncing");

        
        
        
        
        this._syncThresholds[engine.name] -= THRESHOLD_DECREMENT_STEP;
        if (this._syncThresholds[engine.name] <= 0)
          this._syncThresholds[engine.name] = 1;
      }
    }

    if (this._syncError) {
      this._syncError = false;
      throw "Some engines did not sync correctly";
    }
  },

  _syncEngine: function WeaveSvc__syncEngine(engine) {
    let self = yield;
    try { yield engine.sync(self.cb); }
    catch(e) {
      
      this._log.warn("Engine exception: " + e);
      let ok = FaultTolerance.Service.onException(e);
      if (!ok)
        this._syncError = true;
    }
  },

  _resetServer: function WeaveSvc__resetServer() {
    let self = yield;

    let engines = Engines.getAll();
    for (let i = 0; i < engines.length; i++) {
      if (!engines[i].enabled)
        continue;
      engines[i].resetServer(self.cb);
      yield;
    }
  },
  resetServer: function WeaveSvc_resetServer(onComplete) {
    this._notify("reset-server", "",
                 this._localLock(this._resetServer)).async(this, onComplete);
  },

  _resetClient: function WeaveSvc__resetClient() {
    let self = yield;
    let engines = Engines.getAll();
    for (let i = 0; i < engines.length; i++) {
      if (!engines[i].enabled)
        continue;
      engines[i].resetClient(self.cb);
      yield;
    }
  },
  resetClient: function WeaveSvc_resetClient(onComplete) {
    this._localLock(this._notify("reset-client", "",
                                 this._resetClient)).async(this, onComplete);
  } 


















































































};
