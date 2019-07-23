











































const MOUSE_SCROLL_ZOOM = 3;




var FullZoom = {

  
  

  
  name: "browser.content.full-zoom",

  
  
  
  get globalValue FullZoom_get_globalValue() {
    var globalValue = this._cps.getPref(null, this.name);
    if (typeof globalValue != "undefined")
      globalValue = this._ensureValid(globalValue);
    delete this.globalValue;
    return this.globalValue = globalValue;
  },


  
  

  
  get _cps FullZoom_get__cps() {
    delete this._cps;
    return this._cps = Cc["@mozilla.org/content-pref/service;1"].
                       getService(Ci.nsIContentPrefService);
  },

  
  _siteSpecificPref: undefined,

  
  updateBackgroundTabs: undefined,

  
  _inPrivateBrowsing: false,

  get siteSpecific FullZoom_get_siteSpecific() {
    return !this._inPrivateBrowsing && this._siteSpecificPref;
  },

  
  

  
  interfaces: [Components.interfaces.nsIDOMEventListener,
               Components.interfaces.nsIObserver,
               Components.interfaces.nsIContentPrefObserver,
               Components.interfaces.nsISupportsWeakReference,
               Components.interfaces.nsISupports],

  QueryInterface: function FullZoom_QueryInterface(aIID) {
    if (!this.interfaces.some(function (v) aIID.equals(v)))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },


  
  

  init: function FullZoom_init() {
    
    window.addEventListener("DOMMouseScroll", this, false);

    
    this._cps.addObserver(this.name, this);

    
    
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.addObserver(this, "private-browsing", true);

    
    this._inPrivateBrowsing = Cc["@mozilla.org/privatebrowsing;1"].
                              getService(Ci.nsIPrivateBrowsingService).
                              privateBrowsingEnabled;

    this._siteSpecificPref =
      gPrefService.getBoolPref("browser.zoom.siteSpecific");
    this.updateBackgroundTabs = 
      gPrefService.getBoolPref("browser.zoom.updateBackgroundTabs");
    
    
    gPrefService.addObserver("browser.zoom.", this, true);
  },

  destroy: function FullZoom_destroy() {
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);
    os.removeObserver(this, "private-browsing");
    gPrefService.removeObserver("browser.zoom.", this);
    this._cps.removeObserver(this.name, this);
    window.removeEventListener("DOMMouseScroll", this, false);
    delete this._cps;
  },


  
  

  

  handleEvent: function FullZoom_handleEvent(event) {
    switch (event.type) {
      case "DOMMouseScroll":
        this._handleMouseScrolled(event);
        break;
    }
  },

  _handleMouseScrolled: function FullZoom__handleMouseScrolled(event) {
    
    
    var pref = "mousewheel";
    if (event.axis == event.HORIZONTAL_AXIS)
      pref += ".horizscroll";

    if (event.shiftKey)
      pref += ".withshiftkey";
    else if (event.ctrlKey)
      pref += ".withcontrolkey";
    else if (event.altKey)
      pref += ".withaltkey";
    else if (event.metaKey)
      pref += ".withmetakey";
    else
      pref += ".withnokey";

    pref += ".action";

    
    var isZoomEvent = false;
    try {
      isZoomEvent = (gPrefService.getIntPref(pref) == MOUSE_SCROLL_ZOOM);
    } catch (e) {}
    if (!isZoomEvent)
      return;

    
    
    

    
    
    
    window.setTimeout(function (self) { self._applySettingToPref() }, 0, this);
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
      case "private-browsing":
        switch (aData) {
          case "enter":
            this._inPrivateBrowsing = true;
            break;
          case "exit":
            this._inPrivateBrowsing = false;
            break;
        }
        break;
    }
  },

  

  onContentPrefSet: function FullZoom_onContentPrefSet(aGroup, aName, aValue) {
    if (aGroup == this._cps.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting(aValue);
    else if (aGroup == null) {
      this.globalValue = this._ensureValid(aValue);

      
      
      
      if (!this._cps.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  onContentPrefRemoved: function FullZoom_onContentPrefRemoved(aGroup, aName) {
    if (aGroup == this._cps.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting();
    else if (aGroup == null) {
      this.globalValue = undefined;

      
      
      
      if (!this._cps.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  

  










  onLocationChange: function FullZoom_onLocationChange(aURI, aIsTabSwitch, aBrowser) {
    if (!aURI || (aIsTabSwitch && !this.siteSpecific))
      return;

    
    if (aURI.spec == "about:blank") {
      this._applyPrefToSetting();
      return;
    }

    var self = this;
    this._cps.getPref(aURI, this.name, function(aResult) {
      
      let isSaneURI = (aBrowser && aBrowser.currentURI) ?
        aURI.equals(aBrowser.currentURI) : false;
      if (!aBrowser || isSaneURI)
        self._applyPrefToSetting(aResult, aBrowser);
    });
  },

  

  updateMenu: function FullZoom_updateMenu() {
    var menuItem = document.getElementById("toggle_zoom");

    menuItem.setAttribute("checked", !ZoomManager.useFullZoom);
  },

  
  

  reduce: function FullZoom_reduce() {
    ZoomManager.reduce();
    this._applySettingToPref();
  },

  enlarge: function FullZoom_enlarge() {
    ZoomManager.enlarge();
    this._applySettingToPref();
  },

  reset: function FullZoom_reset() {
    if (typeof this.globalValue != "undefined")
      ZoomManager.zoom = this.globalValue;
    else
      ZoomManager.reset();

    this._removePref();
  },

  


















  _applyPrefToSetting: function FullZoom__applyPrefToSetting(aValue, aBrowser) {
    if ((!this.siteSpecific && !this._inPrivateBrowsing) ||
        gInPrintPreviewMode)
      return;

    var browser = aBrowser || (gBrowser && gBrowser.selectedBrowser);
    try {
      if (browser.contentDocument instanceof Ci.nsIImageDocument ||
          this._inPrivateBrowsing)
        ZoomManager.setZoomForBrowser(browser, 1);
      else if (typeof aValue != "undefined")
        ZoomManager.setZoomForBrowser(browser, this._ensureValid(aValue));
      else if (typeof this.globalValue != "undefined")
        ZoomManager.setZoomForBrowser(browser, this.globalValue);
      else
        ZoomManager.setZoomForBrowser(browser, 1);
    }
    catch(ex) {}
  },

  _applySettingToPref: function FullZoom__applySettingToPref() {
    if (!this.siteSpecific || gInPrintPreviewMode ||
        content.document instanceof Ci.nsIImageDocument)
      return;

    var zoomLevel = ZoomManager.zoom;
    this._cps.setPref(gBrowser.currentURI, this.name, zoomLevel);
  },

  _removePref: function FullZoom__removePref() {
    if (!(content.document instanceof Ci.nsIImageDocument))
      this._cps.removePref(gBrowser.currentURI, this.name);
  },


  
  

  _ensureValid: function FullZoom__ensureValid(aValue) {
    if (isNaN(aValue))
      return 1;

    if (aValue < ZoomManager.MIN)
      return ZoomManager.MIN;

    if (aValue > ZoomManager.MAX)
      return ZoomManager.MAX;

    return aValue;
  }
};
