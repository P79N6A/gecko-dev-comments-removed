




this.EXPORTED_SYMBOLS = ["fxAccounts", "FxAccounts"];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/osfile.jsm")
Cu.import("resource://services-common/utils.js");

const defaultBaseDir = OS.Path.join(OS.Constants.Path.profileDir);
const defaultStorageOptions = {
  filename: 'signedInUser.json',
  baseDir: defaultBaseDir,
};








function FxAccounts(signedInUserStorage = new JSONStorage(defaultStorageOptions)) {
  this._signedInUserStorage = signedInUserStorage;
}

FxAccounts.prototype = Object.freeze({
  
  version: 1,

  


















  setSignedInUser: function setSignedInUser(credentials) {
    let record = { version: this.version, accountData: credentials };
    
    this._signedInUser = JSON.parse(JSON.stringify(record));

    return this._signedInUserStorage.set(record);
  },

  


















  getSignedInUser: function getSignedInUser() {
    
    if (typeof this._signedInUser !== 'undefined') {
      let deferred = Promise.defer();
      let result = this._signedInUser ? this._signedInUser.accountData : undefined;
      deferred.resolve(result);
      return deferred.promise;
    }

    return this._signedInUserStorage.get()
      .then((user) => {
          if (user.version === this.version) {
            this._signedInUser = user;
            return user.accountData;
          }
        },
        (err) => undefined);
  },

  





  signOut: function signOut() {
    this._signedInUser = {};
    return this._signedInUserStorage.set(null);
  },
});














function JSONStorage(options) {
  this.baseDir = options.baseDir;
  this.path = OS.Path.join(options.baseDir, options.filename);
}

JSONStorage.prototype = Object.freeze({
  set: function (contents) {
    return OS.File.makeDir(this.baseDir, {ignoreExisting: true})
      .then(CommonUtils.writeJSON.bind(null, contents, this.path));
  },

  get: function () {
    return CommonUtils.readJSON(this.path);
  },
});



this.fxAccounts = new FxAccounts();

