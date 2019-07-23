





































var FullScreen = 
{
  toggle: function()
  {
    
    this.showXULChrome("menubar", window.fullScreen);
    this.showXULChrome("toolbar", window.fullScreen);
    this.showXULChrome("statusbar", window.fullScreen);
  },
  
  showXULChrome: function(aTag, aShow)
  {
    var XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var els = document.getElementsByTagNameNS(XULNS, aTag);
    
    var i;
    for (i = 0; i < els.length; ++i) {
      
      if (els[i].getAttribute("fullscreentoolbar") == "true") {
        this.setToolbarButtonMode(els[i], aShow ? "" : "small");
      } else {
        
        
        if (aShow)
          els[i].removeAttribute("moz-collapsed");
        else
          els[i].setAttribute("moz-collapsed", "true");
      }
    }
    
    var controls = document.getElementsByAttribute("fullscreencontrol", "true");
    for (i = 0; i < controls.length; ++i)
      controls[i].hidden = aShow;
  },
  
  setToolbarButtonMode: function(aToolbar, aMode)
  {
    aToolbar.setAttribute("toolbarmode", aMode);
    this.setToolbarButtonModeFor(aToolbar, "toolbarbutton", aMode);
    this.setToolbarButtonModeFor(aToolbar, "button", aMode);
    this.setToolbarButtonModeFor(aToolbar, "textbox", aMode);
  },
  
  setToolbarButtonModeFor: function(aToolbar, aTag, aMode)
  {
    var XULNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    var els = aToolbar.getElementsByTagNameNS(XULNS, aTag);

    for (var i = 0; i < els.length; ++i) {
      els[i].setAttribute("toolbarmode", aMode);
    }
  }
  
};
