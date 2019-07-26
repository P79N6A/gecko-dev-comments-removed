


"use strict";

const {Cc, Ci, Cu, Cr} = require("chrome");

Cu.import("resource://gre/modules/Services.jsm");

const events = require("sdk/event/core");
const protocol = require("devtools/server/protocol");

const { on, once, off, emit } = events;
const { method, Arg, Option, RetVal } = protocol;

const WEBGL_CONTEXT_NAMES = ["webgl", "experimental-webgl", "moz-webgl"];

const HIGHLIGHT_FRAG_SHADER = [
  "precision lowp float;",
  "void main() {",
    "gl_FragColor.rgba = vec4(%color);",
  "}"
].join("\n");


const PROGRAM_DEFAULT_TRAITS = 0;
const PROGRAM_BLACKBOX_TRAIT = 1;

exports.register = function(handle) {
  handle.addTabActor(WebGLActor, "webglActor");
}

exports.unregister = function(handle) {
  handle.removeTabActor(WebGLActor);
}






let ShaderActor = protocol.ActorClass({
  typeName: "gl-shader",

  











  initialize: function(conn, program, shader, proxy) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.program = program;
    this.shader = shader;
    this.text = proxy.getShaderSource(shader);
    this.linkedProxy = proxy;
  },

  


  getText: method(function() {
    return this.text;
  }, {
    response: { text: RetVal("string") }
  }),

  


  compile: method(function(text) {
    
    let { linkedProxy: proxy, shader, program } = this;

    
    let oldText = this.text;
    let newText = text;

    
    let error = proxy.compileShader(program, shader, this.text = newText);

    
    if (error.compile || error.link) {
      proxy.compileShader(program, shader, this.text = oldText);
      return error;
    }
    return undefined;
  }, {
    request: { text: Arg(0, "string") },
    response: { error: RetVal("nullable:json") }
  })
});




let ShaderFront = protocol.FrontClass(ShaderActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
  }
});





let ProgramActor = protocol.ActorClass({
  typeName: "gl-program",

  













  initialize: function(conn, [program, shaders, cache, proxy]) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this._shaderActorsCache = { vertex: null, fragment: null };
    this.program = program;
    this.shaders = shaders;
    this.linkedCache = cache;
    this.linkedProxy = proxy;
  },

  get ownerWindow() this.linkedCache.ownerWindow,
  get ownerContext() this.linkedCache.ownerContext,

  



  getVertexShader: method(function() {
    return this._getShaderActor("vertex");
  }, {
    response: { shader: RetVal("gl-shader") }
  }),

  



  getFragmentShader: method(function() {
    return this._getShaderActor("fragment");
  }, {
    response: { shader: RetVal("gl-shader") }
  }),

  



  highlight: method(function(color) {
    let shaderActor = this._getShaderActor("fragment");
    let oldText = shaderActor.text;
    let newText = HIGHLIGHT_FRAG_SHADER.replace("%color", color)
    shaderActor.compile(newText);
    shaderActor.text = oldText;
  }, {
    request: { color: Arg(0, "array:string") },
    oneway: true
  }),

  


  unhighlight: method(function() {
    let shaderActor = this._getShaderActor("fragment");
    shaderActor.compile(shaderActor.text);
  }, {
    oneway: true
  }),

  


  blackbox: method(function() {
    this.linkedCache.setProgramTrait(this.program, PROGRAM_BLACKBOX_TRAIT);
  }, {
    oneway: true
  }),

  


  unblackbox: method(function() {
    this.linkedCache.unsetProgramTrait(this.program, PROGRAM_BLACKBOX_TRAIT);
  }, {
    oneway: true
  }),

  







  _getShaderActor: function(type) {
    if (this._shaderActorsCache[type]) {
      return this._shaderActorsCache[type];
    }
    let proxy = this.linkedProxy;
    let shader = proxy.getShaderOfType(this.shaders, type);
    let shaderActor = new ShaderActor(this.conn, this.program, shader, proxy);
    return this._shaderActorsCache[type] = shaderActor;
  }
});




let ProgramFront = protocol.FrontClass(ProgramActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client, form);
  }
});






let WebGLActor = exports.WebGLActor = protocol.ActorClass({
  typeName: "webgl",
  initialize: function(conn, tabActor) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this.tabActor = tabActor;
    this._onGlobalCreated = this._onGlobalCreated.bind(this);
    this._onGlobalDestroyed = this._onGlobalDestroyed.bind(this);
    this._onProgramLinked = this._onProgramLinked.bind(this);
  },
  destroy: function(conn) {
    protocol.Actor.prototype.destroy.call(this, conn);
    this.finalize();
  },

  






  setup: method(function({ reload }) {
    if (this._initialized) {
      return;
    }
    this._initialized = true;

    this._programActorsCache = [];
    this._contentObserver = new ContentObserver(this.tabActor);
    this._webglObserver = new WebGLObserver();

    on(this._contentObserver, "global-created", this._onGlobalCreated);
    on(this._contentObserver, "global-destroyed", this._onGlobalDestroyed);
    on(this._webglObserver, "program-linked", this._onProgramLinked);

    if (reload) {
      this.tabActor.window.location.reload();
    }
  }, {
    request: { reload: Option(0, "boolean") },
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
    off(this._webglObserver, "program-linked", this._onProgramLinked);

    this._programActorsCache = null;
    this._contentObserver = null;
    this._webglObserver = null;
  }, {
   oneway: true
  }),

  



  getPrograms: method(function() {
    let id = getInnerWindowID(this.tabActor.window);
    return this._programActorsCache.filter(e => e.ownerWindow == id);
  }, {
    response: { programs: RetVal("array:gl-program") }
  }),

  



  events: {
    "program-linked": {
      type: "programLinked",
      program: Arg(0, "gl-program")
    }
  },

  


  _onGlobalCreated: function(window) {
    WebGLInstrumenter.handle(window, this._webglObserver);
  },

  


  _onGlobalDestroyed: function(id) {
    removeFromArray(this._programActorsCache, e => e.ownerWindow == id);
    this._webglObserver.unregisterContextsForWindow(id);
  },

  


  _onProgramLinked: function(...args) {
    let programActor = new ProgramActor(this.conn, args);
    this._programActorsCache.push(programActor);
    events.emit(this, "program-linked", programActor);
  }
});




let WebGLFront = exports.WebGLFront = protocol.FrontClass(WebGLActor, {
  initialize: function(client, { webglActor }) {
    protocol.Front.prototype.initialize.call(this, client, { actor: webglActor });
    client.addActorPool(this);
    this.manage(this);
  }
});







function ContentObserver(tabActor) {
  this._contentWindow = tabActor.window;
  this._onContentGlobalCreated = this._onContentGlobalCreated.bind(this);
  this._onInnerWindowDestroyed = this._onInnerWindowDestroyed.bind(this);
  this.startListening();
}

ContentObserver.prototype = {
  


  startListening: function() {
    Services.obs.addObserver(
      this._onContentGlobalCreated, "content-document-global-created", false);
    Services.obs.addObserver(
      this._onInnerWindowDestroyed, "inner-window-destroyed", false);
  },

  


  stopListening: function() {
    Services.obs.removeObserver(
      this._onContentGlobalCreated, "content-document-global-created", false);
    Services.obs.removeObserver(
      this._onInnerWindowDestroyed, "inner-window-destroyed", false);
  },

  


  _onContentGlobalCreated: function(subject, topic, data) {
    if (subject == this._contentWindow) {
      emit(this, "global-created", subject);
    }
  },

  


  _onInnerWindowDestroyed: function(subject, topic, data) {
    let id = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
    emit(this, "global-destroyed", id);
  }
};




let WebGLInstrumenter = {
  







  handle: function(window, observer) {
    let self = this;

    let id = getInnerWindowID(window);
    let canvasElem = XPCNativeWrapper.unwrap(window.HTMLCanvasElement);
    let canvasPrototype = canvasElem.prototype;
    let originalGetContext = canvasPrototype.getContext;

    




    canvasPrototype.getContext = function(name, options) {
      
      let context = originalGetContext.call(this, name, options);
      if (!context) {
        return context;
      }
      
      if (WEBGL_CONTEXT_NAMES.indexOf(name) == -1) {
        return context;
      }
      
      
      if (observer.for(context)) {
        return context;
      }

      
      observer.registerContextForWindow(id, context);

      
      for (let { timing, callback, functions } of self._methods) {
        for (let func of functions) {
          self._instrument(observer, context, func, callback, timing);
        }
      }

      
      
      return context;
    };
  },

  
















  _instrument: function(observer, context, funcName, callbackName, timing = 0) {
    let { cache, proxy } = observer.for(context);
    let originalFunc = context[funcName];
    let proxyFuncName = callbackName || funcName;

    context[funcName] = function(...glArgs) {
      if (timing == 0 && !observer.suppressHandlers) {
        let glBreak = observer[proxyFuncName](glArgs, cache, proxy);
        if (glBreak) return undefined;
      }

      let glResult = originalFunc.apply(this, glArgs);

      if (timing == 1 && !observer.suppressHandlers) {
        let glBreak = observer[proxyFuncName](glArgs, glResult, cache, proxy);
        if (glBreak) return undefined;
      }

      return glResult;
    };
  },

  


  _methods: [{
    timing: 1, 
    functions: [
      "linkProgram", "getAttribLocation", "getUniformLocation"
    ]
  }, {
    callback: "toggleVertexAttribArray",
    functions: [
      "enableVertexAttribArray", "disableVertexAttribArray"
    ]
  }, {
    callback: "attribute_",
    functions: [
      "vertexAttrib1f", "vertexAttrib2f", "vertexAttrib3f", "vertexAttrib4f",
      "vertexAttrib1fv", "vertexAttrib2fv", "vertexAttrib3fv", "vertexAttrib4fv",
      "vertexAttribPointer"
    ]
  }, {
    callback: "uniform_",
    functions: [
      "uniform1i", "uniform2i", "uniform3i", "uniform4i",
      "uniform1f", "uniform2f", "uniform3f", "uniform4f",
      "uniform1iv", "uniform2iv", "uniform3iv", "uniform4iv",
      "uniform1fv", "uniform2fv", "uniform3fv", "uniform4fv",
      "uniformMatrix2fv", "uniformMatrix3fv", "uniformMatrix4fv"
    ]
  }, {
    timing: 1, 
    functions: ["useProgram"]
  }, {
    callback: "draw_",
    functions: [
      "drawArrays", "drawElements"
    ]
  }]
  
  
  
  
  
};




function WebGLObserver() {
  this._contexts = new Map();
}

WebGLObserver.prototype = {
  _contexts: null,

  







  registerContextForWindow: function(id, context) {
    let cache = new WebGLCache(id, context);
    let proxy = new WebGLProxy(id, context, cache, this);

    this._contexts.set(context, {
      ownerWindow: id,
      cache: cache,
      proxy: proxy
    });
  },

  





  unregisterContextsForWindow: function(id) {
    removeFromMap(this._contexts, e => e.ownerWindow == id);
  },

  







  for: function(context) {
    return this._contexts.get(context);
  },

  


  suppressHandlers: false,

  











  linkProgram: function(glArgs, glResult, cache, proxy) {
    let program = glArgs[0];
    let shaders = proxy.getAttachedShaders(program);
    cache.addProgram(program, PROGRAM_DEFAULT_TRAITS);
    emit(this, "program-linked", program, shaders, cache, proxy);
  },

  









  getAttribLocation: function(glArgs, glResult, cache) {
    
    if (glResult < 0) {
      return;
    }
    let [program, name] = glArgs;
    cache.addAttribute(program, name, glResult);
  },

  









  getUniformLocation: function(glArgs, glResult, cache) {
    
    if (!glResult) {
      return;
    }
    let [program, name] = glArgs;
    cache.addUniform(program, name, glResult);
  },

  








  toggleVertexAttribArray: function(glArgs, cache) {
    glArgs[0] = cache.getCurrentAttributeLocation(glArgs[0]);
    return glArgs[0] < 0; 
  },

  







  attribute_: function(glArgs, cache) {
    glArgs[0] = cache.getCurrentAttributeLocation(glArgs[0]);
    return glArgs[0] < 0; 
  },

  







  uniform_: function(glArgs, cache) {
    glArgs[0] = cache.getCurrentUniformLocation(glArgs[0]);
    return !glArgs[0]; 
  },

  









  useProgram: function(glArgs, glResult, cache) {
    
    
    cache.currentProgram = glArgs[0];
  },

  








  draw_: function(glArgs, cache) {
    
    return cache.currentProgramTraits & PROGRAM_BLACKBOX_TRAIT;
  }
};










function WebGLCache(id, context) {
  this._id = id;
  this._gl = context;
  this._programs = new Map();
}

WebGLCache.prototype = {
  _id: 0,
  _gl: null,
  _programs: null,
  _currentProgramInfo: null,
  _currentAttributesMap: null,
  _currentUniformsMap: null,

  get ownerWindow() this._id,
  get ownerContext() this._gl,

  







  addProgram: function(program, traits) {
    this._programs.set(program, {
      traits: traits,
      attributes: [], 
      uniforms: new Map() 
    });
  },

  








  setProgramTrait: function(program, trait) {
    this._programs.get(program).traits |= trait;
  },

  







  unsetProgramTrait: function(program, trait) {
    this._programs.get(program).traits &= ~trait;
  },

  



  set currentProgram(program) {
    let programInfo = this._programs.get(program);
    if (programInfo == null) {
      return;
    }
    this._currentProgramInfo = programInfo;
    this._currentAttributesMap = programInfo.attributes;
    this._currentUniformsMap = programInfo.uniforms;
  },

  



  get currentProgramTraits() {
    return this._currentProgramInfo.traits;
  },

  









  addAttribute: function(program, name, value) {
    this._programs.get(program).attributes[value] = {
      name: name,
      value: value
    };
  },

  









  addUniform: function(program, name, value) {
    this._programs.get(program).uniforms.set(new XPCNativeWrapper(value), {
      name: name,
      value: value
    });
  },

  







  updateAttributesForProgram: function(program) {
    let attributes = this._programs.get(program).attributes;
    for (let attribute of attributes) {
      attribute.value = this._gl.getAttribLocation(program, attribute.name);
    }
  },

  







  updateUniformsForProgram: function(program) {
    let uniforms = this._programs.get(program).uniforms;
    for (let [, uniform] of uniforms) {
      uniform.value = this._gl.getUniformLocation(program, uniform.name);
    }
  },

  










  getCurrentAttributeLocation: function(initialValue) {
    let attributes = this._currentAttributesMap;
    let currentInfo = attributes ? attributes[initialValue] : null;
    return currentInfo ? currentInfo.value : initialValue;
  },

  










  getCurrentUniformLocation: function(initialValue) {
    let uniforms = this._currentUniformsMap;
    let currentInfo = uniforms ? uniforms.get(initialValue) : null;
    return currentInfo ? currentInfo.value : initialValue;
  }
};

















function WebGLProxy(id, context, cache, observer) {
  this._id = id;
  this._gl = context;
  this._cache = cache;
  this._observer = observer;

  let exports = [
    "getAttachedShaders",
    "getShaderSource",
    "getShaderOfType",
    "compileShader"
  ];
  exports.forEach(e => this[e] = (...args) => this._call(e, args));
}

WebGLProxy.prototype = {
  _id: 0,
  _gl: null,
  _cache: null,
  _observer: null,

  get ownerWindow() this._id,
  get ownerContext() this._gl,

  







  _getAttachedShaders: function(program) {
    return this._gl.getAttachedShaders(program);
  },

  







  _getShaderSource: function(shader) {
    return this._gl.getShaderSource(shader);
  },

  









  _getShaderOfType: function(shaders, type) {
    let gl = this._gl;
    let shaderTypeEnum = {
      vertex: gl.VERTEX_SHADER,
      fragment: gl.FRAGMENT_SHADER
    }[type];

    for (let shader of shaders) {
      if (gl.getShaderParameter(shader, gl.SHADER_TYPE) == shaderTypeEnum) {
        return shader;
      }
    }
    return null;
  },

  











  _compileShader: function(program, shader, text) {
    let gl = this._gl;
    gl.shaderSource(shader, text);
    gl.compileShader(shader);
    gl.linkProgram(program);

    let error = { compile: "", link: "" };

    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      error.compile = gl.getShaderInfoLog(shader);
    }
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
      error.link = gl.getShaderInfoLog(shader);
    }

    this._cache.updateAttributesForProgram(program);
    this._cache.updateUniformsForProgram(program);

    return error;
  },

  












  _call: function(funcName, args) {
    let prevState = this._observer.suppressHandlers;

    this._observer.suppressHandlers = true;
    let result = this["_" + funcName].apply(this, args);
    this._observer.suppressHandlers = prevState;

    return result;
  }
};



function getInnerWindowID(window) {
  return window
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindowUtils)
    .currentInnerWindowID;
}

function removeFromMap(map, predicate) {
  for (let [key, value] of map) {
    if (predicate(value)) {
      map.delete(key);
    }
  }
};

function removeFromArray(array, predicate) {
  for (let value of array) {
    if (predicate(value)) {
      array.splice(array.indexOf(value), 1);
    }
  }
}
