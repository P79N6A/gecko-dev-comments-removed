


'use strict';

module.metadata = {
  "stability": "unstable"
};










const { classes: Cc, Constructor: CC, interfaces: Ci, utils: Cu } = Components;

const {
  incompatibility
} = Cu.import("resource://gre/modules/sdk/system/XulApp.js", {}).XulApp;


const loaderURI = module.uri.replace("sdk/loader/cuddlefish.js",
                                     "toolkit/loader.js");



const loaderSandbox = loadSandbox(loaderURI);
const loaderModule = loaderSandbox.exports;

const { override, load } = loaderModule;

function CuddlefishLoader(options) {
  let { manifest } = options;

  options = override(options, {
    
    
    modules: override({
      'toolkit/loader': loaderModule,
      'sdk/loader/cuddlefish': exports
    }, options.modules),
    resolve: function resolve(id, requirer) {
      let entry = requirer && requirer in manifest && manifest[requirer];
      let uri = null;

      
      
      
      if (entry) {
        let requirement = entry.requirements[id];
        
        
        if (!requirement)
          throw Error('Module: ' + requirer + ' has no authority to load: '
                      + id, requirer);

        uri = requirement;
      } else {
        
        
        uri = loaderModule.resolve(id, requirer);
      }
      return uri;
    },
    load: function(loader, module) {
      let result;
      let error;

      
      
      
      
      
      try {
        result = load(loader, module);
      }
      catch (e) {
        error = e;
      }

      error = incompatibility(module) || error;

      if (error)
        throw error;

      return result;
    }
  });

  let loader = loaderModule.Loader(options);
  
  loader.modules[loaderURI] = loaderSandbox;
  return loader;
}

exports = override(loaderModule, {
  Loader: CuddlefishLoader
});
