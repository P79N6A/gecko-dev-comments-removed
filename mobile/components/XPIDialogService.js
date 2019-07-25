



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





function WebInstallPrompt() { }

WebInstallPrompt.prototype = {
  classID: Components.ID("{c1242012-27d8-477e-a0f1-0b098ffc329b}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallPrompt]),

  confirm: function(aWindow, aURL, aInstalls) {
    
    let wm = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
    let browser = wm.getMostRecentWindow("navigator:browser");
    if (browser.ExtensionsView.visible) {
      aInstalls.forEach(function(install) {
        install.install();
      });
      return;
    }
    
    let bundleService = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
    let bundle = bundleService.createBundle("chrome://browser/locale/browser.properties");
    let prompt = Cc["@mozilla.org/embedcomp/prompt-service;1"].getService(Ci.nsIPromptService);

    let flags = prompt.BUTTON_POS_0 * prompt.BUTTON_TITLE_IS_STRING + prompt.BUTTON_POS_1 * prompt.BUTTON_TITLE_CANCEL;
    let title = bundle.GetStringFromName("addonsConfirmInstall.title");
    let button = bundle.GetStringFromName("addonsConfirmInstall.install");

    aInstalls.forEach(function(install) {
      let result = (prompt.confirmEx(aWindow, title, install.name, flags, button, null, null, null, {value: false}) == 0);
      if (result)
        install.install();
      else
        install.cancel();
    });
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([WebInstallPrompt]);
