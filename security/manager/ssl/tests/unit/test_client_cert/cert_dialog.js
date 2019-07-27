




const { utils: Cu, interfaces: Ci } = Components;
const { XPCOMUtils } = Cu.import("resource://gre/modules/XPCOMUtils.jsm", {});

function CertDialogService() {}
CertDialogService.prototype = {
  classID: Components.ID("{a70153f2-3590-4317-93e9-73b3e7ffca5d}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICertificateDialogs]),

  getPKCS12FilePassword: function() {
    return true; 
  }
};

let Prompter = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPrompt]),
  alert: function() {} 
};

function WindowWatcherService() {}
WindowWatcherService.prototype = {
  classID: Components.ID("{01ae923c-81bb-45db-b860-d423b0fc4fe1}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIWindowWatcher]),

  getNewPrompter: function() {
    return Prompter;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([
  CertDialogService,
  WindowWatcherService
]);
