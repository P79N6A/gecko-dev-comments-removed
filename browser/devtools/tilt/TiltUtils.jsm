




"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");

const STACK_THICKNESS = 15;

this.EXPORTED_SYMBOLS = ["TiltUtils"];




this.TiltUtils = {};




TiltUtils.Output = {

  





  log: function TUO_log(aMessage)
  {
    if (this.suppressLogs) {
      return;
    }
    
    let consoleService = Cc["@mozilla.org/consoleservice;1"]
      .getService(Ci.nsIConsoleService);

    
    consoleService.logStringMessage(aMessage);
  },

  







  error: function TUO_error(aMessage, aProperties)
  {
    if (this.suppressErrors) {
      return;
    }
    
    aProperties = aProperties || {};

    
    let consoleService = Cc["@mozilla.org/consoleservice;1"]
      .getService(Ci.nsIConsoleService);

    
    let scriptError = Cc["@mozilla.org/scripterror;1"]
      .createInstance(Ci.nsIScriptError);

    
    scriptError.init(aMessage,
      aProperties.sourceName || "",
      aProperties.sourceLine || "",
      aProperties.lineNumber || 0,
      aProperties.columnNumber || 0,
      aProperties.flags || 0,
      aProperties.category || "");

    
    consoleService.logMessage(scriptError);
  },

  







  alert: function TUO_alert(aTitle, aMessage)
  {
    if (this.suppressAlerts) {
      return;
    }
    if (!aMessage) {
      aMessage = aTitle;
      aTitle = "";
    }

    
    let prompt = Cc["@mozilla.org/embedcomp/prompt-service;1"]
      .getService(Ci.nsIPromptService);

    
    prompt.alert(null, aTitle, aMessage);
  }
};




TiltUtils.Preferences = {

  











  get: function TUP_get(aPref, aType)
  {
    if (!aPref || !aType) {
      return;
    }

    try {
      let prefs = this._branch;

      switch(aType) {
        case "boolean":
          return prefs.getBoolPref(aPref);
        case "string":
          return prefs.getCharPref(aPref);
        case "integer":
          return prefs.getIntPref(aPref);
      }
      return null;

    } catch(e) {
      
      TiltUtils.Output.error(e.message);
      return undefined;
    }
  },

  












  set: function TUP_set(aPref, aType, aValue)
  {
    if (!aPref || !aType || aValue === undefined || aValue === null) {
      return;
    }

    try {
      let prefs = this._branch;

      switch(aType) {
        case "boolean":
          return prefs.setBoolPref(aPref, aValue);
        case "string":
          return prefs.setCharPref(aPref, aValue);
        case "integer":
          return prefs.setIntPref(aPref, aValue);
      }
    } catch(e) {
      
      TiltUtils.Output.error(e.message);
    }
    return false;
  },

  












  create: function TUP_create(aPref, aType, aValue)
  {
    if (!aPref || !aType || aValue === undefined || aValue === null) {
      return;
    }

    try {
      let prefs = this._branch;

      if (!prefs.prefHasUserValue(aPref)) {
        switch(aType) {
          case "boolean":
            return prefs.setBoolPref(aPref, aValue);
          case "string":
            return prefs.setCharPref(aPref, aValue);
          case "integer":
            return prefs.setIntPref(aPref, aValue);
        }
      }
    } catch(e) {
      
      TiltUtils.Output.error(e.message);
    }
    return false;
  },

  


  _branch: (function(aBranch) {
    return Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefService)
      .getBranch(aBranch);

  }("devtools.tilt."))
};




TiltUtils.L10n = {

  


  stringBundle: null,

  








  get: function TUL_get(aName)
  {
    
    if (!this.stringBundle || !aName) {
      return null;
    }
    return this.stringBundle.GetStringFromName(aName);
  },

  










  format: function TUL_format(aName, aArgs)
  {
    
    if (!this.stringBundle || !aName || !aArgs) {
      return null;
    }
    return this.stringBundle.formatStringFromName(aName, aArgs, aArgs.length);
  }
};




TiltUtils.DOM = {

  


  parentNode: null,

  






















  initCanvas: function TUD_initCanvas(aParentNode, aProperties)
  {
    
    if (!(aParentNode = aParentNode || this.parentNode)) {
      return null;
    }

    
    aProperties = aProperties || {};

    
    this.parentNode = aParentNode;

    
    let canvas = aParentNode.ownerDocument.
      createElementNS("http://www.w3.org/1999/xhtml", "canvas");

    let width = aProperties.width || aParentNode.clientWidth;
    let height = aProperties.height || aParentNode.clientHeight;
    let id = aProperties.id || null;

    canvas.setAttribute("style", "min-width: 1px; min-height: 1px;");
    canvas.setAttribute("width", width);
    canvas.setAttribute("height", height);
    canvas.setAttribute("id", id);

    
    if (aProperties.focusable) {
      canvas.setAttribute("tabindex", "1");
      canvas.style.outline = "none";
    }

    
    if (aProperties.append) {
      aParentNode.appendChild(canvas);
    }

    return canvas;
  },

  







  getContentWindowDimensions: function TUD_getContentWindowDimensions(
    aContentWindow)
  {
    return {
      width: aContentWindow.innerWidth + aContentWindow.scrollMaxX,
      height: aContentWindow.innerHeight + aContentWindow.scrollMaxY
    };
  },

  


























  getNodePosition: function TUD_getNodePosition(aContentWindow, aNode,
                                                aParentPosition) {
    
    let coord = LayoutHelpers.getRect(aNode, aContentWindow);
    if (!coord) {
      return null;
    }

    coord.depth = aParentPosition ? (aParentPosition.depth + aParentPosition.thickness) : 0;
    coord.thickness = STACK_THICKNESS;

    return coord;
  },

  

















  traverse: function TUD_traverse(aContentWindow, aProperties)
  {
    
    aProperties = aProperties || {};

    let aInvisibleElements = aProperties.invisibleElements || {};
    let aMinSize = aProperties.minSize || -1;
    let aMaxX = aProperties.maxX || Number.MAX_VALUE;
    let aMaxY = aProperties.maxY || Number.MAX_VALUE;

    let nodes = aContentWindow.document.childNodes;
    let store = { info: [], nodes: [] };
    let depth = 0;

    let queue = [
      { parentPosition: null, nodes: aContentWindow.document.childNodes }
    ]

    while (queue.length) {
      let { nodes, parentPosition } = queue.shift();

      for (let node of nodes) {
        
        let name = node.localName;
        if (!name || aInvisibleElements[name]) {
          continue;
        }

        let coord = this.getNodePosition(aContentWindow, node, parentPosition);
        if (!coord) {
          continue;
        }

        
        if (coord.left > aMaxX || coord.top > aMaxY) {
          continue;
        }

        
        if (coord.width > aMinSize && coord.height > aMinSize) {

          
          store.info.push({ coord: coord, name: name });
          store.nodes.push(node);
        }

        let childNodes = (name === "iframe" || name === "frame") ? node.contentDocument.childNodes : node.childNodes;
        if (childNodes.length > 0)
          queue.push({ parentPosition: coord, nodes: childNodes });
      }
    }

    return store;
  }
};












TiltUtils.bindObjectFunc = function TU_bindObjectFunc(aScope, aRegex, aParent)
{
  if (!aScope) {
    return;
  }

  for (let i in aScope) {
    try {
      if ("function" === typeof aScope[i] && (aRegex ? i.match(aRegex) : 1)) {
        aScope[i] = aScope[i].bind(aParent || aScope);
      }
    } catch(e) {
      TiltUtils.Output.error(e);
    }
  }
};







TiltUtils.destroyObject = function TU_destroyObject(aScope)
{
  if (!aScope) {
    return;
  }

  
  if ("function" === typeof aScope._finalize) {
    aScope._finalize();
  }
  for (let i in aScope) {
    if (aScope.hasOwnProperty(i)) {
      delete aScope[i];
    }
  }
};









TiltUtils.getWindowId = function TU_getWindowId(aWindow)
{
  if (!aWindow) {
    return;
  }

  return aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                .getInterface(Ci.nsIDOMWindowUtils)
                .currentInnerWindowID;
};









TiltUtils.setDocumentZoom = function TU_setDocumentZoom(aChromeWindow, aZoom) {
  aChromeWindow.gBrowser.selectedBrowser.markupDocumentViewer.fullZoom = aZoom;
};







TiltUtils.gc = function TU_gc(aChromeWindow)
{
  aChromeWindow.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIDOMWindowUtils)
               .garbageCollect();
};




TiltUtils.clearCache = function TU_clearCache()
{
  TiltUtils.DOM.parentNode = null;
};


TiltUtils.bindObjectFunc(TiltUtils.Output);
TiltUtils.bindObjectFunc(TiltUtils.Preferences);
TiltUtils.bindObjectFunc(TiltUtils.L10n);
TiltUtils.bindObjectFunc(TiltUtils.DOM);


XPCOMUtils.defineLazyGetter(TiltUtils.L10n, "stringBundle", function() {
  return Services.strings.createBundle(
    "chrome://browser/locale/devtools/tilt.properties");
});
