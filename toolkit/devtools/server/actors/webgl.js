


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

exports.register = function(handle) {
  handle.addTabActor(WebGLActor, "webglActor");
}

exports.unregister = function(handle) {
  handle.removeTabActor(WebGLActor);
}






let ShaderActor = protocol.ActorClass({
  typeName: "gl-shader",
  initialize: function(conn, id) {
    protocol.Actor.prototype.initialize.call(this, conn);
  },

  


  getText: method(function() {
    return this.text;
  }, {
    response: { text: RetVal("string") }
  }),

  


  compile: method(function(text) {
    
    let { context, shader, program, observer: { proxy } } = this;

    
    let oldText = this.text;
    let newText = text;

    
    let error = proxy.call("compileShader", context, program, shader, this.text = newText);

    
    if (error.compile || error.link) {
      proxy.call("compileShader", context, program, shader, this.text = oldText);
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
  initialize: function(conn, id) {
    protocol.Actor.prototype.initialize.call(this, conn);
    this._shaderActorsCache = { vertex: null, fragment: null };
  },

  



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
    this.observer.cache.blackboxedPrograms.add(this.program);
  }, {
    oneway: true
  }),

  


  unblackbox: method(function() {
    this.observer.cache.blackboxedPrograms.delete(this.program);
  }, {
    oneway: true
  }),

  







  _getShaderActor: function(type) {
    if (this._shaderActorsCache[type]) {
      return this._shaderActorsCache[type];
    }

    let shaderActor = new ShaderActor(this.conn);
    shaderActor.context = this.context;
    shaderActor.observer = this.observer;
    shaderActor.program = this.program;
    shaderActor.shader = this.shadersData[type].ref;
    shaderActor.text = this.shadersData[type].text;

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
    this._programActorsCache = [];
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
  }, {
   oneway: true
  }),

  



  getPrograms: method(function() {
    let id = getInnerWindowID(this.tabActor.window);
    return this._programActorsCache.filter(e => e.owner == id).map(e => e.actor);
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
    this._programActorsCache =
      this._programActorsCache.filter(e => e.owner != id);
  },

  


  _onProgramLinked: function(gl, program, shaders) {
    let observer = this._webglObserver;
    let shadersData = { vertex: null, fragment: null };

    for (let shader of shaders) {
      let text = observer.cache.call("getShaderInfo", shader);
      let data = { ref: shader, text: text };

      
      
      
      if (gl.getShaderParameter(shader, gl.SHADER_TYPE) == gl.VERTEX_SHADER) {
        shadersData.vertex = data;
      } else {
        shadersData.fragment = data;
      }
    }

    let programActor = new ProgramActor(this.conn);
    programActor.context = gl;
    programActor.observer = observer;
    programActor.program = program;
    programActor.shadersData = shadersData;

    this._programActorsCache.push({
      owner: getInnerWindowID(this.tabActor.window),
      actor: programActor
    });

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

      
      for (let { timing, callback, functions } of self._methods) {
        for (let func of functions) {
          self._instrument(observer, context, func, timing, callback);
        }
      }

      
      
      return context;
    };
  },

  















  _instrument: function(observer, context, funcName, timing, callbackName) {
    let originalFunc = context[funcName];

    context[funcName] = function() {
      let glArgs = Array.slice(arguments);
      let glResult, glBreak;

      if (timing == "before" && !observer.suppressHandlers) {
        glBreak = observer.call(callbackName || funcName, context, glArgs);
        if (glBreak) return undefined;
      }

      glResult = originalFunc.apply(this, glArgs);

      if (timing == "after" && !observer.suppressHandlers) {
        glBreak = observer.call(callbackName || funcName, context, glArgs, glResult);
        if (glBreak) return undefined;
      }

      return glResult;
    };
  },

  


  _methods: [{
    timing: "after",
    functions: [
      "linkProgram", "getAttribLocation", "getUniformLocation"
    ]
  }, {
    timing: "before",
    callback: "toggleVertexAttribArray",
    functions: [
      "enableVertexAttribArray", "disableVertexAttribArray"
    ]
  }, {
    timing: "before",
    callback: "attribute_",
    functions: [
      "vertexAttrib1f", "vertexAttrib2f", "vertexAttrib3f", "vertexAttrib4f",
      "vertexAttrib1fv", "vertexAttrib2fv", "vertexAttrib3fv", "vertexAttrib4fv",
      "vertexAttribPointer"
    ]
  }, {
    timing: "before",
    callback: "uniform_",
    functions: [
      "uniform1i", "uniform2i", "uniform3i", "uniform4i",
      "uniform1f", "uniform2f", "uniform3f", "uniform4f",
      "uniform1iv", "uniform2iv", "uniform3iv", "uniform4iv",
      "uniform1fv", "uniform2fv", "uniform3fv", "uniform4fv",
      "uniformMatrix2fv", "uniformMatrix3fv", "uniformMatrix4fv"
    ]
  }, {
    timing: "after",
    functions: ["useProgram"]
  }, {
    timing: "before",
    callback: "draw_",
    functions: [
      "drawArrays", "drawElements"
    ]
  }]
  
  
  
  
  
};




function WebGLObserver() {
  this.cache = new WebGLCache(this);
  this.proxy = new WebGLProxy(this);
}

WebGLObserver.prototype = {
  


  suppressHandlers: false,

  









  linkProgram: function(gl, glArgs, glResult) {
    let program = glArgs[0];
    let shaders = gl.getAttachedShaders(program);

    for (let shader of shaders) {
      let source = gl.getShaderSource(shader);
      this.cache.call("addShaderInfo", shader, source);
    }

    emit(this, "program-linked", gl, program, shaders);
  },

  









  getAttribLocation: function(gl, glArgs, glResult) {
    let [program, name] = glArgs;
    this.cache.call("addAttribute", program, name, glResult);
  },

  









  getUniformLocation: function(gl, glArgs, glResult) {
    let [program, name] = glArgs;
    this.cache.call("addUniform", program, name, glResult);
  },

  








  toggleVertexAttribArray: function(gl, glArgs) {
    glArgs[0] = this.cache.call("getCurrentAttributeLocation", glArgs[0]);
    return glArgs[0] < 0; 
  },

  







  attribute_: function(gl, glArgs) {
    glArgs[0] = this.cache.call("getCurrentAttributeLocation", glArgs[0]);
    return glArgs[0] < 0; 
  },

  







  uniform_: function(gl, glArgs) {
    glArgs[0] = this.cache.call("getCurrentUniformLocation", glArgs[0]);
    return !glArgs[0]; 
  },

  









  useProgram: function(gl, glArgs, glResult) {
    
    
    this.cache.currentProgram = glArgs[0];
  },

  








  draw_: function(gl, glArgs) {
    if (this.cache.blackboxedPrograms.has(this.cache.currentProgram)) {
      return true; 
    }
  },

  







  call: function(funcName, ...args) {
    let prevState = this.suppressHandlers;

    this.suppressHandlers = true;
    let result = this[funcName].apply(this, args);
    this.suppressHandlers = prevState;

    return result;
  }
};







function WebGLCache(observer) {
  this._observer = observer;

  this.currentProgram = null;
  this.blackboxedPrograms = new Set();

  this._shaders = new Map();
  this._attributes = [];
  this._uniforms = [];
  this._attributesBridge = new Map();
  this._uniformsBridge = new Map();
}

WebGLCache.prototype = {
  


  currentProgram: null,

  


  blackboxedPrograms: null,

  








  _addShaderInfo: function(shader, text) {
    if (!this._shaders.has(shader)) {
      this._shaders.set(shader, text);
    }
  },

  







  _getShaderInfo: function(shader) {
    return this._shaders.get(shader);
  },

  










  _addAttribute: function(program, name, value) {
    let isCached = this._attributes.some(e => e.program == program && e.name == name);
    if (isCached || value < 0) {
      return;
    }
    let attributeInfo = {
      program: program,
      name: name,
      value: value
    };
    this._attributes.push(attributeInfo);
    this._attributesBridge.set(value, attributeInfo);
  },

  










  _addUniform: function(program, name, value) {
    let isCached = this._uniforms.some(e => e.program == program && e.name == name);
    if (isCached || !value) {
      return;
    }
    let uniformInfo = {
      program: program,
      name: name,
      value: value
    };
    this._uniforms.push(uniformInfo);
    this._uniformsBridge.set(new XPCNativeWrapper(value), uniformInfo);
  },

  







  _getAttributesForProgram: function(program) {
    return this._attributes.filter(e => e.program == program);
  },

  







  _getUniformsForProgram: function(program) {
    return this._uniforms.filter(e => e.program == program);
  },

  









  _updateAttributesForProgram: function(gl, program) {
    let dirty = this._attributes.filter(e => e.program == program);
    dirty.forEach(e => e.value = gl.getAttribLocation(program, e.name));
  },

  









  _updateUniformsForProgram: function(gl, program) {
    let dirty = this._uniforms.filter(e => e.program == program);
    dirty.forEach(e => e.value = gl.getUniformLocation(program, e.name));
  },

  










  _getCurrentAttributeLocation: function(initialValue) {
    let currentInfo = this._attributesBridge.get(initialValue);
    return currentInfo ? currentInfo.value : initialValue;
  },

  










  _getCurrentUniformLocation: function(initialValue) {
    let currentInfo = this._uniformsBridge.get(initialValue);
    return currentInfo ? currentInfo.value : initialValue;
  },

  









  call: function(funcName, ...aArgs) {
    let prevState = this._observer.suppressHandlers;

    this._observer.suppressHandlers = true;
    let result = this["_" + funcName].apply(this, aArgs);
    this._observer.suppressHandlers = prevState;

    return result;
  }
};







function WebGLProxy(observer) {
  this._observer = observer;
}

WebGLProxy.prototype = {
  get cache() this._observer.cache,

  













  _compileShader: function(gl, program, shader, text) {
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

    this.cache.call("updateAttributesForProgram", gl, program);
    this.cache.call("updateUniformsForProgram", gl, program);

    return error;
  },

  









  call: function(funcName, ...aArgs) {
    let prevState = this._observer.suppressHandlers;

    this._observer.suppressHandlers = true;
    let result = this["_" + funcName].apply(this, aArgs);
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
