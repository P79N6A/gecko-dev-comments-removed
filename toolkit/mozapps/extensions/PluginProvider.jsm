



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const URI_EXTENSION_STRINGS  = "chrome://mozapps/locale/extensions/extensions.properties";
const STRING_TYPE_NAME       = "type.%ID%.name";

for (let name of ["LOG", "WARN", "ERROR"]) {
  this.__defineGetter__(name, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.plugins", this);
    return this[name];
  });
}

function getIDHashForString(aStr) {
  
  function toHexString(charCode)
    ("0" + charCode.toString(16)).slice(-2);

  let hasher = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
  hasher.init(Ci.nsICryptoHash.MD5);
  let stringStream = Cc["@mozilla.org/io/string-input-stream;1"].
                     createInstance(Ci.nsIStringInputStream);
                     stringStream.data = aStr ? aStr : "null";
  hasher.updateFromStream(stringStream, -1);

  
  let binary = hasher.finish(false);
  let hash = [toHexString(binary.charCodeAt(i)) for (i in binary)].join("").toLowerCase();
  return "{" + hash.substr(0, 8) + "-" +
               hash.substr(8, 4) + "-" +
               hash.substr(12, 4) + "-" +
               hash.substr(16, 4) + "-" +
               hash.substr(20) + "}";
}

var PluginProvider = {
  
  plugins: null,

  startup: function PL_startup() {
    Services.obs.addObserver(this, AddonManager.OPTIONS_NOTIFICATION_DISPLAYED, false);
  },

  



  shutdown: function PL_shutdown() {
    this.plugins = null;
    Services.obs.removeObserver(this, AddonManager.OPTIONS_NOTIFICATION_DISPLAYED);
  },

  observe: function(aSubject, aTopic, aData) {
    this.getAddonByID(aData, function(plugin) {
      if (!plugin)
        return;

      let libLabel = aSubject.getElementById("pluginLibraries");
      libLabel.textContent = plugin.pluginLibraries.join(", ");

      let typeLabel = aSubject.getElementById("pluginMimeTypes"), types = [];
      for (let type of plugin.pluginMimeTypes) {
        let extras = [type.description.trim(), type.suffixes].
                     filter(function(x) x).join(": ");
        types.push(type.type + (extras ? " (" + extras + ")" : ""));
      }
      typeLabel.textContent = types.join(",\n");
    });
  },

  







  getAddonByID: function PL_getAddon(aId, aCallback) {
    if (!this.plugins)
      this.buildPluginList();

    if (aId in this.plugins) {
      let name = this.plugins[aId].name;
      let description = this.plugins[aId].description;
      let tags = this.plugins[aId].tags;

      aCallback(new PluginWrapper(aId, name, description, tags));
    }
    else {
      aCallback(null);
    }
  },

  







  getAddonsByTypes: function PL_getAddonsByTypes(aTypes, aCallback) {
    if (aTypes && aTypes.indexOf("plugin") < 0) {
      aCallback([]);
      return;
    }

    if (!this.plugins)
      this.buildPluginList();

    let results = [];

    for (let id in this.plugins) {
      this.getAddonByID(id, function(aAddon) {
        results.push(aAddon);
      });
    }

    aCallback(results);
  },

  







  getAddonsWithOperationsByTypes: function PL_getAddonsWithOperationsByTypes(aTypes, aCallback) {
    aCallback([]);
  },

  







  getInstallsByTypes: function PL_getInstallsByTypes(aTypes, aCallback) {
    aCallback([]);
  },

  buildPluginList: function PL_buildPluginList() {
    let tags = Cc["@mozilla.org/plugin/host;1"].
               getService(Ci.nsIPluginHost).
               getPluginTags({});

    this.plugins = {};
    let plugins = {};
    for (let tag of tags) {
      if (!(tag.name in plugins))
        plugins[tag.name] = {};
      if (!(tag.description in plugins[tag.name])) {
        let plugin = {
          name: tag.name,
          description: tag.description,
          tags: [tag]
        };

        let id = getIDHashForString(tag.name + tag.description);

        plugins[tag.name][tag.description] = plugin;
        this.plugins[id] = plugin;
      }
      else {
        plugins[tag.name][tag.description].tags.push(tag);
      }
    }
  }
};





function PluginWrapper(aId, aName, aDescription, aTags) {
  let safedesc = aDescription.replace(/<\/?[a-z][^>]*>/gi, " ");
  let homepageURL = null;
  if (/<A\s+HREF=[^>]*>/i.test(aDescription))
    homepageURL = /<A\s+HREF=["']?([^>"'\s]*)/i.exec(aDescription)[1];

  this.__defineGetter__("id", function() aId);
  this.__defineGetter__("type", function() "plugin");
  this.__defineGetter__("name", function() aName);
  this.__defineGetter__("creator", function() null);
  this.__defineGetter__("description", function() safedesc);
  this.__defineGetter__("version", function() aTags[0].version);
  this.__defineGetter__("homepageURL", function() homepageURL);

  this.__defineGetter__("isActive", function() !aTags[0].blocklisted && !aTags[0].disabled);
  this.__defineGetter__("appDisabled", function() aTags[0].blocklisted);
  this.__defineGetter__("userDisabled", function() aTags[0].disabled);
  this.__defineSetter__("userDisabled", function(aVal) {
    if (aTags[0].disabled == aVal)
      return;

    for (let tag of aTags)
      tag.disabled = aVal;
    AddonManagerPrivate.callAddonListeners(aVal ? "onDisabling" : "onEnabling", this, false);
    AddonManagerPrivate.callAddonListeners(aVal ? "onDisabled" : "onEnabled", this);
    return aVal;
  });

  this.__defineGetter__("blocklistState", function() {
    let bs = Cc["@mozilla.org/extensions/blocklist;1"].
             getService(Ci.nsIBlocklistService);
    return bs.getPluginBlocklistState(aTags[0]);
  });

  this.__defineGetter__("blocklistURL", function() {
    let bs = Cc["@mozilla.org/extensions/blocklist;1"].
             getService(Ci.nsIBlocklistService);
    return bs.getPluginBlocklistURL(aTags[0]);
  });

  this.__defineGetter__("size", function() {
    function getDirectorySize(aFile) {
      let size = 0;
      let entries = aFile.directoryEntries.QueryInterface(Ci.nsIDirectoryEnumerator);
      let entry;
      while (entry = entries.nextFile) {
        if (entry.isSymlink() || !entry.isDirectory())
          size += entry.fileSize;
        else
          size += getDirectorySize(entry);
      }
      entries.close();
      return size;
    }

    let size = 0;
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    for (let tag of aTags) {
      file.initWithPath(tag.fullpath);
      if (file.isDirectory())
        size += getDirectorySize(file);
      else
        size += file.fileSize;
    }
    return size;
  });

  this.__defineGetter__("pluginLibraries", function() {
    let libs = [];
    for (let tag of aTags)
      libs.push(tag.filename);
    return libs;
  });

  this.__defineGetter__("pluginMimeTypes", function() {
    let types = [];
    for (let tag of aTags)
      for (let type of tag.getMimeTypes({}))
        types.push(type);
    return types;
  });

  this.__defineGetter__("installDate", function() {
    let date = 0;
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    for (let tag of aTags) {
      file.initWithPath(tag.fullpath);
      date = Math.max(date, file.lastModifiedTime);
    }
    return new Date(date);
  });

  this.__defineGetter__("scope", function() {
    let path = aTags[0].fullpath;
    
    let dir = Services.dirsvc.get("APlugns", Ci.nsIFile);
    if (path.substring(0, dir.path.length) == dir.path)
      return AddonManager.SCOPE_APPLICATION;

    
    dir = Services.dirsvc.get("ProfD", Ci.nsIFile);
    if (path.substring(0, dir.path.length) == dir.path)
      return AddonManager.SCOPE_PROFILE;

    
    dir = Services.dirsvc.get("Home", Ci.nsIFile);
    if (path.substring(0, dir.path.length) == dir.path)
      return AddonManager.SCOPE_USER;

    
    return AddonManager.SCOPE_SYSTEM;
  });

  this.__defineGetter__("pendingOperations", function() {
    return AddonManager.PENDING_NONE;
  });

  this.__defineGetter__("operationsRequiringRestart", function() {
    return AddonManager.OP_NEEDS_RESTART_NONE;
  });

  this.__defineGetter__("permissions", function() {
    let permissions = 0;
    if (!this.appDisabled) {
      if (this.userDisabled)
        permissions |= AddonManager.PERM_CAN_ENABLE;
      else
        permissions |= AddonManager.PERM_CAN_DISABLE;
    }
    return permissions;
  });
}

PluginWrapper.prototype = {
  optionsType: AddonManager.OPTIONS_TYPE_INLINE,
  optionsURL: "chrome://mozapps/content/extensions/pluginPrefs.xul",

  get updateDate() {
    return this.installDate;
  },

  get isCompatible() {
    return true;
  },

  get isPlatformCompatible() {
    return true;
  },

  get providesUpdatesSecurely() {
    return true;
  },

  get foreignInstall() {
    return true;
  },

  isCompatibleWith: function(aAppVerison, aPlatformVersion) {
    return true;
  },

  findUpdates: function(aListener, aReason, aAppVersion, aPlatformVersion) {
    if ("onNoCompatibilityUpdateAvailable" in aListener)
      aListener.onNoCompatibilityUpdateAvailable(this);
    if ("onNoUpdateAvailable" in aListener)
      aListener.onNoUpdateAvailable(this);
    if ("onUpdateFinished" in aListener)
      aListener.onUpdateFinished(this);
  }
};

AddonManagerPrivate.registerProvider(PluginProvider, [
  new AddonManagerPrivate.AddonType("plugin", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 6000)
]);
