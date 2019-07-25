





































const EXPORTED_SYMBOLS = ['Weave'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


const IDLE_TIME = 5; 


const CLUSTER_BACKOFF = 5 * 60 * 1000; 

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-sync/auth.js");
Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/base_records/keys.js");
Cu.import("resource://services-sync/base_records/wbo.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/ext/Sync.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/util.js");


let Weave = {};
Cu.import("resource://services-sync/auth.js", Weave);
Cu.import("resource://services-sync/constants.js", Weave);
Cu.import("resource://services-sync/base_records/keys.js", Weave);
Cu.import("resource://services-sync/engines.js", Weave);
Cu.import("resource://services-sync/engines/bookmarks.js", Weave);
Cu.import("resource://services-sync/engines/clients.js", Weave);
Cu.import("resource://services-sync/engines/forms.js", Weave);
Cu.import("resource://services-sync/engines/history.js", Weave);
Cu.import("resource://services-sync/engines/prefs.js", Weave);
Cu.import("resource://services-sync/engines/passwords.js", Weave);
Cu.import("resource://services-sync/engines/tabs.js", Weave);
Cu.import("resource://services-sync/ext/Preferences.js");
Cu.import("resource://services-sync/identity.js", Weave);
Cu.import("resource://services-sync/notifications.js", Weave);
Cu.import("resource://services-sync/resource.js", Weave);
Cu.import("resource://services-sync/status.js", Weave);
Cu.import("resource://services-sync/stores.js", Weave);
Cu.import("resource://services-sync/util.js", Weave);

Utils.lazy(Weave, 'Service', WeaveSvc);






function WeaveSvc() {
  this._notify = Utils.notify("weave:service:");
}
WeaveSvc.prototype = {

  _lock: Utils.lock,
  _catch: Utils.catch,
  _locked: false,
  _loggedIn: false,
  keyGenEnabled: true,

  get username() {
    return Svc.Prefs.get("username", "").toLowerCase();
  },
  set username(value) {
    if (value) {
      
      value = value.toLowerCase();
      
      
      value = value.replace("\t", "", "g");
      Svc.Prefs.set("username", value);
    }
    else
      Svc.Prefs.reset("username");

    
    ID.get('WeaveID').username = value;
    ID.get('WeaveCryptoID').username = value;

    
    this._updateCachedURLs();
  },

  get password() ID.get("WeaveID").password,
  set password(value) ID.get("WeaveID").password = value,

  get passphrase() ID.get("WeaveCryptoID").password,
  set passphrase(value) ID.get("WeaveCryptoID").password = value,
  get passphraseUTF8() ID.get("WeaveCryptoID").passwordUTF8,

  get serverURL() Svc.Prefs.get("serverURL"),
  set serverURL(value) {
    
    if (value == this.serverURL)
      return;

    
    Svc.Prefs.set("serverURL", value);
    Svc.Prefs.reset("clusterURL");
  },

  get clusterURL() Svc.Prefs.get("clusterURL", ""),
  set clusterURL(value) {
    Svc.Prefs.set("clusterURL", value);
    this._updateCachedURLs();
  },

  get miscAPI() {
    
    let misc = Svc.Prefs.get("miscURL");
    if (misc.indexOf(":") == -1)
      misc = this.serverURL + misc;
    return misc + "1.0/";
  },

  get userAPI() {
    
    let user = Svc.Prefs.get("userURL");
    if (user.indexOf(":") == -1)
      user = this.serverURL + user;
    return user + "1.0/";
  },

  get pwResetURL() {
    return this.serverURL + "weave-password-reset";
  },

  get updatedURL() {
    return WEAVE_CHANNEL == "dev" ? UPDATED_DEV_URL : UPDATED_REL_URL;
  },

  get syncID() {
    
    let syncID = Svc.Prefs.get("client.syncID", "");
    return syncID == "" ? this.syncID = Utils.makeGUID() : syncID;
  },
  set syncID(value) {
    Svc.Prefs.set("client.syncID", value);
  },

  get isLoggedIn() { return this._loggedIn; },

  
  get nextSync() Svc.Prefs.get("nextSync", 0) * 1000,
  set nextSync(value) Svc.Prefs.set("nextSync", Math.floor(value / 1000)),
  get nextHeartbeat() Svc.Prefs.get("nextHeartbeat", 0) * 1000,
  set nextHeartbeat(value) Svc.Prefs.set("nextHeartbeat", Math.floor(value / 1000)),

  get syncInterval() {
    
    if (Status.partial && Clients.clientType != "mobile")
      return PARTIAL_DATA_SYNC;
    return Svc.Prefs.get("syncInterval", MULTI_MOBILE_SYNC);
  },
  set syncInterval(value) Svc.Prefs.set("syncInterval", value),

  get syncThreshold() Svc.Prefs.get("syncThreshold", SINGLE_USER_THRESHOLD),
  set syncThreshold(value) Svc.Prefs.set("syncThreshold", value),

  get globalScore() Svc.Prefs.get("globalScore", 0),
  set globalScore(value) Svc.Prefs.set("globalScore", value),

  get numClients() Svc.Prefs.get("numClients", 0),
  set numClients(value) Svc.Prefs.set("numClients", value),

  get locked() { return this._locked; },
  lock: function Svc_lock() {
    if (this._locked)
      return false;
    this._locked = true;
    return true;
  },
  unlock: function Svc_unlock() {
    this._locked = false;
  },

  _updateCachedURLs: function _updateCachedURLs() {
    
    if (this.clusterURL == "" || this.username == "")
      return;

    let storageAPI = this.clusterURL + Svc.Prefs.get("storageAPI") + "/";
    let userBase = storageAPI + this.username + "/";
    this._log.debug("Caching URLs under storage user base: " + userBase);

    
    this.infoURL = userBase + "info/collections";
    this.storageURL = userBase + "storage/";
    this.metaURL = this.storageURL + "meta/global";
    PubKeys.defaultKeyUri = this.storageURL + "keys/pubkey";
    PrivKeys.defaultKeyUri = this.storageURL + "keys/privkey";
  },

  _checkCrypto: function WeaveSvc__checkCrypto() {
    let ok = false;

    try {
      let iv = Svc.Crypto.generateRandomIV();
      if (iv.length == 24)
        ok = true;

    } catch (e) {
      this._log.debug("Crypto check failed: " + e);
    }

    return ok;
  },

  


  onStartup: function onStartup() {
    this._migratePrefs();
    this._initLogs();
    this._log.info("Loading Weave " + WEAVE_VERSION);

    this.enabled = true;

    this._registerEngines();

    let ua = Cc["@mozilla.org/network/protocol;1?name=http"].
      getService(Ci.nsIHttpProtocolHandler).userAgent;
    this._log.info(ua);

    if (!this._checkCrypto()) {
      this.enabled = false;
      this._log.error("Could not load the Weave crypto component. Disabling " +
                      "Weave, since it will not work correctly.");
    }

    Svc.Obs.add("network:offline-status-changed", this);
    Svc.Obs.add("private-browsing", this);
    Svc.Obs.add("weave:service:sync:finish", this);
    Svc.Obs.add("weave:service:sync:error", this);
    Svc.Obs.add("weave:service:backoff:interval", this);
    Svc.Obs.add("weave:engine:score:updated", this);
    Svc.Obs.add("weave:resource:status:401", this);

    if (!this.enabled)
      this._log.info("Weave Sync disabled");

    
    ID.set('WeaveID', new Identity(PWDMGR_PASSWORD_REALM, this.username));
    Auth.defaultAuthenticator = new BasicAuthenticator(ID.get('WeaveID'));

    ID.set('WeaveCryptoID',
           new Identity(PWDMGR_PASSPHRASE_REALM, this.username));

    this._updateCachedURLs();

    
    
    let delay = Svc.Prefs.get("autoconnectDelay");
    if (delay) {
      this.delayedAutoConnect(delay);
    }

    
    
    
    Utils.delay(function() Svc.Obs.notify("weave:service:ready"), 0);
  },

  _checkSetup: function WeaveSvc__checkSetup() {
    if (!this.username) {
      this._log.debug("checkSetup: no username set");
      Status.login = LOGIN_FAILED_NO_USERNAME;
    }
    else if (!Utils.mpLocked() && !this.password) {
      this._log.debug("checkSetup: no password set");
      Status.login = LOGIN_FAILED_NO_PASSWORD;
    }
    else if (!Utils.mpLocked() && !this.passphrase) {
      this._log.debug("checkSetup: no passphrase set");
      Status.login = LOGIN_FAILED_NO_PASSPHRASE;
    }
    else
      Status.service = STATUS_OK;

    return Status.service;
  },

  _migratePrefs: function _migratePrefs() {
    
    if (Svc.Prefs.get("migrated", false))
      return;

    
    let oldPrefBranch = "extensions.weave.";
    let oldPrefNames = Cc["@mozilla.org/preferences-service;1"].
                       getService(Ci.nsIPrefService).
                       getBranch(oldPrefBranch).
                       getChildList("", {});

    
    let oldPref = new Preferences(oldPrefBranch);
    for each (let pref in oldPrefNames)
      Svc.Prefs.set(pref, oldPref.get(pref));

    
    oldPref.resetBranch("");
    Svc.Prefs.set("migrated", true);
  },

  _initLogs: function WeaveSvc__initLogs() {
    this._log = Log4Moz.repository.getLogger("Service.Main");
    this._log.level =
      Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];

    let formatter = new Log4Moz.BasicFormatter();
    let root = Log4Moz.repository.rootLogger;
    root.level = Log4Moz.Level[Svc.Prefs.get("log.rootLogger")];

    let capp = new Log4Moz.ConsoleAppender(formatter);
    capp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.console")];
    root.addAppender(capp);

    let dapp = new Log4Moz.DumpAppender(formatter);
    dapp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.dump")];
    root.addAppender(dapp);

    let verbose = Svc.Directory.get("ProfD", Ci.nsIFile);
    verbose.QueryInterface(Ci.nsILocalFile);
    verbose.append("weave");
    verbose.append("logs");
    verbose.append("verbose-log.txt");
    if (!verbose.exists())
      verbose.create(verbose.NORMAL_FILE_TYPE, PERMS_FILE);

    let maxSize = 65536; 
    this._debugApp = new Log4Moz.RotatingFileAppender(verbose, formatter, maxSize);
    this._debugApp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.debugLog")];
    root.addAppender(this._debugApp);
  },

  clearLogs: function WeaveSvc_clearLogs() {
    this._debugApp.clear();
  },

  


  _registerEngines: function WeaveSvc__registerEngines() {
    let engines = [];
    
    
    let pref = Svc.Prefs.get("registerEngines");
    if (pref) {
      engines = pref.split(",");
    }

    
    Engines.register(engines.map(function(name) Weave[name + "Engine"]));
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  

  observe: function WeaveSvc__observe(subject, topic, data) {
    switch (topic) {
      case "network:offline-status-changed":
        
        this._log.trace("Network offline status change: " + data);
        this._checkSyncStatus();
        break;
      case "private-browsing":
        
        this._log.trace("Private browsing change: " + data);
        this._checkSyncStatus();
        break;
      case "weave:service:sync:error":
        this._handleSyncError();
        if (Status.sync == CREDENTIALS_CHANGED) {
          this.logout();
          Utils.delay(function() this.login(), 0, this);
        }
        break;
      case "weave:service:sync:finish":
        this._scheduleNextSync();
        this._syncErrors = 0;
        break;
      case "weave:service:backoff:interval":
        let interval = (data + Math.random() * data * 0.25) * 1000; 
        Status.backoffInterval = interval;
        Status.minimumNextSync = Date.now() + data;
        break;
      case "weave:engine:score:updated":
        this._handleScoreUpdate();
        break;
      case "weave:resource:status:401":
        this._handleResource401(subject);
        break;
      case "idle":
        this._log.trace("Idle time hit, trying to sync");
        Svc.Idle.removeIdleObserver(this, this._idleTime);
        this._idleTime = 0;
        Utils.delay(function() this.sync(false), 0, this);
        break;
    }
  },

  _handleScoreUpdate: function WeaveSvc__handleScoreUpdate() {
    const SCORE_UPDATE_DELAY = 3000;
    Utils.delay(this._calculateScore, SCORE_UPDATE_DELAY, this, "_scoreTimer");
  },

  _calculateScore: function WeaveSvc_calculateScoreAndDoStuff() {
    var engines = Engines.getEnabled();
    for (let i = 0;i < engines.length;i++) {
      this._log.trace(engines[i].name + ": score: " + engines[i].score);
      this.globalScore += engines[i].score;
      engines[i]._tracker.resetScore();
    }

    this._log.trace("Global score updated: " + this.globalScore);
    this._checkSyncStatus();
  },

  _handleResource401: function _handleResource401(request) {
    
    let spec = request.resource.spec;
    let cluster = this.clusterURL;
    if (spec.indexOf(cluster) != 0)
      return;

    
    if (!this._setCluster())
      return;

    
    request.newUri = this.clusterURL + spec.slice(cluster.length);
  },

  
  _findCluster: function _findCluster() {
    this._log.debug("Finding cluster for user " + this.username);

    let fail;
    let res = new Resource(this.userAPI + this.username + "/node/weave");
    try {
      let node = res.get();
      switch (node.status) {
        case 400:
          Status.login = LOGIN_FAILED_LOGIN_REJECTED;
          fail = "Find cluster denied: " + this._errorStr(node);
          break;
        case 404:
          this._log.debug("Using serverURL as data cluster (multi-cluster support disabled)");
          return this.serverURL;
        case 0:
        case 200:
          if (node == "null")
            node = null;
          return node;
        default:
          fail = "Unexpected response code: " + node.status;
          break;
      }
    } catch (e) {
      this._log.debug("Network error on findCluster");
      Status.login = LOGIN_FAILED_NETWORK_ERROR;
      fail = e;
    }
    throw fail;
  },

  
  _setCluster: function _setCluster() {
    
    let cluster = this._findCluster();
    this._log.debug("cluster value = " + cluster);
    if (cluster == null)
      return false;

    
    if (cluster == this.clusterURL)
      return false;

    this.clusterURL = cluster;
    return true;
  },

  
  _updateCluster: function _updateCluster() {
    let cTime = Date.now();
    let lastUp = parseFloat(Svc.Prefs.get("lastClusterUpdate"));
    if (!lastUp || ((cTime - lastUp) >= CLUSTER_BACKOFF)) {
      if (this._setCluster()) {
        Svc.Prefs.set("lastClusterUpdate", cTime.toString());
        return true;
      }
    }
    return false;
  },

  verifyLogin: function verifyLogin()
    this._notify("verify-login", "", function() {
      
      
      
      if (this.clusterURL == "" && !this._setCluster()) {
        Status.sync = NO_SYNC_NODE_FOUND;
        Svc.Obs.notify("weave:service:sync:delayed");
        return true;
      }

      if (!this.username) {
        Status.login = LOGIN_FAILED_NO_USERNAME;
        return false;
      }

      try {
        let test = new Resource(this.infoURL).get();
        switch (test.status) {
          case 200:
            
            if (!this.passphrase) {
              Status.login = LOGIN_FAILED_NO_PASSPHRASE;
              return false;
            }

            
            if (!this._verifyPassphrase()) {
              Status.login = LOGIN_FAILED_INVALID_PASSPHRASE;
              return false;
            }

            
            Status.login = LOGIN_SUCCEEDED;
            return true;

          case 401:
            
            
            let id = ID.get('WeaveID');
            if (id.password != id.passwordUTF8) {
              let res = new Resource(this.infoURL);
              let auth = new BrokenBasicAuthenticator(id);
              res.authenticator = auth;
              test = res.get();
              if (test.status == 200) {
                this._log.debug("Non-ASCII password detected. "
                                + "Changing to UTF-8 version.");
                
                let url = this.userAPI + this.username + "/password";
                res = new Resource(url);
                res.authenticator = auth;
                res.post(id.passwordUTF8);
                return this.verifyLogin();
              }
            }
            
          case 404:
            
            if (this._setCluster())
              return this.verifyLogin();

            
            Status.login = LOGIN_FAILED_LOGIN_REJECTED;
            return false;

          default:
            
            this._checkServerError(test);
            Status.login = LOGIN_FAILED_SERVER_ERROR;
            return false;
        }
      }
      catch (ex) {
        
        this._log.debug("verifyLogin failed: " + Utils.exceptionStr(ex));
        Status.login = LOGIN_FAILED_NETWORK_ERROR;
        return false;
      }
    })(),

  _verifyPassphrase: function _verifyPassphrase()
    this._catch(this._notify("verify-passphrase", "", function() {
      
      if (!this.passphrase)
        return false;

      try {
        let pubkey = PubKeys.getDefaultKey();
        let privkey = PrivKeys.get(pubkey.privateKeyUri);
        let result = Svc.Crypto.verifyPassphrase(
          privkey.payload.keyData, this.passphraseUTF8,
          privkey.payload.salt, privkey.payload.iv
        );
        if (result)
          return true;

        
        
        result = Svc.Crypto.verifyPassphrase(
          privkey.payload.keyData, this.passphrase,
          privkey.payload.salt, privkey.payload.iv
        );
        if (result)
          this._needUpdatedKeys = true;
        return result;
      } catch (e) {
        
        return true;
      }
    }))(),

  changePassword: function WeaveSvc_changePassword(newpass)
    this._notify("changepwd", "", function() {
      let url = this.userAPI + this.username + "/password";
      try {
        let resp = new Resource(url).post(Utils.encodeUTF8(newpass));
        if (resp.status != 200) {
          this._log.debug("Password change failed: " + resp);
          return false;
        }
      }
      catch(ex) {
        
        this._log.debug("changePassword failed: " + Utils.exceptionStr(ex));
        return false;
      }

      
      this.password = newpass;
      this.persistLogin();
      return true;
    })(),

  changePassphrase: function WeaveSvc_changePassphrase(newphrase)
    this._catch(this._notify("changepph", "", function() {
      
      this.wipeServer();
      PubKeys.clearCache();
      PrivKeys.clearCache();

      this.logout();

      
      this.passphrase = newphrase;
      this.persistLogin();

      
      this.login();
      this.sync(true);
      return true;
    }))(),

  startOver: function() {
    
    Status.login = LOGIN_FAILED_NO_USERNAME;
    this.logout();
    
    this.resetClient();
    
    Svc.Prefs.resetBranch("");
    
    Svc.Prefs.set("lastversion", WEAVE_VERSION);
    
    this.password = "";
    this.passphrase = "";
    Svc.Login.findLogins({}, PWDMGR_HOST, "", "").map(function(login) {
      Svc.Login.removeLogin(login);
    });
    Svc.Obs.notify("weave:service:start-over");
  },

  delayedAutoConnect: function delayedAutoConnect(delay) {
    if (this._loggedIn)
      return;

    if (this._checkSetup() == STATUS_OK && Svc.Prefs.get("autoconnect")) {
      Utils.delay(this._autoConnect, delay * 1000, this, "_autoTimer");
    }
  },

  _autoConnect: let (attempts = 0) function _autoConnect() {
    let reason = 
      Utils.mpLocked() ? "master password still locked"
                       : this._checkSync([kSyncNotLoggedIn, kFirstSyncChoiceNotMade]);

    
    if (!reason) {
      if (!this.username || !this.password || !this.passphrase)
        return;

      
      if (this.login())
        return;
    }

    
    let interval = this._calculateBackoff(++attempts, 60 * 1000);
    this._log.debug("Autoconnect failed: " + (reason || Status.login) +
      "; retry in " + Math.ceil(interval / 1000) + " sec.");
    Utils.delay(function() this._autoConnect(), interval, this, "_autoTimer");
  },

  persistLogin: function persistLogin() {
    
    try {
      ID.get("WeaveID").persist();
      ID.get("WeaveCryptoID").persist();
    }
    catch(ex) {}
  },

  login: function WeaveSvc_login(username, password, passphrase)
    this._catch(this._lock(this._notify("login", "", function() {
      this._loggedIn = false;
      if (Svc.IO.offline)
        throw "Application is offline, login should not be called";

      if (username)
        this.username = username;
      if (password)
        this.password = password;
      if (passphrase)
        this.passphrase = passphrase;

      if (this._checkSetup() == CLIENT_NOT_CONFIGURED)
        throw "aborting login, client not configured";

      this._log.info("Logging in user " + this.username);

      if (!this.verifyLogin()) {
        
        throw "Login failed: " + Status.login;
      }

      
      if (this._autoTimer)
        this._autoTimer.clear();

      this._loggedIn = true;
      
      this._checkSyncStatus();
      Svc.Prefs.set("autoconnect", true);

      return true;
    })))(),

  logout: function WeaveSvc_logout() {
    
    if (!this._loggedIn)
      return;

    this._log.info("Logging out");
    this._loggedIn = false;

    
    this._checkSyncStatus();
    Svc.Prefs.set("autoconnect", false);

    Svc.Obs.notify("weave:service:logout:finish");
  },

  _errorStr: function WeaveSvc__errorStr(code) {
    switch (code.toString()) {
    case "1":
      return "illegal-method";
    case "2":
      return "invalid-captcha";
    case "3":
      return "invalid-username";
    case "4":
      return "cannot-overwrite-resource";
    case "5":
      return "userid-mismatch";
    case "6":
      return "json-parse-failure";
    case "7":
      return "invalid-password";
    case "8":
      return "invalid-record";
    case "9":
      return "weak-password";
    default:
      return "generic-server-error";
    }
  },

  checkUsername: function WeaveSvc_checkUsername(username) {
    let url = this.userAPI + username;
    let res = new Resource(url);
    res.authenticator = new NoOpAuthenticator();

    let data = "";
    try {
      data = res.get();
      if (data.status == 200) {
        if (data == "0")
          return "available";
        else if (data == "1")
          return "notAvailable";
      }

    }
    catch(ex) {}

    
    return this._errorStr(data);
  },

  createAccount: function WeaveSvc_createAccount(username, password, email,
                                            captchaChallenge, captchaResponse) {
    let payload = JSON.stringify({
      "password": Utils.encodeUTF8(password),
      "email": email,
      "captcha-challenge": captchaChallenge,
      "captcha-response": captchaResponse
    });

    let url = this.userAPI + username;
    let res = new Resource(url);
    res.authenticator = new Weave.NoOpAuthenticator();

    
    
    if (Svc.Prefs.isSet("admin-secret"))
      res.setHeader("X-Weave-Secret", Svc.Prefs.get("admin-secret", ""));

    let error = "generic-server-error";
    try {
      let register = res.put(payload);
      if (register.success) {
        this._log.info("Account created: " + register);
        return null;
      }

      
      if (register.status == 400)
        error = this._errorStr(register);
    }
    catch(ex) {
      this._log.warn("Failed to create account: " + ex);
    }

    return error;
  },

  
  
  _remoteSetup: function WeaveSvc__remoteSetup() {
    let reset = false;

    this._log.trace("Fetching global metadata record");
    let meta = Records.import(this.metaURL);

    let remoteVersion = (meta && meta.payload.storageVersion)?
      meta.payload.storageVersion : "";

    this._log.debug(["Weave Version:", WEAVE_VERSION, "Local Storage:",
      STORAGE_VERSION, "Remote Storage:", remoteVersion].join(" "));

    
    
    if (!meta || !meta.payload.storageVersion || !meta.payload.syncID ||
        STORAGE_VERSION > parseFloat(remoteVersion)) {

      
      let status = Records.response.status;
      if (status != 200 && status != 404) {
        this._checkServerError(Records.response);
        Status.sync = METARECORD_DOWNLOAD_FAIL;
        this._log.warn("Unknown error while downloading metadata record. " +
                       "Aborting sync.");
        return false;
      }

      if (!meta)
        this._log.info("No metadata record, server wipe needed");
      if (meta && !meta.payload.syncID)
        this._log.warn("No sync id, server wipe needed");

      if (!this.keyGenEnabled) {
        this._log.info("...and key generation is disabled.  Not wiping. " +
                       "Aborting sync.");
        Status.sync = DESKTOP_VERSION_OUT_OF_DATE;
        return false;
      }
      reset = true;
      this._log.info("Wiping server data");
      this._freshStart();

      if (status == 404)
        this._log.info("Metadata record not found, server wiped to ensure " +
                       "consistency.");
      else 
        this._log.info("Wiped server; incompatible metadata: " + remoteVersion);

    }
    else if (remoteVersion > STORAGE_VERSION) {
      Status.sync = VERSION_OUT_OF_DATE;
      this._log.warn("Upgrade required to access newer storage version.");
      return false;
    }
    else if (meta.payload.syncID != this.syncID) {
      this.resetClient();
      this.syncID = meta.payload.syncID;
      this._log.debug("Clear cached values and take syncId: " + this.syncID);

      
      
      Sync.sleep(15000);

      
      if (!this.verifyLogin()) {
        Status.sync = CREDENTIALS_CHANGED;
        this._log.info("Credentials have changed, aborting sync and forcing re-login.");
        return false;
      }
    }

    let needKeys = true;
    let pubkey = PubKeys.getDefaultKey();
    if (!pubkey)
      this._log.debug("Could not get public key");
    else if (pubkey.keyData == null)
      this._log.debug("Public key has no key data");
    else {
      
      let privkey = PrivKeys.get(pubkey.privateKeyUri);
      if (!privkey)
        this._log.debug("Could not get private key");
      else if (privkey.keyData == null)
        this._log.debug("Private key has no key data");
      else
        return true;
    }

    if (needKeys) {
      if (PubKeys.response.status != 404 && PrivKeys.response.status != 404) {
        this._log.warn("Couldn't download keys from server, aborting sync");
        this._log.debug("PubKey HTTP status: " + PubKeys.response.status);
        this._log.debug("PrivKey HTTP status: " + PrivKeys.response.status);
        this._checkServerError(PubKeys.response);
        this._checkServerError(PrivKeys.response);
        Status.sync = KEYS_DOWNLOAD_FAIL;
        return false;
      }

      if (!this.keyGenEnabled) {
        this._log.warn("Couldn't download keys from server, and key generation" +
                       "is disabled.  Aborting sync");
        Status.sync = NO_KEYS_NO_KEYGEN;
        return false;
      }

      if (!reset) {
        this._log.warn("Calling freshStart from !reset case.");
        this._freshStart();
        this._log.info("Server data wiped to ensure consistency due to missing keys");
      }

      let passphrase = ID.get("WeaveCryptoID");
      if (passphrase.password) {
        let keys = PubKeys.createKeypair(passphrase, PubKeys.defaultKeyUri,
                                         PrivKeys.defaultKeyUri);
        try {
          
          PubKeys.uploadKeypair(keys);
          PubKeys.set(keys.pubkey.uri, keys.pubkey);
          PrivKeys.set(keys.privkey.uri, keys.privkey);
          return true;
        } catch (e) {
          Status.sync = KEYS_UPLOAD_FAIL;
          this._log.error("Could not upload keys: " + Utils.exceptionStr(e));
        }
      } else {
        Status.sync = SETUP_FAILED_NO_PASSPHRASE;
        this._log.warn("Could not get encryption passphrase");
      }
    }

    return false;
  },

  







  _checkSync: function WeaveSvc__checkSync(ignore) {
    let reason = "";
    if (!this.enabled)
      reason = kSyncWeaveDisabled;
    else if (Svc.IO.offline)
      reason = kSyncNetworkOffline;
    else if (Svc.Private && Svc.Private.privateBrowsingEnabled)
      
      reason = kSyncInPrivateBrowsing;
    else if (Status.minimumNextSync > Date.now())
      reason = kSyncBackoffNotMet;
    else if (!this._loggedIn)
      reason = kSyncNotLoggedIn;
    else if (Svc.Prefs.get("firstSync") == "notReady")
      reason = kFirstSyncChoiceNotMade;

    if (ignore && ignore.indexOf(reason) != -1)
      return "";

    return reason;
  },

  


  _clearSyncTriggers: function _clearSyncTriggers() {
    
    if (this._syncTimer)
      this._syncTimer.clear();
    if (this._heartbeatTimer)
      this._heartbeatTimer.clear();

    
    try {
      Svc.Idle.removeIdleObserver(this, this._idleTime);
      this._idleTime = 0;
    }
    catch(ex) {}
  },

  


  _checkSyncStatus: function WeaveSvc__checkSyncStatus() {
    
    
    if (this._checkSync([kSyncBackoffNotMet])) {
      this._clearSyncTriggers();
      return;
    }

    if (this._needUpdatedKeys)
      this._updateKeysToUTF8Passphrase();

    
    let wait;
    if (this.globalScore > this.syncThreshold) {
      this._log.debug("Global Score threshold hit, triggering sync.");
      wait = 0;
    }
    this._scheduleNextSync(wait);
  },

  _updateKeysToUTF8Passphrase: function _updateKeysToUTF8Passphrase() {
    
    let pubkey = PubKeys.getDefaultKey();
    let privkey = PrivKeys.get(pubkey.privateKeyUri);

    this._log.debug("Rewrapping private key with UTF-8 encoded passphrase.");
    let oldPrivKeyData = privkey.payload.keyData;
    privkey.payload.keyData = Svc.Crypto.rewrapPrivateKey(
      oldPrivKeyData, this.passphrase,
      privkey.payload.salt, privkey.payload.iv, this.passphraseUTF8
    );
    let response = new Resource(privkey.uri).put(privkey);
    if (!response.success) {
      this._log("Uploading rewrapped private key failed!");
      this._needUpdatedKeys = false;
      return;
    }

    
    let oldHmacKey = Svc.KeyFactory.keyFromString(Ci.nsIKeyObject.HMAC,
                                                  this.passphrase);
    let enginesToWipe = [];

    for each (let engine in Engines.getAll()) {
      let meta = CryptoMetas.get(engine.cryptoMetaURL);
      if (!meta)
        continue;

      this._log.debug("Recomputing HMAC for key at " + engine.cryptoMetaURL
                      + " with UTF-8 encoded passphrase.");
      for each (key in meta.keyring) {
        if (key.hmac != Utils.sha256HMAC(key.wrapped, oldHmacKey)) {
          this._log.debug("Key SHA256 HMAC mismatch! Wiping server.");
          enginesToWipe.push(engine.name);
          meta = null;
          break;
        }
        key.hmac = Utils.sha256HMAC(key.wrapped, meta.hmacKey);
      }

      if (!meta)
        continue;

      response = new Resource(meta.uri).put(meta);
      if (!response.success) {
        this._log.debug("Key upload failed: " + response);
      }
    }

    if (enginesToWipe.length) {
      this._log.debug("Wiping engines " + enginesToWipe.join(", "));
      this.wipeRemote(enginesToWipe);
    }
    this._needUpdatedKeys = false;
  },

  




  syncOnIdle: function WeaveSvc_syncOnIdle(delay) {
    
    if (this._idleTime)
      return;

    this._idleTime = delay || IDLE_TIME;
    this._log.debug("Idle timer created for sync, will sync after " +
                    this._idleTime + " seconds of inactivity.");
    Svc.Idle.addIdleObserver(this, this._idleTime);
  },

  


  _scheduleNextSync: function WeaveSvc__scheduleNextSync(interval) {
    
    if (interval == null) {
      
      if (this.nextSync != 0)
        interval = this.nextSync - Date.now();
      
      else
        interval = Math.max(this.syncInterval, Status.backoffInterval);
    }

    
    if (interval <= 0) {
      this.syncOnIdle();
      return;
    }

    this._log.trace("Next sync in " + Math.ceil(interval / 1000) + " sec.");
    Utils.delay(function() this.syncOnIdle(), interval, this, "_syncTimer");

    
    this.nextSync = Date.now() + interval;

    
    if (this.numClients == 1)
      this._scheduleHeartbeat();
  },

  





  _doHeartbeat: function WeaveSvc__doHeartbeat() {
    if (this._heartbeatTimer)
      this._heartbeatTimer.clear();

    this.nextHeartbeat = 0;
    let info = null;
    try {
      info = new Resource(this.infoURL).get();
      if (info && info.success) {
        
        
        this._log.trace("Remote timestamp:" + info.obj["clients"] +
                        " Local timestamp: " + Clients.lastSync);
        if (info.obj["clients"] > Clients.lastSync) {
          this._log.debug("New clients detected, triggering a full sync");
          this.syncOnIdle();
          return;
        }
      }
      else {
        this._checkServerError(info);
        this._log.debug("Heartbeat failed. HTTP Error: " + info.status);
      }
    } catch(ex) {
      
      this._log.debug("Heartbeat failed unexpectedly: " + ex);
    }

    
    this._scheduleHeartbeat();
  },

  




  _scheduleHeartbeat: function WeaveSvc__scheduleNextHeartbeat() {
    if (this._heartbeatTimer)
      return;

    let now = Date.now();
    if (this.nextHeartbeat && this.nextHeartbeat < now) {
      this._doHeartbeat();
      return;
    }

    
    let interval = MULTI_DESKTOP_SYNC;
    if (this.nextSync < Date.now() + interval ||
        Status.enforceBackoff)
      return;

    if (this.nextHeartbeat)
      interval = this.nextHeartbeat - now;
    else
      this.nextHeartbeat = now + interval;

    this._log.trace("Setting up heartbeat, next ping in " +
                    Math.ceil(interval / 1000) + " sec.");
    Utils.delay(function() this._doHeartbeat(), interval, this, "_heartbeatTimer");
  },

  _syncErrors: 0,
  


  _handleSyncError: function WeaveSvc__handleSyncError() {
    this._syncErrors++;

    
    if (!Status.enforceBackoff) {
      if (this._syncErrors < 3) {
        this._scheduleNextSync();
        return;
      }
      Status.enforceBackoff = true;
    }

    const MINIMUM_BACKOFF_INTERVAL = 15 * 60 * 1000;     
    let interval = this._calculateBackoff(this._syncErrors, MINIMUM_BACKOFF_INTERVAL);

    this._scheduleNextSync(interval);

    let d = new Date(Date.now() + interval);
    this._log.config("Starting backoff, next sync at:" + d.toString());
  },

  


  sync: function sync()
    this._catch(this._lock(this._notify("sync", "", function() {

    let syncStartTime = Date.now();

    Status.resetSync();

    
    let reason = this._checkSync();
    if (reason) {
      
      
      reason = "Can't sync: " + reason;
      throw reason;
    }

    
    if (this.clusterURL == "" && !this._setCluster()) {
      Status.sync = NO_SYNC_NODE_FOUND;
      this._scheduleNextSync(10 * 60 * 1000);
      return;
    }

    
    this._clearSyncTriggers();
    this.nextSync = 0;
    this.nextHeartbeat = 0;

    
    
    Status.resetBackoff();

    this.globalScore = 0;

    
    let infoURL = this.infoURL;
    let now = Math.floor(Date.now() / 1000);
    let lastPing = Svc.Prefs.get("lastPing", 0);
    if (now - lastPing > 86400) { 
      infoURL += "?v=" + WEAVE_VERSION;
      Svc.Prefs.set("lastPing", now);
    }

    
    let info = new Resource(infoURL).get();
    if (!info.success)
      throw "aborting sync, failed to get collections";

    
    for each (let engine in [Clients].concat(Engines.getAll()))
      engine.lastModified = info.obj[engine.name] || 0;

    
    if (info.obj.crypto != this.cryptoModified) {
      this._log.debug("Clearing cached crypto records");
      CryptoMetas.clearCache();
      this.cryptoModified = info.obj.crypto;
    }

    
    if (info.obj.keys != this.keysModified) {
      this._log.debug("Clearing cached keys records");
      PubKeys.clearCache();
      PrivKeys.clearCache();
      this.keysModified = info.obj.keys;
    }

    if (!(this._remoteSetup()))
      throw "aborting sync, remote setup failed";

    
    this._log.trace("Refreshing client list");
    Clients.sync();

    
    switch (Svc.Prefs.get("firstSync")) {
      case "resetClient":
        this.resetClient(Engines.getEnabled().map(function(e) e.name));
        break;
      case "wipeClient":
        this.wipeClient(Engines.getEnabled().map(function(e) e.name));
        break;
      case "wipeRemote":
        this.wipeRemote(Engines.getEnabled().map(function(e) e.name));
        break;
    }

    
    if (Clients.localCommands) {
      try {
        if (!(this.processCommands())) {
          Status.sync = ABORT_SYNC_COMMAND;
          throw "aborting sync, process commands said so";
        }

        
        if (!(this._remoteSetup()))
          throw "aborting sync, remote setup failed after processing commands";
      }
      finally {
        
        Clients.sync();
      }
    }

    
    this._updateClientMode();

    try {
      for each (let engine in Engines.getEnabled()) {
        
        if (!(this._syncEngine(engine)) || Status.enforceBackoff) {
          this._log.info("Aborting sync");
          break;
        }
      }

      
      let meta = Records.get(this.metaURL);
      if (meta.changed)
        new Resource(meta.uri).put(meta);

      if (this._syncError)
        throw "Some engines did not sync correctly";
      else {
        Svc.Prefs.set("lastSync", new Date().toString());
        Status.sync = SYNC_SUCCEEDED;
        let syncTime = ((Date.now() - syncStartTime) / 1000).toFixed(2);
        this._log.info("Sync completed successfully in " + syncTime + " secs.");
      }
    } finally {
      this._syncError = false;
      Svc.Prefs.reset("firstSync");
    }
  })))(),

  


  _updateClientMode: function _updateClientMode() {
    
    let {numClients, hasMobile} = Clients.stats;
    if (this.numClients == numClients)
      return;

    this._log.debug("Client count: " + this.numClients + " -> " + numClients);
    this.numClients = numClients;

    if (numClients == 1) {
      this.syncInterval = SINGLE_USER_SYNC;
      this.syncThreshold = SINGLE_USER_THRESHOLD;
    }
    else {
      this.syncInterval = hasMobile ? MULTI_MOBILE_SYNC : MULTI_DESKTOP_SYNC;
      this.syncThreshold = hasMobile ? MULTI_MOBILE_THRESHOLD : MULTI_DESKTOP_THRESHOLD;
    }
  },

  
  
  _syncEngine: function WeaveSvc__syncEngine(engine) {
    try {
      engine.sync();
      return true;
    }
    catch(e) {
      
      if (e.status == 401 && this._updateCluster())
        return this._syncEngine(engine);

      this._checkServerError(e);

      Status.engines = [engine.name, e.failureCode || ENGINE_UNKNOWN_FAIL];

      this._syncError = true;
      this._log.debug(Utils.exceptionStr(e));
      return true;
    }
    finally {
      
      if (engine.toFetch != null && engine.toFetch.length > 0)
        Status.partial = true;
    }
  },

  _freshStart: function WeaveSvc__freshStart() {
    this.resetClient();

    let meta = new WBORecord(this.metaURL);
    meta.payload.syncID = this.syncID;
    meta.payload.storageVersion = STORAGE_VERSION;

    this._log.debug("New metadata record: " + JSON.stringify(meta.payload));
    let resp = new Resource(meta.uri).put(meta);
    if (!resp.success)
      throw resp;
    Records.set(meta.uri, meta);

    
    let collections = [Clients].concat(Engines.getAll()).map(function(engine) {
      return engine.name;
    });
    this.wipeServer(["crypto", "keys"].concat(collections));
  },

  



  _checkServerError: function WeaveSvc__checkServerError(resp) {
    if (Utils.checkStatus(resp.status, null, [500, [502, 504]])) {
      Status.enforceBackoff = true;
      if (resp.status == 503 && resp.headers["retry-after"])
        Observers.notify("weave:service:backoff:interval", parseInt(resp.headers["retry-after"], 10));
    }
  },
  




  _calculateBackoff: function WeaveSvc__calculateBackoff(attempts, base_interval) {
    const MAXIMUM_BACKOFF_INTERVAL = 8 * 60 * 60 * 1000; 
    let backoffInterval = attempts *
                          (Math.floor(Math.random() * base_interval) +
                           base_interval);
    return Math.max(Math.min(backoffInterval, MAXIMUM_BACKOFF_INTERVAL), Status.backoffInterval);
  },

  





  wipeServer: function WeaveSvc_wipeServer(collections)
    this._catch(this._notify("wipe-server", "", function() {
      if (!collections) {
        collections = [];
        let info = new Resource(this.infoURL).get();
        for (let name in info.obj)
          collections.push(name);
      }
      for each (let name in collections) {
        try {
          new Resource(this.storageURL + name).delete();

          
          let crypto = this.storageURL + "crypto/" + name;
          new Resource(crypto).delete();
          CryptoMetas.del(crypto);
        }
        catch(ex) {
          this._log.debug("Exception on wipe of '" + name + "': " + Utils.exceptionStr(ex));
        }
      }
    }))(),

  





  wipeClient: function WeaveSvc_wipeClient(engines)
    this._catch(this._notify("wipe-client", "", function() {
      
      if (!engines) {
        
        this.resetService();

        engines = [Clients].concat(Engines.getAll());
      }
      
      else
        engines = Engines.get(engines);

      
      for each (let engine in engines)
        if (engine._testDecrypt())
          engine.wipeClient();

      
      this.persistLogin();
    }))(),

  






  wipeRemote: function WeaveSvc_wipeRemote(engines)
    this._catch(this._notify("wipe-remote", "", function() {
      
      this.resetClient(engines);

      
      this.wipeServer(engines);

      
      if (engines)
        engines.forEach(function(e) this.prepCommand("wipeEngine", [e]), this);
      
      else
        this.prepCommand("wipeAll", []);

      
      Clients.sync();
    }))(),

  


  resetService: function WeaveSvc_resetService()
    this._catch(this._notify("reset-service", "", function() {
      
      this.clearLogs();
      this._log.info("Logs reinitialized for service reset");

      
      this.syncID = "";
      Svc.Prefs.reset("lastSync");
      for each (let cache in [PubKeys, PrivKeys, CryptoMetas, Records])
        cache.clearCache();
    }))(),

  





  resetClient: function WeaveSvc_resetClient(engines)
    this._catch(this._notify("reset-client", "", function() {
      
      if (!engines) {
        
        this.resetService();

        engines = [Clients].concat(Engines.getAll());
      }
      
      else
        engines = Engines.get(engines);

      
      for each (let engine in engines)
        engine.resetClient();
    }))(),

  




  _commands: [
    ["resetAll", 0, "Clear temporary local data for all engines"],
    ["resetEngine", 1, "Clear temporary local data for engine"],
    ["wipeAll", 0, "Delete all client data for all engines"],
    ["wipeEngine", 1, "Delete all client data for engine"],
    ["logout", 0, "Log out client"],
  ].reduce(function WeaveSvc__commands(commands, entry) {
    commands[entry[0]] = {};
    for (let [i, attr] in Iterator(["args", "desc"]))
      commands[entry[0]][attr] = entry[i + 1];
    return commands;
  }, {}),

  




  processCommands: function WeaveSvc_processCommands()
    this._notify("process-commands", "", function() {
      
      let commands = Clients.localCommands;
      Clients.clearCommands();

      
      for each ({command: command, args: args} in commands) {
        this._log.debug("Processing command: " + command + "(" + args + ")");

        let engines = [args[0]];
        switch (command) {
          case "resetAll":
            engines = null;
            
          case "resetEngine":
            this.resetClient(engines);
            break;
          case "wipeAll":
            engines = null;
            
          case "wipeEngine":
            this.wipeClient(engines);
            break;
          case "logout":
            this.logout();
            return false;
          default:
            this._log.debug("Received an unknown command: " + command);
            break;
        }
      }

      return true;
    })(),

  









  prepCommand: function WeaveSvc_prepCommand(command, args) {
    let commandData = this._commands[command];
    
    if (commandData == null) {
      this._log.error("Unknown command to send: " + command);
      return;
    }
    
    else if (args == null || args.length != commandData.args) {
      this._log.error("Expected " + commandData.args + " args for '" +
                      command + "', but got " + args);
      return;
    }

    
    this._log.debug("Sending clients: " + [command, args, commandData.desc]);
    Clients.sendCommand(command, args);
  },
};


Weave.Service.onStartup();
