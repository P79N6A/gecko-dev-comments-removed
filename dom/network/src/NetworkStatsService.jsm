



"use strict";

const DEBUG = false;
function debug(s) {
  if (DEBUG) {
    dump("-*- NetworkStatsService: " + s + "\n");
  }
}

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["NetworkStatsService"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetworkStatsDB.jsm");

const NET_NETWORKSTATSSERVICE_CONTRACTID = "@mozilla.org/network/netstatsservice;1";
const NET_NETWORKSTATSSERVICE_CID = Components.ID("{18725604-e9ac-488a-8aa0-2471e7f6c0a4}");

const TOPIC_INTERFACE_REGISTERED   = "network-interface-registered";
const TOPIC_INTERFACE_UNREGISTERED = "network-interface-unregistered";
const NET_TYPE_WIFI = Ci.nsINetworkInterface.NETWORK_TYPE_WIFI;
const NET_TYPE_MOBILE = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE;


const MAX_CACHED_TRAFFIC = 500 * 1000 * 1000; 

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "networkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyGetter(this, "gRadioInterface", function () {
  let ril = Cc["@mozilla.org/ril;1"].getService(Ci["nsIRadioInterfaceLayer"]);
  
  return ril.getRadioInterface(0);
});

this.NetworkStatsService = {
  init: function() {
    debug("Service started");

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, TOPIC_INTERFACE_REGISTERED, false);
    Services.obs.addObserver(this, TOPIC_INTERFACE_UNREGISTERED, false);
    Services.obs.addObserver(this, "profile-after-change", false);

    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    
    
    
    
    
    
    
    
    
    
    
    

    this._networks = Object.create(null);

    
    
    
    let netId = this.getNetworkId('0', NET_TYPE_WIFI);
    this._networks[netId] = { network:       { id: '0',
                                               type: NET_TYPE_WIFI },
                              interfaceName: null };

    this.messages = ["NetworkStats:Get",
                     "NetworkStats:Clear",
                     "NetworkStats:ClearAll",
                     "NetworkStats:Networks",
                     "NetworkStats:SampleRate",
                     "NetworkStats:MaxStorageAge"];

    this.messages.forEach(function(aMsgName) {
      ppmm.addMessageListener(aMsgName, this);
    }, this);

    this._db = new NetworkStatsDB();

    
    this.timer.initWithCallback(this, this._db.sampleRate,
                                Ci.nsITimer.TYPE_REPEATING_PRECISE);

    
    this.cachedAppStats = Object.create(null);
    this.cachedAppStatsDate = new Date();

    this.updateQueue = [];
    this.isQueueRunning = false;
  },

  receiveMessage: function(aMessage) {
    if (!aMessage.target.assertPermission("networkstats-manage")) {
      return;
    }

    debug("receiveMessage " + aMessage.name);

    let mm = aMessage.target;
    let msg = aMessage.json;

    switch (aMessage.name) {
      case "NetworkStats:Get":
        this.getSamples(mm, msg);
        break;
      case "NetworkStats:Clear":
        this.clearInterfaceStats(mm, msg);
        break;
      case "NetworkStats:ClearAll":
        this.clearDB(mm, msg);
        break;
      case "NetworkStats:Networks":
        return this.availableNetworks();
      case "NetworkStats:SampleRate":
        
        return this._db.sampleRate;
      case "NetworkStats:MaxStorageAge":
        
        return this._db.maxStorageSamples * this._db.sampleRate;
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case TOPIC_INTERFACE_REGISTERED:
      case TOPIC_INTERFACE_UNREGISTERED:

        
        
        

        let network = aSubject.QueryInterface(Ci.nsINetworkInterface);
        debug("Network " + network.name + " of type " + network.type + " status change");

        let netId = this.convertNetworkInterface(network);
        if (!netId) {
          break;
        }

        this.updateStats(netId);
        break;
      case "xpcom-shutdown":
        debug("Service shutdown");

        this.messages.forEach(function(aMsgName) {
          ppmm.removeMessageListener(aMsgName, this);
        }, this);

        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, "profile-after-change");
        Services.obs.removeObserver(this, TOPIC_INTERFACE_REGISTERED);
        Services.obs.removeObserver(this, TOPIC_INTERFACE_UNREGISTERED);

        this.timer.cancel();
        this.timer = null;

        
        this.updateAllStats();
        break;
    }
  },

  



  notify: function(aTimer) {
    this.updateAllStats();
  },

  



  convertNetworkInterface: function(aNetwork) {
    if (aNetwork.type != NET_TYPE_MOBILE &&
        aNetwork.type != NET_TYPE_WIFI) {
      return null;
    }

    let id = '0';
    if (aNetwork.type == NET_TYPE_MOBILE) {
      
      
      
      id = gRadioInterface.rilContext.iccInfo.iccid;
    }

    let netId = this.getNetworkId(id, aNetwork.type);

    if (!this._networks[netId]) {
      this._networks[netId] = Object.create(null);
      this._networks[netId].network = { id: id,
                                        type: aNetwork.type };
    }

    this._networks[netId].interfaceName = aNetwork.name;
    return netId;
  },

  getNetworkId: function getNetworkId(aIccId, aNetworkType) {
    return aIccId + '' + aNetworkType;
  },

  availableNetworks: function availableNetworks() {
    let result = [];
    for (let netId in this._networks) {
      result.push(this._networks[netId].network);
    }

    return result;
  },

  







  getSamples: function getSamples(mm, msg) {
    let self = this;
    let network = msg.network;
    let netId = this.getNetworkId(network.id, network.type);

    if (!this._networks[netId]) {
      mm.sendAsyncMessage("NetworkStats:Get:Return",
                          { id: msg.id, error: "Invalid connectionType", result: null });
      return;
    }

    let appId = 0;
    let manifestURL = msg.manifestURL;
    if (manifestURL) {
      appId = appsService.getAppLocalIdByManifestURL(manifestURL);

      if (!appId) {
        mm.sendAsyncMessage("NetworkStats:Get:Return",
                            { id: msg.id, error: "Invalid manifestURL", result: null });
        return;
      }
    }

    let start = new Date(msg.start);
    let end = new Date(msg.end);

    this.updateStats(netId, function onStatsUpdated(aResult, aMessage) {
      debug("getstats for network " + network.id + " of type " + network.type);
      debug("appId: " + appId + " from manifestURL: " + manifestURL);

      self._db.find(function onStatsFound(aError, aResult) {
        mm.sendAsyncMessage("NetworkStats:Get:Return",
                            { id: msg.id, error: aError, result: aResult });
      }, network, start, end, appId, manifestURL);

    });
  },

  clearInterfaceStats: function clearInterfaceStats(mm, msg) {
    let network = msg.network;
    let netId = this.getNetworkId(network.id, network.type);

    debug("clear stats for network " + network.id + " of type " + network.type);

    if (!this._networks[netId]) {
      mm.sendAsyncMessage("NetworkStats:Clear:Return",
                          { id: msg.id, error: "Invalid networkType", result: null });
      return;
    }

    this._db.clearInterfaceStats(network, function onDBCleared(aError, aResult) {
        mm.sendAsyncMessage("NetworkStats:Clear:Return",
                            { id: msg.id, error: aError, result: aResult });
    });
  },

  clearDB: function clearDB(mm, msg) {
    let networks = this.availableNetworks();
    this._db.clearStats(networks, function onDBCleared(aError, aResult) {
      mm.sendAsyncMessage("NetworkStats:ClearAll:Return",
                          { id: msg.id, error: aError, result: aResult });
    });
  },

  updateAllStats: function updateAllStats(aCallback) {
    
    this.updateCachedAppStats();

    let elements = [];
    let lastElement;

    
    
    
    
    
    
    for (let netId in this._networks) {
      lastElement = { netId: netId,
                      queueIndex: this.updateQueueIndex(netId)};

      if (lastElement.queueIndex == -1) {
        elements.push({netId: lastElement.netId, callbacks: []});
      }
    }

    if (elements.length > 0) {
      
      
      elements[elements.length - 1].callbacks.push(aCallback);
      this.updateQueue = this.updateQueue.concat(elements);
    } else {
      
      
      
      
      
      let element = this.updateQueue[lastElement.queueIndex];
      if (aCallback &&
         (!element || element.netId != lastElement.netId)) {
        aCallback();
        return;
      }

      this.updateQueue[lastElement.queueIndex].callbacks.push(aCallback);
    }

    
    this.processQueue();

    if (DEBUG) {
      this.logAllRecords();
    }
  },

  updateStats: function updateStats(aNetId, aCallback) {
    
    
    let index = this.updateQueueIndex(aNetId);
    if (index == -1) {
      this.updateQueue.push({netId: aNetId, callbacks: [aCallback]});
    } else {
      this.updateQueue[index].callbacks.push(aCallback);
    }

    
    this.processQueue();
  },

  



  updateQueueIndex: function updateQueueIndex(aNetId) {
    return this.updateQueue.map(function(e) { return e.netId; }).indexOf(aNetId);
  },

  


  processQueue: function processQueue(aResult, aMessage) {
    
    
    
    if (aResult != undefined) {
      let item = this.updateQueue.shift();
      for (let callback of item.callbacks) {
        if(callback) {
          callback(aResult, aMessage);
        }
      }
    } else {
      
      
      
      if (this.isQueueRunning) {
        if(this.updateQueue.length > 1) {
          return;
        }
      } else {
        this.isQueueRunning = true;
      }
    }

    
    if (this.updateQueue.length < 1) {
      this.isQueueRunning = false;
      return;
    }

    
    this.update(this.updateQueue[0].netId, this.processQueue.bind(this));
  },

  update: function update(aNetId, aCallback) {
    
    if (!this._networks[aNetId]) {
      if (aCallback) {
        aCallback(false, "Invalid network " + aNetId);
      }
      return;
    }

    let interfaceName = this._networks[aNetId].interfaceName;
    debug("Update stats for " + interfaceName);

    
    
    if (interfaceName) {
      networkManager.getNetworkInterfaceStats(interfaceName,
                this.networkStatsAvailable.bind(this, aCallback, aNetId));
      return;
    }

    if (aCallback) {
      aCallback(true, "ok");
    }
  },

  


  networkStatsAvailable: function networkStatsAvailable(aCallback, aNetId,
                                                        aResult, aRxBytes,
                                                        aTxBytes, aDate) {
    if (!aResult) {
      if (aCallback) {
        aCallback(false, "Netd IPC error");
      }
      return;
    }

    let stats = { appId:       0,
                  networkId:   this._networks[aNetId].network.id,
                  networkType: this._networks[aNetId].network.type,
                  date:        aDate,
                  rxBytes:     aTxBytes,
                  txBytes:     aRxBytes };

    debug("Update stats for: " + JSON.stringify(stats));

    this._db.saveStats(stats, function onSavedStats(aError, aResult) {
      if (aCallback) {
        if (aError) {
          aCallback(false, aError);
          return;
        }

        aCallback(true, "OK");
      }
    });
  },

  


  saveAppStats: function saveAppStats(aAppId, aNetwork, aTimeStamp, aRxBytes, aTxBytes, aCallback) {
    let netId = this.convertNetworkInterface(aNetwork);
    if (!netId) {
      if (aCallback) {
        aCallback.notify(false, "Invalid network type");
      }
      return;
    }

    debug("saveAppStats: " + aAppId + " " + netId + " " +
          aTimeStamp + " " + aRxBytes + " " + aTxBytes);

    
    if (!aAppId || !this._networks[netId]) {
      debug("Invalid appId or network interface");
      return;
    }

    let stats = { appId: aAppId,
                  networkId: this._networks[netId].network.id,
                  networkType: this._networks[netId].network.type,
                  date: new Date(aTimeStamp),
                  rxBytes: aRxBytes,
                  txBytes: aTxBytes };

    
    
    let key = stats.appId + "" + netId;

    
    
    
    let diff = (this._db.normalizeDate(stats.date) -
                this._db.normalizeDate(this.cachedAppStatsDate)) /
               this._db.sampleRate;
    if (diff != 0) {
      this.updateCachedAppStats(function onUpdated(success, message) {
        this.cachedAppStatsDate = stats.date;
        this.cachedAppStats[key] = stats;

        if (!aCallback) {
          return;
        }

        if (!success) {
          aCallback.notify(false, message);
          return;
        }

        aCallback.notify(true, "ok");
      }.bind(this));

      return;
    }

    
    
    let appStats = this.cachedAppStats[key];
    if (!appStats) {
      this.cachedAppStats[key] = stats;
      return;
    }

    
    appStats.rxBytes += stats.rxBytes;
    appStats.txBytes += stats.txBytes;

    
    
    
    if (appStats.rxBytes > MAX_CACHED_TRAFFIC ||
        appStats.txBytes > MAX_CACHED_TRAFFIC) {
      this._db.saveStats(appStats,
        function (error, result) {
          debug("Application stats inserted in indexedDB");
        }
      );
      delete this.cachedAppStats[key];
    }
  },

  updateCachedAppStats: function updateCachedAppStats(aCallback) {
    debug("updateCachedAppStats: " + this.cachedAppStatsDate);

    let stats = Object.keys(this.cachedAppStats);
    if (stats.length == 0) {
      
      return;
    }

    let index = 0;
    this._db.saveStats(this.cachedAppStats[stats[index]],
      function onSavedStats(error, result) {
        if (DEBUG) {
          debug("Application stats inserted in indexedDB");
        }

        
        if (index == stats.length - 1) {
          this.cachedAppStats = Object.create(null);

          if (!aCallback) {
            return;
          }

          if (error) {
            aCallback(false, error);
            return;
          }

          aCallback(true, "ok");
          return;
        }

        
        index += 1;
        this._db.saveStats(this.cachedAppStats[stats[index]],
                           onSavedStats.bind(this, error, result));
      }.bind(this));
  },

  get maxCachedTraffic () {
    return MAX_CACHED_TRAFFIC;
  },

  logAllRecords: function logAllRecords() {
    this._db.logAllRecords(function onResult(aError, aResult) {
      if (aError) {
        debug("Error: " + aError);
        return;
      }

      debug("===== LOG =====");
      debug("There are " + aResult.length + " items");
      debug(JSON.stringify(aResult));
    });
  },
};

NetworkStatsService.init();
