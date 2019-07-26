



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const URI_EXTENSION_STRINGS  = "chrome://mozapps/locale/extensions/extensions.properties";
const STRING_TYPE_NAME       = "type.%ID%.name";
const LIST_UPDATED_TOPIC     = "plugins-list-updated";

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
    Services.obs.addObserver(this, LIST_UPDATED_TOPIC, false);
    Services.obs.addObserver(this, AddonManager.OPTIONS_NOTIFICATION_DISPLAYED, false);
  },

  



  shutdown: function PL_shutdown() {
    this.plugins = null;
    Services.obs.removeObserver(this, AddonManager.OPTIONS_NOTIFICATION_DISPLAYED);
    Services.obs.removeObserver(this, LIST_UPDATED_TOPIC);
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
    case AddonManager.OPTIONS_NOTIFICATION_DISPLAYED:
      this.getAddonByID(aData, function PL_displayPluginInfo(plugin) {
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
      break;
    case LIST_UPDATED_TOPIC:
      if (this.plugins)
        this.updatePluginList();
      break;
    }
  },

  


  buildWrapper: function PL_buildWrapper(aPlugin) {
    return new PluginWrapper(aPlugin.id,
                             aPlugin.name,
                             aPlugin.description,
                             aPlugin.tags);
  },

  







  getAddonByID: function PL_getAddon(aId, aCallback) {
    if (!this.plugins)
      this.buildPluginList();

    if (aId in this.plugins)
      aCallback(this.buildWrapper(this.plugins[aId]));
    else
      aCallback(null);
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

  




  getPluginList: function PL_getPluginList() {
    let tags = Cc["@mozilla.org/plugin/host;1"].
               getService(Ci.nsIPluginHost).
               getPluginTags({});

    let list = {};
    let seenPlugins = {};
    for (let tag of tags) {
      if (!(tag.name in seenPlugins))
        seenPlugins[tag.name] = {};
      if (!(tag.description in seenPlugins[tag.name])) {
        let plugin = {
          id: getIDHashForString(tag.name + tag.description),
          name: tag.name,
          description: tag.description,
          tags: [tag]
        };

        seenPlugins[tag.name][tag.description] = plugin;
        list[plugin.id] = plugin;
      }
      else {
        seenPlugins[tag.name][tag.description].tags.push(tag);
      }
    }

    return list;
  },

  


  buildPluginList: function PL_buildPluginList() {
    this.plugins = this.getPluginList();
  },

  




  updatePluginList: function PL_updatePluginList() {
    let newList = this.getPluginList();

    let lostPlugins = [this.buildWrapper(this.plugins[id])
                       for each (id in Object.keys(this.plugins)) if (!(id in newList))];
    let newPlugins = [this.buildWrapper(newList[id])
                      for each (id in Object.keys(newList)) if (!(id in this.plugins))];
    let matchedIDs = [id for each (id in Object.keys(newList)) if (id in this.plugins)];

    
    
    
    
    let changedWrappers = [];
    for (let id of matchedIDs) {
      let oldWrapper = this.buildWrapper(this.plugins[id]);
      let newWrapper = this.buildWrapper(newList[id]);

      if (newWrapper.isActive != oldWrapper.isActive) {
        AddonManagerPrivate.callAddonListeners(newWrapper.isActive ?
                                               "onEnabling" : "onDisabling",
                                               newWrapper, false);
        changedWrappers.push(newWrapper);
      }
    }

    
    for (let plugin of newPlugins) {
      AddonManagerPrivate.callInstallListeners("onExternalInstall", null,
                                               plugin, null, false);
      AddonManagerPrivate.callAddonListeners("onInstalling", plugin, false);
    }

    
    for (let plugin of lostPlugins)
      AddonManagerPrivate.callAddonListeners("onUninstalling", plugin, false);

    this.plugins = newList;

    
    for (let plugin of newPlugins)
      AddonManagerPrivate.callAddonListeners("onInstalled", plugin);

    
    for (let wrapper of changedWrappers) {
      AddonManagerPrivate.callAddonListeners(wrapper.isActive ?
                                             "onEnabled" : "onDisabled",
                                             wrapper);
    }

    
    for (let plugin of lostPlugins)
      AddonManagerPrivate.callAddonListeners("onUninstalled", plugin);
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
      while ((entry = entries.nextFile)) {
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

  this.__defineGetter__("pluginFullpath", function() {
    let paths = [];
    for (let tag of aTags)
      paths.push(tag.fullpath);
    return paths;
  })

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
  optionsType: AddonManager.OPTIONS_TYPE_INLINE_INFO,
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
