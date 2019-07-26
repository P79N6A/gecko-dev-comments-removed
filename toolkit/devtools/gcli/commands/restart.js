



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const gcli = require("gcli/index");
const Services = require("Services");

const BRAND_SHORT_NAME = Cc["@mozilla.org/intl/stringbundle;1"]
                           .getService(Ci.nsIStringBundleService)
                           .createBundle("chrome://branding/locale/brand.properties")
                           .GetStringFromName("brandShortName");













exports.items = [
  {
    name: "restart",
    description: gcli.lookupFormat("restartBrowserDesc", [ BRAND_SHORT_NAME ]),
    params: [
      {
        name: "nocache",
        type: "boolean",
        description: gcli.lookup("restartBrowserNocacheDesc")
      }
    ],
    returnType: "string",
    exec: function Restart(args, context) {
      let canceled = Cc["@mozilla.org/supports-PRBool;1"]
                      .createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(canceled, "quit-application-requested", "restart");
      if (canceled.data) {
        return gcli.lookup("restartBrowserRequestCancelled");
      }

      
      if (args.nocache) {
        Services.appinfo.invalidateCachesOnRestart();
      }

      
      Cc["@mozilla.org/toolkit/app-startup;1"]
          .getService(Ci.nsIAppStartup)
          .quit(Ci.nsIAppStartup.eAttemptQuit | Ci.nsIAppStartup.eRestart);
      return gcli.lookupFormat("restartBrowserRestarting", [ BRAND_SHORT_NAME ]);
    }
  }
];
