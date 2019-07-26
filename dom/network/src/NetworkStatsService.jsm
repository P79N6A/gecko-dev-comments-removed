



"use strict";

const DEBUG = false;
function debug(s) { dump("-*- NetworkStatsService: " + s + "\n"); }

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
const NET_TYPE_UNKNOWN = Ci.nsINetworkInterface.NETWORK_TYPE_UNKNOWN;


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

this.NetworkStatsService = {
  init: function() {
    if (DEBUG) {
      debug("Service started");
    }

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, TOPIC_INTERFACE_REGISTERED, false);
    Services.obs.addObserver(this, TOPIC_INTERFACE_UNREGISTERED, false);
    Services.obs.addObserver(this, "profile-after-change", false);

    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    this._connectionTypes = Object.create(null);
    this._connectionTypes[NET_TYPE_WIFI] = { name: "wifi",
                                             network: Object.create(null) };
    this._connectionTypes[NET_TYPE_MOBILE] = { name: "mobile",
                                               network: Object.create(null) };


    this.messages = ["NetworkStats:Get",
                     "NetworkStats:Clear",
                     "NetworkStats:Types",
                     "NetworkStats:SampleRate",
                     "NetworkStats:MaxStorageSamples"];

    this.messages.forEach(function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }, this);

    this._db = new NetworkStatsDB(this._connectionTypes);

    
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

    if (DEBUG) {
      debug("receiveMessage " + aMessage.name);
    }
    let mm = aMessage.target;
    let msg = aMessage.json;

    switch (aMessage.name) {
      case "NetworkStats:Get":
        this.getStats(mm, msg);
        break;
      case "NetworkStats:Clear":
        this.clearDB(mm, msg);
        break;
      case "NetworkStats:Types":
        
        let types = [];
        for (let i in this._connectionTypes) {
          types.push(this._connectionTypes[i].name);
        }
        return types;
      case "NetworkStats:SampleRate":
        
        return this._db.sampleRate;
      case "NetworkStats:MaxStorageSamples":
        
        return this._db.maxStorageSamples;
    }
  },

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case TOPIC_INTERFACE_REGISTERED:
      case TOPIC_INTERFACE_UNREGISTERED:
        
        
        
        let network = subject.QueryInterface(Ci.nsINetworkInterface);
        if (DEBUG) {
          debug("Network " + network.name + " of type " + network.type + " status change");
        }
        if (this._connectionTypes[network.type]) {
          this._connectionTypes[network.type].network = network;
          this.updateStats(network.type);
        }
        break;
      case "xpcom-shutdown":
        if (DEBUG) {
          debug("Service shutdown");
        }

        this.messages.forEach(function(msgName) {
          ppmm.removeMessageListener(msgName, this);
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

  



  notify: function(timer) {
    this.updateAllStats();
  },

  







  getStats: function getStats(mm, msg) {
    this.updateAllStats(function onStatsUpdated(aResult, aMessage) {

      let data = msg.data;

      let options = { appId:          0,
                      connectionType: data.connectionType,
                      start:          data.start,
                      end:            data.end };

      let manifestURL = data.manifestURL;
      if (manifestURL) {
        let appId = appsService.getAppLocalIdByManifestURL(manifestURL);
        if (DEBUG) {
          debug("get appId: " + appId + " from manifestURL: " + manifestURL);
        }

        if (!appId) {
          mm.sendAsyncMessage("NetworkStats:Get:Return",
                              { id: msg.id, error: "Invalid manifestURL", result: null });
          return;
        }

        options.appId = appId;
        options.manifestURL = manifestURL;
      }

      if (DEBUG) {
        debug("getStats for options: " + JSON.stringify(options));
      }

      if (!options.connectionType || options.connectionType.length == 0) {
        this._db.findAll(function onStatsFound(error, result) {
          mm.sendAsyncMessage("NetworkStats:Get:Return",
                              { id: msg.id, error: error, result: result });
        }, options);
        return;
      }

      for (let i in this._connectionTypes) {
        if (this._connectionTypes[i].name == options.connectionType) {
          this._db.find(function onStatsFound(error, result) {
            mm.sendAsyncMessage("NetworkStats:Get:Return",
                                { id: msg.id, error: error, result: result });
          }, options);
          return;
        }
      }

      mm.sendAsyncMessage("NetworkStats:Get:Return",
                          { id: msg.id, error: "Invalid connectionType", result: null });

    }.bind(this));
  },

  clearDB: function clearDB(mm, msg) {
    this._db.clear(function onDBCleared(error, result) {
      mm.sendAsyncMessage("NetworkStats:Clear:Return",
                          { id: msg.id, error: error, result: result });
    });
  },

  updateAllStats: function updateAllStats(callback) {
    
    this.updateCachedAppStats();

    let elements = [];
    let lastElement;

    
    
    
    
    
    
    for (let i in this._connectionTypes) {
      lastElement = { type: i,
                      queueIndex: this.updateQueueIndex(i)};
      if (lastElement.queueIndex == -1) {
        elements.push({type: lastElement.type, callbacks: []});
      }
    }

    if (elements.length > 0) {
      
      
      elements[elements.length - 1].callbacks.push(callback);
      this.updateQueue = this.updateQueue.concat(elements);
    } else {
      
      
      
      
      

      if (!this.updateQueue[lastElement.queueIndex] ||
          this.updateQueue[lastElement.queueIndex].type != lastElement.queueIndex) {
        if (callback) {
          callback();
        }
        return;
      }

      this.updateQueue[lastElement.queueIndex].callbacks.push(callback);
    }

    
    this.processQueue();

    if (DEBUG) {
      this.logAllRecords();
    }
  },

  updateStats: function updateStats(connectionType, callback) {
    
    
    let index = this.updateQueueIndex(connectionType);
    if (index == -1) {
      this.updateQueue.push({type: connectionType, callbacks: [callback]});
    } else {
      this.updateQueue[index].callbacks.push(callback);
    }

    
    this.processQueue();
  },

  



  updateQueueIndex: function updateQueueIndex(type) {
    for (let i in this.updateQueue) {
      if (this.updateQueue[i].type == type) {
        return i;
      }
    }
    return -1;
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

    
    this.update(this.updateQueue[0].type, this.processQueue.bind(this));
  },

  update: function update(connectionType, callback) {
    
    if (!this._connectionTypes[connectionType]) {
      if (callback) {
        callback(false, "Invalid network type " + connectionType);
      }
      return;
    }

    if (DEBUG) {
      debug("Update stats for " + this._connectionTypes[connectionType].name);
    }

    
    
    let networkName = this._connectionTypes[connectionType].network.name;
    if (networkName) {
      networkManager.getNetworkInterfaceStats(networkName,
                this.networkStatsAvailable.bind(this, callback, connectionType));
      return;
    }
    if (callback) {
      callback(true, "ok");
    }
  },

  


  networkStatsAvailable: function networkStatsAvailable(callback, connType, result, rxBytes, txBytes, date) {
    if (!result) {
      if (callback) {
        callback(false, "Netd IPC error");
      }
      return;
    }

    let stats = { appId:          0,
                  connectionType: this._connectionTypes[connType].name,
                  date:           date,
                  rxBytes:        rxBytes,
                  txBytes:        txBytes };

    if (DEBUG) {
      debug("Update stats for " + stats.connectionType + ": rx=" + stats.rxBytes +
            " tx=" + stats.txBytes + " timestamp=" + stats.date);
    }
    this._db.saveStats(stats, function onSavedStats(error, result) {
      if (callback) {
        if (error) {
          callback(false, error);
          return;
        }

        callback(true, "OK");
      }
    });
  },

  


  saveAppStats: function saveAppStats(aAppId, aConnectionType, aTimeStamp, aRxBytes, aTxBytes, aCallback) {
    if (DEBUG) {
      debug("saveAppStats: " + aAppId + " " + aConnectionType + " " +
            aTimeStamp + " " + aRxBytes + " " + aTxBytes);
    }

    
    if (!aAppId || aConnectionType == NET_TYPE_UNKNOWN) {
      return;
    }

    let stats = { appId: aAppId,
                  connectionType: this._connectionTypes[aConnectionType].name,
                  date: new Date(aTimeStamp),
                  rxBytes: aRxBytes,
                  txBytes: aTxBytes };

    
    
    let key = stats.appId + stats.connectionType;

    
    
    
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
          if (DEBUG) {
            debug("Application stats inserted in indexedDB");
          }
        }
      );
      delete this.cachedAppStats[key];
    }
  },

  updateCachedAppStats: function updateCachedAppStats(callback) {
    if (DEBUG) {
      debug("updateCachedAppStats: " + this.cachedAppStatsDate);
    }

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

          if (!callback) {
            return;
          }

          if (error) {
            callback(false, error);
            return;
          }

          callback(true, "ok");
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
    this._db.logAllRecords(function onResult(error, result) {
      if (error) {
        debug("Error: " + error);
        return;
      }

      debug("===== LOG =====");
      debug("There are " + result.length + " items");
      debug(JSON.stringify(result));
    });
  }
};

NetworkStatsService.init();
