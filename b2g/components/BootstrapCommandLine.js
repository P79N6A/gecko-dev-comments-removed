



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

function BootstrapCommandlineHandler() {
  this.wrappedJSObject = this;
  this.startManifestURL = null;
}

BootstrapCommandlineHandler.prototype = {
    bailout: function(aMsg) {
      dump("************************************************************\n");
      dump("* /!\\ " + aMsg + "\n");
      dump("************************************************************\n");
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                         .getService(Ci.nsIAppStartup);
      appStartup.quit(appStartup.eForceQuit);
    },

    handle: function(aCmdLine) {
      this.startManifestURL = null;

      try {
        
        
        
        
        this.startManifestURL = aCmdLine.handleFlagWithParam("start-manifest", false);
      } catch(e) {
        return;
      }

      if (!this.startManifestURL) {
        return;
      }

      if (!isAbsoluteURI(this.startManifestURL)) {
        this.bailout("The start manifest url must be absolute.");
        return;
      }
    },

    helpInfo: "--start-manifest=manifest_url",
    classID: Components.ID("{fd663ec8-cf3f-4c2b-aacb-17a6915ccb44}"),
    QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([BootstrapCommandlineHandler]);
