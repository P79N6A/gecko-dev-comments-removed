



"use strict";

const { Cc, Ci, Cu } = require("chrome");
const Services = require("Services");
const l10n = require("gcli/l10n");
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
    item: "command",
    runAt: "client",
    name: "listen",
    description: l10n.lookup("listenDesc"),
    manual: l10n.lookupFormat("listenManual2", [ BRAND_SHORT_NAME ]),
    params: [
      {
        name: "port",
        type: "number",
        get defaultValue() {
          return Services.prefs.getIntPref("devtools.debugger.chrome-debugging-port");
        },
        description: l10n.lookup("listenPortDesc"),
      }
    ],
    exec: function(args, context) {
      var listener = debuggerServer.createListener();
      if (!listener) {
        throw new Error(l10n.lookup("listenDisabledOutput"));
      }

      listener.portOrPath = args.port;
      listener.open();

      if (debuggerServer.initialized) {
        return l10n.lookupFormat("listenInitOutput", [ "" + args.port ]);
      }

      return l10n.lookup("listenNoInitOutput");
    },
  }
];
