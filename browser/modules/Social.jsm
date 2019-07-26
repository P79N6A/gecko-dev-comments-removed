



"use strict";

this.EXPORTED_SYMBOLS = ["Social", "OpenGraphBuilder", "DynamicResizeWatcher", "sizeSocialPanelToContent"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;


const PANEL_MIN_HEIGHT = 100;
const PANEL_MIN_WIDTH = 330;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SocialService",
  "resource://gre/modules/SocialService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");

XPCOMUtils.defineLazyServiceGetter(this, "unescapeService",
                                   "@mozilla.org/feed-unescapehtml;1",
                                   "nsIScriptableUnescapeHTML");


function prefObserver(subject, topic, data) {
  let enable = Services.prefs.getBoolPref("social.enabled");
  if (enable && !Social.provider) {
    
    SocialService.getOrderedProviderList(function(providers) {
      Social._updateProviderCache(providers);
      Social.enabled = true;
      Services.obs.notifyObservers(null, "social:providers-changed", null);
    });
  } else if (!enable && Social.provider) {
    Social.provider = null;
  }
}

Services.prefs.addObserver("social.enabled", prefObserver, false);
Services.obs.addObserver(function xpcomShutdown() {
  Services.obs.removeObserver(xpcomShutdown, "xpcom-shutdown");
  Services.prefs.removeObserver("social.enabled", prefObserver);
}, "xpcom-shutdown", false);

function promiseSetAnnotation(aURI, providerList) {
  let deferred = Promise.defer();

  
  
  Services.tm.mainThread.dispatch(function() {
    try {
      if (providerList && providerList.length > 0) {
        PlacesUtils.annotations.setPageAnnotation(
          aURI, "social/mark", JSON.stringify(providerList), 0,
          PlacesUtils.annotations.EXPIRE_WITH_HISTORY);
      } else {
        PlacesUtils.annotations.removePageAnnotation(aURI, "social/mark");
      }
    } catch(e) {
      Cu.reportError("SocialAnnotation failed: " + e);
    }
    deferred.resolve();
  }, Ci.nsIThread.DISPATCH_NORMAL);

  return deferred.promise;
}

function promiseGetAnnotation(aURI) {
  let deferred = Promise.defer();

  
  
  Services.tm.mainThread.dispatch(function() {
    let val = null;
    try {
      val = PlacesUtils.annotations.getPageAnnotation(aURI, "social/mark");
    } catch (ex) { }

    deferred.resolve(val);
  }, Ci.nsIThread.DISPATCH_NORMAL);

  return deferred.promise;
}

this.Social = {
  initialized: false,
  lastEventReceived: 0,
  providers: [],
  _disabledForSafeMode: false,

  get allowMultipleWorkers() {
    return Services.prefs.prefHasUserValue("social.allowMultipleWorkers") &&
           Services.prefs.getBoolPref("social.allowMultipleWorkers");
  },

  get _currentProviderPref() {
    try {
      return Services.prefs.getComplexValue("social.provider.current",
                                            Ci.nsISupportsString).data;
    } catch (ex) {}
    return null;
  },
  set _currentProviderPref(val) {
    let string = Cc["@mozilla.org/supports-string;1"].
                 createInstance(Ci.nsISupportsString);
    string.data = val;
    Services.prefs.setComplexValue("social.provider.current",
                                   Ci.nsISupportsString, string);
  },

  _provider: null,
  get provider() {
    return this._provider;
  },
  set provider(val) {
    this._setProvider(val);
  },

  
  _setProvider: function (provider) {
    if (this._provider == provider)
      return;

    
    
    if (this._provider && !Social.allowMultipleWorkers)
      this._provider.enabled = false;

    this._provider = provider;

    if (this._provider) {
      this._provider.enabled = true;
      this._currentProviderPref = this._provider.origin;
    }
    let enabled = !!provider;
    if (enabled != SocialService.enabled) {
      SocialService.enabled = enabled;
    }

    let origin = this._provider && this._provider.origin;
    Services.obs.notifyObservers(null, "social:provider-set", origin);
  },

  get defaultProvider() {
    if (this.providers.length == 0)
      return null;
    let provider = this._getProviderFromOrigin(this._currentProviderPref);
    return provider || this.providers[0];
  },

  init: function Social_init() {
    this._disabledForSafeMode = Services.appinfo.inSafeMode && this.enabled;

    if (this.initialized) {
      return;
    }
    this.initialized = true;
    
    
    if (SocialService.hasEnabledProviders) {
      
      SocialService.getOrderedProviderList(function (providers) {
        Social._updateProviderCache(providers);
        Social._updateWorkerState(true);
      });
    }

    
    SocialService.registerProviderListener(function providerListener(topic, origin, providers) {
      
      
      if (topic == "provider-installed" || topic == "provider-uninstalled") {
        
        Services.obs.notifyObservers(null, "social:" + topic, origin);
        return;
      }
      if (topic == "provider-enabled" || topic == "provider-disabled") {
        Social._updateProviderCache(providers);
        Social._updateWorkerState(true);
        Services.obs.notifyObservers(null, "social:providers-changed", null);
        Services.obs.notifyObservers(null, "social:" + topic, origin);
        return;
      }
      if (topic == "provider-update") {
        
        
        Social._updateProviderCache(providers);
        let provider = Social._getProviderFromOrigin(origin);
        provider.reload();
        Services.obs.notifyObservers(null, "social:providers-changed", null);
      }
    });
  },

  _updateWorkerState: function(enable) {
    
    
    if (enable && !Social.allowMultipleWorkers)
      return;
    [p.enabled = enable for (p of Social.providers) if (p.enabled != enable)];
  },

  
  _updateProviderCache: function (providers) {
    this.providers = providers;

    
    
    if (!SocialService.enabled) {
      Services.obs.notifyObservers(null, "social:provider-set", null);
      return;
    }
    
    this._setProvider(this.defaultProvider);
  },

  set enabled(val) {
    
    

    this._updateWorkerState(val);

    if (val) {
      if (!this.provider)
        this.provider = this.defaultProvider;
    } else {
      this.provider = null;
    }
  },

  get enabled() {
    return this.provider != null;
  },

  toggle: function Social_toggle() {
    this.enabled = this._disabledForSafeMode ? false : !this.enabled;
    this._disabledForSafeMode = false;
  },

  toggleSidebar: function SocialSidebar_toggle() {
    let prefValue = Services.prefs.getBoolPref("social.sidebar.open");
    Services.prefs.setBoolPref("social.sidebar.open", !prefValue);
  },

  toggleNotifications: function SocialNotifications_toggle() {
    let prefValue = Services.prefs.getBoolPref("social.toast-notifications.enabled");
    Services.prefs.setBoolPref("social.toast-notifications.enabled", !prefValue);
  },

  setProviderByOrigin: function (origin) {
    this.provider = this._getProviderFromOrigin(origin);
  },

  _getProviderFromOrigin: function (origin) {
    for (let p of this.providers) {
      if (p.origin == origin) {
        return p;
      }
    }
    return null;
  },

  getManifestByOrigin: function(origin) {
    return SocialService.getManifestByOrigin(origin);
  },

  installProvider: function(doc, data, installCallback) {
    SocialService.installProvider(doc, data, installCallback);
  },

  uninstallProvider: function(origin, aCallback) {
    SocialService.uninstallProvider(origin, aCallback);
  },

  
  activateFromOrigin: function (origin, callback) {
    
    
    SocialService.addBuiltinProvider(origin, function(provider) {
      if (provider) {
        
        if (provider == this.provider)
          return;
        this.provider = provider;
      }
      if (callback)
        callback(provider);
    }.bind(this));
  },

  deactivateFromOrigin: function (origin, oldOrigin) {
    
    let provider = this._getProviderFromOrigin(origin);
    let oldProvider = this._getProviderFromOrigin(oldOrigin);
    if (!oldProvider && this.providers.length)
      oldProvider = this.providers[0];
    this.provider = oldProvider;
    if (provider)
      SocialService.removeProvider(origin);
  },

  
  isURIMarked: function(origin, aURI, aCallback) {
    promiseGetAnnotation(aURI).then(function(val) {
      if (val) {
        let providerList = JSON.parse(val);
        val = providerList.indexOf(origin) >= 0;
      }
      aCallback(!!val);
    }).then(null, Cu.reportError);
  },

  markURI: function(origin, aURI, aCallback) {
    
    promiseGetAnnotation(aURI).then(function(val) {

      let providerList = val ? JSON.parse(val) : [];
      let marked = providerList.indexOf(origin) >= 0;
      if (marked)
        return;
      providerList.push(origin);
      
      
      let place = {
        uri: aURI,
        visits: [{
          visitDate: Date.now() + 1000,
          transitionType: Ci.nsINavHistoryService.TRANSITION_LINK
        }]
      };
      PlacesUtils.asyncHistory.updatePlaces(place, {
        handleError: function () Cu.reportError("couldn't update history for socialmark annotation"),
        handleResult: function () {},
        handleCompletion: function () {
          promiseSetAnnotation(aURI, providerList).then(function() {
            if (aCallback)
              schedule(function() { aCallback(true); } );
          }).then(null, Cu.reportError);
        }
      });
    }).then(null, Cu.reportError);
  },

  unmarkURI: function(origin, aURI, aCallback) {
    
    
    promiseGetAnnotation(aURI).then(function(val) {
      let providerList = val ? JSON.parse(val) : [];
      let marked = providerList.indexOf(origin) >= 0;
      if (marked) {
        
        providerList.splice(providerList.indexOf(origin), 1);
        promiseSetAnnotation(aURI, providerList).then(function() {
          if (aCallback)
            schedule(function() { aCallback(false); } );
        }).then(null, Cu.reportError);
      }
    }).then(null, Cu.reportError);
  },

  setErrorListener: function(iframe, errorHandler) {
    if (iframe.socialErrorListener)
      return iframe.socialErrorListener;
    return new SocialErrorListener(iframe, errorHandler);
  }
};

function schedule(callback) {
  Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
}




function SocialErrorListener(iframe, errorHandler) {
  this.setErrorMessage = errorHandler;
  this.iframe = iframe;
  iframe.socialErrorListener = this;
  iframe.docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                                   .getInterface(Ci.nsIWebProgress)
                                   .addProgressListener(this,
                                                        Ci.nsIWebProgress.NOTIFY_STATE_REQUEST |
                                                        Ci.nsIWebProgress.NOTIFY_LOCATION);
}

SocialErrorListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  remove: function() {
    this.iframe.docShell.QueryInterface(Ci.nsIInterfaceRequestor)
                                     .getInterface(Ci.nsIWebProgress)
                                     .removeProgressListener(this);
    delete this.iframe.socialErrorListener;
  },

  onStateChange: function SPL_onStateChange(aWebProgress, aRequest, aState, aStatus) {
    let failure = false;
    if ((aState & Ci.nsIWebProgressListener.STATE_STOP)) {
      if (aRequest instanceof Ci.nsIHttpChannel) {
        try {
          
          
          failure = aRequest.responseStatus >= 400 &&
                    aRequest.responseStatus < 600;
        } catch (e) {}
      }
    }

    
    
    if (failure && aStatus != Components.results.NS_BINDING_ABORTED) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      Social.provider.errorState = "content-error";
      this.setErrorMessage(aWebProgress.QueryInterface(Ci.nsIDocShell)
                              .chromeEventHandler);
    }
  },

  onLocationChange: function SPL_onLocationChange(aWebProgress, aRequest, aLocation, aFlags) {
    if (aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      if (!Social.provider.errorState)
        Social.provider.errorState = "content-error";
      schedule(function() {
        this.setErrorMessage(aWebProgress.QueryInterface(Ci.nsIDocShell)
                              .chromeEventHandler);
      }.bind(this));
    }
  },

  onProgressChange: function SPL_onProgressChange() {},
  onStatusChange: function SPL_onStatusChange() {},
  onSecurityChange: function SPL_onSecurityChange() {},
};


function sizeSocialPanelToContent(panel, iframe) {
  let doc = iframe.contentDocument;
  if (!doc || !doc.body) {
    return;
  }
  
  
  let body = doc.body;
  let bodyId = body.getAttribute("contentid");
  if (bodyId) {
    body = doc.getElementById(bodyId) || doc.body;
  }
  
  let cs = doc.defaultView.getComputedStyle(body);
  let width = PANEL_MIN_WIDTH;
  let height = PANEL_MIN_HEIGHT;
  
  
  if (cs) {
    let computedHeight = parseInt(cs.marginTop) + body.offsetHeight + parseInt(cs.marginBottom);
    height = Math.max(computedHeight, height);
    let computedWidth = parseInt(cs.marginLeft) + body.offsetWidth + parseInt(cs.marginRight);
    width = Math.max(computedWidth, width);
  }
  iframe.style.width = width + "px";
  iframe.style.height = height + "px";
  
  if (panel.state == "open")
    panel.adjustArrowPosition();
}

function DynamicResizeWatcher() {
  this._mutationObserver = null;
}

DynamicResizeWatcher.prototype = {
  start: function DynamicResizeWatcher_start(panel, iframe) {
    this.stop(); 
    let doc = iframe.contentDocument;
    this._mutationObserver = new iframe.contentWindow.MutationObserver(function(mutations) {
      sizeSocialPanelToContent(panel, iframe);
    });
    
    let config = {attributes: true, characterData: true, childList: true, subtree: true};
    this._mutationObserver.observe(doc, config);
    
    
    sizeSocialPanelToContent(panel, iframe);
  },
  stop: function DynamicResizeWatcher_stop() {
    if (this._mutationObserver) {
      try {
        this._mutationObserver.disconnect();
      } catch (ex) {
        
        
      }
      this._mutationObserver = null;
    }
  }
}


this.OpenGraphBuilder = {
  generateEndpointURL: function(URLTemplate, pageData) {
    
    
    
    
    let [endpointURL, queryString] = URLTemplate.split("?");
    let query = {};
    if (queryString) {
      queryString.split('&').forEach(function (val) {
        let [name, value] = val.split('=');
        let p = /%\{(.+)\}/.exec(value);
        if (!p) {
          
          query[name] = value;
        } else if (pageData[p[1]]) {
          query[name] = pageData[p[1]];
        } else if (p[1] == "body") {
          
          let body = "";
          if (pageData.title)
            body += pageData.title + "\n\n";
          if (pageData.description)
            body += pageData.description + "\n\n";
          if (pageData.text)
            body += pageData.text + "\n\n";
          body += pageData.url;
          query["body"] = body;
        }
      });
    }
    var str = [];
    for (let p in query)
       str.push(p + "=" + encodeURIComponent(query[p]));
    if (str.length)
      endpointURL = endpointURL + "?" + str.join("&");
    return endpointURL;
  },

  getData: function(browser) {
    let res = {
      url: this._validateURL(browser, browser.currentURI.spec),
      title: browser.contentDocument.title,
      previews: []
    };
    this._getMetaData(browser, res);
    this._getLinkData(browser, res);
    this._getPageData(browser, res);
    return res;
  },

  _getMetaData: function(browser, o) {
    
    let els = browser.contentDocument
                  .querySelectorAll("head > meta[property], head > meta[name]");
    if (els.length < 1)
      return;
    let url;
    for (let el of els) {
      let value = el.getAttribute("content")
      if (!value)
        continue;
      value = unescapeService.unescape(value.trim());
      switch (el.getAttribute("property") || el.getAttribute("name")) {
        case "title":
        case "og:title":
          o.title = value;
          break;
        case "description":
        case "og:description":
          o.description = value;
          break;
        case "og:site_name":
          o.siteName = value;
          break;
        case "medium":
        case "og:type":
          o.medium = value;
          break;
        case "og:video":
          url = this._validateURL(browser, value);
          if (url)
            o.source = url;
          break;
        case "og:url":
          url = this._validateURL(browser, value);
          if (url)
            o.url = url;
          break;
        case "og:image":
          url = this._validateURL(browser, value);
          if (url)
            o.previews.push(url);
          break;
      }
    }
  },

  _getLinkData: function(browser, o) {
    let els = browser.contentDocument
                  .querySelectorAll("head > link[rel], head > link[id]");
    for (let el of els) {
      let url = el.getAttribute("href");
      if (!url)
        continue;
      url = this._validateURL(browser, unescapeService.unescape(url.trim()));
      switch (el.getAttribute("rel") || el.getAttribute("id")) {
        case "shorturl":
        case "shortlink":
          o.shortUrl = url;
          break;
        case "canonicalurl":
        case "canonical":
          o.url = url;
          break;
        case "image_src":
          o.previews.push(url);
          break;
      }
    }
  },

  
  _getPageData: function(browser, o) {
    if (o.previews.length < 1)
      o.previews = this._getImageUrls(browser);
  },

  _validateURL: function(browser, url) {
    let uri = Services.io.newURI(browser.currentURI.resolve(url), null, null);
    if (["http", "https", "ftp", "ftps"].indexOf(uri.scheme) < 0)
      return null;
    uri.userPass = "";
    return uri.spec;
  },

  _getImageUrls: function(browser) {
    let l = [];
    let els = browser.contentDocument.querySelectorAll("img");
    for (let el of els) {
      let content = el.getAttribute("src");
      if (content) {
        l.push(this._validateURL(browser, unescapeService.unescape(content)));
        
        if (l.length > 5)
          break;
      }
    }
    return l;
  }
};
