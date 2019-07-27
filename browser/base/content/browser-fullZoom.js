










var FullZoom = {
  
  name: "browser.content.full-zoom",

  
  _siteSpecificPref: undefined,

  
  updateBackgroundTabs: undefined,

  
  
  
  _browserTokenMap: new WeakMap(),

  
  
  _initialLocations: new WeakMap(),

  get siteSpecific() {
    return this._siteSpecificPref;
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                         Ci.nsIObserver,
                                         Ci.nsIContentPrefObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  
  

  init: function FullZoom_init() {
    gBrowser.addEventListener("ZoomChangeUsingMouseWheel", this);

    
    this._cps2 = Cc["@mozilla.org/content-pref/service;1"].
                 getService(Ci.nsIContentPrefService2);
    this._cps2.addObserverForName(this.name, this);

    this._siteSpecificPref =
      gPrefService.getBoolPref("browser.zoom.siteSpecific");
    this.updateBackgroundTabs =
      gPrefService.getBoolPref("browser.zoom.updateBackgroundTabs");
    
    
    gPrefService.addObserver("browser.zoom.", this, true);

    
    
    for (let browser of gBrowser.browsers) {
      if (this._initialLocations.has(browser)) {
        this.onLocationChange(...this._initialLocations.get(browser), browser);
      }
    }

    
    this._initialLocations.clear();
    this._initialLocations = null;
  },

  destroy: function FullZoom_destroy() {
    gPrefService.removeObserver("browser.zoom.", this);
    this._cps2.removeObserverForName(this.name, this);
    gBrowser.removeEventListener("ZoomChangeUsingMouseWheel", this);
  },


  
  

  

  handleEvent: function FullZoom_handleEvent(event) {
    switch (event.type) {
      case "ZoomChangeUsingMouseWheel":
        let browser = this._getTargetedBrowser(event);
        this._ignorePendingZoomAccesses(browser);
        this._applyZoomToPref(browser);
        break;
    }
  },

  

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "nsPref:changed":
        switch (aData) {
          case "browser.zoom.siteSpecific":
            this._siteSpecificPref =
              gPrefService.getBoolPref("browser.zoom.siteSpecific");
            break;
          case "browser.zoom.updateBackgroundTabs":
            this.updateBackgroundTabs =
              gPrefService.getBoolPref("browser.zoom.updateBackgroundTabs");
            break;
        }
        break;
    }
  },

  

  onContentPrefSet: function FullZoom_onContentPrefSet(aGroup, aName, aValue) {
    this._onContentPrefChanged(aGroup, aValue);
  },

  onContentPrefRemoved: function FullZoom_onContentPrefRemoved(aGroup, aName) {
    this._onContentPrefChanged(aGroup, undefined);
  },

  







  _onContentPrefChanged: function FullZoom__onContentPrefChanged(aGroup, aValue) {
    if (this._isNextContentPrefChangeInternal) {
      
      
      
      delete this._isNextContentPrefChangeInternal;
      return;
    }

    let browser = gBrowser.selectedBrowser;
    if (!browser.currentURI)
      return;

    let domain = this._cps2.extractDomain(browser.currentURI.spec);
    if (aGroup) {
      if (aGroup == domain)
        this._applyPrefToZoom(aValue, browser);
      return;
    }

    this._globalValue = aValue === undefined ? aValue :
                          this._ensureValid(aValue);

    
    
    
    let hasPref = false;
    let ctxt = this._loadContextFromBrowser(browser);
    let token = this._getBrowserToken(browser);
    this._cps2.getByDomainAndName(browser.currentURI.spec, this.name, ctxt, {
      handleResult: function () hasPref = true,
      handleCompletion: function () {
        if (!hasPref && token.isCurrent)
          this._applyPrefToZoom(undefined, browser);
      }.bind(this)
    });
  },

  

  










  onLocationChange: function FullZoom_onLocationChange(aURI, aIsTabSwitch, aBrowser) {
    let browser = aBrowser || gBrowser.selectedBrowser;

    
    
    if (this._initialLocations) {
      this._initialLocations.set(browser, [aURI, aIsTabSwitch]);
      return;
    }

    
    
    
    this._ignorePendingZoomAccesses(browser);

    if (!aURI || (aIsTabSwitch && !this.siteSpecific)) {
      this._notifyOnLocationChange();
      return;
    }

    
    if (aURI.spec == "about:blank") {
      this._applyPrefToZoom(undefined, browser,
                            this._notifyOnLocationChange.bind(this));
      return;
    }

    
    if (!aIsTabSwitch && browser.isSyntheticDocument) {
      ZoomManager.setZoomForBrowser(browser, 1);
      
      this._notifyOnLocationChange();
      return;
    }

    
    let ctxt = this._loadContextFromBrowser(browser);
    let pref = this._cps2.getCachedByDomainAndName(aURI.spec, this.name, ctxt);
    if (pref) {
      this._applyPrefToZoom(pref.value, browser,
                            this._notifyOnLocationChange.bind(this));
      return;
    }

    
    let value = undefined;
    let token = this._getBrowserToken(browser);
    this._cps2.getByDomainAndName(aURI.spec, this.name, ctxt, {
      handleResult: function (resultPref) value = resultPref.value,
      handleCompletion: function () {
        if (!token.isCurrent) {
          this._notifyOnLocationChange();
          return;
        }
        this._applyPrefToZoom(value, browser,
                              this._notifyOnLocationChange.bind(this));
      }.bind(this)
    });
  },

  

  updateMenu: function FullZoom_updateMenu() {
    var menuItem = document.getElementById("toggle_zoom");

    menuItem.setAttribute("checked", !ZoomManager.useFullZoom);
  },

  
  

  


  reduce: function FullZoom_reduce() {
    ZoomManager.reduce();
    let browser = gBrowser.selectedBrowser;
    this._ignorePendingZoomAccesses(browser);
    this._applyZoomToPref(browser);
  },

  


  enlarge: function FullZoom_enlarge() {
    ZoomManager.enlarge();
    let browser = gBrowser.selectedBrowser;
    this._ignorePendingZoomAccesses(browser);
    this._applyZoomToPref(browser);
  },

  



  reset: function FullZoom_reset() {
    let browser = gBrowser.selectedBrowser;
    let token = this._getBrowserToken(browser);
    this._getGlobalValue(browser, function (value) {
      if (token.isCurrent) {
        ZoomManager.setZoomForBrowser(browser, value === undefined ? 1 : value);
        this._ignorePendingZoomAccesses(browser);
        this._executeSoon(function () {
          
          
          Services.obs.notifyObservers(null, "browser-fullZoom:zoomReset", "");
        });
      }
    });
    this._removePref(browser);
  },

  






















  _applyPrefToZoom: function FullZoom__applyPrefToZoom(aValue, aBrowser, aCallback) {
    if (!this.siteSpecific || gInPrintPreviewMode) {
      this._executeSoon(aCallback);
      return;
    }

    
    
    
    if (!aBrowser.parentNode || aBrowser.isSyntheticDocument) {
      this._executeSoon(aCallback);
      return;
    }

    if (aValue !== undefined) {
      ZoomManager.setZoomForBrowser(aBrowser, this._ensureValid(aValue));
      this._ignorePendingZoomAccesses(aBrowser);
      this._executeSoon(aCallback);
      return;
    }

    let token = this._getBrowserToken(aBrowser);
    this._getGlobalValue(aBrowser, function (value) {
      if (token.isCurrent) {
        ZoomManager.setZoomForBrowser(aBrowser, value === undefined ? 1 : value);
        this._ignorePendingZoomAccesses(aBrowser);
      }
      this._executeSoon(aCallback);
    });
  },

  





  _applyZoomToPref: function FullZoom__applyZoomToPref(browser) {
    Services.obs.notifyObservers(null, "browser-fullZoom:zoomChange", "");
    if (!this.siteSpecific ||
        gInPrintPreviewMode ||
        browser.isSyntheticDocument)
      return;

    this._cps2.set(browser.currentURI.spec, this.name,
                   ZoomManager.getZoomForBrowser(browser),
                   this._loadContextFromBrowser(browser), {
      handleCompletion: function () {
        this._isNextContentPrefChangeInternal = true;
      }.bind(this),
    });
  },

  




  _removePref: function FullZoom__removePref(browser) {
    Services.obs.notifyObservers(null, "browser-fullZoom:zoomReset", "");
    if (browser.isSyntheticDocument)
      return;
    let ctxt = this._loadContextFromBrowser(browser);
    this._cps2.removeByDomainAndName(browser.currentURI.spec, this.name, ctxt, {
      handleCompletion: function () {
        this._isNextContentPrefChangeInternal = true;
      }.bind(this),
    });
  },

  
  

  











  _getBrowserToken: function FullZoom__getBrowserToken(browser) {
    let map = this._browserTokenMap;
    if (!map.has(browser))
      map.set(browser, 0);
    return {
      token: map.get(browser),
      get isCurrent() {
        
        
        
        
        
        return map.get(browser) === this.token && browser.parentNode;
      },
    };
  },

  




  _getTargetedBrowser: function FullZoom__getTargetedBrowser(event) {
    let target = event.originalTarget;

    
    
    const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    if (target instanceof window.XULElement &&
        target.localName == "browser" &&
        target.namespaceURI == XUL_NS)
      return target;

    
    
    if (target.nodeType == Node.DOCUMENT_NODE)
      return gBrowser.getBrowserForDocument(target);

    throw new Error("Unexpected ZoomChangeUsingMouseWheel event source");
  },

  






  _ignorePendingZoomAccesses: function FullZoom__ignorePendingZoomAccesses(browser) {
    let map = this._browserTokenMap;
    map.set(browser, (map.get(browser) || 0) + 1);
  },

  _ensureValid: function FullZoom__ensureValid(aValue) {
    
    
    if (isNaN(aValue))
      return 1;

    if (aValue < ZoomManager.MIN)
      return ZoomManager.MIN;

    if (aValue > ZoomManager.MAX)
      return ZoomManager.MAX;

    return aValue;
  },

  













  _getGlobalValue: function FullZoom__getGlobalValue(browser, callback) {
    
    
    
    if ("_globalValue" in this) {
      callback.call(this, this._globalValue, true);
      return;
    }
    let value = undefined;
    this._cps2.getGlobal(this.name, this._loadContextFromBrowser(browser), {
      handleResult: function (pref) value = pref.value,
      handleCompletion: function (reason) {
        this._globalValue = this._ensureValid(value);
        callback.call(this, this._globalValue);
      }.bind(this)
    });
  },

  





  _loadContextFromBrowser: function FullZoom__loadContextFromBrowser(browser) {
    return browser.loadContext;
  },

  





  _notifyOnLocationChange: function FullZoom__notifyOnLocationChange() {
    this._executeSoon(function () {
      Services.obs.notifyObservers(null, "browser-fullZoom:location-change", "");
    });
  },

  _executeSoon: function FullZoom__executeSoon(callback) {
    if (!callback)
      return;
    Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
  },
};
