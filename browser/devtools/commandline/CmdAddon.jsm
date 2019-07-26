



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

let EXPORTED_SYMBOLS = [ "Flags" ];

Cu.import("resource:///modules/devtools/gcli.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");



let Flags = {
  addonsLoaded: false
};




gcli.addCommand({
  name: "addon",
  description: gcli.lookup("addonDesc")
});




gcli.addCommand({
  name: "addon list",
  description: gcli.lookup("addonListDesc"),
  params: [{
    name: 'type',
    type: {
      name: 'selection',
      data: ["dictionary", "extension", "locale", "plugin", "theme", "all"]
    },
    defaultValue: 'all',
    description: gcli.lookup("addonListTypeDesc"),
  }],
  exec: function(aArgs, context) {
    function representEnabledAddon(aAddon) {
      return "<li><![CDATA[" + aAddon.name + "\u2002" + aAddon.version +
      getAddonStatus(aAddon) + "]]></li>";
    }

    function representDisabledAddon(aAddon) {
      return "<li class=\"gcli-addon-disabled\">" +
        "<![CDATA[" + aAddon.name + "\u2002" + aAddon.version + aAddon.version +
        "]]></li>";
    }

    function getAddonStatus(aAddon) {
      let operations = [];

      if (aAddon.pendingOperations & AddonManager.PENDING_ENABLE) {
        operations.push("PENDING_ENABLE");
      }

      if (aAddon.pendingOperations & AddonManager.PENDING_DISABLE) {
        operations.push("PENDING_DISABLE");
      }

      if (aAddon.pendingOperations & AddonManager.PENDING_UNINSTALL) {
        operations.push("PENDING_UNINSTALL");
      }

      if (aAddon.pendingOperations & AddonManager.PENDING_INSTALL) {
        operations.push("PENDING_INSTALL");
      }

      if (aAddon.pendingOperations & AddonManager.PENDING_UPGRADE) {
        operations.push("PENDING_UPGRADE");
      }

      if (operations.length) {
        return " (" + operations.join(", ") + ")";
      }
      return "";
    }

    


    function compareAddonNames(aNameA, aNameB) {
      return String.localeCompare(aNameA.name, aNameB.name);
    }

    



    function list(aType, aAddons) {
      if (!aAddons.length) {
        this.resolve(gcli.lookup("addonNoneOfType"));
      }

      
      let enabledAddons = [];
      let disabledAddons = [];

      aAddons.forEach(function(aAddon) {
        if (aAddon.isActive) {
          enabledAddons.push(aAddon);
        } else {
          disabledAddons.push(aAddon);
        }
      });

      let header;
      switch(aType) {
        case "dictionary":
          header = gcli.lookup("addonListDictionaryHeading");
          break;
        case "extension":
          header = gcli.lookup("addonListExtensionHeading");
          break;
        case "locale":
          header = gcli.lookup("addonListLocaleHeading");
          break;
        case "plugin":
          header = gcli.lookup("addonListPluginHeading");
          break;
        case "theme":
          header = gcli.lookup("addonListThemeHeading");
        case "all":
          header = gcli.lookup("addonListAllHeading");
          break;
        default:
          header = gcli.lookup("addonListUnknownHeading");
      }

      
      this.resolve(header +
        "<ol>" +
        enabledAddons.sort(compareAddonNames).map(representEnabledAddon).join("") +
        disabledAddons.sort(compareAddonNames).map(representDisabledAddon).join("") +
        "</ol>");
    }

    
    
    let promise = context.createPromise();
    let types = aArgs.type == "all" ? null : [aArgs.type];
    AddonManager.getAddonsByTypes(types, list.bind(promise, aArgs.type));
    return promise;
  }
});




AddonManager.getAllAddons(function addonAsync(aAddons) {
  
  
  
  AddonManager.addAddonListener({
    onInstalled: function(aAddon) {
      addonNameCache.push({
        name: representAddon(aAddon).replace(/\s/g, "_"),
        value: aAddon.name
      });
    },
    onUninstalled: function(aAddon) {
      let name = representAddon(aAddon).replace(/\s/g, "_");

      for (let i = 0; i < addonNameCache.length; i++) {
        if(addonNameCache[i].name == name) {
          addonNameCache.splice(i, 1);
          break;
        }
      }
    },
  });

  


  function representAddon(aAddon) {
    let name = aAddon.name + " " + aAddon.version;
    return name.trim();
  }

  let addonNameCache = [];

  
  let nameParameter = {
    name: "name",
    type: {
      name: "selection",
      lookup: addonNameCache
    },
    description: gcli.lookup("addonNameDesc")
  };

  for (let addon of aAddons) {
    addonNameCache.push({
      name: representAddon(addon).replace(/\s/g, "_"),
      value: addon.name
    });
  }

  


  gcli.addCommand({
    name: "addon enable",
    description: gcli.lookup("addonEnableDesc"),
    params: [nameParameter],
    exec: function(aArgs, context) {
      





      function enable(aName, addons) {
        
        let addon = null;
        addons.some(function(candidate) {
          if (candidate.name == aName) {
            addon = candidate;
            return true;
          } else {
            return false;
          }
        });

        let name = representAddon(addon);

        if (!addon.userDisabled) {
          this.resolve("<![CDATA[" +
            gcli.lookupFormat("addonAlreadyEnabled", [name]) + "]]>");
        } else {
          addon.userDisabled = false;
          
          this.resolve("<![CDATA[" +
            gcli.lookupFormat("addonEnabled", [name]) + "]]>");
        }
      }

      let promise = context.createPromise();
      
      AddonManager.getAllAddons(enable.bind(promise, aArgs.name));
      return promise;
    }
  });

  


  gcli.addCommand({
    name: "addon disable",
    description: gcli.lookup("addonDisableDesc"),
    params: [nameParameter],
    exec: function(aArgs, context) {
      


      function disable(aName, addons) {
        
        let addon = null;
        addons.some(function(candidate) {
          if (candidate.name == aName) {
            addon = candidate;
            return true;
          } else {
            return false;
          }
        });

        let name = representAddon(addon);

        if (addon.userDisabled) {
          this.resolve("<![CDATA[" +
            gcli.lookupFormat("addonAlreadyDisabled", [name]) + "]]>");
        } else {
          addon.userDisabled = true;
          
          this.resolve("<![CDATA[" +
            gcli.lookupFormat("addonDisabled", [name]) + "]]>");
        }
      }

      let promise = context.createPromise();
      
      AddonManager.getAllAddons(disable.bind(promise, aArgs.name));
      return promise;
    }
  });
  Flags.addonsLoaded = true;
  Services.obs.notifyObservers(null, "gcli_addon_commands_ready", null);
});
