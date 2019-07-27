










"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginImport",
                                  "resource://gre/modules/LoginImport.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LoginStore",
                                  "resource://gre/modules/LoginStore.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");




this.LoginManagerStorage_json = function () {};

this.LoginManagerStorage_json.prototype = {
  classID : Components.ID("{c00c432d-a0c9-46d7-bef6-9c45b4d07341}"),
  QueryInterface : XPCOMUtils.generateQI([Ci.nsILoginManagerStorage]),

  __crypto : null,  
  get _crypto() {
    if (!this.__crypto)
      this.__crypto = Cc["@mozilla.org/login-manager/crypto/SDR;1"].
                      getService(Ci.nsILoginManagerCrypto);
    return this.__crypto;
  },

  initialize : function () {
    try {
      
      
      this._crypto;

      
      let jsonPath = OS.Path.join(OS.Constants.Path.profileDir,
                                  "logins.json");
      this._store = new LoginStore(jsonPath);

      return Task.spawn(function () {
        
        this.log("Opening database at " + this._store.path);
        yield this._store.load();

        
        
        
        
        try {
          if (Services.prefs.getBoolPref("signon.importedFromSqlite")) {
            return;
          }
        } catch (ex) {
          
        }

        
        let sqlitePath = OS.Path.join(OS.Constants.Path.profileDir,
                                      "signons.sqlite");
        if (yield OS.File.exists(sqlitePath)) {
          let loginImport = new LoginImport(this._store, sqlitePath);
          
          
          
          
          yield loginImport.import().catch(Cu.reportError);
          this._store.saveSoon();
        }

        
        Services.prefs.setBoolPref("signon.importedFromSqlite", true);
      }.bind(this)).catch(Cu.reportError);
    } catch (e) {
      this.log("Initialization failed: " + e);
      throw new Error("Initialization failed");
    }
  },


  





  terminate : function () {
    this._store._saver.disarm();
    return this._store.save();
  },


  



  addLogin : function (login) {
    this._store.ensureDataReady();

    
    LoginHelper.checkLoginValues(login);

    let [encUsername, encPassword, encType] = this._encryptLogin(login);

    
    let loginClone = login.clone();

    
    loginClone.QueryInterface(Ci.nsILoginMetaInfo);
    if (loginClone.guid) {
      if (!this._isGuidUnique(loginClone.guid))
        throw new Error("specified GUID already exists");
    } else {
      loginClone.guid = gUUIDGenerator.generateUUID().toString();
    }

    
    let currentTime = Date.now();
    if (!loginClone.timeCreated)
      loginClone.timeCreated = currentTime;
    if (!loginClone.timeLastUsed)
      loginClone.timeLastUsed = currentTime;
    if (!loginClone.timePasswordChanged)
      loginClone.timePasswordChanged = currentTime;
    if (!loginClone.timesUsed)
      loginClone.timesUsed = 1;

    this._store.data.logins.push({
      id:                  this._store.data.nextId++,
      hostname:            loginClone.hostname,
      httpRealm:           loginClone.httpRealm,
      formSubmitURL:       loginClone.formSubmitURL,
      usernameField:       loginClone.usernameField,
      passwordField:       loginClone.passwordField,
      encryptedUsername:   encUsername,
      encryptedPassword:   encPassword,
      guid:                loginClone.guid,
      encType:             encType,
      timeCreated:         loginClone.timeCreated,
      timeLastUsed:        loginClone.timeLastUsed,
      timePasswordChanged: loginClone.timePasswordChanged,
      timesUsed:           loginClone.timesUsed
    });
    this._store.saveSoon();

    
    this._sendNotification("addLogin", loginClone);
  },


  



  removeLogin : function (login) {
    this._store.ensureDataReady();

    let [idToDelete, storedLogin] = this._getIdForLogin(login);
    if (!idToDelete)
      throw new Error("No matching logins");

    let foundIndex = this._store.data.logins.findIndex(l => l.id == idToDelete);
    if (foundIndex != -1) {
      this._store.data.logins.splice(foundIndex, 1);
      this._store.saveSoon();
    }

    this._sendNotification("removeLogin", storedLogin);
  },


  



  modifyLogin : function (oldLogin, newLoginData) {
    this._store.ensureDataReady();

    let [idToModify, oldStoredLogin] = this._getIdForLogin(oldLogin);
    if (!idToModify)
      throw new Error("No matching logins");

    let newLogin = LoginHelper.buildModifiedLogin(oldStoredLogin, newLoginData);

    
    if (newLogin.guid != oldStoredLogin.guid &&
        !this._isGuidUnique(newLogin.guid))
    {
      throw new Error("specified GUID already exists");
    }

    
    if (!newLogin.matches(oldLogin, true)) {
      let logins = this.findLogins({}, newLogin.hostname,
                                   newLogin.formSubmitURL,
                                   newLogin.httpRealm);

      if (logins.some(login => newLogin.matches(login, true)))
        throw new Error("This login already exists.");
    }

    
    let [encUsername, encPassword, encType] = this._encryptLogin(newLogin);

    for (let loginItem of this._store.data.logins) {
      if (loginItem.id == idToModify) {
        loginItem.hostname = newLogin.hostname;
        loginItem.httpRealm = newLogin.httpRealm;
        loginItem.formSubmitURL = newLogin.formSubmitURL;
        loginItem.usernameField = newLogin.usernameField;
        loginItem.passwordField = newLogin.passwordField;
        loginItem.encryptedUsername = encUsername;
        loginItem.encryptedPassword = encPassword;
        loginItem.guid = newLogin.guid;
        loginItem.encType = encType;
        loginItem.timeCreated = newLogin.timeCreated;
        loginItem.timeLastUsed = newLogin.timeLastUsed;
        loginItem.timePasswordChanged = newLogin.timePasswordChanged;
        loginItem.timesUsed = newLogin.timesUsed;
        this._store.saveSoon();
        break;
      }
    }

    this._sendNotification("modifyLogin", [oldStoredLogin, newLogin]);
  },


  




  getAllLogins : function (count) {
    let [logins, ids] = this._searchLogins({});

    
    logins = this._decryptLogins(logins);

    this.log("_getAllLogins: returning " + logins.length + " logins.");
    if (count)
      count.value = logins.length; 
    return logins;
  },


  







  searchLogins : function(count, matchData) {
    let realMatchData = {};
    
    let propEnum = matchData.enumerator;
    while (propEnum.hasMoreElements()) {
      let prop = propEnum.getNext().QueryInterface(Ci.nsIProperty);
      realMatchData[prop.name] = prop.value;
    }

    let [logins, ids] = this._searchLogins(realMatchData);

    
    logins = this._decryptLogins(logins);

    count.value = logins.length; 
    return logins;
  },


  









  _searchLogins : function (matchData) {
    this._store.ensureDataReady();

    let conditions = [];

    function match(aLogin) {
      for (let field in matchData) {
        let value = matchData[field];
        switch (field) {
          
          case "formSubmitURL":
            if (value != null) {
              if (aLogin.formSubmitURL != "" && aLogin.formSubmitURL != value) {
                return false;
              }
              break;
            }
          
          case "hostname":
          case "httpRealm":
          case "id":
          case "usernameField":
          case "passwordField":
          case "encryptedUsername":
          case "encryptedPassword":
          case "guid":
          case "encType":
          case "timeCreated":
          case "timeLastUsed":
          case "timePasswordChanged":
          case "timesUsed":
            if (value == null && aLogin[field]) {
              return false;
            } else if (aLogin[field] != value) {
              return false;
            }
            break;
          
          default:
            throw new Error("Unexpected field: " + field);
        }
      }
      return true;
    }

    let foundLogins = [], foundIds = [];
    for (let loginItem of this._store.data.logins) {
      if (match(loginItem)) {
        
        let login = Cc["@mozilla.org/login-manager/loginInfo;1"].
                    createInstance(Ci.nsILoginInfo);
        login.init(loginItem.hostname, loginItem.formSubmitURL,
                   loginItem.httpRealm, loginItem.encryptedUsername,
                   loginItem.encryptedPassword, loginItem.usernameField,
                   loginItem.passwordField);
        
        login.QueryInterface(Ci.nsILoginMetaInfo);
        login.guid = loginItem.guid;
        login.timeCreated = loginItem.timeCreated;
        login.timeLastUsed = loginItem.timeLastUsed;
        login.timePasswordChanged = loginItem.timePasswordChanged;
        login.timesUsed = loginItem.timesUsed;
        foundLogins.push(login);
        foundIds.push(loginItem.id);
      }
    }

    this.log("_searchLogins: returning " + foundLogins.length + " logins");
    return [foundLogins, foundIds];
  },

  






  removeAllLogins : function () {
    this._store.ensureDataReady();

    this.log("Removing all logins");
    this._store.data.logins = [];
    this._store.saveSoon();

    this._sendNotification("removeAllLogins", null);
  },


  



  getAllDisabledHosts : function (count) {
    this._store.ensureDataReady();

    let disabledHosts = this._store.data.disabledHosts.slice(0);

    this.log("_getAllDisabledHosts: returning " + disabledHosts.length + " disabled hosts.");
    if (count)
      count.value = disabledHosts.length; 
    return disabledHosts;
  },


  



  getLoginSavingEnabled : function (hostname) {
    this._store.ensureDataReady();

    this.log("Getting login saving is enabled for " + hostname);
    return this._store.data.disabledHosts.indexOf(hostname) == -1;
  },


  



  setLoginSavingEnabled : function (hostname, enabled) {
    this._store.ensureDataReady();

    
    LoginHelper.checkHostnameValue(hostname);

    this.log("Setting login saving enabled for " + hostname + " to " + enabled);
    let foundIndex = this._store.data.disabledHosts.indexOf(hostname);
    if (enabled) {
      if (foundIndex != -1) {
        this._store.data.disabledHosts.splice(foundIndex, 1);
        this._store.saveSoon();
      }
    } else {
      if (foundIndex == -1) {
        this._store.data.disabledHosts.push(hostname);
        this._store.saveSoon();
      }
    }

    this._sendNotification(enabled ? "hostSavingEnabled" : "hostSavingDisabled", hostname);
  },


  



  findLogins : function (count, hostname, formSubmitURL, httpRealm) {
    let loginData = {
      hostname: hostname,
      formSubmitURL: formSubmitURL,
      httpRealm: httpRealm
    };
    let matchData = { };
    for each (let field in ["hostname", "formSubmitURL", "httpRealm"])
      if (loginData[field] != '')
        matchData[field] = loginData[field];
    let [logins, ids] = this._searchLogins(matchData);

    
    logins = this._decryptLogins(logins);

    this.log("_findLogins: returning " + logins.length + " logins");
    count.value = logins.length; 
    return logins;
  },


  



  countLogins : function (hostname, formSubmitURL, httpRealm) {
    let count = {};
    let loginData = {
      hostname: hostname,
      formSubmitURL: formSubmitURL,
      httpRealm: httpRealm
    };
    let matchData = { };
    for each (let field in ["hostname", "formSubmitURL", "httpRealm"])
      if (loginData[field] != '')
        matchData[field] = loginData[field];
    let [logins, ids] = this._searchLogins(matchData);

    this.log("_countLogins: counted logins: " + logins.length);
    return logins.length;
  },


  


  get uiBusy() {
    return this._crypto.uiBusy;
  },


  


  get isLoggedIn() {
    return this._crypto.isLoggedIn;
  },


  




  _sendNotification : function (changeType, data) {
    let dataObject = data;
    
    if (data instanceof Array) {
      dataObject = Cc["@mozilla.org/array;1"].
                   createInstance(Ci.nsIMutableArray);
      for (let i = 0; i < data.length; i++)
        dataObject.appendElement(data[i], false);
    } else if (typeof(data) == "string") {
      dataObject = Cc["@mozilla.org/supports-string;1"].
                   createInstance(Ci.nsISupportsString);
      dataObject.data = data;
    }
    Services.obs.notifyObservers(dataObject, "passwordmgr-storage-changed", changeType);
  },


  






  _getIdForLogin : function (login) {
    let matchData = { };
    for each (let field in ["hostname", "formSubmitURL", "httpRealm"])
      if (login[field] != '')
        matchData[field] = login[field];
    let [logins, ids] = this._searchLogins(matchData);

    let id = null;
    let foundLogin = null;

    
    
    
    
    for (let i = 0; i < logins.length; i++) {
      let [decryptedLogin] = this._decryptLogins([logins[i]]);

      if (!decryptedLogin || !decryptedLogin.equals(login))
        continue;

      
      foundLogin = decryptedLogin;
      id = ids[i];
      break;
    }

    return [id, foundLogin];
  },


  




  _isGuidUnique : function (guid) {
    this._store.ensureDataReady();

    return this._store.data.logins.every(l => l.guid != guid);
  },


  





  _encryptLogin : function (login) {
    let encUsername = this._crypto.encrypt(login.username);
    let encPassword = this._crypto.encrypt(login.password);
    let encType     = this._crypto.defaultEncType;

    return [encUsername, encPassword, encType];
  },


  












  _decryptLogins : function (logins) {
    let result = [];

    for each (let login in logins) {
      try {
        login.username = this._crypto.decrypt(login.username);
        login.password = this._crypto.decrypt(login.password);
      } catch (e) {
        
        
        if (e.result == Cr.NS_ERROR_FAILURE)
          continue;
        throw e;
      }
      result.push(login);
    }

    return result;
  },
};

XPCOMUtils.defineLazyGetter(this.LoginManagerStorage_json.prototype, "log", () => {
  let logger = LoginHelper.createLogger("Login storage");
  return logger.log.bind(logger);
});

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([LoginManagerStorage_json]);
