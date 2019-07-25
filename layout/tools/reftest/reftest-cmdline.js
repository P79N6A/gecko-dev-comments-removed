





































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const nsISupports                    = Components.interfaces.nsISupports;
  
const nsICommandLine                 = Components.interfaces.nsICommandLine;
const nsICommandLineHandler          = Components.interfaces.nsICommandLineHandler;
const nsISupportsString              = Components.interfaces.nsISupportsString;
const nsIWindowWatcher               = Components.interfaces.nsIWindowWatcher;

function RefTestCmdLineHandler() {}
RefTestCmdLineHandler.prototype =
{
  classID: Components.ID('{32530271-8c1b-4b7d-a812-218e42c6bb23}'),

  
  QueryInterface: XPCOMUtils.generateQI([nsICommandLineHandler]),

  
  handle : function handler_handle(cmdLine) {
    var args = { };
    args.wrappedJSObject = args;
    try {
      var uristr = cmdLine.handleFlagWithParam("reftest", false);
      if (uristr == null)
        return;
      try {
        args.uri = cmdLine.resolveURI(uristr).spec;
      }
      catch (e) {
        return;
      }
    }
    catch (e) {
      cmdLine.handleFlag("reftest", true);
    }

    try {
      var nocache = cmdLine.handleFlag("reftestnocache", false);
      args.nocache = nocache;
    }
    catch (e) {
    }

    
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService2);
    ios.manageOfflineStatus = false;
    ios.offline = false;

    

    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefBranch2);
    prefs.setBoolPref("gfx.color_management.force_srgb", true);

    var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                           .getService(nsIWindowWatcher);
    wwatch.openWindow(null, "chrome://reftest/content/reftest.xul", "_blank",
                      "chrome,dialog=no,all", args);
    cmdLine.preventDefault = true;
  },

  helpInfo : "  -reftest <file>    Run layout acceptance tests on given manifest.\n"
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([RefTestCmdLineHandler]);
