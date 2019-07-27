




"use strict";

this.EXPORTED_SYMBOLS = [ "BrowserUtils" ];

const {interfaces: Ci, utils: Cu, classes: Cc} = Components;

Cu.import("resource://gre/modules/Services.jsm");

this.BrowserUtils = {

  


  dumpLn: function (...args) {
    for (let a of args)
      dump(a + " ");
    dump("\n");
  },

  



  restartApplication: function() {
    let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                       .getService(Ci.nsIAppStartup);
    
    if (Services.appinfo.inSafeMode) {
      appStartup.restartInSafeMode(Ci.nsIAppStartup.eAttemptQuit | Ci.nsIAppStartup.eRestart);
      return;
    }
    appStartup.quit(Ci.nsIAppStartup.eAttemptQuit | Ci.nsIAppStartup.eRestart);
  },

  














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

  makeURIFromCPOW: function(aCPOWURI) {
    return Services.io.newURI(aCPOWURI.spec, aCPOWURI.originCharset, null);
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

  







  offsetToTopLevelWindow: function (aTopLevelWindow, aElement) {
    let offsetX = 0;
    let offsetY = 0;
    let element = aElement;
    while (element &&
           element.ownerDocument &&
           element.ownerDocument.defaultView != aTopLevelWindow) {
      element = element.ownerDocument.defaultView.frameElement;
      let rect = element.getBoundingClientRect();
      offsetX += rect.left;
      offsetY += rect.top;
    }
    let win = null;
    if (element == aElement)
      win = aTopLevelWindow;
    else
      win = element.contentDocument.defaultView;
    return { targetWindow: win, offsetX: offsetX, offsetY: offsetY };
  },

  onBeforeLinkTraversal: function(originalTarget, linkURI, linkNode, isAppTab) {
    
    
    if (originalTarget != "" || !isAppTab)
      return originalTarget;

    
    
    let linkHost;
    let docHost;
    try {
      linkHost = linkURI.host;
      docHost = linkNode.ownerDocument.documentURIObject.host;
    } catch(e) {
      
      
      return originalTarget;
    }

    if (docHost == linkHost)
      return originalTarget;

    
    let [longHost, shortHost] =
      linkHost.length > docHost.length ? [linkHost, docHost] : [docHost, linkHost];
    if (longHost == "www." + shortHost)
      return originalTarget;

    return "_blank";
  },

  





  makeNicePluginName: function (aName) {
    if (aName == "Shockwave Flash")
      return "Adobe Flash";
    
    if (/^Java\W/.exec(aName))
      return "Java";

    
    
    
    
    
    
    let newName = aName.replace(/\(.*?\)/g, "").
                        replace(/[\s\d\.\-\_\(\)]+$/, "").
                        replace(/\bplug-?in\b/i, "").trim();
    return newName;
  },

  





  linkHasNoReferrer: function (linkNode) {
    
    
    
    if (!linkNode)
      return true;

    let rel = linkNode.getAttribute("rel");
    if (!rel)
      return false;

    
    
    let values = rel.split(/[ \t\r\n\f]/);
    return values.indexOf('noreferrer') != -1;
  },

  





  mimeTypeIsTextBased: function(mimeType) {
    return mimeType.startsWith("text/") ||
           mimeType.endsWith("+xml") ||
           mimeType == "application/x-javascript" ||
           mimeType == "application/javascript" ||
           mimeType == "application/json" ||
           mimeType == "application/xml" ||
           mimeType == "mozilla.application/cached-xul";
  },

  








  shouldFastFind: function(elt, win) {
    if (elt) {
      if (elt instanceof win.HTMLInputElement && elt.mozIsTextField(false))
        return false;

      if (elt.isContentEditable)
        return false;

      if (elt instanceof win.HTMLTextAreaElement ||
          elt instanceof win.HTMLSelectElement ||
          elt instanceof win.HTMLObjectElement ||
          elt instanceof win.HTMLEmbedElement)
        return false;
    }

    if (win && !this.mimeTypeIsTextBased(win.document.contentType))
      return false;

    
    let loc = win.location;
    if (loc.href == "about:blank")
      return false;

    
    if ((loc.protocol == "about:" || loc.protocol == "chrome:") &&
        (win && win.document.documentElement &&
         win.document.documentElement.getAttribute("disablefastfind") == "true"))
      return false;

    if (win) {
      try {
        let editingSession = win.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
          .getInterface(Components.interfaces.nsIWebNavigation)
          .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
          .getInterface(Components.interfaces.nsIEditingSession);
        if (editingSession.windowIsEditable(win))
          return false;
      }
      catch (e) {
        Cu.reportError(e);
        
      }
    }
    return true;
  },
};
