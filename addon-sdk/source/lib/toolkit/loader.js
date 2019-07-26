




;(function(id, factory) { 
  if (typeof(define) === 'function') { 
    define(factory);
  } else if (typeof(require) === 'function') { 
    factory.call(this, require, exports, module);
  } else if (~String(this).indexOf('BackstagePass')) { 
    this[factory.name] = {};
    factory(function require(uri) {
      var imports = {};
      this['Components'].utils.import(uri, imports);
      return imports;
    }, this[factory.name], { uri: __URI__, id: id });
    this.EXPORTED_SYMBOLS = [factory.name];
  } else if (~String(this).indexOf('Sandbox')) { 
    factory(function require(uri) {}, this, { uri: __URI__, id: id });
  } else {  
    var globals = this
    factory(function require(id) {
      return globals[id];
    }, (globals[id] = {}), { uri: document.location.href + '#' + id, id: id });
  }
}).call(this, 'loader', function Loader(require, exports, module) {

'use strict';

module.metadata = {
  "stability": "unstable"
};

const { classes: Cc, Constructor: CC, interfaces: Ci, utils: Cu,
        results: Cr, manager: Cm } = Components;
const systemPrincipal = CC('@mozilla.org/systemprincipal;1', 'nsIPrincipal')();
const { loadSubScript } = Cc['@mozilla.org/moz/jssubscript-loader;1'].
                     getService(Ci.mozIJSSubScriptLoader);
const { notifyObservers } = Cc['@mozilla.org/observer-service;1'].
                        getService(Ci.nsIObserverService);


const bind = Function.call.bind(Function.bind);
const getOwnPropertyNames = Object.getOwnPropertyNames;
const getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;
const define = Object.defineProperties;
const prototypeOf = Object.getPrototypeOf;
const create = Object.create;
const keys = Object.keys;



function freeze(object) {
  if (prototypeOf(object) === null) {
      Object.freeze(object);
  }
  else {
    prototypeOf(prototypeOf(object.isPrototypeOf)).
      constructor. 
      freeze(object);
  }
  return object;
}


const descriptor = iced(function descriptor(object) {
  let value = {};
  getOwnPropertyNames(object).forEach(function(name) {
    value[name] = getOwnPropertyDescriptor(object, name)
  });
  return value;
});
exports.descriptor = descriptor;



freeze(Object);
freeze(Object.prototype);
freeze(Function);
freeze(Function.prototype);
freeze(Array);
freeze(Array.prototype);
freeze(String);
freeze(String.prototype);





function iced(f) {
  f.prototype = undefined;
  return freeze(f);
}






const override = iced(function override(target, source) {
  let properties = descriptor(target)
  let extension = descriptor(source || {})
  getOwnPropertyNames(extension).forEach(function(name) {
    properties[name] = extension[name];
  });
  return define({}, properties);
});
exports.override = override;


function sourceURI(uri) { return String(uri).split(" -> ").pop(); }
exports.sourceURI = iced(sourceURI);

function isntLoaderFrame(frame) { return frame.fileName !== module.uri }

var parseStack = iced(function parseStack(stack) {
  let lines = String(stack).split("\n");
  return lines.reduce(function(frames, line) {
    if (line) {
      let atIndex = line.indexOf("@");
      let columnIndex = line.lastIndexOf(":");
      let fileName = sourceURI(line.slice(atIndex + 1, columnIndex));
      let lineNumber = parseInt(line.slice(columnIndex + 1));
      let name = line.slice(0, atIndex).split("(").shift();
      frames.unshift({
        fileName: fileName,
        name: name,
        lineNumber: lineNumber
      });
    }
    return frames;
  }, []);
})
exports.parseStack = parseStack

var serializeStack = iced(function serializeStack(frames) {
  return frames.reduce(function(stack, frame) {
    return frame.name + "@" +
           frame.fileName + ":" +
           frame.lineNumber + "\n" +
           stack;
  }, "");
})
exports.serializeStack = serializeStack
















const Sandbox = iced(function Sandbox(options) {
  
  options = {
    
    
    wantComponents: false,
    sandboxName: options.name,
    principal: 'principal' in options ? options.principal : systemPrincipal,
    wantXrays: 'wantXrays' in options ? options.wantXrays : true,
    sandboxPrototype: 'prototype' in options ? options.prototype : {},
    sameGroupAs: 'sandbox' in options ? options.sandbox : null
  };

  
  
  if (!options.sameGroupAs)
    delete options.sameGroupAs;

  let sandbox = Cu.Sandbox(options.principal, options);

  
  
  
  delete sandbox.Iterator;
  delete sandbox.Components;
  delete sandbox.importFunction;
  delete sandbox.debug;

  return sandbox;
});
exports.Sandbox = Sandbox;








const evaluate = iced(function evaluate(sandbox, uri, options) {
  let { source, line, version, encoding } = override({
    encoding: 'UTF-8',
    line: 1,
    version: '1.8',
    source: null
  }, options);

  return source ? Cu.evalInSandbox(source, sandbox, version, uri, line)
                : loadSubScript(uri, sandbox, encoding);
});
exports.evaluate = evaluate;



const load = iced(function load(loader, module) {
  let { sandboxes, globals } = loader;
  let require = Require(loader, module);

  let sandbox = sandboxes[module.uri] = Sandbox({
    name: module.uri,
    
    
    sandbox: sandboxes[keys(sandboxes).shift()],
    
    
    
    prototype: create(globals, descriptor({
      require: require,
      module: module,
      exports: module.exports
    })),
    wantXrays: false
  });

  try {
    evaluate(sandbox, module.uri);
  } catch (error) {
    let { message, fileName, lineNumber } = error;
    let stack = error.stack || Error().stack;
    let frames = parseStack(stack).filter(isntLoaderFrame);
    let toString = String(error);

    
    
    
    
    if (String(error) === "Error opening input stream (invalid filename?)") {
      let caller = frames.slice(0).pop();
      fileName = caller.fileName;
      lineNumber = caller.lineNumber;
      message = "Module `" + module.id + "` is not found at " + module.uri;
      toString = message;
    }

    let prototype = typeof(error) === "object" ? error.constructor.prototype :
                    Error.prototype;

    throw create(prototype, {
      message: { value: message, writable: true, configurable: true },
      fileName: { value: fileName, writable: true, configurable: true },
      lineNumber: { value: lineNumber, writable: true, configurable: true },
      stack: { value: serializeStack(frames), writable: true, configurable: true },
      toString: { value: function() toString, writable: true, configurable: true },
    });
  }

  if (module.exports && typeof(module.exports) === 'object')
    freeze(module.exports);

  return module;
});
exports.load = load;


function isRelative(id) { return id[0] === '.'; }

function normalize(uri) { return uri.substr(-3) === '.js' ? uri : uri + '.js'; }



const resolve = iced(function resolve(id, base) {
  if (!isRelative(id)) return id;
  let paths = id.split('/');
  let result = base.split('/');
  result.pop();
  while (paths.length) {
    let path = paths.shift();
    if (path === '..')
      result.pop();
    else if (path !== '.')
      result.push(path);
  }
  return result.join('/');
});
exports.resolve = resolve;

const resolveURI = iced(function resolveURI(id, mapping) {
  let count = mapping.length, index = 0;
  while (index < count) {
    let [ path, uri ] = mapping[index ++];
    if (id.indexOf(path) === 0)
      return normalize(id.replace(path, uri));
  }
});
exports.resolveURI = resolveURI;





const Require = iced(function Require(loader, requirer) {
  let { modules, mapping, resolve, load } = loader;

  function require(id) {
    if (!id) 
      throw Error('you must provide a module name when calling require() from '
                  + requirer.id, requirer.uri);

    
    let requirement = requirer ? resolve(id, requirer.id) : id;

    
    let uri = resolveURI(requirement, mapping);

    if (!uri) 
      throw Error('Module: Can not resolve "' + id + '" module required by ' +
                  requirer.id + ' located at ' + requirer.uri, requirer.uri);

    let module = null;
    
    if (uri in modules) {
      module = modules[uri];
    }
    
    
    else {
      module = modules[uri] = Module(requirement, uri);
      freeze(load(loader, module));
    }

    return module.exports;
  }
  
  require.main = loader.main === requirer ? requirer : undefined;
  return iced(require);
});
exports.Require = Require;

const main = iced(function main(loader, id) {
  let uri = resolveURI(id, loader.mapping)
  let module = loader.main = loader.modules[uri] = Module(id, uri);
  return load(loader, module).exports;
});
exports.main = main;



const Module = iced(function Module(id, uri) {
  return create(null, {
    id: { enumerable: true, value: id },
    exports: { enumerable: true, writable: true, value: create(null) },
    uri: { value: uri }
  });
});
exports.Module = Module;



const unload = iced(function unload(loader, reason) {
  
  
  
  
  
  
  
  let subject = { wrappedJSObject: loader.destructor };
  notifyObservers(subject, 'sdk:loader:destroy', reason);
});
exports.unload = unload;














const Loader = iced(function Loader(options) {
  let { modules, globals, resolve, paths } = override({
    paths: {},
    modules: {},
    globals: {},
    resolve: exports.resolve
  }, options);

  
  
  
  
  
  let destructor = freeze(create(null));

  
  
  let mapping = keys(paths).
    sort(function(a, b) { return b.length - a.length }).
    map(function(path) { return [ path, paths[path] ] });

  
  modules = override({
    '@loader/unload': destructor,
    '@loader/options': options,
    'chrome': { Cc: Cc, Ci: Ci, Cu: Cu, Cr: Cr, Cm: Cm,
                CC: bind(CC, Components), components: Components,
                
                
                ChromeWorker: ChromeWorker
    }
  }, modules);

  modules = keys(modules).reduce(function(result, id) {
    
    let uri = resolveURI(id, mapping);
    let module = Module(id, uri);
    module.exports = freeze(modules[id]);
    result[uri] = freeze(module);
    return result;
  }, {});

  
  
  
  return freeze(create(null, {
    destructor: { enumerable: false, value: destructor },
    globals: { enumerable: false, value: globals },
    mapping: { enumerable: false, value: mapping },
    
    modules: { enumerable: false, value: modules },
    
    sandboxes: { enumerable: false, value: {} },
    resolve: { enumerable: false, value: resolve },
    load: { enumerable: false, value: options.load || load },
    
    
    main: new function() {
      let main;
      return {
        enumerable: false,
        get: function() { return main; },
        
        set: function(module) { main = main || module; }
      }
    }
  }));
});
exports.Loader = Loader;

});

