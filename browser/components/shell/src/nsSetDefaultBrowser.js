









































const Cc = Components.classes;
const Ci = Components.interfaces;
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function nsSetDefaultBrowser() {}

nsSetDefaultBrowser.prototype = {
  handle: function nsSetDefault_handle(aCmdline) {
    if (aCmdline.handleFlag("setDefaultBrowser", false)) {
      var shell = Cc["@mozilla.org/browser/shell-service;1"].
                  getService(Ci.nsIShellService);
      shell.setDefaultBrowser(true, true);
    }
  },

  helpInfo: "  -setDefaultBrowser Set this app as the default browser.\n",

  classDescription: "Default Browser Cmdline Handler",
  contractID: "@mozilla.org/browser/default-browser-clh;1",
  classID: Components.ID("{F57899D0-4E2C-4ac6-9E29-50C736103B0C}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),
  _xpcom_categories: [{
    category: "command-line-handler",
    entry: "m-setdefaultbrowser"
  }]
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([nsSetDefaultBrowser]);
}
