


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");
const events = require("sdk/event/core");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
const protocol = require("devtools/server/protocol");
const {serializeStack, parseStack} = require("toolkit/loader");

const {on, once, off, emit} = events;
const {method, Arg, Option, RetVal} = protocol;




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

  


























  initialize: function(conn, [window, global, caller, type, name, stack, args, result], holdWeak) {
    protocol.Actor.prototype.initialize.call(this, conn);

    this.details = {
      type: type,
      name: name,
      stack: stack,
    };

    
    
    
    if (holdWeak) {
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
    }
    
    else {
      this.details.window = window;
      this.details.caller = caller;
      this.details.result = result;
      this.details.args = args;
    }

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
    let { caller, args, name } = this.details;
    let { global } = this.meta;

    
    
    let enumArgs = (CallWatcherFront.ENUM_METHODS[global] || {})[name];
    if (typeof enumArgs === "function") {
      enumArgs = enumArgs(args);
    }

    
    
    let serializeArgs = () => args.map((arg, i) => {
      if (arg === undefined) {
        return "undefined";
      }
      if (arg === null) {
        return "null";
      }
      if (typeof arg == "function") {
        return "Function";
      }
      if (typeof arg == "object") {
        return "Object";
      }
      
      
      if (enumArgs && enumArgs.indexOf(i) !== -1) {
        return getBitToEnumValue(global, caller, arg);
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

  




  setup: method(function({ tracedGlobals, tracedFunctions, startRecording, performReload, holdWeak, storeCalls }) {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    this._functionCalls = [];
    this._tracedGlobals = tracedGlobals || [];
    this._tracedFunctions = tracedFunctions || [];
    this._holdWeak = !!holdWeak;
    this._storeCalls = !!storeCalls;

    on(this.tabActor, "window-ready", this._onGlobalCreated);
    on(this.tabActor, "window-destroyed", this._onGlobalDestroyed);

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
      performReload: Option(0, "boolean"),
      holdWeak: Option(0, "boolean"),
      storeCalls: Option(0, "boolean")
    },
    oneway: true
  }),

  




  finalize: method(function() {
    if (!this._initialized) {
      return;
    }
    this._initialized = false;
    this._finalized = true;

    off(this.tabActor, "window-ready", this._onGlobalCreated);
    off(this.tabActor, "window-destroyed", this._onGlobalDestroyed);

    this._tracedGlobals = null;
    this._tracedFunctions = null;
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

  


  _onGlobalCreated: function({window, id, isTopLevel}) {
    let self = this;

    
    if (!isTopLevel) {
      return;
    }
    this._tracedWindowId = id;

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
      
      
      let originalFunc = Cu.unwaiveXrays(target[name]);

      Cu.exportFunction(function(...args) {
        let result;
        try {
          result = Cu.waiveXrays(originalFunc.apply(this, args));
        } catch (e) {
          throw createContentError(e, unwrappedWindow);
        }

        if (self._recording) {
          let stack = getStack(name);
          let type = CallWatcherFront.METHOD_FUNCTION;
          callback(unwrappedWindow, global, this, type, name, stack, args, result);
        }
        return result;
      }, target, { defineAs: name });

      Object.defineProperty(target, name, {
        configurable: descriptor.configurable,
        enumerable: descriptor.enumerable,
        writable: true
      });
    }

    


    function overrideAccessor(global, target, name, descriptor, callback) {
      
      
      let originalGetter = Cu.unwaiveXrays(target.__lookupGetter__(name));
      let originalSetter = Cu.unwaiveXrays(target.__lookupSetter__(name));

      Object.defineProperty(target, name, {
        get: function(...args) {
          if (!originalGetter) return undefined;
          let result = Cu.waiveXrays(originalGetter.apply(this, args));

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

  


  _onGlobalDestroyed: function({window, id, isTopLevel}) {
    if (this._tracedWindowId == id) {
      this.pauseRecording();
      this.eraseRecording();
    }
  },

  


  _onContentFunctionCall: function(...details) {
    
    
    if (this._finalized) {
      return;
    }
    let functionCall = new FunctionCallActor(this.conn, details, this._holdWeak);

    if (this._storeCalls) {
      this._functionCalls.push(functionCall);
    }

    this.onCall(functionCall);
  }
});




let CallWatcherFront = exports.CallWatcherFront = protocol.FrontClass(CallWatcherActor, {
  initialize: function(client, { callWatcherActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: callWatcherActor });
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

CallWatcherFront.ENUM_METHODS = {};
CallWatcherFront.ENUM_METHODS[CallWatcherFront.CANVAS_2D_CONTEXT] = {
  asyncDrawXULElement: [6],
  drawWindow: [6]
};

CallWatcherFront.ENUM_METHODS[CallWatcherFront.CANVAS_WEBGL_CONTEXT] = {
  activeTexture: [0],
  bindBuffer: [0],
  bindFramebuffer: [0],
  bindRenderbuffer: [0],
  bindTexture: [0],
  blendEquation: [0],
  blendEquationSeparate: [0, 1],
  blendFunc: [0, 1],
  blendFuncSeparate: [0, 1, 2, 3],
  bufferData: [0, 1, 2],
  bufferSubData: [0, 1],
  checkFramebufferStatus: [0],
  clear: [0],
  compressedTexImage2D: [0, 2],
  compressedTexSubImage2D: [0, 6],
  copyTexImage2D: [0, 2],
  copyTexSubImage2D: [0],
  createShader: [0],
  cullFace: [0],
  depthFunc: [0],
  disable: [0],
  drawArrays: [0],
  drawElements: [0, 2],
  enable: [0],
  framebufferRenderbuffer: [0, 1, 2],
  framebufferTexture2D: [0, 1, 2],
  frontFace: [0],
  generateMipmap: [0],
  getBufferParameter: [0, 1],
  getParameter: [0],
  getFramebufferAttachmentParameter: [0, 1, 2],
  getProgramParameter: [1],
  getRenderbufferParameter: [0, 1],
  getShaderParameter: [1],
  getShaderPrecisionFormat: [0, 1],
  getTexParameter: [0, 1],
  getVertexAttrib: [1],
  getVertexAttribOffset: [1],
  hint: [0, 1],
  isEnabled: [0],
  pixelStorei: [0],
  readPixels: [4, 5],
  renderbufferStorage: [0, 1],
  stencilFunc: [0],
  stencilFuncSeparate: [0, 1],
  stencilMaskSeparate: [0],
  stencilOp: [0, 1, 2],
  stencilOpSeparate: [0, 1, 2, 3],
  texImage2D: (args) => args.length > 6 ? [0, 2, 6, 7] : [0, 2, 3, 4],
  texParameterf: [0, 1],
  texParameteri: [0, 1, 2],
  texSubImage2D: (args) => args.length === 9 ? [0, 6, 7] : [0, 4, 5],
  vertexAttribPointer: [2]
};








var gEnumRegex = /^[A-Z][A-Z0-9_]+$/;
var gEnumsLookupTable = {};



var INVALID_ENUMS = [
  "INVALID_ENUM", "NO_ERROR", "INVALID_VALUE", "OUT_OF_MEMORY", "NONE"
];

function getBitToEnumValue(type, object, arg) {
  let table = gEnumsLookupTable[type];

  
  if (!table) {
    table = gEnumsLookupTable[type] = {};

    for (let key in object) {
      if (key.match(gEnumRegex)) {
        
        table[object[key]] = key;
      }
    }
  }

  
  if (table[arg]) {
    return table[arg];
  }

  
  
  let flags = [];
  for (let flag in table) {
    if (INVALID_ENUMS.indexOf(table[flag]) !== -1) {
      continue;
    }

    
    
    flag = flag | 0;
    if (flag && (arg & flag) === flag) {
      flags.push(table[flag]);
    }
  }

  
  return table[arg] = flags.join(" | ") || arg;
}











function createContentError (e, win) {
  let { message, name, stack } = e;
  let parsedStack = parseStack(stack);
  let { fileName, lineNumber, columnNumber } = parsedStack[parsedStack.length - 1];
  let error;

  let isDOMException = e instanceof Ci.nsIDOMDOMException;
  let constructor = isDOMException ? win.DOMException : (win[e.name] || win.Error);

  if (isDOMException) {
    error = new constructor(message, name);
    Object.defineProperties(error, {
      code: { value: e.code },
      columnNumber: { value: 0 }, 
      filename: { value: fileName }, 
      lineNumber: { value: lineNumber },
      result: { value: e.result },
      stack: { value: serializeStack(parsedStack) }
    });
  }
  else {
    
    
    
    error = new constructor(message, fileName, lineNumber);
    Object.defineProperty(error, "columnNumber", {
      value: columnNumber
    });
  }
  return error;
}
