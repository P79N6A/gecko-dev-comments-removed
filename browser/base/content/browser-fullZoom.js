









const MOUSE_SCROLL_ZOOM = 3;




var FullZoom = {
  
  name: "browser.content.full-zoom",

  
  _siteSpecificPref: undefined,

  
  updateBackgroundTabs: undefined,

  get siteSpecific() {
    return this._siteSpecificPref;
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                         Ci.nsIObserver,
                                         Ci.nsIContentPrefObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  
  

  init: function FullZoom_init() {
    
    window.addEventListener("DOMMouseScroll", this, false);

    
    this._cps2 = Cc["@mozilla.org/content-pref/service;1"].
                 getService(Ci.nsIContentPrefService2);
    this._cps2.addObserverForName(this.name, this);

    this._siteSpecificPref =
      gPrefService.getBoolPref("browser.zoom.siteSpecific");
    this.updateBackgroundTabs =
      gPrefService.getBoolPref("browser.zoom.updateBackgroundTabs");
    
    
    gPrefService.addObserver("browser.zoom.", this, true);
  },

  destroy: function FullZoom_destroy() {
    gPrefService.removeObserver("browser.zoom.", this);
    this._cps2.removeObserverForName(this.name, this);
    window.removeEventListener("DOMMouseScroll", this, false);
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
      isZoomEvent = (gPrefService.getIntPref(pref) == MOUSE_SCROLL_ZOOM);
    } catch (e) {}
    if (!isZoomEvent)
      return;

    
    
    

    
    
    
    
    
    
    
    
    
    
    window.setTimeout(function (self) self._applySettingToPref(true), 0, this);
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
    if (this._ignoreNextOnContentPrefSet) {
      delete this._ignoreNextOnContentPrefSet;
      return;
    }
    this._onContentPrefChanged(aGroup, aValue);
  },

  onContentPrefRemoved: function FullZoom_onContentPrefRemoved(aGroup, aName) {
    this._onContentPrefChanged(aGroup, undefined);
  },

  







  _onContentPrefChanged: function FullZoom__onContentPrefChanged(aGroup, aValue) {
    if (!gBrowser.currentURI)
      return;

    let domain = this._cps2.extractDomain(gBrowser.currentURI.spec);
    if (aGroup) {
      if (aGroup == domain)
        this._applyPrefToSetting(aValue);
      return;
    }

    this._globalValue = aValue === undefined ? aValue :
                          this._ensureValid(aValue);

    
    
    
    let hasPref = false;
    let ctxt = this._loadContextFromWindow(gBrowser.contentWindow);
    this._cps2.getByDomainAndName(gBrowser.currentURI.spec, this.name, ctxt, {
      handleResult: function () hasPref = true,
      handleCompletion: function () {
        if (!hasPref &&
            gBrowser.currentURI &&
            this._cps2.extractDomain(gBrowser.currentURI.spec) == domain)
          this._applyPrefToSetting();
      }.bind(this)
    });
  },

  

  










  onLocationChange: function FullZoom_onLocationChange(aURI, aIsTabSwitch, aBrowser) {
    if (!aURI || (aIsTabSwitch && !this.siteSpecific)) {
      this._notifyOnLocationChange();
      return;
    }

    
    if (aURI.spec == "about:blank") {
      this._applyPrefToSetting(undefined, aBrowser, function () {
        this._notifyOnLocationChange();
      }.bind(this));
      return;
    }

    let browser = aBrowser || gBrowser.selectedBrowser;

    
    if (!aIsTabSwitch && browser.contentDocument.mozSyntheticDocument) {
      ZoomManager.setZoomForBrowser(browser, 1);
      this._notifyOnLocationChange();
      return;
    }

    let ctxt = this._loadContextFromWindow(browser.contentWindow);
    let pref = this._cps2.getCachedByDomainAndName(aURI.spec, this.name, ctxt);
    if (pref) {
      this._applyPrefToSetting(pref.value, browser, function () {
        this._notifyOnLocationChange();
      }.bind(this));
      return;
    }

    let value = undefined;
    this._cps2.getByDomainAndName(aURI.spec, this.name, ctxt, {
      handleResult: function (resultPref) value = resultPref.value,
      handleCompletion: function () {
        if (browser.currentURI &&
            this._cps2.extractDomain(browser.currentURI.spec) ==
              this._cps2.extractDomain(aURI.spec)) {
          this._applyPrefToSetting(value, browser, function () {
            this._notifyOnLocationChange();
          }.bind(this));
        }
      }.bind(this)
    });
  },

  

  updateMenu: function FullZoom_updateMenu() {
    var menuItem = document.getElementById("toggle_zoom");

    menuItem.setAttribute("checked", !ZoomManager.useFullZoom);
  },

  
  

  





  reduce: function FullZoom_reduce(callback) {
    ZoomManager.reduce();
    this._applySettingToPref(false, callback);
  },

  





  enlarge: function FullZoom_enlarge(callback) {
    ZoomManager.enlarge();
    this._applySettingToPref(false, callback);
  },

  






  reset: function FullZoom_reset(callback) {
    this._getGlobalValue(gBrowser.contentWindow, function (value) {
      if (value === undefined)
        ZoomManager.reset();
      else
        ZoomManager.zoom = value;
      this._removePref(callback);
    });
  },

  
























  _applyPrefToSetting: function FullZoom__applyPrefToSetting(aValue, aBrowser, aCallback) {
    if (!this.siteSpecific || gInPrintPreviewMode) {
      this._executeSoon(aCallback);
      return;
    }

    var browser = aBrowser || (gBrowser && gBrowser.selectedBrowser);
    if (browser.contentDocument.mozSyntheticDocument) {
      this._executeSoon(aCallback);
      return;
    }

    if (aValue !== undefined) {
      ZoomManager.setZoomForBrowser(browser, this._ensureValid(aValue));
      this._executeSoon(aCallback);
      return;
    }

    this._getGlobalValue(browser.contentWindow, function (value) {
      if (gBrowser.selectedBrowser == browser)
        ZoomManager.setZoomForBrowser(browser, value === undefined ? 1 : value);
      this._executeSoon(aCallback);
    });
  },

  












  _applySettingToPref: function FullZoom__applySettingToPref(suppressZoomChange, callback) {
    if (!this.siteSpecific ||
        gInPrintPreviewMode ||
        content.document.mozSyntheticDocument) {
      this._executeSoon(callback);
      return;
    }

    this._cps2.set(gBrowser.currentURI.spec, this.name, ZoomManager.zoom,
                   this._loadContextFromWindow(gBrowser.contentWindow), {
      handleCompletion: function () {
        if (suppressZoomChange)
          
          
          
          
          this._ignoreNextOnContentPrefSet = true;
        if (callback)
          callback();
      }.bind(this)
    });
  },

  




  _removePref: function FullZoom__removePref(callback) {
    if (content.document.mozSyntheticDocument) {
      this._executeSoon(callback);
      return;
    }
    let ctxt = this._loadContextFromWindow(gBrowser.contentWindow);
    this._cps2.removeByDomainAndName(gBrowser.currentURI.spec, this.name, ctxt, {
      handleCompletion: function () {
        if (callback)
          callback();
      }
    });
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
      callback.call(this, this._globalValue);
      return;
    }
    let value = undefined;
    this._cps2.getGlobal(this.name, this._loadContextFromWindow(window), {
      handleResult: function (pref) value = pref.value,
      handleCompletion: function () {
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
      Services.obs.notifyObservers(null, "browser-fullZoom:locationChange", "");
    });
  },

  _executeSoon: function FullZoom__executeSoon(callback) {
    if (!callback)
      return;
    Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
  },
};
