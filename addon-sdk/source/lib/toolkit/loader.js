



;((factory) => { 
  if (typeof(require) === 'function') { 
    require("chrome").Cu.import(module.uri, exports);
  }
  else if (~String(this).indexOf('BackstagePass')) { 
    let module = { uri: __URI__, id: "toolkit/loader", exports: Object.create(null) }
    factory(module);
    Object.assign(this, module.exports);
    this.EXPORTED_SYMBOLS = Object.getOwnPropertyNames(module.exports);
  }
  else {
    throw Error("Loading environment is not supported");
  }
})(module => {

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
const { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});
const { NetUtil } = Cu.import("resource://gre/modules/NetUtil.jsm", {});
const { join: pathJoin, normalize, dirname } = Cu.import("resource://gre/modules/osfile/ospath_unix.jsm");

XPCOMUtils.defineLazyGetter(this, "XulApp", () => {
  let xulappURI = module.uri.replace("toolkit/loader.js",
                                       "sdk/system/xul-app.jsm");
  return Cu.import(xulappURI, {});
});


const bind = Function.call.bind(Function.bind);
const getOwnPropertyDescriptor = Object.getOwnPropertyDescriptor;
const define = Object.defineProperties;
const prototypeOf = Object.getPrototypeOf;
const create = Object.create;
const keys = Object.keys;
const getOwnIdentifiers = x => [...Object.getOwnPropertyNames(x),
                                ...Object.getOwnPropertySymbols(x)];

const NODE_MODULES = ["assert", "buffer_ieee754", "buffer", "child_process", "cluster", "console", "constants", "crypto", "_debugger", "dgram", "dns", "domain", "events", "freelist", "fs", "http", "https", "_linklist", "module", "net", "os", "path", "punycode", "querystring", "readline", "repl", "stream", "string_decoder", "sys", "timers", "tls", "tty", "url", "util", "vm", "zlib"];

const COMPONENT_ERROR = '`Components` is not available in this context.\n' +
  'Functionality provided by Components may be available in an SDK\n' +
  'module: https://developer.mozilla.org/en-US/Add-ons/SDK \n\n' +
  'However, if you still need to import Components, you may use the\n' +
  '`chrome` module\'s properties for shortcuts to Component properties:\n\n' +
  'Shortcuts: \n' +
  '    Cc = Components' + '.classes \n' +
  '    Ci = Components' + '.interfaces \n' +
  '    Cu = Components' + '.utils \n' +
  '    CC = Components' + '.Constructor \n' +
  'Example: \n' +
  '    let { Cc, Ci } = require(\'chrome\');\n';



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
  getOwnIdentifiers(object).forEach(function(name) {
    value[name] = getOwnPropertyDescriptor(object, name)
  });
  return value;
});
Loader.descriptor = descriptor;



freeze(Object);
freeze(Object.prototype);
freeze(Function);
freeze(Function.prototype);
freeze(Array);
freeze(Array.prototype);
freeze(String);
freeze(String.prototype);





function iced(f) {
  if (!Object.isFrozen(f)) {
    f.prototype = undefined;
  }
  return freeze(f);
}






const override = iced(function override(target, source) {
  let properties = descriptor(target)
  let extension = descriptor(source || {})
  getOwnIdentifiers(extension).forEach(function(name) {
    properties[name] = extension[name];
  });
  return define({}, properties);
});
Loader.override = override;

function sourceURI(uri) { return String(uri).split(" -> ").pop(); }
Loader.sourceURI = iced(sourceURI);

function isntLoaderFrame(frame) { return frame.fileName !== module.uri }

function parseURI(uri) { return String(uri).split(" -> ").pop(); }
Loader.parseURI = parseURI;

function parseStack(stack) {
  let lines = String(stack).split("\n");
  return lines.reduce(function(frames, line) {
    if (line) {
      let atIndex = line.indexOf("@");
      let columnIndex = line.lastIndexOf(":");
      let lineIndex = line.lastIndexOf(":", columnIndex - 1);
      let fileName = parseURI(line.slice(atIndex + 1, lineIndex));
      let lineNumber = parseInt(line.slice(lineIndex + 1, columnIndex));
      let columnNumber = parseInt(line.slice(columnIndex + 1));
      let name = line.slice(0, atIndex).split("(").shift();
      frames.unshift({
        fileName: fileName,
        name: name,
        lineNumber: lineNumber,
        columnNumber: columnNumber
      });
    }
    return frames;
  }, []);
}
Loader.parseStack = parseStack;

function serializeStack(frames) {
  return frames.reduce(function(stack, frame) {
    return frame.name + "@" +
           frame.fileName + ":" +
           frame.lineNumber + ":" +
           frame.columnNumber + "\n" +
           stack;
  }, "");
}
Loader.serializeStack = serializeStack;

function readURI(uri) {
  let nsURI = NetUtil.newURI(uri);
  if (nsURI.scheme == "resource") {
    
    
    let proto = Cc["@mozilla.org/network/protocol;1?name=resource"].
                getService(Ci.nsIResProtocolHandler);
    uri = proto.resolveURI(nsURI);
  }

  let stream = NetUtil.newChannel({
    uri: NetUtil.newURI(uri, 'UTF-8'),
    loadUsingSystemPrincipal: true}
  ).open();
  let count = stream.available();
  let data = NetUtil.readInputStreamToString(stream, count, {
    charset: 'UTF-8'
  });

  stream.close();

  return data;
}


function join (...paths) {
  let resolved = normalize(pathJoin(...paths))
  
  
  
  resolved = resolved.replace(/^resource\:\/([^\/])/, 'resource://$1');
  resolved = resolved.replace(/^file\:\/([^\/])/, 'file:///$1');
  resolved = resolved.replace(/^chrome\:\/([^\/])/, 'chrome://$1');
  return resolved;
}
Loader.join = join;


















const Sandbox = iced(function Sandbox(options) {
  
  options = {
    
    
    wantComponents: false,
    sandboxName: options.name,
    principal: 'principal' in options ? options.principal : systemPrincipal,
    wantXrays: 'wantXrays' in options ? options.wantXrays : true,
    wantGlobalProperties: 'wantGlobalProperties' in options ?
                          options.wantGlobalProperties : [],
    sandboxPrototype: 'prototype' in options ? options.prototype : {},
    invisibleToDebugger: 'invisibleToDebugger' in options ?
                         options.invisibleToDebugger : false,
    metadata: 'metadata' in options ? options.metadata : {}
  };

  if (options.metadata && options.metadata.addonID) {
    options.addonId = options.metadata.addonID;
  }

  let sandbox = Cu.Sandbox(options.principal, options);

  
  
  
  delete sandbox.Iterator;
  delete sandbox.Components;
  delete sandbox.importFunction;
  delete sandbox.debug;

  return sandbox;
});
Loader.Sandbox = Sandbox;








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
Loader.evaluate = evaluate;



const load = iced(function load(loader, module) {
  let { sandboxes, globals } = loader;
  let require = Require(loader, module);

  
  
  
  let descriptors = descriptor({
    require: require,
    module: module,
    exports: module.exports,
    get Components() {
      
      
      throw new ReferenceError(COMPONENT_ERROR);
    }
  });

  let sandbox;
  if (loader.sharedGlobalSandbox &&
      loader.sharedGlobalBlacklist.indexOf(module.id) == -1) {
    
    
    sandbox = new loader.sharedGlobalSandbox.Object();
    
    getOwnIdentifiers(globals).forEach(function(name) {
      descriptors[name] = getOwnPropertyDescriptor(globals, name)
    });
    define(sandbox, descriptors);
  }
  else {
    sandbox = Sandbox({
      name: module.uri,
      prototype: create(globals, descriptors),
      wantXrays: false,
      wantGlobalProperties: module.id == "sdk/indexed-db" ? ["indexedDB"] : [],
      invisibleToDebugger: loader.invisibleToDebugger,
      metadata: {
        addonID: loader.id,
        URI: module.uri
      }
    });
  }
  sandboxes[module.uri] = sandbox;

  try {
    evaluate(sandbox, module.uri);
  }
  catch (error) {
    let { message, fileName, lineNumber } = error;
    let stack = error.stack || Error().stack;
    let frames = parseStack(stack).filter(isntLoaderFrame);
    let toString = String(error);
    let file = sourceURI(fileName);

    
    
    
    
    if (/^Error opening input stream/.test(String(error))) {
      let caller = frames.slice(0).pop();
      fileName = caller.fileName;
      lineNumber = caller.lineNumber;
      message = "Module `" + module.id + "` is not found at " + module.uri;
      toString = message;
    }
    
    
    
    
    else if (frames[frames.length - 1].fileName !== file) {
      frames.push({ fileName: file, lineNumber: lineNumber, name: "" });
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

  if (loader.checkCompatibility) {
    let err = XulApp.incompatibility(module);
    if (err) {
      throw err;
    }
  }

  if (module.exports && typeof(module.exports) === 'object')
    freeze(module.exports);

  return module;
});
Loader.load = load;


function normalizeExt (uri) {
  return isJSURI(uri) ? uri :
         isJSONURI(uri) ? uri :
         isJSMURI(uri) ? uri :
         uri + '.js';
}



function stripBase (rootURI, string) {
  return string.replace(rootURI, './');
}




const resolve = iced(function resolve(id, base) {
  if (!isRelative(id)) return id;
  let basePaths = base.split('/');
  
  
  
  basePaths.pop();
  if (!basePaths.length)
    return normalize(id);
  let resolved = join(basePaths.join('/'), id);

  
  
  if (isRelative(base))
    resolved = './' + resolved;

  return resolved;
});
Loader.resolve = resolve;






const nodeResolve = iced(function nodeResolve(id, requirer, { rootURI }) {
  
  id = Loader.resolve(id, requirer);

  
  if (isAbsoluteURI(id))
    return void 0;

  
  
  let fullId = join(rootURI, id);
  let resolvedPath;

  if ((resolvedPath = loadAsFile(fullId)))
    return stripBase(rootURI, resolvedPath);

  if ((resolvedPath = loadAsDirectory(fullId)))
    return stripBase(rootURI, resolvedPath);

  
  
  if (isAbsoluteURI(requirer))
    return void 0;

  
  
  let dirs = getNodeModulePaths(dirname(requirer)).map(dir => join(rootURI, dir, id));
  for (let i = 0; i < dirs.length; i++) {
    if ((resolvedPath = loadAsFile(dirs[i])))
      return stripBase(rootURI, resolvedPath);

    if ((resolvedPath = loadAsDirectory(dirs[i])))
      return stripBase(rootURI, resolvedPath);
  }

  
  
  
  return void 0;
});
Loader.nodeResolve = nodeResolve;



function loadAsFile (path) {
  let found;

  
  
  
  
  try {
    
    path = normalizeExt(path);
    readURI(path);
    found = path;
  } catch (e) {}

  return found;
}



function loadAsDirectory (path) {
  try {
    
    
    let main = getManifestMain(JSON.parse(readURI(path + '/package.json')));
    if (main != null) {
      let tmpPath = join(path, main);
      let found = loadAsFile(tmpPath);
      if (found)
        return found
    }
    try {
      let tmpPath = path + '/index.js';
      readURI(tmpPath);
      return tmpPath;
    } catch (e) {}
  } catch (e) {
    try {
      let tmpPath = path + '/index.js';
      readURI(tmpPath);
      return tmpPath;
    } catch (e) {}
  }
  return void 0;
}



function getNodeModulePaths (start) {
  
  let moduleDir = 'node_modules';

  let parts = start.split('/');
  let dirs = [];
  for (let i = parts.length - 1; i >= 0; i--) {
    if (parts[i] === moduleDir) continue;
    let dir = join(parts.slice(0, i + 1).join('/'), moduleDir);
    dirs.push(dir);
  }
  dirs.push(moduleDir);
  return dirs;
}


function addTrailingSlash (path) {
  return !path ? null : !path.endsWith('/') ? path + '/' : path;
}



function isNodeModule (name) {
  return !!~NODE_MODULES.indexOf(name);
}



function sortPaths (paths) {
  return keys(paths).
    sort((a, b) => (b.length - a.length)).
    map((path) => [ path, paths[path] ]);
}

const resolveURI = iced(function resolveURI(id, mapping) {
  let count = mapping.length, index = 0;

  
  if (isAbsoluteURI(id)) return normalizeExt(id);

  while (index < count) {
    let [ path, uri ] = mapping[index++];
    if (id.indexOf(path) === 0)
      return normalizeExt(id.replace(path, uri));
  }
  return void 0; 
});
Loader.resolveURI = resolveURI;





const Require = iced(function Require(loader, requirer) {
  let {
    modules, mapping, resolve: loaderResolve, load,
    manifest, rootURI, isNative, requireMap
  } = loader;

  function require(id) {
    if (!id) 
      throw Error('You must provide a module name when calling require() from '
                  + requirer.id, requirer.uri);

    let { uri, requirement } = getRequirements(id);
    let module = null;
    
    if (uri in modules) {
      module = modules[uri];
    }
    else if (isJSMURI(uri)) {
      module = modules[uri] = Module(requirement, uri);
      module.exports = Cu.import(uri, {});
      freeze(module);
    }
    else if (isJSONURI(uri)) {
      let data;

      
      
      
      
      try {
        data = JSON.parse(readURI(uri));
        module = modules[uri] = Module(requirement, uri);
        module.exports = data;
        freeze(module);
      }
      catch (err) {
        
        
        if (err && /JSON\.parse/.test(err.message))
          throw err;
        uri = uri + '.js';
      }
    }

    
    
    
    if (!(uri in modules)) {
      
      
      
      module = modules[uri] = Module(requirement, uri);
      try {
        freeze(load(loader, module));
      }
      catch (e) {
        
        delete modules[uri];
        
        delete loader.sandboxes[uri];
        throw e;
      }
    }

    return module.exports;
  }

  
  
  
  function getRequirements(id) {
    if (!id) 
      throw Error('you must provide a module name when calling require() from '
                  + requirer.id, requirer.uri);

    let requirement, uri;

    
    
    if (isNative) {
      
      
      
      if (requireMap && requireMap[requirer.id])
        requirement = requireMap[requirer.id][id];

      let { overrides } = manifest.jetpack;
      for (let key in overrides) {
        
        if (/^[\.\/]/.test(key)) {
          continue;
        }

        
        
        
        if (id == key || (id.substr(0, key.length + 1) == (key + "/"))) {
          id = overrides[key] + id.substr(key.length);
          id = id.replace(/^[\.\/]+/, "./");
          if (id.substr(0, 2) == "./") {
            id = "" + id.substr(2);
          }
        }
      }

      
      
      
      if (!requirement && modules[id])
        uri = requirement = id;

      
      
      if (!requirement && !isNodeModule(id)) {
        
        
        
        
        requirement = loaderResolve(id, requirer.id, {
          manifest: manifest,
          rootURI: rootURI
        });
      }

      
      
      
      
      if (!requirement) {
        requirement = isRelative(id) ? Loader.resolve(id, requirer.id) : id;
      }
    }
    else {
      
      requirement = requirer ? loaderResolve(id, requirer.id) : id;
    }

    
    uri = uri || resolveURI(requirement, mapping);

    
    if (!uri) {
      throw Error('Module: Can not resolve "' + id + '" module required by ' +
                  requirer.id + ' located at ' + requirer.uri, requirer.uri);
    }

    return { uri: uri, requirement: requirement };
  }

  
  require.resolve = function resolve(id) {
    let { uri } = getRequirements(id);
    return uri;
  }

  
  require.main = loader.main === requirer ? requirer : undefined;
  return iced(require);
});
Loader.Require = Require;

const main = iced(function main(loader, id) {
  
  
  if (!id && loader.isNative)
    id = getManifestMain(loader.manifest);
  let uri = resolveURI(id, loader.mapping);
  let module = loader.main = loader.modules[uri] = Module(id, uri);
  return loader.load(loader, module).exports;
});
Loader.main = main;



const Module = iced(function Module(id, uri) {
  return create(null, {
    id: { enumerable: true, value: id },
    exports: { enumerable: true, writable: true, value: create(null),
               configurable: true },
    uri: { value: uri }
  });
});
Loader.Module = Module;



const unload = iced(function unload(loader, reason) {
  
  
  
  
  
  
  
  let subject = { wrappedJSObject: loader.destructor };
  notifyObservers(subject, 'sdk:loader:destroy', reason);
});
Loader.unload = unload;














function Loader(options) {
  let {
    modules, globals, resolve, paths, rootURI, manifest, requireMap, isNative,
    metadata, sharedGlobal, sharedGlobalBlacklist, checkCompatibility
  } = override({
    paths: {},
    modules: {},
    globals: {
      get console() {
        
        let { ConsoleAPI } = Cu.import("resource://gre/modules/devtools/Console.jsm");
        let console = new ConsoleAPI({
          consoleID: options.id ? "addon/" + options.id : ""
        });
        Object.defineProperty(this, "console", { value: console });
        return this.console;
      }
    },
    checkCompatibility: false,
    resolve: options.isNative ?
      
      (id, requirer) => Loader.nodeResolve(id, requirer, { rootURI: rootURI }) :
      Loader.resolve,
    sharedGlobalBlacklist: ["sdk/indexed-db"]
  }, options);

  
  if (typeof manifest != "object" || !manifest) {
    manifest = {};
  }
  if (typeof manifest.jetpack != "object" || !manifest.jetpack) {
    manifest.jetpack = {
      overrides: {}
    };
  }
  if (typeof manifest.jetpack.overrides != "object" || !manifest.jetpack.overrides) {
    manifest.jetpack.overrides = {};
  }

  
  
  
  
  
  let destructor = freeze(create(null));

  let mapping = sortPaths(paths);

  
  modules = override({
    '@loader/unload': destructor,
    '@loader/options': options,
    'chrome': { Cc: Cc, Ci: Ci, Cu: Cu, Cr: Cr, Cm: Cm,
                CC: bind(CC, Components), components: Components,
                
                
                ChromeWorker: ChromeWorker
    }
  }, modules);

  const builtinModuleExports = modules;
  modules = keys(modules).reduce(function(result, id) {
    
    let uri = resolveURI(id, mapping);
    
    
    if (isNative && !uri)
      uri = id;
    let module = Module(id, uri);

    
    
    Object.defineProperty(module, "exports", {
      enumerable: true,
      get: function() {
        return builtinModuleExports[id];
      }
    });

    result[uri] = freeze(module);
    return result;
  }, {});

  let sharedGlobalSandbox;
  if (sharedGlobal) {
    
    
    
    
    sharedGlobalSandbox = Sandbox({
      name: "Addon-SDK",
      wantXrays: false,
      wantGlobalProperties: [],
      invisibleToDebugger: options.invisibleToDebugger || false,
      metadata: {
        addonID: options.id,
        URI: "Addon-SDK"
      },
      prototype: options.sandboxPrototype || {}
    });
  }

  
  
  
  let returnObj = {
    destructor: { enumerable: false, value: destructor },
    globals: { enumerable: false, value: globals },
    mapping: { enumerable: false, value: mapping },
    
    modules: { enumerable: false, value: modules },
    metadata: { enumerable: false, value: metadata },
    sharedGlobalSandbox: { enumerable: false, value: sharedGlobalSandbox },
    sharedGlobalBlacklist: { enumerable: false, value: sharedGlobalBlacklist },
    
    sandboxes: { enumerable: false, value: {} },
    resolve: { enumerable: false, value: resolve },
    
    id: { enumerable: false, value: options.id },
    
    invisibleToDebugger: { enumerable: false,
                           value: options.invisibleToDebugger || false },
    load: { enumerable: false, value: options.load || load },
    checkCompatibility: { enumerable: false, value: checkCompatibility },
    
    
    main: new function() {
      let main;
      return {
        enumerable: false,
        get: function() { return main; },
        
        set: function(module) { main = main || module; }
      }
    }
  };

  if (isNative) {
    returnObj.isNative = { enumerable: false, value: true };
    returnObj.manifest = { enumerable: false, value: manifest };
    returnObj.requireMap = { enumerable: false, value: requireMap };
    returnObj.rootURI = { enumerable: false, value: addTrailingSlash(rootURI) };
  }

  return freeze(create(null, returnObj));
};
Loader.Loader = Loader;

let isJSONURI = uri => uri.substr(-5) === '.json';
let isJSMURI = uri => uri.substr(-4) === '.jsm';
let isJSURI = uri => uri.substr(-3) === '.js';
let isAbsoluteURI = uri => uri.indexOf("resource://") >= 0 ||
                           uri.indexOf("chrome://") >= 0 ||
                           uri.indexOf("file://") >= 0
let isRelative = id => id[0] === '.'

const generateMap = iced(function generateMap(options, callback) {
  let { rootURI, resolve, paths } = override({
    paths: {},
    resolve: Loader.nodeResolve
  }, options);

  rootURI = addTrailingSlash(rootURI);

  let manifest;
  let manifestURI = join(rootURI, 'package.json');

  if (rootURI)
    manifest = JSON.parse(readURI(manifestURI));
  else
    throw new Error('No `rootURI` given to generate map');

  let main = getManifestMain(manifest);

  findAllModuleIncludes(main, {
    resolve: resolve,
    manifest: manifest,
    rootURI: rootURI
  }, {}, callback);

});
Loader.generateMap = generateMap;



function getManifestMain (manifest) {
  let main = manifest.main || './index.js';
  return isRelative(main) ? main : './' + main;
}

function findAllModuleIncludes (uri, options, results, callback) {
  let { resolve, manifest, rootURI } = options;
  results = results || {};

  
  if (isJSONURI(uri) || isJSMURI(uri)) {
    callback(results);
    return;
  }

  findModuleIncludes(join(rootURI, uri), modules => {
    
    if (!modules.length) {
      callback(results);
      return;
    }

    results[uri] = modules.reduce((agg, mod) => {
      let resolved = resolve(mod, uri, { manifest: manifest, rootURI: rootURI });

      
      
      if (!resolved)
        return agg;
      agg[mod] = resolved;
      return agg;
    }, {});

    let includes = keys(results[uri]);
    let count = 0;
    let subcallback = () => { if (++count >= includes.length) callback(results) };
    includes.map(id => {
      let moduleURI = results[uri][id];
      if (!results[moduleURI])
        findAllModuleIncludes(moduleURI, options, results, subcallback);
      else
        subcallback();
    });
  });
}






function findModuleIncludes (uri, callback) {
  let src = isAbsoluteURI(uri) ? readURI(uri) : uri;
  let modules = [];

  walk(src, function (node) {
    if (isRequire(node))
      modules.push(node.arguments[0].value);
  });

  callback(modules);
}

function walk (src, callback) {
  
  let { Reflect } = Cu.import("resource://gre/modules/reflect.jsm", {});
  let nodes = Reflect.parse(src);
  traverse(nodes, callback);
}

function traverse (node, cb) {
  if (Array.isArray(node)) {
    node.map(x => {
      if (x != null) {
        x.parent = node;
        traverse(x, cb);
      }
    });
  }
  else if (node && typeof node === 'object') {
    cb(node);
    keys(node).map(key => {
      if (key === 'parent' || !node[key]) return;
      node[key].parent = node;
      traverse(node[key], cb);
    });
  }
}






function isRequire (node) {
  var c = node.callee;
  return c
    && node.type === 'CallExpression'
    && c.type === 'Identifier'
    && c.name === 'require'
    && node.arguments.length
   && node.arguments[0].type === 'Literal';
}

module.exports = iced(Loader);
});
