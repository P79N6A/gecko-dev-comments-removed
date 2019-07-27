



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;
const kDebuggerPrefs = [
  "devtools.debugger.remote-enabled",
  "devtools.debugger.chrome-enabled",
  "devtools.chrome.enabled"
];
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services", "resource://gre/modules/Services.jsm");

function devtoolsCommandlineHandler() {
}
devtoolsCommandlineHandler.prototype = {
  handle: function(cmdLine) {
    let consoleFlag = cmdLine.handleFlag("jsconsole", false);
    let debuggerFlag = cmdLine.handleFlag("jsdebugger", false);
    if (consoleFlag) {
      this.handleConsoleFlag(cmdLine);
    }
    if (debuggerFlag) {
      this.handleDebuggerFlag(cmdLine);
    }
    let debuggerServerFlag;
    try {
      debuggerServerFlag =
        cmdLine.handleFlagWithParam("start-debugger-server", false);
    } catch(e) {
      
      
      debuggerServerFlag = cmdLine.handleFlag("start-debugger-server", false);
    }
    if (debuggerServerFlag) {
      this.handleDebuggerServerFlag(cmdLine, debuggerServerFlag);
    }
  },

  handleConsoleFlag: function(cmdLine) {
    let window = Services.wm.getMostRecentWindow("devtools:webconsole");
    if (!window) {
      let devtools = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools;
      
      Cu.import("resource:///modules/devtools/gDevTools.jsm");
      let hudservice = devtools.require("devtools/webconsole/hudservice");
      let console = Cu.import("resource://gre/modules/devtools/Console.jsm", {}).console;
      hudservice.toggleBrowserConsole().then(null, console.error);
    } else {
      window.focus(); 
    }

    if (cmdLine.state == Ci.nsICommandLine.STATE_REMOTE_AUTO) {
      cmdLine.preventDefault = true;
    }
  },

  _isRemoteDebuggingEnabled() {
    let remoteDebuggingEnabled = false;
    try {
      remoteDebuggingEnabled = kDebuggerPrefs.every((pref) => Services.prefs.getBoolPref(pref));
    } catch (ex) {
      Cu.reportError(ex);
      return false;
    }
    if (!remoteDebuggingEnabled) {
      let errorMsg = "Could not run chrome debugger! You need the following prefs " +
                     "to be set to true: " + kDebuggerPrefs.join(", ");
      Cu.reportError(errorMsg);
      
      
      dump(errorMsg + "\n");
    }
    return remoteDebuggingEnabled;
  },

  handleDebuggerFlag: function(cmdLine) {
    if (!this._isRemoteDebuggingEnabled()) {
      return;
    }
    Cu.import("resource:///modules/devtools/ToolboxProcess.jsm");
    BrowserToolboxProcess.init();

    if (cmdLine.state == Ci.nsICommandLine.STATE_REMOTE_AUTO) {
      cmdLine.preventDefault = true;
    }
  },

  handleDebuggerServerFlag: function(cmdLine, portOrPath) {
    if (!this._isRemoteDebuggingEnabled()) {
      return;
    }
    if (portOrPath === true) {
      
      portOrPath = 6000;
    }
    let { DevToolsLoader } =
      Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

    try {
      
      
      
      
      
      
      let serverLoader = new DevToolsLoader();
      serverLoader.invisibleToDebugger = true;
      serverLoader.main("devtools/server/main");
      let debuggerServer = serverLoader.DebuggerServer;
      debuggerServer.init();
      debuggerServer.addBrowserActors();
      debuggerServer.allowChromeProcess = true;

      let listener = debuggerServer.createListener();
      listener.portOrPath = portOrPath;
      listener.open();
      dump("Started debugger server on " + portOrPath + "\n");
    } catch(e) {
      dump("Unable to start debugger server on " + portOrPath + ": " + e);
    }

    if (cmdLine.state == Ci.nsICommandLine.STATE_REMOTE_AUTO) {
      cmdLine.preventDefault = true;
    }
  },

  helpInfo : "  --jsconsole        Open the Browser Console.\n" +
             "  --jsdebugger       Open the Browser Toolbox.\n" +
             "  --start-debugger-server [port|path] " +
             "Start the debugger server on a TCP port or " +
             "Unix domain socket path.  Defaults to TCP port 6000.\n",

  classID: Components.ID("{9e9a9283-0ce9-4e4a-8f1c-ba129a032c32}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([devtoolsCommandlineHandler]);
