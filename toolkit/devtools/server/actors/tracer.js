



"use strict";

const { Cu } = require("chrome");

const { reportException } =
  Cu.import("resource://gre/modules/devtools/DevToolsUtils.jsm", {}).DevToolsUtils;

const { DebuggerServer } = Cu.import("resource://gre/modules/devtools/dbg-server.jsm", {});

Cu.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);





function TraceActor(aConn, aParentActor)
{
  this._attached = false;
  this._activeTraces = new MapStack();
  this._totalTraces = 0;
  this._startTime = 0;
  this._requestsForTraceType = Object.create(null);
  for (let type of TraceTypes.types) {
    this._requestsForTraceType[type] = 0;
  }
  this._sequence = 0;

  this.global = aParentActor.window.wrappedJSObject;
}

TraceActor.prototype = {
  actorPrefix: "trace",

  get attached() { return this._attached; },
  get idle()     { return this._attached && this._activeTraces.size === 0; },
  get tracing()  { return this._attached && this._activeTraces.size > 0; },

  












  _handleEvent: function(aEvent, aPacket, aArgs) {
    let handlersForEvent = TraceTypes.handlers[aEvent];
    for (let traceType in handlersForEvent) {
      if (this._requestsForTraceType[traceType]) {
        aPacket[traceType] = handlersForEvent[traceType].call(null, aArgs);
      }
    }
  },

  


  _initDebugger: function() {
    this.dbg = new Debugger();
    this.dbg.onEnterFrame = this.onEnterFrame.bind(this);
    this.dbg.onNewGlobalObject = this.globalManager.onNewGlobal.bind(this);
    this.dbg.enabled = false;
  },

  


  _addDebuggee: function(aGlobal) {
    try {
      this.dbg.addDebuggee(aGlobal);
    } catch (e) {
      
      reportException("TraceActor",
                      new Error("Ignoring request to add the debugger's "
                                + "compartment as a debuggee"));
    }
  },

  


  _addDebuggees: function(aWindow) {
    this._addDebuggee(aWindow);
    let frames = aWindow.frames;
    if (frames) {
      for (let i = 0; i < frames.length; i++) {
        this._addDebuggees(frames[i]);
      }
    }
  },

  



  globalManager: {
    


    findGlobals: function() {
      this._addDebuggees(this.global);
    },

    







    onNewGlobal: function(aGlobal) {
      
      
      if (aGlobal.hostAnnotations &&
          aGlobal.hostAnnotations.type == "document" &&
          aGlobal.hostAnnotations.element === this.global) {
        this._addDebuggee(aGlobal);
      }
    },
  },

  





  onAttach: function(aRequest) {
    if (this.attached) {
      return {
        error: "wrongState",
        message: "Already attached to a client"
      };
    }

    if (!this.dbg) {
      this._initDebugger();
      this.globalManager.findGlobals.call(this);
    }

    this._attached = true;

    return { type: "attached", traceTypes: TraceTypes.types };
  },

  





  onDetach: function() {
    while (this.tracing) {
      this.onStopTrace();
    }

    this.dbg = null;

    this._attached = false;
    this.conn.send({ from: this.actorID, type: "detached" });
  },

  





  onStartTrace: function(aRequest) {
    for (let traceType of aRequest.trace) {
      if (TraceTypes.types.indexOf(traceType) < 0) {
        return {
          error: "badParameterType",
          message: "No such trace type: " + traceType
        };
      }
    }

    if (this.idle) {
      this.dbg.enabled = true;
      this._sequence = 0;
      this._startTime = +new Date;
    }

    
    for (let traceType of aRequest.trace) {
      this._requestsForTraceType[traceType]++;
    }

    this._totalTraces++;
    let name = aRequest.name || "Trace " + this._totalTraces;
    this._activeTraces.push(name, aRequest.trace);

    return { type: "startedTrace", why: "requested", name: name };
  },

  





  onStopTrace: function(aRequest) {
    if (!this.tracing) {
      return {
        error: "wrongState",
        message: "No active traces"
      };
    }

    let stoppedTraceTypes, name;
    if (aRequest && aRequest.name) {
      name = aRequest.name;
      if (!this._activeTraces.has(name)) {
        return {
          error: "noSuchTrace",
          message: "No active trace with name: " + name
        };
      }
      stoppedTraceTypes = this._activeTraces.delete(name);
    } else {
      name = this._activeTraces.peekKey();
      stoppedTraceTypes = this._activeTraces.pop();
    }

    for (let traceType of stoppedTraceTypes) {
      this._requestsForTraceType[traceType]--;
    }

    if (this.idle) {
      this.dbg.enabled = false;
    }

    return { type: "stoppedTrace", why: "requested", name: name };
  },

  

  






  onEnterFrame: function(aFrame) {
    let callee = aFrame.callee;
    let packet = {
      from: this.actorID,
      type: "enteredFrame",
      sequence: this._sequence++
    };

    this._handleEvent(TraceTypes.Events.enterFrame, packet, {
      frame: aFrame,
      startTime: this._startTime
    });

    aFrame.onPop = this.onExitFrame.bind(this);

    this.conn.send(packet);
  },

  






  onExitFrame: function(aValue) {
    let packet = {
      from: this.actorID,
      type: "exitedFrame",
      sequence: this._sequence++
    };

    this._handleEvent(TraceTypes.Events.exitFrame, packet, { value: aValue });

    this.conn.send(packet);
  }
};




TraceActor.prototype.requestTypes = {
  "attach": TraceActor.prototype.onAttach,
  "detach": TraceActor.prototype.onDetach,
  "startTrace": TraceActor.prototype.onStartTrace,
  "stopTrace": TraceActor.prototype.onStopTrace
};

exports.register = function(handle) {
  handle.addTabActor(TraceActor, "traceActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(TraceActor, "traceActor");
};









function MapStack()
{
  
  
  this._stack = [];
  this._map = Object.create(null);
}

MapStack.prototype = {
  get size() { return this._stack.length; },

  



  peekKey: function() {
    return this._stack[this.size - 1];
  },

  





  has: function(aKey) {
    return Object.prototype.hasOwnProperty.call(this._map, aKey);
  },

  






  get: function(aKey) {
    return this._map[aKey];
  },

  










  push: function(aKey, aValue) {
    this.delete(aKey);
    this._stack.push(aKey);
    this._map[aKey] = aValue;
  },

  



  pop: function() {
    let key = this.peekKey();
    let value = this.get(key);
    this._stack.pop();
    delete this._map[key];
    return value;
  },

  







  delete: function(aKey) {
    let value = this.get(aKey);
    if (this.has(aKey)) {
      let keyIndex = this._stack.lastIndexOf(aKey);
      this._stack.splice(keyIndex, 1);
      delete this._map[aKey];
    }
    return value;
  }
};








let TraceTypes = {
  handlers: {},
  types: [],

  register: function(aType, aEvent, aHandler) {
    if (!this.handlers[aEvent]) {
      this.handlers[aEvent] = {};
    }
    this.handlers[aEvent][aType] = aHandler;
    if (this.types.indexOf(aType) < 0) {
      this.types.push(aType);
    }
  }
};

TraceTypes.Events = {
  "enterFrame": "enterFrame",
  "exitFrame": "exitFrame"
};

TraceTypes.register("name", TraceTypes.Events.enterFrame, function({ frame }) {
  return frame.callee
    ? frame.callee.displayName || "(anonymous function)"
    : "(" + frame.type + ")";
});

TraceTypes.register("callsite", TraceTypes.Events.enterFrame, function({ frame }) {
  if (!frame.script) {
    return undefined;
  }
  return {
    url: frame.script.url,
    line: frame.script.getOffsetLine(frame.offset),
    column: getOffsetColumn(frame.offset, frame.script)
  };
});

TraceTypes.register("time", TraceTypes.Events.enterFrame, timeSinceTraceStarted);
TraceTypes.register("time", TraceTypes.Events.exitFrame, timeSinceTraceStarted);

TraceTypes.register("parameterNames", TraceTypes.Events.enterFrame, function({ frame }) {
  return frame.callee ? frame.callee.parameterNames : undefined;
});

TraceTypes.register("arguments", TraceTypes.Events.enterFrame, function({ frame }) {
  if (!frame.arguments) {
    return undefined;
  }
  let objectPool = [];
  let objToId = new Map();
  let args = Array.prototype.slice.call(frame.arguments);
  let values = args.map(arg => createValueGrip(arg, objectPool, objToId));
  return { values: values, objectPool: objectPool };
});

TraceTypes.register("return", TraceTypes.Events.exitFrame,
                    serializeCompletionValue.bind(null, "return"));

TraceTypes.register("throw", TraceTypes.Events.exitFrame,
                    serializeCompletionValue.bind(null, "throw"));

TraceTypes.register("yield", TraceTypes.Events.exitFrame,
                    serializeCompletionValue.bind(null, "yield"));




function getOffsetColumn(aOffset, aScript) {
  let bestOffsetMapping = null;
  for (let offsetMapping of aScript.getAllColumnOffsets()) {
    if (!bestOffsetMapping ||
        (offsetMapping.offset <= aOffset &&
         offsetMapping.offset > bestOffsetMapping.offset)) {
      bestOffsetMapping = offsetMapping;
    }
  }

  if (!bestOffsetMapping) {
    
    
    
    
    reportException("TraceActor",
                    new Error("Could not find a column for offset " + aOffset +
                              " in the script " + aScript));
    return 0;
  }

  return bestOffsetMapping.columnNumber;
}




function timeSinceTraceStarted({ startTime }) {
  return +new Date - startTime;
}








function serializeCompletionValue(aType, { value }) {
  if (typeof value[aType] === "undefined") {
    return undefined;
  }
  let objectPool = [];
  let objToId = new Map();
  let valueGrip = createValueGrip(value[aType], objectPool, objToId);
  return { value: valueGrip, objectPool: objectPool };
}





















function createValueGrip(aValue, aPool, aObjectToId) {
  let type = typeof aValue;

  if (type === "string" && aValue.length >= DebuggerServer.LONG_STRING_LENGTH) {
    return {
      type: "longString",
      initial: aValue.substring(0, DebuggerServer.LONG_STRING_INITIAL_LENGTH),
      length: aValue.length
    };
  }

  if (type === "boolean" || type === "string" || type === "number") {
    return aValue;
  }

  if (aValue === null) {
    return { type: "null" };
  }

  if (aValue === undefined) {
    return { type: "undefined" };
  }

  if (typeof(aValue) === "object") {
    createObjectDescriptor(aValue, aPool, aObjectToId);
    return { type: "object", objectId: aObjectToId.get(aValue) };
  }

  reportException("TraceActor",
                  new Error("Failed to provide a grip for: " + aValue));
  return null;
}














function createObjectDescriptor(aObject, aPool, aObjectToId) {
  if (aObjectToId.has(aObject)) {
    return;
  }

  aObjectToId.set(aObject, aPool.length);
  let desc = Object.create(null);
  aPool.push(desc);

  
  desc.class = aObject.class;
  desc.extensible = aObject.isExtensible();
  desc.frozen = aObject.isFrozen();
  desc.sealed = aObject.isSealed();

  
  if (aObject.class === "Function") {
    if (aObject.name) {
      desc.name = aObject.name;
    }
    if (aObject.displayName) {
      desc.displayName = aObject.displayName;
    }

    
    
    let name = aObject.getOwnPropertyDescriptor("displayName");
    if (name && name.value && typeof name.value == "string") {
      desc.userDisplayName = createValueGrip(name.value, aObject, aPool, aObjectToId);
    }
  }

  let ownProperties = Object.create(null);
  let propNames;
  try {
    propNames = aObject.getOwnPropertyNames();
  } catch(ex) {
    
    
    desc.prototype = createValueGrip(null);
    desc.ownProperties = ownProperties;
    desc.safeGetterValues = Object.create(null);
    return;
  }

  for (let name of propNames) {
    ownProperties[name] = createPropertyDescriptor(name, aObject, aPool, aObjectToId);
  }

  desc.prototype = createValueGrip(aObject.proto, aPool, aObjectToId);
  desc.ownProperties = ownProperties;
  desc.safeGetterValues = findSafeGetterValues(ownProperties, aObject, aPool, aObjectToId);
}




















function createPropertyDescriptor(aName, aObject, aPool, aObjectToId) {
  let desc;
  try {
    desc = aObject.getOwnPropertyDescriptor(aName);
  } catch (e) {
    
    
    
    return {
      configurable: false,
      writable: false,
      enumerable: false,
      value: e.name
    };
  }

  let retval = {
    configurable: desc.configurable,
    enumerable: desc.enumerable
  };

  if ("value" in desc) {
    retval.writable = desc.writable;
    retval.value = createValueGrip(desc.value, aPool, aObjectToId);
  } else {
    if ("get" in desc) {
      retval.get = createValueGrip(desc.get, aPool, aObjectToId);
    }
    if ("set" in desc) {
      retval.set = createValueGrip(desc.set, aPool, aObjectToId);
    }
  }
  return retval;
}



















function findSafeGetterValues(aOwnProperties, aObject, aPool, aObjectToId) {
  let safeGetterValues = Object.create(null);
  let obj = aObject;
  let level = 0;

  while (obj) {
    let getters = findSafeGetters(obj);
    for (let name of getters) {
      
      
      
      if (name in safeGetterValues ||
          (obj != aObject && name in aOwnProperties)) {
        continue;
      }

      let desc = null, getter = null;
      try {
        desc = obj.getOwnPropertyDescriptor(name);
        getter = desc.get;
      } catch (ex) {
        
      }
      if (!getter) {
        continue;
      }

      let result = getter.call(aObject);
      if (result && !("throw" in result)) {
        let getterValue = undefined;
        if ("return" in result) {
          getterValue = result.return;
        } else if ("yield" in result) {
          getterValue = result.yield;
        }
        
        
        if (getterValue !== undefined) {
          safeGetterValues[name] = {
            getterValue: createValueGrip(getterValue, aPool, aObjectToId),
            getterPrototypeLevel: level,
            enumerable: desc.enumerable,
            writable: level == 0 ? desc.writable : true,
          };
        }
      }
    }

    obj = obj.proto;
    level++;
  }

  return safeGetterValues;
}











function findSafeGetters(aObject) {
  let getters = new Set();
  for (let name of aObject.getOwnPropertyNames()) {
    let desc = null;
    try {
      desc = aObject.getOwnPropertyDescriptor(name);
    } catch (e) {
      
      
    }
    if (!desc || desc.value !== undefined || !("get" in desc)) {
      continue;
    }

    let fn = desc.get;
    if (fn && fn.callable && fn.class == "Function" &&
        fn.script === undefined) {
      getters.add(name);
    }
  }

  return getters;
}
