




































const EXPORTED_SYMBOLS = ['Weave'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;












const SCHEDULED_SYNC_INTERVAL = 60 * 1000; 




const INITIAL_THRESHOLD = 100;



const THRESHOLD_DECREMENT_STEP = 5;

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
Cu.import("resource://weave/engines/cookies.js");
Cu.import("resource://weave/engines/bookmarks.js");
Cu.import("resource://weave/engines/history.js");
Cu.import("resource://weave/engines/passwords.js");
Cu.import("resource://weave/engines/forms.js");
Cu.import("resource://weave/engines/tabs.js");

Function.prototype.async = Async.sugar;


let Weave = {};
Cu.import("resource://weave/constants.js", Weave);
Cu.import("resource://weave/util.js", Weave);
Cu.import("resource://weave/async.js", Weave);
Cu.import("resource://weave/crypto.js", Weave);
Cu.import("resource://weave/notifications.js", Weave);
Cu.import("resource://weave/identity.js", Weave);
Cu.import("resource://weave/dav.js", Weave);
Cu.import("resource://weave/stores.js", Weave);
Cu.import("resource://weave/syncCores.js", Weave);
Cu.import("resource://weave/engines.js", Weave);
Cu.import("resource://weave/service.js", Weave);
Utils.lazy(Weave, 'Service', WeaveSvc);






function WeaveSvc(engines) {
  this._startupFinished = false;
  this._initLogs();
  this._log.info("Weave Sync Service Initializing");

  
  ID.set('WeaveID', new Identity('Mozilla Services Password', this.username));
  ID.set('WeaveCryptoID',
         new Identity('Mozilla Services Encryption Passphrase', this.username));

  
  ID.setAlias('WeaveID', 'DAV:default');
  ID.setAlias('WeaveCryptoID', 'Engine:PBE:default');

  if (typeof engines == "undefined")
    engines = [
      new BookmarksEngine(),
      new HistoryEngine(),
      new CookieEngine(),
      new PasswordEngine(),
      new FormEngine(),
      new TabEngine()
    ];

  
  for (let i = 0; i < engines.length; i++)
    Engines.register(engines[i]);

  
  Utils.prefs.addObserver("", this, false);
  this._os.addObserver(this, "quit-application", true);

  if (!this.enabled) {
    this._log.info("Weave Sync disabled");
    return;
  }
}
WeaveSvc.prototype = {

  _notify: Wrap.notify,
  _lock: Wrap.lock,
  _localLock: Wrap.localLock,
  _osPrefix: "weave:service:",
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
  },

  get password() { return ID.get('WeaveID').password; },
  set password(value) { ID.get('WeaveID').password = value; },

  get passphrase() { return ID.get('WeaveCryptoID').password; },
  set passphrase(value) { ID.get('WeaveCryptoID').password = value; },

  get userPath() { return ID.get('WeaveID').username; },

  get isLoggedIn() this._loggedIn,
  get enabled() Utils.prefs.getBoolPref("enabled"),

  get schedule() {
    if (!this.enabled)
      return 0; 
    return Utils.prefs.getIntPref("schedule");
  },

  onWindowOpened: function Weave__onWindowOpened() {
    if (!this._startupFinished) {
      if (Utils.prefs.getBoolPref("autoconnect") &&
          this.username && this.username != 'nobody')
        this._initialLoginAndSync.async(this);
      this._startupFinished = true;
    }
  },

  _initialLoginAndSync: function Weave__initialLoginAndSync() {
    let self = yield;

    
    
    

    yield this.login(self.cb);
    yield this.sync(self.cb);
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
    this._log.info("Weave scheduler enabled");
  },

  _disableSchedule: function WeaveSvc__disableSchedule() {
    if (this._scheduleTimer) {
      this._scheduleTimer.cancel();
      this._scheduleTimer = null;
    }
    this._log.info("Weave scheduler disabled");
  },

  _onSchedule: function WeaveSvc__onSchedule() {
    if (this.enabled) {
      if (DAV.locked) {
        this._log.info("Skipping scheduled sync; local operation in progress")
      } else {
        this._log.info("Running scheduled sync");
        this._notify("sync-as-needed", this._lock(this._syncAsNeeded)).async(this);
      }
    }
  },

  _initLogs: function WeaveSvc__initLogs() {
    this._log = Log4Moz.Service.getLogger("Service.Main");
    this._log.level =
      Log4Moz.Level[Utils.prefs.getCharPref("log.logger.service.main")];

    let formatter = new Log4Moz.BasicFormatter();
    let root = Log4Moz.Service.rootLogger;
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

  _uploadVersion: function WeaveSvc__uploadVersion() {
    let self = yield;

    DAV.MKCOL("meta", self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create meta information directory";

    DAV.PUT("meta/version", STORAGE_FORMAT_VERSION, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload server version file");
  },

  
  _versionCheck: function WeaveSvc__versionCheck() {
    let self = yield;

    DAV.GET("meta/version", self.cb);
    let ret = yield;

    if (!Utils.checkStatus(ret.status)) {
      this._log.info("Server has no version file.  Wiping server data.");
      yield this._serverWipe.async(this, self.cb);
      yield this._uploadVersion.async(this, self.cb);

    } else if (ret.responseText < STORAGE_FORMAT_VERSION) {
      this._log.info("Server version too low.  Wiping server data.");
      yield this._serverWipe.async(this, self.cb);
      yield this._uploadVersion.async(this, self.cb);

    } else if (ret.responseText > STORAGE_FORMAT_VERSION) {
      
    }
  },

  _checkUserDir: function WeaveSvc__checkUserDir() {
    let self = yield;
    let prefix = DAV.defaultPrefix;

    this._log.trace("Checking user directory exists");

    try {
      DAV.defaultPrefix = '';
      DAV.MKCOL("user/" + this.userPath, self.cb);
      let ret = yield;
      if (!ret)
        throw "Could not create user directory";
    }
    catch (e) { throw e; }
    finally { DAV.defaultPrefix = prefix; }
  },

  
  
  
  
  
  
  
  
  
  
  
  
  _getKeypair : function WeaveSvc__getKeypair(id, createIfNecessary) {
    let self = yield;

    if ("none" == Utils.prefs.getCharPref("encryption"))
      return;

    if (typeof(id) == "undefined")
      id = ID.get('WeaveCryptoID');

    if (typeof(createIfNecessary) == "undefined")
      createIfNecessary = true;

    this._log.trace("Retrieving keypair from server");

    let statuses = [[200, 300]];
    if (createIfNecessary)
      statuses.push(404);

    
    

    if (!(this._keyPair['private'] && this._keyPair['public'])) {
      this._log.info("Fetching keypair from server.");

      DAV.GET("private/privkey", self.cb);
      let privkeyResp = yield;
      Utils.ensureStatus(privkeyResp.status,
                         "Could not get private key from server", statuses);

      DAV.GET("public/pubkey", self.cb);
      let pubkeyResp = yield;
      Utils.ensureStatus(pubkeyResp.status,
                         "Could not get public key from server", statuses);

      if (privkeyResp.status == 404 || pubkeyResp.status == 404) {
        yield this._generateKeys.async(this, self.cb);
        return;
      }

      this._keyPair['private'] = this._json.decode(privkeyResp.responseText);
      this._keyPair['public'] = this._json.decode(pubkeyResp.responseText);
    } else {
      this._log.info("Using cached keypair");
    }

    let privkeyData = this._keyPair['private']
    let pubkeyData  = this._keyPair['public'];

    if (!privkeyData || !pubkeyData)
      throw "Bad keypair JSON";
    if (privkeyData.version != 1 || pubkeyData.version != 1)
      throw "Unexpected keypair data version";
    if (privkeyData.algorithm != "RSA" || pubkeyData.algorithm != "RSA")
      throw "Only RSA keys currently supported";


    id.keypairAlg     = privkeyData.algorithm;
    id.privkey        = privkeyData.privkey;
    id.privkeyWrapIV  = privkeyData.privkeyIV;
    id.passphraseSalt = privkeyData.privkeySalt;

    id.pubkey = pubkeyData.pubkey;

    let isValid = yield Crypto.isPassphraseValid.async(Crypto, self.cb, id);
    if (!isValid)
      throw new Error("Passphrase is not valid.");
  },

  _generateKeys: function WeaveSvc__generateKeys() {
    let self = yield;

    this._log.debug("Generating new RSA key");

    
    let id = ID.get('WeaveCryptoID');
    Crypto.RSAkeygen.async(Crypto, self.cb, id);
    yield;

    DAV.MKCOL("private/", self.cb);
    let ret = yield;
    if (!ret)
      throw "Could not create private key directory";

    DAV.MKCOL("public/", self.cb);
    ret = yield;
    if (!ret)
      throw "Could not create public key directory";

    let privkeyData = { version     : 1,
                        algorithm   : id.keypairAlg,
                        privkey     : id.privkey,
                        privkeyIV   : id.privkeyWrapIV,
                        privkeySalt : id.passphraseSalt
                      };
    let data = this._json.encode(privkeyData);

    DAV.PUT("private/privkey", data, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload private key");


    let pubkeyData = { version   : 1,
                       algorithm : id.keypairAlg,
                       pubkey    : id.pubkey
                     };
    data = this._json.encode(pubkeyData);

    DAV.PUT("public/pubkey", data, self.cb);
    ret = yield;
    Utils.ensureStatus(ret.status, "Could not upload public key");
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
    if (!this.enabled ||
        !Utils.prefs.getBoolPref("syncOnQuit.enabled") ||
        !this._loggedIn)
      return;

    let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
             getService(Ci.nsIWindowWatcher);

    
    
    let window = ww.openWindow(null,
                               "chrome://weave/content/status.xul",
                               "Weave:status",
                               "chrome,centerscreen,modal",
                               null);
  },

  

  verifyPassphrase: function WeaveSvc_verifyPassphrase(username, password,
                                                       passphrase) {
    this._localLock(this._notify("verify-passphrase",
                                 this._verifyPassphrase,
                                 username,
                                 password,
                                 passphrase)).async(this, null);
  },

  _verifyPassphrase: function WeaveSvc__verifyPassphrase(username, password,
                                                         passphrase) {
    let self = yield;

    this._log.debug("Verifying passphrase");

    yield this._verifyLogin.async(this, self.cb, username, password);
    let id = new Identity('Passphrase Verification', username);
    id.setTempPassword(passphrase);
    
    
    
    yield this._getKeypair.async(this, self.cb, id, false);
    let isValid = yield Crypto.isPassphraseValid.async(Crypto, self.cb, id);
    if (!isValid)
      throw new Error("Passphrase is not valid.");
  },

  verifyLogin: function WeaveSvc_verifyLogin(username, password) {
    this._log.debug("Verifying login for user " + username);

    this._localLock(this._notify("verify-login", this._verifyLogin,
                                 username, password)).async(this, null);
  },

  _verifyLogin: function WeaveSvc__verifyLogin(username, password) {
    let self = yield;

    this.username = username;
    this.password = password;

    DAV.baseURL = Utils.prefs.getCharPref("serverURL");
    DAV.defaultPrefix = "user/" + username;

    this._log.info("Using server URL: " + DAV.baseURL + DAV.defaultPrefix);
    let status = yield DAV.checkLogin.async(DAV, self.cb, username, password);
    if (status == 404) {
      
      yield this._checkUserDir.async(this, self.cb);
      status = yield DAV.checkLogin.async(DAV, self.cb, username, password);
    }

    Utils.ensureStatus(status, "Login verification failed");
  },

  login: function WeaveSvc_login(onComplete) {
    this._localLock(this._notify("login", this._login)).async(this, onComplete);
  },
  _login: function WeaveSvc__login() {
    let self = yield;

    this._log.debug("Logging in user " + this.username);

    if (!this.username)
      throw "No username set, login failed";
    if (!this.password)
      throw "No password given or found in password manager";

    yield this._verifyLogin.async(this, self.cb, this.username,
                                  this.password);
    yield this._versionCheck.async(this, self.cb);
    yield this._getKeypair.async(this, self.cb);

    this._setSchedule(this.schedule);

    this._loggedIn = true;
    self.done(true);
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
    this._notify("server-wipe", this._lock(cb)).async(this, onComplete);
  },
  _serverWipe: function WeaveSvc__serverWipe() {
    let self = yield;

    this._keyPair = {};
    DAV.listFiles.async(DAV, self.cb);
    let names = yield;

    for (let i = 0; i < names.length; i++) {
      if (names[i].match(/\.htaccess$/))
        continue;
      DAV.DELETE(names[i], self.cb);
      let resp = yield;
      this._log.debug(resp.status);
    }
  },

  

  sync: function WeaveSvc_sync(onComplete) {
    this._notify("sync", this._lock(this._sync)).async(this, onComplete);
  },

  _sync: function WeaveSvc__sync() {
    let self = yield;

    yield this._versionCheck.async(this, self.cb);
    yield this._getKeypair.async(this, self.cb);

    let engines = Engines.getAll();
    for (let i = 0; i < engines.length; i++) {
      if (!engines[i].enabled)
        continue;
      yield this._notify(engines[i].name + "-engine:sync",
                         this._syncEngine, engines[i]).async(this, self.cb);
    }

    if (this._syncError) {
      this._syncError = false;
      throw "Some engines did not sync correctly";
    }
  },

  
  
  
  _syncThresholds: {},

  _syncAsNeeded: function WeaveSvc__syncAsNeeded() {
    let self = yield;

    yield this._versionCheck.async(this, self.cb);
    yield this._getKeypair.async(this, self.cb);

    let engines = Engines.getAll();
    for each (let engine in engines) {
      if (!engine.enabled)
        continue;

      if (!(engine.name in this._syncThresholds))
        this._syncThresholds[engine.name] = INITIAL_THRESHOLD;

      let score = engine._tracker.score;
      if (score >= this._syncThresholds[engine.name]) {
        this._log.debug(engine.name + " score " + score +
                        " reaches threshold " +
                        this._syncThresholds[engine.name] + "; syncing");
        this._notify(engine.name + "-engine:sync",
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
    try {
      yield engine.sync(self.cb);
      engine._tracker.resetScore();
    } catch(e) {
      this._log.error(Utils.exceptionStr(e));
      this._log.error(Utils.stackTrace(e));
      this._syncError = true;
    }
  },

  resetServer: function WeaveSvc_resetServer(onComplete) {
    this._notify("reset-server",
                 this._lock(this._resetServer)).async(this, onComplete);
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

  resetClient: function WeaveSvc_resetClient(onComplete) {
    this._localLock(this._notify("reset-client",
                                 this._resetClient)).async(this, onComplete);
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

  shareData: function WeaveSvc_shareData(dataType,
					 isShareEnabled,
                                         onComplete,
                                         guid,
                                         username) {
    










    let messageName = "share-" + dataType;
    




    let self = this;
    let saved_dataType = dataType;
    let saved_onComplete = onComplete;
    let saved_guid = guid;
    let saved_username = username;
    let saved_isShareEnabled = isShareEnabled;
    let successMsg = "weave:service:global:success";
    let errorMsg = "weave:service:global:error";
    let os = Cc["@mozilla.org/observer-service;1"].
                      getService(Ci.nsIObserverService);

    let observer = {
      observe: function(subject, topic, data) {
	if (!Weave.DAV.locked) {
           self._notify(messageName, self._lock(self._shareData,
					saved_dataType,
					saved_isShareEnabled,
                                        saved_guid,
                                    saved_username)).async(self,
							   saved_onComplete);
	  os.removeObserver(observer, successMsg);
	  os.removeObserver(observer, errorMsg);
	}
      },
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
    };

    if (Weave.DAV.locked) {
      
      dump( "DAV is locked, gonna set up observer to do it later.\n");
      os.addObserver( observer, successMsg, true );
      os.addObserver( observer, errorMsg, true );
    } else {
      
      dump( "DAV not locked, doing it now.\n");
      observer.observe();
    }
  },

  _shareData: function WeaveSvc__shareData(dataType,
					   isShareEnabled,
                                           guid,
                                           username) {
    let self = yield;
    let ret;
    if (Engines.get(dataType).enabled) {
      if (isShareEnabled) {
        Engines.get(dataType).share(self.cb, guid, username);
      } else {
        Engines.get(dataType).stopSharing(self.cb, guid, username);
      }
      ret = yield;
    } else {
      this._log.warn( "Can't share disabled data type: " + dataType );
      ret = false;
    }
    self.done(ret);
  }

};
