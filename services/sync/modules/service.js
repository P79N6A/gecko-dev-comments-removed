









































const EXPORTED_SYMBOLS = ["Service", "Weave"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


const CLUSTER_BACKOFF = 5 * 60 * 1000; 


const PBKDF2_KEY_BYTES = 16;

const CRYPTO_COLLECTION = "crypto";
const KEYS_WBO = "keys";

const LOG_DATE_FORMAT = "%Y-%m-%d %H:%M:%S";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/ext/Preferences.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/rest.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/main.js");

const STORAGE_INFO_TYPES = [INFO_COLLECTIONS,
                            INFO_COLLECTION_USAGE,
                            INFO_COLLECTION_COUNTS,
                            INFO_QUOTA];






function WeaveSvc() {
  this._notify = Utils.notify("weave:service:");
}
WeaveSvc.prototype = {

  _lock: Utils.lock,
  _locked: false,
  _loggedIn: false,

  get account() Svc.Prefs.get("account", this.username),
  set account(value) {
    if (value) {
      value = value.toLowerCase();
      Svc.Prefs.set("account", value);
    } else {
      Svc.Prefs.reset("account");
    }
    this.username = this._usernameFromAccount(value);
  },

  _usernameFromAccount: function _usernameFromAccount(value) {
    
    
    if (value && value.match(/[^A-Z0-9._-]/i))
      return Utils.sha1Base32(value.toLowerCase()).toLowerCase();
    return value;
  },

  get username() {
    return Svc.Prefs.get("username", "").toLowerCase();
  },
  set username(value) {
    if (value) {
      
      value = value.toLowerCase();
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

  get passphrase() ID.get("WeaveCryptoID").keyStr,
  set passphrase(value) ID.get("WeaveCryptoID").keyStr = value,

  get syncKeyBundle() ID.get("WeaveCryptoID"),

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
    return misc + MISC_API_VERSION + "/";
  },

  get userAPI() {
    
    let user = Svc.Prefs.get("userURL");
    if (user.indexOf(":") == -1)
      user = this.serverURL + user;
    return user + USER_API_VERSION + "/";
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

  
  
  
  _catch: function _catch(func) {
    function lockExceptions(ex) {
      if (Utils.isLockException(ex)) {
        
        this._log.info("Cannot start sync: already syncing?");
      }
    }

    return Utils.catch.call(this, func, lockExceptions);
  },

  _updateCachedURLs: function _updateCachedURLs() {
    
    if (this.clusterURL == "" || this.username == "")
      return;

    let storageAPI = this.clusterURL + SYNC_API_VERSION + "/";
    this.userBaseURL = storageAPI + this.username + "/";
    this._log.debug("Caching URLs under storage user base: " + this.userBaseURL);

    
    this.infoURL = this.userBaseURL + "info/collections";
    this.storageURL = this.userBaseURL + "storage/";
    this.metaURL = this.storageURL + "meta/global";
    this.cryptoKeysURL = this.storageURL + CRYPTO_COLLECTION + "/" + KEYS_WBO;
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

  




















  lastHMACEvent: 0,

  


  handleHMACEvent: function handleHMACEvent() {
    let now = Date.now();

    
    
    if ((now - this.lastHMACEvent) < HMAC_EVENT_INTERVAL)
      return false;

    this._log.info("Bad HMAC event detected. Attempting recovery " +
                   "or signaling to other clients.");

    
    this.lastHMACEvent = now;

    
    let cryptoKeys = new CryptoWrapper(CRYPTO_COLLECTION, KEYS_WBO);
    try {
      let cryptoResp = cryptoKeys.fetch(this.cryptoKeysURL).response;

      
      
      let cipherText = cryptoKeys.ciphertext;

      if (!cryptoResp.success) {
        this._log.warn("Failed to download keys.");
        return false;
      }

      let keysChanged = this.handleFetchedKeys(this.syncKeyBundle,
                                               cryptoKeys, true);
      if (keysChanged) {
        
        this._log.info("Suggesting retry.");
        return true;              
      }

      
      cryptoKeys.ciphertext = cipherText;
      cryptoKeys.cleartext  = null;

      let uploadResp = cryptoKeys.upload(this.cryptoKeysURL);
      if (uploadResp.success)
        this._log.info("Successfully re-uploaded keys. Continuing sync.");
      else
        this._log.warn("Got error response re-uploading keys. " +
                       "Continuing sync; let's try again later.");

      return false;            

    } catch (ex) {
      this._log.warn("Got exception \"" + ex + "\" fetching and handling " +
                     "crypto keys. Will try again later.");
      return false;
    }
  },

  handleFetchedKeys: function handleFetchedKeys(syncKey, cryptoKeys, skipReset) {
    
    
    
    let wasBlank = CollectionKeys.isClear;
    let keysChanged = CollectionKeys.updateContents(syncKey, cryptoKeys);

    if (keysChanged && !wasBlank) {
      this._log.debug("Keys changed: " + JSON.stringify(keysChanged));

      if (!skipReset) {
        this._log.info("Resetting client to reflect key change.");

        if (keysChanged.length) {
          
          this.resetClient(keysChanged);
        }
        else {
          
          this.resetClient();
        }

        this._log.info("Downloaded new keys, client reset. Proceeding.");
      }
      return true;
    }
    return false;
  },

  


  onStartup: function onStartup() {
    this._migratePrefs();
    ErrorHandler.init();

    this._log = Log4Moz.repository.getLogger("Sync.Service");
    this._log.level =
      Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];

    this._log.info("Loading Weave " + WEAVE_VERSION);

    this.enabled = true;

    this._registerEngines();

    let ua = Cc["@mozilla.org/network/protocol;1?name=http"].
      getService(Ci.nsIHttpProtocolHandler).userAgent;
    this._log.info(ua);

    if (!this._checkCrypto()) {
      this.enabled = false;
      this._log.info("Could not load the Weave crypto component. Disabling " +
                      "Weave, since it will not work correctly.");
    }

    Svc.Obs.add("weave:service:setup-complete", this);
    Svc.Prefs.observe("engine.", this);

    SyncScheduler.init();

    if (!this.enabled)
      this._log.info("Weave Sync disabled");

    
    let id = ID.get("WeaveID");
    if (!id)
      id = ID.set("WeaveID", new Identity(PWDMGR_PASSWORD_REALM, this.username));
    Auth.defaultAuthenticator = new BasicAuthenticator(id);

    if (!ID.get("WeaveCryptoID"))
      ID.set("WeaveCryptoID",
             new SyncKeyBundle(PWDMGR_PASSPHRASE_REALM, this.username));

    this._updateCachedURLs();

    let status = this._checkSetup();
    if (status != STATUS_DISABLED && status != CLIENT_NOT_CONFIGURED)
      Svc.Obs.notify("weave:engine:start-tracking");

    
    
    
    Utils.nextTick(function() {
      Status.ready = true;
      Svc.Obs.notify("weave:service:ready");
    });
  },

  _checkSetup: function WeaveSvc__checkSetup() {
    if (!this.enabled)
      return Status.service = STATUS_DISABLED;
    return Status.checkSetup();
  },

  _migratePrefs: function _migratePrefs() {
    
    let logLevel = Svc.Prefs.get("log.appender.debugLog");
    if (logLevel) {
      Svc.Prefs.set("log.appender.file.level", logLevel);
      Svc.Prefs.reset("log.appender.debugLog");
    }
    if (Svc.Prefs.get("log.appender.debugLog.enabled")) {
      Svc.Prefs.set("log.appender.file.logOnSuccess", true);
      Svc.Prefs.reset("log.appender.debugLog.enabled");
    }

    
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
      case "weave:service:setup-complete":
        let status = this._checkSetup();
        if (status != STATUS_DISABLED && status != CLIENT_NOT_CONFIGURED)
            Svc.Obs.notify("weave:engine:start-tracking");
        break;
      case "nsPref:changed":
        if (this._ignorePrefObserver)
          return;
        let engine = data.slice((PREFS_BRANCH + "engine.").length);
        this._handleEngineStatusChanged(engine);
        break;
    }
  },

  _handleEngineStatusChanged: function handleEngineDisabled(engine) {
    this._log.trace("Status for " + engine + " engine changed.");
    if (Svc.Prefs.get("engineStatusChanged." + engine, false)) {
      
      Svc.Prefs.reset("engineStatusChanged." + engine);
    } else {
      
      Svc.Prefs.set("engineStatusChanged." + engine, true);
    }
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
          fail = "Find cluster denied: " + ErrorHandler.errorStr(node);
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
          ErrorHandler.checkServerError(node);
          fail = "Unexpected response code: " + node.status;
          break;
      }
    } catch (e) {
      this._log.debug("Network error on findCluster");
      Status.login = LOGIN_FAILED_NETWORK_ERROR;
      ErrorHandler.checkServerError(e);
      fail = e;
    }
    throw fail;
  },

  
  _setCluster: function _setCluster() {
    
    let cluster = this._findCluster();
    this._log.debug("Cluster value = " + cluster);
    if (cluster == null)
      return false;

    
    if (cluster == this.clusterURL)
      return false;

    this._log.debug("Setting cluster to " + cluster);
    this.clusterURL = cluster;
    Svc.Prefs.set("lastClusterUpdate", Date.now().toString());
    return true;
  },

  
  
  _updateCluster: function _updateCluster() {
    this._log.info("Updating cluster.");
    let cTime = Date.now();
    let lastUp = parseFloat(Svc.Prefs.get("lastClusterUpdate"));
    if (!lastUp || ((cTime - lastUp) >= CLUSTER_BACKOFF)) {
      return this._setCluster();
    }
    return false;
  },

  


  _fetchInfo: function _fetchInfo(url) {
    let infoURL = url || this.infoURL;

    this._log.trace("In _fetchInfo: " + infoURL);
    let info;
    try {
      info = new Resource(infoURL).get();
    } catch (ex) {
      ErrorHandler.checkServerError(ex);
      throw ex;
    }
    if (!info.success) {
      ErrorHandler.checkServerError(info);
      throw "aborting sync, failed to get collections";
    }
    return info;
  },

  verifyAndFetchSymmetricKeys: function verifyAndFetchSymmetricKeys(infoResponse) {

    this._log.debug("Fetching and verifying -- or generating -- symmetric keys.");

    
    
    

    let syncKey = this.syncKeyBundle;
    if (!syncKey) {
      this._log.error("No sync key: cannot fetch symmetric keys.");
      Status.login = LOGIN_FAILED_NO_PASSPHRASE;
      Status.sync = CREDENTIALS_CHANGED;             
      return false;
    }

    
    if (!Utils.isPassphrase(syncKey.keyStr)) {
      this._log.warn("Sync key input is invalid: cannot fetch symmetric keys.");
      Status.login = LOGIN_FAILED_INVALID_PASSPHRASE;
      Status.sync = CREDENTIALS_CHANGED;
      return false;
    }

    try {
      if (!infoResponse)
        infoResponse = this._fetchInfo();    

      
      if (infoResponse.status != 200) {
        this._log.warn("info/collections returned non-200 response. Failing key fetch.");
        Status.login = LOGIN_FAILED_SERVER_ERROR;
        ErrorHandler.checkServerError(infoResponse);
        return false;
      }

      let infoCollections = infoResponse.obj;

      this._log.info("Testing info/collections: " + JSON.stringify(infoCollections));

      if (CollectionKeys.updateNeeded(infoCollections)) {
        this._log.info("CollectionKeys reports that a key update is needed.");

        

        
        let cryptoKeys;

        if (infoCollections && (CRYPTO_COLLECTION in infoCollections)) {
          try {
            cryptoKeys = new CryptoWrapper(CRYPTO_COLLECTION, KEYS_WBO);
            let cryptoResp = cryptoKeys.fetch(this.cryptoKeysURL).response;

            if (cryptoResp.success) {
              let keysChanged = this.handleFetchedKeys(syncKey, cryptoKeys);
              return true;
            }
            else if (cryptoResp.status == 404) {
              
              
              this._log.warn("Got 404 for crypto/keys, but 'crypto' in info/collections. Regenerating.");
              cryptoKeys = null;
            }
            else {
              
              Status.login = LOGIN_FAILED_SERVER_ERROR;
              ErrorHandler.checkServerError(cryptoResp);
              this._log.warn("Got status " + cryptoResp.status + " fetching crypto keys.");
              return false;
            }
          }
          catch (ex) {
            this._log.warn("Got exception \"" + ex + "\" fetching cryptoKeys.");
            

            
            if (Utils.isHMACMismatch(ex)) {
              Status.login = LOGIN_FAILED_INVALID_PASSPHRASE;
              Status.sync = CREDENTIALS_CHANGED;
            }
            else {
              
              
              Status.login = LOGIN_FAILED;
            }
            return false;
          }
        }
        else {
          this._log.info("... 'crypto' is not a reported collection. Generating new keys.");
        }

        if (!cryptoKeys) {
          this._log.info("No keys! Generating new ones.");

          
          
          
          
          this._freshStart();
          return true;
        }

        
        return false;
      }
      else {
        
        return true;
      }

    } catch (e) {
      
      return false;
    }
  },

  verifyLogin: function verifyLogin()
    this._notify("verify-login", "", function() {
      if (!this.username) {
        this._log.warn("No username in verifyLogin.");
        Status.login = LOGIN_FAILED_NO_USERNAME;
        return false;
      }

      
      
      
      
      
      try {
        this.passphrase;
      } catch (ex) {
        this._log.debug("Fetching passphrase threw " + ex +
                        "; assuming master password locked.");
        Status.login = MASTER_PASSWORD_LOCKED;
        return false;
      }

      try {
        
        
        
        if (this.clusterURL == "" && !this._setCluster()) {
          Status.sync = NO_SYNC_NODE_FOUND;
          Svc.Obs.notify("weave:service:sync:delayed");
          return true;
        }

        
        let test = new Resource(this.infoURL).get();

        switch (test.status) {
          case 200:
            

            
            
            
            if (!this.passphrase) {
              this._log.warn("No passphrase in verifyLogin.");
              Status.login = LOGIN_FAILED_NO_PASSPHRASE;
              return false;
            }

            
            
            if (this._remoteSetup()) {
              
              Status.login = LOGIN_SUCCEEDED;
              return true;
            }

            this._log.warn("Remote setup failed.");
            
            return false;

          case 401:
            this._log.warn("401: login failed.");
            
            
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
            
            Status.login = LOGIN_FAILED_SERVER_ERROR;
            ErrorHandler.checkServerError(test);
            return false;
        }
      }
      catch (ex) {
        
        this._log.debug("verifyLogin failed: " + Utils.exceptionStr(ex));
        Status.login = LOGIN_FAILED_NETWORK_ERROR;
        ErrorHandler.checkServerError(ex);
        return false;
      }
    })(),

  generateNewSymmetricKeys:
  function WeaveSvc_generateNewSymmetricKeys() {
    this._log.info("Generating new keys WBO...");
    let wbo = CollectionKeys.generateNewKeysWBO();
    this._log.info("Encrypting new key bundle.");
    wbo.encrypt(this.syncKeyBundle);

    this._log.info("Uploading...");
    let uploadRes = wbo.upload(this.cryptoKeysURL);
    if (uploadRes.status != 200) {
      this._log.warn("Got status " + uploadRes.status + " uploading new keys. What to do? Throw!");
      throw new Error("Unable to upload symmetric keys.");
    }
    this._log.info("Got status " + uploadRes.status + " uploading keys.");
    let serverModified = uploadRes.obj;   
    this._log.debug("Server reports crypto modified: " + serverModified);

    
    this._log.debug("Verifying server collection records.");
    let info = this._fetchInfo();
    this._log.debug("info/collections is: " + info);

    if (info.status != 200) {
      this._log.warn("Non-200 info/collections response. Aborting.");
      throw new Error("Unable to upload symmetric keys.");
    }

    info = info.obj;
    if (!(CRYPTO_COLLECTION in info)) {
      this._log.error("Consistency failure: info/collections excludes " + 
                      "crypto after successful upload.");
      throw new Error("Symmetric key upload failed.");
    }

    
    if (info[CRYPTO_COLLECTION] < serverModified) {
      this._log.error("Consistency failure: info/collections crypto entry " + 
                      "is stale after successful upload.");
      throw new Error("Symmetric key upload failed.");
    }
    
    
    
    
    let cryptoKeys = new CryptoWrapper(CRYPTO_COLLECTION, KEYS_WBO);
    let cryptoResp = cryptoKeys.fetch(this.cryptoKeysURL).response;
    if (cryptoResp.status != 200) {
      this._log.warn("Failed to download keys.");
      throw new Error("Symmetric key download failed.");
    }
    let keysChanged = this.handleFetchedKeys(this.syncKeyBundle,
                                             cryptoKeys, true);
    if (keysChanged) {
      this._log.info("Downloaded keys differed, as expected.");
    }
  },

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

      this.logout();

      
      this.passphrase = newphrase;
      this.persistLogin();

      
      this.resetClient();
      CollectionKeys.clear();

      
      this.sync();
      return true;
    }))(),

  startOver: function() {
    Svc.Obs.notify("weave:engine:stop-tracking");

    
    
    
    Service.passphrase = "";
    Status.login = LOGIN_FAILED_NO_PASSPHRASE;
    this.logout();
    Svc.Obs.notify("weave:service:start-over");

    
    if (this.clusterURL != "") {
      
      for each (let engine in [Clients].concat(Engines.getAll())) {
        try {
          engine.removeClientData();
        } catch(ex) {
          this._log.warn("Deleting client data for " + engine.name + " failed:"
                         + Utils.exceptionStr(ex));
        }
      }
    } else {
      this._log.debug("Skipping client data removal: no cluster URL.");
    }

    
    this.resetClient();
    CollectionKeys.clear();
    Status.resetBackoff();
    Status.resetSync();

    
    this._ignorePrefObserver = true;
    Svc.Prefs.resetBranch("");
    this._ignorePrefObserver = false;

    Svc.Prefs.set("lastversion", WEAVE_VERSION);
    
    this.password = "";
    this.passphrase = "";
    Services.logins.findLogins({}, PWDMGR_HOST, "", "").map(function(login) {
      Services.logins.removeLogin(login);
    });
  },

  persistLogin: function persistLogin() {
    
    try {
      ID.get("WeaveID").persist();
      ID.get("WeaveCryptoID").persist();
    }
    catch(ex) {}
  },

  login: function WeaveSvc_login(username, password, passphrase)
    this._catch(this._lock("service.js: login",
          this._notify("login", "", function() {
      this._loggedIn = false;
      if (Services.io.offline) {
        Status.login = LOGIN_FAILED_NETWORK_ERROR;
        throw "Application is offline, login should not be called";
      }

      let initialStatus = this._checkSetup();
      if (username)
        this.username = username;
      if (password)
        this.password = password;
      if (passphrase)
        this.passphrase = passphrase;

      if (this._checkSetup() == CLIENT_NOT_CONFIGURED)
        throw "aborting login, client not configured";

      
      
      if (initialStatus == CLIENT_NOT_CONFIGURED
          && (username || password || passphrase))
        Svc.Obs.notify("weave:service:setup-complete");

      this._log.info("Logging in user " + this.username);

      if (!this.verifyLogin()) {
        
        throw "Login failed: " + Status.login;
      }

      this._loggedIn = true;

      return true;
    })))(),

  logout: function WeaveSvc_logout() {
    
    if (!this._loggedIn)
      return;

    this._log.info("Logging out");
    this._loggedIn = false;

    Svc.Obs.notify("weave:service:logout:finish");
  },

  checkAccount: function checkAccount(account) {
    let username = this._usernameFromAccount(account);
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

    
    return ErrorHandler.errorStr(data);
  },

  createAccount: function createAccount(email, password,
                                        captchaChallenge, captchaResponse) {
    let username = this._usernameFromAccount(email);
    let payload = JSON.stringify({
      "password": Utils.encodeUTF8(password),
      "email": email,
      "captcha-challenge": captchaChallenge,
      "captcha-response": captchaResponse
    });

    let url = this.userAPI + username;
    let res = new Resource(url);
    res.authenticator = new NoOpAuthenticator();

    
    
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
        error = ErrorHandler.errorStr(register);
    }
    catch(ex) {
      this._log.warn("Failed to create account: " + ex);
    }

    return error;
  },

  
  
  _remoteSetup: function WeaveSvc__remoteSetup(infoResponse) {
    let reset = false;

    this._log.debug("Fetching global metadata record");
    let meta = Records.get(this.metaURL);

    
    if (infoResponse &&
        (infoResponse.obj.meta != this.metaModified) &&
        (!meta || !meta.isNew)) {

      
      this._log.debug("Clearing cached meta record. metaModified is " +
          JSON.stringify(this.metaModified) + ", setting to " +
          JSON.stringify(infoResponse.obj.meta));

      Records.del(this.metaURL);

      
      let newMeta       = Records.get(this.metaURL);

      if (!Records.response.success || !newMeta) {
        this._log.debug("No meta/global record on the server. Creating one.");
        newMeta = new WBORecord("meta", "global");
        newMeta.payload.syncID = this.syncID;
        newMeta.payload.storageVersion = STORAGE_VERSION;

        newMeta.isNew = true;

        Records.set(this.metaURL, newMeta);
        if (!newMeta.upload(this.metaURL).success) {
          this._log.warn("Unable to upload new meta/global. Failing remote setup.");
          return false;
        }
      } else {
        
        newMeta.isNew   = meta.isNew;
        newMeta.changed = meta.changed;
      }

      
      meta              = newMeta;
      this.metaModified = infoResponse.obj.meta;
    }

    let remoteVersion = (meta && meta.payload.storageVersion)?
      meta.payload.storageVersion : "";

    this._log.debug(["Weave Version:", WEAVE_VERSION, "Local Storage:",
      STORAGE_VERSION, "Remote Storage:", remoteVersion].join(" "));

    
    
    if (!meta || !meta.payload.storageVersion || !meta.payload.syncID ||
        STORAGE_VERSION > parseFloat(remoteVersion)) {

      this._log.info("One of: no meta, no meta storageVersion, or no meta syncID. Fresh start needed.");

      
      let status = Records.response.status;
      if (status != 200 && status != 404) {
        Status.sync = METARECORD_DOWNLOAD_FAIL;
        ErrorHandler.checkServerError(Records.response);
        this._log.warn("Unknown error while downloading metadata record. " +
                       "Aborting sync.");
        return false;
      }

      if (!meta)
        this._log.info("No metadata record, server wipe needed");
      if (meta && !meta.payload.syncID)
        this._log.warn("No sync id, server wipe needed");

      reset = true;

      this._log.info("Wiping server data");
      this._freshStart();

      if (status == 404)
        this._log.info("Metadata record not found, server was wiped to ensure " +
                       "consistency.");
      else 
        this._log.info("Wiped server; incompatible metadata: " + remoteVersion);

      return true;
    }
    else if (remoteVersion > STORAGE_VERSION) {
      Status.sync = VERSION_OUT_OF_DATE;
      this._log.warn("Upgrade required to access newer storage version.");
      return false;
    }
    else if (meta.payload.syncID != this.syncID) {

      this._log.info("Sync IDs differ. Local is " + this.syncID + ", remote is " + meta.payload.syncID);
      this.resetClient();
      CollectionKeys.clear();
      this.syncID = meta.payload.syncID;
      this._log.debug("Clear cached values and take syncId: " + this.syncID);

      if (!this.upgradeSyncKey(meta.payload.syncID)) {
        this._log.warn("Failed to upgrade sync key. Failing remote setup.");
        return false;
      }

      if (!this.verifyAndFetchSymmetricKeys(infoResponse)) {
        this._log.warn("Failed to fetch symmetric keys. Failing remote setup.");
        return false;
      }

      
      if (!this.verifyLogin()) {
        Status.sync = CREDENTIALS_CHANGED;
        this._log.info("Credentials have changed, aborting sync and forcing re-login.");
        return false;
      }

      return true;
    }
    else {
      if (!this.upgradeSyncKey(meta.payload.syncID)) {
        this._log.warn("Failed to upgrade sync key. Failing remote setup.");
        return false;
      }

      if (!this.verifyAndFetchSymmetricKeys(infoResponse)) {
        this._log.warn("Failed to fetch symmetric keys. Failing remote setup.");
        return false;
      }

      return true;
    }
  },

  






  _shouldLogin: function _shouldLogin() {
    return this.enabled &&
           !Services.io.offline &&
           !this.isLoggedIn;
  },

  







  _checkSync: function WeaveSvc__checkSync(ignore) {
    let reason = "";
    if (!this.enabled)
      reason = kSyncWeaveDisabled;
    else if (Services.io.offline)
      reason = kSyncNetworkOffline;
    else if (Status.minimumNextSync > Date.now())
      reason = kSyncBackoffNotMet;
    else if ((Status.login == MASTER_PASSWORD_LOCKED) &&
             Utils.mpLocked())
      reason = kSyncMasterPasswordLocked;
    else if (Svc.Prefs.get("firstSync") == "notReady")
      reason = kFirstSyncChoiceNotMade;

    if (ignore && ignore.indexOf(reason) != -1)
      return "";

    return reason;
  },

  sync: function sync() {
    let dateStr = new Date().toLocaleFormat(LOG_DATE_FORMAT);
    this._log.debug("User-Agent: " + SyncStorageRequest.prototype.userAgent);
    this._log.info("Starting sync at " + dateStr);
    this._catch(function () {
      
      if (this._shouldLogin()) {
        this._log.debug("In sync: should login.");
        if (!this.login()) {
          this._log.debug("Not syncing: login returned false.");
          return;
        }
      }
      else {
        this._log.trace("In sync: no need to login.");
      }
      return this._lockedSync.apply(this, arguments);
    })();
  },

  


  _lockedSync: function _lockedSync()
    this._lock("service.js: sync",
               this._notify("sync", "", function() {

    this._log.info("In sync().");

    let syncStartTime = Date.now();

    Status.resetSync();

    
    let reason = this._checkSync();
    if (reason) {
      if (reason == kSyncNetworkOffline) {
        Status.sync = LOGIN_FAILED_NETWORK_ERROR;
      }
      
      
      reason = "Can't sync: " + reason;
      throw reason;
    }

    
    if (this.clusterURL == "" && !this._setCluster()) {
      Status.sync = NO_SYNC_NODE_FOUND;
      return;
    }

    
    let infoURL = this.infoURL;
    let now = Math.floor(Date.now() / 1000);
    let lastPing = Svc.Prefs.get("lastPing", 0);
    if (now - lastPing > 86400) { 
      infoURL += "?v=" + WEAVE_VERSION;
      Svc.Prefs.set("lastPing", now);
    }

    
    let info = this._fetchInfo(infoURL);

    
    for each (let engine in [Clients].concat(Engines.getAll()))
      engine.lastModified = info.obj[engine.name] || 0;

    if (!(this._remoteSetup(info)))
      throw "aborting sync, remote setup failed";

    
    this._log.debug("Refreshing client list.");
    if (!this._syncEngine(Clients)) {
      
      
      this._log.warn("Client engine sync failed. Aborting.");
      return;
    }

    
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
        if (!(Clients.processIncomingCommands())) {
          Status.sync = ABORT_SYNC_COMMAND;
          throw "aborting sync, process commands said so";
        }

        
        if (!(this._remoteSetup(info)))
          throw "aborting sync, remote setup failed after processing commands";
      }
      finally {
        
        
        
        
        this._syncEngine(Clients);
      }
    }

    
    this._updateEnabledEngines();

    try {
      for each (let engine in Engines.getEnabled()) {
        
        if (!(this._syncEngine(engine)) || Status.enforceBackoff) {
          this._log.info("Aborting sync");
          break;
        }
      }

      
      
      
      if (!this.clusterURL) {
        this._log.debug("Aborting sync, no cluster URL: " +
                        "not uploading new meta/global.");
        return;
      }

      
      let meta = Records.get(this.metaURL);
      if (meta.isNew || meta.changed) {
        new Resource(this.metaURL).put(meta);
        delete meta.isNew;
        delete meta.changed;
      }

      
      if (Status.service != SYNC_FAILED_PARTIAL) {
        Svc.Prefs.set("lastSync", new Date().toString());
        Status.sync = SYNC_SUCCEEDED;
      }
    } finally {
      Svc.Prefs.reset("firstSync");

      let syncTime = ((Date.now() - syncStartTime) / 1000).toFixed(2);
      let dateStr = new Date().toLocaleFormat(LOG_DATE_FORMAT);
      this._log.info("Sync completed at " + dateStr
                     + " after " + syncTime + " secs.");
    }
  }))(),


  _updateEnabledEngines: function _updateEnabledEngines() {
    this._log.info("Updating enabled engines: " + SyncScheduler.numClients + " clients.");
    let meta = Records.get(this.metaURL);
    if (meta.isNew || !meta.payload.engines)
      return;

    
    
    
    if ((SyncScheduler.numClients <= 1) &&
        ([e for (e in meta.payload.engines) if (e != "clients")].length == 0)) {
      this._log.info("One client and no enabled engines: not touching local engine status.");
      return;
    }

    this._ignorePrefObserver = true;

    let enabled = [eng.name for each (eng in Engines.getEnabled())];
    for (let engineName in meta.payload.engines) {
      if (engineName == "clients") {
        
        continue;
      }
      let index = enabled.indexOf(engineName);
      if (index != -1) {
        
        enabled.splice(index, 1);
        continue;
      }
      let engine = Engines.get(engineName);
      if (!engine) {
        
        continue;
      }

      if (Svc.Prefs.get("engineStatusChanged." + engine.prefName, false)) {
        
        
        this._log.trace("Wiping data for " + engineName + " engine.");
        engine.wipeServer();
        delete meta.payload.engines[engineName];
        meta.changed = true;
      } else {
        
        this._log.trace(engineName + " engine was enabled remotely.");
        engine.enabled = true;
      }
    }

    
    for each (let engineName in enabled) {
      let engine = Engines.get(engineName);
      if (Svc.Prefs.get("engineStatusChanged." + engine.prefName, false)) {
        this._log.trace("The " + engineName + " engine was enabled locally.");
      } else {
        this._log.trace("The " + engineName + " engine was disabled remotely.");
        engine.enabled = false;
      }
    }

    Svc.Prefs.resetBranch("engineStatusChanged.");
    this._ignorePrefObserver = false;
  },

  
  
  _syncEngine: function WeaveSvc__syncEngine(engine) {
    try {
      engine.sync();
      return true;
    }
    catch(e) {
      
      if (e.status == 401) {
        
        
        
        
        this.logout();
        Svc.Prefs.reset("clusterURL");
        Utils.nextTick(this.sync, this);
        return false;
      }
      return true;
    }
  },

  


  syncKeyNeedsUpgrade: function syncKeyNeedsUpgrade() {
    let p = this.passphrase;

    
    if (Utils.isPassphrase(p)) {
      this._log.info("Sync key is up-to-date: no need to upgrade.");
      return false;
    }

    return true;
  },

  









  upgradeSyncKey: function upgradeSyncKey(syncID) {
    let p = this.passphrase;

    
    if (!this.syncKeyNeedsUpgrade(p))
      return true;

    
    

    let s = btoa(syncID);        
    let k = Utils.derivePresentableKeyFromPassphrase(p, s, PBKDF2_KEY_BYTES);   

    if (!k) {
      this._log.error("No key resulted from derivePresentableKeyFromPassphrase. Failing upgrade.");
      return false;
    }

    this._log.info("Upgrading sync key...");
    this.passphrase = k;
    this._log.info("Saving upgraded sync key...");
    this.persistLogin();
    this._log.info("Done saving.");
    return true;
  },

  _freshStart: function WeaveSvc__freshStart() {
    this._log.info("Fresh start. Resetting client and considering key upgrade.");
    this.resetClient();
    CollectionKeys.clear();
    this.upgradeSyncKey(this.syncID);

    let meta = new WBORecord("meta", "global");
    meta.payload.syncID = this.syncID;
    meta.payload.storageVersion = STORAGE_VERSION;
    meta.isNew = true;

    this._log.debug("New metadata record: " + JSON.stringify(meta.payload));
    let resp = new Resource(this.metaURL).put(meta);
    if (!resp.success)
      throw resp;
    Records.set(this.metaURL, meta);

    
    let collections = [Clients].concat(Engines.getAll()).map(function(engine) {
      return engine.name;
    });
    this.wipeServer(collections);

    
    
    this.generateNewSymmetricKeys();
  },

  















  wipeServer: function wipeServer(collections, includeKeyPairs)
    this._notify("wipe-server", "", function() {
      if (!collections) {
        collections = [];
        let info = new Resource(this.infoURL).get();
        for (let name in info.obj) {
          if (includeKeyPairs || (name != "keys"))
            collections.push(name);
        }
      }
      for each (let name in collections) {
        let url = this.storageURL + name;
        let response = new Resource(url).delete();
        if (response.status != 200 && response.status != 404) {
          throw "Aborting wipeServer. Server responded with "
                + response.status + " response for " + url;
        }
      }
    })(),

  





  wipeClient: function WeaveSvc_wipeClient(engines)
    this._catch(this._notify("wipe-client", "", function() {
      
      if (!engines) {
        
        this.resetService();

        engines = [Clients].concat(Engines.getAll());
      }
      
      else
        engines = Engines.get(engines);

      
      for each (let engine in engines)
        if (engine.canDecrypt())
          engine.wipeClient();

      
      this.persistLogin();
    }))(),

  






  wipeRemote: function WeaveSvc_wipeRemote(engines)
    this._catch(this._notify("wipe-remote", "", function() {
      
      this.resetClient(engines);

      
      this.wipeServer(engines);

      
      if (engines) {
        engines.forEach(function(e) Clients.sendCommand("wipeEngine", [e]), this);
      }
      
      else {
        Clients.sendCommand("wipeAll", []);
      }

      
      Clients.sync();
    }))(),

  


  resetService: function WeaveSvc_resetService()
    this._catch(this._notify("reset-service", "", function() {
      this._log.info("Service reset.");

      
      this.syncID = "";
      Svc.Prefs.reset("lastSync");
      Records.clearCache();
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

  












  getStorageInfo: function getStorageInfo(type, callback) {
    if (STORAGE_INFO_TYPES.indexOf(type) == -1) {
      throw "Invalid value for 'type': " + type;
    }

    let info_type = "info/" + type;
    this._log.trace("Retrieving '" + info_type + "'...");
    let url = this.userBaseURL + info_type;
    return new SyncStorageRequest(url).get(function onComplete(error) {
      
      if (error) {
        this._log.debug("Failed to retrieve '" + info_type + "': " +
                        Utils.exceptionStr(error));
        return callback(error);
      }
      if (this.response.status != 200) {
        this._log.debug("Failed to retrieve '" + info_type +
                        "': server responded with HTTP" +
                        this.response.status);
        return callback(this.response);
      }

      let result;
      try {
        result = JSON.parse(this.response.body);
      } catch (ex) {
        this._log.debug("Server returned invalid JSON for '" + info_type +
                        "': " + this.response.body);
        return callback(ex);
      }
      this._log.trace("Successfully retrieved '" + info_type + "'.");
      return callback(null, result);
    });
  }

};


let Service = new WeaveSvc();
Service.onStartup();
