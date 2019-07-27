



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const Services = require("Services");
const gcli = require("gcli/index");
const { XPCOMUtils } = require("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DevToolsLoader",
  "resource://gre/modules/devtools/Loader.jsm");

const BRAND_SHORT_NAME = Cc["@mozilla.org/intl/stringbundle;1"]
                           .getService(Ci.nsIStringBundleService)
                           .createBundle("chrome://branding/locale/brand.properties")
                           .GetStringFromName("brandShortName");

XPCOMUtils.defineLazyGetter(this, "debuggerServer", () => {
  
  
  
  
  
  
  let serverLoader = new DevToolsLoader();
  serverLoader.invisibleToDebugger = true;
  serverLoader.main("devtools/server/main");
  let debuggerServer = serverLoader.DebuggerServer;
  debuggerServer.init();
  debuggerServer.addBrowserActors();
  return debuggerServer;
});

exports.items = [
  {
    name: "listen",
    description: gcli.lookup("listenDesc"),
    manual: gcli.lookupFormat("listenManual2", [ BRAND_SHORT_NAME ]),
    params: [
      {
        name: "port",
        type: "number",
        get defaultValue() {
          return Services.prefs.getIntPref("devtools.debugger.chrome-debugging-port");
        },
        description: gcli.lookup("listenPortDesc"),
      }
    ],
    exec: function(args, context) {
      var reply = debuggerServer.openListener(args.port);
      if (!reply) {
        throw new Error(gcli.lookup("listenDisabledOutput"));
      }

      if (debuggerServer.initialized) {
        return gcli.lookupFormat("listenInitOutput", [ "" + args.port ]);
      }

      return gcli.lookup("listenNoInitOutput");
    },
  }
];
