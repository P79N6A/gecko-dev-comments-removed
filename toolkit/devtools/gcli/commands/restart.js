



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const l10n = require("gcli/l10n");
const Services = require("Services");

const BRAND_SHORT_NAME = Cc["@mozilla.org/intl/stringbundle;1"]
                           .getService(Ci.nsIStringBundleService)
                           .createBundle("chrome://branding/locale/brand.properties")
                           .GetStringFromName("brandShortName");













exports.items = [
  {
    item: "command",
    runAt: "client",
    name: "restart",
    description: l10n.lookupFormat("restartBrowserDesc", [ BRAND_SHORT_NAME ]),
    params: [{
      group: l10n.lookup("restartBrowserGroupOptions"),
      params: [
        {
          name: "nocache",
          type: "boolean",
          description: l10n.lookup("restartBrowserNocacheDesc")
        },
        {
          name: "safemode",
          type: "boolean",
          description: l10n.lookup("restartBrowserSafemodeDesc")
        }
      ]
    }],
    returnType: "string",
    exec: function Restart(args, context) {
      let canceled = Cc["@mozilla.org/supports-PRBool;1"]
                      .createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(canceled, "quit-application-requested", "restart");
      if (canceled.data) {
        return l10n.lookup("restartBrowserRequestCancelled");
      }

      
      if (args.nocache) {
        Services.appinfo.invalidateCachesOnRestart();
      }

      const appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                           .getService(Ci.nsIAppStartup);

      if (args.safemode) {
        
        appStartup.restartInSafeMode(Ci.nsIAppStartup.eAttemptQuit);
      } else {
        
        appStartup.quit(Ci.nsIAppStartup.eAttemptQuit | Ci.nsIAppStartup.eRestart);
      }

      return l10n.lookupFormat("restartBrowserRestarting", [ BRAND_SHORT_NAME ]);
    }
  }
];
