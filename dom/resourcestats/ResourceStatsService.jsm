




"use strict";

this.EXPORTED_SYMBOLS = ["ResourceStatsService"];

const DEBUG = false;
function debug(s) { dump("-*- ResourceStatsService: " + s + "\n"); }

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

Cu.import("resource://gre/modules/ResourceStatsDB.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gIDBManager",
                                   "@mozilla.org/dom/indexeddb/manager;1",
                                   "nsIIndexedDatabaseManager");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

this.ResourceStatsService = {

  init: function() {
    if (DEBUG) {
      debug("Service started");
    }

    
    Services.obs.addObserver(this, "xpcom-shutdown", false);

    
    this.messages = ["ResourceStats:GetStats",
                     "ResourceStats:ClearStats",
                     "ResourceStats:ClearAllStats",
                     "ResourceStats:AddAlarm",
                     "ResourceStats:GetAlarms",
                     "ResourceStats:RemoveAlarm",
                     "ResourceStats:RemoveAllAlarms",
                     "ResourceStats:GetComponents",
                     "ResourceStats:SampleRate",
                     "ResourceStats:MaxStorageAge"];

    this.messages.forEach(function(aMessageName){
      ppmm.addMessageListener(aMessageName, this);
    }, this);

    
    this._db = new ResourceStatsDB();
  },

  receiveMessage: function(aMessage) {
    if (DEBUG) {
      debug("receiveMessage(): " + aMessage.name);
    }

    let mm = aMessage.target;
    let data = aMessage.data;

    if (DEBUG) {
      debug("received aMessage.data = " + JSON.stringify(data));
    }

    
    
    if (!aMessage.target.assertPermission("resourcestats-manage")) {
      return;
    }
    if (!aMessage.target.assertContainApp(data.manifestURL)) {
      if (DEBUG) {
        debug("Got msg from a child process containing illegal manifestURL.");
      }
      return;
    }

    switch (aMessage.name) {
      case "ResourceStats:GetStats":
        this.getStats(mm, data);
        break;
      case "ResourceStats:ClearStats":
        this.clearStats(mm, data);
        break;
      case "ResourceStats:ClearAllStats":
        this.clearAllStats(mm, data);
        break;
      case "ResourceStats:AddAlarm":
        this.addAlarm(mm, data);
        break;
      case "ResourceStats:GetAlarms":
        this.getAlarms(mm, data);
        break;
      case "ResourceStats:RemoveAlarm":
        this.removeAlarm(mm, data);
        break;
      case "ResourceStats:RemoveAllAlarms":
        this.removeAllAlarms(mm, data);
        break;
      case "ResourceStats:GetComponents":
        this.getComponents(mm, data);
        break;
      case "ResourceStats:SampleRate":
        
        return this._db.sampleRate;
      case "ResourceStats:MaxStorageAge":
        
        return this._db.maxStorageAge;
    }
  },

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "xpcom-shutdown":
        if (DEBUG) {
          debug("Service shutdown " + aData);
        }

        this.messages.forEach(function(aMessageName) {
          ppmm.removeMessageListener(aMessageName, this);
        }, this);

        Services.obs.removeObserver(this, "xpcom-shutdown");
        break;
      default:
        return;
    }
  },

  
  _createDbCallback: function(aMm, aId, aMessage) {
    let resolveMsg = aMessage + ":Resolved";
    let rejectMsg = aMessage + ":Rejected";

    return function(aError, aResult) {
      if (aError) {
        aMm.sendAsyncMessage(rejectMsg, { resolverId: aId, reason: aError });
        return;
      }
      aMm.sendAsyncMessage(resolveMsg, { resolverId: aId, value: aResult });
    };
  },

  getStats: function(aMm, aData) {
    if (DEBUG) {
      debug("getStats(): " + JSON.stringify(aData));
    }

    
    let manifestURL = aData.statsOptions.manifestURL || "";
    let serviceType = aData.statsOptions.serviceType || "";
    let component = aData.statsOptions.component || "";

    
    let onStatsGot = this._createDbCallback(aMm, aData.resolverId,
                                            "ResourceStats:GetStats");
    this._db.getStats(aData.type, manifestURL, serviceType, component,
                      aData.start, aData.end, onStatsGot);
  },

  clearStats: function(aMm, aData) {
    if (DEBUG) {
      debug("clearStats(): " + JSON.stringify(aData));
    }

    
    let appId = 0;
    let manifestURL = aData.statsOptions.manifestURL || "";
    if (manifestURL) {
      appId = appsService.getAppLocalIdByManifestURL(manifestURL);

      if (!appId) {
        aMm.sendAsyncMessage("ResourceStats:GetStats:Rejected", {
          resolverId: aData.resolverId,
          reason: "Invalid manifestURL"
        });
        return;
      }
    }

    let serviceType = aData.statsOptions.serviceType || "";
    let component = aData.statsOptions.component || "";

    
    let onStatsCleared = this._createDbCallback(aMm, aData.resolverId,
                                                "ResourceStats:ClearStats");
    this._db.clearStats(aData.type, appId, serviceType, component,
                        aData.start, aData.end, onStatsCleared);
  },

  clearAllStats: function(aMm, aData) {
    if (DEBUG) {
      debug("clearAllStats(): " + JSON.stringify(aData));
    }

    
    let onAllStatsCleared = this._createDbCallback(aMm, aData.resolverId,
                                                   "ResourceStats:ClearAllStats");
    this._db.clearAllStats(aData.type, onAllStatsCleared);
  },

  addAlarm: function(aMm, aData) {
    if (DEBUG) {
      debug("addAlarm(): " + JSON.stringify(aData));
    }

    
    let manifestURL = aData.statsOptions.manifestURL;
    if (manifestURL) {
      let appId = appsService.getAppLocalIdByManifestURL(manifestURL);

      if (!appId) {
        aMm.sendAsyncMessage("ResourceStats:GetStats:Rejected", {
          resolverId: aData.resolverId,
          reason: "Invalid manifestURL"
        });
        return;
      }
    }

    
    let newAlarm = {
      type: aData.type,
      component: aData.statsOptions.component || "",
      serviceType: aData.statsOptions.serviceType || "",
      manifestURL: manifestURL || "",
      threshold: aData.threshold,
      startTime: aData.startTime,
      data: aData.data
    };

    
    let onAlarmAdded = this._createDbCallback(aMm, aData.resolverId,
                                              "ResourceStats:AddAlarm");
    this._db.addAlarm(newAlarm, onAlarmAdded);
  },

  getAlarms: function(aMm, aData) {
    if (DEBUG) {
      debug("getAlarms(): " + JSON.stringify(aData));
    }

    let options = null;
    let statsOptions = aData.statsOptions;
    
    
    
    if (statsOptions.manifestURL || statsOptions.serviceType ||
        statsOptions.component) {
      
      let manifestURL = statsOptions.manifestURL || "";
      if (manifestURL) {
        let appId = appsService.getAppLocalIdByManifestURL(manifestURL);

        if (!appId) {
          aMm.sendAsyncMessage("ResourceStats:GetStats:Rejected", {
            resolverId: aData.resolverId,
            reason: "Invalid manifestURL"
          });
          return;
        }
      }

      options = {
        manifestURL: manifestURL,
        serviceType: statsOptions.serviceType || "",
        component: statsOptions.component || ""
      };
    }

    
    let onAlarmsGot = this._createDbCallback(aMm, aData.resolverId,
                                            "ResourceStats:GetAlarms");
    this._db.getAlarms(aData.type, options, onAlarmsGot);
  },

  removeAlarm: function(aMm, aData) {
    if (DEBUG) {
      debug("removeAlarm(): " + JSON.stringify(aData));
    }

    
    let onAlarmRemoved = function(aError, aResult) {
      if (aError) {
        aMm.sendAsyncMessage("ResourceStats:RemoveAlarm:Rejected",
                             { resolverId: aData.resolverId, reason: aError });
      }

      if (!aResult) {
        aMm.sendAsyncMessage("ResourceStats:RemoveAlarm:Rejected",
                             { resolverId: aData.resolverId,
                               reason: "alarm not existed" });
      }

      aMm.sendAsyncMessage("ResourceStats:RemoveAlarm:Resolved",
                           { resolverId: aData.resolverId, value: aResult });
    };

    this._db.removeAlarm(aData.type, aData.alarmId, onAlarmRemoved);
  },

  removeAllAlarms: function(aMm, aData) {
    if (DEBUG) {
      debug("removeAllAlarms(): " + JSON.stringify(aData));
    }

    
    let onAllAlarmsRemoved = this._createDbCallback(aMm, aData.resolverId,
                                                    "ResourceStats:RemoveAllAlarms");
    this._db.removeAllAlarms(aData.type, onAllAlarmsRemoved);
  },

  getComponents: function(aMm, aData) {
    if (DEBUG) {
      debug("getComponents(): " + JSON.stringify(aData));
    }

    
    let onComponentsGot = this._createDbCallback(aMm, aData.resolverId,
                                                 "ResourceStats:GetComponents");
    this._db.getComponents(aData.type, onComponentsGot);
  },
};

this.ResourceStatsService.init();

