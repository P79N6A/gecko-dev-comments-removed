




Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

function HelperAppDlg() { }
HelperAppDlg.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIHelperAppLauncherDialog]),
  classID: Components.ID("49456eda-4dc4-4d1a-b8e8-0b94749bf99e"),
  show: function (launcher, ctx, reason) {
    launcher.MIMEInfo.preferredAction = Ci.nsIMIMEInfo.saveToDisk;
    launcher.launchWithApplication(null, false);
  },

  promptForSaveToFile: function (launcher, ctx, defaultFile, suggestedExtension, forcePrompt) { }
}


this.NSGetFactory = XPCOMUtils.generateNSGetFactory([HelperAppDlg]);
