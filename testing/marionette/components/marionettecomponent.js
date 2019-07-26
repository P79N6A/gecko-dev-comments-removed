



this.CC = Components.Constructor;
this.Cc = Components.classes;
this.Ci = Components.interfaces;
this.Cu = Components.utils;

const MARIONETTE_CONTRACTID = "@mozilla.org/marionette;1";
const MARIONETTE_CID = Components.ID("{786a1369-dca5-4adc-8486-33d23c88010a}");
const MARIONETTE_ENABLED_PREF = 'marionette.defaultPrefs.enabled';
const MARIONETTE_FORCELOCAL_PREF = 'marionette.force-local';

this.ServerSocket = CC("@mozilla.org/network/server-socket;1",
                       "nsIServerSocket",
                       "initSpecialConnection");

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");

let loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
               .getService(Ci.mozIJSSubScriptLoader);

function MarionetteComponent() {
  this._loaded = false;
  this.observerService = Services.obs;

  
  this.logger = Log.repository.getLogger("Marionette");
  this.logger.level = Log.Level["Trace"];
  let logf = FileUtils.getFile('ProfD', ['marionette.log']);

  let dumper = false;
  let formatter = new Log.BasicFormatter();
  this.logger.addAppender(new Log.BoundedFileAppender(logf.path, formatter));
#ifdef DEBUG
  dumper = true;
#endif
#ifdef MOZ_B2G
  dumper = true;
#endif
  if (dumper) {
    this.logger.addAppender(new Log.DumpAppender(formatter));
  }
  this.logger.info("MarionetteComponent loaded");
}

MarionetteComponent.prototype = {
  classDescription: "Marionette component",
  classID: MARIONETTE_CID,
  contractID: MARIONETTE_CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler, Ci.nsIObserver]),
  _xpcom_categories: [{category: "command-line-handler", entry: "b-marionette"},
                      {category: "profile-after-change", service: true}],
  appName: Services.appinfo.name,
  enabled: false,
  finalUiStartup: false,
  _marionetteServer: null,

  onSocketAccepted: function mc_onSocketAccepted(aSocket, aTransport) {
    this.logger.info("onSocketAccepted for Marionette dummy socket");
  },

  onStopListening: function mc_onStopListening(aSocket, status) {
    this.logger.info("onStopListening for Marionette dummy socket, code " + status);
    aSocket.close();
  },

  
  handle: function mc_handle(cmdLine) {
    
    if (cmdLine.handleFlag("marionette", false)) {
      this.enabled = true;
      this.logger.info("marionette enabled via command-line");
      this.init();
    }
  },

  observe: function mc_observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "profile-after-change":
        
        
        this.observerService.addObserver(this, "final-ui-startup", false);
#ifdef ENABLE_MARIONETTE
        let enabledPref = false;
        try {
          enabledPref = Services.prefs.getBoolPref(MARIONETTE_ENABLED_PREF);
        } catch(e) {}
        if (enabledPref) {
          this.enabled = true;
          this.logger.info("marionette enabled via build flag and pref");

          
          
          if (Services.appinfo.inSafeMode) {
            this.observerService.addObserver(this, "domwindowopened", false);
          }
        }
        else {
          this.logger.info("marionette not enabled via pref");
        }
#endif
        break;
      case "final-ui-startup":
        this.finalUiStartup = true;
        this.observerService.removeObserver(this, aTopic);
        this.observerService.addObserver(this, "xpcom-shutdown", false);
        this.init();
        break;
      case "domwindowopened":
        this.observerService.removeObserver(this, aTopic);
        this._suppressSafeModeDialog(aSubject);
        break;
      case "xpcom-shutdown":
        this.observerService.removeObserver(this, "xpcom-shutdown");
        this.uninit();
        break;
    }
  },

  _suppressSafeModeDialog: function mc_suppressSafeModeDialog(aWindow) {
    
    aWindow.addEventListener("load", function onLoad() {
      aWindow.removeEventListener("load", onLoad);

      if (aWindow.document.getElementById("safeModeDialog")) {
        aWindow.setTimeout(() => {
          
          aWindow.document.documentElement.getButton("accept").click();
        });
      }
    });
  },

  init: function mc_init() {
    if (!this._loaded && this.enabled && this.finalUiStartup) {
      this._loaded = true;

      let marionette_forcelocal = this.appName == 'B2G' ? false : true;
      try {
        marionette_forcelocal = Services.prefs.getBoolPref(MARIONETTE_FORCELOCAL_PREF);
      }
      catch(e) {}
      Services.prefs.setBoolPref(MARIONETTE_FORCELOCAL_PREF, marionette_forcelocal);

      if (!marionette_forcelocal) {
        
        
        
        
        
        let insaneSacrificialGoat = new ServerSocket(666, Ci.nsIServerSocket.KeepWhenOffline, 4);
        insaneSacrificialGoat.asyncListen(this);
      }

      let port;
      try {
        port = Services.prefs.getIntPref('marionette.defaultPrefs.port');
      }
      catch(e) {
        port = 2828;
      }
      try {
        loader.loadSubScript("chrome://marionette/content/marionette-server.js");
        let forceLocal = Services.prefs.getBoolPref(MARIONETTE_FORCELOCAL_PREF);
        this._marionetteServer = new MarionetteServer(port, forceLocal);
        this.logger.info("Marionette server ready");
      }
      catch(e) {
        this.logger.error('exception: ' + e.name + ', ' + e.message);
      }
    }
  },

  uninit: function mc_uninit() {
    if (this._marionetteServer) {
      this._marionetteServer.closeListener();
    }
    this._loaded = false;
  },

};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MarionetteComponent]);
