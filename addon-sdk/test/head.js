



const { utils: Cu } = Components;
const { Services } = Cu.import("resource://gre/modules/Services.jsm", {});
const LoaderModule = Cu.import("resource://gre/modules/commonjs/toolkit/loader.js", {}).Loader;
const { console } = Cu.import("resource://gre/modules/devtools/Console.jsm", {});
let {
  Loader, main, Module, Require, unload
} = LoaderModule;

let CURRENT_DIR = gTestPath.replace(/\/[^\/]*\.js$/,'/');
let loaders = [];


waitForExplicitFinish();

let gEnableLogging = Services.prefs.getBoolPref("devtools.debugger.log");
Services.prefs.setBoolPref("devtools.debugger.log", true);

registerCleanupFunction(() => {
  info("finish() was called, cleaning up...");
  loaders.forEach(unload);
  Services.prefs.setBoolPref("devtools.debugger.log", gEnableLogging);
});

function makePaths (root) {
  return {
    './': CURRENT_DIR,
    '': 'resource://gre/modules/commonjs/'
  };
}

function makeLoader (options) {
  let { paths, globals } = options || {};

  
  
  
  let globalDefaults = {
    console: console
  };

  let loader = Loader({
    paths: paths || makePaths(),
    globals: extend({}, globalDefaults, globals) || null,
    modules: {
      
      
      'toolkit/loader': LoaderModule
    },
    
    
    
    rootURI: CURRENT_DIR,
    
    
    metadata: {}
  });

  loaders.push(loader);
  return loader;
}

function isUUID (string) {
  return /^\{[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}\}$/.test(string);
}

function extend (...objs) {
  if (objs.length === 0 || objs.length === 1)
    return objs[0] || {};

  for (let i = objs.length; i > 1; i--) {
    for (var prop in objs[i - 1])
      objs[0][prop] = objs[i - 1][prop];
  }
  return objs[0];
}
