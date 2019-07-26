




const EXPORTED_SYMBOLS = ["Service", "Weave"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


const CLUSTER_BACKOFF = 5 * 60 * 1000; 


const PBKDF2_KEY_BYTES = 16;

const CRYPTO_COLLECTION = "crypto";
const KEYS_WBO = "keys";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-common/preferences.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/clients.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/main.js");
Cu.import("resource://services-sync/policies.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/rest.js");
Cu.import("resource://services-sync/stages/cluster.js");
Cu.import("resource://services-sync/stages/enginesync.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/userapi.js");
Cu.import("resource://services-sync/util.js");

const ENGINE_MODULES = {
  Addons: "addons.js",
  Apps: "apps.js",
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






function WeaveSvc() {
  this._notify = Utils.notify("weave:service:");
}
WeaveSvc.prototype = {

  _lock: Utils.lock,
  _locked: false,
  _loggedIn: false,
  _identity: Weave.Identity,

  userBaseURL: null,
  infoURL: null,
  storageURL: null,
  metaURL: null,
  cryptoKeyURL: null,

  get enabledEngineNames() {
    return [e.name for each (e in this.engineManager.getEnabled())];
  },

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

  






  get userAPIURI() {
    
    let url = Svc.Prefs.get("userURL");
    if (!url.contains(":")) {
      url = this.serverURL + url;
    }

    return url + USER_API_VERSION + "/";
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

  _updateCachedURLs: function _updateCachedURLs() {
    
    if (this.clusterURL == "" || this._identity.username == "")
      return;

    let storageAPI = this.clusterURL + SYNC_API_VERSION + "/";
    this.userBaseURL = storageAPI + this._identity.username + "/";
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
      let cryptoResp = cryptoKeys.fetch(this.cryptoKeysURL).response;

      
      
      let cipherText = cryptoKeys.ciphertext;

      if (!cryptoResp.success) {
        this._log.warn("Failed to download keys.");
        return false;
      }

      let keysChanged = this.handleFetchedKeys(this._identity.syncKeyBundle,
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

    this.errorHandler = new ErrorHandler();

    this._log = Log4Moz.repository.getLogger("Sync.Service");
    this._log.level =
      Log4Moz.Level[Svc.Prefs.get("log.logger.service.main")];

    this._log.info("Loading Weave " + WEAVE_VERSION);

    this._clusterManager = new ClusterManager(this);

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

    if (!this.enabled) {
      this._log.info("Firefox Sync disabled.");
    }

    this._updateCachedURLs();

    let status = this._checkSetup();
    if (status != STATUS_DISABLED && status != CLIENT_NOT_CONFIGURED) {
      Svc.Obs.notify("weave:engine:start-tracking");
    }

    
    
    
    Utils.nextTick(function onNextTick() {
      Status.ready = true;
      Svc.Obs.notify("weave:service:ready");
    });
  },

  _checkSetup: function _checkSetup() {
    if (!this.enabled) {
      return Status.service = STATUS_DISABLED;
    }
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

  


  _registerEngines: function _registerEngines() {
    
    this.engineManager = Engines;

    let engines = [];
    
    
    let pref = Svc.Prefs.get("registerEngines");
    if (pref) {
      engines = pref.split(",");
    }

    
    this.clientsEngine = Clients;

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

  


  _fetchInfo: function _fetchInfo(url) {
    let infoURL = url || this.infoURL;

    this._log.trace("In _fetchInfo: " + infoURL);
    let info;
    try {
      info = new Resource(infoURL).get();
    } catch (ex) {
      this.errorHandler.checkServerError(ex);
      throw ex;
    }
    if (!info.success) {
      this.errorHandler.checkServerError(info);
      throw "aborting sync, failed to get collections";
    }
    return info;
  },

  verifyAndFetchSymmetricKeys: function verifyAndFetchSymmetricKeys(infoResponse) {

    this._log.debug("Fetching and verifying -- or generating -- symmetric keys.");

    
    
    

    if (!this._identity.syncKey) {
      Status.login = LOGIN_FAILED_NO_PASSPHRASE;
      Status.sync = CREDENTIALS_CHANGED;
      return false;
    }

    let syncKeyBundle = this._identity.syncKeyBundle;
    if (!syncKeyBundle) {
      this._log.error("Sync Key Bundle not set. Invalid Sync Key?");

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
        this.errorHandler.checkServerError(infoResponse);
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
              let keysChanged = this.handleFetchedKeys(syncKeyBundle, cryptoKeys);
              return true;
            }
            else if (cryptoResp.status == 404) {
              
              
              this._log.warn("Got 404 for crypto/keys, but 'crypto' in info/collections. Regenerating.");
              cryptoKeys = null;
            }
            else {
              
              Status.login = LOGIN_FAILED_SERVER_ERROR;
              this.errorHandler.checkServerError(cryptoResp);
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

    } catch (ex) {
      
      this._log.debug("Failed to fetch and verify keys: "
                      + Utils.exceptionStr(ex));
      this.errorHandler.checkServerError(ex);
      return false;
    }
  },

  verifyLogin: function verifyLogin() {
    if (!this._identity.username) {
      this._log.warn("No username in verifyLogin.");
      Status.login = LOGIN_FAILED_NO_USERNAME;
      return false;
    }

    
    
    
    
    
    try {
      this._identity.syncKey;
    } catch (ex) {
      this._log.debug("Fetching passphrase threw " + ex +
                      "; assuming master password locked.");
      Status.login = MASTER_PASSWORD_LOCKED;
      return false;
    }

    try {
      
      
      
      if (this.clusterURL == "" && !this._clusterManager.setCluster()) {
        Status.sync = NO_SYNC_NODE_FOUND;
        Svc.Obs.notify("weave:service:sync:delayed");
        return true;
      }

      
      let test = new Resource(this.infoURL).get();

      switch (test.status) {
        case 200:
          

          
          
          
          if (!this._identity.syncKey) {
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
          

        case 404:
          
          if (this._clusterManager.setCluster()) {
            return this.verifyLogin();
          }

          
          Status.login = LOGIN_FAILED_LOGIN_REJECTED;
          return false;

        default:
          
          Status.login = LOGIN_FAILED_SERVER_ERROR;
          this.errorHandler.checkServerError(test);
          return false;
      }
    } catch (ex) {
      
      this._log.debug("verifyLogin failed: " + Utils.exceptionStr(ex));
      Status.login = LOGIN_FAILED_NETWORK_ERROR;
      this.errorHandler.checkServerError(ex);
      return false;
    }
  },

  generateNewSymmetricKeys: function generateNewSymmetricKeys() {
    this._log.info("Generating new keys WBO...");
    let wbo = CollectionKeys.generateNewKeysWBO();
    this._log.info("Encrypting new key bundle.");
    wbo.encrypt(this._identity.syncKeyBundle);

    this._log.info("Uploading...");
    let uploadRes = wbo.upload(this.cryptoKeysURL);
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
    let cryptoResp = cryptoKeys.fetch(this.cryptoKeysURL).response;
    if (cryptoResp.status != 200) {
      this._log.warn("Failed to download keys.");
      throw new Error("Symmetric key download failed.");
    }
    let keysChanged = this.handleFetchedKeys(this._identity.syncKeyBundle,
                                             cryptoKeys, true);
    if (keysChanged) {
      this._log.info("Downloaded keys differed, as expected.");
    }
  },

  changePassword: function changePassword(newPassword) {
    let client = new UserAPI10Client(this.userAPIURI);
    let cb = Async.makeSpinningCallback();
    client.changePassword(this._identity.username,
                          this._identity.basicPassword, newPassword, cb);

    try {
      cb.wait();
    } catch (ex) {
      this._log.debug("Password change failed: " +
                      CommonUtils.exceptionStr(ex));
      return false;
    }

    
    this._identity.basicPassword = newPassword;
    this.persistLogin();
    return true;
  },

  changePassphrase: function changePassphrase(newphrase) {
    return this._catch(function doChangePasphrase() {
      
      this.wipeServer();

      this.logout();

      
      this._identity.syncKey = newphrase;
      this.persistLogin();

      
      this.resetClient();
      CollectionKeys.clear();

      
      this.sync();

      Svc.Obs.notify("weave:service:change-passphrase", true);

      return true;
    })();
  },

  startOver: function startOver() {
    this._log.trace("Invoking Service.startOver.");
    Svc.Obs.notify("weave:engine:stop-tracking");
    Status.resetSync();

    
    
    
    this._identity.syncKey = null;
    Status.login = LOGIN_FAILED_NO_PASSPHRASE;
    this.logout();
    Svc.Obs.notify("weave:service:start-over");

    
    if (this.clusterURL != "") {
      
      for each (let engine in [this.clientsEngine].concat(this.engineManager.getAll())) {
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

    
    this._ignorePrefObserver = true;
    Svc.Prefs.resetBranch("");
    this._ignorePrefObserver = false;

    Svc.Prefs.set("lastversion", WEAVE_VERSION);

    this._identity.deleteSyncCredentials();
  },

  persistLogin: function persistLogin() {
    try {
      this._identity.persistCredentials(true);
    } catch (ex) {
      this._log.info("Unable to persist credentials: " + ex);
    }
  },

  login: function login(username, password, passphrase) {
    function onNotify() {
      this._loggedIn = false;
      if (Services.io.offline) {
        Status.login = LOGIN_FAILED_NETWORK_ERROR;
        throw "Application is offline, login should not be called";
      }

      let initialStatus = this._checkSetup();
      if (username) {
        this._identity.username = username;
      }
      if (password) {
        this._identity.basicPassword = password;
      }
      if (passphrase) {
        this._identity.syncKey = passphrase;
      }

      if (this._checkSetup() == CLIENT_NOT_CONFIGURED) {
        throw "Aborting login, client not configured.";
      }

      
      
      if (initialStatus == CLIENT_NOT_CONFIGURED
          && (username || password || passphrase)) {
        Svc.Obs.notify("weave:service:setup-complete");
      }

      this._log.info("Logging in user " + this._identity.username);
      this._updateCachedURLs();

      if (!this.verifyLogin()) {
        
        throw "Login failed: " + Status.login;
      }

      this._loggedIn = true;

      return true;
    }

    let notifier = this._notify("login", "", onNotify.bind(this));
    return this._catch(this._lock("service.js: login", notifier))();
  },

  logout: function logout() {
    
    if (!this._loggedIn)
      return;

    this._log.info("Logging out");
    this._loggedIn = false;

    Svc.Obs.notify("weave:service:logout:finish");
  },

  checkAccount: function checkAccount(account) {
    let client = new UserAPI10Client(this.userAPIURI);
    let cb = Async.makeSpinningCallback();

    let username = this._identity.usernameFromAccount(account);
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
    let meta = Records.get(this.metaURL);

    
    if (infoResponse &&
        (infoResponse.obj.meta != this.metaModified) &&
        (!meta || !meta.isNew)) {

      
      this._log.debug("Clearing cached meta record. metaModified is " +
          JSON.stringify(this.metaModified) + ", setting to " +
          JSON.stringify(infoResponse.obj.meta));

      Records.del(this.metaURL);

      
      let newMeta = Records.get(this.metaURL);

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
        this.errorHandler.checkServerError(Records.response);
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

  







  _checkSync: function _checkSync(ignore) {
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

  


  _lockedSync: function _lockedSync() {
    return this._lock("service.js: sync",
                      this._notify("sync", "", function onNotify() {

      let synchronizer = new EngineSynchronizer(this);
      let cb = Async.makeSpinningCallback();
      synchronizer.onComplete = cb;

      synchronizer.sync();
      
      
      let result = cb.wait();
    }))();
  },

  









  upgradeSyncKey: function upgradeSyncKey(syncID) {
    let p = this._identity.syncKey;

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
    this._identity.syncKey = k;
    this._log.info("Saving upgraded sync key...");
    this.persistLogin();
    this._log.info("Done saving.");
    return true;
  },

  _freshStart: function _freshStart() {
    this._log.info("Fresh start. Resetting client and considering key upgrade.");
    this.resetClient();
    CollectionKeys.clear();
    this.upgradeSyncKey(this.syncID);

    
    let wipeTimestamp = this.wipeServer();

    
    let meta = new WBORecord("meta", "global");
    meta.payload.syncID = this.syncID;
    meta.payload.storageVersion = STORAGE_VERSION;
    meta.isNew = true;

    this._log.debug("New metadata record: " + JSON.stringify(meta.payload));
    let res = new Resource(this.metaURL);
    
    
    
    
    let resp = res.put(meta);
    if (!resp.success) {
      
      
      
      throw resp;
    }
    Records.set(this.metaURL, meta);

    
    let engines = [this.clientsEngine].concat(this.engineManager.getAll());
    let collections = [engine.name for each (engine in engines)];

    
    
    this.generateNewSymmetricKeys();
  },

  








  wipeServer: function wipeServer(collections) {
    let response;
    if (!collections) {
      
      let res = new Resource(this.storageURL.slice(0, -1));
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
        response = new Resource(url).delete();
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
        engines.forEach(function(e) this.clientsEngine.sendCommand("wipeEngine", [e]), this);
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
      Records.clearCache();
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
