



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

let EXPORTED_SYMBOLS = [ ];

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/Services.jsm");













gcli.addCommand({
  name: "restart",
  description: gcli.lookup("restartFirefoxDesc"),
  params: [
    {
      name: "nocache",
      type: "boolean",
      description: gcli.lookup("restartFirefoxNocacheDesc")
    }
  ],
  returnType: "string",
  exec: function Restart(args, context) {
    let canceled = Cc["@mozilla.org/supports-PRBool;1"]
                     .createInstance(Ci.nsISupportsPRBool);
    Services.obs.notifyObservers(canceled, "quit-application-requested", "restart");
    if (canceled.data) {
      return gcli.lookup("restartFirefoxRequestCancelled");
    }

    
    if (args.nocache) {
      Services.appinfo.invalidateCachesOnRestart();
    }

    
    Cc['@mozilla.org/toolkit/app-startup;1']
      .getService(Ci.nsIAppStartup)
      .quit(Ci.nsIAppStartup.eAttemptQuit | Ci.nsIAppStartup.eRestart);
    return gcli.lookup("restartFirefoxRestarting");
  }
});
