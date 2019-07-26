


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const events = require("sdk/event/core");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const protocol = require("devtools/server/protocol");
const {ContentObserver} = require("devtools/content-observer");

const {on, once, off, emit} = events;
const {method, Arg, Option, RetVal} = protocol;

exports.register = function(handle) {
  handle.addTabActor(CallWatcherActor, "callWatcherActor");
};

exports.unregister = function(handle) {
  handle.removeTabActor(CallWatcherActor);
};




protocol.types.addDictType("call-stack-item", {
  name: "string",
  file: "string",
  line: "number"
});




protocol.types.addDictType("call-details", {
  type: "number",
  name: "string",
  stack: "array:call-stack-item"
});





let FunctionCallActor = protocol.ActorClass({
  typeName: "function-call",

  























  initialize: function(conn, [window, global, caller, type, name, stack, args, result]) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this.details = {
      type: type,
      name: name,
      stack: stack,
    };

    
    
    let weakRefs = {
      window: Cu.getWeakReference(window),
      caller: Cu.getWeakReference(caller),
      result: Cu.getWeakReference(result),
      args: Cu.getWeakReference(args)
    };

    Object.defineProperties(this.details, {
      window: { get: () => weakRefs.window.get() },
      caller: { get: () => weakRefs.caller.get() },
      result: { get: () => weakRefs.result.get() },
      args: { get: () => weakRefs.args.get() }
    });

    this.meta = {
      global: -1,
      previews: { caller: "", args: "" }
    };

    if (global == "WebGLRenderingContext") {
      this.meta.global = CallWatcherFront.CANVAS_WEBGL_CONTEXT;
    } else if (global == "CanvasRenderingContext2D") {
      this.meta.global = CallWatcherFront.CANVAS_2D_CONTEXT;
    } else if (global == "window") {
      this.meta.global = CallWatcherFront.UNKNOWN_SCOPE;
    } else {
      this.meta.global = CallWatcherFront.GLOBAL_SCOPE;
    }

    this.meta.previews.caller = this._generateCallerPreview();
    this.meta.previews.args = this._generateArgsPreview();
  },

  



  form: function() {
    return {
      actor: this.actorID,
      type: this.details.type,
      name: this.details.name,
      file: this.details.stack[0].file,
      line: this.details.stack[0].line,
      callerPreview: this.meta.previews.caller,
      argsPreview: this.meta.previews.args
    };
  },

  



  getDetails: method(function() {
    let { type, name, stack } = this.details;

    
    
    
    for (let i = stack.length - 1;;) {
      if (stack[i].file) {
        break;
      }
      stack.pop();
      i--;
    }

    
    
    return {
      type: type,
      name: name,
      stack: stack
    };
  }, {
    response: { info: RetVal("call-details") }
  }),

  






  _generateCallerPreview: function() {
    let global = this.meta.global;
    if (global == CallWatcherFront.CANVAS_WEBGL_CONTEXT) {
      return "gl";
    }
    if (global == CallWatcherFront.CANVAS_2D_CONTEXT) {
      return "ctx";
    }
    return "";
  },

  






  _generateArgsPreview: function() {
    let { caller, args } = this.details;
    let { global } = this.meta;

    
    
    let serializeArgs = () => args.map(arg => {
      if (typeof arg == "undefined") {
        return "undefined";
      }
      if (typeof arg == "function") {
        return "Function";
      }
      if (typeof arg == "object") {
        return "Object";
      }
      if (global == CallWatcherFront.CANVAS_WEBGL_CONTEXT) {
        
        return getEnumsLookupTable("webgl", caller)[arg] || arg;
      }
      if (global == CallWatcherFront.CANVAS_2D_CONTEXT) {
        return getEnumsLookupTable("2d", caller)[arg] || arg;
      }
      return arg;
    });

    return serializeArgs().join(", ");
  }
});




let FunctionCallFront = protocol.FrontClass(FunctionCallActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
  },

  



  form: function(form) {
    this.actorID = form.actor;
    this.type = form.type;
    this.name = form.name;
    this.file = form.file;
    this.line = form.line;
    this.callerPreview = form.callerPreview;
    this.argsPreview = form.argsPreview;
  }
});




let CallWatcherActor = exports.CallWatcherActor = protocol.ActorClass({
  typeName: "call-watcher",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._onGlobalCreated = this._onGlobalCreated.bind(this);
    this._onGlobalDestroyed = this._onGlobalDestroyed.bind(this);
    this._onContentFunctionCall = this._onContentFunctionCall.bind(this);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  




  setup: method(function({ tracedGlobals, tracedFunctions, startRecording, performReload }) {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    this._functionCalls = [];
    this._tracedGlobals = tracedGlobals || [];
    this._tracedFunctions = tracedFunctions || [];
    this._contentObserver = new ContentObserver(this.tabActor);

    on(this._contentObserver, "global-created", this._onGlobalCreated);
    on(this._contentObserver, "global-destroyed", this._onGlobalDestroyed);

    if (startRecording) {
      this.resumeRecording();
    }
    if (performReload) {
      this.tabActor.window.location.reload();
    }
  }, {
    request: {
      tracedGlobals: Option(0, "nullable:array:string"),
      tracedFunctions: Option(0, "nullable:array:string"),
      startRecording: Option(0, "boolean"),
      performReload: Option(0, "boolean")
    },
    oneway: true
  }),

  




  finalize: method(function() {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;

    this._contentObserver.stopListening();
    off(this._contentObserver, "global-created", this._onGlobalCreated);
    off(this._contentObserver, "global-destroyed", this._onGlobalDestroyed);

    this._tracedGlobals = null;
    this._tracedFunctions = null;
    this._contentObserver = null;
  }, {
    oneway: true
  }),

  


  isRecording: method(function() {
    return this._recording;
  }, {
    response: RetVal("boolean")
  }),

  


  resumeRecording: method(function() {
    this._recording = true;
  }),

  


  pauseRecording: method(function() {
    this._recording = false;
    return this._functionCalls;
  }, {
    response: { calls: RetVal("array:function-call") }
  }),

  



  eraseRecording: method(function() {
    this._functionCalls = [];
  }),

  




  onCall: function() {},

  


  _onGlobalCreated: function(window) {
    let self = this;

    this._tracedWindowId = ContentObserver.GetInnerWindowID(window);
    let unwrappedWindow = XPCNativeWrapper.unwrap(window);
    let callback = this._onContentFunctionCall;

    for (let global of this._tracedGlobals) {
      let prototype = unwrappedWindow[global].prototype;
      let properties = Object.keys(prototype);
      properties.forEach(name => overrideSymbol(global, prototype, name, callback));
    }

    for (let name of this._tracedFunctions) {
      overrideSymbol("window", unwrappedWindow, name, callback);
    }

    



    function overrideSymbol(global, target, name, callback) {
      let propertyDescriptor = Object.getOwnPropertyDescriptor(target, name);

      if (propertyDescriptor.get || propertyDescriptor.set) {
        overrideAccessor(global, target, name, propertyDescriptor, callback);
        return;
      }
      if (propertyDescriptor.writable && typeof propertyDescriptor.value == "function") {
        overrideFunction(global, target, name, propertyDescriptor, callback);
        return;
      }
    }

    


    function overrideFunction(global, target, name, descriptor, callback) {
      let originalFunc = target[name];

      Object.defineProperty(target, name, {
        value: function(...args) {
          let result = originalFunc.apply(this, args);

          if (self._recording) {
            let stack = getStack(name);
            let type = CallWatcherFront.METHOD_FUNCTION;
            callback(unwrappedWindow, global, this, type, name, stack, args, result);
          }
          return result;
        },
        configurable: descriptor.configurable,
        enumerable: descriptor.enumerable,
        writable: true
      });
    }

    


    function overrideAccessor(global, target, name, descriptor, callback) {
      let originalGetter = target.__lookupGetter__(name);
      let originalSetter = target.__lookupSetter__(name);

      Object.defineProperty(target, name, {
        get: function(...args) {
          if (!originalGetter) return undefined;
          let result = originalGetter.apply(this, args);

          if (self._recording) {
            let stack = getStack(name);
            let type = CallWatcherFront.GETTER_FUNCTION;
            callback(unwrappedWindow, global, this, type, name, stack, args, result);
          }
          return result;
        },
        set: function(...args) {
          if (!originalSetter) return;
          originalSetter.apply(this, args);

          if (self._recording) {
            let stack = getStack(name);
            let type = CallWatcherFront.SETTER_FUNCTION;
            callback(unwrappedWindow, global, this, type, name, stack, args, undefined);
          }
        },
        configurable: descriptor.configurable,
        enumerable: descriptor.enumerable
      });
    }

    



    function getStack(caller) {
      try {
        
        
        throw new Error();
      } catch (e) {
        var stack = e.stack;
      }

      
      
      
      let calls = [];
      let callIndex = 0;
      let currNewLinePivot = stack.indexOf("\n") + 1;
      let nextNewLinePivot = stack.indexOf("\n", currNewLinePivot);

      while (nextNewLinePivot > 0) {
        let nameDelimiterIndex = stack.indexOf("@", currNewLinePivot);
        let columnDelimiterIndex = stack.lastIndexOf(":", nextNewLinePivot - 1);
        let lineDelimiterIndex = stack.lastIndexOf(":", columnDelimiterIndex - 1);

        if (!calls[callIndex]) {
          calls[callIndex] = { name: "", file: "", line: 0 };
        }
        if (!calls[callIndex + 1]) {
          calls[callIndex + 1] = { name: "", file: "", line: 0 };
        }

        if (callIndex > 0) {
          let file = stack.substring(nameDelimiterIndex + 1, lineDelimiterIndex);
          let line = stack.substring(lineDelimiterIndex + 1, columnDelimiterIndex);
          let name = stack.substring(currNewLinePivot, nameDelimiterIndex);
          calls[callIndex].name = name;
          calls[callIndex - 1].file = file;
          calls[callIndex - 1].line = line;
        } else {
          
          
          calls[0].name = caller;
        }

        currNewLinePivot = nextNewLinePivot + 1;
        nextNewLinePivot = stack.indexOf("\n", currNewLinePivot);
        callIndex++;
      }

      return calls;
    }
  },

  


  _onGlobalDestroyed: function(id) {
    if (this._tracedWindowId == id) {
      this.pauseRecording();
      this.eraseRecording();
    }
  },

  


  _onContentFunctionCall: function(...details) {
    let functionCall = new FunctionCallActor(this.conn, details);
    this._functionCalls.push(functionCall);
    this.onCall(functionCall);
  }
});




let CallWatcherFront = exports.CallWatcherFront = protocol.FrontClass(CallWatcherActor, {
  initialize: function(client, { callWatcherActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: callWatcherActor });
    client.addActorPool(this);
    this.manage(this);
  }
});




CallWatcherFront.METHOD_FUNCTION = 0;
CallWatcherFront.GETTER_FUNCTION = 1;
CallWatcherFront.SETTER_FUNCTION = 2;

CallWatcherFront.GLOBAL_SCOPE = 0;
CallWatcherFront.UNKNOWN_SCOPE = 1;
CallWatcherFront.CANVAS_WEBGL_CONTEXT = 2;
CallWatcherFront.CANVAS_2D_CONTEXT = 3;








var gEnumRegex = /^[A-Z_]+$/;
var gEnumsLookupTable = {};

function getEnumsLookupTable(type, object) {
  let cachedEnum = gEnumsLookupTable[type];
  if (cachedEnum) {
    return cachedEnum;
  }

  let table = gEnumsLookupTable[type] = {};

  for (let key in object) {
    if (key.match(gEnumRegex)) {
      table[object[key]] = key;
    }
  }

  return table;
}
