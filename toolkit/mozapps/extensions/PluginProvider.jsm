






































const Cc = Components.classes;
const Ci = Components.interfaces;

var EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/AddonManager.jsm");







function LOG(aStr) {
  dump("*** addons.plugins: " + aStr + "\n");
}







function WARN(aStr) {
  LOG(aStr);
}







function ERROR(aStr) {
  LOG(aStr);
}

var PluginProvider = {
  
  plugins: null,

  







  getAddonByID: function PL_getAddon(aId, aCallback) {
    if (!this.plugins)
      this.buildPluginList();

    if (aId in this.plugins) {
      let name = this.plugins[aId].name;
      let description = this.plugins[aId].description;

      let tags = Cc["@mozilla.org/plugin/host;1"].
                 getService(Ci.nsIPluginHost).
                 getPluginTags({});
      let selected = [];
      tags.forEach(function(aTag) {
        if (aTag.name == name && aTag.description == description)
          selected.push(aTag);
      }, this);

      aCallback(new PluginWrapper(aId, name, description, selected));
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
    let seen = {};
    tags.forEach(function(aTag) {
      if (!(aTag.name in seen))
        seen[aTag.name] = {};
      if (!(aTag.description in seen[aTag.name])) {
        let id = Cc["@mozilla.org/uuid-generator;1"].
                 getService(Ci.nsIUUIDGenerator).
                 generateUUID();
        this.plugins[id] = {
          name: aTag.name,
          description: aTag.description
        };
        seen[aTag.name][aTag.description] = true;
      }
    }, this);
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
  this.__defineGetter__("description", function() safedesc);
  this.__defineGetter__("version", function() aTags[0].version);
  this.__defineGetter__("homepageURL", function() homepageURL);

  this.__defineGetter__("isActive", function() !aTags[0].blocklisted && !aTags[0].disabled);
  this.__defineGetter__("isCompatible", function() true);
  this.__defineGetter__("appDisabled", function() aTags[0].blocklisted);
  this.__defineGetter__("userDisabled", function() aTags[0].disabled);
  this.__defineSetter__("userDisabled", function(aVal) {
    if (aTags[0].disabled == aVal)
      return;

    aTags.forEach(function(aTag) {
      aTag.disabled = aVal;
    });
    AddonManagerPrivate.callAddonListeners(aVal ? "onDisabling" : "onEnabling", this, false);
    AddonManagerPrivate.callAddonListeners(aVal ? "onDisabled" : "onEnabled", this);
    return aVal;
  });

  this.__defineGetter__("pendingOperations", function() {
    return 0;
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

  this.uninstall = function() {
    throw new Error("Cannot uninstall plugins");
  };

  this.cancelUninstall = function() {
    throw new Error("Plugin is not marked to be uninstalled");
  };

  this.findUpdates = function(aListener, aReason, aAppVersion, aPlatformVersion) {
    throw new Error("Cannot search for updates for plugins");
  };

  this.hasResource = function(aPath) {
    return false;
  },

  this.getResourceURL = function(aPath) {
    return null;
  }
}

PluginWrapper.prototype = { };

AddonManagerPrivate.registerProvider(PluginProvider);
