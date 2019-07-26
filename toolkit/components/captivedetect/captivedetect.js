




'use strict';

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

const DEBUG = false; 

const kCAPTIVEPORTALDETECTOR_CONTRACTID = '@mozilla.org/services/captive-detector;1';
const kCAPTIVEPORTALDETECTOR_CID        = Components.ID('{d9cd00ba-aa4d-47b1-8792-b1fe0cd35060}');

const kOpenCaptivePortalLoginEvent = 'captive-portal-login';
const kAbortCaptivePortalLoginEvent = 'captive-portal-login-abort';

function URLFetcher(url, timeout) {
  let self = this;
  let xhr = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
              .createInstance(Ci.nsIXMLHttpRequest);
  xhr.open('GET', url, true);
  
  xhr.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
  
  xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
  
  
  
  xhr.setRequestHeader("Cache-Control", "no-cache");
  
  
  xhr.setRequestHeader("Pragma", "no-cache");

  xhr.timeout = timeout;
  xhr.ontimeout = function () { self.ontimeout(); };
  xhr.onerror = function () { self.onerror(); };
  xhr.onreadystatechange = function(oEvent) {
    if (xhr.readyState === 4) {
      if (self._isAborted) {
        return;
      }
      if (xhr.status === 200) {
        self.onsuccess(xhr.responseText);
      } else {
        self.onredirectorerror(xhr.status);
      }
    }
  };
  xhr.send();
  this._xhr = xhr;
}

URLFetcher.prototype = {
  _isAborted: false,
  ontimeout: function() {},
  onerror: function() {},
  abort: function() {
    if (!this._isAborted) {
      this._isAborted = true;
      this._xhr.abort();
    }
  },
}

function LoginObserver(captivePortalDetector) {
  const LOGIN_OBSERVER_STATE_DETACHED = 0; 
  const LOGIN_OBSERVER_STATE_IDLE = 1; 
  const LOGIN_OBSERVER_STATE_BURST = 2; 
  const LOGIN_OBSERVER_STATE_VERIFY_NEEDED = 3; 
  const LOGIN_OBSERVER_STATE_VERIFYING = 4; 

  let state = LOGIN_OBSERVER_STATE_DETACHED;

  let timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
  let activityDistributor = Cc['@mozilla.org/network/http-activity-distributor;1']
                              .getService(Ci.nsIHttpActivityDistributor);
  let urlFetcher = null;

  let pageCheckingDone = function pageCheckingDone() {
    if (state === LOGIN_OBSERVER_STATE_VERIFYING) {
      urlFetcher = null;
      
      
      state = LOGIN_OBSERVER_STATE_IDLE;
      timer.initWithCallback(observer,
                             captivePortalDetector._pollingTime,
                             timer.TYPE_ONE_SHOT);
    }
  };

  let checkPageContent = function checkPageContent() {
    debug("checking if public network is available after the login procedure");

    urlFetcher = new URLFetcher(captivePortalDetector._canonicalSiteURL,
                                captivePortalDetector._maxWaitingTime);
    urlFetcher.ontimeout = pageCheckingDone;
    urlFetcher.onerror = pageCheckingDone;
    urlFetcher.onsuccess = function (content) {
      if (captivePortalDetector.validateContent(content)) {
        urlFetcher = null;
        captivePortalDetector.executeCallback(true);
      } else {
        pageCheckingDone();
      }
    };
    urlFetcher.onredirectorerror = pageCheckingDone;
  };

  
  let observer = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIHttpActivityOberver,
                                           Ci.nsITimerCallback]),

    attach: function attach() {
      if (state === LOGIN_OBSERVER_STATE_DETACHED) {
        activityDistributor.addObserver(this);
        state = LOGIN_OBSERVER_STATE_IDLE;
        timer.initWithCallback(this,
                               captivePortalDetector._pollingTime,
                               timer.TYPE_ONE_SHOT);
        debug('attach HttpObserver for login activity');
      }
    },

    detach: function detach() {
      if (state !== LOGIN_OBSERVER_STATE_DETACHED) {
        if (urlFetcher) {
          urlFetcher.abort();
          urlFetcher = null;
        }
        activityDistributor.removeObserver(this);
        timer.cancel();
        state = LOGIN_OBSERVER_STATE_DETACHED;
        debug('detach HttpObserver for login activity');
      }
    },

    


    observeActivity: function observeActivity(aHttpChannel, aActivityType,
                                              aActivitySubtype, aTimestamp,
                                              aExtraSizeData, aExtraStringData) {
      if (aActivityType === Ci.nsIHttpActivityObserver.ACTIVITY_TYPE_HTTP_TRANSACTION
          && aActivitySubtype === Ci.nsIHttpActivityObserver.ACTIVITY_SUBTYPE_RESPONSE_COMPLETE) {
        switch (state) {
          case LOGIN_OBSERVER_STATE_IDLE:
          case LOGIN_OBSERVER_STATE_VERIFY_NEEDED:
            state = LOGIN_OBSERVER_STATE_BURST;
            break;
          default:
            break;
        }
      }
    },

    


    notify : function notify() {
      switch(state) {
        case LOGIN_OBSERVER_STATE_BURST:
          
          state = LOGIN_OBSERVER_STATE_VERIFY_NEEDED;
          
        case LOGIN_OBSERVER_STATE_IDLE:
          timer.initWithCallback(this,
                                 captivePortalDetector._pollingTime,
                                 timer.TYPE_ONE_SHOT);
          break;
        case LOGIN_OBSERVER_STATE_VERIFY_NEEDED:
          
          state = LOGIN_OBSERVER_STATE_VERIFYING;
          checkPageContent();
          break;

        default:
          break;
      }
    },
  };

  return observer;
}

function CaptivePortalDetector() {
  
  this._canonicalSiteURL = null;
  this._canonicalSiteExpectedContent = null;

  try {
    this._canonicalSiteURL =
      Services.prefs.getCharPref('captivedetect.canonicalURL');
    this._canonicalSiteExpectedContent =
      Services.prefs.getCharPref('captivedetect.canonicalContent');
  } catch(e) {
    debug('canonicalURL or canonicalContent not set.')
  }

  this._maxWaitingTime =
    Services.prefs.getIntPref('captivedetect.maxWaitingTime');
  this._pollingTime =
    Services.prefs.getIntPref('captivedetect.pollingTime');
  this._maxRetryCount =
    Services.prefs.getIntPref('captivedetect.maxRetryCount');
  debug('Load Prefs {site=' + this._canonicalSiteURL + ',content='
        + this._canonicalSiteExpectedContent + ',time=' + this._maxWaitingTime
        + "max-retry=" + this._maxRetryCount + '}');

  
  this._loginObserver = LoginObserver(this);

  this._nextRequestId = 0;
  this._runningRequest = null;
  this._requestQueue = []; 
  this._interfaceNames = {}; 

  debug('CaptiveProtalDetector initiated, waitng for network connection established');
}

CaptivePortalDetector.prototype = {
  classID:   kCAPTIVEPORTALDETECTOR_CID,
  classInfo: XPCOMUtils.generateCI({classID: kCAPTIVEPORTALDETECTOR_CID,
                                    contractID: kCAPTIVEPORTALDETECTOR_CONTRACTID,
                                    classDescription: 'Captive Portal Detector',
                                    interfaces: [Ci.nsICaptivePortalDetector]}),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICaptivePortalDetector]),

  
  checkCaptivePortal: function checkCaptivePortal(aInterfaceName, aCallback) {
    if (!this._canonicalSiteURL) {
      throw Components.Exception('No canonical URL set up.');
    }

    
    if (this._interfaceNames[aInterfaceName]) {
      throw Components.Exception('Do not allow multiple request on one interface: ' + aInterface);
    }

    let request = {interfaceName: aInterfaceName};
    if (aCallback) {
      let callback = aCallback.QueryInterface(Ci.nsICaptivePortalCallback);
      request['callback'] = callback;
      request['retryCount'] = 0;
    }
    this._addRequest(request);
  },

  abort: function abort(aInterfaceName) {
    debug('abort for ' + aInterfaceName);
    this._removeRequest(aInterfaceName);
  },

  finishPreparation: function finishPreparation(aInterfaceName) {
    debug('finish preparation phase for interface "' + aInterfaceName + '"');
    if (!this._runningRequest
        || this._runningRequest.interfaceName !== aInterfaceName) {
      debug('invalid finishPreparation for ' + aInterfaceName);
      throw Components.Exception('only first request is allowed to invoke |finishPreparation|');
      return;
    }

    this._startDetection();
  },

  cancelLogin: function cancelLogin(eventId) {
    debug('login canceled by user for request "' + eventId + '"');
    
    if (this._runningRequest && this._runningRequest.hasOwnProperty('eventId')) {
      let id = this._runningRequest.eventId;
      if (eventId === id) {
        this.executeCallback(false);
      }
    }
  },

  _applyDetection: function _applyDetection() {
    debug('enter applyDetection('+ this._runningRequest.interfaceName + ')');

    
    if (this._runningRequest.hasOwnProperty('callback')) {
      this._runningRequest.callback.prepare();
    } else {
      this._startDetection();
    }
  },

  _startDetection: function _startDetection() {
    debug('startDetection {site=' + this._canonicalSiteURL + ',content='
          + this._canonicalSiteExpectedContent + ',time=' + this._maxWaitingTime + '}');
    let self = this;

    let urlFetcher = new URLFetcher(this._canonicalSiteURL, this._maxWaitingTime);

    let requestDone = this.executeCallback.bind(this, true);
    urlFetcher.ontimeout = requestDone;
    urlFetcher.onerror = requestDone;
    urlFetcher.onsuccess = function (content) {
      if (self.validateContent(content)) {
        requestDone();
      } else {
        
        self._startLogin();
      }
    };
    urlFetcher.onredirectorerror = function (status) {
      if (status >= 300 && status <= 399) {
        
        self._startLogin();
      } else if (self._runningRequest.retryCount++ < self._maxRetryCount) {
        debug('startDetection-retry: ' + self._runningRequest.retryCount);
        self._startDetection();
      } else {
        requestDone();
      }
    };

    this._runningRequest['urlFetcher'] = urlFetcher;
  },

  _startLogin: function _startLogin() {
    let id = this._allocateRequestId();
    let details = {
      type: kOpenCaptivePortalLoginEvent,
      id: id,
      url: this._canonicalSiteURL,
    };
    this._loginObserver.attach();
    this._runningRequest['eventId'] = id;
    this._sendEvent(kOpenCaptivePortalLoginEvent, details);
  },

  executeCallback: function executeCallback(success) {
    if (this._runningRequest) {
      debug('callback executed');
      if (this._runningRequest.hasOwnProperty('callback')) {
        this._runningRequest.callback.complete(success);
      }

      
      this._runningRequest['complete'] = true;
      this._removeRequest(this._runningRequest.interfaceName);
    }
  },

  _sendEvent: function _sendEvent(topic, details) {
    debug('sendEvent "' + JSON.stringify(details) + '"');
    Services.obs.notifyObservers(this,
                                 topic,
                                 JSON.stringify(details));
  },

  validateContent: function validateContent(content) {
    debug('received content: ' + content);
    return (content === this._canonicalSiteExpectedContent);
  },

  _allocateRequestId: function _allocateRequestId() {
    let newId = this._nextRequestId++;
    return newId.toString();
  },

  _runNextRequest: function _runNextRequest() {
    let nextRequest = this._requestQueue.shift();
    if (nextRequest) {
      this._runningRequest = nextRequest;
      this._applyDetection();
    }
  },

  _addRequest: function _addRequest(request) {
    this._interfaceNames[request.interfaceName] = true;
    this._requestQueue.push(request);
    if (!this._runningRequest) {
      this._runNextRequest();
    }
  },

  _removeRequest: function _removeRequest(aInterfaceName) {
    if (!this._interfaceNames[aInterfaceName]) {
      return;
    }

    delete this._interfaceNames[aInterfaceName];

    if (this._runningRequest
        && this._runningRequest.interfaceName === aInterfaceName) {
      this._loginObserver.detach();

      if (!this._runningRequest.complete) {
        
        if (this._runningRequest.hasOwnProperty('eventId')) {
          let details = {
            type: kAbortCaptivePortalLoginEvent,
            id: this._runningRequest.eventId
          };
          this._sendEvent(kAbortCaptivePortalLoginEvent, details);
        }

        
        if (this._runningRequest.hasOwnProperty('urlFetcher')) {
          this._runningRequest.urlFetcher.abort();
        }
      }

      debug('remove running request');
      this._runningRequest = null;

      
      this._runNextRequest();
      return;
    }

    
    for (let i = 0; i < this._requestQueue.length; i++) {
      if (this._requestQueue[i].interfaceName == aInterfaceName) {
        this._requestQueue.splice(i, 1);

        debug('remove pending request #' + i + ', remaining ' + this._requestQueue.length);
        break;
      }
    }
  },
};

let debug;
if (DEBUG) {
  debug = function (s) {
    dump('-*- CaptivePortalDetector component: ' + s + '\n');
  };
} else {
  debug = function (s) {};
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([CaptivePortalDetector]);
