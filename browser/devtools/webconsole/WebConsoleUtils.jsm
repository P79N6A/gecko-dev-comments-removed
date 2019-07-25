





"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
                                  "resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = ["WebConsoleUtils", "JSPropertyProvider"];

const STRINGS_URI = "chrome://browser/locale/devtools/webconsole.properties";

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

  










  getWindowByInnerId: function WCU_getWindowByInnerId(aInnerId, aHintWindow)
  {
    let someWindow = aHintWindow || Services.wm.getMostRecentWindow(null);
    let content = null;

    if (someWindow) {
      let windowUtils = someWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                                   getInterface(Ci.nsIDOMWindowUtils);
      content = windowUtils.getInnerWindowWithId(aInnerId);
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
        output = aResult.toString();
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
    if (type == "object" && aResult.constructor && aResult.constructor.name) {
      type = aResult.constructor.name;
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
    let desc;
    while (aObject) {
      try {
        if (desc = Object.getOwnPropertyDescriptor(aObject, aProp)) {
          break;
        }
      }
      catch (ex) {
        
        if (ex.name == "NS_ERROR_XPC_BAD_CONVERT_JS" ||
            ex.name == "NS_ERROR_XPC_BAD_OP_ON_WN_PROTO") {
          return false;
        }
        throw ex;
      }
      aObject = Object.getPrototypeOf(aObject);
    }
    if (desc && desc.get && !this.isNativeFunction(desc.get)) {
      return true;
    }
    return false;
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

    pairs.sort(function(a, b)
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
    });

    return pairs;
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
};





WebConsoleUtils.l10n = {
  








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

XPCOMUtils.defineLazyGetter(WebConsoleUtils.l10n, "stringBundle", function() {
  return Services.strings.createBundle(STRINGS_URI);
});






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

  let properties = completionPart.split(".");
  let matchProp;
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
