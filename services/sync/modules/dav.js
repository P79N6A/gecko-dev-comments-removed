



































const EXPORTED_SYMBOLS = ['DAVCollection'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");

Function.prototype.async = generatorAsync;






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

  _makeRequest: function DC__makeRequest(onComplete, op, path, headers, data) {
    let [self, cont] = yield;
    let ret;

    try {
      this._log.debug("Creating " + op + " request for " + this._baseURL + path);
  
      let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
      request = request.QueryInterface(Ci.nsIDOMEventTarget);
    
      request.addEventListener("load", new EventListener(cont, "load"), false);
      request.addEventListener("error", new EventListener(cont, "error"), false);
      request = request.QueryInterface(Ci.nsIXMLHttpRequest);
      request.open(op, this._baseURL + path, true);


      
      let channel = request.channel;
      channel = channel.QueryInterface(Ci.nsIRequest);
      let loadFlags = channel.loadFlags;
      loadFlags |= Ci.nsIRequest.VALIDATE_ALWAYS;
      channel.loadFlags = loadFlags;

      let key;
      for (key in headers) {
        if (key == 'Authentication')
          this._log.debug("HTTP Header " + key + ": (supressed)");
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

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      generatorDone(this, self, onComplete, ret);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  get _defaultHeaders() {
    return {'Authorization': this._auth? this._auth : '',
            'Content-type': 'text/plain',
            'If': this._token?
            "<" + this._baseURL + "> (<" + this._token + ">)" : ''};
  },

  
  _mkcol: function DC__mkcol(path, onComplete) {
    let [self, cont] = yield;
    let ret;

    try {
      let components = path.split('/');
      let path2 = '';

      for (let i = 0; i < components.length; i++) {

	
	if (components[i] == '')
	  break;

	path2 = path2 + components[i];

	
	this._makeRequest.async(this, cont, "GET", path2 + "/", this._defaultHeaders);
	ret = yield;
	if (!(ret.status == 404 || ret.status == 500)) { 
	  this._log.debug("Skipping creation of path " + path2 +
			  " (got status " + ret.status + ")");
	} else {
	  this._log.debug("Creating path: " + path2);
	  let gen = this._makeRequest.async(this, cont, "MKCOL", path2,
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
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      generatorDone(this, self, onComplete, ret);
      yield; 
    }
    this._log.warn("generator not properly closed");
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
    return this._mkcol.async(this, path, onComplete);
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

  

  login: function DC_login(onComplete, username, password) {
    let [self, cont] = yield;

    try {
      if (this._loggedIn) {
        this._log.debug("Login requested, but already logged in");
        return;
      }
   
      this._log.info("Logging in");

      let URI = makeURI(this._baseURL);
      this._auth = "Basic " + btoa(username + ":" + password);

      
      this.GET("", cont);
      let resp = yield;

      if (this._authProvider._authFailed || resp.status < 200 || resp.status >= 300)
        return;
  
      this._loggedIn = true;

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      generatorDone(this, self, onComplete, this._loggedIn);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  logout: function DC_logout() {
    this._log.debug("Logging out (forgetting auth header)");
    this._loggedIn = false;
    this.__auth = null;
  },

  

  _getActiveLock: function DC__getActiveLock(onComplete) {
    let [self, cont] = yield;
    let ret = null;

    try {
      this._log.info("Getting active lock token");
      this.PROPFIND("",
                    "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" +
                    "<D:propfind xmlns:D='DAV:'>" +
                    "  <D:prop><D:lockdiscovery/></D:prop>" +
                    "</D:propfind>", cont);
      let resp = yield;

      if (this._authProvider._authFailed || resp.status < 200 || resp.status >= 300)
        return;

      let tokens = xpath(resp.responseXML, '//D:locktoken/D:href');
      let token = tokens.iterateNext();
      ret = token.textContent;

    } catch (e) {
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      if (ret)
        this._log.debug("Found an active lock token");
      else
        this._log.debug("No active lock token found");
      generatorDone(this, self, onComplete, ret);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  lock: function DC_lock(onComplete) {
    let [self, cont] = yield;
    this._token = null;

    try {
      this._log.info("Acquiring lock");

      if (this._token) {
        this._log.debug("Lock called, but we already hold a token");
        return;
      }

      this.LOCK("",
                "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n" +
                "<D:lockinfo xmlns:D=\"DAV:\">\n" +
                "  <D:locktype><D:write/></D:locktype>\n" +
                "  <D:lockscope><D:exclusive/></D:lockscope>\n" +
                "</D:lockinfo>", cont);
      let resp = yield;

      if (this._authProvider._authFailed || resp.status < 200 || resp.status >= 300)
        return;

      let tokens = xpath(resp.responseXML, '//D:locktoken/D:href');
      let token = tokens.iterateNext();
      if (token)
        this._token = token.textContent;

    } catch (e){
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      if (this._token)
        this._log.info("Lock acquired");
      else
        this._log.warn("Could not acquire lock");
      generatorDone(this, self, onComplete, this._token);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  unlock: function DC_unlock(onComplete) {
    let [self, cont] = yield;
    try {
      this._log.info("Releasing lock");

      if (this._token === null) {
        this._log.debug("Unlock called, but we don't hold a token right now");
        return;
      }

      this.UNLOCK("", cont);
      let resp = yield;

      if (this._authProvider._authFailed || resp.status < 200 || resp.status >= 300)
        return;

      this._token = null;

    } catch (e){
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      if (this._token) {
        this._log.info("Could not release lock");
        generatorDone(this, self, onComplete, false);
      } else {
        this._log.info("Lock released (or we didn't have one)");
        generatorDone(this, self, onComplete, true);
      }
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  forceUnlock: function DC_forceUnlock(onComplete) {
    let [self, cont] = yield;
    let unlocked = true;

    try {
      this._log.info("Forcibly releasing any server locks");

      this._getActiveLock.async(this, cont);
      this._token = yield;

      if (!this._token) {
        this._log.info("No server lock found");
        return;
      }

      this._log.info("Server lock found, unlocking");
      this.unlock.async(this, cont);
      unlocked = yield;

    } catch (e){
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      if (unlocked)
        this._log.debug("Lock released");
      else
        this._log.debug("No lock released");
      generatorDone(this, self, onComplete, unlocked);
      yield; 
    }
    this._log.warn("generator not properly closed");
  },

  stealLock: function DC_stealLock(onComplete) {
    let [self, cont] = yield;
    let stolen = null;

    try {
      this.forceUnlock.async(this, cont);
      let unlocked = yield;

      if (unlocked) {
        this.lock.async(this, cont);
        stolen = yield;
      }

    } catch (e){
      this._log.error("Exception caught: " + (e.message? e.message : e));

    } finally {
      generatorDone(this, self, onComplete, stolen);
      yield; 
    }
    this._log.warn("generator not properly closed");
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
