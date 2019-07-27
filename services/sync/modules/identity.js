



"use strict";

this.EXPORTED_SYMBOLS = ["IdentityManager"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/util.js");


for (let symbol of ["BulkKeyBundle", "SyncKeyBundle"]) {
  XPCOMUtils.defineLazyModuleGetter(this, symbol,
                                    "resource://services-sync/keys.js",
                                    symbol);
}






































this.IdentityManager = function IdentityManager() {
  this._log = Log.repository.getLogger("Sync.Identity");
  this._log.Level = Log.Level[Svc.Prefs.get("log.logger.identity")];

  this._basicPassword = null;
  this._basicPasswordAllowLookup = true;
  this._basicPasswordUpdated = false;
  this._syncKey = null;
  this._syncKeyAllowLookup = true;
  this._syncKeySet = false;
  this._syncKeyBundle = null;
}
IdentityManager.prototype = {
  _log: null,

  _basicPassword: null,
  _basicPasswordAllowLookup: true,
  _basicPasswordUpdated: false,

  _syncKey: null,
  _syncKeyAllowLookup: true,
  _syncKeySet: false,

  _syncKeyBundle: null,

  




  initialize: function() {
    
    return Promise.resolve();
  },

  finalize: function() {
    
    return Promise.resolve();
  },

  


  logout: function() {
    
  },

  



  ensureLoggedIn: function() {
    
    return Promise.resolve();
  },

  


  get readyToAuthenticate() {
    
    return true;
  },

  get account() {
    return Svc.Prefs.get("account", this.username);
  },

  











  set account(value) {
    if (value) {
      value = value.toLowerCase();
      Svc.Prefs.set("account", value);
    } else {
      Svc.Prefs.reset("account");
    }

    this.username = this.usernameFromAccount(value);
  },

  get username() {
    return Svc.Prefs.get("username", null);
  },

  




  set username(value) {
    if (value) {
      value = value.toLowerCase();

      if (value == this.username) {
        return;
      }

      Svc.Prefs.set("username", value);
    } else {
      Svc.Prefs.reset("username");
    }

    
    
    this._log.info("Username changed. Removing stored credentials.");
    this.resetCredentials();
  },

  


  resetCredentials: function() {
    this.basicPassword = null;
    this.resetSyncKey();
  },

  


  resetSyncKey: function() {
    this.syncKey = null;
    
  },

  




  get basicPassword() {
    if (this._basicPasswordAllowLookup) {
      
      let username = this.username;
      if (!username) {
        return null;
      }

      for each (let login in this._getLogins(PWDMGR_PASSWORD_REALM)) {
        if (login.username.toLowerCase() == username) {
          
          this._basicPassword = Utils.encodeUTF8(login.password);
        }
      }

      this._basicPasswordAllowLookup = false;
    }

    return this._basicPassword;
  },

  




  set basicPassword(value) {
    
    if (!value) {
      this._log.info("Basic password has no value. Removing.");
      this._basicPassword = null;
      this._basicPasswordUpdated = true;
      this._basicPasswordAllowLookup = false;
      return;
    }

    let username = this.username;
    if (!username) {
      throw new Error("basicPassword cannot be set before username.");
    }

    this._log.info("Basic password being updated.");
    this._basicPassword = Utils.encodeUTF8(value);
    this._basicPasswordUpdated = true;
  },

  








  get syncKey() {
    if (this._syncKeyAllowLookup) {
      let username = this.username;
      if (!username) {
        return null;
      }

      for each (let login in this._getLogins(PWDMGR_PASSPHRASE_REALM)) {
        if (login.username.toLowerCase() == username) {
          this._syncKey = login.password;
        }
      }

      this._syncKeyAllowLookup = false;
    }

    return this._syncKey;
  },

  















  set syncKey(value) {
    if (!value) {
      this._log.info("Sync Key has no value. Deleting.");
      this._syncKey = null;
      this._syncKeyBundle = null;
      this._syncKeyUpdated = true;
      return;
    }

    if (!this.username) {
      throw new Error("syncKey cannot be set before username.");
    }

    this._log.info("Sync Key being updated.");
    this._syncKey = value;

    
    this._syncKeyBundle = null;
    let bundle = this.syncKeyBundle;

    this._syncKeyUpdated = true;
  },

  








  get syncKeyBundle() {
    
    if (!this.username) {
      this._log.warn("Attempted to obtain Sync Key Bundle with no username set!");
      return null;
    }

    if (!this.syncKey) {
      this._log.warn("Attempted to obtain Sync Key Bundle with no Sync Key " +
                     "set!");
      return null;
    }

    if (!this._syncKeyBundle) {
      try {
        this._syncKeyBundle = new SyncKeyBundle(this.username, this.syncKey);
      } catch (ex) {
        this._log.warn(Utils.exceptionStr(ex));
        return null;
      }
    }

    return this._syncKeyBundle;
  },

  





  get currentAuthState() {
    if (!this.username) {
      return LOGIN_FAILED_NO_USERNAME;
    }

    if (Utils.mpLocked()) {
      return STATUS_OK;
    }

    if (!this.basicPassword) {
      return LOGIN_FAILED_NO_PASSWORD;
    }

    if (!this.syncKey) {
      return LOGIN_FAILED_NO_PASSPHRASE;
    }

    
    
    if (!this.syncKeyBundle) {
      return LOGIN_FAILED_INVALID_PASSPHRASE;
    }

    return STATUS_OK;
  },

  





  unlockAndVerifyAuthState: function() {
    
    
    try {
      this.syncKey;
    } catch (ex) {
      this._log.debug("Fetching passphrase threw " + ex +
                      "; assuming master password locked.");
      return Promise.resolve(MASTER_PASSWORD_LOCKED);
    }
    return Promise.resolve(STATUS_OK);
  },

  








  persistCredentials: function persistCredentials(force) {
    if (this._basicPasswordUpdated || force) {
      if (this._basicPassword) {
        this._setLogin(PWDMGR_PASSWORD_REALM, this.username,
                       this._basicPassword);
      } else {
        for each (let login in this._getLogins(PWDMGR_PASSWORD_REALM)) {
          Services.logins.removeLogin(login);
        }
      }

      this._basicPasswordUpdated = false;
    }

    if (this._syncKeyUpdated || force) {
      if (this._syncKey) {
        this._setLogin(PWDMGR_PASSPHRASE_REALM, this.username, this._syncKey);
      } else {
        for each (let login in this._getLogins(PWDMGR_PASSPHRASE_REALM)) {
          Services.logins.removeLogin(login);
        }
      }

      this._syncKeyUpdated = false;
    }

  },

  


  deleteSyncKey: function deleteSyncKey() {
    this.syncKey = null;
    this.persistCredentials();
  },

  hasBasicCredentials: function hasBasicCredentials() {
    
    return this.username && this.basicPassword && true;
  },

  


  _getLogins: function _getLogins(realm) {
    return Services.logins.findLogins({}, PWDMGR_HOST, null, realm);
  },

  





  _setLogin: function _setLogin(realm, username, password) {
    let exists = false;
    for each (let login in this._getLogins(realm)) {
      if (login.username == username && login.password == password) {
        exists = true;
      } else {
        this._log.debug("Pruning old login for " + username + " from " + realm);
        Services.logins.removeLogin(login);
      }
    }

    if (exists) {
      return;
    }

    this._log.debug("Updating saved password for " + username + " in " +
                    realm);

    let loginInfo = new Components.Constructor(
      "@mozilla.org/login-manager/loginInfo;1", Ci.nsILoginInfo, "init");
    let login = new loginInfo(PWDMGR_HOST, null, realm, username,
                                password, "", "");
    Services.logins.addLogin(login);
  },

  


  deleteSyncCredentials: function deleteSyncCredentials() {
    for (let host of Utils.getSyncCredentialsHosts()) {
      let logins = Services.logins.findLogins({}, host, "", "");
      for each (let login in logins) {
        Services.logins.removeLogin(login);
      }
    }

    
    this._basicPassword = null;
    this._basicPasswordAllowLookup = true;
    this._basicPasswordUpdated = false;

    this._syncKey = null;
    
    this._syncKeyAllowLookup = true;
    this._syncKeyUpdated = false;
  },

  usernameFromAccount: function usernameFromAccount(value) {
    
    
    if (value && value.match(/[^A-Z0-9._-]/i)) {
      return Utils.sha1Base32(value.toLowerCase()).toLowerCase();
    }

    return value ? value.toLowerCase() : value;
  },

  


  getResourceAuthenticator: function getResourceAuthenticator() {
    if (this.hasBasicCredentials()) {
      return this._onResourceRequestBasic.bind(this);
    }

    return null;
  },

  


  getBasicResourceAuthenticator:
    function getBasicResourceAuthenticator(username, password) {

    return function basicAuthenticator(resource) {
      let value = "Basic " + btoa(username + ":" + password);
      return {headers: {authorization: value}};
    };
  },

  _onResourceRequestBasic: function _onResourceRequestBasic(resource) {
    let value = "Basic " + btoa(this.username + ":" + this.basicPassword);
    return {headers: {authorization: value}};
  },

  _onResourceRequestMAC: function _onResourceRequestMAC(resource, method) {
    
    let identifier;
    let key;
    let result = Utils.computeHTTPMACSHA1(identifier, key, method, resource.uri);

    return {headers: {authorization: result.header}};
  },

  


  getRESTRequestAuthenticator: function getRESTRequestAuthenticator() {
    if (this.hasBasicCredentials()) {
      return this.onRESTRequestBasic.bind(this);
    }

    return null;
  },

  onRESTRequestBasic: function onRESTRequestBasic(request) {
    let up = this.username + ":" + this.basicPassword;
    request.setHeader("authorization", "Basic " + btoa(up));
  },

  createClusterManager: function(service) {
    Cu.import("resource://services-sync/stages/cluster.js");
    return new ClusterManager(service);
  },

  offerSyncOptions: function () {
    
    return {accepted: true};
  },
};
