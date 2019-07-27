



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
Cu.import("resource://gre/modules/Timer.jsm");

const NET_NETWORKSTATSSERVICE_CONTRACTID = "@mozilla.org/network/netstatsservice;1";
const NET_NETWORKSTATSSERVICE_CID = Components.ID("{18725604-e9ac-488a-8aa0-2471e7f6c0a4}");

const TOPIC_BANDWIDTH_CONTROL = "netd-bandwidth-control"

const TOPIC_CONNECTION_STATE_CHANGED = "network-connection-state-changed";
const NET_TYPE_WIFI = Ci.nsINetworkInterface.NETWORK_TYPE_WIFI;
const NET_TYPE_MOBILE = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE;



const NETWORK_STATUS_READY   = 0;


const NETWORK_STATUS_STANDBY = 1;

const NETWORK_STATUS_AWAY    = 2;


const MAX_CACHED_TRAFFIC = 500 * 1000 * 1000; 

const QUEUE_TYPE_UPDATE_STATS = 0;
const QUEUE_TYPE_UPDATE_CACHE = 1;
const QUEUE_TYPE_WRITE_CACHE = 2;

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "gRil",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

XPCOMUtils.defineLazyServiceGetter(this, "networkService",
                                   "@mozilla.org/network/service;1",
                                   "nsINetworkService");

XPCOMUtils.defineLazyServiceGetter(this, "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

XPCOMUtils.defineLazyServiceGetter(this, "gSettingsService",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyServiceGetter(this, "messenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/iccservice;1",
                                   "nsIIccService");

this.NetworkStatsService = {
  init: function() {
    debug("Service started");

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, TOPIC_CONNECTION_STATE_CHANGED, false);
    Services.obs.addObserver(this, TOPIC_BANDWIDTH_CONTROL, false);
    Services.obs.addObserver(this, "profile-after-change", false);

    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    
    
    
    
    
    
    
    
    
    
    
    

    this._networks = Object.create(null);

    
    
    
    let netId = this.getNetworkId('0', NET_TYPE_WIFI);
    this._networks[netId] = { network:       { id: '0',
                                               type: NET_TYPE_WIFI },
                              interfaceName: null,
                              status:        NETWORK_STATUS_STANDBY };

    this.messages = ["NetworkStats:Get",
                     "NetworkStats:Clear",
                     "NetworkStats:ClearAll",
                     "NetworkStats:SetAlarm",
                     "NetworkStats:GetAlarms",
                     "NetworkStats:RemoveAlarms",
                     "NetworkStats:GetAvailableNetworks",
                     "NetworkStats:GetAvailableServiceTypes",
                     "NetworkStats:SampleRate",
                     "NetworkStats:MaxStorageAge"];

    this.messages.forEach(function(aMsgName) {
      ppmm.addMessageListener(aMsgName, this);
    }, this);

    this._db = new NetworkStatsDB();

    
    this.timer.initWithCallback(this, this._db.sampleRate,
                                Ci.nsITimer.TYPE_REPEATING_PRECISE);

    
    this.cachedStats = Object.create(null);
    this.cachedStatsDate = new Date();

    this.updateQueue = [];
    this.isQueueRunning = false;

    this._currentAlarms = {};
    this.initAlarms();
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
      case "NetworkStats:SetAlarm":
        this.setAlarm(mm, msg);
        break;
      case "NetworkStats:GetAlarms":
        this.getAlarms(mm, msg);
        break;
      case "NetworkStats:RemoveAlarms":
        this.removeAlarms(mm, msg);
        break;
      case "NetworkStats:GetAvailableNetworks":
        this.getAvailableNetworks(mm, msg);
        break;
      case "NetworkStats:GetAvailableServiceTypes":
        this.getAvailableServiceTypes(mm, msg);
        break;
      case "NetworkStats:SampleRate":
        
        return this._db.sampleRate;
      case "NetworkStats:MaxStorageAge":
        
        return this._db.maxStorageSamples * this._db.sampleRate;
    }
  },

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case TOPIC_CONNECTION_STATE_CHANGED:

        
        
        

        let network = aSubject.QueryInterface(Ci.nsINetworkInterface);
        debug("Network " + network.name + " of type " + network.type + " status change");

        let netId = this.convertNetworkInterface(network);
        if (!netId) {
          break;
        }

        this._updateCurrentAlarm(netId);

        debug("NetId: " + netId);
        this.updateStats(netId);
        break;

      case TOPIC_BANDWIDTH_CONTROL:
        debug("Bandwidth message from netd: " + JSON.stringify(aData));

        let interfaceName = aData.substring(aData.lastIndexOf(" ") + 1);
        for (let networkId in this._networks) {
          if (interfaceName == this._networks[networkId].interfaceName) {
            let currentAlarm = this._currentAlarms[networkId];
            if (Object.getOwnPropertyNames(currentAlarm).length !== 0) {
              this._fireAlarm(currentAlarm.alarm);
            }
            break;
          }
        }
        break;

      case "xpcom-shutdown":
        debug("Service shutdown");

        this.messages.forEach(function(aMsgName) {
          ppmm.removeMessageListener(aMsgName, this);
        }, this);

        Services.obs.removeObserver(this, "xpcom-shutdown");
        Services.obs.removeObserver(this, "profile-after-change");
        Services.obs.removeObserver(this, TOPIC_CONNECTION_STATE_CHANGED);
        Services.obs.removeObserver(this, TOPIC_BANDWIDTH_CONTROL);

        this.timer.cancel();
        this.timer = null;

        
        this.updateAllStats();
        break;
    }
  },

  



  notify: function(aTimer) {
    this.updateAllStats();
  },

  


  getRilNetworks: function() {
    let networks = {};
    let numRadioInterfaces = gRil.numRadioInterfaces;
    for (let i = 0; i < numRadioInterfaces; i++) {
      let icc = gIccService.getIccByServiceId(i);
      let radioInterface = gRil.getRadioInterface(i);
      if (icc && icc.iccInfo) {
        let netId = this.getNetworkId(icc.iccInfo.iccid,
                                      NET_TYPE_MOBILE);
        networks[netId] = { id : icc.iccInfo.iccid,
                            type: NET_TYPE_MOBILE };
      }
    }
    return networks;
  },

  convertNetworkInterface: function(aNetwork) {
    if (aNetwork.type != NET_TYPE_MOBILE &&
        aNetwork.type != NET_TYPE_WIFI) {
      return null;
    }

    let id = '0';
    if (aNetwork.type == NET_TYPE_MOBILE) {
      if (!(aNetwork instanceof Ci.nsIRilNetworkInterface)) {
        debug("Error! Mobile network should be an nsIRilNetworkInterface!");
        return null;
      }

      let rilNetwork = aNetwork.QueryInterface(Ci.nsIRilNetworkInterface);
      id = rilNetwork.iccId;
    }

    let netId = this.getNetworkId(id, aNetwork.type);

    if (!this._networks[netId]) {
      this._networks[netId] = Object.create(null);
      this._networks[netId].network = { id: id,
                                        type: aNetwork.type };
    }

    this._networks[netId].status = NETWORK_STATUS_READY;
    this._networks[netId].interfaceName = aNetwork.name;
    return netId;
  },

  getNetworkId: function getNetworkId(aIccId, aNetworkType) {
    return aIccId + '' + aNetworkType;
  },

  





  validateNetwork: function validateNetwork(aNetwork, aCallback) {
    let netId = this.getNetworkId(aNetwork.id, aNetwork.type);

    if (this._networks[netId]) {
      aCallback(netId);
      return;
    }

    
    
    let rilNetworks = this.getRilNetworks();
    if (rilNetworks[netId]) {
      this._networks[netId] = Object.create(null);
      this._networks[netId].network = rilNetworks[netId];
      this._networks[netId].status = NETWORK_STATUS_STANDBY;
      this._currentAlarms[netId] = Object.create(null);
      aCallback(netId);
      return;
    }

    
    this._db.isNetworkAvailable(aNetwork, function(aError, aResult) {
      if (aResult) {
        this._networks[netId] = Object.create(null);
        this._networks[netId].network = aNetwork;
        this._networks[netId].status = NETWORK_STATUS_AWAY;
        this._currentAlarms[netId] = Object.create(null);
        aCallback(netId);
        return;
      }

      aCallback(null);
    }.bind(this));
  },

  getAvailableNetworks: function getAvailableNetworks(mm, msg) {
    let self = this;
    let rilNetworks = this.getRilNetworks();
    this._db.getAvailableNetworks(function onGetNetworks(aError, aResult) {

      
      
      for (let netId in rilNetworks) {
        let found = false;
        for (let i = 0; i < aResult.length; i++) {
          if (netId == self.getNetworkId(aResult[i].id, aResult[i].type)) {
            found = true;
            break;
          }
        }
        if (!found) {
          aResult.push(rilNetworks[netId]);
        }
      }

      mm.sendAsyncMessage("NetworkStats:GetAvailableNetworks:Return",
                          { id: msg.id, error: aError, result: aResult });
    });
  },

  getAvailableServiceTypes: function getAvailableServiceTypes(mm, msg) {
    this._db.getAvailableServiceTypes(function onGetServiceTypes(aError, aResult) {
      mm.sendAsyncMessage("NetworkStats:GetAvailableServiceTypes:Return",
                          { id: msg.id, error: aError, result: aResult });
    });
  },

  initAlarms: function initAlarms() {
    debug("Init usage alarms");
    let self = this;

    for (let netId in this._networks) {
      this._currentAlarms[netId] = Object.create(null);

      this._db.getFirstAlarm(netId, function getResult(error, result) {
        if (!error && result) {
          self._setAlarm(result, function onSet(error, success) {
            if (error == "InvalidStateError") {
              self._fireAlarm(result);
            }
          });
        }
      });
    }
  },

  







  getSamples: function getSamples(mm, msg) {
    let network = msg.network;
    let netId = this.getNetworkId(network.id, network.type);

    let appId = 0;
    let appManifestURL = msg.appManifestURL;
    if (appManifestURL) {
      appId = appsService.getAppLocalIdByManifestURL(appManifestURL);

      if (!appId) {
        mm.sendAsyncMessage("NetworkStats:Get:Return",
                            { id: msg.id,
                              error: "Invalid appManifestURL", result: null });
        return;
      }
    }

    let browsingTrafficOnly = msg.browsingTrafficOnly || false;
    let serviceType = msg.serviceType || "";

    let start = new Date(msg.start);
    let end = new Date(msg.end);

    let callback = (function (aError, aResult) {
      this._db.find(function onStatsFound(aError, aResult) {
        mm.sendAsyncMessage("NetworkStats:Get:Return",
                            { id: msg.id, error: aError, result: aResult });
      }, appId, browsingTrafficOnly, serviceType, network, start, end, appManifestURL);
    }).bind(this);

    this.validateNetwork(network, function onValidateNetwork(aNetId) {
      if (!aNetId) {
        mm.sendAsyncMessage("NetworkStats:Get:Return",
                            { id: msg.id, error: "Invalid connectionType", result: null });
        return;
      }

      
      
      if (this._networks[aNetId].status == NETWORK_STATUS_READY) {
        debug("getstats for network " + network.id + " of type " + network.type);
        debug("appId: " + appId + " from appManifestURL: " + appManifestURL);
        debug("browsingTrafficOnly: " + browsingTrafficOnly);
        debug("serviceType: " + serviceType);

        if (appId || serviceType) {
          this.updateCachedStats(callback);
          return;
        }

        this.updateStats(aNetId, function onStatsUpdated(aResult, aMessage) {
          this.updateCachedStats(callback);
        }.bind(this));
        return;
      }

      
      this._db.find(function onStatsFound(aError, aResult) {
        mm.sendAsyncMessage("NetworkStats:Get:Return",
                            { id: msg.id, error: aError, result: aResult });
      }, appId, browsingTrafficOnly, serviceType, network, start, end, appManifestURL);
    }.bind(this));
  },

  clearInterfaceStats: function clearInterfaceStats(mm, msg) {
    let self = this;
    let network = msg.network;

    debug("clear stats for network " + network.id + " of type " + network.type);

    this.validateNetwork(network, function onValidateNetwork(aNetId) {
      if (!aNetId) {
        mm.sendAsyncMessage("NetworkStats:Clear:Return",
                            { id: msg.id, error: "Invalid connectionType", result: null });
        return;
      }

      network = {network: network, networkId: aNetId};
      self.updateStats(aNetId, function onUpdate(aResult, aMessage) {
        if (!aResult) {
          mm.sendAsyncMessage("NetworkStats:Clear:Return",
                              { id: msg.id, error: aMessage, result: null });
          return;
        }

        self._db.clearInterfaceStats(network, function onDBCleared(aError, aResult) {
          self._updateCurrentAlarm(aNetId);
          mm.sendAsyncMessage("NetworkStats:Clear:Return",
                              { id: msg.id, error: aError, result: aResult });
        });
      });
    });
  },

  clearDB: function clearDB(mm, msg) {
    let self = this;
    this._db.getAvailableNetworks(function onGetNetworks(aError, aResult) {
      if (aError) {
        mm.sendAsyncMessage("NetworkStats:ClearAll:Return",
                            { id: msg.id, error: aError, result: aResult });
        return;
      }

      let networks = aResult;
      networks.forEach(function(network, index) {
        networks[index] = {network: network, networkId: self.getNetworkId(network.id, network.type)};
      }, self);

      self.updateAllStats(function onUpdate(aResult, aMessage){
        if (!aResult) {
          mm.sendAsyncMessage("NetworkStats:ClearAll:Return",
                              { id: msg.id, error: aMessage, result: null });
          return;
        }

        self._db.clearStats(networks, function onDBCleared(aError, aResult) {
          networks.forEach(function(network, index) {
            self._updateCurrentAlarm(network.networkId);
          }, self);
          mm.sendAsyncMessage("NetworkStats:ClearAll:Return",
                              { id: msg.id, error: aError, result: aResult });
        });
      });
    });
  },

  updateAllStats: function updateAllStats(aCallback) {
    let elements = [];
    let lastElement = null;
    let callback = (function (success, message) {
      this.updateCachedStats(aCallback);
    }).bind(this);

    
    
    
    
    
    
    for (let netId in this._networks) {
      if (this._networks[netId].status != NETWORK_STATUS_READY) {
        continue;
      }

      lastElement = { netId: netId,
                      queueIndex: this.updateQueueIndex(netId) };

      if (lastElement.queueIndex == -1) {
        elements.push({ netId:     lastElement.netId,
                        callbacks: [],
                        queueType: QUEUE_TYPE_UPDATE_STATS });
      }
    }

    if (!lastElement) {
      
      
      if (aCallback) {
        aCallback(true, "OK");
      }
      return;
    }

    if (elements.length > 0) {
      
      
      elements[elements.length - 1].callbacks.push(callback);
      this.updateQueue = this.updateQueue.concat(elements);
    } else {
      
      
      
      
      
      let element = this.updateQueue[lastElement.queueIndex];
      if (aCallback &&
         (!element || element.netId != lastElement.netId)) {
        aCallback();
        return;
      }

      this.updateQueue[lastElement.queueIndex].callbacks.push(callback);
    }

    
    this.processQueue();

    if (DEBUG) {
      this.logAllRecords();
    }
  },

  updateStats: function updateStats(aNetId, aCallback) {
    
    
    let index = this.updateQueueIndex(aNetId);
    if (index == -1) {
      this.updateQueue.push({ netId: aNetId,
                              callbacks: [aCallback],
                              queueType: QUEUE_TYPE_UPDATE_STATS });
    } else {
      this.updateQueue[index].callbacks.push(aCallback);
      return;
    }

    
    this.processQueue();
  },

  



  updateQueueIndex: function updateQueueIndex(aNetId) {
    return this.updateQueue.map(function(e) { return e.netId; }).indexOf(aNetId);
  },

  


  processQueue: function processQueue(aResult, aMessage) {
    
    
    
    let self = this;

    if (aResult != undefined) {
      let item = this.updateQueue.shift();
      for (let callback of item.callbacks) {
        if (callback) {
          callback(aResult, aMessage);
        }
      }
    } else {
      
      
      
      if (this.isQueueRunning) {
        return;
      } else {
        this.isQueueRunning = true;
      }
    }

    
    if (this.updateQueue.length < 1) {
      this.isQueueRunning = false;
      return;
    }

    
    setTimeout(function () {
                 self.run(self.updateQueue[0]);
               }, 0);
  },

  run: function run(item) {
    switch (item.queueType) {
      case QUEUE_TYPE_UPDATE_STATS:
        this.update(item.netId, this.processQueue.bind(this));
        break;
      case QUEUE_TYPE_UPDATE_CACHE:
        this.updateCache(this.processQueue.bind(this));
        break;
      case QUEUE_TYPE_WRITE_CACHE:
        this.writeCache(item.stats, this.processQueue.bind(this));
        break;
    }
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
      networkService.getNetworkInterfaceStats(interfaceName,
                this.networkStatsAvailable.bind(this, aCallback, aNetId));
      return;
    }

    if (aCallback) {
      aCallback(true, "ok");
    }
  },

  


  networkStatsAvailable: function networkStatsAvailable(aCallback, aNetId,
                                                        aResult, aRxBytes,
                                                        aTxBytes, aTimestamp) {
    if (!aResult) {
      if (aCallback) {
        aCallback(false, "Netd IPC error");
      }
      return;
    }

    let stats = { appId:          0,
                  isInBrowser:    false,
                  serviceType:    "",
                  networkId:      this._networks[aNetId].network.id,
                  networkType:    this._networks[aNetId].network.type,
                  date:           new Date(aTimestamp),
                  rxBytes:        aTxBytes,
                  txBytes:        aRxBytes,
                  isAccumulative: true };

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

  


  saveStats: function saveStats(aAppId, aIsInBrowser, aServiceType, aNetwork,
                                aTimeStamp, aRxBytes, aTxBytes, aIsAccumulative,
                                aCallback) {
    let netId = this.convertNetworkInterface(aNetwork);
    if (!netId) {
      if (aCallback) {
        aCallback(false, "Invalid network type");
      }
      return;
    }

    
    
    
    
    
    if (!this._networks[netId] || (aAppId && aServiceType) ||
        (!aAppId && !aServiceType)) {
      debug("Invalid network interface, appId or serviceType");
      return;
    }

    let stats = { appId:          aAppId,
                  isInBrowser:    aIsInBrowser,
                  serviceType:    aServiceType,
                  networkId:      this._networks[netId].network.id,
                  networkType:    this._networks[netId].network.type,
                  date:           new Date(aTimeStamp),
                  rxBytes:        aRxBytes,
                  txBytes:        aTxBytes,
                  isAccumulative: aIsAccumulative };

    this.updateQueue.push({ stats: stats,
                            callbacks: [aCallback],
                            queueType: QUEUE_TYPE_WRITE_CACHE });

    this.processQueue();
  },

  


  writeCache: function writeCache(aStats, aCallback) {
    debug("saveStats: " + aStats.appId + " " + aStats.isInBrowser + " " +
          aStats.serviceType + " " + aStats.networkId + " " +
          aStats.networkType + " " + aStats.date + " " +
          aStats.rxBytes + " " + aStats.txBytes);

    
    
    let netId = this.getNetworkId(aStats.networkId, aStats.networkType);
    let key = aStats.appId + "" + aStats.isInBrowser + "" +
              aStats.serviceType + "" + netId;

    
    
    
    let diff = (this._db.normalizeDate(aStats.date) -
                this._db.normalizeDate(this.cachedStatsDate)) /
               this._db.sampleRate;
    if (diff != 0) {
      this.updateCache(function onUpdated(success, message) {
        this.cachedStatsDate = aStats.date;
        this.cachedStats[key] = aStats;

        if (aCallback) {
          aCallback(true, "ok");
        }
      }.bind(this));
      return;
    }

    
    
    let cachedStats = this.cachedStats[key];
    if (!cachedStats) {
      this.cachedStats[key] = aStats;
      if (aCallback) {
        aCallback(true, "ok");
      }
      return;
    }

    
    cachedStats.rxBytes += aStats.rxBytes;
    cachedStats.txBytes += aStats.txBytes;

    
    
    
    if (cachedStats.rxBytes > MAX_CACHED_TRAFFIC ||
        cachedStats.txBytes > MAX_CACHED_TRAFFIC) {
      this._db.saveStats(cachedStats, function (error, result) {
        debug("Application stats inserted in indexedDB");
        if (aCallback) {
          aCallback(true, "ok");
        }
      });
      delete this.cachedStats[key];
      return;
    }

    if (aCallback) {
      aCallback(true, "ok");
    }
  },

  updateCachedStats: function updateCachedStats(aCallback) {
    this.updateQueue.push({ callbacks: [aCallback],
                            queueType: QUEUE_TYPE_UPDATE_CACHE });

    this.processQueue();
  },

  updateCache: function updateCache(aCallback) {
    debug("updateCache: " + this.cachedStatsDate);

    let stats = Object.keys(this.cachedStats);
    if (stats.length == 0) {
      
      if (aCallback) {
        aCallback(true, "no need to update");
      }
      return;
    }

    let index = 0;
    this._db.saveStats(this.cachedStats[stats[index]],
                       function onSavedStats(error, result) {
      debug("Application stats inserted in indexedDB");

      
      if (index == stats.length - 1) {
        this.cachedStats = Object.create(null);

        if (aCallback) {
          aCallback(true, "ok");
        }
        return;
      }

      
      index += 1;
      this._db.saveStats(this.cachedStats[stats[index]],
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

  getAlarms: function getAlarms(mm, msg) {
    let self = this;
    let network = msg.data.network;
    let manifestURL = msg.data.manifestURL;

    if (network) {
      this.validateNetwork(network, function onValidateNetwork(aNetId) {
        if (!aNetId) {
          mm.sendAsyncMessage("NetworkStats:GetAlarms:Return",
                              { id: msg.id, error: "InvalidInterface", result: null });
          return;
        }

        self._getAlarms(mm, msg, aNetId, manifestURL);
      });
      return;
    }

    this._getAlarms(mm, msg, null, manifestURL);
  },

  _getAlarms: function _getAlarms(mm, msg, aNetId, aManifestURL) {
    let self = this;
    this._db.getAlarms(aNetId, aManifestURL, function onCompleted(error, result) {
      if (error) {
        mm.sendAsyncMessage("NetworkStats:GetAlarms:Return",
                            { id: msg.id, error: error, result: result });
        return;
      }

      let alarms = []
      
      for (let i = 0; i < result.length; i++) {
        let alarm = result[i];
        alarms.push({ id: alarm.id,
                      network: self._networks[alarm.networkId].network,
                      threshold: alarm.absoluteThreshold,
                      data: alarm.data });
      }

      mm.sendAsyncMessage("NetworkStats:GetAlarms:Return",
                          { id: msg.id, error: null, result: alarms });
    });
  },

  removeAlarms: function removeAlarms(mm, msg) {
    let alarmId = msg.data.alarmId;
    let manifestURL = msg.data.manifestURL;

    let self = this;
    let callback = function onRemove(error, result) {
      if (error) {
        mm.sendAsyncMessage("NetworkStats:RemoveAlarms:Return",
                            { id: msg.id, error: error, result: result });
        return;
      }

      for (let i in self._currentAlarms) {
        let currentAlarm = self._currentAlarms[i].alarm;
        if (currentAlarm && ((alarmId == currentAlarm.id) ||
            (alarmId == -1 && currentAlarm.manifestURL == manifestURL))) {

          self._updateCurrentAlarm(currentAlarm.networkId);
        }
      }

      mm.sendAsyncMessage("NetworkStats:RemoveAlarms:Return",
                          { id: msg.id, error: error, result: true });
    };

    if (alarmId == -1) {
      this._db.removeAlarms(manifestURL, callback);
    } else {
      this._db.removeAlarm(alarmId, manifestURL, callback);
    }
  },

  


  setAlarm: function setAlarm(mm, msg) {
    let options = msg.data;
    let network = options.network;
    let threshold = options.threshold;

    debug("Set alarm at " + threshold + " for " + JSON.stringify(network));

    if (threshold < 0) {
      mm.sendAsyncMessage("NetworkStats:SetAlarm:Return",
                          { id: msg.id, error: "InvalidThresholdValue", result: null });
      return;
    }

    let self = this;
    this.validateNetwork(network, function onValidateNetwork(aNetId) {
      if (!aNetId) {
        mm.sendAsyncMessage("NetworkStats:SetAlarm:Return",
                            { id: msg.id, error: "InvalidiConnectionType", result: null });
        return;
      }

      let newAlarm = {
        id: null,
        networkId: aNetId,
        absoluteThreshold: threshold,
        relativeThreshold: null,
        startTime: options.startTime,
        data: options.data,
        pageURL: options.pageURL,
        manifestURL: options.manifestURL
      };

      self._getAlarmQuota(newAlarm, function onUpdate(error, quota) {
        if (error) {
          mm.sendAsyncMessage("NetworkStats:SetAlarm:Return",
                              { id: msg.id, error: error, result: null });
          return;
        }

        self._db.addAlarm(newAlarm, function addSuccessCb(error, newId) {
          if (error) {
            mm.sendAsyncMessage("NetworkStats:SetAlarm:Return",
                                { id: msg.id, error: error, result: null });
            return;
          }

          newAlarm.id = newId;
          self._setAlarm(newAlarm, function onSet(error, success) {
            mm.sendAsyncMessage("NetworkStats:SetAlarm:Return",
                                { id: msg.id, error: error, result: newId });

            if (error == "InvalidStateError") {
              self._fireAlarm(newAlarm);
            }
          });
        });
      });
    });
  },

  _setAlarm: function _setAlarm(aAlarm, aCallback) {
    let currentAlarm = this._currentAlarms[aAlarm.networkId];
    if ((Object.getOwnPropertyNames(currentAlarm).length !== 0 &&
         aAlarm.relativeThreshold > currentAlarm.alarm.relativeThreshold) ||
        this._networks[aAlarm.networkId].status != NETWORK_STATUS_READY) {
      aCallback(null, true);
      return;
    }

    let self = this;

    this._getAlarmQuota(aAlarm, function onUpdate(aError, aQuota) {
      if (aError) {
        aCallback(aError, null);
        return;
      }

      let callback = function onAlarmSet(aError) {
        if (aError) {
          debug("Set alarm error: " + aError);
          aCallback("netdError", null);
          return;
        }

        self._currentAlarms[aAlarm.networkId].alarm = aAlarm;

        aCallback(null, true);
      };

      debug("Set alarm " + JSON.stringify(aAlarm));
      let interfaceName = self._networks[aAlarm.networkId].interfaceName;
      if (interfaceName) {
        networkService.setNetworkInterfaceAlarm(interfaceName,
                                                aQuota,
                                                callback);
        return;
      }

      aCallback(null, true);
    });
  },

  _getAlarmQuota: function _getAlarmQuota(aAlarm, aCallback) {
    let self = this;
    this.updateStats(aAlarm.networkId, function onStatsUpdated(aResult, aMessage) {
      self._db.getCurrentStats(self._networks[aAlarm.networkId].network,
                               aAlarm.startTime,
                               function onStatsFound(error, result) {
        if (error) {
          debug("Error getting stats for " +
                JSON.stringify(self._networks[aAlarm.networkId]) + ": " + error);
          aCallback(error, result);
          return;
        }

        let quota = aAlarm.absoluteThreshold - result.rxBytes - result.txBytes;

        
        if (quota <= 0) {
          aCallback("InvalidStateError", null);
          return;
        }

        aAlarm.relativeThreshold = aAlarm.startTime
                                 ? result.rxTotalBytes + result.txTotalBytes + quota
                                 : aAlarm.absoluteThreshold;

        aCallback(null, quota);
      });
    });
  },

  _fireAlarm: function _fireAlarm(aAlarm) {
    debug("Fire alarm");

    let self = this;
    this._db.removeAlarm(aAlarm.id, null, function onRemove(aError, aResult){
      if (!aError && !aResult) {
        return;
      }

      self._fireSystemMessage(aAlarm);
      self._updateCurrentAlarm(aAlarm.networkId);
    });
  },

  _updateCurrentAlarm: function _updateCurrentAlarm(aNetworkId) {
    this._currentAlarms[aNetworkId] = Object.create(null);

    let self = this;
    this._db.getFirstAlarm(aNetworkId, function onGet(error, result){
      if (error) {
        debug("Error getting the first alarm");
        return;
      }

      if (!result) {
        let interfaceName = self._networks[aNetworkId].interfaceName;
        networkService.setNetworkInterfaceAlarm(interfaceName, -1,
                                                function onComplete(){});
        return;
      }

      self._setAlarm(result, function onSet(error, success){
        if (error == "InvalidStateError") {
          self._fireAlarm(result);
          return;
        }
      });
    });
  },

  _fireSystemMessage: function _fireSystemMessage(aAlarm) {
    debug("Fire system message: " + JSON.stringify(aAlarm));

    let manifestURI = Services.io.newURI(aAlarm.manifestURL, null, null);
    let pageURI = Services.io.newURI(aAlarm.pageURL, null, null);

    let alarm = { "id":        aAlarm.id,
                  "threshold": aAlarm.absoluteThreshold,
                  "data":      aAlarm.data };
    messenger.sendMessage("networkstats-alarm", alarm, pageURI, manifestURI);
  }
};

NetworkStatsService.init();
