







































var Ci = Components.interfaces;
var Cc = Components.classes;

function SpecialPowers(window) {
  this.window = window;
  bindDOMWindowUtils(this, window);
  this._encounteredCrashDumpFiles = [];
  this._unexpectedCrashDumpFiles = { };
  this._crashDumpDir = null;
  this._pongHandlers = [];
  this._messageListener = this._messageReceived.bind(this);
  addMessageListener("SPPingService", this._messageListener);
}

function bindDOMWindowUtils(sp, window) {
  var util = window.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils);
  
  
  
  
  
  
  
  
  var proto = Object.getPrototypeOf(util);
  var target = {};
  function rebind(desc, prop) {
    if (prop in desc && typeof(desc[prop]) == "function") {
      var oldval = desc[prop];
      desc[prop] = function() { return oldval.apply(util, arguments); };
    }
  }
  for (var i in proto) {
    var desc = Object.getOwnPropertyDescriptor(proto, i);
    rebind(desc, "get");
    rebind(desc, "set");
    rebind(desc, "value");
    Object.defineProperty(target, i, desc);
  }
  sp.DOMWindowUtils = target;
}

SpecialPowers.prototype = {
  toString: function() { return "[SpecialPowers]"; },
  sanityCheck: function() { return "foo"; },

  
  DOMWindowUtils: undefined,

  
  getBoolPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'BOOL'));
  },
  getIntPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'INT'));
  },
  getCharPref: function(aPrefName) {
    return (this._getPref(aPrefName, 'CHAR'));
  },
  getComplexValue: function(aPrefName, aIid) {
    return (this._getPref(aPrefName, 'COMPLEX', aIid));
  },

  
  setBoolPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'BOOL', aValue));
  },
  setIntPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'INT', aValue));
  },
  setCharPref: function(aPrefName, aValue) {
    return (this._setPref(aPrefName, 'CHAR', aValue));
  },
  setComplexValue: function(aPrefName, aIid, aValue) {
    return (this._setPref(aPrefName, 'COMPLEX', aValue, aIid));
  },

  
  clearUserPref: function(aPrefName) {
    var msg = {'op':'clear', 'prefName': aPrefName, 'prefType': ""};
    sendSyncMessage('SPPrefService', msg);
  },

  
  _getPref: function(aPrefName, aPrefType, aIid) {
    var msg = {};
    if (aIid) {
      
      msg = {'op':'get', 'prefName': aPrefName, 'prefType':aPrefType, 'prefValue':[aIid]};
    } else {
      msg = {'op':'get', 'prefName': aPrefName,'prefType': aPrefType};
    }
    return(sendSyncMessage('SPPrefService', msg)[0]);
  },
  _setPref: function(aPrefName, aPrefType, aValue, aIid) {
    var msg = {};
    if (aIid) {
      msg = {'op':'set','prefName':aPrefName, 'prefType': aPrefType, 'prefValue': [aIid,aValue]};
    } else {
      msg = {'op':'set', 'prefName': aPrefName, 'prefType': aPrefType, 'prefValue': aValue};
    }
    return(sendSyncMessage('SPPrefService', msg)[0]);
  },

  
  
  _getTopChromeWindow: function(window) {
    return window.QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIWebNavigation)
                 .QueryInterface(Ci.nsIDocShellTreeItem)
                 .rootTreeItem
                 .QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIDOMWindow)
                 .QueryInterface(Ci.nsIDOMChromeWindow);
  },
  _getDocShell: function(window) {
    return window.QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIWebNavigation)
                 .QueryInterface(Ci.nsIDocShell);
  },
  _getMUDV: function(window) {
    return this._getDocShell(window).contentViewer
               .QueryInterface(Ci.nsIMarkupDocumentViewer);
  },
  _getAutoCompletePopup: function(window) {
    return this._getTopChromeWindow(window).document
                                           .getElementById("PopupAutoComplete");
  },
  addAutoCompletePopupEventListener: function(window, listener) {
    this._getAutoCompletePopup(window).addEventListener("popupshowing",
                                                        listener,
                                                        false);
  },
  removeAutoCompletePopupEventListener: function(window, listener) {
    this._getAutoCompletePopup(window).removeEventListener("popupshowing",
                                                           listener,
                                                           false);
  },
  isBackButtonEnabled: function(window) {
    return !this._getTopChromeWindow(window).document
                                      .getElementById("Browser:Back")
                                      .hasAttribute("disabled");
  },

  addChromeEventListener: function(type, listener, capture, allowUntrusted) {
    addEventListener(type, listener, capture, allowUntrusted);
  },
  removeChromeEventListener: function(type, listener, capture) {
    removeEventListener(type, listener, capture);
  },

  getFullZoom: function(window) {
    return this._getMUDV(window).fullZoom;
  },
  setFullZoom: function(window, zoom) {
    this._getMUDV(window).fullZoom = zoom;
  },
  getTextZoom: function(window) {
    return this._getMUDV(window).textZoom;
  },
  setTextZoom: function(window, zoom) {
    this._getMUDV(window).textZoom = zoom;
  },

  createSystemXHR: function() {
    return Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
             .createInstance(Ci.nsIXMLHttpRequest);
  },

  gc: function() {
    this.DOMWindowUtils.garbageCollect();
  },

  hasContentProcesses: function() {
    try {
      var rt = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);
      return rt.processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
    } catch (e) {
      return true;
    }
  },

  registerProcessCrashObservers: function() {
    addMessageListener("SPProcessCrashService", this._messageListener);
    sendSyncMessage("SPProcessCrashService", { op: "register-observer" });
  },

  _messageReceived: function(aMessage) {
    switch (aMessage.name) {
      case "SPProcessCrashService":
        if (aMessage.json.type == "crash-observed") {
          var self = this;
          aMessage.json.dumpIDs.forEach(function(id) {
            self._encounteredCrashDumpFiles.push(id + ".dmp");
            self._encounteredCrashDumpFiles.push(id + ".extra");
          });
        }
        break;

      case "SPPingService":
        if (aMessage.json.op == "pong") {
          var handler = this._pongHandlers.shift();
          if (handler) {
            handler();
          }
        }
        break;
    }
    return true;
  },

  removeExpectedCrashDumpFiles: function(aExpectingProcessCrash) {
    var success = true;
    if (aExpectingProcessCrash) {
      var message = {
        op: "delete-crash-dump-files",
        filenames: this._encounteredCrashDumpFiles 
      };
      if (!sendSyncMessage("SPProcessCrashService", message)[0]) {
        success = false;
      }
    }
    this._encounteredCrashDumpFiles.length = 0;
    return success;
  },

  findUnexpectedCrashDumpFiles: function() {
    var self = this;
    var message = {
      op: "find-crash-dump-files",
      crashDumpFilesToIgnore: this._unexpectedCrashDumpFiles
    };
    var crashDumpFiles = sendSyncMessage("SPProcessCrashService", message)[0];
    crashDumpFiles.forEach(function(aFilename) {
      self._unexpectedCrashDumpFiles[aFilename] = true;
    });
    return crashDumpFiles;
  },

  executeAfterFlushingMessageQueue: function(aCallback) {
    this._pongHandlers.push(aCallback);
    sendAsyncMessage("SPPingService", { op: "ping" });
  },

  executeSoon: function(aFunc) {
    var tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
    tm.mainThread.dispatch({
      run: function() {
        aFunc();
      }
    }, Ci.nsIThread.DISPATCH_NORMAL);
  },

  addSystemEventListener: function(target, type, listener, useCapture) {
    Components.classes["@mozilla.org/eventlistenerservice;1"].
      getService(Components.interfaces.nsIEventListenerService).
      addSystemEventListener(target, type, listener, useCapture);
  },
  removeSystemEventListener: function(target, type, listener, useCapture) {
    Components.classes["@mozilla.org/eventlistenerservice;1"].
      getService(Components.interfaces.nsIEventListenerService).
      removeSystemEventListener(target, type, listener, useCapture);
  }
};



SpecialPowers.prototype.__exposedProps__ = {};
for each (i in Object.keys(SpecialPowers.prototype).filter(function(v) {return v.charAt(0) != "_";})) {
  SpecialPowers.prototype.__exposedProps__[i] = "r";
}


function attachSpecialPowersToWindow(aWindow) {
  try {
    if ((aWindow !== null) &&
        (aWindow !== undefined) &&
        (aWindow.wrappedJSObject) &&
        !(aWindow.wrappedJSObject.SpecialPowers)) {
      aWindow.wrappedJSObject.SpecialPowers = new SpecialPowers(aWindow);
    }
  } catch(ex) {
    dump("TEST-INFO | specialpowers.js |  Failed to attach specialpowers to window exception: " + ex + "\n");
  }
}






function SpecialPowersManager() {
  addEventListener("DOMWindowCreated", this, false);
}

SpecialPowersManager.prototype = {
  handleEvent: function handleEvent(aEvent) {
    var window = aEvent.target.defaultView;

    
    
    
    
    var uri = window.document.documentURIObject;
    if (uri.scheme === "chrome" || uri.spec.split(":")[0] == "about") {
      return;
    }

    attachSpecialPowersToWindow(window);
  }
};

var specialpowersmanager = new SpecialPowersManager();
