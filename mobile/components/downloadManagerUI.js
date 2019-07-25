



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function DownloadManagerUI() { }

DownloadManagerUI.prototype = {
  classDescription: "Download Manager UI",
  contractID: "@mozilla.org/download-manager-ui;1",
  classID: Components.ID("{93db15b1-b408-453e-9a2b-6619e168324a}"),

  show: function show(aWindowContext, aID, aReason) {
    if (!aReason)
      aReason = Ci.nsIDownloadManagerUI.REASON_USER_INTERACTED;

    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    let browser = wm.getMostRecentWindow("navigator:browser");
    if (browser)
      browser.showDownloadManager(aWindowContext, aID, aReason);
  },

  get visible() {
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    let browser = wm.getMostRecentWindow("navigator:browser");
    if (browser) {
      return browser.DownloadsView.visible;
    }
    return false;
  },

  getAttention: function getAttention() {
    if (this.visible)
      this.show(null, null, null);
    else
      throw Cr.NS_ERROR_UNEXPECTED;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadManagerUI])
};

function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([DownloadManagerUI]);
}
