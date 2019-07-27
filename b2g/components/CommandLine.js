



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services", "resource://gre/modules/Services.jsm");



function CommandlineHandler() {
  this.wrappedJSObject = this;
}

CommandlineHandler.prototype = {
    handle: function(cmdLine) {
      this.cmdLine = cmdLine;
    },

    helpInfo: "",
    classID: Components.ID("{385993fe-8710-4621-9fb1-00a09d8bec37}"),
    QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([CommandlineHandler]);
