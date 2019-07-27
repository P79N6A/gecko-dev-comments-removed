


"use strict";

this.EXPORTED_SYMBOLS = [
  "FxAccountsStorageManager",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://services-common/utils.js");

this.FxAccountsStorageManager = function(options = {}) {
  this.options = {
    filename: options.filename || DEFAULT_STORAGE_FILENAME,
    baseDir: options.baseDir || OS.Constants.Path.profileDir,
  }
  this.plainStorage = new JSONStorage(this.options);
  
  
  let useSecure = 'useSecure' in options ? options.useSecure : haveLoginManager;
  if (useSecure) {
    this.secureStorage = new LoginManagerStorage();
  } else {
    this.secureStorage = null;
  }
  this._clearCachedData();
  
  this._promiseInitialized = Promise.reject("initialize not called");
  
  this._promiseStorageComplete = Promise.resolve();
}

this.FxAccountsStorageManager.prototype = {
  _initialized: false,
  _needToReadSecure: true,

  
  
  initialize(accountData) {
    if (this._initialized) {
      throw new Error("already initialized");
    }
    this._initialized = true;
    
    
    
    this._promiseInitialized.catch(() => {});
    this._promiseInitialized = this._initialize(accountData);
  },

  _initialize: Task.async(function* (accountData) {
    log.trace("initializing new storage manager");
    try {
      if (accountData) {
        
        this._needToReadSecure = false;
        
        for (let [name, val] of Iterator(accountData)) {
          if (FXA_PWDMGR_PLAINTEXT_FIELDS.indexOf(name) >= 0) {
            this.cachedPlain[name] = val;
          } else {
            this.cachedSecure[name] = val;
          }
        }
        
        yield this._write();
        return;
      }
      
      
      
      this._needToReadSecure = yield this._readPlainStorage();
      if (this._needToReadSecure && this.secureStorage) {
        yield this._doReadAndUpdateSecure();
      }
    } finally {
      log.trace("initializing of new storage manager done");
    }
  }),

  finalize() {
    
    
    log.trace("StorageManager finalizing");
    return this._promiseInitialized.then(() => {
      return this._promiseStorageComplete;
    }).then(() => {
      this._promiseStorageComplete = null;
      this._promiseInitialized = null;
      this._clearCachedData();
      log.trace("StorageManager finalized");
    })
  },

  
  
  
  
  
  
  
  
  _queueStorageOperation(func) {
    
    
    let result = this._promiseStorageComplete.then(func);
    
    
    
    
    this._promiseStorageComplete = result.catch(err => {
      log.error("${func} failed: ${err}", {func, err});
    });
    return result;
  },

  
  getAccountData: Task.async(function* () {
    yield this._promiseInitialized;
    
    
    
    if (!('uid' in this.cachedPlain)) {
      return null;
    }
    let result = {};
    for (let [name, value] of Iterator(this.cachedPlain)) {
      result[name] = value;
    }
    
    yield this._maybeReadAndUpdateSecure();
    
    
    
    for (let [name, value] of Iterator(this.cachedSecure)) {
      result[name] = value;
    }
    return result;
  }),


  
  
  updateAccountData: Task.async(function* (newFields) {
    yield this._promiseInitialized;
    if (!('uid' in this.cachedPlain)) {
      
      
      throw new Error("No user is logged in");
    }
    if (!newFields || 'uid' in newFields || 'email' in newFields) {
      
      
      
      throw new Error("Can't change uid or email address");
    }
    log.debug("_updateAccountData with items", Object.keys(newFields));
    
    for (let [name, value] of Iterator(newFields)) {
      if (FXA_PWDMGR_PLAINTEXT_FIELDS.indexOf(name) >= 0) {
        if (value == null) {
          delete this.cachedPlain[name];
        } else {
          this.cachedPlain[name] = value;
        }
      } else {
        
        
        this.cachedSecure[name] = value;
      }
    }
    
    
    yield this._maybeReadAndUpdateSecure();
    
    
    
    this._write();
  }),

  _clearCachedData() {
    this.cachedPlain = {};
    
    
    this.cachedSecure = this.secureStorage == null ? this.cachedPlain : {};
  },

  










  _readPlainStorage: Task.async(function* () {
    let got;
    try {
      got = yield this.plainStorage.get();
    } catch(err) {
      
      
      if (!(err instanceof OS.File.Error) || !err.becauseNoSuchFile) {
        log.error("Failed to read plain storage", err);
      }
      
      got = null;
    }
    if (!got || !got.accountData || !got.accountData.uid ||
        got.version != DATA_FORMAT_VERSION) {
      return false;
    }
    
    
    
    
    
    if (Object.keys(this.cachedPlain).length != 0) {
      throw new Error("should be impossible to have cached data already.")
    }
    for (let [name, value] of Iterator(got.accountData)) {
      this.cachedPlain[name] = value;
    }
    return true;
  }),

  


  _maybeReadAndUpdateSecure: Task.async(function* () {
    if (this.secureStorage == null || !this._needToReadSecure) {
      return;
    }
    return this._queueStorageOperation(() => {
      if (this._needToReadSecure) { 
        return this._doReadAndUpdateSecure();
      }
    });
  }),

  



  _doReadAndUpdateSecure: Task.async(function* () {
    let { uid, email } = this.cachedPlain;
    try {
      log.debug("reading secure storage with existing", Object.keys(this.cachedSecure));
      
      
      
      let needWrite = Object.keys(this.cachedSecure).length != 0;
      let readSecure = yield this.secureStorage.get(uid, email);
      
      
      
      if (readSecure && readSecure.version != DATA_FORMAT_VERSION) {
        log.warn("got secure data but the data format version doesn't match");
        readSecure = null;
      }
      if (readSecure && readSecure.accountData) {
        log.debug("secure read fetched items", Object.keys(readSecure.accountData));
        for (let [name, value] of Iterator(readSecure.accountData)) {
          if (!(name in this.cachedSecure)) {
            this.cachedSecure[name] = value;
          }
        }
        if (needWrite) {
          log.debug("successfully read secure data; writing updated data back")
          yield this._doWriteSecure();
        }
      }
      this._needToReadSecure = false;
    } catch (ex if ex instanceof this.secureStorage.STORAGE_LOCKED) {
      log.debug("setAccountData: secure storage is locked trying to read");
    } catch (ex) {
      log.error("failed to read secure storage", ex);
      throw ex;
    }
  }),

  _write() {
    
    
    return this._queueStorageOperation(() => this.__write());
  },

  __write: Task.async(function* () {
    
    
    log.debug("writing plain storage", Object.keys(this.cachedPlain));
    let toWritePlain = {
      version: DATA_FORMAT_VERSION,
      accountData: this.cachedPlain,
    }
    yield this.plainStorage.set(toWritePlain);

    
    if (this.secureStorage == null) {
      return;
    }
    
    
    if (!this._needToReadSecure) {
      yield this._doWriteSecure();
    }
  }),

  


  _doWriteSecure: Task.async(function* () {
    
    for (let [name, value] of Iterator(this.cachedSecure)) {
      if (value == null) {
        delete this.cachedSecure[name];
      }
    }
    log.debug("writing secure storage", Object.keys(this.cachedSecure));
    let toWriteSecure = {
      version: DATA_FORMAT_VERSION,
      accountData: this.cachedSecure,
    }
    try {
      yield this.secureStorage.set(this.cachedPlain.email, toWriteSecure);
    } catch (ex if ex instanceof this.secureStorage.STORAGE_LOCKED) {
      
      
      
      log.error("setAccountData: secure storage is locked trying to write");
    }
  }),

  
  deleteAccountData() {
    return this._queueStorageOperation(() => this._deleteAccountData());
  },

  _deleteAccountData: Task.async(function() {
    log.debug("removing account data");
    yield this._promiseInitialized;
    yield this.plainStorage.set(null);
    if (this.secureStorage) {
      yield this.secureStorage.set(null);
    }
    this._clearCachedData();
    log.debug("account data reset");
  }),
}












function JSONStorage(options) {
  this.baseDir = options.baseDir;
  this.path = OS.Path.join(options.baseDir, options.filename);
};

JSONStorage.prototype = {
  set: function(contents) {
    log.trace("starting write of json user data", contents ? Object.keys(contents.accountData) : "null");
    let start = Date.now();
    return OS.File.makeDir(this.baseDir, {ignoreExisting: true})
      .then(CommonUtils.writeJSON.bind(null, contents, this.path))
      .then(result => {
        log.trace("finished write of json user data - took", Date.now()-start);
        return result;
      });
  },

  get: function() {
    log.trace("starting fetch of json user data");
    let start = Date.now();
    return CommonUtils.readJSON(this.path).then(result => {
      log.trace("finished fetch of json user data - took", Date.now()-start);
      return result;
    });
  },
};

function StorageLockedError() {
}







function LoginManagerStorage() {
}

LoginManagerStorage.prototype = {
  STORAGE_LOCKED: StorageLockedError,
  
  
  

  
  get _isLoggedIn() {
    return Services.logins.isLoggedIn;
  },

  
  
  
  _clearLoginMgrData: Task.async(function* () {
    try { 
      yield Services.logins.initializationPromise;
      if (!this._isLoggedIn) {
        return false;
      }
      let logins = Services.logins.findLogins({}, FXA_PWDMGR_HOST, null, FXA_PWDMGR_REALM);
      for (let login of logins) {
        Services.logins.removeLogin(login);
      }
      return true;
    } catch (ex) {
      log.error("Failed to clear login data: ${}", ex);
      return false;
    }
  }),

  set: Task.async(function* (email, contents) {
    if (!contents) {
      
      let cleared = yield this._clearLoginMgrData();
      if (!cleared) {
        
        
        log.info("not removing credentials from login manager - not logged in");
      }
      log.trace("storage set finished clearing account data");
      return;
    }

    
    log.trace("starting write of user data to the login manager");
    try { 
      
      yield Services.logins.initializationPromise;
      
      
      if (!this._isLoggedIn) {
        log.info("not saving credentials to login manager - not logged in");
        throw new this.STORAGE_LOCKED();
      }
      
      let loginInfo = new Components.Constructor(
         "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo, "init");
      let login = new loginInfo(FXA_PWDMGR_HOST,
                                null, 
                                FXA_PWDMGR_REALM, 
                                email, 
                                JSON.stringify(contents), 
                                "", 
                                "");

      let existingLogins = Services.logins.findLogins({}, FXA_PWDMGR_HOST, null,
                                                      FXA_PWDMGR_REALM);
      if (existingLogins.length) {
        Services.logins.modifyLogin(existingLogins[0], login);
      } else {
        Services.logins.addLogin(login);
      }
      log.trace("finished write of user data to the login manager");
    } catch (ex if ex instanceof this.STORAGE_LOCKED) {
      throw ex;
    } catch (ex) {
      
      
      log.error("Failed to save data to the login manager", ex);
    }
  }),

  get: Task.async(function* (uid, email) {
    log.trace("starting fetch of user data from the login manager");

    try { 
      
      yield Services.logins.initializationPromise;

      if (!this._isLoggedIn) {
        log.info("returning partial account data as the login manager is locked.");
        throw new this.STORAGE_LOCKED();
      }

      let logins = Services.logins.findLogins({}, FXA_PWDMGR_HOST, null, FXA_PWDMGR_REALM);
      if (logins.length == 0) {
        
        log.info("Can't find any credentials in the login manager");
        return null;
      }
      let login = logins[0];
      
      
      
      if (login.username == uid || login.username == email) {
        return JSON.parse(login.password);
      }
      log.info("username in the login manager doesn't match - ignoring it");
      yield this._clearLoginMgrData();
    } catch (ex if ex instanceof this.STORAGE_LOCKED) {
      throw ex;
    } catch (ex) {
      
      
      log.error("Failed to get data from the login manager", ex);
    }
    return null;
  }),
}





let haveLoginManager =
#if defined(MOZ_B2G)
                       false;
#else
                       true;
#endif
