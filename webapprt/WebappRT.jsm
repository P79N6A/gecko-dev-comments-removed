



const EXPORTED_SYMBOLS = ["WebappRT"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyGetter(this, "FileUtils", function() {
  Cu.import("resource://gre/modules/FileUtils.jsm");
  return FileUtils;
});

XPCOMUtils.defineLazyGetter(this, "DOMApplicationRegistry", function() {
  Cu.import("resource://gre/modules/Webapps.jsm");
  return DOMApplicationRegistry;
});


Services.obs.addObserver(function observeCmdLine(subj, topic, data) {
  Services.obs.removeObserver(observeCmdLine, "webapprt-command-line");
  let args = subj.QueryInterface(Ci.nsIPropertyBag2);
  if (!args.hasKey("test-mode"))
    return;
  Services.obs.addObserver(function observeInstall(subj, topic, data) {
    
    let config = JSON.parse(data);
    config.registryDir = Services.dirsvc.get("ProfD", Ci.nsIFile).path;
    DOMApplicationRegistry.confirmInstall(config);
    delete WebappRT.config;
    WebappRT.config = deepFreeze(config);
    Services.obs.notifyObservers(null, "webapprt-test-did-install",
                                 JSON.stringify(config));
  }, "webapps-ask-install", false);
}, "webapprt-command-line", false);

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
