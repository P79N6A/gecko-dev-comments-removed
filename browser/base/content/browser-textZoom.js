









































const MOUSE_SCROLL_IS_HORIZONTAL = 1 << 2;



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

  get _prefBranch FullZoom_get__prefBranch() {
    delete this._prefBranch;
    return this._prefBranch = Cc["@mozilla.org/preferences-service;1"].
                              getService(Ci.nsIPrefBranch2);
  },

  
  siteSpecific: undefined,


  
  

  
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

    
    
    this.siteSpecific =
      this._prefBranch.getBoolPref("browser.zoom.siteSpecific");
    this._prefBranch.addObserver("browser.zoom.siteSpecific", this, true);
  },

  destroy: function FullZoom_destroy() {
    this._prefBranch.removeObserver("browser.zoom.siteSpecific", this);
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
    if (event.scrollFlags & MOUSE_SCROLL_IS_HORIZONTAL)
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
    switch(aTopic) {
      case "nsPref:changed":
        switch(aData) {
          case "browser.zoom.siteSpecific":
            this.siteSpecific =
              this._prefBranch.getBoolPref("browser.zoom.siteSpecific");
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

  

  onLocationChange: function FullZoom_onLocationChange(aURI) {
    if (!aURI)
      return;
    this._applyPrefToSetting(this._cps.getPref(aURI, this.name));
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

  setSettingValue: function FullZoom_setSettingValue() {
    var value = this._cps.getPref(gBrowser.currentURI, this.name);
    this._applyPrefToSetting(value);
  },

  


















  _applyPrefToSetting: function FullZoom__applyPrefToSetting(aValue) {
    if (!this.siteSpecific || gInPrintPreviewMode)
      return;

    try {
      if (typeof aValue != "undefined")
        ZoomManager.zoom = this._ensureValid(aValue);
      else if (typeof this.globalValue != "undefined")
        ZoomManager.zoom = this.globalValue;
      else
        ZoomManager.zoom = 1;
    }
    catch(ex) {}
  },

  _applySettingToPref: function FullZoom__applySettingToPref() {
    if (!this.siteSpecific || gInPrintPreviewMode)
      return;

    var zoomLevel = ZoomManager.zoom;
    this._cps.setPref(gBrowser.currentURI, this.name, zoomLevel);
  },

  _removePref: function FullZoom__removePref() {
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
