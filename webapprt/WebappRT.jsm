



const EXPORTED_SYMBOLS = ["WebappRT"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "FileUtils", function() {
  Cu.import("resource://gre/modules/FileUtils.jsm");
  return FileUtils;
});

let WebappRT = {
  get config() {
    let webappFile = FileUtils.getFile("AppRegD", ["webapp.json"]);
    let inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
    inputStream.init(webappFile, -1, 0, 0);
    let json = Cc["@mozilla.org/dom/json;1"].createInstance(Ci.nsIJSON);
    let config = json.decodeFromStream(inputStream, webappFile.fileSize);

    
    
    
    config = deepFreeze(config);
    delete this.config;
    Object.defineProperty(this, "config", { get: function getConfig() config });
    return this.config;
  }
};

function deepFreeze(o) {
  
  Object.freeze(o);

  
  for (let p in o) {
    
    
    
    
    if (!o.hasOwnProperty(p) || !(typeof o[p] == "object") ||
        Object.isFrozen(o[p]))
      continue;

    deepFreeze(o[p]);
  }

  return o;
}
