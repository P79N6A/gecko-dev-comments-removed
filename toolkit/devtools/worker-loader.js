



"use strict";














this.EXPORTED_SYMBOLS = ["WorkerDebuggerLoader", "worker"];






















function resolveId(id, baseId) {
  return baseId + "/../" + id;
};










function normalizeId(id) {
  
  
  
  let [_, root, path] = id.match(/^(\w+:\/\/\/?|\/)?(.*)/);

  let stack = [];
  path.split("/").forEach(function (component) {
    switch (component) {
    case "":
    case ".":
      break;
    case "..":
      if (stack.length === 0) {
        if (root !== undefined) {
          throw new Error("Can't normalize absolute id '" + id + "'!");
        } else {
          stack.push("..");
        }
      } else {
        if (stack[stack.length] == "..") {
          stack.push("..");
        } else {
          stack.pop();
        }
      }
      break;
    default:
      stack.push(component);
      break;
    }
  });

  return (root ? root : "") + stack.join("/");
}










function createModule(id) {
  return Object.create(null, {
    
    
    id: {
      configurable: false,
      enumerable: true,
      value: id,
      writable: false
    },

    
    
    exports: {
      configurable: false,
      enumerable: true,
      value: Object.create(null),
      writable: true
    }
  });
};


























function WorkerDebuggerLoader(options) {
  









  function resolveURL(url) {
    let found = false;
    for (let [path, baseURL] of paths) {
      if (url.startsWith(path)) {
        found = true;
        url = url.replace(path, baseURL);
        break;
      }
    }
    if (!found) {
      throw new Error("Can't resolve relative URL '" + url + "'!");
    }

    
    return url.endsWith(".js") ? url : url + ".js";
  }

  







  function loadModule(module, url) {
    
    
    
    
    let prototype = Object.create(globals);
    prototype.Components = {};
    prototype.require = createRequire(module);
    prototype.exports = module.exports;
    prototype.module = module;

    let sandbox = createSandbox(url, prototype);
    try {
      loadSubScript(url, sandbox);
    } catch (error) {
      if (/^Error opening input stream/.test(String(error))) {
        throw new Error("Can't load module '" + module.id + "' with url '" +
                        url + "'!");
      }
      throw error;
    }

    
    
    if (typeof module.exports === "object" && module.exports !== null) {
      Object.freeze(module.exports);
    }
  };

  









  function createRequire(requirer) {
    return function require(id) {
      
      if (id === undefined) {
        throw new Error("Can't require module without id!");
      }

      
      
      let module = modules[id];
      if (module === undefined) {
        
        

        
        if (id.startsWith(".")) {
          if (requirer === undefined) {
            throw new Error("Can't require top-level module with relative id " +
                            "'" + id + "'!");
          }
          id = resolve(id, requirer.id);
        }

        
        id = normalizeId(id);

        
        let url = id;

        
        if (url.match(/^\w+:\/\//) === null) {
          url = resolveURL(id);
        }

        
        module = modules[url];
        if (module === undefined) {
          
          

          
          
          
          module = modules[url] = createModule(id);

          try {
            loadModule(module, url);
          } catch (error) {
            
            
            
            
            delete modules[url];
            throw error;
          }

          Object.freeze(module);
        }
      }

      return module.exports;
    };
  }

  let createSandbox = options.createSandbox;
  let globals = options.globals || Object.create(null);
  let loadSubScript = options.loadSubScript;

  
  
  
  let modules = options.modules || {};
  for (let id in modules) {
    let module = createModule(id);
    module.exports = Object.freeze(modules[id]);
    modules[id] = module;
  }

  
  
  
  let paths = options.paths || Object.create(null);
  paths = Object.keys(paths)
                .sort((a, b) => b.length - a.length)
                .map(path => [path, paths[path]]);

  let resolve = options.resolve || resolveId;

  this.require = createRequire();
}

this.WorkerDebuggerLoader = WorkerDebuggerLoader;





let PromiseDebugging = {
  getState: function () {
    throw new Error("PromiseDebugging is not available in workers!");
  }
};

let chrome = {
  CC: undefined,
  Cc: undefined,
  ChromeWorker: undefined,
  Cm: undefined,
  Ci: undefined,
  Cu: undefined,
  Cr: undefined,
  components: undefined
};

let loader = {
  lazyGetter: function (object, name, lambda) {
    Object.defineProperty(object, name, {
      get: function () {
        delete object[name];
        return object[name] = lambda.apply(object);
      },
      configurable: true,
      enumerable: true
    });
  },
  lazyImporter: function () {
    throw new Error("Can't import JSM from worker thread!");
  },
  lazyServiceGetter: function () {
    throw new Error("Can't import XPCOM service from worker thread!");
  },
  lazyRequireGetter: function (obj, property, module, destructure) {
    Object.defineProperty(obj, property, {
      get: () => destructure ? worker.require(module)[property]
                             : worker.require(module || property)
    });
  }
};






let {
  Debugger,
  createSandbox,
  dump,
  loadSubScript,
  reportError,
  setImmediate,
  xpcInspector
} = (function () {
  if (typeof Components === "object") { 
    let {
      Constructor: CC,
      classes: Cc,
      manager: Cm,
      interfaces: Ci,
      results: Cr,
      utils: Cu
    } = Components;

    let principal = CC('@mozilla.org/systemprincipal;1', 'nsIPrincipal')();

    
    
    let sandbox = Cu.Sandbox(principal, {});
    Cu.evalInSandbox(
      "Components.utils.import('resource://gre/modules/jsdebugger.jsm');" +
      "addDebuggerToGlobal(this);",
      sandbox
    );
    let Debugger = sandbox.Debugger;

    let createSandbox = function(name, prototype) {
      return Cu.Sandbox(principal, {
        invisibleToDebugger: true,
        sandboxName: name,
        sandboxPrototype: prototype,
        wantComponents: false,
        wantXrays: false
      });
    };

    let subScriptLoader = Cc['@mozilla.org/moz/jssubscript-loader;1'].
                 getService(Ci.mozIJSSubScriptLoader);

    let loadSubScript = function (url, sandbox) {
      subScriptLoader.loadSubScript(url, sandbox, "UTF-8");
    };

    let reportError = Cu.reportError;

    let Timer = Cu.import("resource://gre/modules/Timer.jsm", {});

    let setImmediate = function (callback) {
      Timer.setTimeout(callback, 0);
    }

    let xpcInspector = Cc["@mozilla.org/jsinspector;1"].
                       getService(Ci.nsIJSInspector);

    return {
      Debugger,
      createSandbox,
      dump,
      loadSubScript,
      reportError,
      setImmediate,
      xpcInspector
    };
  } else { 
    let requestors = [];

    let scope = this;

    let xpcInspector = {
      get lastNestRequestor() {
        return requestors.length === 0 ? null : requestors[0];
      },

      enterNestedEventLoop: function (requestor) {
        requestors.push(requestor);
        scope.enterEventLoop();
        return requestors.length;
      },

      exitNestedEventLoop: function () {
        requestors.pop();
        scope.leaveEventLoop();
        return requestors.length;
      }
    };

    return {
      Debugger: this.Debugger,
      createSandbox: this.createSandbox,
      dump: this.dump,
      loadSubScript: this.loadSubScript,
      reportError: this.reportError,
      setImmediate: this.setImmediate,
      xpcInspector: xpcInspector
    };
  }
}).call(this);




this.worker = new WorkerDebuggerLoader({
  createSandbox: createSandbox,
  globals: {
    "isWorker": true,
    "dump": dump,
    "loader": loader,
    "reportError": reportError,
    "setImmediate": setImmediate
  },
  loadSubScript: loadSubScript,
  modules: {
    "Debugger": Debugger,
    "PromiseDebugging": PromiseDebugging,
    "Services": Object.create(null),
    "chrome": chrome,
    "xpcInspector": xpcInspector
  },
  paths: {
    "": "resource://gre/modules/commonjs/",
    "devtools": "resource:///modules/devtools",
    "devtools/server": "resource://gre/modules/devtools/server",
    "devtools/toolkit": "resource://gre/modules/devtools",
    "promise": "resource://gre/modules/Promise-backend.js",
    "source-map": "resource://gre/modules/devtools/source-map",
    "xpcshell-test": "resource://test"
  }
});
