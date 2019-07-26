



module.metadata = {
  "stability": "experimental"
};

const { Cc, Ci, Cu } = require("chrome");
const { AddonManager } = Cu.import("resource://gre/modules/AddonManager.jsm");
const { defer } = require("../core/promise");
const { setTimeout } = require("../timers");






exports.ERROR_NETWORK_FAILURE = AddonManager.ERROR_NETWORK_FAILURE;
exports.ERROR_INCORRECT_HASH = AddonManager.ERROR_INCORRECT_HASH;
exports.ERROR_CORRUPT_FILE = AddonManager.ERROR_CORRUPT_FILE;
exports.ERROR_FILE_ACCESS = AddonManager.ERROR_FILE_ACCESS;










exports.install = function install(xpiPath) {
  let { promise, resolve, reject } = defer();

  
  let file = Cc['@mozilla.org/file/local;1'].createInstance(Ci.nsILocalFile);
  try {
    file.initWithPath(xpiPath);
  }
  catch(e) {
    reject(exports.ERROR_FILE_ACCESS);
    return promise;
  }

  
  let listener = {
    onInstallEnded: function(aInstall, aAddon) {
      aInstall.removeListener(listener);
      
      
      
      
      setTimeout(resolve, 0, aAddon.id);
    },
    onInstallFailed: function (aInstall) {
      console.log("failed");
      aInstall.removeListener(listener);
      reject(aInstall.error);
    },
    onDownloadFailed: function(aInstall) {
      this.onInstallFailed(aInstall);
    }
  };

  
  AddonManager.getInstallForFile(file, function(install) {
    if (install.error != null) {
      install.addListener(listener);
      install.install();
    } else {
      reject(install.error);
    }
  });

  return promise;
};

exports.uninstall = function uninstall(addonId) {
  let { promise, resolve, reject } = defer();

  
  let listener = {
    onUninstalled: function onUninstalled(aAddon) {
      if (aAddon.id != addonId)
        return;
      AddonManager.removeAddonListener(listener);
      resolve();
    }
  };
  AddonManager.addAddonListener(listener);

  
  getAddon(addonId).then(addon => addon.uninstall(), reject);

  return promise;
};

exports.disable = function disable(addonId) {
  return getAddon(addonId).then(addon => {
    addon.userDisabled = true;
    return addonId;
  });
};

exports.enable = function enabled(addonId) {
  return getAddon(addonId).then(addon => {
    addon.userDisabled = false;
    return addonId;
  });
};

exports.isActive = function isActive(addonId) {
  return getAddon(addonId).then(addon => addon.isActive && !addon.appDisabled);
};

function getAddon (id) {
  let { promise, resolve, reject } = defer();
  AddonManager.getAddonByID(id, addon => addon ? resolve(addon) : reject());
  return promise;
}
