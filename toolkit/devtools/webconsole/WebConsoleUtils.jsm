





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ConsoleAPIStorage",
                                  "resource://gre/modules/ConsoleAPIStorage.jsm");

var EXPORTED_SYMBOLS = ["WebConsoleUtils", "JSPropertyProvider", "JSTermHelpers",
                        "PageErrorListener", "ConsoleAPIListener"];







const REGEX_MATCH_FUNCTION_NAME = /^\(?function\s+([^(\s]+)\s*\(/;


const REGEX_MATCH_FUNCTION_ARGS = /^\(?function\s*[^\s(]*\s*\((.+?)\)/;

const TYPES = { OBJECT: 0,
                FUNCTION: 1,
                ARRAY: 2,
                OTHER: 3,
                ITERATOR: 4,
                GETTER: 5,
                GENERATOR: 6,
                STRING: 7
              };

var gObjectId = 0;

var WebConsoleUtils = {
  TYPES: TYPES,

  





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

  













  presentableValueFor: function WCU_presentableValueFor(aObject)
  {
    let type = this.getResultType(aObject);
    let presentable;

    switch (type) {
      case "undefined":
      case "null":
        return {
          type: TYPES.OTHER,
          display: type
        };

      case "array":
        return {
          type: TYPES.ARRAY,
          display: "Array"
        };

      case "string":
        return {
          type: TYPES.STRING,
          display: "\"" + aObject + "\""
        };

      case "date":
      case "regexp":
      case "number":
      case "boolean":
        return {
          type: TYPES.OTHER,
          display: aObject.toString()
        };

      case "iterator":
        return {
          type: TYPES.ITERATOR,
          display: "Iterator"
        };

      case "function":
        presentable = aObject.toString();
        return {
          type: TYPES.FUNCTION,
          display: presentable.substring(0, presentable.indexOf(')') + 1)
        };

      default:
        presentable = String(aObject);
        let m = /^\[object (\S+)\]/.exec(presentable);

        try {
          if (typeof aObject == "object" && typeof aObject.next == "function" &&
              m && m[1] == "Generator") {
            return {
              type: TYPES.GENERATOR,
              display: m[1]
            };
          }
        }
        catch (ex) {
          
          return {
            type: TYPES.OBJECT,
            display: m ? m[1] : "Object"
          };
        }

        if (typeof aObject == "object" &&
            typeof aObject.__iterator__ == "function") {
          return {
            type: TYPES.ITERATOR,
            display: "Iterator"
          };
        }

        return {
          type: TYPES.OBJECT,
          display: m ? m[1] : "Object"
        };
    }
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

  






















  namesAndValuesOf: function WCU_namesAndValuesOf(aObject, aObjectCache)
  {
    let pairs = [];
    let value, presentable;

    let isDOMDocument = aObject instanceof Ci.nsIDOMDocument;
    let deprecated = ["width", "height", "inputEncoding"];

    for (let propName in aObject) {
      
      if (isDOMDocument && deprecated.indexOf(propName) > -1) {
        continue;
      }

      
      if (this.isNonNativeGetter(aObject, propName)) {
        value = "";
        presentable = {type: TYPES.GETTER, display: "Getter"};
      }
      else {
        try {
          value = aObject[propName];
          presentable = this.presentableValueFor(value);
        }
        catch (ex) {
          continue;
        }
      }

      let pair = {};
      pair.name = propName;
      pair.value = presentable.display;
      pair.inspectable = false;
      pair.type = presentable.type;

      switch (presentable.type) {
        case TYPES.GETTER:
        case TYPES.ITERATOR:
        case TYPES.GENERATOR:
        case TYPES.STRING:
          break;
        default:
          try {
            for (let p in value) {
              pair.inspectable = true;
              break;
            }
          }
          catch (ex) { }
          break;
      }

      
      if (pair.inspectable && aObjectCache) {
        pair.objectId = ++gObjectId;
        aObjectCache[pair.objectId] = value;
      }

      pairs.push(pair);
    }

    pairs.sort(this.propertiesSort);

    return pairs;
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
      if (this.isNativeFunction(descriptor.get)) {
        result.value = this.createValueGrip(aObject[aProperty], aObjectWrapper);
      }
      else {
        result.get = this.createValueGrip(descriptor.get, aObjectWrapper);
        result.set = this.createValueGrip(descriptor.set, aObjectWrapper);
      }
    }

    
    
    if (result.value === undefined && result.get === undefined) {
      result.value = this.createValueGrip(aObject[aProperty], aObjectWrapper);
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
      case "string":
      case "number":
        return aValue;
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
    if (aGrip && type == "object") {
      return aGrip.displayString || aGrip.className || aGrip.type || type;
    }
    return type == "string" && aFormatString ?
           this.formatResultString(aGrip) : aGrip + "";
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
      return type;
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

    if (val.type == "function" && val.functionName) {
      return "function " + val.functionName + "(" +
             val.functionArguments.join(", ") + ")";
    }
    if (val.type == "object" && val.className) {
      return val.className;
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






var JSPropertyProvider = (function _JSPP(WCU) {
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
  let names = {};   

  
  let ownNames = null;
  while (aObj !== null) {
    ownNames = Object.getOwnPropertyNames(aObj);
    for (let i = 0; i < ownNames.length; i++) {
      
      
      if (ownNames[i].indexOf(aOptions.matchProp) != 0 ||
          names[ownNames[i]] == true) {
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

















function PageErrorListener(aWindow, aListener)
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
    if (!this.window || !this.listener ||
        !(aScriptError instanceof Ci.nsIScriptError) ||
        !aScriptError.outerWindowID) {
      return;
    }

    if (!this.isCategoryAllowed(aScriptError.category)) {
      return;
    }

    let errorWindow =
      WebConsoleUtils.getWindowByOuterId(aScriptError.outerWindowID, this.window);
    if (!errorWindow || errorWindow.top != this.window) {
      return;
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
    let innerWindowId = WebConsoleUtils.getInnerWindowId(this.window);
    let result = [];
    let errors = {};
    Services.console.getMessageArray(errors, {});

    (errors.value || []).forEach(function(aError) {
      if (!(aError instanceof Ci.nsIScriptError) ||
          aError.innerWindowID != innerWindowId ||
          !this.isCategoryAllowed(aError.category)) {
        return;
      }

      let remoteMessage = WebConsoleUtils.cloneObject(aError);
      result.push(remoteMessage);
    }, this);

    return result;
  },

  


  destroy: function PEL_destroy()
  {
    Services.console.unregisterListener(this);
    this.listener = this.window = null;
  },
};



















function ConsoleAPIListener(aWindow, aOwner)
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
    if (!this.owner || !this.window) {
      return;
    }

    let apiMessage = aMessage.wrappedJSObject;
    let msgWindow = WebConsoleUtils.getWindowByOuterId(apiMessage.ID,
                                                       this.window);
    if (!msgWindow || msgWindow.top != this.window) {
      
      return;
    }

    this.owner.onConsoleAPICall(apiMessage);
  },

  






  getCachedMessages: function CAL_getCachedMessages()
  {
    let innerWindowId = WebConsoleUtils.getInnerWindowId(this.window);
    let messages = ConsoleAPIStorage.getEvents(innerWindowId);
    return messages;
  },

  


  destroy: function CAL_destroy()
  {
    Services.obs.removeObserver(this, "console-api-log-event");
    this.window = this.owner = null;
  },
};















function JSTermHelpers(aOwner)
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
        return aOwner.chromeWindow().InspectorUI.selection;
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
}
