



"use strict";

this.EXPORTED_SYMBOLS = ["Social"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SocialService",
  "resource://gre/modules/SocialService.jsm");


function prefObserver(subject, topic, data) {
  let enable = Services.prefs.getBoolPref("social.enabled");
  if (enable && !Social.provider) {
    Social.provider = Social.defaultProvider;
  } else if (!enable && Social.provider) {
    Social.provider = null;
  }
}

Services.prefs.addObserver("social.enabled", prefObserver, false);
Services.obs.addObserver(function xpcomShutdown() {
  Services.obs.removeObserver(xpcomShutdown, "xpcom-shutdown");
  Services.prefs.removeObserver("social.enabled", prefObserver);
}, "xpcom-shutdown", false);

this.Social = {
  lastEventReceived: 0,
  providers: null,
  _disabledForSafeMode: false,

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

    
    
    if (this._provider)
      this._provider.enabled = false;

    this._provider = provider;

    if (this._provider) {
      this._provider.enabled = true;
      this._currentProviderPref = this._provider.origin;
    }
    let enabled = !!provider;
    if (enabled != SocialService.enabled) {
      SocialService.enabled = enabled;
      Services.prefs.setBoolPref("social.enabled", enabled);
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

    if (this.providers) {
      return;
    }

    
    SocialService.getProviderList(function (providers) {
      this._updateProviderCache(providers);
    }.bind(this));

    
    SocialService.registerProviderListener(function providerListener(topic, data) {
      
      if (topic == "provider-added" || topic == "provider-removed") {
        this._updateProviderCache(data);
        Services.obs.notifyObservers(null, "social:providers-changed", null);
      }
    }.bind(this));
  },

  
  _updateProviderCache: function (providers) {
    this.providers = providers;

    
    if (!SocialService.enabled)
      return;
    
    this._setProvider(this.defaultProvider);
  },

  set enabled(val) {
    
    
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

  haveLoggedInUser: function () {
    return !!(this.provider && this.provider.profile && this.provider.profile.userName);
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

  installProvider: function(origin ,sourceURI, data, installCallback) {
    SocialService.installProvider(origin ,sourceURI, data, installCallback);
  },

  uninstallProvider: function(origin) {
    SocialService.uninstallProvider(origin);
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

  
  _getShareablePageUrl: function Social_getShareablePageUrl(aURI) {
    let uri = aURI.clone();
    try {
      
      uri.userPass = "";
    } catch (e) {}
    return uri.spec;
  },

  isPageShared: function Social_isPageShared(aURI) {
    let url = this._getShareablePageUrl(aURI);
    return this._sharedUrls.hasOwnProperty(url);
  },

  sharePage: function Social_sharePage(aURI) {
    
    if (!this.provider) {
      Cu.reportError("Can't share a page when no provider is current");
      return;
    }
    let port = this.provider.getWorkerPort();
    if (!port) {
      Cu.reportError("Can't share page as no provider port is available");
      return;
    }
    let url = this._getShareablePageUrl(aURI);
    this._sharedUrls[url] = true;
    port.postMessage({
      topic: "social.user-recommend",
      data: { url: url }
    });
    port.close();
  },

  unsharePage: function Social_unsharePage(aURI) {
    
    if (!this.provider) {
      Cu.reportError("Can't unshare a page when no provider is current");
      return;
    }
    let port = this.provider.getWorkerPort();
    if (!port) {
      Cu.reportError("Can't unshare page as no provider port is available");
      return;
    }
    let url = this._getShareablePageUrl(aURI);
    delete this._sharedUrls[url];
    port.postMessage({
      topic: "social.user-unrecommend",
      data: { url: url }
    });
    port.close();
  },

  _sharedUrls: {},

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
    let failure = aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE;
    if (failure && Social.provider.errorState != "frameworker-error") {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
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
