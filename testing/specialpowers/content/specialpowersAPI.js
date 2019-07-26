






var Ci = Components.interfaces;
var Cc = Components.classes;
var Cu = Components.utils;

Cu.import("resource://specialpowers/MockFilePicker.jsm");
Cu.import("resource://specialpowers/MockPermissionPrompt.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PrivateBrowsingUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function SpecialPowersAPI() {
  this._consoleListeners = [];
  this._encounteredCrashDumpFiles = [];
  this._unexpectedCrashDumpFiles = { };
  this._crashDumpDir = null;
  this._mfl = null;
  this._prefEnvUndoStack = [];
  this._pendingPrefs = [];
  this._applyingPrefs = false;
  this._fm = null;
  this._cb = null;
}

function bindDOMWindowUtils(aWindow) {
  if (!aWindow)
    return

  var util = aWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                   .getInterface(Components.interfaces.nsIDOMWindowUtils);
  
  
  
  
  
  
  
  
  
  
  
  
  var proto = Object.getPrototypeOf(util);
  var target = { __exposedProps__: {} };
  function rebind(desc, prop) {
    if (prop in desc && typeof(desc[prop]) == "function") {
      var oldval = desc[prop];
      try {
        desc[prop] = function() {
          return oldval.apply(util, arguments);
        };
      } catch (ex) {
        dump("WARNING: Special Powers failed to rebind function: " + desc + "::" + prop + "\n");
      }
    }
  }
  for (var i in proto) {
    var desc = Object.getOwnPropertyDescriptor(proto, i);
    rebind(desc, "get");
    rebind(desc, "set");
    rebind(desc, "value");
    Object.defineProperty(target, i, desc);
    target.__exposedProps__[i] = 'rw';
  }
  return target;
}

function getRawComponents(aWindow) {
  return Cu.getComponentsForScope(aWindow);
}

function isWrappable(x) {
  if (typeof x === "object")
    return x !== null;
  return typeof x === "function";
};

function isWrapper(x) {
  return isWrappable(x) && (typeof x.SpecialPowers_wrappedObject !== "undefined");
};

function unwrapIfWrapped(x) {
  return isWrapper(x) ? unwrapPrivileged(x) : x;
};

function isXrayWrapper(x) {
  return Cu.isXrayWrapper(x);
}

function callGetOwnPropertyDescriptor(obj, name) {
  
  
  
  
  
  
  
  try {
    obj.__lookupGetter__(name);
    obj.__lookupSetter__(name);
  } catch(e) { }
  return Object.getOwnPropertyDescriptor(obj, name);
}



function doApply(fun, invocant, args) {
  return Function.prototype.apply.call(fun, invocant, args);
}

function wrapPrivileged(obj) {

  
  if (!isWrappable(obj))
    return obj;

  
  if (isWrapper(obj))
    throw "Trying to double-wrap object!";

  
  var handler = new SpecialPowersHandler(obj);

  
  if (typeof obj === "function") {
    var callTrap = function() {
      
      var invocant = unwrapIfWrapped(this);
      var unwrappedArgs = Array.prototype.slice.call(arguments).map(unwrapIfWrapped);

      return wrapPrivileged(doApply(obj, invocant, unwrappedArgs));
    };
    var constructTrap = function() {
      
      var unwrappedArgs = Array.prototype.slice.call(arguments).map(unwrapIfWrapped);

      
      
      
      
      
      
      
      var FakeConstructor = function() {
        return doApply(obj, this, unwrappedArgs);
      };
      FakeConstructor.prototype = obj.prototype;

      return wrapPrivileged(new FakeConstructor());
    };

    return Proxy.createFunction(handler, callTrap, constructTrap);
  }

  
  return Proxy.create(handler);
};

function unwrapPrivileged(x) {

  
  
  
  
  if (!isWrappable(x))
    return x;

  
  if (!isWrapper(x))
    throw "Trying to unwrap a non-wrapped object!";

  
  return x.SpecialPowers_wrappedObject;
};

function crawlProtoChain(obj, fn) {
  var rv = fn(obj);
  if (rv !== undefined)
    return rv;
  if (Object.getPrototypeOf(obj))
    return crawlProtoChain(Object.getPrototypeOf(obj), fn);
};






function ExposedPropsWaiverHandler() {
  
  
  var _permit = { value: 'rw', writable: false, configurable: false, enumerable: true };
  return {
    getOwnPropertyDescriptor: function(name) { return _permit; },
    getPropertyDescriptor: function(name) { return _permit; },
    getOwnPropertyNames: function() { throw Error("Can't enumerate ExposedPropsWaiver"); },
    getPropertyNames: function() { throw Error("Can't enumerate ExposedPropsWaiver"); },
    enumerate: function() { throw Error("Can't enumerate ExposedPropsWaiver"); },
    defineProperty: function(name) { throw Error("Can't define props on ExposedPropsWaiver"); },
    delete: function(name) { throw Error("Can't delete props from ExposedPropsWaiver"); }
  };
};
ExposedPropsWaiver = Proxy.create(ExposedPropsWaiverHandler());

function SpecialPowersHandler(obj) {
  this.wrappedObject = obj;
};



SpecialPowersHandler.prototype.doGetPropertyDescriptor = function(name, own) {

  
  if (name == "SpecialPowers_wrappedObject")
    return { value: this.wrappedObject, writeable: false, configurable: false, enumerable: false };

  
  if (name == "__exposedProps__")
    return { value: ExposedPropsWaiver, writable: false, configurable: false, enumerable: false };

  
  
  
  
  
  
  var obj = name == 'toString' ? XPCNativeWrapper.unwrap(this.wrappedObject)
                               : this.wrappedObject;

  
  
  
  
  
  var desc;

  
  
  
  if (own)
    desc = callGetOwnPropertyDescriptor(obj, name);

  
  
  
  
  
  
  
  else if (!isXrayWrapper(this.wrappedObject))
    desc = crawlProtoChain(obj, function(o) {return callGetOwnPropertyDescriptor(o, name);});

  
  
  
  
  
  
  
  
  
  
  else {
    desc = Object.getOwnPropertyDescriptor(obj, name);
    if (!desc) {
      var getter = Object.prototype.__lookupGetter__.call(obj, name);
      var setter = Object.prototype.__lookupSetter__.call(obj, name);
      if (getter || setter)
        desc = {get: getter, set: setter, configurable: true, enumerable: true};
      else if (name in obj)
        desc = {value: obj[name], writable: false, configurable: true, enumerable: true};
    }
  }

  
  if (typeof desc === 'undefined')
    return undefined;

  
  
  
  
  if (desc && 'value' in desc && desc.value === undefined)
    desc.value = obj[name];

  
  
  
  desc.configurable = true;

  
  function wrapIfExists(key) { if (key in desc) desc[key] = wrapPrivileged(desc[key]); };
  wrapIfExists('value');
  wrapIfExists('get');
  wrapIfExists('set');

  return desc;
};

SpecialPowersHandler.prototype.getOwnPropertyDescriptor = function(name) {
  return this.doGetPropertyDescriptor(name, true);
};

SpecialPowersHandler.prototype.getPropertyDescriptor = function(name) {
  return this.doGetPropertyDescriptor(name, false);
};

function doGetOwnPropertyNames(obj, props) {

  
  
  var specialAPI = 'SpecialPowers_wrappedObject';
  if (props.indexOf(specialAPI) == -1)
    props.push(specialAPI);

  
  var flt = function(a) { return props.indexOf(a) == -1; };
  props = props.concat(Object.getOwnPropertyNames(obj).filter(flt));

  
  if ('wrappedJSObject' in obj)
    props = props.concat(Object.getOwnPropertyNames(obj.wrappedJSObject)
                         .filter(flt));

  return props;
}

SpecialPowersHandler.prototype.getOwnPropertyNames = function() {
  return doGetOwnPropertyNames(this.wrappedObject, []);
};

SpecialPowersHandler.prototype.getPropertyNames = function() {

  
  
  
  
  
  
  
  
  
  
  var obj = this.wrappedObject;
  var props = [];
  while (obj) {
    props = doGetOwnPropertyNames(obj, props);
    obj = Object.getPrototypeOf(XPCNativeWrapper.unwrap(obj));
  }
  return props;
};

SpecialPowersHandler.prototype.defineProperty = function(name, desc) {
  return Object.defineProperty(this.wrappedObject, name, desc);
};

SpecialPowersHandler.prototype.delete = function(name) {
  return delete this.wrappedObject[name];
};

SpecialPowersHandler.prototype.fix = function() { return undefined;  };



SpecialPowersHandler.prototype.enumerate = function() {
  var t = this;
  var filt = function(name) { return t.getPropertyDescriptor(name).enumerable; };
  return this.getPropertyNames().filter(filt);
};





function SPConsoleListener(callback) {
  this.callback = callback;
}

SPConsoleListener.prototype = {
  observe: function(msg) {
    let m = { message: msg.message,
              errorMessage: null,
              sourceName: null,
              sourceLine: null,
              lineNumber: null,
              columnNumber: null,
              category: null,
              windowID: null,
              isScriptError: false,
              isWarning: false,
              isException: false,
              isStrict: false };
    if (msg instanceof Ci.nsIScriptError) {
      m.errorMessage  = msg.errorMessage;
      m.sourceName    = msg.sourceName;
      m.sourceLine    = msg.sourceLine;
      m.lineNumber    = msg.lineNumber;
      m.columnNumber  = msg.columnNumber;
      m.category      = msg.category;
      m.windowID      = msg.outerWindowID;
      m.isScriptError = true;
      m.isWarning     = ((msg.flags & Ci.nsIScriptError.warningFlag) === 1);
      m.isException   = ((msg.flags & Ci.nsIScriptError.exceptionFlag) === 1);
      m.isStrict      = ((msg.flags & Ci.nsIScriptError.strictFlag) === 1);
    }

    
    let expose = {};
    for (let prop in m)
      expose[prop] = 'r';
    m.__exposedProps__ = expose;
    Object.freeze(m);

    this.callback.call(undefined, m);

    if (!m.isScriptError && m.message === "SENTINEL")
      Services.console.unregisterListener(this);
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIConsoleListener])
};

SpecialPowersAPI.prototype = {

  



























  wrap: function(obj) { return isWrapper(obj) ? obj : wrapPrivileged(obj); },
  unwrap: unwrapIfWrapped,
  isWrapper: isWrapper,

  get MockFilePicker() {
    return MockFilePicker
  },

  get MockPermissionPrompt() {
    return MockPermissionPrompt
  },

  get Services() {
    return wrapPrivileged(Services);
  },

  




  get Cc() { return wrapPrivileged(this.Components).classes; },
  get Ci() { return this.Components.interfaces; },
  get Cu() { return wrapPrivileged(this.Components).utils; },
  get Cr() { return wrapPrivileged(this.Components).results; },

  








  getRawComponents: getRawComponents,

  getDOMWindowUtils: function(aWindow) {
    if (aWindow == this.window.get() && this.DOMWindowUtils != null)
      return this.DOMWindowUtils;

    return bindDOMWindowUtils(aWindow);
  },

  removeExpectedCrashDumpFiles: function(aExpectingProcessCrash) {
    var success = true;
    if (aExpectingProcessCrash) {
      var message = {
        op: "delete-crash-dump-files",
        filenames: this._encounteredCrashDumpFiles 
      };
      if (!this._sendSyncMessage("SPProcessCrashService", message)[0]) {
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
    var crashDumpFiles = this._sendSyncMessage("SPProcessCrashService", message)[0];
    crashDumpFiles.forEach(function(aFilename) {
      self._unexpectedCrashDumpFiles[aFilename] = true;
    });
    return crashDumpFiles;
  },

  




























  pushPrefEnv: function(inPrefs, callback) {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                           getService(Components.interfaces.nsIPrefBranch);

    var pref_string = [];
    pref_string[prefs.PREF_INT] = "INT";
    pref_string[prefs.PREF_BOOL] = "BOOL";
    pref_string[prefs.PREF_STRING] = "CHAR";

    var pendingActions = [];
    var cleanupActions = [];

    for (var action in inPrefs) { 
      for (var idx in inPrefs[action]) {
        var aPref = inPrefs[action][idx];
        var prefName = aPref[0];
        var prefValue = null;
        var prefIid = null;
        var prefType = prefs.PREF_INVALID;
        var originalValue = null;

        if (aPref.length == 3) {
          prefValue = aPref[1];
          prefIid = aPref[2];
        } else if (aPref.length == 2) {
          prefValue = aPref[1];
        }

        
        if (prefs.getPrefType(prefName) != prefs.PREF_INVALID) {
          prefType = pref_string[prefs.getPrefType(prefName)];
          if ((prefs.prefHasUserValue(prefName) && action == 'clear') ||
              (action == 'set'))
            originalValue = this._getPref(prefName, prefType);
        } else if (action == 'set') {
          
          if (aPref.length == 3) {
            prefType = "COMPLEX";
          } else if (aPref.length == 2) {
            if (typeof(prefValue) == "boolean") 
              prefType = "BOOL";
            else if (typeof(prefValue) == "number")
              prefType = "INT";
            else if (typeof(prefValue) == "string")
              prefType = "CHAR";
          }
        }

        
        if (prefType == prefs.PREF_INVALID)
          continue;

        
        if (originalValue == prefValue)
          continue;

        pendingActions.push({'action': action, 'type': prefType, 'name': prefName, 'value': prefValue, 'Iid': prefIid});

        
        var cleanupTodo = {'action': action, 'type': prefType, 'name': prefName, 'value': originalValue, 'Iid': prefIid};
        if (originalValue == null) {
          cleanupTodo.action = 'clear';
        } else {
          cleanupTodo.action = 'set';
        }
        cleanupActions.push(cleanupTodo);
      }
    }

    if (pendingActions.length > 0) {
      
      
      
      
      
      
      
      function delayedCallback() {
        function delayAgain() {
          content.window.setTimeout(callback, 0);
        }
        content.window.setTimeout(delayAgain, 0);
      }
      this._prefEnvUndoStack.push(cleanupActions);
      this._pendingPrefs.push([pendingActions, delayedCallback]);
      this._applyPrefs();
    } else {
      content.window.setTimeout(callback, 0);
    }
  },

  popPrefEnv: function(callback) {
    if (this._prefEnvUndoStack.length > 0) {
      
      function delayedCallback() {
        function delayAgain() {
          content.window.setTimeout(callback, 0);
        }
        content.window.setTimeout(delayAgain, 0);
      }
      let cb = callback ? delayedCallback : null; 
      
      this._pendingPrefs.push([this._prefEnvUndoStack.pop(), cb]);
      this._applyPrefs();
    } else {
      content.window.setTimeout(callback, 0);
    }
  },

  flushPrefEnv: function(callback) {
    while (this._prefEnvUndoStack.length > 1)
      this.popPrefEnv(null);

    this.popPrefEnv(callback);
  },

  



  _applyPrefs: function() {
    if (this._applyingPrefs || this._pendingPrefs.length <= 0) {
      return;
    }

    
    this._applyingPrefs = true;
    var transaction = this._pendingPrefs.shift();
    var pendingActions = transaction[0];
    var callback = transaction[1];

    var lastPref = pendingActions[pendingActions.length-1];

    var pb = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
    var self = this;
    pb.addObserver(lastPref.name, function prefObs(subject, topic, data) {
      pb.removeObserver(lastPref.name, prefObs);

      content.window.setTimeout(callback, 0);
      content.window.setTimeout(function () {
        self._applyingPrefs = false;
        
        self._applyPrefs();
      }, 0);
    }, false);

    for (var idx in pendingActions) {
      var pref = pendingActions[idx];
      if (pref.action == 'set') {
        this._setPref(pref.name, pref.type, pref.value, pref.Iid);
      } else if (pref.action == 'clear') {
        this.clearUserPref(pref.name);
      }
    }
  },

  
  
  
  
  autoConfirmAppInstall: function(cb) {
    this.pushPrefEnv({set: [['dom.mozApps.auto_confirm_install', true]]}, cb);
  },

  addObserver: function(obs, notification, weak) {
    var obsvc = Cc['@mozilla.org/observer-service;1']
                   .getService(Ci.nsIObserverService);
    obsvc.addObserver(obs, notification, weak);
  },
  removeObserver: function(obs, notification) {
    var obsvc = Cc['@mozilla.org/observer-service;1']
                   .getService(Ci.nsIObserverService);
    obsvc.removeObserver(obs, notification);
  },
   
  can_QI: function(obj) {
    return obj.QueryInterface !== undefined;
  },
  do_QueryInterface: function(obj, iface) {
    return obj.QueryInterface(Ci[iface]); 
  },

  call_Instanceof: function (obj1, obj2) {
     obj1=unwrapIfWrapped(obj1);
     obj2=unwrapIfWrapped(obj2);
     return obj1 instanceof obj2;
  },

  
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
    this._sendSyncMessage('SPPrefService', msg);
  },

  
  _getPref: function(aPrefName, aPrefType, aIid) {
    var msg = {};
    if (aIid) {
      
      msg = {'op':'get', 'prefName': aPrefName, 'prefType':aPrefType, 'prefValue':[aIid]};
    } else {
      msg = {'op':'get', 'prefName': aPrefName,'prefType': aPrefType};
    }
    var val = this._sendSyncMessage('SPPrefService', msg);

    if (val == null || val[0] == null)
      throw "Error getting pref";
    return val[0];
  },
  _setPref: function(aPrefName, aPrefType, aValue, aIid) {
    var msg = {};
    if (aIid) {
      msg = {'op':'set','prefName':aPrefName, 'prefType': aPrefType, 'prefValue': [aIid,aValue]};
    } else {
      msg = {'op':'set', 'prefName': aPrefName, 'prefType': aPrefType, 'prefValue': aValue};
    }
    return(this._sendSyncMessage('SPPrefService', msg)[0]);
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
  
  
  _getTopChromeWindow: function(window) {
    return window.QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIWebNavigation)
                 .QueryInterface(Ci.nsIDocShellTreeItem)
                 .rootTreeItem
                 .QueryInterface(Ci.nsIInterfaceRequestor)
                 .getInterface(Ci.nsIDOMWindow)
                 .QueryInterface(Ci.nsIDOMChromeWindow);
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
  getFormFillController: function(window) {
    return Components.classes["@mozilla.org/satchel/form-fill-controller;1"]
                     .getService(Components.interfaces.nsIFormFillController);
  },
  attachFormFillControllerTo: function(window) {
    this.getFormFillController()
        .attachToBrowser(this._getDocShell(window),
                         this._getAutoCompletePopup(window));
  },
  detachFormFillControllerFrom: function(window) {
    this.getFormFillController().detachFromBrowser(this._getDocShell(window));
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

  
  
  
  
  
  
  registerConsoleListener: function(callback) {
    let listener = new SPConsoleListener(callback);
    Services.console.registerListener(listener);
  },
  postConsoleSentinel: function() {
    Services.console.logStringMessage("SENTINEL");
  },
  resetConsole: function() {
    Services.console.reset();
  },

  getMaxLineBoxWidth: function(window) {
    return this._getMUDV(window).maxLineBoxWidth;
  },

  setMaxLineBoxWidth: function(window, width) {
    this._getMUDV(window).changeMaxLineBoxWidth(width);
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
    return this.wrap(Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest));
  },

  snapshotWindow: function (win, withCaret, rect, bgcolor) {
    var el = this.window.get().document.createElementNS("http://www.w3.org/1999/xhtml", "canvas");
    if (arguments.length < 3) {
      rect = { top: win.scrollY, left: win.scrollX,
               width: win.innerWidth, height: win.innerHeight };
    }
    if (arguments.length < 4) {
      bgcolor = "rgb(255,255,255)";
    }

    el.width = rect.width;
    el.height = rect.height;
    var ctx = el.getContext("2d");
    var flags = 0;

    ctx.drawWindow(win,
                   rect.left, rect.top, rect.width, rect.height,
                   bgcolor,
                   withCaret ? ctx.DRAWWINDOW_DRAW_CARET : 0);
    return el;
  },

  snapshotRect: function (win, rect, bgcolor) {
    
    args = Array.slice(arguments);
    args.splice(1, 0, false);
    return this.snapshotWindow.apply(this, args);
  },

  gc: function() {
    this.DOMWindowUtils.garbageCollect();
  },

  forceGC: function() {
    Cu.forceGC();
  },

  forceCC: function() {
    Cu.forceCC();
  },

  exactGC: function(win, callback) {
    var self = this;
    let count = 0;

    function doPreciseGCandCC() {
      function scheduledGCCallback() {
        self.getDOMWindowUtils(win).cycleCollect();

        if (++count < 2) {
          doPreciseGCandCC();
        } else {
          callback();
        }
      }

      Cu.schedulePreciseGC(scheduledGCCallback);
    }

    doPreciseGCandCC();
  },

  setGCZeal: function(zeal) {
    Cu.setGCZeal(zeal);
  },

  isMainProcess: function() {
    try {
      return Cc["@mozilla.org/xre/app-info;1"].
               getService(Ci.nsIXULRuntime).
               processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
    } catch (e) { }
    return true;
  },

  _xpcomabi: null,

  get XPCOMABI() {
    if (this._xpcomabi != null)
      return this._xpcomabi;

    var xulRuntime = Cc["@mozilla.org/xre/app-info;1"]
                        .getService(Components.interfaces.nsIXULAppInfo)
                        .QueryInterface(Components.interfaces.nsIXULRuntime);

    this._xpcomabi = xulRuntime.XPCOMABI;
    return this._xpcomabi;
  },

  
  
  
  executeSoon: function(aFun, aWin) {
    
    var runnable = {};
    if (aWin)
        runnable = Cu.createObjectIn(aWin);
    runnable.run = aFun;
    Cu.dispatch(runnable, aWin);
  },

  _os: null,

  get OS() {
    if (this._os != null)
      return this._os;

    var xulRuntime = Cc["@mozilla.org/xre/app-info;1"]
                        .getService(Components.interfaces.nsIXULAppInfo)
                        .QueryInterface(Components.interfaces.nsIXULRuntime);

    this._os = xulRuntime.OS;
    return this._os;
  },

  addSystemEventListener: function(target, type, listener, useCapture) {
    Cc["@mozilla.org/eventlistenerservice;1"].
      getService(Ci.nsIEventListenerService).
      addSystemEventListener(target, type, listener, useCapture);
  },
  removeSystemEventListener: function(target, type, listener, useCapture) {
    Cc["@mozilla.org/eventlistenerservice;1"].
      getService(Ci.nsIEventListenerService).
      removeSystemEventListener(target, type, listener, useCapture);
  },

  getDOMRequestService: function() {
    var serv = Cc["@mozilla.org/dom/dom-request-service;1"].
      getService(Ci.nsIDOMRequestService);
    var res = { __exposedProps__: {} };
    var props = ["createRequest", "fireError", "fireSuccess"];
    for (i in props) {
      let prop = props[i];
      res[prop] = function() { return serv[prop].apply(serv, arguments) };
      res.__exposedProps__[prop] = "r";
    }
    return res;
  },

  setLogFile: function(path) {
    this._mfl = new MozillaFileLogger(path);
  },

  log: function(data) {
    this._mfl.log(data);
  },

  closeLogFile: function() {
    this._mfl.close();
  },

  addCategoryEntry: function(category, entry, value, persists, replace) {
    Components.classes["@mozilla.org/categorymanager;1"].
      getService(Components.interfaces.nsICategoryManager).
      addCategoryEntry(category, entry, value, persists, replace);
  },

  getNodePrincipal: function(aNode) {
      return aNode.nodePrincipal;
  },

  getNodeBaseURIObject: function(aNode) {
      return aNode.baseURIObject;
  },

  getDocumentURIObject: function(aDocument) {
      return aDocument.documentURIObject;
  },

  copyString: function(str, doc) {
    Components.classes["@mozilla.org/widget/clipboardhelper;1"].
      getService(Components.interfaces.nsIClipboardHelper).
      copyString(str, doc);
  },

  openDialog: function(win, args) {
    return win.openDialog.apply(win, args);
  },

  
  getPrivilegedProps: function(obj, props) {
    var parts = props.split('.');

    for (var i = 0; i < parts.length; i++) {
      var p = parts[i];
      if (obj[p]) {
        obj = obj[p];
      } else {
        return null;
      }
    }
    return obj;
  },

  get focusManager() {
    if (this._fm != null)
      return this._fm;

    this._fm = Components.classes["@mozilla.org/focus-manager;1"].
                        getService(Components.interfaces.nsIFocusManager);

    return this._fm;
  },

  getFocusedElementForWindow: function(targetWindow, aDeep, childTargetWindow) {
    return this.focusManager.getFocusedElementForWindow(targetWindow, aDeep, childTargetWindow);
  },

  activeWindow: function() {
    return this.focusManager.activeWindow;
  },

  focusedWindow: function() {
    return this.focusManager.focusedWindow;
  },

  focus: function(window) {
    window.focus();
  },
  
  getClipboardData: function(flavor) {  
    if (this._cb == null)
      this._cb = Components.classes["@mozilla.org/widget/clipboard;1"].
                            getService(Components.interfaces.nsIClipboard);

    var xferable = Components.classes["@mozilla.org/widget/transferable;1"].
                   createInstance(Components.interfaces.nsITransferable);
    xferable.init(this._getDocShell(content.window)
                      .QueryInterface(Components.interfaces.nsILoadContext));
    xferable.addDataFlavor(flavor);
    this._cb.getData(xferable, this._cb.kGlobalClipboard);
    var data = {};
    try {
      xferable.getTransferData(flavor, data, {});
    } catch (e) {}
    data = data.value || null;
    if (data == null)
      return "";
      
    return data.QueryInterface(Components.interfaces.nsISupportsString).data;
  },

  clipboardCopyString: function(preExpectedVal, doc) {
    var cbHelperSvc = Components.classes["@mozilla.org/widget/clipboardhelper;1"].
                      getService(Components.interfaces.nsIClipboardHelper);
    cbHelperSvc.copyString(preExpectedVal, doc);
  },

  supportsSelectionClipboard: function() {
    if (this._cb == null) {
      this._cb = Components.classes["@mozilla.org/widget/clipboard;1"].
                            getService(Components.interfaces.nsIClipboard);
    }
    return this._cb.supportsSelectionClipboard();
  },

  swapFactoryRegistration: function(cid, contractID, newFactory, oldFactory) {  
    var componentRegistrar = Components.manager.QueryInterface(Components.interfaces.nsIComponentRegistrar);

    var unregisterFactory = newFactory;
    var registerFactory = oldFactory;
    
    if (cid == null) {
      if (contractID != null) {
        cid = componentRegistrar.contractIDToCID(contractID);
        oldFactory = Components.manager.getClassObject(Components.classes[contractID],
                                                            Components.interfaces.nsIFactory);
      } else {
        return {'error': "trying to register a new contract ID: Missing contractID"};
      }

      unregisterFactory = oldFactory;
      registerFactory = newFactory;
    }
    componentRegistrar.unregisterFactory(cid,
                                         unregisterFactory);

    
    componentRegistrar.registerFactory(cid,
                                       "",
                                       contractID,
                                       registerFactory);
    return {'cid':cid, 'originalFactory':oldFactory};
  },
  
  _getElement: function(aWindow, id) {
    return ((typeof(id) == "string") ?
        aWindow.document.getElementById(id) : id); 
  },
  
  dispatchEvent: function(aWindow, target, event) {
    var el = this._getElement(aWindow, target);
    return el.dispatchEvent(event);
  },

  get isDebugBuild() {
    delete this.isDebugBuild;
    var debug = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
    return this.isDebugBuild = debug.isDebugBuild;
  },

  


  getBrowserFrameMessageManager: function(aFrameElement) {
    return this.wrap(aFrameElement.QueryInterface(Ci.nsIFrameLoaderOwner)
                                  .frameLoader
                                  .messageManager);
  },
  
  setFullscreenAllowed: function(document) {
    var pm = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
    pm.addFromPrincipal(document.nodePrincipal, "fullscreen", Ci.nsIPermissionManager.ALLOW_ACTION);
    var obsvc = Cc['@mozilla.org/observer-service;1']
                   .getService(Ci.nsIObserverService);
    obsvc.notifyObservers(document, "fullscreen-approved", null);
  },
  
  removeFullscreenAllowed: function(document) {
    var pm = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
    pm.removeFromPrincipal(document.nodePrincipal, "fullscreen");
  },

  _getInfoFromPermissionArg: function(arg) {
    let url = "";
    let appId = Ci.nsIScriptSecurityManager.NO_APP_ID;
    let isInBrowserElement = false;

    if (typeof(arg) == "string") {
      
      url = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService)
              .newURI(arg, null, null)
              .spec;
    } else if (arg.manifestURL) {
      
      let appsSvc = Cc["@mozilla.org/AppsService;1"]
                      .getService(Ci.nsIAppsService)
      let app = appsSvc.getAppByManifestURL(arg.manifestURL); 

      if (!app) {
        throw "No app for this manifest!";
      }

      appId = appsSvc.getAppLocalIdByManifestURL(arg.manifestURL);
      url = app.origin;
      isInBrowserElement = arg.isInBrowserElement || false;
    } else if (arg.nodePrincipal) {
      
      url = arg.nodePrincipal.URI.spec;
      appId = arg.nodePrincipal.appId;
      isInBrowserElement = arg.nodePrincipal.isInBrowserElement;
    } else {
      url = arg.url;
      appId = arg.appId;
      isInBrowserElement = arg.isInBrowserElement;
    }

    return [ url, appId, isInBrowserElement ];
  },

  addPermission: function(type, allow, arg) {
    let [url, appId, isInBrowserElement] = this._getInfoFromPermissionArg(arg);

    let permission = allow ? Ci.nsIPermissionManager.ALLOW_ACTION
                           : Ci.nsIPermissionManager.DENY_ACTION;

    var msg = {
      'op': "add",
      'type': type,
      'permission': permission,
      'url': url,
      'appId': appId,
      'isInBrowserElement': isInBrowserElement
    };

    this._sendSyncMessage('SPPermissionManager', msg);
  },

  removePermission: function(type, arg) {
    let [url, appId, isInBrowserElement] = this._getInfoFromPermissionArg(arg);

    var msg = {
      'op': "remove",
      'type': type,
      'url': url,
      'appId': appId,
      'isInBrowserElement': isInBrowserElement
    };

    this._sendSyncMessage('SPPermissionManager', msg);
  },

  getMozFullPath: function(file) {
    return file.mozFullPath;
  },

  isWindowPrivate: function(win) {
    return PrivateBrowsingUtils.isWindowPrivate(win);
  },
};
