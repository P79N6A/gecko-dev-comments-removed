




































var EXPORTED_SYMBOLS = ["TPS", "SYNC_WIPE_SERVER", "SYNC_RESET_CLIENT",
                        "SYNC_WIPE_CLIENT"];

const CC = Components.classes;
const CI = Components.interfaces;
const CU = Components.utils;

CU.import("resource://gre/modules/XPCOMUtils.jsm");
CU.import("resource://gre/modules/Services.jsm");
CU.import("resource://tps/logger.jsm");
CU.import("resource://services-sync/service.js");
CU.import("resource://services-sync/util.js");
var utils = {}; CU.import('resource://mozmill/modules/utils.js', utils);

const SYNC_WIPE_SERVER = "wipe-server";
const SYNC_RESET_CLIENT = "reset-client";
const SYNC_WIPE_CLIENT = "wipe-client";

var prefs = CC["@mozilla.org/preferences-service;1"]
            .getService(CI.nsIPrefBranch);

var syncFinishedCallback = function() {
  Logger.logInfo('syncFinishedCallback returned ' + !TPS._waitingForSync);
  return !TPS._waitingForSync;
};

var TPS = {
  _waitingForSync: false,
  _syncErrors: 0,

  QueryInterface: XPCOMUtils.generateQI([CI.nsIObserver,
                                         CI.nsISupportsWeakReference]),

  observe: function TPS__observe(subject, topic, data) {
    Logger.logInfo('Mozmill observed: ' + topic);
    switch(topic) {
      case "weave:service:sync:error":
        if (this._waitingForSync && this._syncErrors == 0) {
          Logger.logInfo("sync error; retrying...");
          this._syncErrors++;
          Utils.namedTimer(function() {
            Weave.service.sync();
          }, 1000, this, "resync");
        }
        else if (this._waitingForSync) {
          this._syncErrors = "sync error, see log";
          this._waitingForSync = false;
        }
        break;
      case "weave:service:sync:finish":
        if (this._waitingForSync) {
          this._syncErrors = 0;
          this._waitingForSync = false;
        }
        break;
    }
  },

  SetupSyncAccount: function TPS__SetupSyncAccount() {
    try {
      let serverURL = prefs.getCharPref('tps.account.serverURL');
      if (serverURL) {
        Weave.Service.serverURL = serverURL;
      }
    }
    catch(e) {}
    Weave.Service.account = prefs.getCharPref('tps.account.username');
    Weave.Service.password = prefs.getCharPref('tps.account.password');
    Weave.Service.passphrase = prefs.getCharPref('tps.account.passphrase');
    Weave.Svc.Obs.notify("weave:service:setup-complete");
  },

  Sync: function TPS__Sync(options) {
    Logger.logInfo('Mozmill starting sync operation: ' + options);
    switch(options) {
      case SYNC_WIPE_SERVER:
        Weave.Svc.Prefs.set("firstSync", "wipeRemote");
        break;
      case SYNC_WIPE_CLIENT:
        Weave.Svc.Prefs.set("firstSync", "wipeClient");
        break;
      case SYNC_RESET_CLIENT:
        Weave.Svc.Prefs.set("firstSync", "resetClient");
        break;
      default:
        Weave.Svc.Prefs.reset("firstSync");
    }

    if (Weave.Status.service != Weave.STATUS_OK) {
      return "Sync status not ok: " + Weave.Status.service;
    }

    this._waitingForSync = true;
    this._syncErrors = 0;
    Weave.Service.sync();
    utils.waitFor(syncFinishedCallback, null, 20000, 500, TPS);
    return this._syncErrors;
  },
};

Services.obs.addObserver(TPS, "weave:service:sync:finish", true);
Services.obs.addObserver(TPS, "weave:service:sync:error", true);
Logger.init();


