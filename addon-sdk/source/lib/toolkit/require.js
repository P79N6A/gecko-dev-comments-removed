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

  
  
  
  
  

  const require = id => {
    const requirerURI = components.stack.caller.filename;
    const requirer = Module(requirerURI, requirerURI);
    return Require(loader, requirer)(id);
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
