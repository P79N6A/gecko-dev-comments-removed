



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

const TOPIC_INTERFACE_REGISTERED = "network-interface-registered";
const NETWORK_TYPE_WIFI = Ci.nsINetworkInterface.NETWORK_TYPE_WIFI;
const NETWORK_TYPE_MOBILE = Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE;

XPCOMUtils.defineLazyServiceGetter(this, "gIDBManager",
                                   "@mozilla.org/dom/indexeddb/manager;1",
                                   "nsIIndexedDatabaseManager");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "networkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

let myGlobal = this;

this.NetworkStatsService = {
  init: function() {
    if (DEBUG) {
      debug("Service started");
    }

    Services.obs.addObserver(this, "xpcom-shutdown", false);
    Services.obs.addObserver(this, TOPIC_INTERFACE_REGISTERED, false);
    Services.obs.addObserver(this, "profile-after-change", false);

    this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    this._connectionTypes = Object.create(null);
    this._connectionTypes[NETWORK_TYPE_WIFI] = "wifi";
    this._connectionTypes[NETWORK_TYPE_MOBILE] = "mobile";

    this.messages = ["NetworkStats:Get",
                     "NetworkStats:Clear",
                     "NetworkStats:Types",
                     "NetworkStats:SampleRate",
                     "NetworkStats:MaxStorageSamples"];

    this.messages.forEach(function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }, this);

    gIDBManager.initWindowless(myGlobal);
    this._db = new NetworkStatsDB(myGlobal);

    
    this.timer.initWithCallback(this, this._db.sampleRate,
                                Ci.nsITimer.TYPE_REPEATING_PRECISE);

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
          types.push(this._connectionTypes[i]);
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
        
        
        
        let network = subject.QueryInterface(Ci.nsINetworkInterface);
        if (DEBUG) {
          debug("Network " + network.name + " of type " + network.type + " registered");
        }
        this.updateStats(network.type);
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

      let options = msg.data;
      if (DEBUG) {
        debug("getstats for: - " + options.connectionType + " -");
      }

      if (!options.connectionType || options.connectionType.length == 0) {
        this._db.findAll(function onStatsFound(error, result) {
          mm.sendAsyncMessage("NetworkStats:Get:Return",
                              { id: msg.id, error: error, result: result });
        }, options);
        return;
      }

      for (let i in this._connectionTypes) {
        if (this._connectionTypes[i] == options.connectionType) {
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
    if (DEBUG) {
      debug("Update stats for " + this._connectionTypes[connectionType]);
    }

    
    if (!this._connectionTypes[connectionType]) {
      if (callback) {
        callback(false, "Invalid network type " + connectionType);
      }
      return;
    }

    
    
    if (!networkManager.getNetworkInterfaceStats(connectionType, this.networkStatsAvailable.bind(this, callback))) {
      if (DEBUG) {
        debug("There is no interface registered for network type " + this._connectionTypes[connectionType]);
      }

      
      this.networkStatsAvailable(callback, true, connectionType, 0, 0, new Date());
    }
  },

  


  networkStatsAvailable: function networkStatsAvailable(callback, result, connType, rxBytes, txBytes, date) {
    if (!result) {
      if (callback) {
        callback(false, "Netd IPC error");
      }
      return;
    }

    let stats = { connectionType: this._connectionTypes[connType],
                  date:           date,
                  rxBytes:        txBytes,
                  txBytes:        rxBytes};

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
