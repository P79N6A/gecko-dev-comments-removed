



































var EXPORTED_SYMBOLS = ["Addon", "STATE_ENABLED", "STATE_DISABLED"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;

CU.import("resource://gre/modules/AddonManager.jsm");
CU.import("resource://gre/modules/AddonRepository.jsm");
CU.import("resource://gre/modules/Services.jsm");
CU.import("resource://services-sync/async.js");
CU.import("resource://services-sync/util.js");
CU.import("resource://tps/logger.jsm");
var XPIProvider = CU.import("resource://gre/modules/XPIProvider.jsm")
                  .XPIProvider;

const ADDONSGETURL = 'http://127.0.0.1:4567/';
const STATE_ENABLED = 1;
const STATE_DISABLED = 2;

function GetFileAsText(file)
{
  let channel = Services.io.newChannel(file, null, null);
  let inputStream = channel.open();
  if (channel instanceof CI.nsIHttpChannel && 
      channel.responseStatus != 200) {
    return "";
  }

  let streamBuf = "";
  let sis = CC["@mozilla.org/scriptableinputstream;1"]
            .createInstance(CI.nsIScriptableInputStream);
  sis.init(inputStream);

  let available;
  while ((available = sis.available()) != 0) {
    streamBuf += sis.read(available);
  }

  inputStream.close();
  return streamBuf;
}

function Addon(TPS, id) {
  this.TPS = TPS;
  this.id = id;
}

Addon.prototype = {
  _addons_requiring_restart: [],
  _addons_pending_install: [],

  Delete: function() {
    
    let cb = Async.makeSyncCallback();
    XPIProvider.getAddonsByTypes(null, cb);
    let results =  Async.waitForSyncCallback(cb);
    var addon;
    var id = this.id;
    results.forEach(function(result) {
      if (result.id == id) {
        addon = result;
      }
    });
    Logger.AssertTrue(!!addon, 'could not find addon ' + this.id + ' to uninstall');
    addon.uninstall();
  },

  Find: function(state) {
    let cb = Async.makeSyncCallback();
    let addon_found = false;
    var that = this;

    var log_addon = function(addon) {
      that.addon = addon;
      Logger.logInfo('addon ' + addon.id + ' found, isActive: ' + addon.isActive);
      if (state == STATE_ENABLED || state == STATE_DISABLED) {
          Logger.AssertEqual(addon.isActive,
            state == STATE_ENABLED ? true : false,
            "addon " + that.id + " has an incorrect enabled state");
      }
    };

    
    XPIProvider.getAddonsByTypes(null, cb);
    let addonlist = Async.waitForSyncCallback(cb);
    addonlist.forEach(function(addon) {
      if (addon.id == that.id) {
        addon_found = true;
        log_addon.call(that, addon);
      }
    });

    if (!addon_found) {
      
      cb = Async.makeSyncCallback();
      XPIProvider.getInstallsByTypes(null, cb);
      addonlist = Async.waitForSyncCallback(cb);
      for (var i in addonlist) {
        if (addonlist[i].addon && addonlist[i].addon.id == that.id &&
            addonlist[i].state == AddonManager.STATE_INSTALLED) {
          addon_found = true;
          log_addon.call(that, addonlist[i].addon);
        }
      }
    }

    return addon_found;
  },

  Install: function() {
    
    
    let url = this.id;

    
    var prefs = CC["@mozilla.org/preferences-service;1"]
                .getService(CI.nsIPrefBranch);
    prefs.setCharPref('extensions.getAddons.get.url', ADDONSGETURL + url);

    
    xml = GetFileAsText(ADDONSGETURL + url);
    Logger.AssertTrue(xml.indexOf("<guid>") > -1, 'guid not found in ' + url);
    this.id = xml.substring(xml.indexOf("<guid>") + 6, xml.indexOf("</guid"));
    Logger.logInfo('addon XML = ' + this.id);

    
    let cb = Async.makeSyncCallback();
    AddonRepository.getAddonsByIDs([this.id], {
      searchSucceeded: cb,
      searchFailed: cb
    }, false);

    
    
    let install_addons = Async.waitForSyncCallback(cb);

    Logger.AssertTrue(install_addons,
                      "no addons found for id " + this.id);
    Logger.AssertEqual(install_addons.length,
                       1,
                       "multiple addons found for id " + this.id);

    let addon = install_addons[0];
    Logger.logInfo(JSON.stringify(addon), null, ' ');
    if (XPIProvider.installRequiresRestart(addon)) {
      this._addons_requiring_restart.push(addon.id);
    }

    
    
    this._addons_pending_install.push(addon.id);
    this.TPS.StartAsyncOperation();

    Utils.nextTick(function() {
      let callback = function(aInstall) {
        addon.install = aInstall;
        Logger.logInfo("addon install: " + addon.install);
        Logger.AssertTrue(addon.install,
                          "could not get install object for id " + this.id);
        addon.install.addListener(this);
        addon.install.install();
      };

      AddonManager.getInstallForURL(addon.sourceURI.spec,
                                    callback.bind(this),
                                    "application/x-xpinstall");
    }, this);
  },

  SetState: function(state) {
    if (!this.Find())
      return false;
    this.addon.userDisabled = state == STATE_ENABLED ? false : true;
      return true;
  },

  
  onInstallEnded: function(addon) {
    try {
      Logger.logInfo('--------- event observed: addon onInstallEnded');
      Logger.AssertTrue(addon.addon,
        "No addon object in addon instance passed to onInstallEnded");
      Logger.AssertTrue(this._addons_pending_install.indexOf(addon.addon.id) > -1,
        "onInstallEnded received for unexpected addon " + addon.addon.id);
      this._addons_pending_install.splice(
        this._addons_pending_install.indexOf(addon.addon.id),
        1);
    }
    catch(e) {
      
      
      Utils.nextTick(function() {
        this.DumpError(e);
      }, this);
      return;
    }
    this.TPS.FinishAsyncOperation();
  },

  onInstallFailed: function(addon) {
    Logger.logInfo('--------- event observed: addon onInstallFailed');
    Utils.nextTick(function() {
      this.DumpError('Installation failed for addon ' + 
        (addon.addon && addon.addon.id ? addon.addon.id : 'unknown'));
    }, this);
  },

  onDownloadFailed: function(addon) {
    Logger.logInfo('--------- event observed: addon onDownloadFailed');
    Utils.nextTick(function() {
      this.DumpError('Download failed for addon ' + 
        (addon.addon && addon.addon.id ? addon.addon.id : 'unknown'));
    }, this);
  },

};
