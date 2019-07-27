



const make = (exports, rootURI, components) => {
  const { Loader: { Loader, Require, Module, main } } =
        components.utils.import(rootURI + "toolkit/loader.js", {});

  const loader = Loader({
    id: "toolkit/require",
    rootURI: rootURI,
    isNative: true,
    paths: {
     "": rootURI,
     "devtools/": "resource://gre/modules/devtools/"
    }
  });

  
  
  const unload = uri => {
    delete loader.sandboxes[uri];
    delete loader.modules[uri];
  };

  const builtins = new Set(Object.keys(loader.modules));

  
  
  
  
  

  const require = (id, options={}) => {
    const { reload, all } = options;
    const requirerURI = components.stack.caller.filename;
    const requirer = Module(requirerURI, requirerURI);
    const require = Require(loader, requirer);
    if (reload) {
      
      
      
      
      
      
      
      
      components.classes["@mozilla.org/observer-service;1"].
        getService(components.interfaces.nsIObserverService).
        notifyObservers({}, "startupcache-invalidate", null);

      if (all) {
        for (let uri of Object.keys(loader.sandboxes)) {
          unload(uri);
        }
      }
      else {
        unload(require.resolve(id));
      }
    }
    return require(id);
  };

  require.resolve = id => {
    const requirerURI = components.stack.caller.filename;
    const requirer = Module(requirerURI, requirerURI);
    return Require(loader, requirer).resolve(id);
  };

  exports.require = require;
}



if (typeof(require) === "function" && typeof(module) === "object") {
  require("chrome").Cu.import(module.uri, module.exports);
}


else if (typeof(__URI__) === "string" && this["Components"]) {
  const builtin = Object.keys(this);
  const uri = __URI__.replace("toolkit/require.js", "");
  make(this, uri, this["Components"]);

  this.EXPORTED_SYMBOLS = Object.
                            keys(this).
                            filter($ => builtin.indexOf($) < 0);
}
else {
  throw Error("Loading require.js in this environment isn't supported")
}
