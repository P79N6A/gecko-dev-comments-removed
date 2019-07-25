




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function BrowserCLH() { }

BrowserCLH.prototype = {
  
  
  
  handle: function fs_handle(cmdLine) {
    
    
    
    
    
    
    if (cmdLine.findFlag("silent", false) > -1) {
      let searchService = Cc["@mozilla.org/browser/search-service;1"].
                          getService(Ci.nsIBrowserSearchService);
    }

    if (cmdLine.state == Ci.nsICommandLine.STATE_INITIAL_LAUNCH)
      return;

    cmdLine.preventDefault = true;

    let win;
    try {
      var windowMediator =
        Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);

      win = windowMediator.getMostRecentWindow("navigator:browser");
      if (!win)
        return;

      win.focus();
    } catch (e) { }

    
    
    for (let i = 0; i < cmdLine.length; i++) {
      let arg = cmdLine.getArgument(i);
      if (!arg || arg[0] == '-')
        continue;

      let uri = cmdLine.resolveURI(arg);
      if (uri)
        win.browserDOMWindow.openURI(uri, null, Ci.nsIBrowserDOMWindow.OPEN_NEWTAB, null);
    }
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),

  
  classDescription: "Command Line Handler",
  contractID: "@mozilla.org/mobile/browser-clh;1",
  classID: Components.ID("{be623d20-d305-11de-8a39-0800200c9a66}"),
  _xpcom_categories: [{ category: "command-line-handler", entry: "m-browser" }],
};

var components = [ BrowserCLH ];

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(components);
}
