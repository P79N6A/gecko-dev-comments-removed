



const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");





function DownloadManagerUI() { }

DownloadManagerUI.prototype = {
  classID: Components.ID("{93db15b1-b408-453e-9a2b-6619e168324a}"),

  show: function show(aWindowContext, aDownload, aReason, aUsePrivateUI) {
    if (!aReason)
      aReason = Ci.nsIDownloadManagerUI.REASON_USER_INTERACTED;

    this._getBrowserApp().selectOrOpenTab("about:downloads");
  },

  get visible() {
    let browserApp = this._getBrowserApp();
    let downloadsTab = browserApp.getTabWithURL("about:downloads");
    return (downloadsTab && downloadsTab == browserApp.selectedTab) ? true : false;
  },

  getAttention: function getAttention() {
    if (this._getBrowserApp().getTabWithURL("about:downloads"))
      return;

    throw Cr.NS_ERROR_UNEXPECTED;
  },

  _getBrowserApp: function getBrowserApp() {
    try {
      return Services.wm.getMostRecentWindow("navigator:browser").BrowserApp;
    } catch (e) {
      throw Cr.NS_ERROR_FAILURE;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDownloadManagerUI])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DownloadManagerUI]);
