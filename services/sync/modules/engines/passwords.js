




































const EXPORTED_SYMBOLS = ['PasswordEngine'];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/async.js");
Cu.import("resource://weave/ext/Observers.js");
Cu.import("resource://weave/type_records/passwords.js");

Function.prototype.async = Async.sugar;

function PasswordEngine() {
  this._init();
}
PasswordEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "passwords",
  displayName: "Passwords",
  logName: "Passwords",
  _storeObj: PasswordStore,
  _trackerObj: PasswordTracker,
  _recordObj: LoginRec,
  
  
  _syncFinish: function PasswordEngine__syncFinish() {
    let self = yield;
    this._store._clearLoginCache();
    yield SyncEngine.prototype._syncFinish.async(this, self.cb);
  }
};

function PasswordStore() {
  this._init();
}
PasswordStore.prototype = {
  __proto__: Store.prototype,
  _logName: "PasswordStore",

  get _loginManager() {
    let loginManager = Utils.getLoginManager();
    this.__defineGetter__("_loginManager", function() loginManager);
    return loginManager;
  },
  
  __loginItems: null,
  get _loginItems() {
    if (!this.__loginItems) {
      this.__loginItems = {};
      let logins = this._loginManager.getAllLogins({});
      for (let i = 0; i < logins.length; i++) {
        let metaInfo = logins[i].QueryInterface(Ci.nsILoginMetaInfo);
        this.__loginItems[metaInfo.guid] = logins[i];
      }
    }

    return this.__loginItems;
  },
  
  _nsLoginInfo: null,
  _init: function PasswordStore_init() {
    Store.prototype._init.call(this);
    this._nsLoginInfo = new Components.Constructor(
      "@mozilla.org/login-manager/loginInfo;1",
      Ci.nsILoginInfo,
      "init"
    );   
  },
  
  _clearLoginCache: function PasswordStore__clearLoginCache() {
    this.__loginItems = null;
  },
  
  _nsLoginInfoFromRecord: function PasswordStore__nsLoginInfoRec(record) {
    return new this._nsLoginInfo(record.hostname,
                                  record.formSubmitURL,
                                  record.httpRealm,
                                  record.username,
                                  record.password,
                                  record.usernameField,
                                  record.passwordField);
  },

  getAllIDs: function PasswordStore__getAllIDs() {
    let items = {};
    let logins = this._loginManager.getAllLogins({});
    
    for (let i = 0; i < logins.length; i++) {
      let metaInfo = logins[i].QueryInterface(Ci.nsILoginMetaInfo);
      items[metaInfo.guid] = logins[i].hostname;
    }
    
    return items;
  },

  changeItemID: function PasswordStore__changeItemID(oldID, newID) {
    if (!(oldID in this._loginItems)) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                     newID + ": Item does not exist");
      return;
    }
    let info = this._loginItems[oldID];
    
    if (newID in this._loginItems) {
      this._log.warn("Can't change GUID " + oldID + " to " +
                     newID + ": new ID already in use");
      return;
    }
    
    this._log.debug("Changing GUID " + oldID + " to " + newID);

    let prop = Cc["@mozilla.org/hash-property-bag;1"].
               createInstance(Ci.nsIWritablePropertyBag2);
    prop.setPropertyAsAUTF8String("guid", newID);

    this._loginManager.modifyLogin(info, prop);
  },

  itemExists: function PasswordStore__itemExists(id) {
    return ((id in this._loginItems) == true);
  },

  createRecord: function PasswordStore__createRecord(guid, cryptoMetaURL) {
    let record = new LoginRec();
    record.id = guid;
    if (guid in this._loginItems) {
      let login = this._loginItems[guid];
      record.encryption = cryptoMetaURL;
      record.hostname = login.hostname;
      record.formSubmitURL = login.formSubmitURL;
      record.httpRealm = login.httpRealm;
      record.username = login.username;
      record.password = login.password;
      record.usernameField = login.usernameField;
      record.passwordField = login.passwordField;
    } else {
      
      record.cleartext = null;
    }
    return record;
  },

  create: function PasswordStore__create(record) {
    this._loginManager.addLogin(this._nsLoginInfoFromRecord(record));
  },

  remove: function PasswordStore__remove(record) {
    if (record.id in this._loginItems) {
      this._loginManager.removeLogin(this._loginItems[record.id]);
      return;
    }
    
    this._log.debug("Asked to remove record that doesn't exist, ignoring!");
  },

  update: function PasswordStore__update(record) {
    if (!(record.id in this._loginItems)) {
      this._log.debug("Skipping update for unknown item: " + record.id);
      return;
    }
    let login = this._loginItems[record.id];
    this._log.trace("Updating " + record.id + " (" + itemId + ")");

    let newinfo = this._nsLoginInfoFromRecord(record);
    this._loginManager.modifyLogin(login, newinfo);
  },

  wipe: function PasswordStore_wipe() {
    this._loginManager.removeAllLogins();
  }
};

function PasswordTracker() {
  this._init();
}
PasswordTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "PasswordTracker",

  _init: function PasswordTracker_init() {
    Tracker.prototype._init.call(this);
    Observers.add("passwordmgr-storage-changed", this);
  },

  
  observe: function PasswordTracker_observe(aSubject, aTopic, aData) {
    if (this.ignoreAll)
      return;

    this._log.debug("Received notification " + aData);

    switch (aData) {
    case 'addLogin':
    case 'modifyLogin':
    case 'removeLogin':
      let metaInfo = aSubject.QueryInterface(Ci.nsILoginMetaInfo);
      this._score += 15;
      this.addChangedID(metaInfo.guid);
      break;
    case 'removeAllLogins':
      this._score += 50;
      break;
    }
  }
};
