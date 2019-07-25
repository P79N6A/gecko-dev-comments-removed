











































const MOUSE_SCROLL_ZOOM = 3;




var FullZoom = {
  
  name: "browser.content.full-zoom",

  
  
  
  get globalValue() {
    var globalValue = Services.contentPrefs.getPref(null, this.name);
    if (typeof globalValue != "undefined")
      globalValue = this._ensureValid(globalValue);
    delete this.globalValue;
    return this.globalValue = globalValue;
  },

  
  _siteSpecificPref: undefined,

  
  updateBackgroundTabs: undefined,

  
  _inPrivateBrowsing: false,

  get siteSpecific() {
    return !this._inPrivateBrowsing && this._siteSpecificPref;
  },

  
  

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMEventListener,
                                         Ci.nsIObserver,
                                         Ci.nsIContentPrefObserver,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsISupports]),

  
  

  init: function FullZoom_init() {
    
    window.addEventListener("DOMMouseScroll", this, false);

    
    Services.contentPrefs.addObserver(this.name, this);

    
    
    Services.obs.addObserver(this, "private-browsing", true);

    
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
    Services.obs.removeObserver(this, "private-browsing");
    gPrefService.removeObserver("browser.zoom.", this);
    Services.contentPrefs.removeObserver(this.name, this);
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
    if (aGroup == Services.contentPrefs.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting(aValue);
    else if (aGroup == null) {
      this.globalValue = this._ensureValid(aValue);

      
      
      
      if (!Services.contentPrefs.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  onContentPrefRemoved: function FullZoom_onContentPrefRemoved(aGroup, aName) {
    if (aGroup == Services.contentPrefs.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting();
    else if (aGroup == null) {
      this.globalValue = undefined;

      
      
      
      if (!Services.contentPrefs.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  

  










  onLocationChange: function FullZoom_onLocationChange(aURI, aIsTabSwitch, aBrowser) {
    if (!aURI || (aIsTabSwitch && !this.siteSpecific))
      return;

    
    if (aURI.spec == "about:blank") {
      this._applyPrefToSetting(undefined, aBrowser);
      return;
    }

    var self = this;
    Services.contentPrefs.getPref(aURI, this.name, function (aResult) {
      
      let browser = aBrowser || gBrowser.selectedBrowser;
      if (aURI.equals(browser.currentURI)) {
        self._applyPrefToSetting(aResult, browser);
      }
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
    Services.contentPrefs.setPref(gBrowser.currentURI, this.name, zoomLevel);
  },

  _removePref: function FullZoom__removePref() {
    if (!(content.document instanceof Ci.nsIImageDocument))
      Services.contentPrefs.removePref(gBrowser.currentURI, this.name);
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
