






































const Cc = Components.classes;
const Ci = Components.interfaces;

var EXPORTED_SYMBOLS = [];

Components.utils.import("resource://gre/modules/AddonManager.jsm");






function LOG(str) {
  dump("*** addons.plugins: " + str + "\n");
}






function WARN(str) {
  LOG(str);
}






function ERROR(str) {
  LOG(str);
}

var PluginProvider = {
  
  plugins: null,

  






  getAddon: function PL_getAddon(id, callback) {
    if (!this.plugins)
      this.buildPluginList();

    if (id in this.plugins) {
      let name = this.plugins[id].name;
      let description = this.plugins[id].description;

      let tags = Cc["@mozilla.org/plugin/host;1"].
                 getService(Ci.nsIPluginHost).
                 getPluginTags({});
      let selected = [];
      tags.forEach(function(tag) {
        if (tag.name == name && tag.description == description)
          selected.push(tag);
      }, this);

      callback(new PluginWrapper(id, name, description, selected));
    }
    else {
      callback(null);
    }
  },

  






  getAddonsByTypes: function PL_getAddonsByTypes(types, callback) {
    if (types && types.indexOf("plugin") < 0) {
      callback([]);
      return;
    }

    if (!this.plugins)
      this.buildPluginList();

    let results = [];

    for (let id in this.plugins) {
      this.getAddon(id, function(addon) {
        results.push(addon);
      });
    }

    callback(results);
  },

  






  getAddonsWithPendingOperations: function PL_getAddonsWithPendingOperations(types, callback) {
    callback([]);
  },

  






  getInstalls: function PL_getInstalls(types, callback) {
    callback([]);
  },

  buildPluginList: function PL_buildPluginList() {
    let tags = Cc["@mozilla.org/plugin/host;1"].
               getService(Ci.nsIPluginHost).
               getPluginTags({});

    this.plugins = {};
    let seen = {};
    tags.forEach(function(tag) {
      if (!(tag.name in seen))
        seen[tag.name] = {};
      if (!(tag.description in seen[tag.name])) {
        let id = Cc["@mozilla.org/uuid-generator;1"].
                 getService(Ci.nsIUUIDGenerator).
                 generateUUID();
        this.plugins[id] = {
          name: tag.name,
          description: tag.description
        };
        seen[tag.name][tag.description] = true;
      }
    }, this);
  }
};





function PluginWrapper(id, name, description, tags) {
  let safedesc = description.replace(/<\/?[a-z][^>]*>/gi, " ");
  let homepageURL = null;
  if (/<A\s+HREF=[^>]*>/i.test(description))
    homepageURL = /<A\s+HREF=["']?([^>"'\s]*)/i.exec(description)[1];

  this.__defineGetter__("id", function() id);
  this.__defineGetter__("type", function() "plugin");
  this.__defineGetter__("name", function() name);
  this.__defineGetter__("description", function() safedesc);
  this.__defineGetter__("version", function() tags[0].version);
  this.__defineGetter__("homepageURL", function() homepageURL);

  this.__defineGetter__("isActive", function() !tags[0].blocklisted && !tags[0].disabled);
  this.__defineGetter__("isCompatible", function() true);
  this.__defineGetter__("appDisabled", function() tags[0].blocklisted);
  this.__defineGetter__("userDisabled", function() tags[0].disabled);
  this.__defineSetter__("userDisabled", function(val) {
    if (tags[0].disabled == val)
      return;

    tags.forEach(function(tag) {
      tag.disabled = val;
    });
    AddonManagerPrivate.callAddonListeners(val ? "onDisabling" : "onEnabling", this, false);
    AddonManagerPrivate.callAddonListeners(val ? "onDisabled" : "onEnabled", this);
    return val;
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

  this.findUpdates = function(listener, reason, appVersion, platformVersion) {
    throw new Error("Cannot search for updates for plugins");
  };

  this.hasResource = function(path) {
    return false;
  },

  this.getResourceURL = function(path) {
    return null;
  }
}

PluginWrapper.prototype = { };

AddonManagerPrivate.registerProvider(PluginProvider);
