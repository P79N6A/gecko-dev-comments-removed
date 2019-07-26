






'use strict';




const { classes: Cc, Constructor: CC, interfaces: Ci, utils: Cu,
        results: Cr, manager: Cm } = Components;
const ioService = Cc['@mozilla.org/network/io-service;1'].
                  getService(Ci.nsIIOService);
const resourceHandler = ioService.getProtocolHandler('resource').
                        QueryInterface(Ci.nsIResProtocolHandler);
const systemPrincipal = CC('@mozilla.org/systemprincipal;1', 'nsIPrincipal')();
const scriptLoader = Cc['@mozilla.org/moz/jssubscript-loader;1'].
                     getService(Ci.mozIJSSubScriptLoader);

const REASON = [ 'unknown', 'startup', 'shutdown', 'enable', 'disable',
                 'install', 'uninstall', 'upgrade', 'downgrade' ];

const bind = Function.call.bind(Function.bind);

let loader = null;
let unload = null;
let cuddlefishSandbox = null;
let nukeTimer = null;



function readURI(uri) {
  let ioservice = Cc['@mozilla.org/network/io-service;1'].
    getService(Ci.nsIIOService);
  let channel = ioservice.newChannel(uri, 'UTF-8', null);
  let stream = channel.open();

  let cstream = Cc['@mozilla.org/intl/converter-input-stream;1'].
    createInstance(Ci.nsIConverterInputStream);
  cstream.init(stream, 'UTF-8', 0, 0);

  let str = {};
  let data = '';
  let read = 0;
  do {
    read = cstream.readString(0xffffffff, str);
    data += str.value;
  } while (read != 0);

  cstream.close();

  return data;
}



function install(data, reason) {}
function uninstall(data, reason) {}

function startup(data, reasonCode) {
  try {
    let reason = REASON[reasonCode];
    
    
    
    let rootURI = data.resourceURI.spec;

    
    
    let options = JSON.parse(readURI(rootURI + './harness-options.json'));

    let id = options.jetpackID;
    let name = options.name;

    
    options.metadata[name]['permissions'] = options.metadata[name]['permissions'] || {};

    
    Object.freeze(options.metadata[name]['permissions']);
    
    Object.freeze(options.metadata[name]);

    
    
    
    
    let uuidRe =
      /^\{([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})\}$/;

    let domain = id.
      toLowerCase().
      replace(/@/g, '-at-').
      replace(/\./g, '-dot-').
      replace(uuidRe, '$1');

    let prefixURI = 'resource://' + domain + '/';
    let resourcesURI = ioService.newURI(rootURI + '/resources/', null, null);
    resourceHandler.setSubstitution(domain, resourcesURI);

    
    let paths = {
      
      './': prefixURI + name + '/lib/',
      './tests/': prefixURI + name + '/tests/',
      '': 'resource://gre/modules/commonjs/'
    };

    
    paths = Object.keys(options.metadata).reduce(function(result, name) {
      result[name + '/'] = prefixURI + name + '/lib/'
      result[name + '/tests/'] = prefixURI + name + '/tests/'
      return result;
    }, paths);

    
    
    if (name == 'addon-sdk')
      paths['tests/'] = prefixURI + name + '/tests/';

    
    paths['sdk/'] = prefixURI + 'addon-sdk/lib/sdk/';
    paths['toolkit/'] = prefixURI + 'addon-sdk/lib/toolkit/';
    
    
    
    paths['test'] = prefixURI + 'addon-sdk/lib/sdk/test.js';

    
    let manifest = options.manifest;

    
    let cuddlefishURI = prefixURI + options.loader;
    cuddlefishSandbox = loadSandbox(cuddlefishURI);
    let cuddlefish = cuddlefishSandbox.exports;

    
    
    let main = options.mainPath;

    unload = cuddlefish.unload;
    loader = cuddlefish.Loader({
      paths: paths,
      
      manifest: manifest,

      
      id: id,
      
      name: name,
      
      version: options.metadata[name].version,
      
      metadata: options.metadata[name],
      
      loadReason: reason,

      prefixURI: prefixURI,
      
      rootURI: rootURI,
      
      
      resultFile: options.resultFile,
      
      logFile: options.logFile,
      
      staticArgs: options.staticArgs,

      
      modules: {
        '@test/options': {
          allTestModules: options.allTestModules,
          iterations: options.iterations,
          filter: options.filter,
          profileMemory: options.profileMemory,
          stopOnError: options.stopOnError,
          verbose: options.verbose,
          parseable: options.parseable,
        }
      }
    });

    let module = cuddlefish.Module('sdk/loader/cuddlefish', cuddlefishURI);
    let require = cuddlefish.Require(loader, module);

    require('sdk/addon/runner').startup(reason, {
      loader: loader,
      main: main,
      prefsURI: rootURI + 'defaults/preferences/prefs.js'
    });
  } catch (error) {
    dump('Bootstrap error: ' +
         (error.message ? error.message : String(error)) + '\n' +
         (error.stack || error.fileName + ': ' + error.lineNumber) + '\n');
    throw error;
  }
};

function loadSandbox(uri) {
  let proto = {
    sandboxPrototype: {
      loadSandbox: loadSandbox,
      ChromeWorker: ChromeWorker
    }
  };
  let sandbox = Cu.Sandbox(systemPrincipal, proto);
  
  
  sandbox.exports = {};
  sandbox.module = { uri: uri, exports: sandbox.exports };
  sandbox.require = function (id) {
    if (id !== "chrome")
      throw new Error("Bootstrap sandbox `require` method isn't implemented.");

    return Object.freeze({ Cc: Cc, Ci: Ci, Cu: Cu, Cr: Cr, Cm: Cm,
      CC: bind(CC, Components), components: Components,
      ChromeWorker: ChromeWorker });
  };
  scriptLoader.loadSubScript(uri, sandbox, 'UTF-8');
  return sandbox;
}

function unloadSandbox(sandbox) {
  if ("nukeSandbox" in Cu)
    Cu.nukeSandbox(sandbox);
}

function setTimeout(callback, delay) {
  let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback({ notify: callback }, delay,
                         Ci.nsITimer.TYPE_ONE_SHOT);
  return timer;
}

function shutdown(data, reasonCode) {
  let reason = REASON[reasonCode];
  if (loader) {
    unload(loader, reason);
    unload = null;

    
    if (reason != "shutdown") {
      
      
      
      
      
      nukeTimer = setTimeout(nukeModules, 1000);
    }
  }
};

function nukeModules() {
  nukeTimer = null;
  
  
  for (let key in loader.modules) {
    delete loader.modules[key];
  }
  
  for (let key in loader.sandboxes) {
    let sandbox = loader.sandboxes[key];
    delete loader.sandboxes[key];
    
    unloadSandbox(sandbox);
  }
  loader = null;

  
  
  

  unloadSandbox(cuddlefishSandbox.loaderSandbox);
  unloadSandbox(cuddlefishSandbox.xulappSandbox);

  
  
  unloadSandbox(cuddlefishSandbox);
  cuddlefishSandbox = null;
}
