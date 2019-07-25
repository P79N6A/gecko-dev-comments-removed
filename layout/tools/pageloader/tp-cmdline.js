






































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");




const nsISupports               = Components.interfaces.nsISupports;
  
const nsICommandLine            = Components.interfaces.nsICommandLine;
const nsICommandLineHandler     = Components.interfaces.nsICommandLineHandler;
const nsISupportsString         = Components.interfaces.nsISupportsString;
const nsIWindowWatcher          = Components.interfaces.nsIWindowWatcher;

function PageLoaderCmdLineHandler() {}
PageLoaderCmdLineHandler.prototype =
{
  classID: Components.ID('{8AF052F5-8EFE-4359-8266-E16498A82E8B}'),

  
  QueryInterface : XPCOMUtils.generateQI([nsICommandLineHandler]),

  
  handle : function handler_handle(cmdLine) {
    var args = {};
    try {
      var uristr = cmdLine.handleFlagWithParam("tp", false);
      if (uristr == null)
        return;
      try {
        args.manifest = cmdLine.resolveURI(uristr).spec;
      } catch (e) {
        return;
      }

      args.numCycles = cmdLine.handleFlagWithParam("tpcycles", false);
      args.startIndex = cmdLine.handleFlagWithParam("tpstart", false);
      args.endIndex = cmdLine.handleFlagWithParam("tpend", false);
      args.filter = cmdLine.handleFlagWithParam("tpfilter", false);
      args.format = cmdLine.handleFlagWithParam("tpformat", false);
      args.useBrowserChrome = cmdLine.handleFlag("tpchrome", false);
      args.doRender = cmdLine.handleFlag("tprender", false);
      args.width = cmdLine.handleFlagWithParam("tpwidth", false);
      args.height = cmdLine.handleFlagWithParam("tpheight", false);
      args.offline = cmdLine.handleFlag("tpoffline", false);
      args.noisy = cmdLine.handleFlag("tpnoisy", false);
      args.timeout = cmdLine.handleFlagWithParam("tptimeout", false);
      args.noForceCC = cmdLine.handleFlag("tpnoforcecc", false);
    }
    catch (e) {
      return;
    }

    
    args.wrappedJSObject = args;

    var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                           .getService(nsIWindowWatcher);
    wwatch.openWindow(null, "chrome://pageloader/content/pageloader.xul", "_blank",
                      "chrome,dialog=no,all", args);
    cmdLine.preventDefault = true;
  },

  helpInfo :
  "  -tp <file>         Run pageload perf tests on given manifest\n" +
  "  -tpfilter str      Only include pages from manifest that contain str (regexp)\n" +
  "  -tpcycles n        Loop through pages n times\n" +
  "  -tpstart n         Start at index n in the manifest\n" +
  "  -tpend n           End with index n in the manifest\n" +
  "  -tpformat f1,f2,.. Report format(s) to use\n" +
  "  -tpchrome          Test with normal browser chrome\n" +
  "  -tprender          Run render-only benchmark for each page\n" +
  "  -tpwidth width     Width of window\n" +
  "  -tpheight height   Height of window\n" +
  "  -tpoffline         Force offline mode\n" +
  "  -tpnoisy           Dump the name of the last loaded page to console\n" + 
  "  -tptimeout         Max amount of time given for a page to load, quit if exceeded\n" +
  "  -tpnoforcecc       Don't force cycle collection between each pageload\n"

};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([PageLoaderCmdLineHandler]);
