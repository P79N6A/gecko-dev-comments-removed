






"use strict";

this.EXPORTED_SYMBOLS = [
  "AddonTestUtils",
];

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");

this.AddonTestUtils = {
  





  getAddonById: function (id) {
    return new Promise(resolve => AddonManager.getAddonByID(id, addon => resolve(addon)));
  },

  







  uninstallAddonByID: function (id) {
    let deferred = Promise.defer();

    AddonManager.getAddonByID(id, (addon) => {
      if (!addon) {
        deferred.reject(new Error("Add-on is not known: " + id));
        return;
      }

      let listener = {
        onUninstalling: function (addon, needsRestart) {
          if (addon.id != id) {
            return;
          }

          if (needsRestart) {
            AddonManager.removeAddonListener(listener);
            deferred.resolve(true);
          }
        },

        onUninstalled: function (addon) {
          if (addon.id != id) {
            return;
          }

          AddonManager.removeAddonListener(listener);
          deferred.resolve(false);
        },

        onOperationCancelled: function (addon) {
          if (addon.id != id) {
            return;
          }

          AddonManager.removeAddonListener(listener);
          deferred.reject(new Error("Uninstall cancelled."));
        },
      };

      AddonManager.addAddonListener(listener);
      addon.uninstall();
    });

    return deferred.promise;
  },

  




  installXPIFromURL: function (url, hash, name, iconURL, version) {
    let deferred = Promise.defer();

    AddonManager.getInstallForURL(url, (install) => {
      let fail = () => { deferred.reject(new Error("Add-on install failed.")) };

      let listener = {
        onDownloadCancelled: fail,
        onDownloadFailed: fail,
        onInstallCancelled: fail,
        onInstallFailed: fail,
        onInstallEnded: function (install, addon) {
          deferred.resolve(addon);
        },
      };

      install.addListener(listener);
      install.install();
    }, "application/x-xpinstall", hash, name, iconURL, version);

    return deferred.promise;
  },
};
