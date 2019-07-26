



"use strict";







let Ci = Components['interfaces'];







 const UNWRAP_ACCESS_KEY = {};


 



















function ContentScriptFunctionWrapper(fun, obj, name) {
  if ("___proxy" in fun && typeof fun.___proxy == "function")
    return fun.___proxy;
  
  let wrappedFun = function () {
    let args = [];
    for (let i = 0, l = arguments.length; i < l; i++)
      args.push(wrap(arguments[i]));
    
    
    
    
    
    
    
    if (this == wrappedFun)
      return fun.apply(fun, args);
    
    return fun.apply(wrap(this), args);
  };
  
  Object.defineProperty(fun, "___proxy", {value : wrappedFun,
                                          writable : false,
                                          enumerable : false,
                                          configurable : false});
  
  return wrappedFun;
}




















function NativeFunctionWrapper(fun, originalObject, name) {
  return function () {
    let args = [];
    let obj = this && typeof this.valueOf == "function" ?
              this.valueOf(UNWRAP_ACCESS_KEY) : this;

    for (let i = 0, l = arguments.length; i < l; i++)
      args.push( unwrap(arguments[i], obj, name) );
    
    
    
    
    
    
    let unwrapResult = Function.prototype.apply.apply(fun, [obj, args]);
    let result = wrap(unwrapResult, obj, name);
    
    
    
    return result;
  };
}





function unwrap(value, obj, name) {
  
  if (!value)
    return value;
  let type = typeof value;  
  
  
  
  if (["object", "function"].indexOf(type) !== -1 && 
      "__isWrappedProxy" in value) {
    while("__isWrappedProxy" in value)
      value = value.valueOf(UNWRAP_ACCESS_KEY);
    return value;
  }
  
  
  
  if (type == "function")
    return ContentScriptFunctionWrapper(value, obj, name);
  
  
  
  
  
  if (type == "object")
    return ContentScriptObjectWrapper(value);

  if (["string", "number", "boolean"].indexOf(type) !== -1)
    return value;
  
  return value;
}




















function ContentScriptObjectWrapper(obj) {
  if ("___proxy" in obj && typeof obj.___proxy == "object")
    return obj.___proxy;

  function valueOf(key) {
    if (key === UNWRAP_ACCESS_KEY)
      return obj;
    return this;
  }

  let proxy = Proxy.create({
    
    getPropertyDescriptor:  function(name) {
      return Object.getOwnPropertyDescriptor(obj, name);
    },
    defineProperty: function(name, desc) {
      return Object.defineProperty(obj, name, desc);
    },
    getOwnPropertyNames: function () {
      return Object.getOwnPropertyNames(obj);
    },
    delete: function(name) {
      return delete obj[name];
    },
    
    has: function(name) {
      return name === "__isXrayWrapperProxy" ||
             name in obj;
    },
    hasOwn: function(name) {
      return Object.prototype.hasOwnProperty.call(obj, name);
    },
    get: function(receiver, name) {
      if (name == "valueOf")
        return valueOf;
      let value = obj[name];
      if (!value)
        return value;

      return unwrap(value);
    },
    set: function(receiver, name, val) {
      obj[name] = val;
      return true;
    },
    enumerate: function() {
      var result = [];
      for each (let name in obj) {
        result.push(name);
      };
      return result;
    },
    keys: function() {
      return Object.keys(obj);
    }
  });

  Object.defineProperty(obj, "___proxy", {value : proxy,
                                          writable : false,
                                          enumerable : false,
                                          configurable : false});

  return proxy;
}




const typedArraysCtor = [
  ArrayBuffer,
  Int8Array,
  Uint8Array,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array,
  Uint8ClampedArray
];




function wrap(value, obj, name, debug) {
  if (!value)
    return value;
  let type = typeof value;
  if (type == "object") {
    
    
    
    if (!Object.isExtensible(value) &&
        typedArraysCtor.indexOf(value.constructor) !== -1)
      return value;

    
    
    
    try {
      ("nonExistantAttribute" in value);
    }
    catch(e) {
      if (e.message.indexOf("Permission denied to access property") !== -1)
        return value;
    }

    
    
    
    
    
    
    
    
    
    if ("__isXrayWrapperProxy" in value)
      return value.valueOf(UNWRAP_ACCESS_KEY);

    
    
    while("__isWrappedProxy" in value) {
      value = value.valueOf(UNWRAP_ACCESS_KEY);
    }

    if (XPCNativeWrapper.unwrap(value) !== value)
      return getProxyForObject(value);
    
    
    
    return getProxyForObject(value);
  }
  if (type == "function") {
    if (XPCNativeWrapper.unwrap(value) !== value
        || (typeof value.toString === "function" && 
            value.toString().match(/\[native code\]/))) {
      return getProxyForFunction(value, NativeFunctionWrapper(value, obj, name));
    }
    return value;
  }
  if (type == "string")
    return value;
  if (type == "number")
    return value;
  if (type == "boolean")
    return value;
  
  return value;
}




function getProxyForObject(obj) {
  if (typeof obj != "object") {
    let msg = "tried to proxify something other than an object: " + typeof obj;
    console.warn(msg);
    throw msg;
  }
  if ("__isWrappedProxy" in obj) {
    return obj;
  }
  
  
  if (obj && obj.___proxy && obj.___proxy.valueOf(UNWRAP_ACCESS_KEY) === obj) {
    return obj.___proxy;
  }
  
  let proxy = Proxy.create(handlerMaker(obj));
  
  Object.defineProperty(obj, "___proxy", {value : proxy,
                                          writable : false,
                                          enumerable : false,
                                          configurable : false});
  return proxy;
}




function getProxyForFunction(fun, callTrap) {
  if (typeof fun != "function") {
    let msg = "tried to proxify something other than a function: " + typeof fun;
    console.warn(msg);
    throw msg;
  }
  if ("__isWrappedProxy" in fun)
    return obj;
  if ("___proxy" in fun)
    return fun.___proxy;
  
  let proxy = Proxy.createFunction(handlerMaker(fun), callTrap);
  
  Object.defineProperty(fun, "___proxy", {value : proxy,
                                          writable : false,
                                          enumerable : false,
                                          configurable : false});
  
  return proxy;
}




function isEventName(id) {
  if (id.indexOf("on") != 0 || id.length == 2) 
    return false;
  
  
  switch (id[2]) {
    case 'a' :
      return (id == "onabort" ||
              id == "onafterscriptexecute" ||
              id == "onafterprint");
    case 'b' :
      return (id == "onbeforeunload" ||
              id == "onbeforescriptexecute" ||
              id == "onblur" ||
              id == "onbeforeprint");
    case 'c' :
      return (id == "onchange"       ||
              id == "onclick"        ||
              id == "oncontextmenu"  ||
              id == "oncopy"         ||
              id == "oncut"          ||
              id == "oncanplay"      ||
              id == "oncanplaythrough");
    case 'd' :
      return (id == "ondblclick"     || 
              id == "ondrag"         ||
              id == "ondragend"      ||
              id == "ondragenter"    ||
              id == "ondragleave"    ||
              id == "ondragover"     ||
              id == "ondragstart"    ||
              id == "ondrop"         ||
              id == "ondurationchange");
    case 'e' :
      return (id == "onerror" ||
              id == "onemptied" ||
              id == "onended");
    case 'f' :
      return id == "onfocus";
    case 'h' :
      return id == "onhashchange";
    case 'i' :
      return (id == "oninput" ||
              id == "oninvalid");
    case 'k' :
      return (id == "onkeydown"      ||
              id == "onkeypress"     ||
              id == "onkeyup");
    case 'l' :
      return (id == "onload"           ||
              id == "onloadeddata"     ||
              id == "onloadedmetadata" ||
              id == "onloadstart");
    case 'm' :
      return (id == "onmousemove"    ||
              id == "onmouseout"     ||
              id == "onmouseover"    ||
              id == "onmouseup"      ||
              id == "onmousedown"    ||
              id == "onmessage");
    case 'p' :
      return (id == "onpaint"        ||
              id == "onpageshow"     ||
              id == "onpagehide"     ||
              id == "onpaste"        ||
              id == "onpopstate"     ||
              id == "onpause"        ||
              id == "onplay"         ||
              id == "onplaying"      ||
              id == "onprogress");
    case 'r' :
      return (id == "onreadystatechange" ||
              id == "onreset"            ||
              id == "onresize"           ||
              id == "onratechange");
    case 's' :
      return (id == "onscroll"       ||
              id == "onselect"       ||
              id == "onsubmit"       || 
              id == "onseeked"       ||
              id == "onseeking"      ||
              id == "onstalled"      ||
              id == "onsuspend");
    case 't':
      return id == "ontimeupdate" 
      








;
      
    case 'u' :
      return id == "onunload";
    case 'v':
      return id == "onvolumechange";
    case 'w':
      return id == "onwaiting";
    }
  
  return false;
}



const NODES_INDEXED_BY_NAME = ["IMG", "FORM", "APPLET", "EMBED", "OBJECT"];
const xRayWrappersMissFixes = [

  
  
  function (obj, name) {
    let i = parseInt(name);
    if (obj.toString().match(/HTMLCollection|NodeList/) && 
        i >= 0 && i < obj.length) {
      return wrap(XPCNativeWrapper(obj.wrappedJSObject[name]), obj, name);
    }
    return null;
  },

  
  
  
  function (obj, name) {
    if ("nodeType" in obj && obj.nodeType == 9) {
      let node = obj.wrappedJSObject[name];
      
      
      if (node && NODES_INDEXED_BY_NAME.indexOf(node.tagName) != -1)
        return wrap(XPCNativeWrapper(node));
    }
    return null;
  },

  
  
  
  function (obj, name) {
    if (typeof obj == "object" && "document" in obj) {
      
      try {
        obj.QueryInterface(Ci.nsIDOMWindow);
      }
      catch(e) {
        return null;
      }

      
      let i = parseInt(name);
      if (i >= 0 && i in obj) {
        return wrap(XPCNativeWrapper(obj[i]));
      }

      
      if (name in obj.wrappedJSObject) {
        let win = obj.wrappedJSObject[name];
        let nodes = obj.document.getElementsByName(name);
        for (let i = 0, l = nodes.length; i < l; i++) {
          let node = nodes[i];
          if ("contentWindow" in node && node.contentWindow == win)
            return wrap(node.contentWindow);
        }
      }
    }
    return null;
  },

  
  
  function (obj, name) {
    if (typeof obj == "object" && "tagName" in obj && obj.tagName == "FORM") {
      let match = obj.wrappedJSObject[name];
      let nodes = obj.ownerDocument.getElementsByName(name);
      for (let i = 0, l = nodes.length; i < l; i++) {
        let node = nodes[i];
        if (node == match)
          return wrap(node);
      }
    }
    return null;
  }

];




const xRayWrappersMethodsFixes = {
  
  
  
  
  
  
  
  postMessage: function (obj) {
    
    try {
      obj.QueryInterface(Ci.nsIDOMWindow);
    }
    catch(e) {
      return null;
    }

    
    let f = function postMessage(message, targetOrigin) {
      let jscode = "this.postMessage(";
      if (typeof message != "string")
        jscode += JSON.stringify(message);
      else
        jscode += "'" + message.toString().replace(/['\\]/g,"\\$&") + "'";

      targetOrigin = targetOrigin.toString().replace(/['\\]/g,"\\$&");

      jscode += ", '" + targetOrigin + "')";
      return this.wrappedJSObject.eval(jscode);
    };
    return getProxyForFunction(f, NativeFunctionWrapper(f));
  },
  
  
  
  
  
  mozMatchesSelector: function (obj) {
    
    try {
      
      
      obj.QueryInterface("nsIDOMElement" in Ci ? Ci.nsIDOMElement :
                                                 Ci.nsIDOMNSElement);
    }
    catch(e) {
      return null;
    }
    
    
    let f = function mozMatchesSelector(selectors) {
      return this.mozMatchesSelector(selectors);
    };

    return getProxyForFunction(f, NativeFunctionWrapper(f));
  },

  
  
  
  pushState: function (obj) {
    
    try {
      obj.QueryInterface(Ci.nsIDOMHistory);
    }
    catch(e) {
      return null;
    }
    let f = function fix() {
      
      
      return this.pushState.apply(this, JSON.parse(JSON.stringify(Array.slice(arguments))));
    };

    return getProxyForFunction(f, NativeFunctionWrapper(f));
  },
  replaceState: function (obj) {
    
    try {
      obj.QueryInterface(Ci.nsIDOMHistory);
    }
    catch(e) {
      return null;
    }
    let f = function fix() {
      
      
      return this.replaceState.apply(this, JSON.parse(JSON.stringify(Array.slice(arguments))));
    };

    return getProxyForFunction(f, NativeFunctionWrapper(f));
  },

  
  
  observe: function observe(obj) {
    
    try {
      
      if ("nsIDOMMutationObserver" in Ci)
        obj.QueryInterface(Ci.nsIDOMMutationObserver);
      else
        return null;
    }
    catch(e) {
      return null;
    }
    return function nsIDOMMutationObserverObserveFix(target, options) {
      
      let self = this && typeof this.valueOf == "function" ?
                 this.valueOf(UNWRAP_ACCESS_KEY) : this;
      
      let targetXray = unwrap(target);
      
      let result = wrap(self.observe(targetXray, options));
      
      return wrap(result);
    };
  }
};




function handlerMaker(obj) {
  
  let overload = {};
  
  let expando = {};
  
  let methodFixes = {};
  return {
    
    getPropertyDescriptor:  function(name) {
      return Object.getOwnPropertyDescriptor(obj, name);
    },
    defineProperty: function(name, desc) {
      return Object.defineProperty(obj, name, desc);
    },
    getOwnPropertyNames: function () {
      return Object.getOwnPropertyNames(obj);
    },
    delete: function(name) {
      delete expando[name];
      delete overload[name];
      return delete obj[name];
    },
    
    
    has: function(name) {
      if (name == "___proxy") return false;
      if (isEventName(name)) {
        
        
        return name in expando;
      }
      return name in obj || name in overload || name == "__isWrappedProxy" ||
             undefined !== this.get(null, name);
    },
    hasOwn: function(name) {
      return Object.prototype.hasOwnProperty.call(obj, name);
    },
    get: function(receiver, name) {
      if (name == "___proxy")
        return undefined;
      
      
      
      if (name == "toString") {
        
        
        
        
        let unwrappedObj = XPCNativeWrapper.unwrap(obj);
        return wrap(function () {
          return unwrappedObj.toString.call(
                   this.valueOf(UNWRAP_ACCESS_KEY), arguments);
        }, obj, name);
      }

      
      if (name == "valueOf")
        return function (key) {
          if (key === UNWRAP_ACCESS_KEY)
            return obj;
          return this;
        };
      
      
      
      
      
      if (name in overload &&
          overload[name] != Object.getPrototypeOf(overload)[name] &&
          name != "__proto__") {
        return overload[name];
      }
      
      
      
      if (isEventName(name)) {
        
        return name in expando ? expando[name].original : undefined;
      }
      
      
      if (name in methodFixes && 
          methodFixes[name] != Object.getPrototypeOf(methodFixes)[name] &&
          name != "__proto__")
        return methodFixes[name];
      if (Object.keys(xRayWrappersMethodsFixes).indexOf(name) !== -1) {
        let fix = xRayWrappersMethodsFixes[name](obj);
        if (fix)
          return methodFixes[name] = fix;
      }
      
      let o = obj[name];
      
      
      if (!o) {
        for each(let atttributeFixer in xRayWrappersMissFixes) {
          let fix = atttributeFixer(obj, name);
          if (fix)
            return fix;
        }
      }

      
      return wrap(o, obj, name);
      
    },
    
    set: function(receiver, name, val) {

      if (isEventName(name)) {
        
        let shortName = name.replace(/^on/,"");
        
        
        if (expando[name]) {
          obj.removeEventListener(shortName, expando[name], true);
          delete expando[name];
        }
        
        
        if (typeof val != "function")
          return false;
        
        
        let original = val;
        val = ContentScriptFunctionWrapper(val);
        expando[name] = val;
        val.original = original;
        obj.addEventListener(name.replace(/^on/, ""), val, true);
        return true;
      }
      
      obj[name] = val;
      
      
      
      
      
      
      
      
      
      if ((typeof val == "function" || typeof val == "object") && name) {
        overload[name] = val;
      }
      
      return true;
    },
    
    enumerate: function() {
      var result = [];
      for each (let name in Object.keys(obj)) {
        result.push(name);
      };
      return result;
    },
    
    keys: function() {
      return Object.keys(obj);
    }
  };
};





function create(object) {
  if ("wrappedJSObject" in object)
    object = object.wrappedJSObject;
  let xpcWrapper = XPCNativeWrapper(object);
  
  
  
  
  
  if (object === xpcWrapper) {
    return object;
  }
  return getProxyForObject(xpcWrapper);
}
