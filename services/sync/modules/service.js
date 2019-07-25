





































const EXPORTED_SYMBOLS = ['Weave'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;












const SCHEDULED_SYNC_INTERVAL = 60 * 1000 * 5; 


const IDLE_TIME = 5; 




const INITIAL_THRESHOLD = 75;



const THRESHOLD_DECREMENT_STEP = 25;


const CLUSTER_BACKOFF = SCHEDULED_SYNC_INTERVAL;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/ext/Sync.js");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/faultTolerance.js");
Cu.import("resource://weave/auth.js");
Cu.import("resource://weave/resource.js");
Cu.import("resource://weave/base_records/wbo.js");
Cu.import("resource://weave/base_records/crypto.js");
Cu.import("resource://weave/base_records/keys.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/engines/clientData.js");


let Weave = {};
Cu.import("resource://weave/constants.js", Weave);
Cu.import("resource://weave/util.js", Weave);
Cu.import("resource://weave/faultTolerance.js", Weave);
Cu.import("resource://weave/auth.js", Weave);
Cu.import("resource://weave/resource.js", Weave);
Cu.import("resource://weave/base_records/keys.js", Weave);
Cu.import("resource://weave/notifications.js", Weave);
Cu.import("resource://weave/identity.js", Weave);
Cu.import("resource://weave/stores.js", Weave);
Cu.import("resource://weave/engines.js", Weave);

Cu.import("resource://weave/engines/bookmarks.js", Weave);
Cu.import("resource://weave/engines/clientData.js", Weave);
Cu.import("resource://weave/engines/cookies.js", Weave);
Cu.import("resource://weave/engines/extensions.js", Weave);
Cu.import("resource://weave/engines/forms.js", Weave);
Cu.import("resource://weave/engines/history.js", Weave);
Cu.import("resource://weave/engines/input.js", Weave);
Cu.import("resource://weave/engines/prefs.js", Weave);
Cu.import("resource://weave/engines/microformats.js", Weave);
Cu.import("resource://weave/engines/passwords.js", Weave);
Cu.import("resource://weave/engines/plugins.js", Weave);
Cu.import("resource://weave/engines/tabs.js", Weave);
Cu.import("resource://weave/engines/themes.js", Weave);

Utils.lazy(Weave, 'Service', WeaveSvc);





function StatusRecord() {
  this._init();
}
StatusRecord.prototype = {
  _init: function() {
    this.server = [];
    this.sync = null;
    this.engines = {};
  },

  addServerStatus: function(statusCode) {
    this.server.push(statusCode);
  },

  setSyncStatus: function(statusCode) {
    this.sync = statusCode;
  },

  setEngineStatus: function(engineName, statusCode) {
    this.engines[engineName] = statusCode;
  }
};








function WeaveSvc() {
  this._notify = Utils.notify("weave:service:");
}
WeaveSvc.prototype = {

  _lock: Utils.lock,
  _catch: Utils.catch,
  _isQuitting: false,
  _loggedIn: false,
  _syncInProgress: false,
  _keyGenEnabled: true,

  
  _weaveStatusCode: null,
  
  _detailedStatus: null,

  
  _keyPair: {},

  
  _syncTimer: null,

  get username() {
    return Svc.Prefs.get("username", "");
  },
  set username(value) {
    if (value)
      Svc.Prefs.set("username", value);
    else
      Svc.Prefs.reset("username");

    
    ID.get('WeaveID').username = value;
    ID.get('WeaveCryptoID').username = value;

    
    this._genKeyURLs();
  },

  get password() { return ID.get('WeaveID').password; },
  set password(value) {
    ID.get('WeaveID').setTempPassword(null);
    ID.get('WeaveID').password = value;
  },

  get passphrase() { return ID.get('WeaveCryptoID').password; },
  set passphrase(value) {
    ID.get('WeaveCryptoID').setTempPassword(null);
    ID.get('WeaveCryptoID').password = value;
  },

  
  set onGetPassword(value) {
    ID.get('WeaveID').onGetPassword = value;
  },
  set onGetPassphrase(value) {
    ID.get('WeaveCryptoID').onGetPassword = value;
  },

  get baseURL() {
    let url = Svc.Prefs.get("serverURL");
    if (!url)
      throw "No server URL set";
    if (url[url.length-1] != '/')
      url += '/';
    url += "0.3/";
    return url;
  },
  set baseURL(value) {
    Svc.Prefs.set("serverURL", value);
  },

  get clusterURL() {
    let url = Svc.Prefs.get("clusterURL");
    if (!url)
      return null;
    if (url[url.length-1] != '/')
      url += '/';
    url += "0.3/user/";
    return url;
  },
  set clusterURL(value) {
    Svc.Prefs.set("clusterURL", value);
    this._genKeyURLs();
  },

  get userPath() { return ID.get('WeaveID').username; },

  get isLoggedIn() { return this._loggedIn; },

  get isQuitting() { return this._isQuitting; },
  set isQuitting(value) { this._isQuitting = value; },

  get cancelRequested() { return Engines.cancelRequested; },
  set cancelRequested(value) { Engines.cancelRequested = value; },

  get keyGenEnabled() { return this._keyGenEnabled; },
  set keyGenEnabled(value) { this._keyGenEnabled = value; },

  get enabled() { return Svc.Prefs.get("enabled"); },
  set enabled(value) { Svc.Prefs.set("enabled", value); },

  get statusCode() { return this._weaveStatusCode; },
  get detailedStatus() { return this._detailedStatus; },

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

  _setSyncFailure: function WeavSvc__setSyncFailure(code) {
    this._weaveStatusCode = WEAVE_STATUS_FAILED;
    this._detailedStatus.setSyncStatus(code);
  },

  _genKeyURLs: function WeaveSvc__genKeyURLs() {
    let url = this.clusterURL + this.username;
    PubKeys.defaultKeyUri = url + "/keys/pubkey";
    PrivKeys.defaultKeyUri = url + "/keys/privkey";
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

  onWindowOpened: function WeaveSvc__onWindowOpened() {
  },

  
  
  
  onStartup: function WeaveSvc_onStartup() {
    this._initLogs();
    this._log.info("Weave " + WEAVE_VERSION + " initializing");
    this._registerEngines();
    this._detailedStatus = new StatusRecord();

    
    if (WEAVE_VERSION != Svc.Prefs.get("lastversion")) {
      this._log.info("Resetting client syncID from _onStartup.");
      Clients.resetSyncID();
    }

    let ua = Cc["@mozilla.org/network/protocol;1?name=http"].
      getService(Ci.nsIHttpProtocolHandler).userAgent;
    this._log.info(ua);

    if (!this._checkCrypto()) {
      this.enabled = false;
      this._log.error("Could not load the Weave crypto component. Disabling " +
                      "Weave, since it will not work correctly.");
    }

    Utils.prefs.addObserver("", this, false);
    Svc.Observer.addObserver(this, "network:offline-status-changed", true);
    Svc.Observer.addObserver(this, "private-browsing", true);
    Svc.Observer.addObserver(this, "quit-application", true);
    Svc.Observer.addObserver(this, "weave:service:sync:finish", true);
    Svc.Observer.addObserver(this, "weave:service:sync:error", true);
    FaultTolerance.Service; 

    if (!this.enabled)
      this._log.info("Weave Sync disabled");

    
    ID.set('WeaveID', new Identity('Mozilla Services Password', this.username));
    Auth.defaultAuthenticator = new BasicAuthenticator(ID.get('WeaveID'));

    ID.set('WeaveCryptoID',
           new Identity('Mozilla Services Encryption Passphrase', this.username));

    this._genKeyURLs();

    if (Svc.Prefs.get("autoconnect"))
      this._autoConnect();
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

    let brief = Svc.Directory.get("ProfD", Ci.nsIFile);
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
    this._briefApp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.briefLog")];
    root.addAppender(this._briefApp);
    this._debugApp = new Log4Moz.RotatingFileAppender(verbose, formatter);
    this._debugApp.level = Log4Moz.Level[Svc.Prefs.get("log.appender.debugLog")];
    root.addAppender(this._debugApp);
  },

  clearLogs: function WeaveSvc_clearLogs() {
    this._briefApp.clear();
    this._debugApp.clear();
  },

  


  _registerEngines: function WeaveSvc__registerEngines() {
    let engines = [];
    switch (Svc.AppInfo.ID) {
      case FENNEC_ID:
        engines = ["Bookmarks", "History", "Password", "Tab"];
        break;

      case FIREFOX_ID:
        engines = ["Bookmarks", "Cookie", "Extension", "Form", "History", 
          "Input", "MicroFormat", "Password", "Plugin", "Prefs", "Tab", 
          "Theme"];
        break;

      case SEAMONKEY_ID:
        engines = ["Form", "History", "Password"];
        break;

      case THUNDERBIRD_ID:
        engines = ["Cookie", "Password"];
        break;
    }

    
    Engines.register(engines.map(function(name) Weave[name + "Engine"]));
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  

  observe: function WeaveSvc__observe(subject, topic, data) {
    switch (topic) {
      case "nsPref:changed":
        switch (data) {
          case "enabled":
          case "schedule":
            
            this._checkSyncStatus();
            break;
        }
        break;
      case "network:offline-status-changed":
        
        this._log.debug("Network offline status change: " + data);
        this._checkSyncStatus();
        break;
      case "private-browsing":
        
        this._log.debug("Private browsing change: " + data);
        this._checkSyncStatus();
        break;
      case "quit-application":
        this._onQuitApplication();
        break;
      case "weave:service:sync:error":
        this._handleSyncError();
        break;
      case "weave:service:sync:finish":
        this._scheduleNextSync();
        this._serverErrors = 0;
        break;
      case "idle":
        this._log.trace("Idle time hit, trying to sync");
        Svc.Idle.removeIdleObserver(this, IDLE_TIME);
        this.sync(false);
        break;
    }
  },

  _onQuitApplication: function WeaveSvc__onQuitApplication() {
  },

  

  
  findCluster: function WeaveSvc_findCluster(username) {
    this._log.debug("Finding cluster for user " + username);

    let res = new Resource(this.baseURL + "api/register/chknode/" + username);
    try {
      res.get();

      switch (res.lastChannel.responseStatus) {
        case 404:
          this._log.debug("Using serverURL as data cluster (multi-cluster support disabled)");
          return Svc.Prefs.get("serverURL");
        case 200:
          return "https://" + res.data + "/";
        default:
          this._log.debug("Unexpected response code trying to find cluster: " + res.lastChannel.responseStatus);
          break;
      }
    } catch(ex) { }

    return false;
  },

  
  setCluster: function WeaveSvc_setCluster(username) {
    let cluster = this.findCluster(username);

    if (cluster) {
      if (cluster == this.clusterURL)
        return false;

      this._log.debug("Saving cluster setting");
      this.clusterURL = cluster;
      return true;
    }

    this._log.debug("Error setting cluster for user " + username);
    return false;
  },
  
  
  updateCluster: function WeaveSvc_updateCluster(username) {
    let cTime = Date.now();
    let lastUp = parseFloat(Svc.Prefs.get("lastClusterUpdate"));
    if (!lastUp || ((cTime - lastUp) >= CLUSTER_BACKOFF)) {
      if (this.setCluster(username)) {
        Svc.Prefs.set("lastClusterUpdate", cTime.toString());
        return true;
      }
    }
    return false;
  },
  
  verifyLogin: function WeaveSvc_verifyLogin(username, password, passphrase, isLogin)
    this._catch(this._notify("verify-login", "", function() {
      this._log.debug("Verifying login for user " + username);

      let url = this.findCluster(username);
      if (isLogin)
        this.clusterURL = url;

      if (url[url.length-1] != '/')
        url += '/';
      url += "0.3/user/";

      let res = new Resource(url + username);
      res.authenticator = {
        onRequest: function(headers) {
          headers['Authorization'] = 'Basic ' + btoa(username + ':' + password);
          return headers;
        }
      };
      
      
      try {
        res.get();
      } catch (e) {}
  
      try {
        switch (res.lastChannel.responseStatus) {
          case 200:
            if (passphrase && !this.verifyPassphrase(username, password, passphrase)) {
              this._setSyncFailure(LOGIN_FAILED_INVALID_PASSPHRASE);
              return false;
            }
            return true;
          case 401:
            if (this.updateCluster(username))
              return this.verifyLogin(username, password, passphrase, isLogin);

            this._setSyncFailure(LOGIN_FAILED_LOGIN_REJECTED);
            this._log.debug("verifyLogin failed: login failed")
            return false;
          default:
            throw "unexpected HTTP response: " + res.lastChannel.responseStatus;
        }
      } catch (e) {
        
        this._log.debug("verifyLogin failed: " + e)
        this._setSyncFailure(LOGIN_FAILED_NETWORK_ERROR);
        throw e;
      }
    }))(),

  verifyPassphrase: function WeaveSvc_verifyPassphrase(username, password, passphrase)
    this._catch(this._notify("verify-passphrase", "", function() {
      this._log.debug("Verifying passphrase");
      this.username = username;
      ID.get("WeaveID").setTempPassword(password);
        
      try {
        let pubkey = PubKeys.getDefaultKey();
        let privkey = PrivKeys.get(pubkey.privateKeyUri);
        return Svc.Crypto.verifyPassphrase(
          privkey.payload.keyData, passphrase,
          privkey.payload.salt, privkey.payload.iv
        );
      } catch (e) {
        
        return true;
      }
      return true;
    }))(),

  changePassphrase: function WeaveSvc_changePassphrase(newphrase)
    this._catch(this._notify("changepph", "", function() {
      let pubkey = PubKeys.getDefaultKey();
      let privkey = PrivKeys.get(pubkey.privateKeyUri);
      
      


      let newkey = Svc.Crypto.rewrapPrivateKey(privkey.payload.keyData,
          this.passphrase, privkey.payload.salt,
          privkey.payload.iv, newphrase);
      privkey.payload.keyData = newkey;
      
      new Resource(privkey.uri).put(privkey.serialize());
      this.passphrase = newphrase;
      
      return true;
    }))(),
  
  changePassword: function WeaveSvc_changePassword(newpass)
    this._catch(this._notify("changepwd", "", function() {
      function enc(x) encodeURIComponent(x);
      let message = "uid=" + enc(this.username) + "&password=" + 
        enc(this.password) + "&new=" + enc(newpass);
      let url = Svc.Prefs.get('tmpServerURL') + '0.3/api/register/chpwd';
      let res = new Weave.Resource(url);
      res.authenticator = new Weave.NoOpAuthenticator();
      res.setHeader("Content-Type", "application/x-www-form-urlencoded",
                    "Content-Length", message.length);

      let resp = res.post(message);
      if (res.lastChannel.responseStatus != 200) {
        this._log.info("Password change failed: " + resp);
        throw "Could not change password";
      }
      
      this.password = newpass;
      return true;
    }))(),
    
  resetPassphrase: function WeaveSvc_resetPassphrase(newphrase)
    this._catch(this._notify("resetpph", "", function() {
      
      this.prepCommand("logout", []);
      let clientsBackup = Clients._store.clients;
      
      
      this.wipeServer();
      
      
      Clients._store.clients = clientsBackup;
      let username = this.username;
      let password = this.password;
      this.logout();
      
      
      this.passphrase = newphrase;
      
      
      this.login(username, password, newphrase);
      this.sync(true);
      return true;
    }))(),
  
  _autoConnectAttempts: 0,
  _autoConnect: function WeaveSvc__attemptAutoConnect() {
    try {
      if (!this.username || !this.password || !this.passphrase)
        return;

      let failureReason;
      if (Svc.IO.offline)
        failureReason = "Application is offline";
      else if (this.login()) {
        this.syncOnIdle();
        return;
      }

      failureReason = this.detailedStatus.sync;
    }
    catch (ex) {
      failureReason = ex;
    }

    this._log.debug("Autoconnect failed: " + failureReason);

    let listener = new Utils.EventListener(Utils.bind2(this,
      function WeaveSvc__autoConnectCallback(timer) {
        this._autoConnectTimer = null;
        this._autoConnect();
      }));
    this._autoConnectTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    
    var interval = Math.floor(Math.random() * SCHEDULED_SYNC_INTERVAL + 
                              SCHEDULED_SYNC_INTERVAL * this._autoConnectAttempts);
    this._autoConnectAttempts++;
    this._autoConnectTimer.initWithCallback(listener, interval,
                                            Ci.nsITimer.TYPE_ONE_SHOT);
    this._log.debug("Scheduling next autoconnect attempt in " + 
                    interval / 1000 + " seconds.");
  },
  
  login: function WeaveSvc_login(username, password, passphrase)
    this._catch(this._lock(this._notify("login", "", function() {
      this._loggedIn = false;
      this._detailedStatus = new StatusRecord();
      if (Svc.IO.offline)
        throw "Application is offline, login should not be called";

      if (typeof(username) != "undefined")
        this.username = username;
      if (typeof(password) != "undefined")
        ID.get("WeaveID").setTempPassword(password);
      if (typeof(passphrase) != "undefined")
        ID.get("WeaveCryptoID").setTempPassword(passphrase);

      if (!this.username) {
        this._setSyncFailure(LOGIN_FAILED_NO_USERNAME);
        throw "No username set, login failed";
      }
      if (!this.password) {
        this._setSyncFailure(LOGIN_FAILED_NO_PASSWORD);
        throw "No password given or found in password manager";
      }
      this._log.debug("Logging in user " + this.username);

      if (!(this.verifyLogin(this.username, this.password,
        passphrase, true))) {
        
        throw "Login failed: " + this.detailedStatus.sync;
      }

      
      this._loggedIn = true;
      this._checkSyncStatus();
      if (this._autoConnectTimer) {
        this._autoConnectTimer.cancel();
        this._autoConnectTimer = null;
      }

      return true;
    })))(),

  logout: function WeaveSvc_logout() {
    this._log.info("Logging out");
    this._loggedIn = false;
    this._keyPair = {};
    ID.get('WeaveID').setTempPassword(null); 
    ID.get('WeaveCryptoID').setTempPassword(null); 

    
    this._checkSyncStatus();

    Svc.Observer.notifyObservers(null, "weave:service:logout:finish", "");
  },

  createAccount: function WeaveSvc_createAccount(username, password, email,
                                                 captchaChallenge, captchaResponse) {
    function enc(x) encodeURIComponent(x);
    let message = "uid=" + enc(username) + "&password=" + enc(password) +
      "&mail=" + enc(email) + "&recaptcha_challenge_field=" +
      enc(captchaChallenge) + "&recaptcha_response_field=" + enc(captchaResponse);

    let url = Svc.Prefs.get('tmpServerURL') + '0.3/api/register/new';
    let res = new Weave.Resource(url);
    res.authenticator = new Weave.NoOpAuthenticator();
    res.setHeader("Content-Type", "application/x-www-form-urlencoded",
                  "Content-Length", message.length);

    
    let resp;
    try {
      resp = res.post(message);
    }
    catch(ex) {
      this._log.trace("Create account error: " + ex);
    }

    if (res.lastChannel.responseStatus != 200 &&
        res.lastChannel.responseStatus != 201)
      this._log.info("Failed to create account. " +
                       "status: " + res.lastChannel.responseStatus + ", " +
                       "response: " + resp);
    else
      this._log.info("Account created: " + resp);

    return res.lastChannel.responseStatus;
  },

  
  
  _remoteSetup: function WeaveSvc__remoteSetup() {
    let reset = false;

    this._log.debug("Fetching global metadata record");
    let meta = Records.import(this.clusterURL + this.username + "/meta/global");

    let remoteVersion = (meta && meta.payload.storageVersion)?
      meta.payload.storageVersion : "";

    this._log.debug(["Weave Version:", WEAVE_VERSION, "Compatible:", 
      COMPATIBLE_VERSION, "Remote:", remoteVersion].join(" "));

    if (!meta || !meta.payload.storageVersion || !meta.payload.syncID ||
        Svc.Version.compare(COMPATIBLE_VERSION, remoteVersion) > 0) {

      
      let status = Records.lastResource.lastChannel.responseStatus;
      if (status != 200 && status != 404) {
        this._setSyncFailure(METARECORD_DOWNLOAD_FAIL);
        this._log.warn("Unknown error while downloading metadata record. " +
                       "Aborting sync.");
        return false;
      }

      if (!meta)
        this._log.info("No metadata record, server wipe needed");
      if (meta && !meta.payload.syncID)
        this._log.warn("No sync id, server wipe needed");
      if (Svc.Version.compare(COMPATIBLE_VERSION, remoteVersion) > 0)
        this._log.info("Server data is older than what Weave supports, server wipe needed");

      if (!this._keyGenEnabled) {
        this._log.info("...and key generation is disabled.  Not wiping. " +
                       "Aborting sync.");
        this._setSyncFailure(DESKTOP_VERSION_OUT_OF_DATE);
        return false;
      }
      reset = true;
      this._log.info("Wiping server data");
      this._freshStart();

      if (status == 404)
        this._log.info("Metadata record not found, server wiped to ensure " +
                       "consistency.");
      else 
        this._log.info("Server data wiped to ensure consistency after client " +
                       "upgrade (" + remoteVersion + " -> " + WEAVE_VERSION + ")");

    } else if (Svc.Version.compare(remoteVersion, WEAVE_VERSION) > 0) {
      this._setSyncFailure(VERSION_OUT_OF_DATE);
      this._log.warn("Server data is of a newer Weave version, this client " +
                     "needs to be upgraded.  Aborting sync.");
      return false;

    } else if (meta.payload.syncID != Clients.syncID) {
      this._log.warn("Meta.payload.syncID is " + meta.payload.syncID +
                     ", Clients.syncID is " + Clients.syncID);
      this.resetClient();
      this._log.info("Reset client because of syncID mismatch.");
      Clients.syncID = meta.payload.syncID;
      this._log.info("Reset the client after a server/client sync ID mismatch");
      this._updateRemoteVersion(meta);
    }
    
    else
      this._updateRemoteVersion(meta);

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
      if (PubKeys.lastResource != null && PrivKeys.lastResource != null &&
          PubKeys.lastResource.lastChannel.responseStatus != 404 &&
          PrivKeys.lastResource.lastChannel.responseStatus != 404) {
        this._log.warn("Couldn't download keys from server, aborting sync");
        this._log.debug("PubKey HTTP response status: " +
                        PubKeys.lastResource.lastChannel.responseStatus);
        this._log.debug("PrivKey HTTP response status: " +
                        PrivKeys.lastResource.lastChannel.responseStatus);
        this._setSyncFailure(KEYS_DOWNLOAD_FAIL);
        return false;
      }

      if (!this._keyGenEnabled) {
        this._log.warn("Couldn't download keys from server, and key generation" +
                       "is disabled.  Aborting sync");
        this._setSyncFailure(NO_KEYS_NO_KEYGEN);
        return false;
      }

      if (!reset) {
        this._log.warn("Calling freshStart from !reset case.");
        this._freshStart();
        this._log.info("Server data wiped to ensure consistency due to missing keys");
      }

      let passphrase = ID.get("WeaveCryptoID");
      if (passphrase.getPassword()) {
        let keys = PubKeys.createKeypair(passphrase, PubKeys.defaultKeyUri,
                                         PrivKeys.defaultKeyUri);
        try {
          
          PubKeys.uploadKeypair(keys);
          PubKeys.set(keys.pubkey.uri, keys.pubkey);
          PrivKeys.set(keys.privkey.uri, keys.privkey);
          return true;
        } catch (e) {
          this._setSyncFailure(KEYS_UPLOAD_FAIL);
          this._log.error("Could not upload keys: " + Utils.exceptionStr(e));
          
          
        }
      } else {
        this._setSyncFailure(SETUP_FAILED_NO_PASSPHRASE);
        this._log.warn("Could not get encryption passphrase");
      }
    }

    return false;
  },

  




  _syncThresh: {},

  




  _checkSync: function WeaveSvc__checkSync() {
    let reason = "";
    if (!this.enabled)
      reason = kSyncWeaveDisabled;
    else if (!this._loggedIn)
      reason = kSyncNotLoggedIn;
    else if (Svc.IO.offline)
      reason = kSyncNetworkOffline;
    else if (Svc.Private && Svc.Private.privateBrowsingEnabled)
      
      reason = kSyncInPrivateBrowsing;
    else if (Svc.Prefs.get("schedule", 0) != 1)
      reason = kSyncNotScheduled;

    return reason;
  },

  


  _checkSyncStatus: function WeaveSvc__checkSyncStatus() {
    
    if (this._checkSync()) {
      if (this._syncTimer) {
        this._syncTimer.cancel();
        this._syncTimer = null;
      }

      try {
        Svc.Idle.removeIdleObserver(this, IDLE_TIME);
      } catch(e) {} 

      return;
    }

    
    this._scheduleNextSync();
  },

  



  syncOnIdle: function WeaveSvc_syncOnIdle() {
    this._log.debug("Idle timer created for sync, will sync after " +
                    IDLE_TIME + " seconds of inactivity.");
    Svc.Idle.addIdleObserver(this, IDLE_TIME);
  },
  
  


  _scheduleNextSync: function WeaveSvc__scheduleNextSync(interval) {
    if (!interval)
      interval = SCHEDULED_SYNC_INTERVAL;

    
    if (this._syncTimer)
      this._syncTimer.cancel();
    else
      this._syncTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    
    let listener = new Utils.EventListener(Utils.bind2(this,
      function WeaveSvc__scheduleNextSyncCallback(timer) {
        this._syncTimer = null;
        this.syncOnIdle();
      }));
    this._syncTimer.initWithCallback(listener, interval,
                                     Ci.nsITimer.TYPE_ONE_SHOT);
    this._log.debug("Next sync call in: " + this._syncTimer.delay / 1000 + " seconds.")
  },

  _serverErrors: 0,
  


  _handleSyncError: function WeaveSvc__handleSyncError() {
    let shouldBackoff = false;
    
    let err = Weave.Service.detailedStatus.sync;
    
    switch (err) {
      case METARECORD_DOWNLOAD_FAIL:
      case KEYS_DOWNLOAD_FAIL:
      case KEYS_UPLOAD_FAIL:
        shouldBackoff = true;
    }

    
    
    
    if (!shouldBackoff) {
      try {
        shouldBackoff = Utils.checkStatus(Records.lastResource.lastChannel.responseStatus, null, [500,[502,504]]);
      }
      catch (e) {
        
        shouldBackoff = true;
      }
    }
    
    
    if (!shouldBackoff) {
      this._scheduleNextSync();
      return;      
    }

    
    this._serverErrors++;

    
    if (this._serverErrors < 2) {
      this._scheduleNextSync();
      return;
    }

    
    const MINIMUM_BACKOFF_INTERVAL = 15 * 60 * 1000;     
    const MAXIMUM_BACKOFF_INTERVAL = 8 * 60 * 60 * 1000; 
    let backoffInterval = this._serverErrors *
                          (Math.floor(Math.random() * MINIMUM_BACKOFF_INTERVAL) +
                           MINIMUM_BACKOFF_INTERVAL);
    backoffInterval = Math.min(backoffInterval, MAXIMUM_BACKOFF_INTERVAL);
    this._scheduleNextSync(backoffInterval);

    let d = new Date(Date.now() + backoffInterval);
    this._log.config("Starting backoff, next sync at:" + d.toString());
  },

  





  sync: function WeaveSvc_sync(fullSync)
    this._catch(this._lock(this._notify("sync", "", function() {

    fullSync = true; 

    
    let useThresh = !fullSync;

    
    
    
    let reason = this._checkSync();
    if (reason && (useThresh || reason != kSyncNotScheduled)) {
      
      
      this._detailedStatus.setSyncStatus(reason);
      reason = "Can't sync: " + reason;
      throw reason;
    }

    if (!(this._remoteSetup()))
      throw "aborting sync, remote setup failed";

    this._log.debug("Refreshing client list");
    Clients.sync();

    
    if (Clients.getClients()[Clients.clientID].commands) {
      try {
        if (!(this.processCommands())) {
          this._detailedStatus.setSyncStatus(ABORT_SYNC_COMMAND);
          throw "aborting sync, process commands said so";
        }

        
        if (!(this._remoteSetup()))
          throw "aborting sync, remote setup failed after processing commands";
      }
      finally {
        
        Clients.sync();
      }
    }

    try {
      for each (let engine in Engines.getAll()) {
        let name = engine.name;

        
        if (!engine.enabled)
          continue;

        
        let resetThresh = Utils.bind2(this, function WeaveSvc__resetThresh(cond)
          cond ? this._syncThresh[name] = INITIAL_THRESHOLD : undefined);

        
        resetThresh(!(name in this._syncThresh));

        
        if (useThresh) {
          let score = engine.score;
          let thresh = this._syncThresh[name];
          if (score >= thresh)
            this._log.debug("Syncing " + name + "; " +
                            "score " + score + " >= thresh " + thresh);
          else {
            this._log.debug("Not syncing " + name + "; " +
                            "score " + score + " < thresh " + thresh);

            
            this._syncThresh[name] = Math.max(thresh - THRESHOLD_DECREMENT_STEP, 1);

            
            continue;
          }
        }

        
        if (!(this._syncEngine(engine))) {
          this._log.info("Aborting sync");
          break;
        }

        
        
        
        resetThresh(useThresh);
      }

      if (this._syncError)
        this._log.warn("Some engines did not sync correctly");
      else {
        Svc.Prefs.set("lastSync", new Date().toString());
        this._weaveStatusCode = WEAVE_STATUS_OK;
        this._log.info("Sync completed successfully");
      }
    } finally {
      this.cancelRequested = false;
      this._syncError = false;
    }
  })))(),

  
  
  _syncEngine: function WeaveSvc__syncEngine(engine) {
    try {
      engine.sync();
      if (!this.cancelRequested)
        return true;
    }
    catch(e) {
      
      if (e.constructor.name == "RequestException" && e.status == 401) {
        if (this.updateCluster(this.username))
          return this._syncEngine(engine);
      }
      this._syncError = true;
      this._weaveStatusCode = WEAVE_STATUS_PARTIAL;
      this._detailedStatus.setEngineStatus(engine.name, e);
      if (FaultTolerance.Service.onException(e))
        return true;
    }
  },

  _freshStart: function WeaveSvc__freshStart() {
    this.resetClient();
    this._log.info("Reset client data from freshStart.");
    this._log.info("Client metadata wiped, deleting server data");
    this.wipeServer();

    
    Sync.sleep(2000);

    this._log.debug("Uploading new metadata record");
    meta = new WBORecord(this.clusterURL + this.username + "/meta/global");
    meta.payload.syncID = Clients.syncID;
    this._updateRemoteVersion(meta);
  },

  _updateRemoteVersion: function WeaveSvc__updateRemoteVersion(meta) {
    
    if (Svc.Version.compare(meta.payload.storageVersion, WEAVE_VERSION) >= 0)
      return;

    this._log.debug("Setting meta payload storage version to " + WEAVE_VERSION);
    meta.payload.storageVersion = WEAVE_VERSION;
    let res = new Resource(meta.uri);
    res.put(meta.serialize());
  },

  





  wipeServer: function WeaveSvc_wipeServer(engines)
    this._catch(this._notify("wipe-server", "", function() {
      
      let userURL = this.clusterURL + this.username + "/";
      let res = new Resource(userURL);
      res.get();

      
      let allCollections = JSON.parse(res.data);
      for each (let name in allCollections) {
        try {
          
          if (engines && engines.indexOf(name) == -1)
            continue;

          new Resource(userURL + name).delete();
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
        engine.wipeClient();
    }))(),

  






  wipeRemote: function WeaveSvc_wipeRemote(engines)
    this._catch(this._notify("wipe-remote", "", function() {
      
      

      
      if (engines) {
        engines.forEach(function(e) this.prepCommand("wipeEngine", [e]), this);
        return;
      }

      
      this.prepCommand("wipeAll", []);
    }))(),

  


  resetService: function WeaveSvc_resetService()
    this._catch(this._notify("reset-service", "", function() {
      
      this.clearLogs();
      this._log.info("Logs reinitialized for service reset");

      
      Clients.resetSyncID();
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

      
      try {
        let cruft = Svc.Directory.get("ProfD", Ci.nsIFile);
        cruft.QueryInterface(Ci.nsILocalFile);
        cruft.append("weave");
        cruft.append("snapshots");
        if (cruft.exists())
          cruft.remove(true);
      } catch (e) {
        this._log.debug("Could not remove old snapshots: " + Utils.exceptionStr(e));
      }
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
      let info = Clients.getInfo(Clients.clientID);
      let commands = info.commands;

      
      delete info.commands;
      Clients.setInfo(Clients.clientID, info);

      
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

    
    let action = {
      command: command,
      args: args,
    };
    let actionStr = command + "(" + args + ")";

    
    let jsonArgs = JSON.stringify(args);
    let notDupe = function(action) action.command != command ||
      JSON.stringify(action.args) != jsonArgs;

    this._log.info("Sending clients: " + actionStr + "; " + commandData.desc);

    
    for (let guid in Clients.getClients()) {
      
      if (guid == Clients.clientID)
        continue;

      let info = Clients.getInfo(guid);
      
      if (info.commands == null)
        info.commands = [action];
      
      else if (info.commands.every(notDupe))
        info.commands.push(action);
      
      else
        continue;

      Clients.setInfo(guid, info);
      this._log.trace("Client " + guid + " got a new action: " + actionStr);
    }
  },
};
