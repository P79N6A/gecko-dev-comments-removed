










var FullZoom = {
  
  name: "browser.content.full-zoom",

  
  _siteSpecificPref: undefined,

  
  updateBackgroundTabs: undefined,

  
  
  ACTION_ZOOM: 3,

  
  
  
  
  _browserTokenMap: new Map(),

  get siteSpecific() {
    return this._siteSpecificPref;
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                         Ci.nsIObserver,
                                         Ci.nsIContentPrefObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  
  

  init: function FullZoom_init() {
    
    if (gMultiProcessBrowser)
      return;

    
    window.addEventListener("DOMMouseScroll", this, false);

    
    this._cps2 = Cc["@mozilla.org/content-pref/service;1"].
                 getService(Ci.nsIContentPrefService2);
    this._cps2.addObserverForName(this.name, this);

    this._siteSpecificPref =
      gPrefService.getBoolPref("browser.zoom.siteSpecific");
    this.updateBackgroundTabs =
      gPrefService.getBoolPref("browser.zoom.updateBackgroundTabs");
    
    
    gPrefService.addObserver("browser.zoom.", this, true);

    Services.obs.addObserver(this, "outer-window-destroyed", false);
  },

  destroy: function FullZoom_destroy() {
    
    if (gMultiProcessBrowser)
      return;

    gPrefService.removeObserver("browser.zoom.", this);
    this._cps2.removeObserverForName(this.name, this);
    window.removeEventListener("DOMMouseScroll", this, false);
    Services.obs.removeObserver(this, "outer-window-destroyed");
  },


  
  

  

  handleEvent: function FullZoom_handleEvent(event) {
    switch (event.type) {
      case "DOMMouseScroll":
        this._handleMouseScrolled(event);
        break;
    }
  },

  _handleMouseScrolled: function FullZoom__handleMouseScrolled(event) {
    
    
    var pref = "mousewheel.";

    var pressedModifierCount = event.shiftKey + event.ctrlKey + event.altKey +
                                 event.metaKey + event.getModifierState("OS");
    if (pressedModifierCount != 1) {
      pref += "default.";
    } else if (event.shiftKey) {
      pref += "with_shift.";
    } else if (event.ctrlKey) {
      pref += "with_control.";
    } else if (event.altKey) {
      pref += "with_alt.";
    } else if (event.metaKey) {
      pref += "with_meta.";
    } else {
      pref += "with_win.";
    }

    pref += "action";

    
    var isZoomEvent = false;
    try {
      isZoomEvent = (gPrefService.getIntPref(pref) == this.ACTION_ZOOM);
    } catch (e) {}
    if (!isZoomEvent)
      return;

    
    
    

    
    
    
    let browser = gBrowser.selectedBrowser;
    let token = this._getBrowserToken(browser);
    window.setTimeout(function () {
      if (token.isCurrent)
        this._applyZoomToPref(browser);
    }.bind(this), 0);
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
      case "outer-window-destroyed":
        let outerID = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
        this._browserTokenMap.delete(outerID);
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
    let ctxt = this._loadContextFromWindow(browser.contentWindow);
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
    
    if (gMultiProcessBrowser)
      return;

    
    
    
    let browser = aBrowser || gBrowser.selectedBrowser;
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

    
    if (!aIsTabSwitch && browser.contentDocument.mozSyntheticDocument) {
      ZoomManager.setZoomForBrowser(browser, 1);
      
      this._notifyOnLocationChange();
      return;
    }

    
    let ctxt = this._loadContextFromWindow(browser.contentWindow);
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
    this._getGlobalValue(browser.contentWindow, function (value) {
      if (token.isCurrent) {
        ZoomManager.setZoomForBrowser(browser, value === undefined ? 1 : value);
        this._ignorePendingZoomAccesses(browser);
      }
    });
    this._removePref(browser);
  },

  






















  _applyPrefToZoom: function FullZoom__applyPrefToZoom(aValue, aBrowser, aCallback) {
    if (!this.siteSpecific || gInPrintPreviewMode) {
      this._executeSoon(aCallback);
      return;
    }

    
    
    
    if (!aBrowser.contentDocument ||
        aBrowser.contentDocument.mozSyntheticDocument) {
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
    this._getGlobalValue(aBrowser.contentWindow, function (value) {
      if (token.isCurrent) {
        ZoomManager.setZoomForBrowser(aBrowser, value === undefined ? 1 : value);
        this._ignorePendingZoomAccesses(aBrowser);
      }
      this._executeSoon(aCallback);
    });
  },

  





  _applyZoomToPref: function FullZoom__applyZoomToPref(browser) {
    if (!this.siteSpecific ||
        gInPrintPreviewMode ||
        browser.contentDocument.mozSyntheticDocument)
      return;

    this._cps2.set(browser.currentURI.spec, this.name,
                   ZoomManager.getZoomForBrowser(browser),
                   this._loadContextFromWindow(browser.contentWindow), {
      handleCompletion: function () {
        this._isNextContentPrefChangeInternal = true;
      }.bind(this),
    });
  },

  




  _removePref: function FullZoom__removePref(browser) {
    if (browser.contentDocument.mozSyntheticDocument)
      return;
    let ctxt = this._loadContextFromWindow(browser.contentWindow);
    this._cps2.removeByDomainAndName(browser.currentURI.spec, this.name, ctxt, {
      handleCompletion: function () {
        this._isNextContentPrefChangeInternal = true;
      }.bind(this),
    });
  },

  
  

  











  _getBrowserToken: function FullZoom__getBrowserToken(browser) {
    let outerID = this._browserOuterID(browser);
    let map = this._browserTokenMap;
    if (!map.has(outerID))
      map.set(outerID, 0);
    return {
      token: map.get(outerID),
      get isCurrent() {
        
        
        
        
        
        return map.get(outerID) === this.token && browser.docShell;
      },
    };
  },

  






  _ignorePendingZoomAccesses: function FullZoom__ignorePendingZoomAccesses(browser) {
    let outerID = this._browserOuterID(browser);
    let map = this._browserTokenMap;
    map.set(outerID, (map.get(outerID) || 0) + 1);
  },

  _browserOuterID: function FullZoom__browserOuterID(browser) {
    return browser.
           contentWindow.
           QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIDOMWindowUtils).
           outerWindowID;
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

  













  _getGlobalValue: function FullZoom__getGlobalValue(window, callback) {
    
    
    
    if ("_globalValue" in this) {
      callback.call(this, this._globalValue, true);
      return;
    }
    let value = undefined;
    this._cps2.getGlobal(this.name, this._loadContextFromWindow(window), {
      handleResult: function (pref) value = pref.value,
      handleCompletion: function (reason) {
        this._globalValue = this._ensureValid(value);
        callback.call(this, this._globalValue);
      }.bind(this)
    });
  },

  





  _loadContextFromWindow: function FullZoom__loadContextFromWindow(window) {
    return window.
           QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIWebNavigation).
           QueryInterface(Ci.nsILoadContext);
  },

  





  _notifyOnLocationChange: function FullZoom__notifyOnLocationChange() {
    this._executeSoon(function () {
      Services.obs.notifyObservers(null, "FullZoom:TESTS:location-change", "");
    });
  },

  _executeSoon: function FullZoom__executeSoon(callback) {
    if (!callback)
      return;
    Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
  },
};
