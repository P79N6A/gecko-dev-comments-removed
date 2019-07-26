









const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const REMOTE_ENABLED_PREF = "devtools.debugger.remote-enabled";
const UNIX_SOCKET_PREF = "devtools.debugger.unix-domain-socket";
const REMOTE_PORT_PREF = "devtools.debugger.remote-port";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this,
    "DebuggerServer",
    "resource://gre/modules/devtools/dbg-server.jsm");

function DevToolsAppStartup() {
}

DevToolsAppStartup.prototype = {
  classID: Components.ID("{9ba9bbe7-5866-46f1-bea6-3299066b7933}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler, Ci.nsIObserver]),

  get dbgPortOrPath() {
    if (!this._dbgPortOrPath) {
      
      try {
        this._dbgPortOrPath = Services.prefs.getCharPref(UNIX_SOCKET_PREF);
      } catch(e) {
        try {
          this._dbgPortOrPath = Services.prefs.getIntPref(REMOTE_PORT_PREF);
        } catch(e) {}
      }
    }
    return this._dbgPortOrPath;
  },

  set dbgPortOrPath(value) {
    this._dbgPortOrPath = value;
  },

  

  get helpInfo() {
    let str = "";

    
    
    

    if (DebuggerServer.controller) {
      str += "  -start-debugger-server [<port or unix domain socket path>]";
      if (this.dbgPortOrPath) {
        str += " (default: " + this.dbgPortOrPath + ")";
      }
      str += "\n";
    }

    return str;
  },

  handle: function(cmdLine) {
    if (!DebuggerServer.controller) {
      
      
      
      return;
    }

    let startDebuggerServerBecauseCmdLine = false;

    try {
      
      
      
      
      let param = cmdLine.handleFlagWithParam("start-debugger-server", false);
      if (param) {
        startDebuggerServerBecauseCmdLine = true;
        this.dbgPortOrPath = param;
      }
    } catch(e) {
      startDebuggerServerBecauseCmdLine = true;
    }

    
    
    

    if (startDebuggerServerBecauseCmdLine ||
        Services.prefs.getBoolPref(REMOTE_ENABLED_PREF)) {
      if (this.dbgPortOrPath) {
        DebuggerServer.controller.start(this.dbgPortOrPath);
      } else {
        dump("Can't start debugger: no port or path specified\n");
      }
    }

    Services.prefs.addObserver(REMOTE_ENABLED_PREF, this, false);
    Services.prefs.addObserver(UNIX_SOCKET_PREF, this, false);
    Services.prefs.addObserver(REMOTE_PORT_PREF, this, false);
  },

  

  observe: function (subject, topic, data) {
    if (topic == "nsPref:changed") {
      switch (data) {
        case REMOTE_ENABLED_PREF:
          if (Services.prefs.getBoolPref(data)) {
            DebuggerServer.controller.start(this.dbgPortOrPath);
          } else {
            DebuggerServer.controller.stop();
          }
        break;
        case UNIX_SOCKET_PREF:
        case REMOTE_PORT_PREF:
          
          this.dbgPortOrPath = null;
        break;
      }
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DevToolsAppStartup]);
