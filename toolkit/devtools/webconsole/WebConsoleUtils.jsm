





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ConsoleAPIStorage",
                                  "resource://gre/modules/ConsoleAPIStorage.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetworkHelper",
                                  "resource://gre/modules/devtools/NetworkHelper.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gActivityDistributor",
                                   "@mozilla.org/network/http-activity-distributor;1",
                                   "nsIHttpActivityDistributor");

XPCOMUtils.defineLazyModuleGetter(this, "gDevTools",
                                  "resource:///modules/devtools/gDevTools.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "TargetFactory",
                                  "resource:///modules/devtools/Target.jsm");

this.EXPORTED_SYMBOLS = ["WebConsoleUtils", "JSPropertyProvider", "JSTermHelpers",
                         "PageErrorListener", "ConsoleAPIListener",
                         "NetworkResponseListener", "NetworkMonitor",
                         "ConsoleProgressListener"];







const REGEX_MATCH_FUNCTION_NAME = /^\(?function\s+([^(\s]+)\s*\(/;


const REGEX_MATCH_FUNCTION_ARGS = /^\(?function\s*[^\s(]*\s*\((.+?)\)/;

this.WebConsoleUtils = {
  





  unwrap: function WCU_unwrap(aObject)
  {
    try {
      return XPCNativeWrapper.unwrap(aObject);
    }
    catch (ex) {
      return aObject;
    }
  },

  





  supportsString: function WCU_supportsString(aString)
  {
    let str = Cc["@mozilla.org/supports-string;1"].
              createInstance(Ci.nsISupportsString);
    str.data = aString;
    return str;
  },

  















  cloneObject: function WCU_cloneObject(aObject, aRecursive, aFilter)
  {
    if (typeof aObject != "object") {
      return aObject;
    }

    let temp;

    if (Array.isArray(aObject)) {
      temp = [];
      Array.forEach(aObject, function(aValue, aIndex) {
        if (!aFilter || aFilter(aIndex, aValue, aObject)) {
          temp.push(aRecursive ? WCU_cloneObject(aValue) : aValue);
        }
      });
    }
    else {
      temp = {};
      for (let key in aObject) {
        let value = aObject[key];
        if (aObject.hasOwnProperty(key) &&
            (!aFilter || aFilter(key, value, aObject))) {
          temp[key] = aRecursive ? WCU_cloneObject(value) : value;
        }
      }
    }

    return temp;
  },

  






  getInnerWindowId: function WCU_getInnerWindowId(aWindow)
  {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIDOMWindowUtils).currentInnerWindowID;
  },

  






  getOuterWindowId: function WCU_getOuterWindowId(aWindow)
  {
    return aWindow.QueryInterface(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIDOMWindowUtils).outerWindowID;
  },

  










  getWindowByOuterId: function WCU_getWindowByOuterId(aOuterId, aHintWindow)
  {
    let someWindow = aHintWindow || Services.wm.getMostRecentWindow(null);
    let content = null;

    if (someWindow) {
      let windowUtils = someWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                                   getInterface(Ci.nsIDOMWindowUtils);
      content = windowUtils.getOuterWindowWithId(aOuterId);
    }

    return content;
  },

  








  abbreviateSourceURL: function WCU_abbreviateSourceURL(aSourceURL)
  {
    
    let hookIndex = aSourceURL.indexOf("?");
    if (hookIndex > -1) {
      aSourceURL = aSourceURL.substring(0, hookIndex);
    }

    
    if (aSourceURL[aSourceURL.length - 1] == "/") {
      aSourceURL = aSourceURL.substring(0, aSourceURL.length - 1);
    }

    
    let slashIndex = aSourceURL.lastIndexOf("/");
    if (slashIndex > -1) {
      aSourceURL = aSourceURL.substring(slashIndex + 1);
    }

    return aSourceURL;
  },

  







  formatResult: function WCU_formatResult(aResult)
  {
    let output = "";
    let type = this.getResultType(aResult);

    switch (type) {
      case "string":
        output = this.formatResultString(aResult);
        break;
      case "boolean":
      case "date":
      case "error":
      case "number":
      case "regexp":
        try {
          output = aResult + "";
        }
        catch (ex) {
          output = ex;
        }
        break;
      case "null":
      case "undefined":
        output = type;
        break;
      default:
        try {
          if (aResult.toSource) {
            output = aResult.toSource();
          }
          if (!output || output == "({})") {
            output = aResult + "";
          }
        }
        catch (ex) {
          output = ex;
        }
        break;
    }

    return output + "";
  },

  







  formatResultString: function WCU_formatResultString(aString)
  {
    function isControlCode(c) {
      
      
      
      
      return (c <= 0x1F) || (0x7F <= c && c <= 0xA0);
    }

    function replaceFn(aMatch, aType, aHex) {
      
      let c = parseInt(aHex, 16);
      return isControlCode(c) ? aMatch : String.fromCharCode(c);
    }

    let output = uneval(aString).replace(/\\(x)([0-9a-fA-F]{2})/g, replaceFn)
                 .replace(/\\(u)([0-9a-fA-F]{4})/g, replaceFn);

    return output;
  },

  







  isObjectInspectable: function WCU_isObjectInspectable(aObject)
  {
    let isEnumerable = false;

    
    if (this.isIteratorOrGenerator(aObject)) {
      return false;
    }

    try {
      for (let p in aObject) {
        isEnumerable = true;
        break;
      }
    }
    catch (ex) {
      
    }

    return isEnumerable && typeof(aObject) != "string";
  },

  








  getResultType: function WCU_getResultType(aResult)
  {
    let type = aResult === null ? "null" : typeof aResult;
    try {
      if (type == "object" && aResult.constructor && aResult.constructor.name) {
        type = aResult.constructor.name + "";
      }
    }
    catch (ex) {
      
      
      
    }

    return type.toLowerCase();
  },

  







  isNativeFunction: function WCU_isNativeFunction(aFunction)
  {
    return typeof aFunction == "function" && !("prototype" in aFunction);
  },

  










  isNonNativeGetter: function WCU_isNonNativeGetter(aObject, aProp)
  {
    if (typeof aObject != "object") {
      return false;
    }
    let desc = this.getPropertyDescriptor(aObject, aProp);
    return desc && desc.get && !this.isNativeFunction(desc.get);
  },

  









  getPropertyDescriptor: function WCU_getPropertyDescriptor(aObject, aProp)
  {
    let desc = null;
    while (aObject) {
      try {
        if (desc = Object.getOwnPropertyDescriptor(aObject, aProp)) {
          break;
        }
      }
      catch (ex if (ex.name == "NS_ERROR_XPC_BAD_CONVERT_JS" ||
                    ex.name == "NS_ERROR_XPC_BAD_OP_ON_WN_PROTO" ||
                    ex.name == "TypeError")) {
        
        
      }
      try {
        aObject = Object.getPrototypeOf(aObject);
      }
      catch (ex if (ex.name == "TypeError")) {
        return desc;
      }
    }
    return desc;
  },

  











  propertiesSort: function WCU_propertiesSort(a, b)
  {
    
    let aNumber = parseFloat(a.name);
    let bNumber = parseFloat(b.name);

    
    if (!isNaN(aNumber) && isNaN(bNumber)) {
      return -1;
    }
    else if (isNaN(aNumber) && !isNaN(bNumber)) {
      return 1;
    }
    else if (!isNaN(aNumber) && !isNaN(bNumber)) {
      return aNumber - bNumber;
    }
    
    else if (a.name < b.name) {
      return -1;
    }
    else if (a.name > b.name) {
      return 1;
    }
    else {
      return 0;
    }
  },

  
















  inspectObject: function WCU_inspectObject(aObject, aObjectWrapper)
  {
    let properties = [];
    let isDOMDocument = aObject instanceof Ci.nsIDOMDocument;
    let deprecated = ["width", "height", "inputEncoding"];

    for (let name in aObject) {
      
      if (isDOMDocument && deprecated.indexOf(name) > -1) {
        continue;
      }

      properties.push(this.inspectObjectProperty(aObject, name, aObjectWrapper));
    }

    return properties.sort(this.propertiesSort);
  },

  


















  inspectObjectProperty:
  function WCU_inspectObjectProperty(aObject, aProperty, aObjectWrapper)
  {
    let descriptor = this.getPropertyDescriptor(aObject, aProperty) || {};

    let result = { name: aProperty };
    result.configurable = descriptor.configurable;
    result.enumerable = descriptor.enumerable;
    result.writable = descriptor.writable;
    if (descriptor.value !== undefined) {
      result.value = this.createValueGrip(descriptor.value, aObjectWrapper);
    }
    else if (descriptor.get) {
      let gotValue = false;
      if (this.isNativeFunction(descriptor.get)) {
        try {
          result.value = this.createValueGrip(aObject[aProperty], aObjectWrapper);
          gotValue = true;
        } catch (e) {}
      }
      if (!gotValue) {
        result.get = this.createValueGrip(descriptor.get, aObjectWrapper);
        result.set = this.createValueGrip(descriptor.set, aObjectWrapper);
      }
    }

    
    
    if (result.value === undefined && result.get === undefined) {
      try {
        result.value = this.createValueGrip(aObject[aProperty], aObjectWrapper);
      }
      catch (ex) {
        
        
      }
    }

    return result;
  },

  











  getObjectGrip: function WCU_getObjectGrip(aObject)
  {
    let className = null;
    let type = typeof aObject;

    let result = {
      "type": type,
      "className": this.getObjectClassName(aObject),
      "displayString": this.formatResult(aObject),
      "inspectable": this.isObjectInspectable(aObject),
    };

    if (type == "function") {
      result.functionName = this.getFunctionName(aObject);
      result.functionArguments = this.getFunctionArguments(aObject);
    }

    return result;
  },

  












  createValueGrip: function WCU_createValueGrip(aValue, aObjectWrapper)
  {
    let type = typeof(aValue);
    switch (type) {
      case "boolean":
      case "number":
        return aValue;
      case "string":
          return aObjectWrapper(aValue);
      case "object":
      case "function":
        if (aValue) {
          return aObjectWrapper(aValue);
        }
      default:
        if (aValue === null) {
          return { type: "null" };
        }

        if (aValue === undefined) {
          return { type: "undefined" };
        }

        Cu.reportError("Failed to provide a grip for value of " + type + ": " +
                       aValue);
        return null;
    }
  },

  








  isIteratorOrGenerator: function WCU_isIteratorOrGenerator(aObject)
  {
    if (aObject === null) {
      return false;
    }

    if (typeof aObject == "object") {
      if (typeof aObject.__iterator__ == "function" ||
          aObject.constructor && aObject.constructor.name == "Iterator") {
        return true;
      }

      try {
        let str = aObject.toString();
        if (typeof aObject.next == "function" &&
            str.indexOf("[object Generator") == 0) {
          return true;
        }
      }
      catch (ex) {
        
        return false;
      }
    }

    return false;
  },

  









  isMixedHTTPSRequest: function WCU_isMixedHTTPSRequest(aRequest, aLocation)
  {
    try {
      let requestURI = Services.io.newURI(aRequest, null, null);
      let contentURI = Services.io.newURI(aLocation, null, null);
      return (contentURI.scheme == "https" && requestURI.scheme != "https");
    }
    catch (ex) {
      return false;
    }
  },

  










  objectActorGripToString: function WCU_objectActorGripToString(aGrip, aFormatString)
  {
    
    
    
    

    let type = typeof(aGrip);
    if (type == "string" ||
        (aGrip && type == "object" && aGrip.type == "longString")) {
      let str = type == "string" ? aGrip : aGrip.initial;
      if (aFormatString) {
        return this.formatResultString(str);
      }
      return str;
    }

    if (aGrip && type == "object") {
      if (aGrip.displayString && typeof aGrip.displayString == "object" &&
          aGrip.displayString.type == "longString") {
        return aGrip.displayString.initial;
      }
      return aGrip.displayString || aGrip.className || aGrip.type || type;
    }

    return aGrip + "";
  },

  







  getFunctionName: function WCF_getFunctionName(aFunction)
  {
    let name = null;
    if (aFunction.name) {
      name = aFunction.name;
    }
    else {
      let desc;
      try {
        desc = aFunction.getOwnPropertyDescriptor("displayName");
      }
      catch (ex) { }
      if (desc && typeof desc.value == "string") {
        name = desc.value;
      }
    }
    if (!name) {
      try {
        let str = (aFunction.toString() || aFunction.toSource()) + "";
        name = (str.match(REGEX_MATCH_FUNCTION_NAME) || [])[1];
      }
      catch (ex) { }
    }
    return name;
  },

  







  getFunctionArguments: function WCF_getFunctionArguments(aFunction)
  {
    let args = [];
    try {
      let str = (aFunction.toString() || aFunction.toSource()) + "";
      let argsString = (str.match(REGEX_MATCH_FUNCTION_ARGS) || [])[1];
      if (argsString) {
        args = argsString.split(/\s*,\s*/);
      }
    }
    catch (ex) { }
    return args;
  },

  








  getObjectClassName: function WCF_getObjectClassName(aObject)
  {
    if (aObject === null) {
      return "null";
    }
    if (aObject === undefined) {
      return "undefined";
    }

    let type = typeof aObject;
    if (type != "object") {
      
      return type.charAt(0).toUpperCase() + type.substr(1);
    }

    let className;

    try {
      className = ((aObject + "").match(/^\[object (\S+)\]$/) || [])[1];
      if (!className) {
        className = ((aObject.constructor + "").match(/^\[object (\S+)\]$/) || [])[1];
      }
      if (!className && typeof aObject.constructor == "function") {
        className = this.getFunctionName(aObject.constructor);
      }
    }
    catch (ex) { }

    return className;
  },

  







  getPropertyPanelValue: function WCU_getPropertyPanelValue(aActor)
  {
    if (aActor.get) {
      return "Getter";
    }

    let val = aActor.value;
    if (typeof val == "string") {
      return this.formatResultString(val);
    }

    if (typeof val != "object" || !val) {
      return val;
    }

    if (val.type == "longString") {
      return this.formatResultString(val.initial) + "\u2026";
    }

    if (val.type == "function" && val.functionName) {
      return "function " + val.functionName + "(" +
             val.functionArguments.join(", ") + ")";
    }
    if (val.type == "object" && val.className) {
      return val.className;
    }

    if (val.displayString && typeof val.displayString == "object" &&
        val.displayString.type == "longString") {
      return val.displayString.initial;
    }

    return val.displayString || val.type;
  },
};





WebConsoleUtils.l10n = function WCU_l10n(aBundleURI)
{
  this._bundleUri = aBundleURI;
};

WebConsoleUtils.l10n.prototype = {
  _stringBundle: null,

  get stringBundle()
  {
    if (!this._stringBundle) {
      this._stringBundle = Services.strings.createBundle(this._bundleUri);
    }
    return this._stringBundle;
  },

  








  timestampString: function WCU_l10n_timestampString(aMilliseconds)
  {
    let d = new Date(aMilliseconds ? aMilliseconds : null);
    let hours = d.getHours(), minutes = d.getMinutes();
    let seconds = d.getSeconds(), milliseconds = d.getMilliseconds();
    let parameters = [hours, minutes, seconds, milliseconds];
    return this.getFormatStr("timestampFormat", parameters);
  },

  







  getStr: function WCU_l10n_getStr(aName)
  {
    let result;
    try {
      result = this.stringBundle.GetStringFromName(aName);
    }
    catch (ex) {
      Cu.reportError("Failed to get string: " + aName);
      throw ex;
    }
    return result;
  },

  










  getFormatStr: function WCU_l10n_getFormatStr(aName, aArray)
  {
    let result;
    try {
      result = this.stringBundle.formatStringFromName(aName, aArray, aArray.length);
    }
    catch (ex) {
      Cu.reportError("Failed to format string: " + aName);
      throw ex;
    }
    return result;
  },
};






this.JSPropertyProvider = (function _JSPP(WCU) {
const STATE_NORMAL = 0;
const STATE_QUOTE = 2;
const STATE_DQUOTE = 3;

const OPEN_BODY = "{[(".split("");
const CLOSE_BODY = "}])".split("");
const OPEN_CLOSE_BODY = {
  "{": "}",
  "[": "]",
  "(": ")",
};

const MAX_COMPLETIONS = 256;




















function findCompletionBeginning(aStr)
{
  let bodyStack = [];

  let state = STATE_NORMAL;
  let start = 0;
  let c;
  for (let i = 0; i < aStr.length; i++) {
    c = aStr[i];

    switch (state) {
      
      case STATE_NORMAL:
        if (c == '"') {
          state = STATE_DQUOTE;
        }
        else if (c == "'") {
          state = STATE_QUOTE;
        }
        else if (c == ";") {
          start = i + 1;
        }
        else if (c == " ") {
          start = i + 1;
        }
        else if (OPEN_BODY.indexOf(c) != -1) {
          bodyStack.push({
            token: c,
            start: start
          });
          start = i + 1;
        }
        else if (CLOSE_BODY.indexOf(c) != -1) {
          var last = bodyStack.pop();
          if (!last || OPEN_CLOSE_BODY[last.token] != c) {
            return {
              err: "syntax error"
            };
          }
          if (c == "}") {
            start = i + 1;
          }
          else {
            start = last.start;
          }
        }
        break;

      
      case STATE_DQUOTE:
        if (c == "\\") {
          i++;
        }
        else if (c == "\n") {
          return {
            err: "unterminated string literal"
          };
        }
        else if (c == '"') {
          state = STATE_NORMAL;
        }
        break;

      
      case STATE_QUOTE:
        if (c == "\\") {
          i++;
        }
        else if (c == "\n") {
          return {
            err: "unterminated string literal"
          };
        }
        else if (c == "'") {
          state = STATE_NORMAL;
        }
        break;
    }
  }

  return {
    state: state,
    startPos: start
  };
}




















function JSPropertyProvider(aScope, aInputValue)
{
  let obj = WCU.unwrap(aScope);

  
  
  let beginning = findCompletionBeginning(aInputValue);

  
  if (beginning.err) {
    return null;
  }

  
  
  if (beginning.state != STATE_NORMAL) {
    return null;
  }

  let completionPart = aInputValue.substring(beginning.startPos);

  
  if (completionPart.trim() == "") {
    return null;
  }

  let matches = null;
  let matchProp = "";

  let lastDot = completionPart.lastIndexOf(".");
  if (lastDot > 0 &&
      (completionPart[0] == "'" || completionPart[0] == '"') &&
      completionPart[lastDot - 1] == completionPart[0]) {
    
    obj = obj.String.prototype;
    matchProp = completionPart.slice(lastDot + 1);

  }
  else {
    

    let properties = completionPart.split(".");
    if (properties.length > 1) {
      matchProp = properties.pop().trimLeft();
      for (let i = 0; i < properties.length; i++) {
        let prop = properties[i].trim();
        if (!prop) {
          return null;
        }

        
        
        if (obj == null) {
          return null;
        }

        
        
        if (WCU.isNonNativeGetter(obj, prop)) {
          return null;
        }
        try {
          obj = obj[prop];
        }
        catch (ex) {
          return null;
        }
      }
    }
    else {
      matchProp = properties[0].trimLeft();
    }

    
    
    if (obj == null) {
      return null;
    }

    
    if (WCU.isIteratorOrGenerator(obj)) {
      return null;
    }
  }

  let matches = Object.keys(getMatchedProps(obj, {matchProp:matchProp}));

  return {
    matchProp: matchProp,
    matches: matches.sort(),
  };
}

















function getMatchedProps(aObj, aOptions = {matchProp: ""})
{
  
  aOptions.matchProp = aOptions.matchProp || "";

  if (aObj == null) { return {}; }
  try {
    Object.getPrototypeOf(aObj);
  } catch(e) {
    aObj = aObj.constructor.prototype;
  }
  let c = MAX_COMPLETIONS;
  let names = Object.create(null);   

  
  let ownNames = null;
  while (aObj !== null) {
    ownNames = Object.getOwnPropertyNames(aObj);
    for (let i = 0; i < ownNames.length; i++) {
      
      
      if (ownNames[i].indexOf(aOptions.matchProp) != 0 ||
          ownNames[i] in names) {
        continue;
      }
      c--;
      if (c < 0) {
        return names;
      }
      
      
      
      if (+ownNames[i] != +ownNames[i]) {
        names[ownNames[i]] = true;
      }
    }
    aObj = Object.getPrototypeOf(aObj);
  }

  return names;
}


return JSPropertyProvider;
})(WebConsoleUtils);


















this.PageErrorListener = function PageErrorListener(aWindow, aListener)
{
  this.window = aWindow;
  this.listener = aListener;
}

PageErrorListener.prototype =
{
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIConsoleListener]),

  



  window: null,

  




  listener: null,

  


  init: function PEL_init()
  {
    Services.console.registerListener(this);
  },

  







  observe: function PEL_observe(aScriptError)
  {
    if (!this.listener ||
        !(aScriptError instanceof Ci.nsIScriptError)) {
      return;
    }

    if (this.window) {
      if (!aScriptError.outerWindowID ||
          !this.isCategoryAllowed(aScriptError.category)) {
        return;
      }

      let errorWindow =
        WebConsoleUtils.getWindowByOuterId(aScriptError.outerWindowID,
                                           this.window);
      if (!errorWindow || errorWindow.top != this.window) {
        return;
      }
    }

    this.listener.onPageError(aScriptError);
  },

  








  isCategoryAllowed: function PEL_isCategoryAllowed(aCategory)
  {
    switch (aCategory) {
      case "XPConnect JavaScript":
      case "component javascript":
      case "chrome javascript":
      case "chrome registration":
      case "XBL":
      case "XBL Prototype Handler":
      case "XBL Content Sink":
      case "xbl javascript":
        return false;
    }

    return true;
  },

  







  getCachedMessages: function PEL_getCachedMessages()
  {
    let innerWindowId = this.window ?
                        WebConsoleUtils.getInnerWindowId(this.window) : null;
    let errors = Services.console.getMessageArray() || [];

    return errors.filter(function(aError) {
      return aError instanceof Ci.nsIScriptError &&
             (!innerWindowId ||
              (aError.innerWindowID == innerWindowId &&
               this.isCategoryAllowed(aError.category)));
    }, this);
  },

  


  destroy: function PEL_destroy()
  {
    Services.console.unregisterListener(this);
    this.listener = this.window = null;
  },
};




















this.ConsoleAPIListener = function ConsoleAPIListener(aWindow, aOwner)
{
  this.window = aWindow;
  this.owner = aOwner;
}

ConsoleAPIListener.prototype =
{
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  



  window: null,

  







  owner: null,

  


  init: function CAL_init()
  {
    
    
    Services.obs.addObserver(this, "console-api-log-event", false);
  },

  








  observe: function CAL_observe(aMessage, aTopic)
  {
    if (!this.owner) {
      return;
    }

    let apiMessage = aMessage.wrappedJSObject;
    if (this.window) {
      let msgWindow = WebConsoleUtils.getWindowByOuterId(apiMessage.ID,
                                                         this.window);
      if (!msgWindow || msgWindow.top != this.window) {
        
        return;
      }
    }

    this.owner.onConsoleAPICall(apiMessage);
  },

  






  getCachedMessages: function CAL_getCachedMessages()
  {
    let innerWindowId = this.window ?
                        WebConsoleUtils.getInnerWindowId(this.window) : null;
    return ConsoleAPIStorage.getEvents(innerWindowId);
  },

  


  destroy: function CAL_destroy()
  {
    Services.obs.removeObserver(this, "console-api-log-event");
    this.window = this.owner = null;
  },
};















this.JSTermHelpers = function JSTermHelpers(aOwner)
{
  







  aOwner.sandbox.$ = function JSTH_$(aSelector)
  {
    return aOwner.window.document.querySelector(aSelector);
  };

  







  aOwner.sandbox.$$ = function JSTH_$$(aSelector)
  {
    return aOwner.window.document.querySelectorAll(aSelector);
  };

  








  aOwner.sandbox.$x = function JSTH_$x(aXPath, aContext)
  {
    let nodes = [];
    let doc = aOwner.window.document;
    let aContext = aContext || doc;

    try {
      let results = doc.evaluate(aXPath, aContext, null,
                                 Ci.nsIDOMXPathResult.ANY_TYPE, null);
      let node;
      while (node = results.iterateNext()) {
        nodes.push(node);
      }
    }
    catch (ex) {
      aOwner.window.console.error(ex.message);
    }

    return nodes;
  };

  










  Object.defineProperty(aOwner.sandbox, "$0", {
    get: function() {
      try {
        let window = aOwner.chromeWindow();
        let target = TargetFactory.forTab(window.gBrowser.selectedTab);
        let toolbox = gDevTools.getToolbox(target);

        return toolbox == null ?
            undefined :
            toolbox.getPanel("inspector").selection.node;
      }
      catch (ex) {
        aOwner.window.console.error(ex.message);
      }
    },
    enumerable: true,
    configurable: false
  });

  


  aOwner.sandbox.clear = function JSTH_clear()
  {
    aOwner.helperResult = {
      type: "clearOutput",
    };
  };

  






  aOwner.sandbox.keys = function JSTH_keys(aObject)
  {
    return Object.keys(WebConsoleUtils.unwrap(aObject));
  };

  






  aOwner.sandbox.values = function JSTH_values(aObject)
  {
    let arrValues = [];
    let obj = WebConsoleUtils.unwrap(aObject);

    try {
      for (let prop in obj) {
        arrValues.push(obj[prop]);
      }
    }
    catch (ex) {
      aOwner.window.console.error(ex.message);
    }

    return arrValues;
  };

  


  aOwner.sandbox.help = function JSTH_help()
  {
    aOwner.helperResult = { type: "help" };
  };

  





  aOwner.sandbox.inspect = function JSTH_inspect(aObject)
  {
    let obj = WebConsoleUtils.unwrap(aObject);
    if (!WebConsoleUtils.isObjectInspectable(obj)) {
      return aObject;
    }

    aOwner.helperResult = {
      type: "inspectObject",
      input: aOwner.evalInput,
      object: aOwner.createValueGrip(obj),
    };
  };

  






  aOwner.sandbox.pprint = function JSTH_pprint(aObject)
  {
    if (aObject === null || aObject === undefined || aObject === true ||
        aObject === false) {
      aOwner.helperResult = {
        type: "error",
        message: "helperFuncUnsupportedTypeError",
      };
      return;
    }

    aOwner.helperResult = { rawOutput: true };

    if (typeof aObject == "function") {
      return aObject + "\n";
    }

    let output = [];
    let getObjectGrip = WebConsoleUtils.getObjectGrip.bind(WebConsoleUtils);
    let obj = WebConsoleUtils.unwrap(aObject);
    let props = WebConsoleUtils.inspectObject(obj, getObjectGrip);
    props.forEach(function(aProp) {
      output.push(aProp.name + ": " +
                  WebConsoleUtils.getPropertyPanelValue(aProp));
    });

    return "  " + output.join("\n  ");
  };

  






  aOwner.sandbox.print = function JSTH_print(aString)
  {
    aOwner.helperResult = { rawOutput: true };
    return String(aString);
  };
};


(function(_global, WCU) {





const PR_UINT32_MAX = 4294967295;


const HTTP_MOVED_PERMANENTLY = 301;
const HTTP_FOUND = 302;
const HTTP_SEE_OTHER = 303;
const HTTP_TEMPORARY_REDIRECT = 307;


const RESPONSE_BODY_LIMIT = 1048576; 



















function NetworkResponseListener(aOwner, aHttpActivity)
{
  this.owner = aOwner;
  this.receivedData = "";
  this.httpActivity = aHttpActivity;
  this.bodySize = 0;
}

NetworkResponseListener.prototype = {
  QueryInterface:
    XPCOMUtils.generateQI([Ci.nsIStreamListener, Ci.nsIInputStreamCallback,
                           Ci.nsIRequestObserver, Ci.nsISupports]),

  




  _foundOpenResponse: false,

  


  owner: null,

  



  sink: null,

  


  httpActivity: null,

  


  receivedData: null,

  


  bodySize: null,

  


  request: null,

  











  setAsyncListener: function NRL_setAsyncListener(aStream, aListener)
  {
    
    aStream.asyncWait(aListener, 0, 0, Services.tm.mainThread);
  },

  













  onDataAvailable:
  function NRL_onDataAvailable(aRequest, aContext, aInputStream, aOffset, aCount)
  {
    this._findOpenResponse();
    let data = NetUtil.readInputStreamToString(aInputStream, aCount);

    this.bodySize += aCount;

    if (!this.httpActivity.discardResponseBody &&
        this.receivedData.length < RESPONSE_BODY_LIMIT) {
      this.receivedData += NetworkHelper.
                           convertToUnicode(data, aRequest.contentCharset);
    }
  },

  






  onStartRequest: function NRL_onStartRequest(aRequest)
  {
    this.request = aRequest;
    this._findOpenResponse();
    
    this.setAsyncListener(this.sink.inputStream, this);
  },

  





  onStopRequest: function NRL_onStopRequest()
  {
    this._findOpenResponse();
    this.sink.outputStream.close();
  },

  








  _findOpenResponse: function NRL__findOpenResponse()
  {
    if (!this.owner || this._foundOpenResponse) {
      return;
    }

    let openResponse = null;

    for each (let item in this.owner.openResponses) {
      if (item.channel === this.httpActivity.channel) {
        openResponse = item;
        break;
      }
    }

    if (!openResponse) {
      return;
    }
    this._foundOpenResponse = true;

    delete this.owner.openResponses[openResponse.id];

    this.httpActivity.owner.addResponseHeaders(openResponse.headers);
    this.httpActivity.owner.addResponseCookies(openResponse.cookies);
  },

  





  onStreamClose: function NRL_onStreamClose()
  {
    if (!this.httpActivity) {
      return;
    }
    
    this.setAsyncListener(this.sink.inputStream, null);

    this._findOpenResponse();

    if (!this.httpActivity.discardResponseBody && this.receivedData.length) {
      this._onComplete(this.receivedData);
    }
    else if (!this.httpActivity.discardResponseBody &&
             this.httpActivity.responseStatus == 304) {
      
      let charset = this.request.contentCharset || this.httpActivity.charset;
      NetworkHelper.loadFromCache(this.httpActivity.url, charset,
                                  this._onComplete.bind(this));
    }
    else {
      this._onComplete();
    }
  },

  







  _onComplete: function NRL__onComplete(aData)
  {
    let response = {
      mimeType: "",
      text: aData || "",
    };

    response.size = response.text.length;

    try {
      response.mimeType = this.request.contentType;
    }
    catch (ex) { }

    if (!response.mimeType || !NetworkHelper.isTextMimeType(response.mimeType)) {
      response.encoding = "base64";
      response.text = btoa(response.text);
    }

    if (response.mimeType && this.request.contentCharset) {
      response.mimeType += "; charset=" + this.request.contentCharset;
    }

    this.receivedData = "";

    this.httpActivity.owner.
      addResponseContent(response, this.httpActivity.discardResponseBody);

    this.httpActivity.channel = null;
    this.httpActivity.owner = null;
    this.httpActivity = null;
    this.sink = null;
    this.inputStream = null;
    this.request = null;
    this.owner = null;
  },

  







  onInputStreamReady: function NRL_onInputStreamReady(aStream)
  {
    if (!(aStream instanceof Ci.nsIAsyncInputStream) || !this.httpActivity) {
      return;
    }

    let available = -1;
    try {
      
      available = aStream.available();
    }
    catch (ex) { }

    if (available != -1) {
      if (available != 0) {
        
        
        
        this.onDataAvailable(this.request, null, aStream, 0, available);
      }
      this.setAsyncListener(aStream, this);
    }
    else {
      this.onStreamClose();
    }
  },
};





















function NetworkMonitor(aWindow, aOwner)
{
  this.window = aWindow;
  this.owner = aOwner;
  this.openRequests = {};
  this.openResponses = {};
  this._httpResponseExaminer = this._httpResponseExaminer.bind(this);
}

NetworkMonitor.prototype = {
  httpTransactionCodes: {
    0x5001: "REQUEST_HEADER",
    0x5002: "REQUEST_BODY_SENT",
    0x5003: "RESPONSE_START",
    0x5004: "RESPONSE_HEADER",
    0x5005: "RESPONSE_COMPLETE",
    0x5006: "TRANSACTION_CLOSE",

    0x804b0003: "STATUS_RESOLVING",
    0x804b000b: "STATUS_RESOLVED",
    0x804b0007: "STATUS_CONNECTING_TO",
    0x804b0004: "STATUS_CONNECTED_TO",
    0x804b0005: "STATUS_SENDING_TO",
    0x804b000a: "STATUS_WAITING_FOR",
    0x804b0006: "STATUS_RECEIVING_FROM"
  },

  
  
  responsePipeSegmentSize: null,

  owner: null,

  



  get saveRequestAndResponseBodies()
    this.owner && this.owner.saveRequestAndResponseBodies,

  


  openRequests: null,

  


  openResponses: null,

  


  init: function NM_init()
  {
    this.responsePipeSegmentSize = Services.prefs
                                   .getIntPref("network.buffer.cache.size");

    gActivityDistributor.addObserver(this);

    Services.obs.addObserver(this._httpResponseExaminer,
                             "http-on-examine-response", false);
  },

  








  _httpResponseExaminer: function NM__httpResponseExaminer(aSubject, aTopic)
  {
    
    
    
    

    if (!this.owner || aTopic != "http-on-examine-response" ||
        !(aSubject instanceof Ci.nsIHttpChannel)) {
      return;
    }

    let channel = aSubject.QueryInterface(Ci.nsIHttpChannel);

    if (this.window) {
      
      let win = NetworkHelper.getWindowForRequest(channel);
      if (!win || win.top !== this.window) {
        return;
      }
    }

    let response = {
      id: gSequenceId(),
      channel: channel,
      headers: [],
      cookies: [],
    };

    let setCookieHeader = null;

    channel.visitResponseHeaders({
      visitHeader: function NM__visitHeader(aName, aValue) {
        let lowerName = aName.toLowerCase();
        if (lowerName == "set-cookie") {
          setCookieHeader = aValue;
        }
        response.headers.push({ name: aName, value: aValue });
      }
    });

    if (!response.headers.length) {
      return; 
    }

    if (setCookieHeader) {
      response.cookies = NetworkHelper.parseSetCookieHeader(setCookieHeader);
    }

    
    let httpVersionMaj = {};
    let httpVersionMin = {};

    channel.QueryInterface(Ci.nsIHttpChannelInternal);
    channel.getResponseVersion(httpVersionMaj, httpVersionMin);

    response.status = channel.responseStatus;
    response.statusText = channel.responseStatusText;
    response.httpVersion = "HTTP/" + httpVersionMaj.value + "." +
                                     httpVersionMin.value;

    this.openResponses[response.id] = response;
  },

  











  observeActivity:
  function NM_observeActivity(aChannel, aActivityType, aActivitySubtype,
                              aTimestamp, aExtraSizeData, aExtraStringData)
  {
    if (!this.owner ||
        aActivityType != gActivityDistributor.ACTIVITY_TYPE_HTTP_TRANSACTION &&
        aActivityType != gActivityDistributor.ACTIVITY_TYPE_SOCKET_TRANSPORT) {
      return;
    }

    if (!(aChannel instanceof Ci.nsIHttpChannel)) {
      return;
    }

    aChannel = aChannel.QueryInterface(Ci.nsIHttpChannel);

    if (aActivitySubtype ==
        gActivityDistributor.ACTIVITY_SUBTYPE_REQUEST_HEADER) {
      this._onRequestHeader(aChannel, aTimestamp, aExtraStringData);
      return;
    }

    
    
    let httpActivity = null;
    for each (let item in this.openRequests) {
      if (item.channel === aChannel) {
        httpActivity = item;
        break;
      }
    }

    if (!httpActivity) {
      return;
    }

    let transCodes = this.httpTransactionCodes;

    
    if (aActivitySubtype in transCodes) {
      let stage = transCodes[aActivitySubtype];
      if (stage in httpActivity.timings) {
        httpActivity.timings[stage].last = aTimestamp;
      }
      else {
        httpActivity.timings[stage] = {
          first: aTimestamp,
          last: aTimestamp,
        };
      }
    }

    switch (aActivitySubtype) {
      case gActivityDistributor.ACTIVITY_SUBTYPE_REQUEST_BODY_SENT:
        this._onRequestBodySent(httpActivity);
        break;
      case gActivityDistributor.ACTIVITY_SUBTYPE_RESPONSE_HEADER:
        this._onResponseHeader(httpActivity, aExtraStringData);
        break;
      case gActivityDistributor.ACTIVITY_SUBTYPE_TRANSACTION_CLOSE:
        this._onTransactionClose(httpActivity);
        break;
      default:
        break;
    }
  },

  











  _onRequestHeader:
  function NM__onRequestHeader(aChannel, aTimestamp, aExtraStringData)
  {
    let win = null;
    try {
      win = NetworkHelper.getWindowForRequest(aChannel);
    }
    catch (ex) {
      
    }

    
    if (this.window && (!win || win.top !== this.window)) {
      return;
    }

    let httpActivity = this.createActivityObject(aChannel);

    
    httpActivity.charset = win ? win.document.characterSet : null;

    httpActivity.timings.REQUEST_HEADER = {
      first: aTimestamp,
      last: aTimestamp
    };

    let httpVersionMaj = {};
    let httpVersionMin = {};
    let event = {};
    event.startedDateTime = new Date(Math.round(aTimestamp / 1000)).toISOString();
    event.headersSize = aExtraStringData.length;
    event.method = aChannel.requestMethod;
    event.url = aChannel.URI.spec;

    
    aChannel.QueryInterface(Ci.nsIHttpChannelInternal);
    aChannel.getRequestVersion(httpVersionMaj, httpVersionMin);

    event.httpVersion = "HTTP/" + httpVersionMaj.value + "." +
                                  httpVersionMin.value;

    event.discardRequestBody = !this.saveRequestAndResponseBodies;
    event.discardResponseBody = !this.saveRequestAndResponseBodies;

    let headers = [];
    let cookies = [];
    let cookieHeader = null;

    
    aChannel.visitRequestHeaders({
      visitHeader: function NM__visitHeader(aName, aValue)
      {
        if (aName == "Cookie") {
          cookieHeader = aValue;
        }
        headers.push({ name: aName, value: aValue });
      }
    });

    if (cookieHeader) {
      cookies = NetworkHelper.parseCookieHeader(cookieHeader);
    }

    httpActivity.owner = this.owner.onNetworkEvent(event);

    this._setupResponseListener(httpActivity);

    this.openRequests[httpActivity.id] = httpActivity;

    httpActivity.owner.addRequestHeaders(headers);
    httpActivity.owner.addRequestCookies(cookies);
  },

  














  createActivityObject: function NM_createActivityObject(aChannel)
  {
    return {
      id: gSequenceId(),
      channel: aChannel,
      charset: null, 
      url: aChannel.URI.spec,
      discardRequestBody: !this.saveRequestAndResponseBodies,
      discardResponseBody: !this.saveRequestAndResponseBodies,
      timings: {}, 
      responseStatus: null, 
      owner: null, 
    };
  },

  







  _setupResponseListener: function NM__setupResponseListener(aHttpActivity)
  {
    let channel = aHttpActivity.channel;
    channel.QueryInterface(Ci.nsITraceableChannel);

    
    
    
    
    let sink = Cc["@mozilla.org/pipe;1"].createInstance(Ci.nsIPipe);

    
    
    sink.init(false, false, this.responsePipeSegmentSize, PR_UINT32_MAX, null);

    
    let newListener = new NetworkResponseListener(this, aHttpActivity);

    
    newListener.inputStream = sink.inputStream;
    newListener.sink = sink;

    let tee = Cc["@mozilla.org/network/stream-listener-tee;1"].
              createInstance(Ci.nsIStreamListenerTee);

    let originalListener = channel.setNewListener(tee);

    tee.init(originalListener, sink.outputStream, newListener);
  },

  







  _onRequestBodySent: function NM__onRequestBodySent(aHttpActivity)
  {
    if (aHttpActivity.discardRequestBody) {
      return;
    }

    let sentBody = NetworkHelper.
                   readPostTextFromRequest(aHttpActivity.channel,
                                           aHttpActivity.charset);

    if (!sentBody && this.window &&
        aHttpActivity.url == this.window.location.href) {
      
      
      
      
      
      
      
      let webNav = this.window.QueryInterface(Ci.nsIInterfaceRequestor).
                   getInterface(Ci.nsIWebNavigation);
      sentBody = NetworkHelper.
                 readPostTextFromPageViaWebNav(webNav, aHttpActivity.charset);
    }

    if (sentBody) {
      aHttpActivity.owner.addRequestPostData({ text: sentBody });
    }
  },

  









  _onResponseHeader:
  function NM__onResponseHeader(aHttpActivity, aExtraStringData)
  {
    
    
    
    
    
    
    
    
    
    

    let headers = aExtraStringData.split(/\r\n|\n|\r/);
    let statusLine = headers.shift();
    let statusLineArray = statusLine.split(" ");

    let response = {};
    response.httpVersion = statusLineArray.shift();
    response.status = statusLineArray.shift();
    response.statusText = statusLineArray.join(" ");
    response.headersSize = aExtraStringData.length;

    aHttpActivity.responseStatus = response.status;

    
    switch (parseInt(response.status)) {
      case HTTP_MOVED_PERMANENTLY:
      case HTTP_FOUND:
      case HTTP_SEE_OTHER:
      case HTTP_TEMPORARY_REDIRECT:
        aHttpActivity.discardResponseBody = true;
        break;
    }

    response.discardResponseBody = aHttpActivity.discardResponseBody;

    aHttpActivity.owner.addResponseStart(response);
  },

  








  _onTransactionClose: function NM__onTransactionClose(aHttpActivity)
  {
    let result = this._setupHarTimings(aHttpActivity);
    aHttpActivity.owner.addEventTimings(result.total, result.timings);
    delete this.openRequests[aHttpActivity.id];
  },

  












  _setupHarTimings: function NM__setupHarTimings(aHttpActivity)
  {
    let timings = aHttpActivity.timings;
    let harTimings = {};

    
    harTimings.blocked = -1;

    
    
    harTimings.dns = timings.STATUS_RESOLVING && timings.STATUS_RESOLVED ?
                     timings.STATUS_RESOLVED.last -
                     timings.STATUS_RESOLVING.first : -1;

    if (timings.STATUS_CONNECTING_TO && timings.STATUS_CONNECTED_TO) {
      harTimings.connect = timings.STATUS_CONNECTED_TO.last -
                           timings.STATUS_CONNECTING_TO.first;
    }
    else if (timings.STATUS_SENDING_TO) {
      harTimings.connect = timings.STATUS_SENDING_TO.first -
                           timings.REQUEST_HEADER.first;
    }
    else {
      harTimings.connect = -1;
    }

    if ((timings.STATUS_WAITING_FOR || timings.STATUS_RECEIVING_FROM) &&
        (timings.STATUS_CONNECTED_TO || timings.STATUS_SENDING_TO)) {
      harTimings.send = (timings.STATUS_WAITING_FOR ||
                         timings.STATUS_RECEIVING_FROM).first -
                        (timings.STATUS_CONNECTED_TO ||
                         timings.STATUS_SENDING_TO).last;
    }
    else {
      harTimings.send = -1;
    }

    if (timings.RESPONSE_START) {
      harTimings.wait = timings.RESPONSE_START.first -
                        (timings.REQUEST_BODY_SENT ||
                         timings.STATUS_SENDING_TO).last;
    }
    else {
      harTimings.wait = -1;
    }

    if (timings.RESPONSE_START && timings.RESPONSE_COMPLETE) {
      harTimings.receive = timings.RESPONSE_COMPLETE.last -
                           timings.RESPONSE_START.first;
    }
    else {
      harTimings.receive = -1;
    }

    let totalTime = 0;
    for (let timing in harTimings) {
      let time = Math.max(Math.round(harTimings[timing] / 1000), -1);
      harTimings[timing] = time;
      if (time > -1) {
        totalTime += time;
      }
    }

    return {
      total: totalTime,
      timings: harTimings,
    };
  },

  



  destroy: function NM_destroy()
  {
    Services.obs.removeObserver(this._httpResponseExaminer,
                                "http-on-examine-response");

    gActivityDistributor.removeObserver(this);

    this.openRequests = {};
    this.openResponses = {};
    this.owner = null;
    this.window = null;
  },
};

_global.NetworkMonitor = NetworkMonitor;
_global.NetworkResponseListener = NetworkResponseListener;
})(this, WebConsoleUtils);















this.ConsoleProgressListener =
 function ConsoleProgressListener(aWindow, aOwner)
{
  this.window = aWindow;
  this.owner = aOwner;
}

ConsoleProgressListener.prototype = {
  



  MONITOR_FILE_ACTIVITY: 1,

  



  MONITOR_LOCATION_CHANGE: 2,

  




  _fileActivity: false,

  




  _locationChange: false,

  




  _initialized: false,

  _webProgress: null,

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener,
                                         Ci.nsISupportsWeakReference]),

  



  _init: function CPL__init()
  {
    if (this._initialized) {
      return;
    }

    this._webProgress = this.window.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIWebProgress);
    this._webProgress.addProgressListener(this,
                                          Ci.nsIWebProgress.NOTIFY_STATE_ALL);

    this._initialized = true;
  },

  










  startMonitor: function CPL_startMonitor(aMonitor)
  {
    switch (aMonitor) {
      case this.MONITOR_FILE_ACTIVITY:
        this._fileActivity = true;
        break;
      case this.MONITOR_LOCATION_CHANGE:
        this._locationChange = true;
        break;
      default:
        throw new Error("ConsoleProgressListener: unknown monitor type " +
                        aMonitor + "!");
    }
    this._init();
  },

  






  stopMonitor: function CPL_stopMonitor(aMonitor)
  {
    switch (aMonitor) {
      case this.MONITOR_FILE_ACTIVITY:
        this._fileActivity = false;
        break;
      case this.MONITOR_LOCATION_CHANGE:
        this._locationChange = false;
        break;
      default:
        throw new Error("ConsoleProgressListener: unknown monitor type " +
                        aMonitor + "!");
    }

    if (!this._fileActivity && !this._locationChange) {
      this.destroy();
    }
  },

  onStateChange:
  function CPL_onStateChange(aProgress, aRequest, aState, aStatus)
  {
    if (!this.owner) {
      return;
    }

    if (this._fileActivity) {
      this._checkFileActivity(aProgress, aRequest, aState, aStatus);
    }

    if (this._locationChange) {
      this._checkLocationChange(aProgress, aRequest, aState, aStatus);
    }
  },

  





  _checkFileActivity:
  function CPL__checkFileActivity(aProgress, aRequest, aState, aStatus)
  {
    if (!(aState & Ci.nsIWebProgressListener.STATE_START)) {
      return;
    }

    let uri = null;
    if (aRequest instanceof Ci.imgIRequest) {
      let imgIRequest = aRequest.QueryInterface(Ci.imgIRequest);
      uri = imgIRequest.URI;
    }
    else if (aRequest instanceof Ci.nsIChannel) {
      let nsIChannel = aRequest.QueryInterface(Ci.nsIChannel);
      uri = nsIChannel.URI;
    }

    if (!uri || !uri.schemeIs("file") && !uri.schemeIs("ftp")) {
      return;
    }

    this.owner.onFileActivity(uri.spec);
  },

  





  _checkLocationChange:
  function CPL__checkLocationChange(aProgress, aRequest, aState, aStatus)
  {
    let isStart = aState & Ci.nsIWebProgressListener.STATE_START;
    let isStop = aState & Ci.nsIWebProgressListener.STATE_STOP;
    let isNetwork = aState & Ci.nsIWebProgressListener.STATE_IS_NETWORK;
    let isWindow = aState & Ci.nsIWebProgressListener.STATE_IS_WINDOW;

    
    if (!isNetwork || !isWindow || aProgress.DOMWindow != this.window) {
      return;
    }

    if (isStart && aRequest instanceof Ci.nsIChannel) {
      this.owner.onLocationChange("start", aRequest.URI.spec, "");
    }
    else if (isStop) {
      this.owner.onLocationChange("stop", this.window.location.href,
                                  this.window.document.title);
    }
  },

  onLocationChange: function() {},
  onStatusChange: function() {},
  onProgressChange: function() {},
  onSecurityChange: function() {},

  


  destroy: function CPL_destroy()
  {
    if (!this._initialized) {
      return;
    }

    this._initialized = false;
    this._fileActivity = false;
    this._locationChange = false;

    try {
      this._webProgress.removeProgressListener(this);
    }
    catch (ex) {
      
    }

    this._webProgress = null;
    this.window = null;
    this.owner = null;
  },
};

function gSequenceId()
{
  return gSequenceId.n++;
}
gSequenceId.n = 0;
