



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const WEBAPP_REGISTRY_DIR = "WebappRegD";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/WebappRT.jsm");

function DirectoryProvider() {}

DirectoryProvider.prototype = {
  classID: Components.ID("{e1799fda-4b2f-4457-b671-e0641d95698d}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider]),

  getFile: function(prop, persistent) {
    if (prop == WEBAPP_REGISTRY_DIR) {
      let dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      dir.initWithPath(WebappRT.config.registryDir);
      return dir;
    }

    
    
    return null;
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([DirectoryProvider]);
