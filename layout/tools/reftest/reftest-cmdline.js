



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

    try {
      var skipslowtests = cmdLine.handleFlag("reftestskipslowtests", false);
      args.skipslowtests = skipslowtests;
    }
    catch (e) {
    }

    
    var ios = Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService2);
    ios.manageOfflineStatus = false;
    ios.offline = false;

    














    var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                getService(Components.interfaces.nsIPrefService);
    var branch = prefs.getDefaultBranch("");
    branch.setBoolPref("dom.use_xbl_scopes_for_remote_xul", true);
    branch.setBoolPref("gfx.color_management.force_srgb", true);
    branch.setBoolPref("browser.dom.window.dump.enabled", true);
    branch.setIntPref("ui.caretBlinkTime", -1);
    branch.setBoolPref("dom.send_after_paint_to_content", true);
    
    branch.setIntPref("dom.max_script_run_time", 0);
    branch.setIntPref("dom.max_chrome_script_run_time", 0);
    branch.setIntPref("hangmonitor.timeout", 0);
    
    branch.setBoolPref("media.autoplay.enabled", true);
    
    branch.setBoolPref("app.update.enabled", false);
    
    branch.setBoolPref("extensions.update.enabled", false);
    branch.setBoolPref("extensions.getAddons.cache.enabled", false);
    
    branch.setBoolPref("extensions.blocklist.enabled", false);
    
    branch.setIntPref("urlclassifier.updateinterval", 172800);
    
    branch.setBoolPref("image.high_quality_downscaling.enabled", false);
    
    
    branch.setBoolPref("security.fileuri.strict_origin_policy", false);

    var wwatch = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                           .getService(nsIWindowWatcher);
    wwatch.openWindow(null, "chrome://reftest/content/reftest.xul", "_blank",
                      "chrome,dialog=no,all", args);
    cmdLine.preventDefault = true;
  },

  helpInfo : "  -reftest <file>    Run layout acceptance tests on given manifest.\n"
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([RefTestCmdLineHandler]);
