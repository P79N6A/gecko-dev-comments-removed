




































const EXPORTED_SYMBOLS = ['PasswordEngine'];

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/ext/Observers.js");
Cu.import("resource://weave/type_records/passwords.js");

function PasswordEngine() {
  this._init();
}
PasswordEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "passwords",
  displayName: "Passwords",
  description: "Forget all your passwords, Weave will remember them for you",
  logName: "Passwords",
  _storeObj: PasswordStore,
  _trackerObj: PasswordTracker,
  _recordObj: LoginRec,

  _findDupe: function _findDupe(item) {
    let login = this._store._nsLoginInfoFromRecord(item);
    let logins = Svc.Login.findLogins({}, login.hostname, login.formSubmitURL,
      login.httpRealm);

    
    for each (let local in logins)
      if (login.matches(local, true) && local instanceof Ci.nsILoginMetaInfo)
        return local.guid;
  }
};

function PasswordStore() {
  this._init();
}
PasswordStore.prototype = {
  __proto__: Store.prototype,
  name: "passwords",
  _logName: "PasswordStore",

  _nsLoginInfo: null,
  _init: function PasswordStore_init() {
    Store.prototype._init.call(this);
    this._nsLoginInfo = new Components.Constructor(
      "@mozilla.org/login-manager/loginInfo;1",
      Ci.nsILoginInfo,
      "init"
    );
  },

  _nsLoginInfoFromRecord: function PasswordStore__nsLoginInfoRec(record) {
    let info = new this._nsLoginInfo(record.hostname,
                                     record.formSubmitURL,
                                     record.httpRealm,
                                     record.username,
                                     record.password,
                                     record.usernameField,
                                     record.passwordField);
    info.QueryInterface(Ci.nsILoginMetaInfo);
    info.guid = record.id;
    return info;
  },

  _getLoginFromGUID: function PasswordStore__getLoginFromGUID(id) {
    let prop = Cc["@mozilla.org/hash-property-bag;1"].
      createInstance(Ci.nsIWritablePropertyBag2);
    prop.setPropertyAsAUTF8String("guid", id);
    
    let logins = Svc.Login.searchLogins({}, prop);
    if (logins.length > 0) {
      this._log.trace(logins.length + " items matching " + id + " found.");
      return logins[0];
    } else {
      this._log.trace("No items matching " + id + " found. Ignoring");
    }
    return false;
  },
  
  getAllIDs: function PasswordStore__getAllIDs() {
    let items = {};
    let logins = Svc.Login.getAllLogins({});

    for (let i = 0; i < logins.length; i++) {
      let metaInfo = logins[i].QueryInterface(Ci.nsILoginMetaInfo);
      items[metaInfo.guid] = metaInfo;
    }

    return items;
  },

  changeItemID: function PasswordStore__changeItemID(oldID, newID) {
    this._log.trace("Changing item ID: " + oldID + " to " + newID);

    let oldLogin = this._getLoginFromGUID(oldID);
    if (!oldLogin) {
      this._log.trace("Can't change item ID: item doesn't exist");
      return;
    }
    if (this._getLoginFromGUID(newID)) {
      this._log.trace("Can't change item ID: new ID already in use");
      return;
    }

    let prop = Cc["@mozilla.org/hash-property-bag;1"].
      createInstance(Ci.nsIWritablePropertyBag2);
    prop.setPropertyAsAUTF8String("guid", newID);

    Svc.Login.modifyLogin(oldLogin, prop);
  },

  itemExists: function PasswordStore__itemExists(id) {
    if (this._getLoginFromGUID(id))
      return true;
    return false;
  },

  createRecord: function PasswordStore__createRecord(guid, cryptoMetaURL) {
    let record = new LoginRec();
    let login = this._getLoginFromGUID(guid);

    record.id = guid;    
    if (login) {
      record.encryption = cryptoMetaURL;
      record.hostname = login.hostname;
      record.formSubmitURL = login.formSubmitURL;
      record.httpRealm = login.httpRealm;
      record.username = login.username;
      record.password = login.password;
      record.usernameField = login.usernameField;
      record.passwordField = login.passwordField;
    }
    else
      record.deleted = true;
    return record;
  },

  create: function PasswordStore__create(record) {
    this._log.debug("Adding login for " + record.hostname);
    Svc.Login.addLogin(this._nsLoginInfoFromRecord(record));
  },

  remove: function PasswordStore__remove(record) {
    this._log.trace("Removing login " + record.id);
    
    let loginItem = this._getLoginFromGUID(record.id);
    if (!loginItem) {
      this._log.trace("Asked to remove record that doesn't exist, ignoring");
      return;
    }

    Svc.Login.removeLogin(loginItem);
  },

  update: function PasswordStore__update(record) {
    let loginItem = this._getLoginFromGUID(record.id);
    if (!loginItem) {
      this._log.debug("Skipping update for unknown item: " + record.hostname);
      return;
    }

    this._log.debug("Updating " + record.hostname);
    let newinfo = this._nsLoginInfoFromRecord(record);
    Svc.Login.modifyLogin(loginItem, newinfo);
  },

  wipe: function PasswordStore_wipe() {
    Svc.Login.removeAllLogins();
  }
};

function PasswordTracker() {
  this._init();
}
PasswordTracker.prototype = {
  __proto__: Tracker.prototype,
  _logName: "PasswordTracker",
  name: "passwords",
  file: "password",

  _init: function PasswordTracker_init() {
    Tracker.prototype._init.call(this);
    Observers.add("passwordmgr-storage-changed", this);
  },

  
  observe: function PasswordTracker_observe(aSubject, aTopic, aData) {
    if (this.ignoreAll)
      return;

    switch (aData) {
    case 'modifyLogin':
      aSubject = aSubject.QueryInterface(Ci.nsIArray).
        queryElementAt(1, Ci.nsILoginMetaInfo);
    case 'addLogin':
    case 'removeLogin':
      aSubject.QueryInterface(Ci.nsILoginMetaInfo);
      this._score += 15;
      this._log.trace(aData + ": " + aSubject.guid);
      this.addChangedID(aSubject.guid);
      break;
    case 'removeAllLogins':
      this._log.trace(aData);
      this._score += 50;
      break;
    }
  }
};
