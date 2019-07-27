





"use strict";

const { Cu, Ci } = require("chrome");
const { GeneratedLocation } = require("devtools/server/actors/common");
const { DebuggerServer } = require("devtools/server/main")
const DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
const { dbg_assert, dumpn } = DevToolsUtils;
const PromiseDebugging = require("PromiseDebugging");

const TYPED_ARRAY_CLASSES = ["Uint8Array", "Uint8ClampedArray", "Uint16Array",
      "Uint32Array", "Int8Array", "Int16Array", "Int32Array", "Float32Array",
      "Float64Array"];



const OBJECT_PREVIEW_MAX_ITEMS = 10;

























function ObjectActor(obj, {
  createValueGrip,
  sources,
  createEnvironmentActor,
  getGripDepth,
  incrementGripDepth,
  decrementGripDepth,
  getGlobalDebugObject
}) {
  dbg_assert(!obj.optimizedOut,
    "Should not create object actors for optimized out values!");
  this.obj = obj;
  this.hooks = {
    createValueGrip,
    sources,
    createEnvironmentActor,
    getGripDepth,
    incrementGripDepth,
    decrementGripDepth,
    getGlobalDebugObject
  };
  this.iterators = new Set();
}

ObjectActor.prototype = {
  actorPrefix: "obj",

  


  grip: function() {
    this.hooks.incrementGripDepth();

    let g = {
      "type": "object",
      "class": this.obj.class,
      "actor": this.actorID,
      "extensible": this.obj.isExtensible(),
      "frozen": this.obj.isFrozen(),
      "sealed": this.obj.isSealed()
    };

    if (this.obj.class != "DeadObject") {
      if (this.obj.class == "Promise") {
        g.promiseState = this._createPromiseState();
      }

      
      
      
      try {
        
        if (this.obj.class != "Function") {
          g.ownPropertyLength = this.obj.getOwnPropertyNames().length;
        }
      } catch(e) {}

      let raw = this.obj.unsafeDereference();

      
      
      if (Cu) {
        raw = Cu.unwaiveXrays(raw);
      }

      if (!DevToolsUtils.isSafeJSObject(raw)) {
        raw = null;
      }

      let previewers = DebuggerServer.ObjectActorPreviewers[this.obj.class] ||
                       DebuggerServer.ObjectActorPreviewers.Object;
      for (let fn of previewers) {
        try {
          if (fn(this, g, raw)) {
            break;
          }
        } catch (e) {
          let msg = "ObjectActor.prototype.grip previewer function";
          DevToolsUtils.reportException(msg, e);
        }
      }
    }

    this.hooks.decrementGripDepth();
    return g;
  },

  


  _createPromiseState: function() {
    const { state, value, reason } = getPromiseState(this.obj);
    let promiseState = { state };
    let rawPromise = this.obj.unsafeDereference();

    if (state == "fulfilled") {
      promiseState.value = this.hooks.createValueGrip(value);
    } else if (state == "rejected") {
      promiseState.reason = this.hooks.createValueGrip(reason);
    }

    promiseState.creationTimestamp = Date.now() -
      PromiseDebugging.getPromiseLifetime(rawPromise);

    
    
    try {
      promiseState.timeToSettle = PromiseDebugging.getTimeToSettle(rawPromise);
    } catch(e) {}

    return promiseState;
  },

  


  release: function() {
    if (this.registeredPool.objectActors) {
      this.registeredPool.objectActors.delete(this.obj);
    }
    this.iterators.forEach(actor => this.registeredPool.removeActor(actor));
    this.iterators.clear();
    this.registeredPool.removeActor(this);
  },

  



  onDefinitionSite: function() {
    if (this.obj.class != "Function") {
      return {
        from: this.actorID,
        error: "objectNotFunction",
        message: this.actorID + " is not a function."
      };
    }

    if (!this.obj.script) {
      return {
        from: this.actorID,
        error: "noScript",
        message: this.actorID + " has no Debugger.Script"
      };
    }

    return this.hooks.sources().getOriginalLocation(new GeneratedLocation(
      this.hooks.sources().createNonSourceMappedActor(this.obj.script.source),
      this.obj.script.startLine,
      0 
    )).then((originalLocation) => {
      return {
        source: originalLocation.originalSourceActor.form(),
        line: originalLocation.originalLine,
        column: originalLocation.originalColumn
      };
    });
  },

  



  onOwnPropertyNames: function() {
    return { from: this.actorID,
             ownPropertyNames: this.obj.getOwnPropertyNames() };
  },

  






  onEnumProperties: function(request) {
    let actor = new PropertyIteratorActor(this, request.options);
    this.registeredPool.addActor(actor);
    this.iterators.add(actor);
    return { iterator: actor.grip() };
  },

  



  onPrototypeAndProperties: function() {
    let ownProperties = Object.create(null);
    let names;
    try {
      names = this.obj.getOwnPropertyNames();
    } catch (ex) {
      
      
      return { from: this.actorID,
               prototype: this.hooks.createValueGrip(null),
               ownProperties: ownProperties,
               safeGetterValues: Object.create(null) };
    }
    for (let name of names) {
      ownProperties[name] = this._propertyDescriptor(name);
    }
    return { from: this.actorID,
             prototype: this.hooks.createValueGrip(this.obj.proto),
             ownProperties: ownProperties,
             safeGetterValues: this._findSafeGetterValues(names) };
  },

  












  _findSafeGetterValues: function(ownProperties, limit = 0) {
    let safeGetterValues = Object.create(null);
    let obj = this.obj;
    let level = 0, i = 0;

    while (obj) {
      let getters = this._findSafeGetters(obj);
      for (let name of getters) {
        
        
        
        if (name in safeGetterValues ||
            (obj != this.obj && ownProperties.indexOf(name) !== -1)) {
          continue;
        }

        
        if (!obj.proto && name == "__proto__") {
          continue;
        }

        let desc = null, getter = null;
        try {
          desc = obj.getOwnPropertyDescriptor(name);
          getter = desc.get;
        } catch (ex) {
          
        }
        if (!getter) {
          obj._safeGetters = null;
          continue;
        }

        let result = getter.call(this.obj);
        if (result && !("throw" in result)) {
          let getterValue = undefined;
          if ("return" in result) {
            getterValue = result.return;
          } else if ("yield" in result) {
            getterValue = result.yield;
          }
          
          
          if (getterValue !== undefined) {
            safeGetterValues[name] = {
              getterValue: this.hooks.createValueGrip(getterValue),
              getterPrototypeLevel: level,
              enumerable: desc.enumerable,
              writable: level == 0 ? desc.writable : true,
            };
            if (limit && ++i == limit) {
              break;
            }
          }
        }
      }
      if (limit && i == limit) {
        break;
      }

      obj = obj.proto;
      level++;
    }

    return safeGetterValues;
  },

  










  _findSafeGetters: function(object) {
    if (object._safeGetters) {
      return object._safeGetters;
    }

    let getters = new Set();
    let names = [];
    try {
      names = object.getOwnPropertyNames()
    } catch (ex) {
      
      
    }

    for (let name of names) {
      let desc = null;
      try {
        desc = object.getOwnPropertyDescriptor(name);
      } catch (e) {
        
        
      }
      if (!desc || desc.value !== undefined || !("get" in desc)) {
        continue;
      }

      if (DevToolsUtils.hasSafeGetter(desc)) {
        getters.add(name);
      }
    }

    object._safeGetters = getters;
    return getters;
  },

  


  onPrototype: function() {
    return { from: this.actorID,
             prototype: this.hooks.createValueGrip(this.obj.proto) };
  },

  






  onProperty: function(request) {
    if (!request.name) {
      return { error: "missingParameter",
               message: "no property name was specified" };
    }

    return { from: this.actorID,
             descriptor: this._propertyDescriptor(request.name) };
  },

  


  onDisplayString: function() {
    const string = stringify(this.obj);
    return { from: this.actorID,
             displayString: this.hooks.createValueGrip(string) };
  },

  













  _propertyDescriptor: function(name, onlyEnumerable) {
    let desc;
    try {
      desc = this.obj.getOwnPropertyDescriptor(name);
    } catch (e) {
      
      
      
      return {
        configurable: false,
        writable: false,
        enumerable: false,
        value: e.name
      };
    }

    if (!desc || onlyEnumerable && !desc.enumerable) {
      return undefined;
    }

    let retval = {
      configurable: desc.configurable,
      enumerable: desc.enumerable
    };

    if ("value" in desc) {
      retval.writable = desc.writable;
      retval.value = this.hooks.createValueGrip(desc.value);
    } else {
      if ("get" in desc) {
        retval.get = this.hooks.createValueGrip(desc.get);
      }
      if ("set" in desc) {
        retval.set = this.hooks.createValueGrip(desc.set);
      }
    }
    return retval;
  },

  





  onDecompile: function(request) {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "decompile request is only valid for object grips " +
                        "with a 'Function' class." };
    }

    return { from: this.actorID,
             decompiledCode: this.obj.decompile(!!request.pretty) };
  },

  


  onParameterNames: function() {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "'parameterNames' request is only valid for object " +
                        "grips with a 'Function' class." };
    }

    return { parameterNames: this.obj.parameterNames };
  },

  


  onRelease: function() {
    this.release();
    return {};
  },

  


  onScope: function() {
    if (this.obj.class !== "Function") {
      return { error: "objectNotFunction",
               message: "scope request is only valid for object grips with a" +
                        " 'Function' class." };
    }

    let envActor = this.hooks.createEnvironmentActor(this.obj.environment,
                                                     this.registeredPool);
    if (!envActor) {
      return { error: "notDebuggee",
               message: "cannot access the environment of this function." };
    }

    return { from: this.actorID, scope: envActor.form() };
  },

  







  onDependentPromises: function() {
    if (this.obj.class != "Promise") {
      return { error: "objectNotPromise",
               message: "'dependentPromises' request is only valid for " +
                        "object grips with a 'Promise' class." };
    }

    let rawPromise = this.obj.unsafeDereference();
    let promises = PromiseDebugging.getDependentPromises(rawPromise).map(p =>
      this.hooks.createValueGrip(this.obj.makeDebuggeeValue(p)));

    return { promises };
  }
};

ObjectActor.prototype.requestTypes = {
  "definitionSite": ObjectActor.prototype.onDefinitionSite,
  "parameterNames": ObjectActor.prototype.onParameterNames,
  "prototypeAndProperties": ObjectActor.prototype.onPrototypeAndProperties,
  "enumProperties": ObjectActor.prototype.onEnumProperties,
  "prototype": ObjectActor.prototype.onPrototype,
  "property": ObjectActor.prototype.onProperty,
  "displayString": ObjectActor.prototype.onDisplayString,
  "ownPropertyNames": ObjectActor.prototype.onOwnPropertyNames,
  "decompile": ObjectActor.prototype.onDecompile,
  "release": ObjectActor.prototype.onRelease,
  "scope": ObjectActor.prototype.onScope,
  "dependentPromises": ObjectActor.prototype.onDependentPromises
};
























function PropertyIteratorActor(objectActor, options){
  this.objectActor = objectActor;

  let ownProperties = Object.create(null);
  let names = [];
  try {
    names = this.objectActor.obj.getOwnPropertyNames();
  } catch (ex) {}

  let safeGetterValues = {};
  let safeGetterNames = [];
  if (!options.ignoreSafeGetters) {
    
    safeGetterValues = this.objectActor._findSafeGetterValues(names);
    safeGetterNames = Object.keys(safeGetterValues);
    for (let name of safeGetterNames) {
      if (names.indexOf(name) === -1) {
        names.push(name);
      }
    }
  }

  if (options.ignoreIndexedProperties || options.ignoreNonIndexedProperties) {
    let length = DevToolsUtils.getProperty(this.objectActor.obj, "length");
    if (typeof(length) !== "number") {
      
      
      length = 0;
      for (let key of names) {
        if (isNaN(key) || key != length++) {
          break;
        }
      }
    }

    if (options.ignoreIndexedProperties) {
      names = names.filter(i => {
        
        
        
        i = parseFloat(i);
        return !Number.isInteger(i) || i < 0 || i >= length;
      });
    }

    if (options.ignoreNonIndexedProperties) {
      names = names.filter(i => {
        i = parseFloat(i);
        return Number.isInteger(i) && i >= 0 && i < length;
      });
    }
  }

  if (options.query) {
    let { query } = options;
    query = query.toLowerCase();
    names = names.filter(name => {
      return name.toLowerCase().includes(query);
    });
  }

  if (options.sort) {
    names.sort();
  }

  
  for (let name of names) {
    let desc = this.objectActor._propertyDescriptor(name);
    if (!desc) {
      desc = safeGetterValues[name];
    }
    else if (name in safeGetterValues) {
      
      let { getterValue, getterPrototypeLevel } = safeGetterValues[name];
      desc.getterValue = getterValue;
      desc.getterPrototypeLevel = getterPrototypeLevel;
    }
    ownProperties[name] = desc;
  }

  this.names = names;
  this.ownProperties = ownProperties;
}

PropertyIteratorActor.prototype = {
  actorPrefix: "propertyIterator",

  grip: function() {
    return {
      type: "propertyIterator",
      actor: this.actorID,
      count: this.names.length
    };
  },

  names: function({ indexes }) {
    let list = [];
    for (let idx of indexes) {
      list.push(this.names[idx]);
    }
    return {
      names: list
    };
  },

  slice: function({ start, count }) {
    let names = this.names.slice(start, start + count);
    let props = Object.create(null);
    for (let name of names) {
      props[name] = this.ownProperties[name];
    }
    return {
      ownProperties: props
    };
  },

  all: function() {
    return {
      ownProperties: this.ownProperties
    };
  }
};

PropertyIteratorActor.prototype.requestTypes = {
  "names": PropertyIteratorActor.prototype.names,
  "slice": PropertyIteratorActor.prototype.slice,
  "all": PropertyIteratorActor.prototype.all,
};

















DebuggerServer.ObjectActorPreviewers = {
  String: [function({obj, hooks}, grip) {
    let result = genericObjectPreviewer("String", String, obj, hooks);
    let length = DevToolsUtils.getProperty(obj, "length");

    if (!result || typeof length != "number") {
      return false;
    }

    grip.preview = {
      kind: "ArrayLike",
      length: length
    };

    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let items = grip.preview.items = [];

    const max = Math.min(result.value.length, OBJECT_PREVIEW_MAX_ITEMS);
    for (let i = 0; i < max; i++) {
      let value = hooks.createValueGrip(result.value[i]);
      items.push(value);
    }

    return true;
  }],

  Boolean: [function({obj, hooks}, grip) {
    let result = genericObjectPreviewer("Boolean", Boolean, obj, hooks);
    if (result) {
      grip.preview = result;
      return true;
    }

    return false;
  }],

  Number: [function({obj, hooks}, grip) {
    let result = genericObjectPreviewer("Number", Number, obj, hooks);
    if (result) {
      grip.preview = result;
      return true;
    }

    return false;
  }],

  Function: [function({obj, hooks}, grip) {
    if (obj.name) {
      grip.name = obj.name;
    }

    if (obj.displayName) {
      grip.displayName = obj.displayName.substr(0, 500);
    }

    if (obj.parameterNames) {
      grip.parameterNames = obj.parameterNames;
    }

    
    
    let userDisplayName;
    try {
      userDisplayName = obj.getOwnPropertyDescriptor("displayName");
    } catch (e) {
      
      
      dumpn(e);
    }

    if (userDisplayName && typeof userDisplayName.value == "string" &&
        userDisplayName.value) {
      grip.userDisplayName = hooks.createValueGrip(userDisplayName.value);
    }

    let dbgGlobal = hooks.getGlobalDebugObject();
    if (dbgGlobal) {
      let script = dbgGlobal.makeDebuggeeValue(obj.unsafeDereference()).script;
      if (script) {
        grip.location = {
          url: script.url,
          line: script.startLine
        };
      }
    }

    return true;
  }],

  RegExp: [function({obj, hooks}, grip) {
    
    if (!obj.proto || obj.proto.class != "RegExp") {
      return false;
    }

    let str = RegExp.prototype.toString.call(obj.unsafeDereference());
    grip.displayString = hooks.createValueGrip(str);
    return true;
  }],

  Date: [function({obj, hooks}, grip) {
    let time = Date.prototype.getTime.call(obj.unsafeDereference());

    grip.preview = {
      timestamp: hooks.createValueGrip(time),
    };
    return true;
  }],

  Array: [function({obj, hooks}, grip) {
    let length = DevToolsUtils.getProperty(obj, "length");
    if (typeof length != "number") {
      return false;
    }

    grip.preview = {
      kind: "ArrayLike",
      length: length,
    };

    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let items = grip.preview.items = [];

    for (let i = 0; i < length; ++i) {
      
      
      
      
      
      
      let desc = Object.getOwnPropertyDescriptor(Cu.waiveXrays(raw), i);
      if (desc && !desc.get && !desc.set) {
        let value = Cu.unwaiveXrays(desc.value);
        value = makeDebuggeeValueIfNeeded(obj, value);
        items.push(hooks.createValueGrip(value));
      } else {
        items.push(null);
      }

      if (items.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }],

  Set: [function({obj, hooks}, grip) {
    let size = DevToolsUtils.getProperty(obj, "size");
    if (typeof size != "number") {
      return false;
    }

    grip.preview = {
      kind: "ArrayLike",
      length: size,
    };

    
    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let items = grip.preview.items = [];
    
    
    
    
    
    
    
    
    
    
    for (let item of Cu.waiveXrays(Set.prototype.values.call(raw))) {
      item = Cu.unwaiveXrays(item);
      item = makeDebuggeeValueIfNeeded(obj, item);
      items.push(hooks.createValueGrip(item));
      if (items.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }],

  Map: [function({obj, hooks}, grip) {
    let size = DevToolsUtils.getProperty(obj, "size");
    if (typeof size != "number") {
      return false;
    }

    grip.preview = {
      kind: "MapLike",
      size: size,
    };

    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let entries = grip.preview.entries = [];
    
    
    
    
    
    
    
    
    
    
    
    
    for (let keyValuePair of Cu.waiveXrays(Map.prototype.entries.call(raw))) {
      let key = Cu.unwaiveXrays(keyValuePair[0]);
      let value = Cu.unwaiveXrays(keyValuePair[1]);
      key = makeDebuggeeValueIfNeeded(obj, key);
      value = makeDebuggeeValueIfNeeded(obj, value);
      entries.push([hooks.createValueGrip(key),
                    hooks.createValueGrip(value)]);
      if (entries.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }],

  DOMStringMap: [function({obj, hooks}, grip, rawObj) {
    if (!rawObj) {
      return false;
    }

    let keys = obj.getOwnPropertyNames();
    grip.preview = {
      kind: "MapLike",
      size: keys.length,
    };

    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let entries = grip.preview.entries = [];
    for (let key of keys) {
      let value = makeDebuggeeValueIfNeeded(obj, rawObj[key]);
      entries.push([key, hooks.createValueGrip(value)]);
      if (entries.length == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    return true;
  }],
};


















function genericObjectPreviewer(className, classObj, obj, hooks) {
  if (!obj.proto || obj.proto.class != className) {
    return null;
  }

  let raw = obj.unsafeDereference();
  let v = null;
  try {
    v = classObj.prototype.valueOf.call(raw);
  } catch (ex) {
    
    return null;
  }

  if (v !== null) {
    v = hooks.createValueGrip(makeDebuggeeValueIfNeeded(obj, v));
    return { value: v };
  }

  return null;
}


DebuggerServer.ObjectActorPreviewers.Object = [
  function TypedArray({obj, hooks}, grip) {
    if (TYPED_ARRAY_CLASSES.indexOf(obj.class) == -1) {
      return false;
    }

    let length = DevToolsUtils.getProperty(obj, "length");
    if (typeof length != "number") {
      return false;
    }

    grip.preview = {
      kind: "ArrayLike",
      length: length,
    };

    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let raw = obj.unsafeDereference();
    let global = Cu.getGlobalForObject(DebuggerServer);
    let classProto = global[obj.class].prototype;
    
    
    let safeView = Cu.cloneInto(classProto.subarray.call(raw, 0,
      OBJECT_PREVIEW_MAX_ITEMS), global);
    let items = grip.preview.items = [];
    for (let i = 0; i < safeView.length; i++) {
      items.push(safeView[i]);
    }

    return true;
  },

  function Error({obj, hooks}, grip) {
    switch (obj.class) {
      case "Error":
      case "EvalError":
      case "RangeError":
      case "ReferenceError":
      case "SyntaxError":
      case "TypeError":
      case "URIError":
        let name = DevToolsUtils.getProperty(obj, "name");
        let msg = DevToolsUtils.getProperty(obj, "message");
        let stack = DevToolsUtils.getProperty(obj, "stack");
        let fileName = DevToolsUtils.getProperty(obj, "fileName");
        let lineNumber = DevToolsUtils.getProperty(obj, "lineNumber");
        let columnNumber = DevToolsUtils.getProperty(obj, "columnNumber");
        grip.preview = {
          kind: "Error",
          name: hooks.createValueGrip(name),
          message: hooks.createValueGrip(msg),
          stack: hooks.createValueGrip(stack),
          fileName: hooks.createValueGrip(fileName),
          lineNumber: hooks.createValueGrip(lineNumber),
          columnNumber: hooks.createValueGrip(columnNumber),
        };
        return true;
      default:
        return false;
    }
  },

  function CSSMediaRule({obj, hooks}, grip, rawObj) {
    if (isWorker || !rawObj || !(rawObj instanceof Ci.nsIDOMCSSMediaRule)) {
      return false;
    }
    grip.preview = {
      kind: "ObjectWithText",
      text: hooks.createValueGrip(rawObj.conditionText),
    };
    return true;
  },

  function CSSStyleRule({obj, hooks}, grip, rawObj) {
    if (isWorker || !rawObj || !(rawObj instanceof Ci.nsIDOMCSSStyleRule)) {
      return false;
    }
    grip.preview = {
      kind: "ObjectWithText",
      text: hooks.createValueGrip(rawObj.selectorText),
    };
    return true;
  },

  function ObjectWithURL({obj, hooks}, grip, rawObj) {
    if (isWorker || !rawObj || !(rawObj instanceof Ci.nsIDOMCSSImportRule ||
                                 rawObj instanceof Ci.nsIDOMCSSStyleSheet ||
                                 rawObj instanceof Ci.nsIDOMLocation ||
                                 rawObj instanceof Ci.nsIDOMWindow)) {
      return false;
    }

    let url;
    if (rawObj instanceof Ci.nsIDOMWindow && rawObj.location) {
      url = rawObj.location.href;
    } else if (rawObj.href) {
      url = rawObj.href;
    } else {
      return false;
    }

    grip.preview = {
      kind: "ObjectWithURL",
      url: hooks.createValueGrip(url),
    };

    return true;
  },

  function ArrayLike({obj, hooks}, grip, rawObj) {
    if (isWorker || !rawObj ||
        obj.class != "DOMStringList" &&
        obj.class != "DOMTokenList" &&
        !(rawObj instanceof Ci.nsIDOMMozNamedAttrMap ||
          rawObj instanceof Ci.nsIDOMCSSRuleList ||
          rawObj instanceof Ci.nsIDOMCSSValueList ||
          rawObj instanceof Ci.nsIDOMFileList ||
          rawObj instanceof Ci.nsIDOMFontFaceList ||
          rawObj instanceof Ci.nsIDOMMediaList ||
          rawObj instanceof Ci.nsIDOMNodeList ||
          rawObj instanceof Ci.nsIDOMStyleSheetList)) {
      return false;
    }

    if (typeof rawObj.length != "number") {
      return false;
    }

    grip.preview = {
      kind: "ArrayLike",
      length: rawObj.length,
    };

    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let items = grip.preview.items = [];

    for (let i = 0; i < rawObj.length &&
                    items.length < OBJECT_PREVIEW_MAX_ITEMS; i++) {
      let value = makeDebuggeeValueIfNeeded(obj, rawObj[i]);
      items.push(hooks.createValueGrip(value));
    }

    return true;
  },

  function CSSStyleDeclaration({obj, hooks}, grip, rawObj) {
    if (isWorker || !rawObj ||
        !(rawObj instanceof Ci.nsIDOMCSSStyleDeclaration)) {
      return false;
    }

    grip.preview = {
      kind: "MapLike",
      size: rawObj.length,
    };

    let entries = grip.preview.entries = [];

    for (let i = 0; i < OBJECT_PREVIEW_MAX_ITEMS &&
                    i < rawObj.length; i++) {
      let prop = rawObj[i];
      let value = rawObj.getPropertyValue(prop);
      entries.push([prop, hooks.createValueGrip(value)]);
    }

    return true;
  },

  function DOMNode({obj, hooks}, grip, rawObj) {
    if (isWorker || obj.class == "Object" || !rawObj ||
        !(rawObj instanceof Ci.nsIDOMNode)) {
      return false;
    }

    let preview = grip.preview = {
      kind: "DOMNode",
      nodeType: rawObj.nodeType,
      nodeName: rawObj.nodeName,
    };

    if (rawObj instanceof Ci.nsIDOMDocument && rawObj.location) {
      preview.location = hooks.createValueGrip(rawObj.location.href);
    } else if (rawObj instanceof Ci.nsIDOMDocumentFragment) {
      preview.childNodesLength = rawObj.childNodes.length;

      if (hooks.getGripDepth() < 2) {
        preview.childNodes = [];
        for (let node of rawObj.childNodes) {
          let actor = hooks.createValueGrip(obj.makeDebuggeeValue(node));
          preview.childNodes.push(actor);
          if (preview.childNodes.length == OBJECT_PREVIEW_MAX_ITEMS) {
            break;
          }
        }
      }
    } else if (rawObj instanceof Ci.nsIDOMElement) {
      
      if (rawObj instanceof Ci.nsIDOMHTMLElement) {
        preview.nodeName = preview.nodeName.toLowerCase();
      }

      let i = 0;
      preview.attributes = {};
      preview.attributesLength = rawObj.attributes.length;
      for (let attr of rawObj.attributes) {
        preview.attributes[attr.nodeName] = hooks.createValueGrip(attr.value);
        if (++i == OBJECT_PREVIEW_MAX_ITEMS) {
          break;
        }
      }
    } else if (rawObj instanceof Ci.nsIDOMAttr) {
      preview.value = hooks.createValueGrip(rawObj.value);
    } else if (rawObj instanceof Ci.nsIDOMText ||
               rawObj instanceof Ci.nsIDOMComment) {
      preview.textContent = hooks.createValueGrip(rawObj.textContent);
    }

    return true;
  },

  function DOMEvent({obj, hooks}, grip, rawObj) {
    if (isWorker || !rawObj || !(rawObj instanceof Ci.nsIDOMEvent)) {
      return false;
    }

    let preview = grip.preview = {
      kind: "DOMEvent",
      type: rawObj.type,
      properties: Object.create(null),
    };

    if (hooks.getGripDepth() < 2) {
      let target = obj.makeDebuggeeValue(rawObj.target);
      preview.target = hooks.createValueGrip(target);
    }

    let props = [];
    if (rawObj instanceof Ci.nsIDOMMouseEvent) {
      props.push("buttons", "clientX", "clientY", "layerX", "layerY");
    } else if (rawObj instanceof Ci.nsIDOMKeyEvent) {
      let modifiers = [];
      if (rawObj.altKey) {
        modifiers.push("Alt");
      }
      if (rawObj.ctrlKey) {
        modifiers.push("Control");
      }
      if (rawObj.metaKey) {
        modifiers.push("Meta");
      }
      if (rawObj.shiftKey) {
        modifiers.push("Shift");
      }
      preview.eventKind = "key";
      preview.modifiers = modifiers;

      props.push("key", "charCode", "keyCode");
    } else if (rawObj instanceof Ci.nsIDOMTransitionEvent) {
      props.push("propertyName", "pseudoElement");
    } else if (rawObj instanceof Ci.nsIDOMAnimationEvent) {
      props.push("animationName", "pseudoElement");
    } else if (rawObj instanceof Ci.nsIDOMClipboardEvent) {
      props.push("clipboardData");
    }

    
    for (let prop of props) {
      let value = rawObj[prop];
      if (value && (typeof value == "object" || typeof value == "function")) {
        
        if (hooks.getGripDepth() > 1) {
          continue;
        }
        value = obj.makeDebuggeeValue(value);
      }
      preview.properties[prop] = hooks.createValueGrip(value);
    }

    
    if (!props.length) {
      let i = 0;
      for (let prop in rawObj) {
        let value = rawObj[prop];
        if (prop == "target" || prop == "type" || value === null ||
            typeof value == "function") {
          continue;
        }
        if (value && typeof value == "object") {
          if (hooks.getGripDepth() > 1) {
            continue;
          }
          value = obj.makeDebuggeeValue(value);
        }
        preview.properties[prop] = hooks.createValueGrip(value);
        if (++i == OBJECT_PREVIEW_MAX_ITEMS) {
          break;
        }
      }
    }

    return true;
  },

  function DOMException({obj, hooks}, grip, rawObj) {
    if (isWorker || !rawObj || !(rawObj instanceof Ci.nsIDOMDOMException)) {
      return false;
    }

    grip.preview = {
      kind: "DOMException",
      name: hooks.createValueGrip(rawObj.name),
      message: hooks.createValueGrip(rawObj.message),
      code: hooks.createValueGrip(rawObj.code),
      result: hooks.createValueGrip(rawObj.result),
      filename: hooks.createValueGrip(rawObj.filename),
      lineNumber: hooks.createValueGrip(rawObj.lineNumber),
      columnNumber: hooks.createValueGrip(rawObj.columnNumber),
    };

    return true;
  },

  function PseudoArray({obj, hooks}, grip, rawObj) {
    let length = 0;

    
    let keys = obj.getOwnPropertyNames();
    if (keys.length == 0) {
      return false;
    }
    for (let key of keys) {
      if (isNaN(key) || key != length++) {
        return false;
      }
    }

    grip.preview = {
      kind: "ArrayLike",
      length: length,
    };

    
    if (hooks.getGripDepth() > 1) {
      return true;
    }

    let items = grip.preview.items = [];

    let i = 0;
    for (let key of keys) {
      if (rawObj.hasOwnProperty(key) && i++ < OBJECT_PREVIEW_MAX_ITEMS) {
        let value = makeDebuggeeValueIfNeeded(obj, rawObj[key]);
        items.push(hooks.createValueGrip(value));
      }
    }

    return true;
  },

  function GenericObject(objectActor, grip) {
    let {obj, hooks} = objectActor;
    if (grip.preview || grip.displayString || hooks.getGripDepth() > 1) {
      return false;
    }

    let i = 0, names = [];
    let preview = grip.preview = {
      kind: "Object",
      ownProperties: Object.create(null),
    };

    try {
      names = obj.getOwnPropertyNames();
    } catch (ex) {
      
      
    }

    preview.ownPropertiesLength = names.length;

    for (let name of names) {
      let desc = objectActor._propertyDescriptor(name, true);
      if (!desc) {
        continue;
      }

      preview.ownProperties[name] = desc;
      if (++i == OBJECT_PREVIEW_MAX_ITEMS) {
        break;
      }
    }

    if (i < OBJECT_PREVIEW_MAX_ITEMS) {
      preview.safeGetterValues = objectActor._findSafeGetterValues(
                                    Object.keys(preview.ownProperties),
                                    OBJECT_PREVIEW_MAX_ITEMS - i);
    }

    return true;
  },
];













function getPromiseState(obj) {
  if (obj.class != "Promise") {
    throw new Error(
      "Can't call `getPromiseState` on `Debugger.Object`s that don't " +
      "refer to Promise objects.");
  }

  const state = PromiseDebugging.getState(obj.unsafeDereference());
  return {
    state: state.state,
    value: obj.makeDebuggeeValue(state.value),
    reason: obj.makeDebuggeeValue(state.reason)
  };
};









function isObject(value) {
  const type = typeof value;
  return type == "object" ? value !== null : type == "function";
}










function createBuiltinStringifier(ctor) {
  return obj => ctor.prototype.toString.call(obj.unsafeDereference());
}









function errorStringify(obj) {
  let name = DevToolsUtils.getProperty(obj, "name");
  if (name === "" || name === undefined) {
    name = obj.class;
  } else if (isObject(name)) {
    name = stringify(name);
  }

  let message = DevToolsUtils.getProperty(obj, "message");
  if (isObject(message)) {
    message = stringify(message);
  }

  if (message === "" || message === undefined) {
    return name;
  }
  return name + ": " + message;
}









function stringify(obj) {
  if (obj.class == "DeadObject") {
    const error = new Error("Dead object encountered.");
    DevToolsUtils.reportException("stringify", error);
    return "<dead object>";
  }

  const stringifier = stringifiers[obj.class] || stringifiers.Object;

  try {
    return stringifier(obj);
  } catch (e) {
    DevToolsUtils.reportException("stringify", e);
    return "<failed to stringify object>";
  }
}


let seen = null;

let stringifiers = {
  Error: errorStringify,
  EvalError: errorStringify,
  RangeError: errorStringify,
  ReferenceError: errorStringify,
  SyntaxError: errorStringify,
  TypeError: errorStringify,
  URIError: errorStringify,
  Boolean: createBuiltinStringifier(Boolean),
  Function: createBuiltinStringifier(Function),
  Number: createBuiltinStringifier(Number),
  RegExp: createBuiltinStringifier(RegExp),
  String: createBuiltinStringifier(String),
  Object: obj => "[object " + obj.class + "]",
  Array: obj => {
    
    
    const topLevel = !seen;
    if (topLevel) {
      seen = new Set();
    } else if (seen.has(obj)) {
      return "";
    }

    seen.add(obj);

    const len = DevToolsUtils.getProperty(obj, "length");
    let string = "";

    
    
    
    if (typeof len == "number" && len > 0) {
      for (let i = 0; i < len; i++) {
        const desc = obj.getOwnPropertyDescriptor(i);
        if (desc) {
          const { value } = desc;
          if (value != null) {
            string += isObject(value) ? stringify(value) : value;
          }
        }

        if (i < len - 1) {
          string += ",";
        }
      }
    }

    if (topLevel) {
      seen = null;
    }

    return string;
  },
  DOMException: obj => {
    const message = DevToolsUtils.getProperty(obj, "message") || "<no message>";
    const result = (+DevToolsUtils.getProperty(obj, "result")).toString(16);
    const code = DevToolsUtils.getProperty(obj, "code");
    const name = DevToolsUtils.getProperty(obj, "name") || "<unknown>";

    return '[Exception... "' + message + '" ' +
           'code: "' + code +'" ' +
           'nsresult: "0x' + result + ' (' + name + ')"]';
  },
  Promise: obj => {
    const { state, value, reason } = getPromiseState(obj);
    let statePreview = state;
    if (state != "pending") {
      const settledValue = state === "fulfilled" ? value : reason;
      statePreview += ": " + (typeof settledValue === "object" && settledValue !== null
                                ? stringify(settledValue)
                                : settledValue);
    }
    return "Promise (" + statePreview + ")";
  },
};














function makeDebuggeeValueIfNeeded(obj, value) {
  if (value && (typeof value == "object" || typeof value == "function")) {
    return obj.makeDebuggeeValue(value);
  }
  return value;
}








function LongStringActor(string) {
  this.string = string;
  this.stringLength = string.length;
}

LongStringActor.prototype = {
  actorPrefix: "longString",

  disconnect: function() {
    
    
    
    this._releaseActor();
  },

  


  grip: function() {
    return {
      "type": "longString",
      "initial": this.string.substring(
        0, DebuggerServer.LONG_STRING_INITIAL_LENGTH),
      "length": this.stringLength,
      "actor": this.actorID
    };
  },

  





  onSubstring: function(request) {
    return {
      "from": this.actorID,
      "substring": this.string.substring(request.start, request.end)
    };
  },

  


  onRelease: function () {
    
    
    
    this._releaseActor();
    this.registeredPool.removeActor(this);
    return {};
  },

  _releaseActor: function() {
    if (this.registeredPool && this.registeredPool.longStringActors) {
      delete this.registeredPool.longStringActors[this.string];
    }
  }
};

LongStringActor.prototype.requestTypes = {
  "substring": LongStringActor.prototype.onSubstring,
  "release": LongStringActor.prototype.onRelease
};





function createValueGrip(value, pool, makeObjectGrip) {
  switch (typeof value) {
    case "boolean":
      return value;

    case "string":
      if (stringIsLong(value)) {
        return longStringGrip(value, pool);
      }
      return value;

    case "number":
      if (value === Infinity) {
        return { type: "Infinity" };
      } else if (value === -Infinity) {
        return { type: "-Infinity" };
      } else if (Number.isNaN(value)) {
        return { type: "NaN" };
      } else if (!value && 1 / value === -Infinity) {
        return { type: "-0" };
      }
      return value;

    case "undefined":
      return { type: "undefined" };

    case "object":
      if (value === null) {
        return { type: "null" };
      }
    else if(value.optimizedOut ||
            value.uninitialized ||
            value.missingArguments) {
        
        
        return {
          type: "null",
          optimizedOut: value.optimizedOut,
          uninitialized: value.uninitialized,
          missingArguments: value.missingArguments
        };
      }
      return makeObjectGrip(value, pool);

    case "symbol":
      let form = {
        type: "symbol"
      };
      let name = getSymbolName(value);
      if (name !== undefined) {
        form.name = createValueGrip(name, pool, makeObjectGrip);
      }
      return form;

    default:
      dbg_assert(false, "Failed to provide a grip for: " + value);
      return null;
  }
}

const symbolProtoToString = Symbol.prototype.toString;

function getSymbolName(symbol) {
  const name = symbolProtoToString.call(symbol).slice("Symbol(".length, -1);
  return name || undefined;
}








function stringIsLong(str) {
  return str.length >= DebuggerServer.LONG_STRING_LENGTH;
}









function longStringGrip(str, pool) {
  if (!pool.longStringActors) {
    pool.longStringActors = {};
  }

  if (pool.longStringActors.hasOwnProperty(str)) {
    return pool.longStringActors[str].grip();
  }

  let actor = new LongStringActor(str);
  pool.addActor(actor);
  pool.longStringActors[str] = actor;
  return actor.grip();
}

exports.ObjectActor = ObjectActor;
exports.PropertyIteratorActor = PropertyIteratorActor;
exports.LongStringActor = LongStringActor;
exports.createValueGrip = createValueGrip;
exports.stringIsLong = stringIsLong;
exports.longStringGrip = longStringGrip;
