












































Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;

const CHILD_SCRIPT = "chrome://specialpowers/content/specialpowers.js"




function SpecialPowersException(aMsg) {
  this.message = aMsg;
  this.name = "SpecialPowersException";
}

SpecialPowersException.prototype.toString = function() {
  return this.name + ': "' + this.message + '"';
};


function SpecialPowersObserver() {
  this._isFrameScriptLoaded = false;
  this._messageManager = Cc["@mozilla.org/globalmessagemanager;1"].
                         getService(Ci.nsIChromeFrameMessageManager);
}

SpecialPowersObserver.prototype = {
  classDescription: "Special powers Observer for use in testing.",
  classID:          Components.ID("{59a52458-13e0-4d93-9d85-a637344f29a1}"),
  contractID:       "@mozilla.org/special-powers-observer;1",
  QueryInterface:   XPCOMUtils.generateQI([Components.interfaces.nsIObserver]),
  _xpcom_categories: [{category: "profile-after-change", service: true }],

  observe: function(aSubject, aTopic, aData)
  {
    switch (aTopic) {
      case "profile-after-change":
        this.init();
        break;

      case "chrome-document-global-created":
        if (!this._isFrameScriptLoaded) {
          
          this._messageManager.addMessageListener("SPPrefService", this);
          this._messageManager.addMessageListener("SPProcessCrashService", this);
          this._messageManager.addMessageListener("SPPingService", this);

          this._messageManager.loadFrameScript(CHILD_SCRIPT, true);
          this._isFrameScriptLoaded = true;
        }
        break;

      case "xpcom-shutdown":
        this.uninit();
        break;

      case "plugin-crashed":
      case "ipc:content-shutdown":
        function addDumpIDToMessage(propertyName) {
          var id = aSubject.getPropertyAsAString(propertyName);
          if (id) {
            message.dumpIDs.push(id);
          }
        }

        var message = { type: "crash-observed", dumpIDs: [] };
        aSubject = aSubject.QueryInterface(Ci.nsIPropertyBag2);
        if (aTopic == "plugin-crashed") {
          addDumpIDToMessage("pluginDumpID");
          addDumpIDToMessage("browserDumpID");
        } else { 
          addDumpIDToMessage("dumpID");
        }
        this._messageManager.sendAsyncMessage("SPProcessCrashService", message);
        break;
    }
  },

  init: function()
  {
    var obs = Services.obs;
    obs.addObserver(this, "xpcom-shutdown", false);
    obs.addObserver(this, "chrome-document-global-created", false);
  },

  uninit: function()
  {
    var obs = Services.obs;
    obs.removeObserver(this, "chrome-document-global-created", false);
    this.removeProcessCrashObservers();
  },
  
  addProcessCrashObservers: function() {
    if (this._processCrashObserversRegistered) {
      return;
    }

    Services.obs.addObserver(this, "plugin-crashed", false);
    Services.obs.addObserver(this, "ipc:content-shutdown", false);
    this._processCrashObserversRegistered = true;
  },

  removeProcessCrashObservers: function() {
    if (!this._processCrashObserversRegistered) {
      return;
    }

    Services.obs.removeObserver(this, "plugin-crashed");
    Services.obs.removeObserver(this, "ipc:content-shutdown");
    this._processCrashObserversRegistered = false;
  },

  getCrashDumpDir: function() {
    if (!this._crashDumpDir) {
      var directoryService = Cc["@mozilla.org/file/directory_service;1"]
                             .getService(Ci.nsIProperties);
      this._crashDumpDir = directoryService.get("ProfD", Ci.nsIFile);
      this._crashDumpDir.append("minidumps");
    }
    return this._crashDumpDir;
  },

  deleteCrashDumpFiles: function(aFilenames) {
    var crashDumpDir = this.getCrashDumpDir();
    if (!crashDumpDir.exists()) {
      return false;
    }

    var success = aFilenames.length != 0;
    aFilenames.forEach(function(crashFilename) {
      var file = crashDumpDir.clone();
      file.append(crashFilename);
      if (file.exists()) {
        file.remove(false);
      } else {
        success = false;
      }
    });
    return success;
  },

  findCrashDumpFiles: function(aToIgnore) {
    var crashDumpDir = this.getCrashDumpDir();
    var entries = crashDumpDir.exists() && crashDumpDir.directoryEntries;
    if (!entries) {
      return [];
    }

    var crashDumpFiles = [];
    while (entries.hasMoreElements()) {
      var file = entries.getNext().QueryInterface(Ci.nsIFile);
      var path = String(file.path);
      if (path.match(/\.(dmp|extra)$/) && !aToIgnore[path]) {
        crashDumpFiles.push(path);
      }
    }
    return crashDumpFiles.concat();
  },

  



  receiveMessage: function(aMessage) {
    switch(aMessage.name) {
      case "SPPrefService":
        var prefs = Services.prefs;
        var prefType = aMessage.json.prefType.toUpperCase();
        var prefName = aMessage.json.prefName;
        var prefValue = "prefValue" in aMessage.json ? aMessage.json.prefValue : null;

        if (aMessage.json.op == "get") {
          if (!prefName || !prefType)
            throw new SpecialPowersException("Invalid parameters for get in SPPrefService");
        } else if (aMessage.json.op == "set") {
          if (!prefName || !prefType  || prefValue === null)
            throw new SpecialPowersException("Invalid parameters for set in SPPrefService");
        } else if (aMessage.json.op == "clear") {
          if (!prefName)
            throw new SpecialPowersException("Invalid parameters for clear in SPPrefService");
        } else {
          throw new SpecialPowersException("Invalid operation for SPPrefService");
        }
        
        switch(prefType) {
          case "BOOL":
            if (aMessage.json.op == "get")
              return(prefs.getBoolPref(prefName));
            else 
              return(prefs.setBoolPref(prefName, prefValue));
          case "INT":
            if (aMessage.json.op == "get") 
              return(prefs.getIntPref(prefName));
            else
              return(prefs.setIntPref(prefName, prefValue));
          case "CHAR":
            if (aMessage.json.op == "get")
              return(prefs.getCharPref(prefName));
            else
              return(prefs.setCharPref(prefName, prefValue));
          case "COMPLEX":
            if (aMessage.json.op == "get")
              return(prefs.getComplexValue(prefName, prefValue[0]));
            else
              return(prefs.setComplexValue(prefName, prefValue[0], prefValue[1]));
          case "":
            if (aMessage.json.op == "clear") {
              prefs.clearUserPref(prefName);
              return;
            }
        }
        break;

      case "SPProcessCrashService":
        switch (aMessage.json.op) {
          case "register-observer":
            this.addProcessCrashObservers();
            break;
          case "unregister-observer":
            this.removeProcessCrashObservers();
            break;
          case "delete-crash-dump-files":
            return this.deleteCrashDumpFiles(aMessage.json.filenames);
          case "find-crash-dump-files":
            return this.findCrashDumpFiles(aMessage.json.crashDumpFilesToIgnore);
          default:
            throw new SpecialPowersException("Invalid operation for SPProcessCrashService");
        }
        break;

      case "SPPingService":
        if (aMessage.json.op == "ping") {
          aMessage.target
                  .QueryInterface(Ci.nsIFrameLoaderOwner)
                  .frameLoader
                  .messageManager
                  .sendAsyncMessage("SPPingService", { op: "pong" });
        }
        break;

      default:
        throw new SpecialPowersException("Unrecognized Special Powers API");
    }
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([SpecialPowersObserver]);
