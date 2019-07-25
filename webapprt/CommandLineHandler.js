



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function CommandLineHandler() {}

CommandLineHandler.prototype = {
  classID: Components.ID("{6d69c782-40a3-469b-8bfd-3ee366105a4a}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),

  handle: function handle(cmdLine) {
    let args = Cc["@mozilla.org/hash-property-bag;1"].
               createInstance(Ci.nsIWritablePropertyBag);
    let inTestMode = this._handleTestMode(cmdLine, args);
    Services.obs.notifyObservers(args, "webapprt-command-line", null);

    
    
    
    
    
    Cu.import("resource://gre/modules/Webapps.jsm");

    if (!inTestMode) {
      startUp(inTestMode);
    } else {
      
      
      Services.obs.addObserver(function onInstall(subj, topic, data) {
        Services.obs.removeObserver(onInstall, "webapprt-test-did-install");
        startUp(inTestMode);
      }, "webapprt-test-did-install", false);
    }

    
    Services.ww.openWindow(null,
                           "chrome://webapprt/content/webapp.xul",
                           "_blank",
                           "chrome,dialog=no,resizable,scrollbars,centerscreen",
                           args);
  },

  _handleTestMode: function _handleTestMode(cmdLine, args) {
    
    let idx = cmdLine.findFlag("test-mode", true);
    if (idx < 0)
      return false;
    let url = null;
    let urlIdx = idx + 1;
    if (urlIdx < cmdLine.length) {
      let potentialURL = cmdLine.getArgument(urlIdx);
      if (potentialURL && potentialURL[0] != "-") {
        url = potentialURL;
        try {
          Services.io.newURI(url, null, null);
        } catch (err) {
          throw Components.Exception(
            "-test-mode argument is not a valid URL: " + url,
            Components.results.NS_ERROR_INVALID_ARG);
        }
        cmdLine.removeArguments(urlIdx, urlIdx);
      }
    }
    cmdLine.removeArguments(idx, idx);
    args.setProperty("test-mode", url);
    return true;
  },

  helpInfo : "",
};

let components = [CommandLineHandler];
let NSGetFactory = XPCOMUtils.generateNSGetFactory(components);









function startUp(inTestMode) {
  try {
    if (!inTestMode) {
      
      
      
      
      Cu.import("resource://webapprt/modules/WebappsHandler.jsm");
      WebappsHandler.init();
    }

    
    if (!Services.prefs.getBoolPref("webapprt.firstrun")) {
      Cu.import("resource://webapprt/modules/WebappRT.jsm");
      let uri = Services.io.newURI(WebappRT.config.app.origin, null, null);

      
      Services.perms.add(uri, "pin-app",
                         Ci.nsIPermissionManager.ALLOW_ACTION);
      Services.perms.add(uri, "offline-app",
                         Ci.nsIPermissionManager.ALLOW_ACTION);

      Services.perms.add(uri, "indexedDB",
                         Ci.nsIPermissionManager.ALLOW_ACTION);
      Services.perms.add(uri, "indexedDB-unlimited",
                         Ci.nsIPermissionManager.ALLOW_ACTION);

      
      
      Services.prefs.setBoolPref("webapprt.firstrun", true);
    }
  } catch(ex) {
#ifdef MOZ_DEBUG
    dump(ex + "\n");
#endif
  }
}
