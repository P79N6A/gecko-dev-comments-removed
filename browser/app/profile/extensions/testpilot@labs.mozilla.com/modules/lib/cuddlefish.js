



































(function(global) {
   const Cc = Components.classes;
   const Ci = Components.interfaces;
   const Cu = Components.utils;
   const Cr = Components.results;

   var exports = {};

   
   var securableModule;

   if (global.require)
     
     securableModule = require("securable-module");
   else {
     var myURI = Components.stack.filename.split(" -> ").slice(-1)[0];
     var ios = Cc['@mozilla.org/network/io-service;1']
               .getService(Ci.nsIIOService);
     var securableModuleURI = ios.newURI("securable-module.js", null,
                                         ios.newURI(myURI, null, null));
     if (securableModuleURI.scheme == "chrome") {
       
       
       
       var loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                    .getService(Ci.mozIJSSubScriptLoader);

       
       securableModule = {__proto__: global};
       loader.loadSubScript(securableModuleURI.spec, securableModule);
       securableModule = securableModule.SecurableModule;
     } else {
       securableModule = {};
       try {
         Cu.import(securableModuleURI.spec, securableModule);
       } catch (e if e.result == Cr.NS_ERROR_ILLEGAL_VALUE) {
         Cu.reportError("Failed to load " + securableModuleURI.spec);
       }
     }
   }

   function unloadLoader() {
     this.require("unload").send();
   }

   var cuddlefishSandboxFactory = {
     createSandbox: function(options) {
       var filename = options.filename ? options.filename : null;
       var sandbox = this.__proto__.createSandbox(options);
       sandbox.defineProperty("__url__", filename);
       return sandbox;
     },
     __proto__: new securableModule.SandboxFactory("system")
   };

   function CuddlefishModule(loader) {
     this.parentLoader = loader;
     this.__proto__ = exports;
   }

   var Loader = exports.Loader = function Loader(options) {
     var globals = {Cc: Components.classes,
                    Ci: Components.interfaces,
                    Cu: Components.utils,
                    Cr: Components.results};

     if (options.console)
       globals.console = options.console;
     if (options.memory)
       globals.memory = options.memory;

     var modules = {};
     var loaderOptions = {rootPath: options.rootPath,
                          rootPaths: options.rootPaths,
                          fs: options.fs,
                          sandboxFactory: cuddlefishSandboxFactory,
                          globals: globals,
                          modules: modules};

     var loader = new securableModule.Loader(loaderOptions);
     var path = loader.fs.resolveModule(null, "cuddlefish");
     modules[path] = new CuddlefishModule(loader);

     if (!globals.console) {
       var console = loader.require("plain-text-console");
       globals.console = new console.PlainTextConsole(options.print);
     }
     if (!globals.memory)
       globals.memory = loader.require("memory");

     loader.console = globals.console;
     loader.memory = globals.memory;
     loader.unload = unloadLoader;

     return loader;
   };

   if (global.window) {
     
     
     global.Cuddlefish = exports;
   } else if (global.exports) {
     
     for (name in exports) {
       global.exports[name] = exports[name];
     }
   } else {
     
     global.EXPORTED_SYMBOLS = [];
     for (name in exports) {
       global.EXPORTED_SYMBOLS.push(name);
       global[name] = exports[name];
     }
   }
 })(this);
