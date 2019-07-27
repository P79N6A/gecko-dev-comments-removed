



"use strict";

this.EXPORTED_SYMBOLS = ["Social", "CreateSocialStatusWidget",
                         "CreateSocialMarkWidget", "OpenGraphBuilder",
                         "DynamicResizeWatcher", "sizeSocialPanelToContent"];

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;



const PANEL_MIN_HEIGHT = 190;
const PANEL_MIN_WIDTH = 330;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "SocialService",
  "resource://gre/modules/SocialService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PageMetadata",
  "resource://gre/modules/PageMetadata.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
  "resource://gre/modules/PlacesUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
  "resource://gre/modules/PrivateBrowsingUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");


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

  init: function Social_init() {
    this._disabledForSafeMode = Services.appinfo.inSafeMode && this.enabled;
    let deferred = Promise.defer();

    if (this.initialized) {
      deferred.resolve(true);
      return deferred.promise;
    }
    this.initialized = true;
    
    
    if (SocialService.hasEnabledProviders) {
      
      SocialService.getOrderedProviderList(function (providers) {
        Social._updateProviderCache(providers);
        Social._updateWorkerState(SocialService.enabled);
        deferred.resolve(false);
      });
    } else {
      deferred.resolve(false);
    }

    
    SocialService.registerProviderListener(function providerListener(topic, origin, providers) {
      
      
      if (topic == "provider-installed" || topic == "provider-uninstalled") {
        
        Services.obs.notifyObservers(null, "social:" + topic, origin);
        return;
      }
      if (topic == "provider-enabled") {
        Social._updateProviderCache(providers);
        Social._updateWorkerState(true);
        Services.obs.notifyObservers(null, "social:" + topic, origin);
        return;
      }
      if (topic == "provider-disabled") {
        
        
        Social._updateProviderCache(providers);
        Social._updateWorkerState(providers.length > 0);
        Services.obs.notifyObservers(null, "social:" + topic, origin);
        return;
      }
      if (topic == "provider-update") {
        
        
        Social._updateProviderCache(providers);
        let provider = Social._getProviderFromOrigin(origin);
        provider.reload();
      }
    });
    return deferred.promise;
  },

  _updateWorkerState: function(enable) {
    [p.enabled = enable for (p of Social.providers) if (p.enabled != enable)];
  },

  
  _updateProviderCache: function (providers) {
    this.providers = providers;
    Services.obs.notifyObservers(null, "social:providers-changed", null);
  },

  get enabled() {
    return !this._disabledForSafeMode && this.providers.length > 0;
  },

  toggleNotifications: function SocialNotifications_toggle() {
    let prefValue = Services.prefs.getBoolPref("social.toast-notifications.enabled");
    Services.prefs.setBoolPref("social.toast-notifications.enabled", !prefValue);
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

  installProvider: function(data, installCallback, options={}) {
    SocialService.installProvider(data, installCallback, options);
  },

  uninstallProvider: function(origin, aCallback) {
    SocialService.uninstallProvider(origin, aCallback);
  },

  
  activateFromOrigin: function (origin, callback) {
    
    
    SocialService.enableProvider(origin, callback);
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

function CreateSocialStatusWidget(aId, aProvider) {
  if (!aProvider.statusURL)
    return;
  let widget = CustomizableUI.getWidget(aId);
  
  
  
  if (widget && widget.provider == CustomizableUI.PROVIDER_API)
    return;

  CustomizableUI.createWidget({
    id: aId,
    type: "custom",
    removable: true,
    defaultArea: CustomizableUI.AREA_NAVBAR,
    onBuild: function(aDocument) {
      let node = aDocument.createElement("toolbarbutton");
      node.id = this.id;
      node.setAttribute("class", "toolbarbutton-1 chromeclass-toolbar-additional social-status-button badged-button");
      node.style.listStyleImage = "url(" + (aProvider.icon32URL || aProvider.iconURL) + ")";
      node.setAttribute("origin", aProvider.origin);
      node.setAttribute("label", aProvider.name);
      node.setAttribute("tooltiptext", aProvider.name);
      node.setAttribute("oncommand", "SocialStatus.showPopup(this);");
      node.setAttribute("constrain-size", "true");

      if (PrivateBrowsingUtils.isWindowPrivate(aDocument.defaultView))
        node.setAttribute("disabled", "true");

      return node;
    }
  });
};

function CreateSocialMarkWidget(aId, aProvider) {
  if (!aProvider.markURL)
    return;
  let widget = CustomizableUI.getWidget(aId);
  
  
  
  if (widget && widget.provider == CustomizableUI.PROVIDER_API)
    return;

  CustomizableUI.createWidget({
    id: aId,
    type: "custom",
    removable: true,
    defaultArea: CustomizableUI.AREA_NAVBAR,
    onBuild: function(aDocument) {
      let node = aDocument.createElement("toolbarbutton");
      node.id = this.id;
      node.setAttribute("class", "toolbarbutton-1 chromeclass-toolbar-additional social-mark-button");
      node.setAttribute("type", "socialmark");
      node.setAttribute("constrain-size", "true");
      node.style.listStyleImage = "url(" + (aProvider.unmarkedIcon || aProvider.icon32URL || aProvider.iconURL) + ")";
      node.setAttribute("origin", aProvider.origin);

      let window = aDocument.defaultView;
      let menuLabel = window.gNavigatorBundle.getFormattedString("social.markpageMenu.label", [aProvider.name]);
      node.setAttribute("label", menuLabel);
      node.setAttribute("tooltiptext", menuLabel);
      node.setAttribute("observes", "Social:PageShareOrMark");

      return node;
    }
  });
};



function SocialErrorListener(iframe, errorHandler) {
  this.setErrorMessage = errorHandler;
  this.iframe = iframe;
  iframe.socialErrorListener = this;
  
  
  iframe.clientTop;
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
        } catch (e) {
          failure = aStatus == Components.results.NS_ERROR_CONNECTION_REFUSED;
        }
      }
    }

    
    
    if (failure && aStatus != Components.results.NS_BINDING_ABORTED) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      let origin = this.iframe.getAttribute("origin");
      if (origin) {
        let provider = Social._getProviderFromOrigin(origin);
        provider.errorState = "content-error";
      }
      this.setErrorMessage(aWebProgress.QueryInterface(Ci.nsIDocShell)
                              .chromeEventHandler);
    }
  },

  onLocationChange: function SPL_onLocationChange(aWebProgress, aRequest, aLocation, aFlags) {
    if (aFlags & Ci.nsIWebProgressListener.LOCATION_CHANGE_ERROR_PAGE) {
      aRequest.cancel(Components.results.NS_BINDING_ABORTED);
      let origin = this.iframe.getAttribute("origin");
      if (origin) {
        let provider = Social._getProviderFromOrigin(origin);
        if (!provider.errorState)
          provider.errorState = "content-error";
      }
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


function sizeSocialPanelToContent(panel, iframe, requestedSize) {
  let doc = iframe.contentDocument;
  if (!doc || !doc.body) {
    return;
  }
  
  
  let body = doc.body;
  let docEl = doc.documentElement;
  let bodyId = body.getAttribute("contentid");
  if (bodyId) {
    body = doc.getElementById(bodyId) || doc.body;
  }
  
  let cs = doc.defaultView.getComputedStyle(body);
  let width = Math.max(PANEL_MIN_WIDTH, docEl.offsetWidth);
  let height = Math.max(PANEL_MIN_HEIGHT, docEl.offsetHeight);
  
  
  if (cs) {
    let computedHeight = parseInt(cs.marginTop) + body.offsetHeight + parseInt(cs.marginBottom);
    height = Math.max(computedHeight, height);
    let computedWidth = parseInt(cs.marginLeft) + body.offsetWidth + parseInt(cs.marginRight);
    width = Math.max(computedWidth, width);
  }

  
  
  
  if (docEl.scrollHeight > iframe.boxObject.height)
    height = docEl.scrollHeight;

  
  if (requestedSize) {
    if (requestedSize.height)
      height = Math.max(height, requestedSize.height);
    if (requestedSize.width)
      width = Math.max(width, requestedSize.width);
  }

  
  
  if (iframe.boxObject.width && iframe.boxObject.height) {
    
    width += panel.boxObject.width - iframe.boxObject.width;
    height += panel.boxObject.height - iframe.boxObject.height;
  }

  
  if (Math.abs(panel.boxObject.width - width) >= 2)
    panel.style.width = width + "px";
  if (Math.abs(panel.boxObject.height - height) >= 2)
    panel.style.height = height + "px";
}

function DynamicResizeWatcher() {
  this._mutationObserver = null;
}

DynamicResizeWatcher.prototype = {
  start: function DynamicResizeWatcher_start(panel, iframe, requestedSize) {
    this.stop(); 
    let doc = iframe.contentDocument;
    this._mutationObserver = new iframe.contentWindow.MutationObserver((mutations) => {
      sizeSocialPanelToContent(panel, iframe, requestedSize);
    });
    
    let config = {attributes: true, characterData: true, childList: true, subtree: true};
    this._mutationObserver.observe(doc, config);
    
    
    sizeSocialPanelToContent(panel, iframe, requestedSize);
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
          if (p[1] == "previews")
            query[name] = pageData[p[1]][0];
          else
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
      
      if (!query.text && !query.title && pageData.title) {
        query.text = pageData.title;
      }
    }
    var str = [];
    for (let p in query)
       str.push(p + "=" + encodeURIComponent(query[p]));
    if (str.length)
      endpointURL = endpointURL + "?" + str.join("&");
    return endpointURL;
  },
};
