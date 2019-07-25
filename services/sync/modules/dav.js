



































const EXPORTED_SYMBOLS = ['DAV', 'DAVCollection'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;

Utils.lazy(this, 'DAV', DAVCollection);

let DAVLocks = {};






function DAVCollection(baseURL, defaultPrefix) {
  this.baseURL = baseURL;
  this.defaultPrefix = defaultPrefix;
  this._identity = 'DAV:default';
  this._authProvider = new DummyAuthProvider();
  this._log = Log4Moz.Service.getLogger("Service.DAV");
  this._log.level =
    Log4Moz.Level[Utils.prefs.getCharPref("log.logger.service.dav")];
}
DAVCollection.prototype = {

  __dp: null,
  get _dp() {
    if (!this.__dp)
      this.__dp = Cc["@mozilla.org/xmlextras/domparser;1"].
        createInstance(Ci.nsIDOMParser);
    return this.__dp;
  },

  get identity() { return this._identity; },
  set identity(value) { this._identity = value; },

  get baseURL() {
    return this._baseURL;
  },
  set baseURL(value) {
    if (value && value[value.length-1] != '/')
      value = value + '/';
    this._baseURL = value;
  },

  get defaultPrefix() {
    return this._defaultPrefix;
  },
  set defaultPrefix(value) {
    if (value && value[value.length-1] != '/')
      value = value + '/';
    if (value && value[0] == '/')
      value = value.slice(1);
    if (!value)
      value = '';
    this._defaultPrefix = value;
  },

  get locked() {
    return !this._lockAllowed || (DAVLocks['default'] &&
                                  DAVLocks['default'].token);
  },

  _lockAllowed: true,
  get allowLock() {
    return this._lockAllowed;
  },
  set allowLock(value) {
    this._lockAllowed = value;
  },

  _makeRequest: function DC__makeRequest(op, path, headers, data) {
    let self = yield;
    let ret;

    this._log.debug(op + " request for " + (path? path : 'root folder'));

    path = this._defaultPrefix + path;

    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
    request = request.QueryInterface(Ci.nsIDOMEventTarget);

    request.addEventListener("load", new Utils.EventListener(self.cb, "load"), false);
    request.addEventListener("error", new Utils.EventListener(self.cb, "error"), false);
    request = request.QueryInterface(Ci.nsIXMLHttpRequest);
    request.open(op, this._baseURL + path, true);


    
    let channel = request.channel;
    channel = channel.QueryInterface(Ci.nsIRequest);
    let loadFlags = channel.loadFlags;
    loadFlags |= Ci.nsIRequest.VALIDATE_ALWAYS;
    channel.loadFlags = loadFlags;

    let key;
    for (key in headers) {
      if (key == 'Authorization')
        this._log.trace("HTTP Header " + key + ": ***** (suppressed)");
      else
        this._log.trace("HTTP Header " + key + ": " + headers[key]);
      request.setRequestHeader(key, headers[key]);
    }

    this._authProvider._authFailed = false;
    request.channel.notificationCallbacks = this._authProvider;

    request.send(data);
    let event = yield;
    ret = event.target;

    if (this._authProvider._authFailed)
      this._log.warn("_makeRequest: authentication failed");
    if (ret.status < 200 || ret.status >= 300)
      this._log.warn("_makeRequest: got status " + ret.status);

    self.done(ret);
  },

  get _defaultHeaders() {
    let h = {'Content-type': 'text/plain'},
      id = ID.get(this.identity),
      lock = DAVLocks['default'];
    if (id)
      h['Authorization'] = 'Basic ' + btoa(id.username + ":" + id.password);
    if (lock)
      h['If'] = "<" + lock.URL + "> (<" + lock.token + ">)";
    return h;
  },

  
  _mkcol: function DC__mkcol(path) {
    let self = yield;
    let ok = true;

    try {
      let components = path.split('/');
      let path2 = '';

      for (let i = 0; i < components.length; i++) {

        
        if (components[i] == '')
          break;

        path2 = path2 + components[i];

        
        this._makeRequest.async(this, self.cb, "GET", path2 + "/", this._defaultHeaders);
        let ret = yield;
        if (ret.status != 404) {
          this._log.trace("Skipping creation of path " + path2 +
        		  " (got status " + ret.status + ")");
        } else {
          this._log.debug("Creating path: " + path2);
          this._makeRequest.async(this, self.cb, "MKCOL", path2,
        			  this._defaultHeaders);
          ret = yield;

          if (ret.status != 201) {
            this._log.debug(ret.responseText);
            throw 'request failed: ' + ret.status;
          }
        }

        
        path2 = path2 + "/";
      }

    } catch (e) {
      this._log.error("Could not create directory on server");
      this._log.error("Exception caught: " + (e.message? e.message : e) +
                      " - " + (e.location? e.location : ""));
      ok = false;
    }

    self.done(ok);
  },

  GET: function DC_GET(path, onComplete) {
    return this._makeRequest.async(this, onComplete, "GET", path,
                                   this._defaultHeaders);
  },

  PUT: function DC_PUT(path, data, onComplete) {
    return this._makeRequest.async(this, onComplete, "PUT", path,
                                   this._defaultHeaders, data);
  },

  DELETE: function DC_DELETE(path, onComplete) {
    return this._makeRequest.async(this, onComplete, "DELETE", path,
                                   this._defaultHeaders);
  },

  MKCOL: function DC_MKCOL(path, onComplete) {
    return this._mkcol.async(this, onComplete, path);
  },

  PROPFIND: function DC_PROPFIND(path, data, onComplete) {
    let headers = {'Content-type': 'text/xml; charset="utf-8"',
                   'Depth': '0'};
    headers.__proto__ = this._defaultHeaders;
    return this._makeRequest.async(this, onComplete, "PROPFIND", path,
                                   headers, data);
  },

  LOCK: function DC_LOCK(path, data, onComplete) {
    if (!this._lockAllowed)
      throw "Cannot acquire lock (internal lock)";

    let headers = {'Content-type': 'text/xml; charset="utf-8"',
                   'Depth': 'infinity',
                   'Timeout': 'Second-600'};
    headers.__proto__ = this._defaultHeaders;
    return this._makeRequest.async(this, onComplete, "LOCK", path, headers, data);
  },

  UNLOCK: function DC_UNLOCK(path, onComplete) {
    let headers = {'Lock-Token': '<' + DAVLocks['default'].token + '>'};
    headers.__proto__ = this._defaultHeaders;
    return this._makeRequest.async(this, onComplete, "UNLOCK", path, headers);
  },

  
  listFiles: function DC_listFiles(path) {
    let self = yield;

    if (!path)
      path = "";

    let headers = {'Content-type': 'text/xml; charset="utf-8"',
                   'Depth': '1'};
    headers.__proto__ = this._defaultHeaders;

    this._makeRequest.async(this, self.cb, "PROPFIND", path, headers,
                           "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" +
                            "<D:propfind xmlns:D='DAV:'><D:prop/></D:propfind>");
    let resp = yield;
    Utils.ensureStatus(resp.status, "propfind failed");

    let ret = [];
    try {
      let elts = Utils.xpath(resp.responseXML, '//D:href');
      
      let root = elts.iterateNext();
      root = root.textContent;
      let elt;
      while (elt = elts.iterateNext())
        ret.push(elt.textContent.replace(root, ''));
    } catch (e) {}

    self.done(ret);
  },

  

  checkLogin: function DC_checkLogin() {
    let self = yield;

    this._log.debug("Checking login");

    
    this.GET("", self.cb);
    let resp = yield;

    if (this._authProvider._authFailed ||
        resp.status < 200 || resp.status >= 300) {
      self.done(false);
      yield;
    }

    self.done(true);
  },

  

  _getActiveLock: function DC__getActiveLock() {
    let self = yield;
    let ret = null;

    this._log.debug("Getting active lock token");
    this.PROPFIND("lock",
                  "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" +
                  "<D:propfind xmlns:D='DAV:'>" +
                  "  <D:prop><D:lockdiscovery/></D:prop>" +
                  "</D:propfind>", self.cb);
    let resp = yield;

    if (this._authProvider._authFailed ||
        resp.status < 200 || resp.status >= 300) {
      self.done(false);
      yield;
    }

    let tokens = Utils.xpath(resp.responseXML, '//D:locktoken/D:href');
    let token = tokens.iterateNext();
    if (token)
      ret = token.textContent;

    if (ret)
      this._log.trace("Found an active lock token");
    else
      this._log.trace("No active lock token found");
    self.done({URL: this._baseURL, token: ret});
  },

  lock: function DC_lock() {
    let self = yield;

    this._log.trace("Acquiring lock");

    if (DAVLocks['default']) {
      this._log.debug("Lock called, but we already hold a token");
      self.done(DAVLocks['default']);
      yield;
    }

    this.LOCK("lock",
              "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" +
              "<D:lockinfo xmlns:D=\"DAV:\">\n" +
              "  <D:locktype><D:write/></D:locktype>\n" +
              "  <D:lockscope><D:exclusive/></D:lockscope>\n" +
              "</D:lockinfo>", self.cb);
    let resp = yield;

    if (this._authProvider._authFailed ||
        resp.status < 200 || resp.status >= 300) {
      self.done();
      yield;
    }

    let tokens = Utils.xpath(resp.responseXML, '//D:locktoken/D:href');
    let token = tokens.iterateNext();
    if (token) {
      DAVLocks['default'] = {
        URL: this._baseURL,
        token: token.textContent
      };
    }

    if (!DAVLocks['default']) {
      this._log.warn("Could not acquire lock");
      self.done();
      return;
    }

    this._log.trace("Lock acquired");
    self.done(DAVLocks['default']);
  },

  unlock: function DC_unlock() {
    let self = yield;

    this._log.trace("Releasing lock");

    if (!this.locked) {
      this._log.debug("Unlock called, but we don't hold a token right now");
      self.done(true);
      yield;
    }

    this.UNLOCK("lock", self.cb);
    let resp = yield;

    if (this._authProvider._authFailed ||
        resp.status < 200 || resp.status >= 300) {
      self.done(false);
      yield;
    }

    delete DAVLocks['default']
    this._log.trace("Lock released (or we didn't have one)");
    self.done(true);
  },

  forceUnlock: function DC_forceUnlock() {
    let self = yield;
    let unlocked = true;

    this._log.debug("Forcibly releasing any server locks");

    this._getActiveLock.async(this, self.cb);
    DAVLocks['default'] = yield;

    if (!DAVLocks['default']) {
      this._log.debug("No server lock found");
      self.done(true);
      yield;
    }

    this._log.trace("Server lock found, unlocking");
    this.unlock.async(this, self.cb);
    unlocked = yield;

    if (unlocked)
      this._log.trace("Lock released");
    else
      this._log.trace("No lock released");
    self.done(unlocked);
  }
};







function DummyAuthProvider() {}
DummyAuthProvider.prototype = {
  
  

  
  interfaces: [Ci.nsIBadCertListener,
               Ci.nsIAuthPromptProvider,
               Ci.nsIAuthPrompt,
               Ci.nsIPrompt,
               Ci.nsIProgressEventSink,
               Ci.nsIInterfaceRequestor,
               Ci.nsISupports],

  
  
  
  get _authFailed()         { return this.__authFailed; },
  set _authFailed(newValue) { return this.__authFailed = newValue },

  

  QueryInterface: function DAP_QueryInterface(iid) {
    if (!this.interfaces.some( function(v) { return iid.equals(v) } ))
      throw Cr.NS_ERROR_NO_INTERFACE;

    
    
    
    switch(iid) {
    case Ci.nsIAuthPrompt:
      return this.authPrompt;
    case Ci.nsIPrompt:
      return this.prompt;
    default:
      return this;
    }
  },

  

  getInterface: function DAP_getInterface(iid) {
    return this.QueryInterface(iid);
  },

  

  

  confirmUnknownIssuer: function DAP_confirmUnknownIssuer(socketInfo, cert, certAddType) {
    return false;
  },

  confirmMismatchDomain: function DAP_confirmMismatchDomain(socketInfo, targetURL, cert) {
    return false;
  },

  confirmCertExpired: function DAP_confirmCertExpired(socketInfo, cert) {
    return false;
  },

  notifyCrlNextupdate: function DAP_notifyCrlNextupdate(socketInfo, targetURL, cert) {
  },

  

  getAuthPrompt: function(aPromptReason, aIID) {
    this._authFailed = true;
    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  
  
  

  

  get authPrompt() {
    var resource = this;
    return {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
      prompt: function(dialogTitle, text, passwordRealm, savePassword, defaultText, result) {
        resource._authFailed = true;
        return false;
      },
      promptUsernameAndPassword: function(dialogTitle, text, passwordRealm, savePassword, user, pwd) {
        resource._authFailed = true;
        return false;
      },
      promptPassword: function(dialogTitle, text, passwordRealm, savePassword, pwd) {
        resource._authFailed = true;
        return false;
      }
    };
  },

  

  get prompt() {
    var resource = this;
    return {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
      alert: function(dialogTitle, text) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      alertCheck: function(dialogTitle, text, checkMessage, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirm: function(dialogTitle, text) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirmCheck: function(dialogTitle, text, checkMessage, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      confirmEx: function(dialogTitle, text, buttonFlags, button0Title, button1Title, button2Title, checkMsg, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      prompt: function(dialogTitle, text, value, checkMsg, checkValue) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      },
      promptPassword: function(dialogTitle, text, password, checkMsg, checkValue) {
        resource._authFailed = true;
        return false;
      },
      promptUsernameAndPassword: function(dialogTitle, text, username, password, checkMsg, checkValue) {
        resource._authFailed = true;
        return false;
      },
      select: function(dialogTitle, text, count, selectList, outSelection) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      }
    };
  },

  

  onProgress: function DAP_onProgress(aRequest, aContext,
                                      aProgress, aProgressMax) {
  },

  onStatus: function DAP_onStatus(aRequest, aContext,
                                  aStatus, aStatusArg) {
  }
};
