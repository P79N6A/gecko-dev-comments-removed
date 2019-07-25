



































const EXPORTED_SYMBOLS = ['Utils', 'Svc', 'Str'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/ext/Observers.js");
Cu.import("resource://services-sync/ext/Preferences.js");
Cu.import("resource://services-sync/ext/StringBundle.js");
Cu.import("resource://services-sync/ext/Sync.js");
Cu.import("resource://services-sync/log4moz.js");





let Utils = {
  





  catch: function Utils_catch(func) {
    let thisArg = this;
    return function WrappedCatch() {
      try {
        return func.call(thisArg);
      }
      catch(ex) {
        thisArg._log.debug("Exception: " + Utils.exceptionStr(ex));
      }
    };
  },

  





  lock: function Utils_lock(func) {
    let thisArg = this;
    return function WrappedLock() {
      if (!thisArg.lock())
        throw "Could not acquire lock";

      try {
        return func.call(thisArg);
      }
      finally {
        thisArg.unlock();
      }
    };
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
  
  createStatement: function createStatement(db, query) {
    
    if (db.createAsyncStatement)
      return db.createAsyncStatement(query);

    
    return db.createStatement(query);
  },

  queryAsync: function(query, names) {
    
    if (!Utils.isArray(names))
      names = names == null ? [] : [names];

    
    let [exec, execCb] = Sync.withCb(query.executeAsync, query);
    return exec({
      items: [],
      handleResult: function handleResult(results) {
        let row;
        while ((row = results.getNextRow()) != null) {
          this.items.push(names.reduce(function(item, name) {
            item[name] = row.getResultByName(name);
            return item;
          }, {}));
        }
      },
      handleError: function handleError(error) {
        execCb.throw(error);
      },
      handleCompletion: function handleCompletion(reason) {
        execCb(this.items);
      }
    });
  },

  
  makeGUID: function makeGUID() {
    
    const code =
      "!()*-.0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz~";

    let guid = "";
    let num = 0;
    let val;

    
    for (let i = 0; i < 10; i++) {
      
      if (i == 0 || i == 5)
        num = Math.random();

      
      num *= 70;
      val = Math.floor(num);
      guid += code[val];
      num -= val;
    }

    return guid;
  },

  anno: function anno(id, anno, val, expire) {
    
    let annoFunc = (typeof id == "number" ? "Item" : "Page") + "Annotation";

    
    if (typeof id == "string")
      id = Utils.makeURI(id);

    if (id == null)
      throw "Null id for anno! (invalid uri)";

    switch (arguments.length) {
      case 2:
        
        return Svc.Annos["get" + annoFunc](id, anno);
      case 3:
        expire = "NEVER";
        
      case 4:
        
        expire = Svc.Annos["EXPIRE_" + expire];

        
        return Svc.Annos["set" + annoFunc](id, anno, val, 0, expire);
    }
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

    
    let parts = defer.split(".");
    let deref = function(base) Utils.deref(base, parts);

    let prot = obj.prototype;

    
    if (!prot.__lookupGetter__(prop))
      prot.__defineGetter__(prop, function() deref(this)[prop]);

    
    if (!prot.__lookupSetter__(prop))
      prot.__defineSetter__(prop, function(val) deref(this)[prop] = val);
  },

  







  deref: function Utils_deref(base, props) props.reduce(function(curr, prop)
    curr[prop], base),

  






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

  digest: function digest(message, hasher) {
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
      createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let data = converter.convertToByteArray(message, {});
    hasher.update(data, data.length);

    
    return [("0" + byte.charCodeAt().toString(16)).slice(-2) for each (byte in
      hasher.finish(false))].join("");
  },

  sha1: function sha1(message) {
    let hasher = Cc["@mozilla.org/security/hash;1"].
      createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.SHA1);
    return Utils.digest(message, hasher);
  },

  


  sha256HMAC: function sha256HMAC(message, key) {
    let hasher = Cc["@mozilla.org/security/hmac;1"].
      createInstance(Ci.nsICryptoHMAC);
    hasher.init(hasher.SHA256, key);
    return Utils.digest(message, hasher);
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
    if (!file.exists())
      return;

    try {
      let [is] = Utils.open(file, "<");
      let json = Utils.readStream(is);
      is.close();
      callback.call(that, JSON.parse(json));
    }
    catch (ex) {
      if (that._log)
        that._log.debug("Failed to load json: " + Utils.exceptionStr(ex));
    }
  },

  










  jsonSave: function Utils_jsonSave(filePath, that, callback) {
    filePath = "weave/" + filePath + ".json";
    if (that._log)
      that._log.trace("Saving json to disk: " + filePath);

    let file = Utils.getProfileFile({ autoCreate: true, path: filePath });
    let json = typeof callback == "function" ? callback.call(that) : callback;
    let out = JSON.stringify(json);
    let [fos] = Utils.open(file, ">");
    fos.writeString(out);
    fos.close();
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

  open: function open(pathOrFile, mode, perms) {
    let stream, file;

    if (pathOrFile instanceof Ci.nsIFile) {
      file = pathOrFile;
    } else {
      file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      dump("PATH IS" + pathOrFile + "\n");
      file.initWithPath(pathOrFile);
    }

    if (!perms)
      perms = PERMS_FILE;

    switch(mode) {
    case "<": {
      if (!file.exists())
        throw "Cannot open file for reading, file does not exist";
      let fis = Cc["@mozilla.org/network/file-input-stream;1"].
        createInstance(Ci.nsIFileInputStream);
      fis.init(file, MODE_RDONLY, perms, 0);
      stream = Cc["@mozilla.org/intl/converter-input-stream;1"].
        createInstance(Ci.nsIConverterInputStream);
      stream.init(fis, "UTF-8", 4096,
                  Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
    } break;

    case ">": {
      let fos = Cc["@mozilla.org/network/file-output-stream;1"].
        createInstance(Ci.nsIFileOutputStream);
      fos.init(file, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, perms, 0);
      stream = Cc["@mozilla.org/intl/converter-output-stream;1"]
        .createInstance(Ci.nsIConverterOutputStream);
      stream.init(fos, "UTF-8", 4096, 0x0000);
    } break;

    case ">>": {
      let fos = Cc["@mozilla.org/network/file-output-stream;1"].
        createInstance(Ci.nsIFileOutputStream);
      fos.init(file, MODE_WRONLY | MODE_CREATE | MODE_APPEND, perms, 0);
      stream = Cc["@mozilla.org/intl/converter-output-stream;1"]
        .createInstance(Ci.nsIConverterOutputStream);
      stream.init(fos, "UTF-8", 4096, 0x0000);
    } break;

    default:
      throw "Illegal mode to open(): " + mode;
    }

    return [stream, file];
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

  
  readStream: function Weave_readStream(is) {
    let ret = "", str = {};
    while (is.readString(4096, str) != 0) {
      ret += str.value;
    }
    return ret;
  },

  encodeUTF8: function(str) {
    try {
      var unicodeConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                             .createInstance(Ci.nsIScriptableUnicodeConverter);
      unicodeConverter.charset = "UTF-8";
      str = unicodeConverter.ConvertFromUnicode(str);
      return str + unicodeConverter.Finish();
    } catch(ex) {
      return null;
    }
  },

  decodeUTF8: function(str) {
    try {
      var unicodeConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                             .createInstance(Ci.nsIScriptableUnicodeConverter);
      unicodeConverter.charset = "UTF-8";
      str = unicodeConverter.ConvertToUnicode(str);
      return str + unicodeConverter.Finish();
    } catch(ex) {
      return null;
    }
  },

  


  arraySub: function arraySub(minuend, subtrahend) {
    return minuend.filter(function(i) subtrahend.indexOf(i) == -1);
  },

  bind2: function Async_bind2(object, method) {
    return function innerBind() { return method.apply(object, arguments); };
  },

  mpLocked: function mpLocked() {
    let modules = Cc["@mozilla.org/security/pkcs11moduledb;1"].
                  getService(Ci.nsIPKCS11ModuleDB);
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

  __prefs: null,
  get prefs() {
    if (!this.__prefs) {
      this.__prefs = Cc["@mozilla.org/preferences-service;1"]
        .getService(Ci.nsIPrefService);
      this.__prefs = this.__prefs.getBranch(PREFS_BRANCH);
      this.__prefs.QueryInterface(Ci.nsIPrefBranch2);
    }
    return this.__prefs;
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



Utils.lazySvc(FakeSvc, "@labs.mozilla.com/Weave/Crypto;2",
              "@labs.mozilla.com/Weave/Crypto;1", "IWeaveCrypto");




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
 ["Crypto", "@labs.mozilla.com/Weave/Crypto;2", "IWeaveCrypto"],
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

let Str = {};
["errors", "sync"]
  .forEach(function(lazy) Utils.lazy2(Str, lazy, Utils.lazyStrings(lazy)));

Svc.Obs.add("xpcom-shutdown", function () {
  for (let name in Svc)
    delete Svc[name];
});
