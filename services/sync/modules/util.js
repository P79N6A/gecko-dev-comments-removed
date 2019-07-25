




































const EXPORTED_SYMBOLS = ['Utils', 'Svc', 'Str'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/ext/Preferences.js");
Cu.import("resource://services-sync/ext/StringBundle.js");
Cu.import("resource://services-sync/log4moz.js");
Cu.import("resource://gre/modules/NetUtil.jsm");


const CB_READY = {};
const CB_COMPLETE = {};
const CB_FAIL = {};
const REASON_ERROR = Ci.mozIStorageStatementCallback.REASON_ERROR;





let Utils = {
  

















  asyncChain: function asyncChain() {
    let funcs = Array.slice(arguments);
    let thisObj = this;
    return function callback() {
      if (funcs.length) {
        let args = Array.slice(arguments).concat(callback);
        let f = funcs.shift();
        f.apply(thisObj, args);
      }
    };
  },

  








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
    return function NotifyMaker(name, subject, func) {
      let thisArg = this;
      let notify = function(state) {
        let mesg = prefix + name + ":" + state;
        thisArg._log.trace("Event: " + mesg);
        Observers.notify(mesg, subject);
      };

      return function WrappedNotify() {
        try {
          notify("start");
          let ret = func.call(thisArg);
          notify("finish");
          return ret;
        }
        catch(ex) {
          notify("error");
          throw ex;
        }
      };
    };
  },

  batchSync: function batchSync(service, engineType) {
    return function batchedSync() {
      let engine = this;
      let batchEx = null;

      
      Svc[service].runInBatchMode({
        runBatched: function wrappedSync() {
          try {
            engineType.prototype._sync.call(engine);
          }
          catch(ex) {
            batchEx = ex;
          }
        }
      }, null);

      
      if (batchEx!= null)
        throw batchEx;
    };
  },

  runInTransaction: function(db, callback, thisObj) {
    let hasTransaction = false;
    try {
      db.beginTransaction();
      hasTransaction = true;
    } catch(e) {  }

    try {
      return callback.call(thisObj);
    } finally {
      if (hasTransaction) {
        db.commitTransaction();
      }
    }
  },

  
  
  
  _storageCallbackPrototype: {
    results: null,

    
    names: null,
    syncCb: null,

    handleResult: function handleResult(results) {
      if (!this.names) {
        return;
      }
      if (!this.results) {
        this.results = [];
      }
      let row;
      while ((row = results.getNextRow()) != null) {
        let item = {};
        for each (name in this.names) {
          item[name] = row.getResultByName(name);
        }
        this.results.push(item);
      }
    },
    handleError: function handleError(error) {
      this.syncCb.throw(error);
    },
    handleCompletion: function handleCompletion(reason) {

      
      
      
      if (reason == REASON_ERROR)
        return;

      
      
      if (this.names && !this.results) {
        this.results = [];
      }
      this.syncCb(this.results);
    }
  },

  queryAsync: function(query, names) {
    
    let storageCallback = {names: names,
                           syncCb: Utils.makeSyncCallback()};
    storageCallback.__proto__ = Utils._storageCallbackPrototype;
    query.executeAsync(storageCallback);
    return Utils.waitForSyncCallback(storageCallback.syncCb);
  },

  byteArrayToString: function byteArrayToString(bytes) {
    return [String.fromCharCode(byte) for each (byte in bytes)].join("");
  },

  


  generateRandomBytes: function generateRandomBytes(length) {
    let rng = Cc["@mozilla.org/security/random-generator;1"]
                .createInstance(Ci.nsIRandomGenerator);
    let bytes = rng.generateRandomBytes(length);
    return Utils.byteArrayToString(bytes);
  },

  


  encodeBase64url: function encodeBase64url(bytes) {
    return btoa(bytes).replace('+', '-', 'g').replace('/', '_', 'g');
  },

  



  makeGUID: function makeGUID() {
    return Utils.encodeBase64url(Utils.generateRandomBytes(9));
  },

  _base64url_regex: /^[-abcdefghijklmnopqrstuvwxyz0123456789_]{12}$/i,
  checkGUID: function checkGUID(guid) {
    return !!guid && this._base64url_regex.test(guid);
  },

  ensureOneOpen: let (windows = {}) function ensureOneOpen(window) {
    
    let url = window.location.href;
    let other = windows[url];
    if (other != null)
      other.close();

    
    windows[url] = window;

    
    window.addEventListener("unload", function() windows[url] = null, false);
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  getProfileFile: function getProfileFile(arg) {
    if (typeof arg == "string")
      arg = {path: arg};

    let pathParts = arg.path.split("/");
    let file = Svc.Directory.get("ProfD", Ci.nsIFile);
    file.QueryInterface(Ci.nsILocalFile);
    for (let i = 0; i < pathParts.length; i++)
      file.append(pathParts[i]);
    if (arg.autoCreate && !file.exists())
      file.create(file.NORMAL_FILE_TYPE, PERMS_FILE);
    return file;
  },

  










  deferGetSet: function Utils_deferGetSet(obj, defer, prop) {
    if (Utils.isArray(prop))
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

  






  isArray: function Utils_isArray(val) val != null && typeof val == "object" &&
    val.constructor.name == "Array",

  
  
  lazy: function Weave_lazy(dest, prop, ctr) {
    delete dest[prop];
    dest.__defineGetter__(prop, Utils.lazyCb(dest, prop, ctr));
  },
  lazyCb: function Weave_lazyCb(dest, prop, ctr) {
    return function() {
      delete dest[prop];
      dest[prop] = new ctr();
      return dest[prop];
    };
  },

  
  lazy2: function Weave_lazy2(dest, prop, fn) {
    delete dest[prop];
    dest.__defineGetter__(prop, Utils.lazyCb2(dest, prop, fn));
  },
  lazyCb2: function Weave_lazyCb2(dest, prop, fn) {
    return function() {
      delete dest[prop];
      return dest[prop] = fn();
    };
  },

  lazySvc: function Weave_lazySvc(dest, prop, cid, iface) {
    let getter = function() {
      delete dest[prop];
      let svc = null;

      
      if (cid in Cc && iface in Ci)
        svc = Cc[cid].getService(Ci[iface]);
      else {
        svc = FakeSvc[cid];

        let log = Log4Moz.repository.getLogger("Service.Util");
        if (svc == null)
          log.warn("Component " + cid + " doesn't exist on this platform.");
        else
          log.debug("Using a fake svc object for " + cid);
      }

      return dest[prop] = svc;
    };
    dest.__defineGetter__(prop, getter);
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

  deepCopy: function Weave_deepCopy(thing, noSort) {
    if (typeof(thing) != "object" || thing == null)
      return thing;
    let ret;

    if (Utils.isArray(thing)) {
      ret = [];
      for (let i = 0; i < thing.length; i++)
        ret.push(Utils.deepCopy(thing[i], noSort));

    } else {
      ret = {};
      let props = [p for (p in thing)];
      if (!noSort)
        props = props.sort();
      props.forEach(function(k) ret[k] = Utils.deepCopy(thing[k], noSort));
    }

    return ret;
  },

  
  
  
  formatFrame: function Utils_formatFrame(frame) {
    let tmp = "<file:unknown>";

    let file = frame.filename || frame.fileName;
    if (file)
      tmp = file.replace(/^(?:chrome|file):.*?([^\/\.]+\.\w+)$/, "$1");

    if (frame.lineNumber)
      tmp += ":" + frame.lineNumber;
    if (frame.name)
      tmp = frame.name + "()@" + tmp;

    return tmp;
  },

  exceptionStr: function Weave_exceptionStr(e) {
    let message = e.message ? e.message : e;
    return message + " " + Utils.stackTrace(e);
  },

  stackTraceFromFrame: function Weave_stackTraceFromFrame(frame) {
    let output = [];
    while (frame) {
      let str = Utils.formatFrame(frame);
      if (str)
        output.push(str);
      frame = frame.caller;
    }
    return output.join(" < ");
  },

  stackTrace: function Weave_stackTrace(e) {
    
    if (e.location)
      return "Stack trace: " + Utils.stackTraceFromFrame(e.location);

    
    if (e.stack)
      return "JS Stack trace: " + e.stack.trim().replace(/\n/g, " < ").
        replace(/@[^@]*?([^\/\.]+\.\w+:)/g, "@$1");

    return "No traceback available";
  },
  
  
  
  
  throwHMACMismatch: function throwHMACMismatch(shouldBe, is) {
    throw "Record SHA256 HMAC mismatch: should be " + shouldBe + ", is " + is;
  },
  
  isHMACMismatch: function isHMACMismatch(ex) {
    const hmacFail = "Record SHA256 HMAC mismatch: ";
    return ex && ex.indexOf && (ex.indexOf(hmacFail) == 0);
  },
  
  checkStatus: function Weave_checkStatus(code, msg, ranges) {
    if (!ranges)
      ranges = [[200,300]];

    for (let i = 0; i < ranges.length; i++) {
      var rng = ranges[i];
      if (typeof(rng) == "object" && code >= rng[0] && code < rng[1])
        return true;
      else if (typeof(rng) == "number" && code == rng) {
        return true;
      }
    }

    if (msg) {
      let log = Log4Moz.repository.getLogger("Service.Util");
      log.error(msg + " Error code: " + code);
    }

    return false;
  },

  ensureStatus: function Weave_ensureStatus(args) {
    if (!Utils.checkStatus.apply(Utils, arguments))
      throw 'checkStatus failed';
  },

  



  digestUTF8: function digestUTF8(message, hasher) {
    let data = this._utf8Converter.convertToByteArray(message, {});
    hasher.update(data, data.length);
    let result = hasher.finish(false);
    if (hasher instanceof Ci.nsICryptoHMAC) {
      hasher.reset();
    }
    return result;
  },

  




  digestBytes: function digestBytes(message, hasher) {
    
    let bytes = [b.charCodeAt() for each (b in message)];
    hasher.update(bytes, bytes.length);
    let result = hasher.finish(false);
    if (hasher instanceof Ci.nsICryptoHMAC) {
      hasher.reset();
    }
    return result;
  },

  bytesAsHex: function bytesAsHex(bytes) {
    let hex = "";
    for (let i = 0; i < bytes.length; i++) {
      hex += ("0" + bytes[i].charCodeAt().toString(16)).slice(-2);
    }
    return hex;
  },

  _sha256: function _sha256(message) {
    let hasher = Cc["@mozilla.org/security/hash;1"].
      createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA256);
    return Utils.digestUTF8(message, hasher);
  },

  sha256: function sha256(message) {
    return Utils.bytesAsHex(Utils._sha256(message));
  },

  sha256Base64: function (message) {
    return btoa(Utils._sha256(message));
  },

  _sha1: function _sha1(message) {
    let hasher = Cc["@mozilla.org/security/hash;1"].
      createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);
    return Utils.digestUTF8(message, hasher);
  },

  sha1: function sha1(message) {
    return Utils.bytesAsHex(Utils._sha1(message));
  },

  sha1Base32: function sha1Base32(message) {
    return Utils.encodeBase32(Utils._sha1(message));
  },
  
  sha1Base64: function (message) {
    return btoa(Utils._sha1(message));
  },

  


  makeHMACKey: function makeHMACKey(str) {
    return Svc.KeyFactory.keyFromString(Ci.nsIKeyObject.HMAC, str);
  },
    
  


  makeHMACHasher: function makeHMACHasher(type, key) {
    let hasher = Cc["@mozilla.org/security/hmac;1"]
                   .createInstance(Ci.nsICryptoHMAC);
    hasher.init(type, key);
    return hasher;
  },

  






  sha1HMACBytes: function sha1HMACBytes(message, key) {
    let h = Utils.makeHMACHasher(Ci.nsICryptoHMAC.SHA1, key);
    return Utils.digestBytes(message, h);
  },
  sha256HMAC: function sha256HMAC(message, key) {
    let h = Utils.makeHMACHasher(Ci.nsICryptoHMAC.SHA256, key);
    return Utils.bytesAsHex(Utils.digestUTF8(message, h));
  },
  sha256HMACBytes: function sha256HMACBytes(message, key) {
    let h = Utils.makeHMACHasher(Ci.nsICryptoHMAC.SHA256, key);
    return Utils.digestBytes(message, h);
  },

  


  hkdfExpand: function hkdfExpand(prk, info, len) {
    const BLOCKSIZE = 256 / 8;
    let h = Utils.makeHMACHasher(Ci.nsICryptoHMAC.SHA256,
                                 Utils.makeHMACKey(prk));
    let T = "";
    let Tn = "";
    let iterations = Math.ceil(len/BLOCKSIZE);
    for (let i = 0; i < iterations; i++) {
      Tn = Utils.digestBytes(Tn + info + String.fromCharCode(i + 1), h);
      T += Tn;
    }
    return T.slice(0, len);
  },

  byteArrayToString: function byteArrayToString(bytes) {
    return [String.fromCharCode(byte) for each (byte in bytes)].join("");
  },
  
  














  pbkdf2Generate : function pbkdf2Generate(P, S, c, dkLen) {
    
    
    if (!dkLen)
      dkLen = SYNC_KEY_DECODED_LENGTH;
    
    
    const HLEN = 20;
    
    function F(S, c, i, h) {
    
      function XOR(a, b, isA) {
        if (a.length != b.length) {
          return false;
        }

        let val = [];
        for (let i = 0; i < a.length; i++) {
          if (isA) {
            val[i] = a[i] ^ b[i];
          } else {
            val[i] = a.charCodeAt(i) ^ b.charCodeAt(i);
          }
        }

        return val;
      }
    
      let ret;
      let U = [];

      
      let I = [];
      I[0] = String.fromCharCode((i >> 24) & 0xff);
      I[1] = String.fromCharCode((i >> 16) & 0xff);
      I[2] = String.fromCharCode((i >> 8) & 0xff);
      I[3] = String.fromCharCode(i & 0xff);

      U[0] = Utils.digestBytes(S + I.join(''), h);
      for (let j = 1; j < c; j++) {
        U[j] = Utils.digestBytes(U[j - 1], h);
      }

      ret = U[0];
      for (j = 1; j < c; j++) {
        ret = Utils.byteArrayToString(XOR(ret, U[j]));
      }

      return ret;
    }
    
    let l = Math.ceil(dkLen / HLEN);
    let r = dkLen - ((l - 1) * HLEN);

    
    let h = Utils.makeHMACHasher(Ci.nsICryptoHMAC.SHA1, Utils.makeHMACKey(P));
    
    T = [];
    for (let i = 0; i < l;) {
      T[i] = F(S, c, ++i, h);
    }

    let ret = '';
    for (i = 0; i < l-1;) {
      ret += T[i++];
    }
    ret += T[l - 1].substr(0, r);

    return ret;
  },


  


  decodeBase32: function decodeBase32(str) {
    const key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";

    let padChar = str.indexOf("=");
    let chars = (padChar == -1) ? str.length : padChar;
    let bytes = Math.floor(chars * 5 / 8);
    let blocks = Math.ceil(chars / 8);

    
    
    
    function processBlock(ret, cOffset, rOffset) {
      let c, val;

      
      
      function accumulate(val) {
        ret[rOffset] |= val;
      }

      function advance() {
        c  = str[cOffset++];
        if (!c || c == "" || c == "=") 
          throw "Done";                
        val = key.indexOf(c);
        if (val == -1)
          throw "Unknown character in base32: " + c;
      }

      
      function left(octet, shift)
        (octet << shift) & 0xff;

      advance();
      accumulate(left(val, 3));
      advance();
      accumulate(val >> 2);
      ++rOffset;
      accumulate(left(val, 6));
      advance();
      accumulate(left(val, 1));
      advance();
      accumulate(val >> 4);
      ++rOffset;
      accumulate(left(val, 4));
      advance();
      accumulate(val >> 1);
      ++rOffset;
      accumulate(left(val, 7));
      advance();
      accumulate(left(val, 2));
      advance();
      accumulate(val >> 3);
      ++rOffset;
      accumulate(left(val, 5));
      advance();
      accumulate(val);
      ++rOffset;
    }

    
    let ret  = new Array(bytes);
    let i    = 0;
    let cOff = 0;
    let rOff = 0;

    for (; i < blocks; ++i) {
      try {
        processBlock(ret, cOff, rOff);
      } catch (ex) {
        
        if (ex == "Done")
          break;
        throw ex;
      }
      cOff += 8;
      rOff += 5;
    }

    
    return Utils.byteArrayToString(ret.slice(0, bytes));
  },

  


  encodeBase32: function encodeBase32(bytes) {
    const key = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    let quanta = Math.floor(bytes.length / 5);
    let leftover = bytes.length % 5;

    
    if (leftover) {
      quanta += 1;
      for (let i = leftover; i < 5; i++)
        bytes += "\0";
    }

    
    
    let ret = "";
    for (let i = 0; i < bytes.length; i += 5) {
      let c = [byte.charCodeAt() for each (byte in bytes.slice(i, i + 5))];
      ret += key[c[0] >> 3]
           + key[((c[0] << 2) & 0x1f) | (c[1] >> 6)]
           + key[(c[1] >> 1) & 0x1f]
           + key[((c[1] << 4) & 0x1f) | (c[2] >> 4)]
           + key[((c[2] << 1) & 0x1f) | (c[3] >> 7)]
           + key[(c[3] >> 2) & 0x1f]
           + key[((c[3] << 3) & 0x1f) | (c[4] >> 5)]
           + key[c[4] & 0x1f];
    }

    switch (leftover) {
      case 1:
        return ret.slice(0, -6) + "======";
      case 2:
        return ret.slice(0, -4) + "====";
      case 3:
        return ret.slice(0, -3) + "===";
      case 4:
        return ret.slice(0, -1) + "=";
      default:
        return ret;
    }
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

  deriveKeyFromPassphrase: function deriveKeyFromPassphrase(passphrase, salt, keyLength, forceJS) {
    if (Svc.Crypto.deriveKeyFromPassphrase && !forceJS) {
      return Svc.Crypto.deriveKeyFromPassphrase(passphrase, salt, keyLength);
    }
    else {
      
      
      return Utils.pbkdf2Generate(passphrase, atob(salt), 4096, keyLength);
    }
  },

  



  derivePresentableKeyFromPassphrase : function derivePresentableKeyFromPassphrase(passphrase, salt, keyLength, forceJS) {
    let k = Utils.deriveKeyFromPassphrase(passphrase, salt, keyLength, forceJS);
    return Utils.encodeKeyBase32(k);
  },

  



  deriveEncodedKeyFromPassphrase : function deriveEncodedKeyFromPassphrase(passphrase, salt, keyLength, forceJS) {
    let k = Utils.deriveKeyFromPassphrase(passphrase, salt, keyLength, forceJS);
    return Utils.base64Key(k);
  },

  




  presentEncodedKeyAsSyncKey : function presentEncodedKeyAsSyncKey(encodedKey) {
    return Utils.encodeKeyBase32(atob(encodedKey));
  },

  makeURI: function Weave_makeURI(URIString) {
    if (!URIString)
      return null;
    try {
      return Svc.IO.newURI(URIString, null, null);
    } catch (e) {
      let log = Log4Moz.repository.getLogger("Service.Util");
      log.debug("Could not create URI: " + Utils.exceptionStr(e));
      return null;
    }
  },

  makeURL: function Weave_makeURL(URIString) {
    let url = Utils.makeURI(URIString);
    url.QueryInterface(Ci.nsIURL);
    return url;
  },

  xpath: function Weave_xpath(xmlDoc, xpathString) {
    let root = xmlDoc.ownerDocument == null ?
      xmlDoc.documentElement : xmlDoc.ownerDocument.documentElement;
    let nsResolver = xmlDoc.createNSResolver(root);

    return xmlDoc.evaluate(xpathString, xmlDoc, nsResolver,
                           Ci.nsIDOMXPathResult.ANY_TYPE, null);
  },

  getTmp: function Weave_getTmp(name) {
    let tmp = Svc.Directory.get("ProfD", Ci.nsIFile);
    tmp.QueryInterface(Ci.nsILocalFile);

    tmp.append("weave");
    tmp.append("tmp");
    if (!tmp.exists())
      tmp.create(tmp.DIRECTORY_TYPE, PERMS_DIRECTORY);

    if (name)
      tmp.append(name);

    return tmp;
  },

  









  jsonLoad: function Utils_jsonLoad(filePath, that, callback) {
    filePath = "weave/" + filePath + ".json";
    if (that._log)
      that._log.trace("Loading json from disk: " + filePath);

    let file = Utils.getProfileFile(filePath);
    if (!file.exists()) {
      callback.call(that);
      return;
    }

    let channel = NetUtil.newChannel(file);
    channel.contentType = "application/json";

    NetUtil.asyncFetch(channel, function (is, result) {
      if (!Components.isSuccessCode(result)) {
        callback.call(that);
        return;
      }
      let string = NetUtil.readInputStreamToString(is, is.available());
      is.close();
      let json;
      try {
        json = JSON.parse(string);
      } catch (ex) {
        if (that._log)
          that._log.debug("Failed to load json: " + Utils.exceptionStr(ex));
      }
      callback.call(that, json);
    });
  },

  












  jsonSave: function Utils_jsonSave(filePath, that, obj, callback) {
    filePath = "weave/" + filePath + ".json";
    if (that._log)
      that._log.trace("Saving json to disk: " + filePath);

    let file = Utils.getProfileFile({ autoCreate: true, path: filePath });
    let json = typeof obj == "function" ? obj.call(that) : obj;
    let out = JSON.stringify(json);

    let fos = Cc["@mozilla.org/network/safe-file-output-stream;1"]
                .createInstance(Ci.nsIFileOutputStream);
    fos.init(file, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, PERMS_FILE,
             fos.DEFER_OPEN);
    let is = this._utf8Converter.convertToInputStream(out);
    NetUtil.asyncCopy(is, fos, function (result) {
      if (typeof callback == "function") {
        callback.call(that);        
      }
    });
  },

  




  delay: function delay(callback, wait, thisObj, name) {
    
    wait = wait || 0;

    
    thisObj = thisObj || {};

    
    if (name in thisObj && thisObj[name] instanceof Ci.nsITimer) {
      thisObj[name].delay = wait;
      return;
    }

    
    let timer = {};
    timer.__proto__ = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    
    timer.clear = function() {
      thisObj[name] = null;
      timer.cancel();
    };

    
    timer.initWithCallback({
      notify: function notify() {
        
        timer.clear();
        callback.call(thisObj, timer);
      }
    }, wait, timer.TYPE_ONE_SHOT);

    return thisObj[name] = timer;
  },

  getIcon: function(iconUri, defaultIcon) {
    try {
      let iconURI = Utils.makeURI(iconUri);
      return Svc.Favicon.getFaviconLinkForIcon(iconURI).spec;
    }
    catch(ex) {}

    
    return defaultIcon || Svc.Favicon.defaultFavicon.spec;
  },

  getErrorString: function Utils_getErrorString(error, args) {
    try {
      return Str.errors.get(error, args || null);
    } catch (e) {}

    
    return Str.errors.get("error.reason.unknown");
  },

  encodeUTF8: function(str) {
    try {
      str = this._utf8Converter.ConvertFromUnicode(str);
      return str + this._utf8Converter.Finish();
    } catch(ex) {
      return null;
    }
  },

  decodeUTF8: function(str) {
    try {
      str = this._utf8Converter.ConvertToUnicode(str);
      return str + this._utf8Converter.Finish();
    } catch(ex) {
      return null;
    }
  },

  


  generatePassphrase: function generatePassphrase() {
    
    
    
    return Utils.encodeKeyBase32(Utils.generateRandomBytes(16));
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

  
  
  
  safeAtoB: function safeAtoB(b64) {
    let len = b64.length;
    let over = len % 4;
    return over ? atob(b64.substr(0, len - over)) : atob(b64);
  },

  



  passphraseStrength: function passphraseStrength(value) {
    let bits = 0;

    
    if (value.length)
      bits = 4;

    
    if (value.length > 1)
      bits += Math.min(value.length - 1, 7) * 2;

    
    
    if (value.length > 8)
      bits += Math.min(value.length - 8, 12) * 1.5;

    
    if (value.length > 20)
      bits += value.length - 20;

    
    if ([char.charCodeAt() for each (char in value.toLowerCase())]
        .some(function(chr) chr < 97 || chr > 122))
      bits += 6;
      
    return bits;
  },

  


  arraySub: function arraySub(minuend, subtrahend) {
    return minuend.filter(function(i) subtrahend.indexOf(i) == -1);
  },

  


  arrayUnion: function arrayUnion(foo, bar) {
    return foo.concat(Utils.arraySub(bar, foo));
  },

  bind2: function Async_bind2(object, method) {
    return function innerBind() { return method.apply(object, arguments); };
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
    let sdr = Cc["@mozilla.org/security/sdr;1"]
                .getService(Ci.nsISecretDecoderRing);
    try {
      sdr.encryptString("bacon");
      return true;
    } catch(e) {}
    return false;
  },
  
  __prefs: null,
  get prefs() {
    if (!this.__prefs) {
      this.__prefs = Cc["@mozilla.org/preferences-service;1"]
        .getService(Ci.nsIPrefService);
      this.__prefs = this.__prefs.getBranch(PREFS_BRANCH);
      this.__prefs.QueryInterface(Ci.nsIPrefBranch2);
    }
    return this.__prefs;
  },

  





  


  checkAppReady: function checkAppReady() {
    
    Svc.Obs.add("quit-application", function() {
      Utils.checkAppReady = function() {
        throw Components.Exception("App. Quitting", Cr.NS_ERROR_ABORT);
      };
    });
    
    return (Utils.checkAppReady = function() true)();
  },

  


  makeSyncCallback: function makeSyncCallback() {
    
    let onComplete = function onComplete(data) {
      onComplete.state = CB_COMPLETE;
      onComplete.value = data;
    };

    
    onComplete.state = CB_READY;
    onComplete.value = null;

    
    onComplete.throw = function onComplete_throw(data) {
      onComplete.state = CB_FAIL;
      onComplete.value = data;

      
      throw data;
    };

    return onComplete;
  },

  


  waitForSyncCallback: function waitForSyncCallback(callback) {
    
    let thread = Cc["@mozilla.org/thread-manager;1"].getService().currentThread;

    
    while (Utils.checkAppReady() && callback.state == CB_READY) {
      thread.processNextEvent(true);
    }

    
    let state = callback.state;
    callback.state = CB_READY;

    
    if (state == CB_FAIL) {
      throw callback.value;
    }

    
    return callback.value;
  }
};

let FakeSvc = {
  
  "@mozilla.org/privatebrowsing;1": {
    autoStarted: false,
    privateBrowsingEnabled: false
  },
  
  "@mozilla.org/browser/sessionstore;1": {
    setTabValue: function(tab, key, value) {
      if (!tab.__SS_extdata)
        tab.__SS_extdata = {};
      tab.__SS_extData[key] = value;
    },
    getBrowserState: function() {
      
      let state = { windows: [{ tabs: [] }] };
      let window = Svc.WinMediator.getMostRecentWindow("navigator:browser");

      
      window.Browser._tabs.forEach(function(tab) {
        let tabState = { entries: [{}] };
        let browser = tab.browser;

        
        
        if (!browser || !browser.currentURI || !browser.sessionHistory)
          return;

        let history = browser.sessionHistory;
        if (history.count > 0) {
          
          let entry = history.getEntryAtIndex(history.index, false);
          tabState.entries[0].url = entry.URI.spec;
          
          if (entry.title && entry.title != entry.url)
            tabState.entries[0].title = entry.title;
        }
        
        tabState.index = 1;

        
        
        tabState.attributes = { image: browser.mIconURL };

        
        if (tab.__SS_extdata) {
          tabState.extData = {};
          for (let key in tab.__SS_extdata)
            tabState.extData[key] = tab.__SS_extdata[key];
        }

        
        state.windows[0].tabs.push(tabState);
      });
      return JSON.stringify(state);
    }
  },
  
  "@labs.mozilla.com/Fake/Thing;1": {
    isFake: true
  }
};
Utils.lazy2(Utils, "_utf8Converter", function() {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  return converter;
});




let Svc = {};
Svc.Prefs = new Preferences(PREFS_BRANCH);
Svc.DefaultPrefs = new Preferences({branch: PREFS_BRANCH, defaultBranch: true});
Svc.Obs = Observers;

this.__defineGetter__("_sessionCID", function() {
  
  let appInfo = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo);
  return appInfo.ID == SEAMONKEY_ID ? "@mozilla.org/suite/sessionstore;1"
                                    : "@mozilla.org/browser/sessionstore;1";
});
[["Annos", "@mozilla.org/browser/annotation-service;1", "nsIAnnotationService"],
 ["AppInfo", "@mozilla.org/xre/app-info;1", "nsIXULAppInfo"],
 ["Bookmark", "@mozilla.org/browser/nav-bookmarks-service;1", "nsINavBookmarksService"],
 ["Directory", "@mozilla.org/file/directory_service;1", "nsIProperties"],
 ["Env", "@mozilla.org/process/environment;1", "nsIEnvironment"],
 ["Favicon", "@mozilla.org/browser/favicon-service;1", "nsIFaviconService"],
 ["Form", "@mozilla.org/satchel/form-history;1", "nsIFormHistory2"],
 ["History", "@mozilla.org/browser/nav-history-service;1", "nsPIPlacesDatabase"],
 ["Idle", "@mozilla.org/widget/idleservice;1", "nsIIdleService"],
 ["IO", "@mozilla.org/network/io-service;1", "nsIIOService"],
 ["KeyFactory", "@mozilla.org/security/keyobjectfactory;1", "nsIKeyObjectFactory"],
 ["Login", "@mozilla.org/login-manager;1", "nsILoginManager"],
 ["Memory", "@mozilla.org/xpcom/memory-service;1", "nsIMemory"],
 ["Private", "@mozilla.org/privatebrowsing;1", "nsIPrivateBrowsingService"],
 ["Profiles", "@mozilla.org/toolkit/profile-service;1", "nsIToolkitProfileService"],
 ["Prompt", "@mozilla.org/embedcomp/prompt-service;1", "nsIPromptService"],
 ["Script", "@mozilla.org/moz/jssubscript-loader;1", "mozIJSSubScriptLoader"],
 ["SysInfo", "@mozilla.org/system-info;1", "nsIPropertyBag2"],
 ["Version", "@mozilla.org/xpcom/version-comparator;1", "nsIVersionComparator"],
 ["WinMediator", "@mozilla.org/appshell/window-mediator;1", "nsIWindowMediator"],
 ["WinWatcher", "@mozilla.org/embedcomp/window-watcher;1", "nsIWindowWatcher"],
 ["Session", this._sessionCID, "nsISessionStore"],
].forEach(function(lazy) Utils.lazySvc(Svc, lazy[0], lazy[1], lazy[2]));

Svc.__defineGetter__("Crypto", function() {
  let cryptoSvc;
  let ns = {};
  Cu.import("resource://services-crypto/WeaveCrypto.js", ns);
  cryptoSvc = new ns.WeaveCrypto();
  delete Svc.Crypto;
  return Svc.Crypto = cryptoSvc;
});

let Str = {};
["errors", "sync"]
  .forEach(function(lazy) Utils.lazy2(Str, lazy, Utils.lazyStrings(lazy)));

Svc.Obs.add("xpcom-shutdown", function () {
  for (let name in Svc)
    delete Svc[name];
});
