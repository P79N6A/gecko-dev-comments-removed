









































const MOUSE_SCROLL_IS_HORIZONTAL = 1 << 2;



const MOUSE_SCROLL_FULLZOOM = 5;




var FullZoom = {

  
  

  
  name: "browser.content.full-zoom",

  
  
  
  globalValue: undefined,


  
  

  
  get _cps() {
    delete this._cps;
    return this._cps = Cc["@mozilla.org/content-pref/service;1"].
                       getService(Ci.nsIContentPrefService);
  },


  
  

  
  interfaces: [Components.interfaces.nsIDOMEventListener,
               Components.interfaces.nsIContentPrefObserver,
               Components.interfaces.nsISupports],

  QueryInterface: function (aIID) {
    if (!this.interfaces.some(function (v) aIID.equals(v)))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },


  
  

  init: function () {
    
    window.addEventListener("DOMMouseScroll", this, false);

    
    this._cps.addObserver(this.name, this);

    
    var globalValue = ContentPrefSink.addObserver(this.name, this);
    this.globalValue = this._ensureValid(globalValue);

    
    this._applyPrefToSetting();
  },

  destroy: function () {
    ContentPrefSink.removeObserver(this.name, this);
    this._cps.removeObserver(this.name, this);
    window.removeEventListener("DOMMouseScroll", this, false);

    
    
    for (var i in this) {
      try { this[i] = null }
      
      catch(ex) {}
    }
  },


  
  

  

  handleEvent: function (event) {
    switch (event.type) {
      case "DOMMouseScroll":
        this._handleMouseScrolled(event);
        break;
    }
  },

  _handleMouseScrolled: function (event) {
    
    
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
      isZoomEvent = (gPrefService.getIntPref(pref) == MOUSE_SCROLL_FULLZOOM);
    } catch (e) {}
    if (!isZoomEvent)
      return;

    
    
    

    
    
    
    window.setTimeout(function (self) { self._applySettingToPref() }, 0, this);
  },

  

  onContentPrefSet: function (aGroup, aName, aValue) {
    if (aGroup == this._cps.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting(aValue);
    else if (aGroup == null) {
      this.globalValue = this._ensureValid(aValue);

      
      
      
      if (!this._cps.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  onContentPrefRemoved: function (aGroup, aName) {
    if (aGroup == this._cps.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting();
    else if (aGroup == null) {
      this.globalValue = undefined;

      
      
      
      if (!this._cps.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  

  onLocationChanged: function (aURI, aName, aValue) {
    this._applyPrefToSetting(aValue);
  },


  
  

  reduce: function () {
    ZoomManager.reduce();
    this._applySettingToPref();
  },

  enlarge: function () {
    ZoomManager.enlarge();
    this._applySettingToPref();
  },

  reset: function () {
    if (typeof this.globalValue != "undefined")
      ZoomManager.fullZoom = this.globalValue;
    else
      ZoomManager.reset();

    this._removePref();
  },

  setSettingValue: function () {
    var value = this._cps.getPref(gBrowser.currentURI, this.name);
    this._applyPrefToSetting(value);
  },

  
















  _applyPrefToSetting: function (aValue) {
    if (gInPrintPreviewMode)
      return;

    
    
    try {
      if (typeof aValue != "undefined")
        ZoomManager.fullZoom = this._ensureValid(aValue);
      else if (typeof this.globalValue != "undefined")
        ZoomManager.fullZoom = this.globalValue;
      else
        ZoomManager.reset();
    }
    catch(ex) {}
  },

  _applySettingToPref: function () {
    if (gInPrintPreviewMode)
      return;

    var fullZoom = ZoomManager.fullZoom;
    this._cps.setPref(gBrowser.currentURI, this.name, fullZoom);
  },

  _removePref: function () {
    this._cps.removePref(gBrowser.currentURI, this.name);
  },


  
  

  _ensureValid: function (aValue) {
    if (isNaN(aValue))
      return 1;

    if (aValue < ZoomManager.MIN)
      return ZoomManager.MIN;

    if (aValue > ZoomManager.MAX)
      return ZoomManager.MAX;

    return aValue;
  }
};
