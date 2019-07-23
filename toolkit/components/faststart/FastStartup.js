 





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const Timer = Components.Constructor("@mozilla.org/timer;1", "nsITimer", "initWithCallback");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function getPreferredBrowserURI() {
  let chromeURI;
  try {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
                getService(Ci.nsIPrefBranch);
    chromeURI = prefs.getCharPref("toolkit.defaultChromeURI");
  } catch (e) { }
  return chromeURI;
}

function nsFastStartupObserver() {
  let _browserWindowCount = 0;
  let _memCleanupTimer = 0;

  function stopMemoryCleanup() {
    if (_memCleanupTimer) {
      _memCleanupTimer.cancel();
      _memCleanupTimer = null;
    }
  }

  function scheduleMemoryCleanup() {
    stopMemoryCleanup();

    function memoryCleanup() {









      
      _memCleanupTimer = null;
    }

    
    
    _memCleanupTimer = new Timer(memoryCleanup, 30000, Ci.nsITimer.TYPE_ONE_SHOT);
  }

  
  
  
  this.observe = function observe(subject, topic, data) {
    var win = subject;

    
    
    
    if (topic == "domwindowopened") {
      var loadListener = function(e) {
        if (win.document.documentURI == getPreferredBrowserURI()) {
          stopMemoryCleanup();
          _browserWindowCount++;
        }
        win.removeEventListener("load", loadListener, false);
        return false;
      }

      win.addEventListener("load", loadListener, false);
    } else if (topic == "domwindowclosed") {
      if (win.document.documentURI == getPreferredBrowserURI()) {
        _browserWindowCount--;
        if (_browserWindowCount == 0)
          scheduleMemoryCleanup();
      }
    }
  }

  
  this.QueryInterface = XPCOMUtils.generateQI([Ci.nsIObserver]);
}

function nsFastStartupCLH() { }

nsFastStartupCLH.prototype = {
  
  
  
  handle: function fs_handle(cmdLine) {
    
    if (!cmdLine.handleFlag("faststart", false))
      return;

    cmdLine.preventDefault = true;

    try {
      
      
      if (this.inited)
        return;

      this.inited = true;

      let wwatch = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                   getService(Ci.nsIWindowWatcher);
      wwatch.registerNotification(new nsFastStartupObserver());

      let appstartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                       getService(Ci.nsIAppStartup);
      appstartup.enterLastWindowClosingSurvivalArea();
    } catch (e) {
      Cu.reportError(e);
    }
  },

  helpInfo: "    -faststart\n",

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),

  
  classDescription: "Fast Startup Component",
  contractID: "@mozilla.org/browser/faststart;1",
  classID: Components.ID("{580c6c51-f690-4ce1-9ecc-b678e0c031c7}"),
  _xpcom_categories: [{ category: "command-line-handler", entry: "00-faststart" }],
};

var components = [ nsFastStartupCLH ];

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(components);
}
