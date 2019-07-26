



"use strict";

this.EXPORTED_SYMBOLS = ["AddonUtils"];

const {interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://services-common/log4moz.js");
Cu.import("resource://services-sync/util.js");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonRepository",
  "resource://gre/modules/AddonRepository.jsm");

function AddonUtilsInternal() {
  this._log = Log4Moz.repository.getLogger("Sync.AddonUtils");
  this._log.Level = Log4Moz.Level[Svc.Prefs.get("log.logger.addonutils")];
}
AddonUtilsInternal.prototype = {
  














  getInstallFromSearchResult:
    function getInstallFromSearchResult(addon, cb, requireSecureURI=true) {

    this._log.debug("Obtaining install for " + addon.id);

    
    
    
    if (requireSecureURI) {
      let scheme = addon.sourceURI.scheme;
      if (scheme != "https") {
        cb(new Error("Insecure source URI scheme: " + scheme), addon.install);
        return;
      }
    }

    
    
    
    
    
    AddonManager.getInstallForURL(
      addon.sourceURI.spec,
      function handleInstall(install) {
        cb(null, install);
      },
      "application/x-xpinstall",
      undefined,
      addon.name,
      addon.iconURL,
      addon.version
    );
  },

  





























  installAddonFromSearchResult:
    function installAddonFromSearchResult(addon, options, cb) {
    this._log.info("Trying to install add-on from search result: " + addon.id);

    if (options.requireSecureURI === undefined) {
      options.requireSecureURI = true;
    }

    this.getInstallFromSearchResult(addon, function onResult(error, install) {
      if (error) {
        cb(error, null);
        return;
      }

      if (!install) {
        cb(new Error("AddonInstall not available: " + addon.id), null);
        return;
      }

      try {
        this._log.info("Installing " + addon.id);
        let log = this._log;

        let listener = {
          onInstallStarted: function onInstallStarted(install) {
            if (!options) {
              return;
            }

            if (options.syncGUID) {
              log.info("Setting syncGUID of " + install.name  +": " +
                       options.syncGUID);
              install.addon.syncGUID = options.syncGUID;
            }

            
            
            if ("enabled" in options && !options.enabled) {
              log.info("Marking add-on as disabled for install: " +
                       install.name);
              install.addon.userDisabled = true;
            }
          },
          onInstallEnded: function(install, addon) {
            install.removeListener(listener);

            cb(null, {id: addon.id, install: install, addon: addon});
          },
          onInstallFailed: function(install) {
            install.removeListener(listener);

            cb(new Error("Install failed: " + install.error), null);
          },
          onDownloadFailed: function(install) {
            install.removeListener(listener);

            cb(new Error("Download failed: " + install.error), null);
          }
        };
        install.addListener(listener);
        install.install();
      }
      catch (ex) {
        this._log.error("Error installing add-on: " + Utils.exceptionstr(ex));
        cb(ex, null);
      }
    }.bind(this), options.requireSecureURI);
  },

  








  uninstallAddon: function uninstallAddon(addon, cb) {
    let listener = {
      onUninstalling: function(uninstalling, needsRestart) {
        if (addon.id != uninstalling.id) {
          return;
        }

        
        
        if (!needsRestart) {
          return;
        }

        
        
        AddonManager.removeAddonListener(listener);
        cb(null, addon);
      },
      onUninstalled: function(uninstalled) {
        if (addon.id != uninstalled.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        cb(null, addon);
      }
    };
    AddonManager.addAddonListener(listener);
    addon.uninstall();
  },

  































  installAddons: function installAddons(installs, cb) {
    if (!cb) {
      throw new Error("Invalid argument: cb is not defined.");
    }

    let ids = [];
    for each (let addon in installs) {
      ids.push(addon.id);
    }

    AddonRepository.getAddonsByIDs(ids, {
      searchSucceeded: function searchSucceeded(addons, addonsLength, total) {
        this._log.info("Found " + addonsLength + "/" + ids.length +
                       " add-ons during repository search.");

        let ourResult = {
          installedIDs: [],
          installs:     [],
          addons:       [],
          errors:       []
        };

        if (!addonsLength) {
          cb(null, ourResult);
          return;
        }

        let expectedInstallCount = 0;
        let finishedCount = 0;
        let installCallback = function installCallback(error, result) {
          finishedCount++;

          if (error) {
            ourResult.errors.push(error);
          } else {
            ourResult.installedIDs.push(result.id);
            ourResult.installs.push(result.install);
            ourResult.addons.push(result.addon);
          }

          if (finishedCount >= expectedInstallCount) {
            if (ourResult.errors.length > 0) {
              cb(new Error("1 or more add-ons failed to install"), ourResult);
            } else {
              cb(null, ourResult);
            }
          }
        }.bind(this);

        let toInstall = [];

        
        
        
        
        
        for each (let addon in addons) {
          
          
          if (!addon.sourceURI) {
            this._log.info("Skipping install of add-on because missing " +
                           "sourceURI: " + addon.id);
            continue;
          }

          toInstall.push(addon);

          
          
          
          try {
            addon.sourceURI.QueryInterface(Ci.nsIURL);
          } catch (ex) {
            this._log.warn("Unable to QI sourceURI to nsIURL: " +
                           addon.sourceURI.spec);
            continue;
          }

          let params = addon.sourceURI.query.split("&").map(
            function rewrite(param) {

            if (param.indexOf("src=") == 0) {
              return "src=sync";
            } else {
              return param;
            }
          });

          addon.sourceURI.query = params.join("&");
        }

        expectedInstallCount = toInstall.length;

        if (!expectedInstallCount) {
          cb(null, ourResult);
          return;
        }

        
        
        for each (let addon in toInstall) {
          let options = {};
          for each (let install in installs) {
            if (install.id == addon.id) {
              options = install;
              break;
            }
          }

          this.installAddonFromSearchResult(addon, options, installCallback);
        }

      }.bind(this),

      searchFailed: function searchFailed() {
        cb(new Error("AddonRepository search failed"), null);
      },
    });
  },

  




















  updateUserDisabled: function updateUserDisabled(addon, value, cb) {
    if (addon.userDisabled == value) {
      cb(null, addon);
      return;
    }

    let listener = {
      onEnabling: function onEnabling(wrapper, needsRestart) {
        this._log.debug("onEnabling: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        
        if (!needsRestart) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        cb(null, wrapper);
      }.bind(this),

      onEnabled: function onEnabled(wrapper) {
        this._log.debug("onEnabled: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        cb(null, wrapper);
      }.bind(this),

      onDisabling: function onDisabling(wrapper, needsRestart) {
        this._log.debug("onDisabling: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        if (!needsRestart) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        cb(null, wrapper);
      }.bind(this),

      onDisabled: function onDisabled(wrapper) {
        this._log.debug("onDisabled: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        cb(null, wrapper);
      }.bind(this),

      onOperationCancelled: function onOperationCancelled(wrapper) {
        this._log.debug("onOperationCancelled: " + wrapper.id);
        if (wrapper.id != addon.id) {
          return;
        }

        AddonManager.removeAddonListener(listener);
        cb(new Error("Operation cancelled"), wrapper);
      }.bind(this)
    };

    
    

    if (!addon.appDisabled) {
      AddonManager.addAddonListener(listener);
    }

    this._log.info("Updating userDisabled flag: " + addon.id + " -> " + value);
    addon.userDisabled = !!value;

    if (!addon.appDisabled) {
      cb(null, addon);
      return;
    }
    
  },

};

XPCOMUtils.defineLazyGetter(this, "AddonUtils", function() {
  return new AddonUtilsInternal();
});
