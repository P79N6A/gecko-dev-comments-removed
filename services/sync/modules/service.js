



this.EXPORTED_SYMBOLS = ["Service"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


const CLUSTER_BACKOFF = 5 * 60 * 1000; 


const PBKDF2_KEY_BYTES = 16;

const CRYPTO_COLLECTION = "crypto";
const KEYS_WBO = "keys";

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/rest.js");
Cu.import("resource://services-sync/stages/enginesync.js");
Cu.import("resource://services-sync/stages/declined.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/userapi.js");
Cu.import("resource://services-sync/util.js");

const ENGINE_MODULES = {
  Addons: "addons.js",
  Bookmarks: "bookmarks.js",
  Form: "forms.js",
  History: "history.js",
  Password: "passwords.js",
  Prefs: "prefs.js",
  Tab: "tabs.js",
};

const STORAGE_INFO_TYPES = [INFO_COLLECTIONS,
                            INFO_COLLECTION_USAGE,
                            INFO_COLLECTION_COUNTS,
                            INFO_QUOTA];



const TELEMETRY_CUSTOM_SERVER_PREFS = {
  WEAVE_CUSTOM_LEGACY_SERVER_CONFIGURATION: "services.sync.serverURL",
  WEAVE_CUSTOM_FXA_SERVER_CONFIGURATION: "identity.fxaccounts.auth.uri",
  WEAVE_CUSTOM_TOKEN_SERVER_CONFIGURATION: "services.sync.tokenServerURI",
};


function Sync11Service() {
  this._notify = Utils.notify("weave:service:");
}
Sync11Service.prototype = {

  _lock: Utils.lock,
  _locked: false,
  _loggedIn: false,

  infoURL: null,
  storageURL: null,
  metaURL: null,
  cryptoKeyURL: null,

  get serverURL() Svc.Prefs.get("serverURL"),
  set serverURL(value) {
    if (!value.endsWith("/")) {
      value += "/";
    }

    
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

  






  get userAPIURI() {
    
    let url = Svc.Prefs.get("userURL");
    if (!url.includes(":")) {
      url = this.serverURL + url;
    }

    return url + USER_API_VERSION + "/";
  },

  get pwResetURL() {
    return this.serverURL + "weave-password-reset";
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
  lock: function lock() {
    if (this._locked)
      return false;
    this._locked = true;
    return true;
  },
  unlock: function unlock() {
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

  get userBaseURL() {
    if (!this._clusterManager) {
      return null;
    }
    return this._clusterManager.getUserBaseURL();
  },

  _updateCachedURLs: function _updateCachedURLs() {
    
    if (!this.clusterURL || !this.identity.username)
      return;

    this._log.debug("Caching URLs under storage user base: " + this.userBaseURL);

    
    this.infoURL = this.userBaseURL + "info/collections";
    this.storageURL = this.userBaseURL + "storage/";
    this.metaURL = this.storageURL + "meta/global";
    this.cryptoKeysURL = this.storageURL + CRYPTO_COLLECTION + "/" + KEYS_WBO;
  },

  _checkCrypto: function _checkCrypto() {
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
      let cryptoResp = cryptoKeys.fetch(this.resource(this.cryptoKeysURL)).response;

      
      
      let cipherText = cryptoKeys.ciphertext;

      if (!cryptoResp.success) {
        this._log.warn("Failed to download keys.");
        return false;
      }

      let keysChanged = this.handleFetchedKeys(this.identity.syncKeyBundle,
                                               cryptoKeys, true);
      if (keysChanged) {
        
        this._log.info("Suggesting retry.");
        return true;              
      }

      
      cryptoKeys.ciphertext = cipherText;
      cryptoKeys.cleartext  = null;

      let uploadResp = cryptoKeys.upload(this.resource(this.cryptoKeysURL));
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
    
    let wasBlank = this.collectionKeys.isClear;
    let keysChanged = this.collectionKeys.updateContents(syncKey, cryptoKeys);

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

  
  
  get enabled() {
    return Svc.Prefs.get("enabled");
  },
  set enabled(val) {
    
    
    
    if (val) {
      throw new Error("Only disabling via this setter is supported");
    }
    Svc.Prefs.set("enabled", val);
  },

  


  onStartup: function onStartup() {
    this._migratePrefs();

    
    
    
    
    if (!Status || !Status._authManager) {
      throw new Error("Status or Status._authManager not initialized.");
    }

    this.status = Status;
    this.identity = Status._authManager;
    this.collectionKeys = new CollectionKeyManager();

    this.errorHandler = new ErrorHandler(this);

    this._log = Log.repository.getLogger("Sync.Service");
    this._log.level =
      Log.Level[Svc.Prefs.get("log.logger.service.main")];

    this._log.info("Loading Weave " + WEAVE_VERSION);

    this._clusterManager = this.identity.createClusterManager(this);
    this.recordManager = new RecordManager(this);

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

    this.scheduler = new SyncScheduler(this);

    if (!this.enabled) {
      this._log.info("Firefox Sync disabled.");
    }

    this._updateCachedURLs();

    let status = this._checkSetup();
    if (status != STATUS_DISABLED && status != CLIENT_NOT_CONFIGURED) {
      Svc.Obs.notify("weave:engine:start-tracking");
    }

    
    for (let [probeName, prefName] of Iterator(TELEMETRY_CUSTOM_SERVER_PREFS)) {
      let isCustomized = Services.prefs.prefHasUserValue(prefName);
      Services.telemetry.getHistogramById(probeName).add(isCustomized);
    }

    
    
    
    Utils.nextTick(function onNextTick() {
      this.status.ready = true;

      
      
      let xps = Cc["@mozilla.org/weave/service;1"]
                  .getService(Ci.nsISupports)
                  .wrappedJSObject;
      xps.ready = true;

      Svc.Obs.notify("weave:service:ready");
    }.bind(this));
  },

  _checkSetup: function _checkSetup() {
    if (!this.enabled) {
      return this.status.service = STATUS_DISABLED;
    }
    return this.status.checkSetup();
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

  


  _registerEngines: function _registerEngines() {
    this.engineManager = new EngineManager(this);

    let engines = [];
    
    
    let pref = Svc.Prefs.get("registerEngines");
    if (pref) {
      engines = pref.split(",");
    }

    let declined = [];
    pref = Svc.Prefs.get("declinedEngines");
    if (pref) {
      declined = pref.split(",");
    }

    this.clientsEngine = new ClientEngine(this);

    for (let name of engines) {
      if (!name in ENGINE_MODULES) {
        this._log.info("Do not know about engine: " + name);
        continue;
      }

      let ns = {};
      try {
        Cu.import("resource://services-sync/engines/" + ENGINE_MODULES[name], ns);

        let engineName = name + "Engine";
        if (!(engineName in ns)) {
          this._log.warn("Could not find exported engine instance: " + engineName);
          continue;
        }

        this.engineManager.register(ns[engineName]);
      } catch (ex) {
        this._log.warn("Could not register engine " + name + ": " +
                       CommonUtils.exceptionStr(ex));
      }
    }

    this.engineManager.setDeclined(declined);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),

  

  observe: function observe(subject, topic, data) {
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

  


  resource: function resource(url) {
    let res = new Resource(url);
    res.authenticator = this.identity.getResourceAuthenticator();

    return res;
  },

  


  getStorageRequest: function getStorageRequest(url) {
    let request = new SyncStorageRequest(url);
    request.authenticator = this.identity.getRESTRequestAuthenticator();

    return request;
  },

  



  _fetchInfo: function (url) {
    let infoURL = url || this.infoURL;

    this._log.trace("In _fetchInfo: " + infoURL);
    let info;
    try {
      info = this.resource(infoURL).get();
    } catch (ex) {
      this.errorHandler.checkServerError(ex);
      throw ex;
    }

    
    this.errorHandler.checkServerError(info);
    if (!info.success) {
      throw "Aborting sync: failed to get collections.";
    }
    return info;
  },

  verifyAndFetchSymmetricKeys: function verifyAndFetchSymmetricKeys(infoResponse) {

    this._log.debug("Fetching and verifying -- or generating -- symmetric keys.");

    
    
    

    if (!this.identity.syncKey) {
      this.status.login = LOGIN_FAILED_NO_PASSPHRASE;
      this.status.sync = CREDENTIALS_CHANGED;
      return false;
    }

    let syncKeyBundle = this.identity.syncKeyBundle;
    if (!syncKeyBundle) {
      this._log.error("Sync Key Bundle not set. Invalid Sync Key?");

      this.status.login = LOGIN_FAILED_INVALID_PASSPHRASE;
      this.status.sync = CREDENTIALS_CHANGED;
      return false;
    }

    try {
      if (!infoResponse)
        infoResponse = this._fetchInfo();    

      
      if (infoResponse.status != 200) {
        this._log.warn("info/collections returned non-200 response. Failing key fetch.");
        this.status.login = LOGIN_FAILED_SERVER_ERROR;
        this.errorHandler.checkServerError(infoResponse);
        return false;
      }

      let infoCollections = infoResponse.obj;

      this._log.info("Testing info/collections: " + JSON.stringify(infoCollections));

      if (this.collectionKeys.updateNeeded(infoCollections)) {
        this._log.info("collection keys reports that a key update is needed.");

        

        
        let cryptoKeys;

        if (infoCollections && (CRYPTO_COLLECTION in infoCollections)) {
          try {
            cryptoKeys = new CryptoWrapper(CRYPTO_COLLECTION, KEYS_WBO);
            let cryptoResp = cryptoKeys.fetch(this.resource(this.cryptoKeysURL)).response;

            if (cryptoResp.success) {
              let keysChanged = this.handleFetchedKeys(syncKeyBundle, cryptoKeys);
              return true;
            }
            else if (cryptoResp.status == 404) {
              
              
              this._log.warn("Got 404 for crypto/keys, but 'crypto' in info/collections. Regenerating.");
              cryptoKeys = null;
            }
            else {
              
              this.status.login = LOGIN_FAILED_SERVER_ERROR;
              this.errorHandler.checkServerError(cryptoResp);
              this._log.warn("Got status " + cryptoResp.status + " fetching crypto keys.");
              return false;
            }
          }
          catch (ex) {
            this._log.warn("Got exception \"" + ex + "\" fetching cryptoKeys.");
            

            
            if (Utils.isHMACMismatch(ex)) {
              this.status.login = LOGIN_FAILED_INVALID_PASSPHRASE;
              this.status.sync = CREDENTIALS_CHANGED;
            }
            else {
              
              
              this.status.login = LOGIN_FAILED;
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

    } catch (ex) {
      
      this._log.debug("Failed to fetch and verify keys: "
                      + Utils.exceptionStr(ex));
      this.errorHandler.checkServerError(ex);
      return false;
    }
  },

  verifyLogin: function verifyLogin(allow40XRecovery = true) {
    
    if (!this.identity.readyToAuthenticate) {
      this._log.info("Not ready to authenticate in verifyLogin.");
      this.status.login = LOGIN_FAILED_NOT_READY;
      return false;
    }

    if (!this.identity.username) {
      this._log.warn("No username in verifyLogin.");
      this.status.login = LOGIN_FAILED_NO_USERNAME;
      return false;
    }

    
    
    
    
    
    
    let cb = Async.makeSpinningCallback();
    this.identity.unlockAndVerifyAuthState().then(
      result => cb(null, result),
      cb
    );
    let unlockedState = cb.wait();
    this._log.debug("Fetching unlocked auth state returned " + unlockedState);
    if (unlockedState != STATUS_OK) {
      this.status.login = unlockedState;
      return false;
    }

    try {
      
      
      
      if (this.clusterURL == "" && !this._clusterManager.setCluster()) {
        this.status.sync = NO_SYNC_NODE_FOUND;
        return true;
      }

      
      let test = this.resource(this.infoURL).get();

      switch (test.status) {
        case 200:
          

          
          
          
          if (!this.identity.syncKey) {
            this._log.warn("No passphrase in verifyLogin.");
            this.status.login = LOGIN_FAILED_NO_PASSPHRASE;
            return false;
          }

          
          
          if (this._remoteSetup(test)) {
            
            this.status.login = LOGIN_SUCCEEDED;
            return true;
          }

          this._log.warn("Remote setup failed.");
          
          return false;

        case 401:
          this._log.warn("401: login failed.");
          

        case 404:
          
          if (allow40XRecovery && this._clusterManager.setCluster()) {
            return this.verifyLogin(false);
          }

          
          this.status.login = LOGIN_FAILED_LOGIN_REJECTED;
          return false;

        default:
          
          this.status.login = LOGIN_FAILED_SERVER_ERROR;
          this.errorHandler.checkServerError(test);
          return false;
      }
    } catch (ex) {
      
      this._log.debug("verifyLogin failed: " + Utils.exceptionStr(ex));
      this.status.login = LOGIN_FAILED_NETWORK_ERROR;
      this.errorHandler.checkServerError(ex);
      return false;
    }
  },

  generateNewSymmetricKeys: function generateNewSymmetricKeys() {
    this._log.info("Generating new keys WBO...");
    let wbo = this.collectionKeys.generateNewKeysWBO();
    this._log.info("Encrypting new key bundle.");
    wbo.encrypt(this.identity.syncKeyBundle);

    this._log.info("Uploading...");
    let uploadRes = wbo.upload(this.resource(this.cryptoKeysURL));
    if (uploadRes.status != 200) {
      this._log.warn("Got status " + uploadRes.status + " uploading new keys. What to do? Throw!");
      this.errorHandler.checkServerError(uploadRes);
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
    let cryptoResp = cryptoKeys.fetch(this.resource(this.cryptoKeysURL)).response;
    if (cryptoResp.status != 200) {
      this._log.warn("Failed to download keys.");
      throw new Error("Symmetric key download failed.");
    }
    let keysChanged = this.handleFetchedKeys(this.identity.syncKeyBundle,
                                             cryptoKeys, true);
    if (keysChanged) {
      this._log.info("Downloaded keys differed, as expected.");
    }
  },

  changePassword: function changePassword(newPassword) {
    let client = new UserAPI10Client(this.userAPIURI);
    let cb = Async.makeSpinningCallback();
    client.changePassword(this.identity.username,
                          this.identity.basicPassword, newPassword, cb);

    try {
      cb.wait();
    } catch (ex) {
      this._log.debug("Password change failed: " +
                      CommonUtils.exceptionStr(ex));
      return false;
    }

    
    this.identity.basicPassword = newPassword;
    this.persistLogin();
    return true;
  },

  changePassphrase: function changePassphrase(newphrase) {
    return this._catch(function doChangePasphrase() {
      
      this.wipeServer();

      this.logout();

      
      this.identity.syncKey = newphrase;
      this.persistLogin();

      
      this.resetClient();
      this.collectionKeys.clear();

      
      this.sync();

      Svc.Obs.notify("weave:service:change-passphrase", true);

      return true;
    })();
  },

  startOver: function startOver() {
    this._log.trace("Invoking Service.startOver.");
    Svc.Obs.notify("weave:engine:stop-tracking");
    this.status.resetSync();

    
    if (this.clusterURL != "") {
      
      for each (let engine in [this.clientsEngine].concat(this.engineManager.getAll())) {
        try {
          engine.removeClientData();
        } catch(ex) {
          this._log.warn("Deleting client data for " + engine.name + " failed:"
                         + Utils.exceptionStr(ex));
        }
      }
      this._log.debug("Finished deleting client data.");
    } else {
      this._log.debug("Skipping client data removal: no cluster URL.");
    }

    
    
    
    this._log.info("Service.startOver dropping sync key and logging out.");
    this.identity.resetSyncKey();
    this.status.login = LOGIN_FAILED_NO_PASSPHRASE;
    this.logout();
    Svc.Obs.notify("weave:service:start-over");

    
    this.resetClient();
    this.collectionKeys.clear();
    this.status.resetBackoff();

    
    this._ignorePrefObserver = true;
    Svc.Prefs.resetBranch("");
    this._ignorePrefObserver = false;

    Svc.Prefs.set("lastversion", WEAVE_VERSION);

    this.identity.deleteSyncCredentials();

    
    
    let keepIdentity = false;
    try {
      keepIdentity = Services.prefs.getBoolPref("services.sync-testing.startOverKeepIdentity");
    } catch (_) {  }
    if (keepIdentity) {
      Svc.Obs.notify("weave:service:start-over:finish");
      return;
    }

    this.identity.finalize().then(
      () => {
        
        
        Svc.Obs.notify("weave:service:start-over:init-identity");
        this.identity.username = "";
        this.status.__authManager = null;
        this.identity = Status._authManager;
        this._clusterManager = this.identity.createClusterManager(this);
        Svc.Obs.notify("weave:service:start-over:finish");
      }
    ).then(null,
      err => {
        this._log.error("startOver failed to re-initialize the identity manager: " + err);
        
        
        Svc.Obs.notify("weave:service:start-over:finish");
      }
    );
  },

  persistLogin: function persistLogin() {
    try {
      this.identity.persistCredentials(true);
    } catch (ex) {
      this._log.info("Unable to persist credentials: " + ex);
    }
  },

  login: function login(username, password, passphrase) {
    function onNotify() {
      this._loggedIn = false;
      if (Services.io.offline) {
        this.status.login = LOGIN_FAILED_NETWORK_ERROR;
        throw "Application is offline, login should not be called";
      }

      let initialStatus = this._checkSetup();
      if (username) {
        this.identity.username = username;
      }
      if (password) {
        this.identity.basicPassword = password;
      }
      if (passphrase) {
        this.identity.syncKey = passphrase;
      }

      if (this._checkSetup() == CLIENT_NOT_CONFIGURED) {
        throw "Aborting login, client not configured.";
      }

      
      let cb = Async.makeSpinningCallback();
      this.identity.ensureLoggedIn().then(
        () => cb(null),
        err => cb(err || "ensureLoggedIn failed")
      );

      
      cb.wait();

      
      
      if (initialStatus == CLIENT_NOT_CONFIGURED
          && (username || password || passphrase)) {
        Svc.Obs.notify("weave:service:setup-complete");
      }
      this._log.info("Logging in the user.");
      this._updateCachedURLs();

      if (!this.verifyLogin()) {
        
        throw "Login failed: " + this.status.login;
      }

      this._loggedIn = true;

      return true;
    }

    let notifier = this._notify("login", "", onNotify.bind(this));
    return this._catch(this._lock("service.js: login", notifier))();
  },

  logout: function logout() {
    
    
    
    this._log.info("Logging out");
    this.identity.logout();
    this._loggedIn = false;

    Svc.Obs.notify("weave:service:logout:finish");
  },

  checkAccount: function checkAccount(account) {
    let client = new UserAPI10Client(this.userAPIURI);
    let cb = Async.makeSpinningCallback();

    let username = this.identity.usernameFromAccount(account);
    client.usernameExists(username, cb);

    try {
      let exists = cb.wait();
      return exists ? "notAvailable" : "available";
    } catch (ex) {
      
      return this.errorHandler.errorStr(ex);
    }
  },

  createAccount: function createAccount(email, password,
                                        captchaChallenge, captchaResponse) {
    let client = new UserAPI10Client(this.userAPIURI);

    
    
    if (Svc.Prefs.isSet("admin-secret")) {
      client.adminSecret = Svc.Prefs.get("admin-secret", "");
    }

    let cb = Async.makeSpinningCallback();

    client.createAccount(email, password, captchaChallenge, captchaResponse,
                         cb);

    try {
      cb.wait();
      return null;
    } catch (ex) {
      return this.errorHandler.errorStr(ex.body);
    }
  },

  
  
  _remoteSetup: function _remoteSetup(infoResponse) {
    let reset = false;

    this._log.debug("Fetching global metadata record");
    let meta = this.recordManager.get(this.metaURL);

    
    if (infoResponse &&
        (infoResponse.obj.meta != this.metaModified) &&
        (!meta || !meta.isNew)) {

      
      this._log.debug("Clearing cached meta record. metaModified is " +
          JSON.stringify(this.metaModified) + ", setting to " +
          JSON.stringify(infoResponse.obj.meta));

      this.recordManager.del(this.metaURL);

      
      let newMeta = this.recordManager.get(this.metaURL);

      
      
      if (this.recordManager.response.status == 401) {
        this._log.debug("Fetching meta/global record on the server returned 401.");
        this.errorHandler.checkServerError(this.recordManager.response);
        return false;
      }

      if (!this.recordManager.response.success || !newMeta) {
        this._log.debug("No meta/global record on the server. Creating one.");
        newMeta = new WBORecord("meta", "global");
        newMeta.payload.syncID = this.syncID;
        newMeta.payload.storageVersion = STORAGE_VERSION;
        newMeta.payload.declined = this.engineManager.getDeclined();

        newMeta.isNew = true;

        this.recordManager.set(this.metaURL, newMeta);
        if (!newMeta.upload(this.resource(this.metaURL)).success) {
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

      
      let status = this.recordManager.response.status;
      if (status != 200 && status != 404) {
        this.status.sync = METARECORD_DOWNLOAD_FAIL;
        this.errorHandler.checkServerError(this.recordManager.response);
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
      this.status.sync = VERSION_OUT_OF_DATE;
      this._log.warn("Upgrade required to access newer storage version.");
      return false;
    }
    else if (meta.payload.syncID != this.syncID) {

      this._log.info("Sync IDs differ. Local is " + this.syncID + ", remote is " + meta.payload.syncID);
      this.resetClient();
      this.collectionKeys.clear();
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
        this.status.sync = CREDENTIALS_CHANGED;
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

  







  _checkSync: function _checkSync(ignore) {
    let reason = "";
    if (!this.enabled)
      reason = kSyncWeaveDisabled;
    else if (Services.io.offline)
      reason = kSyncNetworkOffline;
    else if (this.status.minimumNextSync > Date.now())
      reason = kSyncBackoffNotMet;
    else if ((this.status.login == MASTER_PASSWORD_LOCKED) &&
             Utils.mpLocked())
      reason = kSyncMasterPasswordLocked;
    else if (Svc.Prefs.get("firstSync") == "notReady")
      reason = kFirstSyncChoiceNotMade;

    if (ignore && ignore.indexOf(reason) != -1)
      return "";

    return reason;
  },

  sync: function sync() {
    if (!this.enabled) {
      this._log.debug("Not syncing as Sync is disabled.");
      return;
    }
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

  


  _lockedSync: function _lockedSync() {
    return this._lock("service.js: sync",
                      this._notify("sync", "", function onNotify() {

      let histogram = Services.telemetry.getHistogramById("WEAVE_START_COUNT");
      histogram.add(1);

      let synchronizer = new EngineSynchronizer(this);
      let cb = Async.makeSpinningCallback();
      synchronizer.onComplete = cb;

      synchronizer.sync();
      
      
      let result = cb.wait();

      histogram = Services.telemetry.getHistogramById("WEAVE_COMPLETE_SUCCESS_COUNT");
      histogram.add(1);

      
      
      
      
      
      if (this.clusterURL) {
        this.identity.prefetchMigrationSentinel(this);
      }

      
      let meta = this.recordManager.get(this.metaURL);
      if (!meta) {
        this._log.warn("No meta/global; can't update declined state.");
        return;
      }

      let declinedEngines = new DeclinedEngines(this);
      let didChange = declinedEngines.updateDeclined(meta, this.engineManager);
      if (!didChange) {
        this._log.info("No change to declined engines. Not reuploading meta/global.");
        return;
      }

      this.uploadMetaGlobal(meta);
    }))();
  },

  


  uploadMetaGlobal: function (meta) {
    this._log.debug("Uploading meta/global: " + JSON.stringify(meta));

    
    
    
    
    let res = this.resource(this.metaURL);
    let response = res.put(meta);
    if (!response.success) {
      throw response;
    }
    this.recordManager.set(this.metaURL, meta);
  },

  






  getFxAMigrationSentinel: function() {
    if (this._shouldLogin()) {
      this._log.debug("In getFxAMigrationSentinel: should login.");
      if (!this.login()) {
        this._log.debug("Can't get migration sentinel: login returned false.");
        return Promise.resolve(null);
      }
    }
    if (!this.identity.syncKeyBundle) {
      this._log.error("Can't get migration sentinel: no syncKeyBundle.");
      return Promise.resolve(null);
    }
    try {
      let collectionURL = this.storageURL + "meta/fxa_credentials";
      let cryptoWrapper = this.recordManager.get(collectionURL);
      if (!cryptoWrapper || !cryptoWrapper.payload) {
        
        
        return Promise.resolve(null);
      }
      
      
      if (cryptoWrapper.payload.sentinel) {
        return Promise.resolve(cryptoWrapper.payload.sentinel);
      }
      
      
      let payload = cryptoWrapper.decrypt(this.identity.syncKeyBundle);
      
      
      cryptoWrapper.payload = payload;
      return Promise.resolve(payload.sentinel);
    } catch (ex) {
      this._log.error("Failed to fetch the migration sentinel: ${}", ex);
      return Promise.resolve(null);
    }
  },

  







  setFxAMigrationSentinel: function(sentinel) {
    if (this._shouldLogin()) {
      this._log.debug("In setFxAMigrationSentinel: should login.");
      if (!this.login()) {
        this._log.debug("Can't set migration sentinel: login returned false.");
        return Promise.resolve(false);
      }
    }
    if (!this.identity.syncKeyBundle) {
      this._log.error("Can't set migration sentinel: no syncKeyBundle.");
      return Promise.resolve(false);
    }
    try {
      let collectionURL = this.storageURL + "meta/fxa_credentials";
      let cryptoWrapper = new CryptoWrapper("meta", "fxa_credentials");
      cryptoWrapper.cleartext.sentinel = sentinel;

      cryptoWrapper.encrypt(this.identity.syncKeyBundle);

      let res = this.resource(collectionURL);
      let response = res.put(cryptoWrapper.toJSON());

      if (!response.success) {
        throw response;
      }
      this.recordManager.set(collectionURL, cryptoWrapper);
    } catch (ex) {
      this._log.error("Failed to set the migration sentinel: ${}", ex);
      return Promise.resolve(false);
    }
    return Promise.resolve(true);
  },

  









  upgradeSyncKey: function upgradeSyncKey(syncID) {
    let p = this.identity.syncKey;

    if (!p) {
      return false;
    }

    
    if (Utils.isPassphrase(p)) {
      this._log.info("Sync key is up-to-date: no need to upgrade.");
      return true;
    }

    
    

    let s = btoa(syncID);        
    let k = Utils.derivePresentableKeyFromPassphrase(p, s, PBKDF2_KEY_BYTES);   

    if (!k) {
      this._log.error("No key resulted from derivePresentableKeyFromPassphrase. Failing upgrade.");
      return false;
    }

    this._log.info("Upgrading sync key...");
    this.identity.syncKey = k;
    this._log.info("Saving upgraded sync key...");
    this.persistLogin();
    this._log.info("Done saving.");
    return true;
  },

  _freshStart: function _freshStart() {
    this._log.info("Fresh start. Resetting client and considering key upgrade.");
    this.resetClient();
    this.collectionKeys.clear();
    this.upgradeSyncKey(this.syncID);

    
    let wipeTimestamp = this.wipeServer();

    
    let meta = new WBORecord("meta", "global");
    meta.payload.syncID = this.syncID;
    meta.payload.storageVersion = STORAGE_VERSION;
    meta.payload.declined = this.engineManager.getDeclined();
    meta.isNew = true;

    
    
    
    
    this.uploadMetaGlobal(meta);

    
    let engines = [this.clientsEngine].concat(this.engineManager.getAll());
    let collections = [engine.name for each (engine in engines)];
    

    
    
    this.generateNewSymmetricKeys();
  },

  








  wipeServer: function wipeServer(collections) {
    let response;
    if (!collections) {
      
      let res = this.resource(this.storageURL.slice(0, -1));
      res.setHeader("X-Confirm-Delete", "1");
      try {
        response = res.delete();
      } catch (ex) {
        this._log.debug("Failed to wipe server: " + CommonUtils.exceptionStr(ex));
        throw ex;
      }
      if (response.status != 200 && response.status != 404) {
        this._log.debug("Aborting wipeServer. Server responded with " +
                        response.status + " response for " + this.storageURL);
        throw response;
      }
      return response.headers["x-weave-timestamp"];
    }

    let timestamp;
    for (let name of collections) {
      let url = this.storageURL + name;
      try {
        response = this.resource(url).delete();
      } catch (ex) {
        this._log.debug("Failed to wipe '" + name + "' collection: " +
                        Utils.exceptionStr(ex));
        throw ex;
      }

      if (response.status != 200 && response.status != 404) {
        this._log.debug("Aborting wipeServer. Server responded with " +
                        response.status + " response for " + url);
        throw response;
      }

      if ("x-weave-timestamp" in response.headers) {
        timestamp = response.headers["x-weave-timestamp"];
      }
    }

    return timestamp;
  },

  





  wipeClient: function wipeClient(engines) {
    
    if (!engines) {
      
      this.resetService();

      engines = [this.clientsEngine].concat(this.engineManager.getAll());
    }
    
    else {
      engines = this.engineManager.get(engines);
    }

    
    for each (let engine in engines) {
      if (engine.canDecrypt()) {
        engine.wipeClient();
      }
    }

    
    this.persistLogin();
  },

  






  wipeRemote: function wipeRemote(engines) {
    try {
      
      this.resetClient(engines);

      
      this.wipeServer(engines);

      
      if (engines) {
        engines.forEach(function(e) {
            this.clientsEngine.sendCommand("wipeEngine", [e]);
          }, this);
      }
      
      else {
        this.clientsEngine.sendCommand("wipeAll", []);
      }

      
      this.clientsEngine.sync();
    } catch (ex) {
      this.errorHandler.checkServerError(ex);
      throw ex;
    }
  },

  


  resetService: function resetService() {
    this._catch(function reset() {
      this._log.info("Service reset.");

      
      this.syncID = "";
      this.recordManager.clearCache();
    })();
  },

  





  resetClient: function resetClient(engines) {
    this._catch(function doResetClient() {
      
      if (!engines) {
        
        this.resetService();

        engines = [this.clientsEngine].concat(this.engineManager.getAll());
      }
      
      else {
        engines = this.engineManager.get(engines);
      }

      
      for each (let engine in engines) {
        engine.resetClient();
      }
    })();
  },

  












  getStorageInfo: function getStorageInfo(type, callback) {
    if (STORAGE_INFO_TYPES.indexOf(type) == -1) {
      throw "Invalid value for 'type': " + type;
    }

    let info_type = "info/" + type;
    this._log.trace("Retrieving '" + info_type + "'...");
    let url = this.userBaseURL + info_type;
    return this.getStorageRequest(url).get(function onComplete(error) {
      
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
  },
};

this.Service = new Sync11Service();
Service.onStartup();
