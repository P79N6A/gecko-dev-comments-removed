



"use strict";












const EXPORTED_SYMBOLS = [ "define", "require" ];

const console = (function() {
  const tempScope = {};
  Components.utils.import("resource:///modules/devtools/Console.jsm", tempScope);
  return tempScope.console;
})();







function define(moduleName, deps, payload) {
  if (typeof moduleName != "string") {
    console.error(this.depth + " Error: Module name is not a string.");
    console.trace();
    return;
  }

  if (arguments.length == 2) {
    payload = deps;
  }
  else {
    payload.deps = deps;
  }

  if (define.debugDependencies) {
    console.log("define: " + moduleName + " -> " + payload.toString()
        .slice(0, 40).replace(/\n/, '\\n').replace(/\r/, '\\r') + "...");
  }

  if (moduleName in define.modules) {
    console.error(this.depth + " Error: Redefining module: " + moduleName);
  }
  define.modules[moduleName] = payload;
}




define.modules = {};




define.debugDependencies = false;





var Syntax = {
  COMMON_JS: 'commonjs',
  AMD: 'amd'
};









function Domain() {
  this.modules = {};
  this.syntax = Syntax.COMMON_JS;

  if (define.debugDependencies) {
    this.depth = "";
  }
}














Domain.prototype.require = function(config, deps, callback) {
  if (arguments.length <= 2) {
    callback = deps;
    deps = config;
    config = undefined;
  }

  if (Array.isArray(deps)) {
    this.syntax = Syntax.AMD;
    var params = deps.map(function(dep) {
      return this.lookup(dep);
    }, this);
    if (callback) {
      callback.apply(null, params);
    }
    return undefined;
  }
  else {
    return this.lookup(deps);
  }
};







Domain.prototype.lookup = function(moduleName) {
  if (moduleName in this.modules) {
    var module = this.modules[moduleName];
    if (define.debugDependencies) {
      console.log(this.depth + " Using module: " + moduleName);
    }
    return module;
  }

  if (!(moduleName in define.modules)) {
    console.error(this.depth + " Missing module: " + moduleName);
    return null;
  }

  var module = define.modules[moduleName];

  if (define.debugDependencies) {
    console.log(this.depth + " Compiling module: " + moduleName);
  }

  if (typeof module == "function") {
    if (define.debugDependencies) {
      this.depth += ".";
    }

    var exports;
    try {
      if (this.syntax === Syntax.COMMON_JS) {
        exports = {};
        module(this.require.bind(this), exports, { id: moduleName, uri: "" });
      }
      else {
        var modules = module.deps.map(function(dep) {
          return this.lookup(dep);
        }.bind(this));
        exports = module.apply(null, modules);
      }
    }
    catch (ex) {
      console.error("Error using module: " + moduleName, ex);
      throw ex;
    }
    module = exports;

    if (define.debugDependencies) {
      this.depth = this.depth.slice(0, -1);
    }
  }

  
  this.modules[moduleName] = module;

  return module;
};






define.Domain = Domain;
define.globalDomain = new Domain();





const require = define.globalDomain.require.bind(define.globalDomain);
