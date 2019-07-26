









const MOUSE_SCROLL_ZOOM = 3;

Cu.import('resource://gre/modules/ContentPrefInstance.jsm');

function getContentPrefs(aWindow) {
  let context = aWindow ? aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                                 .getInterface(Ci.nsIWebNavigation)
                                 .QueryInterface(Ci.nsILoadContext) : null;
  return new ContentPrefInstance(context);
}




var FullZoom = {
  
  name: "browser.content.full-zoom",

  
  
  
  get globalValue() {
    var globalValue = getContentPrefs(gBrowser.contentDocument.defaultView).getPref(null, this.name);
    if (typeof globalValue != "undefined")
      globalValue = this._ensureValid(globalValue);
    delete this.globalValue;
    return this.globalValue = globalValue;
  },

  
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

    
    getContentPrefs().addObserver(this.name, this);

    this._siteSpecificPref =
      gPrefService.getBoolPref("browser.zoom.siteSpecific");
    this.updateBackgroundTabs =
      gPrefService.getBoolPref("browser.zoom.updateBackgroundTabs");
    
    
    gPrefService.addObserver("browser.zoom.", this, true);
  },

  destroy: function FullZoom_destroy() {
    gPrefService.removeObserver("browser.zoom.", this);
    getContentPrefs().removeObserver(this.name, this);
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
    }
  },

  

  onContentPrefSet: function FullZoom_onContentPrefSet(aGroup, aName, aValue) {
    let contentPrefs = getContentPrefs(gBrowser.contentDocument.defaultView);
    if (aGroup == contentPrefs.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting(aValue);
    else if (aGroup == null) {
      this.globalValue = this._ensureValid(aValue);

      
      
      
      if (!contentPrefs.hasPref(gBrowser.currentURI, this.name))
        this._applyPrefToSetting();
    }
  },

  onContentPrefRemoved: function FullZoom_onContentPrefRemoved(aGroup, aName) {
    let contentPrefs = getContentPrefs(gBrowser.contentDocument.defaultView);
    if (aGroup == contentPrefs.grouper.group(gBrowser.currentURI))
      this._applyPrefToSetting();
    else if (aGroup == null) {
      this.globalValue = undefined;

      
      
      
      if (!contentPrefs.hasPref(gBrowser.currentURI, this.name))
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

    let browser = aBrowser || gBrowser.selectedBrowser;

    
    if (!aIsTabSwitch && browser.contentDocument.mozSyntheticDocument) {
      ZoomManager.setZoomForBrowser(browser, 1);
      return;
    }

    let contentPrefs = getContentPrefs(gBrowser.contentDocument.defaultView);
    if (contentPrefs.hasCachedPref(aURI, this.name)) {
      let zoomValue = contentPrefs.getPref(aURI, this.name);
      this._applyPrefToSetting(zoomValue, browser);
    } else {
      var self = this;
      contentPrefs.getPref(aURI, this.name, function (aResult) {
        
        
        
        if (browser.currentURI && aURI.equals(browser.currentURI)) {
          self._applyPrefToSetting(aResult, browser);
        }
      });
    }
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
    if ((!this.siteSpecific) || gInPrintPreviewMode)
      return;

    var browser = aBrowser || (gBrowser && gBrowser.selectedBrowser);
    try {
      if (browser.contentDocument.mozSyntheticDocument)
        return;

      if (typeof aValue != "undefined")
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
        content.document.mozSyntheticDocument)
      return;

    var zoomLevel = ZoomManager.zoom;
    getContentPrefs(gBrowser.contentDocument.defaultView).setPref(gBrowser.currentURI, this.name, zoomLevel);
  },

  _removePref: function FullZoom__removePref() {
    if (!(content.document.mozSyntheticDocument))
      getContentPrefs(gBrowser.contentDocument.defaultView).removePref(gBrowser.currentURI, this.name);
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
