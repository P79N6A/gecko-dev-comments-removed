



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const WEBAPP_REGISTRY_DIR = "WebappRegD";
const NS_APP_CHROME_DIR_LIST = "AChromDL";

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://webapprt/modules/WebappRT.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let gInTestMode = false;
Services.obs.addObserver(function observe(subj, topic, data) {
  Services.obs.removeObserver(observe, "webapprt-command-line");
  let args = subj.QueryInterface(Ci.nsIPropertyBag2);
  gInTestMode = args.hasKey("test-mode");
}, "webapprt-command-line", false);

function DirectoryProvider() {}

DirectoryProvider.prototype = {
  classID: Components.ID("{e1799fda-4b2f-4457-b671-e0641d95698d}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider,
                                         Ci.nsIDirectoryServiceProvider2]),

  getFile: function(prop, persistent) {
    if (prop == WEBAPP_REGISTRY_DIR) {
      if (gInTestMode) {
        
        
        
        
        return FileUtils.getDir("ProfD", []);
      }
      let dir = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      dir.initWithPath(WebappRT.config.registryDir);
      return dir;
    }

    
    
    return null;
  },

  getFiles: function(prop, persistent) {
    if (prop == NS_APP_CHROME_DIR_LIST) {
      return {
        _done: false,
        QueryInterface: XPCOMUtils.generateQI([Ci.nsISimpleEnumerator]),
        hasMoreElements: function() {
          return !this._done;
        },
        getNext: function() {
          this._done = true;
          return FileUtils.getDir("AppRegD", ["chrome"], false);
        }
      };
    }

    return null;
  },
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([DirectoryProvider]);
