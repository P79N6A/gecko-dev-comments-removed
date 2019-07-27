



this.EXPORTED_SYMBOLS = ["XPCOMUtils", "Services", "Utils", "Async", "Svc", "Str"];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-common/stringbundle.js");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/async.js", this);
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/osfile.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);


XPCOMUtils.defineLazyGetter(this, "FxAccountsCommon", function() {
  let FxAccountsCommon = {};
  Cu.import("resource://gre/modules/FxAccountsCommon.js", FxAccountsCommon);
  return FxAccountsCommon;
});





this.Utils = {
  
  
  nextTick: CommonUtils.nextTick,
  namedTimer: CommonUtils.namedTimer,
  exceptionStr: CommonUtils.exceptionStr,
  stackTrace: CommonUtils.stackTrace,
  makeURI: CommonUtils.makeURI,
  encodeUTF8: CommonUtils.encodeUTF8,
  decodeUTF8: CommonUtils.decodeUTF8,
  safeAtoB: CommonUtils.safeAtoB,
  byteArrayToString: CommonUtils.byteArrayToString,
  bytesAsHex: CommonUtils.bytesAsHex,
  hexToBytes: CommonUtils.hexToBytes,
  encodeBase32: CommonUtils.encodeBase32,
  decodeBase32: CommonUtils.decodeBase32,

  
  generateRandomBytes: CryptoUtils.generateRandomBytes,
  computeHTTPMACSHA1: CryptoUtils.computeHTTPMACSHA1,
  digestUTF8: CryptoUtils.digestUTF8,
  digestBytes: CryptoUtils.digestBytes,
  sha1: CryptoUtils.sha1,
  sha1Base32: CryptoUtils.sha1Base32,
  makeHMACKey: CryptoUtils.makeHMACKey,
  makeHMACHasher: CryptoUtils.makeHMACHasher,
  hkdfExpand: CryptoUtils.hkdfExpand,
  pbkdf2Generate: CryptoUtils.pbkdf2Generate,
  deriveKeyFromPassphrase: CryptoUtils.deriveKeyFromPassphrase,
  getHTTPMACSHA1Header: CryptoUtils.getHTTPMACSHA1Header,

  








  catch: function Utils_catch(func, exceptionCallback) {
    let thisArg = this;
    return function WrappedCatch() {
      try {
        return func.call(thisArg);
      }
      catch(ex) {
        thisArg._log.debug("Exception: " + Utils.exceptionStr(ex));
        if (exceptionCallback) {
          return exceptionCallback.call(thisArg, ex);
        }
        return null;
      }
    };
  },

  





  lock: function lock(label, func) {
    let thisArg = this;
    return function WrappedLock() {
      if (!thisArg.lock()) {
        throw "Could not acquire lock. Label: \"" + label + "\".";
      }

      try {
        return func.call(thisArg);
      }
      finally {
        thisArg.unlock();
      }
    };
  },

  isLockException: function isLockException(ex) {
    return ex && ex.indexOf && ex.indexOf("Could not acquire lock.") == 0;
  },

  




















  notify: function Utils_notify(prefix) {
    return function NotifyMaker(name, data, func) {
      let thisArg = this;
      let notify = function(state, subject) {
        let mesg = prefix + name + ":" + state;
        thisArg._log.trace("Event: " + mesg);
        Observers.notify(mesg, subject, data);
      };

      return function WrappedNotify() {
        try {
          notify("start", null);
          let ret = func.call(thisArg);
          notify("finish", ret);
          return ret;
        }
        catch(ex) {
          notify("error", ex);
          throw ex;
        }
      };
    };
  },

  



  makeGUID: function makeGUID() {
    return CommonUtils.encodeBase64URL(Utils.generateRandomBytes(9));
  },

  _base64url_regex: /^[-abcdefghijklmnopqrstuvwxyz0123456789_]{12}$/i,
  checkGUID: function checkGUID(guid) {
    return !!guid && this._base64url_regex.test(guid);
  },

  










  deferGetSet: function Utils_deferGetSet(obj, defer, prop) {
    if (Array.isArray(prop))
      return prop.map(function(prop) Utils.deferGetSet(obj, defer, prop));

    let prot = obj.prototype;

    
    if (!prot.__lookupGetter__(prop)) {
      prot.__defineGetter__(prop, function () {
        return this[defer][prop];
      });
    }

    
    if (!prot.__lookupSetter__(prop)) {
      prot.__defineSetter__(prop, function (val) {
        this[defer][prop] = val;
      });
    }
  },

  lazyStrings: function Weave_lazyStrings(name) {
    let bundle = "chrome://weave/locale/services/" + name + ".properties";
    return function() new StringBundle(bundle);
  },

  deepEquals: function eq(a, b) {
    
    if (a === b)
      return true;

    
    if (typeof a != "object" || typeof b != "object")
      return false;

    
    if (a === null || b === null)
      return false;

    
    for (let k in a)
      if (!eq(a[k], b[k]))
        return false;

    
    for (let k in b)
      if (!(k in a) && !eq(a[k], b[k]))
        return false;

    return true;
  },

  
  
  
  throwHMACMismatch: function throwHMACMismatch(shouldBe, is) {
    throw "Record SHA256 HMAC mismatch: should be " + shouldBe + ", is " + is;
  },

  isHMACMismatch: function isHMACMismatch(ex) {
    const hmacFail = "Record SHA256 HMAC mismatch: ";
    return ex && ex.indexOf && (ex.indexOf(hmacFail) == 0);
  },

  





  base32ToFriendly: function base32ToFriendly(input) {
    return input.toLowerCase()
                .replace("l", '8', "g")
                .replace("o", '9', "g");
  },

  base32FromFriendly: function base32FromFriendly(input) {
    return input.toUpperCase()
                .replace("8", 'L', "g")
                .replace("9", 'O', "g");
  },

  



  
  encodeKeyBase32: function encodeKeyBase32(keyData) {
    return Utils.base32ToFriendly(
             Utils.encodeBase32(keyData))
           .slice(0, SYNC_KEY_ENCODED_LENGTH);
  },

  decodeKeyBase32: function decodeKeyBase32(encoded) {
    return Utils.decodeBase32(
             Utils.base32FromFriendly(
               Utils.normalizePassphrase(encoded)))
           .slice(0, SYNC_KEY_DECODED_LENGTH);
  },

  base64Key: function base64Key(keyData) {
    return btoa(keyData);
  },

  



  derivePresentableKeyFromPassphrase : function derivePresentableKeyFromPassphrase(passphrase, salt, keyLength, forceJS) {
    let k = CryptoUtils.deriveKeyFromPassphrase(passphrase, salt, keyLength,
                                                forceJS);
    return Utils.encodeKeyBase32(k);
  },

  



  deriveEncodedKeyFromPassphrase : function deriveEncodedKeyFromPassphrase(passphrase, salt, keyLength, forceJS) {
    let k = CryptoUtils.deriveKeyFromPassphrase(passphrase, salt, keyLength,
                                                forceJS);
    return Utils.base64Key(k);
  },

  




  presentEncodedKeyAsSyncKey : function presentEncodedKeyAsSyncKey(encodedKey) {
    return Utils.encodeKeyBase32(atob(encodedKey));
  },

  












  jsonLoad: Task.async(function*(filePath, that, callback) {
    let path = OS.Path.join(OS.Constants.Path.profileDir, "weave", filePath + ".json");

    if (that._log) {
      that._log.trace("Loading json from disk: " + filePath);
    }

    let json;

    try {
      json = yield CommonUtils.readJSON(path);
    } catch (e if e instanceof OS.File.Error && e.becauseNoSuchFile) {
      
    } catch (e) {
      if (that._log) {
        that._log.debug("Failed to load json: " +
                        CommonUtils.exceptionStr(e));
      }
    }

    callback.call(that, json);
  }),

  















  jsonSave: Task.async(function*(filePath, that, obj, callback) {
    let path = OS.Path.join(OS.Constants.Path.profileDir, "weave",
                            ...(filePath + ".json").split("/"));
    let dir = OS.Path.dirname(path);
    let error = null;

    try {
      yield OS.File.makeDir(dir, { from: OS.Constants.Path.profileDir });

      if (that._log) {
        that._log.trace("Saving json to disk: " + path);
      }

      let json = typeof obj == "function" ? obj.call(that) : obj;

      yield CommonUtils.writeJSON(json, path);
    } catch (e) {
      error = e
    }

    if (typeof callback == "function") {
      callback.call(that, error);
    }
  }),

  getErrorString: function Utils_getErrorString(error, args) {
    try {
      return Str.errors.get(error, args || null);
    } catch (e) {}

    
    return Str.errors.get("error.reason.unknown");
  },

  


  generatePassphrase: function generatePassphrase() {
    
    
    
    return Utils.encodeKeyBase32(CryptoUtils.generateRandomBytes(16));
  },

  


















  isPassphrase: function(s) {
    if (s) {
      return /^[abcdefghijkmnpqrstuvwxyz23456789]{26}$/.test(Utils.normalizePassphrase(s));
    }
    return false;
  },

  





  hyphenatePassphrase: function hyphenatePassphrase(passphrase) {
    
    return Utils.hyphenatePartialPassphrase(passphrase, true);
  },

  hyphenatePartialPassphrase: function hyphenatePartialPassphrase(passphrase, omitTrailingDash) {
    if (!passphrase)
      return null;

    
    let data = passphrase.toLowerCase().replace(/[^abcdefghijkmnpqrstuvwxyz23456789]/g, "");

    
    if ((data.length == 1) && !omitTrailingDash)
      return data + "-";

    
    let y = data.substr(0,1);
    let z = data.substr(1).replace(/(.{1,5})/g, "-$1");

    
    if ((z.length == 30) || omitTrailingDash)
      return y + z;

    
    return (y + z.replace(/([^-]{5})$/, "$1-")).substr(0, SYNC_KEY_HYPHENATED_LENGTH);
  },

  normalizePassphrase: function normalizePassphrase(pp) {
    
    
    pp = pp.trim().toLowerCase();

    
    if (pp.length == 23 &&
        [5, 11, 17].every(function(i) pp[i] == '-')) {

      return pp.slice(0, 5) + pp.slice(6, 11)
             + pp.slice(12, 17) + pp.slice(18, 23);
    }

    
    if (pp.length == 31 &&
        [1, 7, 13, 19, 25].every(function(i) pp[i] == '-')) {

      return pp.slice(0, 1) + pp.slice(2, 7)
             + pp.slice(8, 13) + pp.slice(14, 19)
             + pp.slice(20, 25) + pp.slice(26, 31);
    }

    
    return pp;
  },

  normalizeAccount: function normalizeAccount(acc) {
    return acc.trim();
  },

  



  arraySub: function arraySub(minuend, subtrahend) {
    if (!minuend.length || !subtrahend.length)
      return minuend;
    return minuend.filter(function(i) subtrahend.indexOf(i) == -1);
  },

  


  arrayUnion: function arrayUnion(foo, bar) {
    if (!foo.length)
      return bar;
    if (!bar.length)
      return foo;
    return foo.concat(Utils.arraySub(bar, foo));
  },

  bind2: function Async_bind2(object, method) {
    return function innerBind() { return method.apply(object, arguments); };
  },

  


  mpEnabled: function mpEnabled() {
    let modules = Cc["@mozilla.org/security/pkcs11moduledb;1"]
                    .getService(Ci.nsIPKCS11ModuleDB);
    let sdrSlot = modules.findSlotByName("");
    let status  = sdrSlot.status;
    let slots = Ci.nsIPKCS11Slot;

    return status != slots.SLOT_UNINITIALIZED && status != slots.SLOT_READY;
  },

  


  mpLocked: function mpLocked() {
    let modules = Cc["@mozilla.org/security/pkcs11moduledb;1"]
                    .getService(Ci.nsIPKCS11ModuleDB);
    let sdrSlot = modules.findSlotByName("");
    let status  = sdrSlot.status;
    let slots = Ci.nsIPKCS11Slot;

    if (status == slots.SLOT_READY || status == slots.SLOT_LOGGED_IN
                                   || status == slots.SLOT_UNINITIALIZED)
      return false;

    if (status == slots.SLOT_NOT_LOGGED_IN)
      return true;

    
    return true;
  },

  
  
  ensureMPUnlocked: function ensureMPUnlocked() {
    if (!Utils.mpLocked()) {
      return true;
    }
    let sdr = Cc["@mozilla.org/security/sdr;1"]
                .getService(Ci.nsISecretDecoderRing);
    try {
      sdr.encryptString("bacon");
      return true;
    } catch(e) {}
    return false;
  },

  




  calculateBackoff: function calculateBackoff(attempts, baseInterval,
                                              statusInterval) {
    let backoffInterval = attempts *
                          (Math.floor(Math.random() * baseInterval) +
                           baseInterval);
    return Math.max(Math.min(backoffInterval, MAXIMUM_BACKOFF_INTERVAL),
                    statusInterval);
  },

  






  getSyncCredentialsHosts: function() {
    
    if (this._syncCredentialsHosts) {
      return this._syncCredentialsHosts;
    }
    let result = new Set();
    
    result.add(PWDMGR_HOST);
    
    result.add(FxAccountsCommon.FXA_PWDMGR_HOST);
    
    
    
    for (let prefName of ["identity.fxaccounts.remote.force_auth.uri",
                          "identity.fxaccounts.remote.signup.uri",
                          "identity.fxaccounts.remote.signin.uri",
                          "identity.fxaccounts.settings.uri"]) {
      let prefVal;
      try {
        prefVal = Services.prefs.getCharPref(prefName);
      } catch (_) {
        continue;
      }
      let uri = Services.io.newURI(prefVal, null, null);
      result.add(uri.prePath);
    }
    return this._syncCredentialsHosts = result;
  },
};

XPCOMUtils.defineLazyGetter(Utils, "_utf8Converter", function() {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  return converter;
});




this.Svc = {};
Svc.Prefs = new Preferences(PREFS_BRANCH);
Svc.DefaultPrefs = new Preferences({branch: PREFS_BRANCH, defaultBranch: true});
Svc.Obs = Observers;

let _sessionCID = Services.appinfo.ID == SEAMONKEY_ID ?
  "@mozilla.org/suite/sessionstore;1" :
  "@mozilla.org/browser/sessionstore;1";

[
 ["Idle", "@mozilla.org/widget/idleservice;1", "nsIIdleService"],
 ["Session", _sessionCID, "nsISessionStore"]
].forEach(function([name, contract, iface]) {
  XPCOMUtils.defineLazyServiceGetter(Svc, name, contract, iface);
});

XPCOMUtils.defineLazyModuleGetter(Svc, "FormHistory", "resource://gre/modules/FormHistory.jsm");

Svc.__defineGetter__("Crypto", function() {
  let cryptoSvc;
  let ns = {};
  Cu.import("resource://services-crypto/WeaveCrypto.js", ns);
  cryptoSvc = new ns.WeaveCrypto();
  delete Svc.Crypto;
  return Svc.Crypto = cryptoSvc;
});

this.Str = {};
["errors", "sync"].forEach(function(lazy) {
  XPCOMUtils.defineLazyGetter(Str, lazy, Utils.lazyStrings(lazy));
});

Svc.Obs.add("xpcom-shutdown", function () {
  for (let name in Svc)
    delete Svc[name];
});
