








































const MOUSE_SCROLL_IS_HORIZONTAL = 1 << 2;



const MOUSE_SCROLL_TEXTSIZE = 3;




var TextZoom = {

  
  

  
  name: "browser.content.text-zoom",

  
  
  
  globalValue: undefined,

  
  minValue: 1,
  maxValue: 2000,
  defaultValue: 100,


  
  

  __zoomManager: null,
  get _zoomManager() {
    if (!this.__zoomManager)
      this.__zoomManager = ZoomManager.prototype.getInstance();
    return this.__zoomManager;
  },

  
  __cps: null,
  get _cps() {
    if (!this.__cps)
      this.__cps = Cc["@mozilla.org/content-pref/service;1"].
                   getService(Ci.nsIContentPrefService);
    return this.__cps;
  },

  
  __prefBranch: null,
  get _prefBranch() {
    if (!this.__prefBranch)
      this.__prefBranch = Cc["@mozilla.org/preferences-service;1"].
                           getService(Ci.nsIPrefBranch);
    return this.__prefBranch;
  },


  
  

  
  interfaces: [Components.interfaces.nsIDOMEventListener,
               Components.interfaces.nsIContentPrefObserver,
               Components.interfaces.nsISupports],

  QueryInterface: function TextZoom_QueryInterface(aIID) {
    if (!this.interfaces.some( function(v) { return aIID.equals(v) } ))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },


  
  

  init: function TextZoom_init() {
    
    window.addEventListener("DOMMouseScroll", this, false);

    
    this._cps.addObserver(this.name, this);

    
    var globalValue = ContentPrefSink.addObserver(this.name, this);
    this.globalValue = this._ensureValid(globalValue);

    
    this._applyPrefToSetting();
  },

  destroy: function TextZoom_destroy() {
    ContentPrefSink.removeObserver(this.name, this);
    this._cps.removeObserver(this.name, this);
    window.removeEventListener("DOMMouseScroll", this, false);

    
    
    for (var i in this) {
      try { this[i] = null }
      
      catch(ex) {}
    }
  },


  
  

  

  handleEvent: function TextZoom_handleEvent(event) {
    
    this._handleMouseScrolled(event);
  },

  _handleMouseScrolled: function TextZoom__handleMouseScrolled(event) {
    
    
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

    
    if (this._getAppPref(pref, null) != MOUSE_SCROLL_TEXTSIZE)
      return;

    
    
    

    
    
    
    window.setTimeout(function() { TextZoom._applySettingToPref() }, 0);
  },

  

  onContentPrefSet: function TextZoom_onContentPrefSet(aGroup, aName, aValue) {
    if (aGroup == this._cps.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting(aValue);
    else if (aGroup == null) {
      this.globalValue = this._ensureValid(aValue);

      
      
      
      if (!this._cps.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  onContentPrefRemoved: function TextZoom_onContentPrefRemoved(aGroup, aName) {
    if (aGroup == this._cps.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting();
    else if (aGroup == null) {
      this.globalValue = undefined;

      
      
      
      if (!this._cps.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  

  onLocationChanged: function TextZoom_onLocationChanged(aURI, aName, aValue) {
    this._applyPrefToSetting(aValue);
  },


  
  

  reduce: function TextZoom_reduce() {
    this._zoomManager.reduce();
    this._applySettingToPref();
  },

  enlarge: function TextZoom_enlarge() {
    this._zoomManager.enlarge();
    this._applySettingToPref();
  },

  reset: function TextZoom_reset() {
    if (typeof this.globalValue != "undefined")
      this._zoomManager.textZoom = this.globalValue;
    else
      this._zoomManager.reset();

    this._removePref();
  },

  
















  _applyPrefToSetting: function TextZoom__applyPrefToSetting(aValue) {
    
    
    try {
      if (typeof aValue != "undefined")
        this._zoomManager.textZoom = this._ensureValid(aValue);
      else if (typeof this.globalValue != "undefined")
        this._zoomManager.textZoom = this.globalValue;
      else
        this._zoomManager.reset();
    }
    catch(ex) {}
  },

  _applySettingToPref: function TextZoom__applySettingToPref() {
    var textZoom = this._zoomManager.textZoom;
    this._cps.setPref(gBrowser.currentURI, this.name, textZoom);
  },

  _removePref: function TextZoom__removePref() {
    this._cps.removePref(gBrowser.currentURI, this.name);
  },


  
  

  _ensureValid: function TextZoom__ensureValid(aValue) {
    if (isNaN(aValue))
      return this.defaultValue;

    if (aValue < this.minValue)
      return this.minValue;

    if (aValue > this.maxValue)
      return this.maxValue;

    return aValue;
  },

  






  _getAppPref: function TextZoom__getAppPref(aPrefName, aDefaultValue) {
    try {
      switch (this._prefBranch.getPrefType(aPrefName)) {
        case this._prefBranch.PREF_STRING:
          return this._prefBranch.getCharPref(aPrefName);

        case this._prefBranch.PREF_BOOL:
          return this._prefBranch.getBoolPref(aPrefName);

        case this._prefBranch.PREF_INT:
          return this._prefBranch.getIntPref(aPrefName);
      }
    }
    catch (ex) {  }
    
    return aDefaultValue;
  }
};
