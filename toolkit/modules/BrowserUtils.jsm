



"use strict";

this.EXPORTED_SYMBOLS = [ "BrowserUtils" ];

const {interfaces: Ci, utils: Cu, classes: Cc} = Components;

Cu.import("resource://gre/modules/Services.jsm");

this.BrowserUtils = {

  














  urlSecurityCheck: function(aURL, aPrincipal, aFlags) {
    var secMan = Services.scriptSecurityManager;
    if (aFlags === undefined) {
      aFlags = secMan.STANDARD;
    }

    try {
      if (aURL instanceof Ci.nsIURI)
        secMan.checkLoadURIWithPrincipal(aPrincipal, aURL, aFlags);
      else
        secMan.checkLoadURIStrWithPrincipal(aPrincipal, aURL, aFlags);
    } catch (e) {
      let principalStr = "";
      try {
        principalStr = " from " + aPrincipal.URI.spec;
      }
      catch(e2) { }

      throw "Load of " + aURL + principalStr + " denied.";
    }
  },

  






  makeURI: function(aURL, aOriginCharset, aBaseURI) {
    return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
  },

  makeFileURI: function(aFile) {
    return Services.io.newFileURI(aFile);
  },

  









  getFocusSync: function(document) {
    let elt = document.commandDispatcher.focusedElement;
    var window = document.commandDispatcher.focusedWindow;

    const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    if (elt instanceof window.XULElement &&
        elt.localName == "browser" &&
        elt.namespaceURI == XUL_NS &&
        elt.getAttribute("remote")) {
      [elt, window] = elt.syncHandler.getFocusedElementAndWindow();
    }

    return [elt, window];
  },

  





  getElementBoundingScreenRect: function(aElement) {
    let rect = aElement.getBoundingClientRect();
    let window = aElement.ownerDocument.defaultView;

    
    
    let fullZoom = window.getInterface(Ci.nsIDOMWindowUtils).fullZoom;
    rect = {
      left: (rect.left + window.mozInnerScreenX) * fullZoom,
      top: (rect.top + window.mozInnerScreenY) * fullZoom,
      width: rect.width * fullZoom,
      height: rect.height * fullZoom
    };

    return rect;
  },

};
