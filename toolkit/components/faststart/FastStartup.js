





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const Timer = Components.Constructor("@mozilla.org/timer;1", "nsITimer", "initWithCallback");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const RESTART_ENV_VAR = "FASTSTART_INITIATED_RESTART";

function setenv(key, value) {
  let env = Cc["@mozilla.org/process/environment;1"].
            getService(Components.interfaces.nsIEnvironment);
  env.set(key, value);
}

function getenv(key) {
  let env = Cc["@mozilla.org/process/environment;1"].
            getService(Components.interfaces.nsIEnvironment);
  return env.get(key);
}

function nsFastStartupObserver() {
  let _browserWindowCount = 0;
  let _memCleanupTimer;
  let _restartTimer;
  let _isShuttingDown;

  function stopMemoryCleanup() {
    if (_memCleanupTimer) {
      _memCleanupTimer.cancel();
      _memCleanupTimer = null;
    }

    if (_restartTimer) {
      _restartTimer.cancel();
      _restartTimer = null;
    }
  }

  function scheduleMemoryCleanup() {
    if (_isShuttingDown)
      return;

    stopMemoryCleanup();

    function restart() {
      setenv(RESTART_ENV_VAR, "1");
      let appstartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                       getService(Ci.nsIAppStartup);
      appstartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
      
      _restartTimer = null;
    }

    function memoryCleanup() {
      var os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);

      
      
      os.notifyObservers(null, "memory-pressure", "heap-minimize");
      os.notifyObservers(null, "memory-pressure", "heap-minimize");
      os.notifyObservers(null, "memory-pressure", "heap-minimize");

      _memCleanupTimer = null; 
    }

    
    
    _memCleanupTimer = new Timer(memoryCleanup, 30000, Ci.nsITimer.TYPE_ONE_SHOT);

    
    _restartTimer = new Timer(restart, 60000 * 15, Ci.nsITimer.TYPE_ONE_SHOT);
  }

  
  
  
  this.observe = function observe(subject, topic, data) {
    var win = subject;

    
    
    
    if (topic == "domwindowopened") {
      stopMemoryCleanup();
      _browserWindowCount++;
    } else if (topic == "domwindowclosed") {
      if (_browserWindowCount > 0)
        _browserWindowCount--;
      if (_browserWindowCount == 0)
        scheduleMemoryCleanup();
    } else if (topic == "quit-application-granted") {
      stopMemoryCleanup();
      _isShuttingDown = true;
      let appstartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                       getService(Ci.nsIAppStartup);
      appstartup.exitLastWindowClosingSurvivalArea();
    }
  }

  




  this.QueryInterface = XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]);
}

function nsFastStartupCLH() { }

nsFastStartupCLH.prototype = {
  
  
  
  handle: function fs_handle(cmdLine) {
    
    if (cmdLine.handleFlag("faststart-hidden", false) || (getenv(RESTART_ENV_VAR) == "1"))
      cmdLine.preventDefault = true;

    try {
      
      
      if (this.inited)
        return;

      this.inited = true;

      
      setenv(RESTART_ENV_VAR, "0");

      let fsobs = new nsFastStartupObserver();
      let wwatch = Cc["@mozilla.org/embedcomp/window-watcher;1"].
                   getService(Ci.nsIWindowWatcher);
      wwatch.registerNotification(fsobs);

      let obsService = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
      obsService.addObserver(fsobs, "quit-application-granted", true);

      let appstartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                       getService(Ci.nsIAppStartup);
      appstartup.enterLastWindowClosingSurvivalArea();
    } catch (e) {
      Cu.reportError(e);
    }
  },

  helpInfo: "    -faststart-hidden\n",

  
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
