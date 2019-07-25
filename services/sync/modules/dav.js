



































const EXPORTED_SYMBOLS = ['DAVCollection'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/async.js");

Function.prototype.async = Async.sugar;






function DAVCollection(baseURL) {
  this._baseURL = baseURL;
  this._authProvider = new DummyAuthProvider();
  this._log = Log4Moz.Service.getLogger("Service.DAV");
}
DAVCollection.prototype = {
  __dp: null,
  get _dp() {
    if (!this.__dp)
      this.__dp = Cc["@mozilla.org/xmlextras/domparser;1"].
        createInstance(Ci.nsIDOMParser);
    return this.__dp;
  },

  _auth: null,

  get baseURL() {
    return this._baseURL;
  },
  set baseURL(value) {
    this._baseURL = value;
  },

  _loggedIn: false,
  get loggedIn() {
    return this._loggedIn;
  },

  _makeRequest: function DC__makeRequest(op, path, headers, data) {
    let self = yield;
    let ret;

    this._log.debug("Creating " + op + " request for " + this._baseURL + path);

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
        this._log.debug("HTTP Header " + key + ": ***** (suppressed)");
      else
        this._log.debug("HTTP Header " + key + ": " + headers[key]);
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
    return {'Authorization': this._auth? this._auth : '',
            'Content-type': 'text/plain',
            'If': this._token?
            "<" + this._baseURL + "> (<" + this._token + ">)" : ''};
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
        if (!(ret.status == 404 || ret.status == 500)) { 
          this._log.debug("Skipping creation of path " + path2 +
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
    let headers = {'Content-type': 'text/xml; charset="utf-8"',
                   'Depth': 'infinity',
                   'Timeout': 'Second-600'};
    headers.__proto__ = this._defaultHeaders;
    return this._makeRequest.async(this, onComplete, "LOCK", path, headers, data);
  },

  UNLOCK: function DC_UNLOCK(path, onComplete) {
    let headers = {'Lock-Token': '<' + this._token + '>'};
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

    
    let tokens = Utils.xpath(resp.responseXML, '//D:href');
    let ret = [],
        token,
        root = tokens.iterateNext();
    root = root.textContent;

    while (token = tokens.iterateNext())
      ret.push(token.textContent.replace(root, ''));

    self.done(ret);
  },

  

  login: function DC_login(username, password) {
    let self = yield;

    if (this._loggedIn) {
      this._log.debug("Login requested, but already logged in");
      self.done(true);
      yield;
    }
 
    this._log.info("Logging in");

    let URI = Utils.makeURI(this._baseURL);
    this._auth = "Basic " + btoa(username + ":" + password);

    
    this.GET("", self.cb);
    let resp = yield;

    if (this._authProvider._authFailed ||
        resp.status < 200 || resp.status >= 300) {
      self.done(false);
      yield;
    }

    this._loggedIn = true;

    self.done(true);
  },

  logout: function DC_logout() {
    this._log.debug("Logging out (forgetting auth header)");
    this._loggedIn = false;
    this.__auth = null;
  },

  

  _getActiveLock: function DC__getActiveLock() {
    let self = yield;
    let ret = null;

    this._log.info("Getting active lock token");
    this.PROPFIND("",
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
    ret = token.textContent;

    if (ret)
      this._log.debug("Found an active lock token");
    else
      this._log.debug("No active lock token found");
    self.done(ret);
  },

  lock: function DC_lock() {
    let self = yield;
    this._token = null;

    this._log.info("Acquiring lock");

    if (this._token) {
      this._log.debug("Lock called, but we already hold a token");
      self.done(this._token);
      yield;
    }

    this.LOCK("",
              "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" +
              "<D:lockinfo xmlns:D=\"DAV:\">\n" +
              "  <D:locktype><D:write/></D:locktype>\n" +
              "  <D:lockscope><D:exclusive/></D:lockscope>\n" +
              "</D:lockinfo>", self.cb);
    let resp = yield;

    if (this._authProvider._authFailed ||
        resp.status < 200 || resp.status >= 300) {
      self.done(this._token);
      yield;
    }

    let tokens = Utils.xpath(resp.responseXML, '//D:locktoken/D:href');
    let token = tokens.iterateNext();
    if (token)
      this._token = token.textContent;

    if (this._token)
      this._log.debug("Lock acquired");
    else
      this._log.warn("Could not acquire lock");

    self.done(this._token);
  },

  unlock: function DC_unlock() {
    let self = yield;

    this._log.info("Releasing lock");

    if (this._token === null) {
      this._log.debug("Unlock called, but we don't hold a token right now");
      self.done(true);
      yield;
    }

    this.UNLOCK("", self.cb);
    let resp = yield;

    if (this._authProvider._authFailed ||
        resp.status < 200 || resp.status >= 300) {
      self.done(false);
      yield;
    }

    this._token = null;

    if (this._token)
      this._log.info("Could not release lock");
    else
      this._log.info("Lock released (or we didn't have one)");

    self.done(!this._token);
  },

  forceUnlock: function DC_forceUnlock() {
    let self = yield;
    let unlocked = true;

    this._log.info("Forcibly releasing any server locks");

    this._getActiveLock.async(this, self.cb);
    this._token = yield;

    if (!this._token) {
      this._log.info("No server lock found");
      self.done(true);
      yield;
    }

    this._log.info("Server lock found, unlocking");
    this.unlock.async(this, self.cb);
    unlocked = yield;

    if (unlocked)
      this._log.debug("Lock released");
    else
      this._log.debug("No lock released");
    self.done(unlocked);
  },

  stealLock: function DC_stealLock() {
    let self = yield;
    let stolen = null;

    this.forceUnlock.async(this, self.cb);
    let unlocked = yield;

    if (unlocked) {
      this.lock.async(this, self.cb);
      stolen = yield;
    }

    self.done(stolen);
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
