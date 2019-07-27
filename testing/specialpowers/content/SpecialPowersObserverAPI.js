



"use strict";

Components.utils.import("resource://gre/modules/Services.jsm");

if (typeof(Ci) == 'undefined') {
  var Ci = Components.interfaces;
}

if (typeof(Cc) == 'undefined') {
  var Cc = Components.classes;
}

this.SpecialPowersError = function(aMsg) {
  Error.call(this);
  let {stack} = new Error();
  this.message = aMsg;
  this.name = "SpecialPowersError";
}
SpecialPowersError.prototype = Object.create(Error.prototype);

SpecialPowersError.prototype.toString = function() {
  return `${this.name}: ${this.message}`;
};

this.SpecialPowersObserverAPI = function SpecialPowersObserverAPI() {
  this._crashDumpDir = null;
  this._processCrashObserversRegistered = false;
  this._chromeScriptListeners = [];
}

function parseKeyValuePairs(text) {
  var lines = text.split('\n');
  var data = {};
  for (let i = 0; i < lines.length; i++) {
    if (lines[i] == '')
      continue;

    
    let eq = lines[i].indexOf('=');
    if (eq != -1) {
      let [key, value] = [lines[i].substring(0, eq),
                          lines[i].substring(eq + 1)];
      if (key && value)
        data[key] = value.replace(/\\n/g, "\n").replace(/\\\\/g, "\\");
    }
  }
  return data;
}

function parseKeyValuePairsFromFile(file) {
  var fstream = Cc["@mozilla.org/network/file-input-stream;1"].
                createInstance(Ci.nsIFileInputStream);
  fstream.init(file, -1, 0, 0);
  var is = Cc["@mozilla.org/intl/converter-input-stream;1"].
           createInstance(Ci.nsIConverterInputStream);
  is.init(fstream, "UTF-8", 1024, Ci.nsIConverterInputStream.DEFAULT_REPLACEMENT_CHARACTER);
  var str = {};
  var contents = '';
  while (is.readString(4096, str) != 0) {
    contents += str.value;
  }
  is.close();
  fstream.close();
  return parseKeyValuePairs(contents);
}

function getTestPlugin(pluginName) {
  var ph = Cc["@mozilla.org/plugin/host;1"]
    .getService(Ci.nsIPluginHost);
  var tags = ph.getPluginTags();
  var name = pluginName || "Test Plug-in";
  for (var tag of tags) {
    if (tag.name == name) {
      return tag;
    }
  }

  return null;
}

SpecialPowersObserverAPI.prototype = {

  _observe: function(aSubject, aTopic, aData) {
    function addDumpIDToMessage(propertyName) {
      try {
        var id = aSubject.getPropertyAsAString(propertyName);
      } catch(ex) {
        var id = null;
      }
      if (id) {
        message.dumpIDs.push({id: id, extension: "dmp"});
        message.dumpIDs.push({id: id, extension: "extra"});
      }
    }

    switch(aTopic) {
      case "plugin-crashed":
      case "ipc:content-shutdown":
        var message = { type: "crash-observed", dumpIDs: [] };
        aSubject = aSubject.QueryInterface(Ci.nsIPropertyBag2);
        if (aTopic == "plugin-crashed") {
          addDumpIDToMessage("pluginDumpID");
          addDumpIDToMessage("browserDumpID");

          let pluginID = aSubject.getPropertyAsAString("pluginDumpID");
          let extra = this._getExtraData(pluginID);
          if (extra && ("additional_minidumps" in extra)) {
            let dumpNames = extra.additional_minidumps.split(',');
            for (let name of dumpNames) {
              message.dumpIDs.push({id: pluginID + "-" + name, extension: "dmp"});
            }
          }
        } else { 
          addDumpIDToMessage("dumpID");
        }
        this._sendAsyncMessage("SPProcessCrashService", message);
        break;
    }
  },

  _getCrashDumpDir: function() {
    if (!this._crashDumpDir) {
      this._crashDumpDir = Services.dirsvc.get("ProfD", Ci.nsIFile);
      this._crashDumpDir.append("minidumps");
    }
    return this._crashDumpDir;
  },

  _getExtraData: function(dumpId) {
    let extraFile = this._getCrashDumpDir().clone();
    extraFile.append(dumpId + ".extra");
    if (!extraFile.exists()) {
      return null;
    }
    return parseKeyValuePairsFromFile(extraFile);
  },

  _deleteCrashDumpFiles: function(aFilenames) {
    var crashDumpDir = this._getCrashDumpDir();
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

  _findCrashDumpFiles: function(aToIgnore) {
    var crashDumpDir = this._getCrashDumpDir();
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

  _getURI: function (url) {
    return Services.io.newURI(url, null, null);
  },

  _readUrlAsString: function(aUrl) {
    
    
    var scriptableStream = Cc["@mozilla.org/scriptableinputstream;1"]
                             .getService(Ci.nsIScriptableInputStream);
    var channel = Services.io.newChannel2(aUrl,
                                          null,
                                          null,
                                          null,      
                                          Services.scriptSecurityManager.getSystemPrincipal(),
                                          null,      
                                          Ci.nsILoadInfo.SEC_NORMAL,
                                          Ci.nsIContentPolicy.TYPE_OTHER);
    var input = channel.open();
    scriptableStream.init(input);

    var str;
    var buffer = [];

    while ((str = scriptableStream.read(4096))) {
      buffer.push(str);
    }

    var output = buffer.join("");

    scriptableStream.close();
    input.close();

    var status;
    try {
      channel.QueryInterface(Ci.nsIHttpChannel);
      status = channel.responseStatus;
    } catch(e) {
      
      dump("-*- _readUrlAsString: Got an error while fetching " +
           "chrome script '" + aUrl + "': (" + e.name + ") " + e.message + ". " +
           "Ignoring.\n");
    }

    if (status == 404) {
      throw new SpecialPowersError(
        "Error while executing chrome script '" + aUrl + "':\n" +
        "The script doesn't exists. Ensure you have registered it in " +
        "'support-files' in your mochitest.ini.");
    }

    return output;
  },

  



  _receiveMessageAPI: function(aMessage) {
    
    
    
    switch(aMessage.name) {
      case "SPPrefService": {
        let prefs = Services.prefs;
        let prefType = aMessage.json.prefType.toUpperCase();
        let prefName = aMessage.json.prefName;
        let prefValue = "prefValue" in aMessage.json ? aMessage.json.prefValue : null;

        if (aMessage.json.op == "get") {
          if (!prefName || !prefType)
            throw new SpecialPowersError("Invalid parameters for get in SPPrefService");

          
          if (prefs.getPrefType(prefName) == prefs.PREF_INVALID)
            return null;
        } else if (aMessage.json.op == "set") {
          if (!prefName || !prefType  || prefValue === null)
            throw new SpecialPowersError("Invalid parameters for set in SPPrefService");
        } else if (aMessage.json.op == "clear") {
          if (!prefName)
            throw new SpecialPowersError("Invalid parameters for clear in SPPrefService");
        } else {
          throw new SpecialPowersError("Invalid operation for SPPrefService");
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
              return undefined;
            }
        }
        return undefined;	
      }

      case "SPProcessCrashService": {
        switch (aMessage.json.op) {
          case "register-observer":
            this._addProcessCrashObservers();
            break;
          case "unregister-observer":
            this._removeProcessCrashObservers();
            break;
          case "delete-crash-dump-files":
            return this._deleteCrashDumpFiles(aMessage.json.filenames);
          case "find-crash-dump-files":
            return this._findCrashDumpFiles(aMessage.json.crashDumpFilesToIgnore);
          default:
            throw new SpecialPowersError("Invalid operation for SPProcessCrashService");
        }
        return undefined;	
      }

      case "SPPermissionManager": {
        let msg = aMessage.json;

        let secMan = Services.scriptSecurityManager;
        let principal = secMan.getAppCodebasePrincipal(this._getURI(msg.url), msg.appId, msg.isInBrowserElement);

        switch (msg.op) {
          case "add":
            Services.perms.addFromPrincipal(principal, msg.type, msg.permission);
            break;
          case "remove":
            Services.perms.removeFromPrincipal(principal, msg.type);
            break;
          case "has":
            let hasPerm = Services.perms.testPermissionFromPrincipal(principal, msg.type);
            if (hasPerm == Ci.nsIPermissionManager.ALLOW_ACTION) 
              return true;
            return false;
            break;
          case "test":
            let testPerm = Services.perms.testPermissionFromPrincipal(principal, msg.type, msg.value);
            if (testPerm == msg.value)  {
              return true;
            }
            return false;
            break;
          default:
            throw new SpecialPowersError(
              "Invalid operation for SPPermissionManager");
        }
        return undefined;	
      }

      case "SPSetTestPluginEnabledState": {
        var plugin = getTestPlugin(aMessage.data.pluginName);
        if (!plugin) {
          return undefined;
        }
        var oldEnabledState = plugin.enabledState;
        plugin.enabledState = aMessage.data.newEnabledState;
        return oldEnabledState;
      }

      case "SPWebAppService": {
        let Webapps = {};
        Components.utils.import("resource://gre/modules/Webapps.jsm", Webapps);
        switch (aMessage.json.op) {
          case "set-launchable":
            let val = Webapps.DOMApplicationRegistry.allAppsLaunchable;
            Webapps.DOMApplicationRegistry.allAppsLaunchable = aMessage.json.launchable;
            return val;
          case "allow-unsigned-addons":
            {
              let utils = {};
              Components.utils.import("resource://gre/modules/AppsUtils.jsm", utils);
              utils.AppsUtils.allowUnsignedAddons = true;
              return;
            }
          case "debug-customizations":
            {
              let scope = {};
              Components.utils.import("resource://gre/modules/UserCustomizations.jsm", scope);
              scope.UserCustomizations._debug = aMessage.json.value;
              return;
            }
          default:
            throw new SpecialPowersError("Invalid operation for SPWebAppsService");
        }
        return undefined;	
      }

      case "SPObserverService": {
        switch (aMessage.json.op) {
          case "notify":
            let topic = aMessage.json.observerTopic;
            let data = aMessage.json.observerData
            Services.obs.notifyObservers(null, topic, data);
            break;
          default:
            throw new SpecialPowersError("Invalid operation for SPObserverervice");
        }
        return undefined;	
      }

      case "SPLoadChromeScript": {
        let url = aMessage.json.url;
        let id = aMessage.json.id;

        let jsScript = this._readUrlAsString(url);

        
        
        
        let systemPrincipal = Services.scriptSecurityManager.getSystemPrincipal();
        let sb = Components.utils.Sandbox(systemPrincipal);
        let mm = aMessage.target
                         .QueryInterface(Ci.nsIFrameLoaderOwner)
                         .frameLoader
                         .messageManager;
        sb.sendAsyncMessage = (name, message) => {
          mm.sendAsyncMessage("SPChromeScriptMessage",
                              { id: id, name: name, message: message });
        };
        sb.addMessageListener = (name, listener) => {
          this._chromeScriptListeners.push({ id: id, name: name, listener: listener });
        };
        sb.browserElement = aMessage.target;

        
        let reporter = function (err, message, stack) {
          
          mm.sendAsyncMessage("SPChromeScriptAssert",
                              { id: id, url: url, err: err, message: message,
                                stack: stack });
        };
        Object.defineProperty(sb, "assert", {
          get: function () {
            let scope = Components.utils.createObjectIn(sb);
            Services.scriptloader.loadSubScript("resource://specialpowers/Assert.jsm",
                                                scope);

            let assert = new scope.Assert(reporter);
            delete sb.assert;
            return sb.assert = assert;
          },
          configurable: true
        });

        
        try {
          Components.utils.evalInSandbox(jsScript, sb, "1.8", url, 1);
        } catch(e) {
          throw new SpecialPowersError(
            "Error while executing chrome script '" + url + "':\n" +
            e + "\n" +
            e.fileName + ":" + e.lineNumber);
        }
        return undefined;	
      }

      case "SPChromeScriptMessage": {
        let id = aMessage.json.id;
        let name = aMessage.json.name;
        let message = aMessage.json.message;
        this._chromeScriptListeners
            .filter(o => (o.name == name && o.id == id))
            .forEach(o => o.listener(message));
        return undefined;	
      }

      case 'SPQuotaManager': {
        let qm = Cc['@mozilla.org/dom/quota/manager;1']
                   .getService(Ci.nsIQuotaManager);
        let mm = aMessage.target
                         .QueryInterface(Ci.nsIFrameLoaderOwner)
                         .frameLoader
                         .messageManager;
        let msg = aMessage.data;
        let op = msg.op;

        if (op != 'clear' && op != 'getUsage') {
          throw new SpecialPowersError('Invalid operation for SPQuotaManager');
        }

        let uri = this._getURI(msg.uri);

        if (op == 'clear') {
          if (('inBrowser' in msg) && msg.inBrowser !== undefined) {
            qm.clearStoragesForURI(uri, msg.appId, msg.inBrowser);
          } else if (('appId' in msg) && msg.appId !== undefined) {
            qm.clearStoragesForURI(uri, msg.appId);
          } else {
            qm.clearStoragesForURI(uri);
          }
        }

        
        
        
        let callback = function(uri, usage, fileUsage) {
          let reply = { id: msg.id };
          if (op == 'getUsage') {
            reply.usage = usage;
            reply.fileUsage = fileUsage;
          }
          mm.sendAsyncMessage(aMessage.name, reply);
        };

        if (('inBrowser' in msg) && msg.inBrowser !== undefined) {
          qm.getUsageForURI(uri, callback, msg.appId, msg.inBrowser);
        } else if (('appId' in msg) && msg.appId !== undefined) {
          qm.getUsageForURI(uri, callback, msg.appId);
        } else {
          qm.getUsageForURI(uri, callback);
        }

        return undefined;	
      }

      case "SPPeriodicServiceWorkerUpdates": {
        
        
        
        Cc["@mozilla.org/service-worker-periodic-updater;1"].
          getService(Ci.nsIObserver).
          observe(null, "idle-daily", "");

        return undefined;	
      }

      default:
        throw new SpecialPowersError("Unrecognized Special Powers API");
    }

    
    
    throw new SpecialPowersError("Unreached code");
    return undefined;
  }
};
