



"use strict";

function debug(s) {
  
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/IndexedDBHelper.jsm");
Cu.import("resource://gre/modules/Timer.jsm");
Cu.import("resource://gre/modules/services-common/preferences.js");
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");

const kPUSHDB_DB_NAME = "push";
const kPUSHDB_DB_VERSION = 1; 
const kPUSHDB_STORE_NAME = "push";
const kCONFLICT_RETRY_ATTEMPTS = 3; 
                                    

const kERROR_CHID_CONFLICT = 409;   
                                    

const kUDP_WAKEUP_WS_STATUS_CODE = 4774;  
                                          
                                          

const kCHILD_PROCESS_MESSAGES = ["Push:Register", "Push:Unregister",
                                 "Push:Registrations"];


this.PushDB = function PushDB(aGlobal) {
  debug("PushDB()");

  
  let idbManager = Cc["@mozilla.org/dom/indexeddb/manager;1"]
                     .getService(Ci.nsIIndexedDatabaseManager);
  idbManager.initWindowless(aGlobal);
  this.initDBHelper(kPUSHDB_DB_NAME, kPUSHDB_DB_VERSION,
                    [kPUSHDB_STORE_NAME], aGlobal);
};

this.PushDB.prototype = {
  __proto__: IndexedDBHelper.prototype,

  upgradeSchema: function(aTransaction, aDb, aOldVersion, aNewVersion) {
    debug("PushDB.upgradeSchema()")

    let objectStore = aDb.createObjectStore(kPUSHDB_STORE_NAME,
                                            { keyPath: "channelID" });

    
    objectStore.createIndex("pushEndpoint", "pushEndpoint", { unique: true });
    
    
    
    objectStore.createIndex("manifestURL", "manifestURL", { unique: false });
  },

  







  put: function(aChannelRecord, aSuccessCb, aErrorCb) {
    debug("put()");

    this.newTxn(
      "readwrite",
      kPUSHDB_STORE_NAME,
      function txnCb(aTxn, aStore) {
        debug("Going to put " + aChannelRecord.channelID);
        aStore.put(aChannelRecord).onsuccess = function setTxnResult(aEvent) {
          debug("Request successful. Updated record ID: " + aEvent.target.result);
        };
      },
      aSuccessCb,
      aErrorCb
    );
  },

  







  delete: function(aChannelID, aSuccessCb, aErrorCb) {
    debug("delete()");

    this.newTxn(
      "readwrite",
      kPUSHDB_STORE_NAME,
      function txnCb(aTxn, aStore) {
        debug("Going to delete " + aChannelID);
        aStore.delete(aChannelID);
      },
      aSuccessCb,
      aErrorCb
    );
  },

  getByPushEndpoint: function(aPushEndpoint, aSuccessCb, aErrorCb) {
    debug("getByPushEndpoint()");

    this.newTxn(
      "readonly",
      kPUSHDB_STORE_NAME,
      function txnCb(aTxn, aStore) {
        aTxn.result = undefined;

        var index = aStore.index("pushEndpoint");
        index.get(aPushEndpoint).onsuccess = function setTxnResult(aEvent) {
          aTxn.result = aEvent.target.result;
          debug("Fetch successful " + aEvent.target.result);
        }
      },
      aSuccessCb,
      aErrorCb
    );
  },

  getByChannelID: function(aChannelID, aSuccessCb, aErrorCb) {
    debug("getByChannelID()");

    this.newTxn(
      "readonly",
      kPUSHDB_STORE_NAME,
      function txnCb(aTxn, aStore) {
        aTxn.result = undefined;

        aStore.get(aChannelID).onsuccess = function setTxnResult(aEvent) {
          aTxn.result = aEvent.target.result;
          debug("Fetch successful " + aEvent.target.result);
        }
      },
      aSuccessCb,
      aErrorCb
    );
  },

  getAllByManifestURL: function(aManifestURL, aSuccessCb, aErrorCb) {
    debug("getAllByManifestURL()");
    if (!aManifestURL) {
      if (typeof aErrorCb == "function") {
        aErrorCb("PushDB.getAllByManifestURL: Got undefined aManifestURL");
      }
      return;
    }
    this.newTxn(
      "readonly",
      kPUSHDB_STORE_NAME,
      function txnCb(aTxn, aStore) {
        var index = aStore.index("manifestURL");
        index.mozGetAll().onsuccess = function(event) {
          aTxn.result = event.target.result;
        }
      },
      aSuccessCb,
      aErrorCb
    );
  },

  getAllChannelIDs: function(aSuccessCb, aErrorCb) {
    debug("getAllChannelIDs()");

    this.newTxn(
      "readonly",
      kPUSHDB_STORE_NAME,
      function txnCb(aTxn, aStore) {
        aStore.mozGetAll().onsuccess = function(event) {
          aTxn.result = event.target.result;
        }
      },
      aSuccessCb,
      aErrorCb
    );
  },

  drop: function(aSuccessCb, aErrorCb) {
    debug("drop()");
    this.newTxn(
      "readwrite",
      kPUSHDB_STORE_NAME,
      function txnCb(aTxn, aStore) {
        aStore.clear();
      },
      aSuccessCb(),
      aErrorCb()
    );
  }
};









this.PushWebSocketListener = function(pushService) {
  this._pushService = pushService;
}

this.PushWebSocketListener.prototype = {
  onStart: function(context) {
    if (!this._pushService)
        return;
    this._pushService._wsOnStart(context);
  },

  onStop: function(context, statusCode) {
    if (!this._pushService)
        return;
    this._pushService._wsOnStop(context, statusCode);
  },

  onAcknowledge: function(context, size) {
    
  },

  onBinaryMessageAvailable: function(context, message) {
    
  },

  onMessageAvailable: function(context, message) {
    if (!this._pushService)
        return;
    this._pushService._wsOnMessageAvailable(context, message);
  },

  onServerClose: function(context, aStatusCode, aReason) {
    if (!this._pushService)
        return;
    this._pushService._wsOnServerClose(context, aStatusCode, aReason);
  }
}






function PushService()
{
  debug("PushService Constructor.");
}



const STATE_SHUT_DOWN = 0;


const STATE_WAITING_FOR_WS_START = 1;

const STATE_WAITING_FOR_HELLO = 2;

const STATE_READY = 3;

PushService.prototype = {
  classID : Components.ID("{0ACE8D15-9B15-41F4-992F-C88820421DBF}"),

  QueryInterface : XPCOMUtils.generateQI([Ci.nsIObserver,
                                          Ci.nsIUDPServerSocketListener]),

  observe: function observe(aSubject, aTopic, aData) {
    switch (aTopic) {
      case "app-startup":
        Services.obs.addObserver(this, "final-ui-startup", false);
        Services.obs.addObserver(this, "profile-change-teardown", false);
        Services.obs.addObserver(this,
                                 "network-interface-state-changed",
                                 false);
        break;
      case "final-ui-startup":
        Services.obs.removeObserver(this, "final-ui-startup");
        this.init();
        break;
      case "profile-change-teardown":
        Services.obs.removeObserver(this, "profile-change-teardown");
        this._shutdown();
        break;
      case "network-interface-state-changed":
        debug("network-interface-state-changed");

        if (this._udpServer) {
          this._udpServer.close();
        }

        this._shutdownWS();

        
        this._db.getAllChannelIDs(function(channelIDs) {
          if (channelIDs.length > 0) {
            this._beginWSSetup();
          }
        }.bind(this));
        break;
      case "nsPref:changed":
        if (aData == "services.push.serverURL") {
          debug("services.push.serverURL changed! websocket. new value " +
                this._prefs.get("serverURL"));
          this._shutdownWS();
        }
        break;
      case "timer-callback":
        if (aSubject == this._requestTimeoutTimer) {
          if (Object.keys(this._pendingRequests).length == 0)
            this._requestTimeoutTimer.cancel();

          for (var channelID in this._pendingRequests) {
            var duration = Date.now() - this._pendingRequests[channelID].ctime;
            if (duration > this._requestTimeout) {
              debug("Request timeout: Removing " + channelID);
              this._pendingRequests[channelID]
                .deferred.reject({status: 0, error: "Timeout"});

              delete this._pendingRequests[channelID];
              for (var i = this._requestQueue.length - 1; i >= 0; --i)
                if (this._requestQueue[i].channelID == channelID)
                  this._requestQueue.splice(i, 1);
            }
          }
        }
        else if (aSubject == this._retryTimeoutTimer) {
          this._beginWSSetup();
        }
    }
  },

  _prefs : new Preferences("services.push."),

  get _UAID() {
    return this._prefs.get("userAgentID");
  },

  set _UAID(newID) {
    if (typeof(newID) !== "string") {
      debug("Got invalid, non-string UAID " + newID +
            ". Not updating userAgentID");
      return;
    }
    debug("New _UAID: " + newID);
    this._prefs.set("userAgentID", newID);
  },

  
  _requestQueue: [],
  _ws: null,
  _pendingRequests: {},
  _currentState: STATE_SHUT_DOWN,
  _requestTimeout: 0,
  _requestTimeoutTimer: null,

  
















  _retryTimeoutTimer: null,
  _retryFailCount: 0,

  









  _willBeWokenUpByUDP: false,

  init: function() {
    debug("init()");
    this._db = new PushDB(this);

    let ppmm = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                 .getService(Ci.nsIMessageBroadcaster);

    kCHILD_PROCESS_MESSAGES.forEach(function addMessage(msgName) {
        ppmm.addMessageListener(msgName, this);
    }.bind(this));

    this._requestTimeout = this._prefs.get("requestTimeout");

    this._udpPort = this._prefs.get("udp.port");

    this._db.getAllChannelIDs(
      function(channelIDs) {
        if (channelIDs.length > 0) {
          debug("Found registered channelIDs. Starting WebSocket");
          this._beginWSSetup();
        }
      }.bind(this),

      function(error) {
        debug("db error " + error);
      }
    );

    
    
    this._prefs.observe("serverURL", this);
  },

  _shutdownWS: function() {
    debug("shutdownWS()");
    this._currentState = STATE_SHUT_DOWN;
    this._willBeWokenUpByUDP = false;
    if (this._wsListener)
      this._wsListener._pushService = null;
    try {
        this._ws.close(0, null);
    } catch (e) {}
    this._ws = null;
  },

  _shutdown: function() {
    debug("_shutdown()");
    this._db.close();
    this._db = null;

    if (this._udpServer) {
      this._udpServer.close();
    }

    
    
    
    this._shutdownWS();

    debug("shutdown complete!");
  },

  
  _socketError: function(aStatusCode) {
    debug("socketError()");

    
    var retryTimeout = this._prefs.get("retryBaseInterval") *
                        Math.pow(2, this._retryFailCount);

    
    
    
    retryTimeout = Math.min(retryTimeout, this._prefs.get("maxRetryInterval"));

    this._retryFailCount++;

    debug("Retry in " + retryTimeout + " Try number " + this._retryFailCount);

    if (!this._retryTimeoutTimer) {
      this._retryTimeoutTimer = Cc["@mozilla.org/timer;1"]
                                  .createInstance(Ci.nsITimer);
    }

    this._retryTimeoutTimer.init(this,
                                 retryTimeout,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _beginWSSetup: function() {
    debug("beginWSSetup()");
    if (this._currentState != STATE_SHUT_DOWN) {
      debug("_beginWSSetup: Not in shutdown state! Current state " +
            this._currentState);
      return;
    }

    var serverURL = this._prefs.get("serverURL");
    if (!serverURL) {
      debug("No services.push.serverURL found!");
      return;
    }

    var uri;
    try {
      uri = Services.io.newURI(serverURL, null, null);
    } catch(e) {
      debug("Error creating valid URI from services.push.serverURL (" +
            serverURL + ")");
      return;
    }

    if (uri.scheme === "wss") {
      this._ws = Cc["@mozilla.org/network/protocol;1?name=wss"]
                   .createInstance(Ci.nsIWebSocketChannel);
    }
    else if (uri.scheme === "ws") {
      debug("Push over an insecure connection (ws://) is not allowed!");
      return;
    }
    else {
      debug("Unsupported websocket scheme " + uri.scheme);
      return;
    }

    debug("serverURL: " + uri.spec);
    this._wsListener = new PushWebSocketListener(this);
    this._ws.protocol = "push-notification";
    this._ws.asyncOpen(uri, serverURL, this._wsListener, null);
    this._currentState = STATE_WAITING_FOR_WS_START;
  },

  


  _handleHelloReply: function(reply) {
    debug("handleHelloReply()");
    if (this._currentState != STATE_WAITING_FOR_HELLO) {
      debug("Unexpected state " + this._currentState +
            "(expected STATE_WAITING_FOR_HELLO)");
      this._shutdownWS();
      return;
    }

    if (typeof reply.uaid !== "string") {
      debug("No UAID received or non string UAID received");
      this._shutdownWS();
      return;
    }

    if (reply.uaid === "") {
      debug("Empty UAID received!");
      this._shutdownWS();
      return;
    }

    
    if (reply.uaid.length > 128) {
      debug("UAID received from server was too long: " +
            reply.uaid);
      this._shutdownWS();
      return;
    }

    function finishHandshake() {
      this._UAID = reply.uaid;
      this._currentState = STATE_READY;
      this._processNextRequestInQueue();
    }

    
    
    
    
    
    if (this._UAID && this._UAID != reply.uaid) {
      debug("got new UAID: all re-register");
      this._dropRegistrations()
        .then(
          function() {
            
            
            this._notifyAllAppsRegister();
            finishHandshake.bind(this)();
          }.bind(this),
          function(error) {
            debug("Error deleting all registrations. SHOULD NEVER HAPPEN!");
            this._shutdownWS();
            return;
          }.bind(this)
        );

      return;
    }

    
    finishHandshake.bind(this)();
  },

  


  _handleRegisterReply: function(reply) {
    debug("handleRegisterReply()");
    if (typeof reply.channelID !== "string" ||
        typeof this._pendingRequests[reply.channelID] !== "object")
      return;

    var tmp = this._pendingRequests[reply.channelID];
    delete this._pendingRequests[reply.channelID];
    if (Object.keys(this._pendingRequests).length == 0 &&
        this._requestTimeoutTimer)
      this._requestTimeoutTimer.cancel();

    if (reply.status == 200) {
      tmp.deferred.resolve(reply);
    } else {
      tmp.deferred.reject(reply);
    }
  },

  


  _handleUnregisterReply: function(reply) {
    debug("handleUnregisterReply()");
    if (typeof reply.channelID !== "string" ||
        typeof this._pendingRequests[reply.channelID] !== "object")
      return;

    var tmp = this._pendingRequests[reply.channelID];
    delete this._pendingRequests[reply.channelID];
    if (Object.keys(this._pendingRequests).length == 0 &&
        this._requestTimeoutTimer)
      this._requestTimeoutTimer.cancel();

    if (reply.status == 200) {
      tmp.deferred.resolve(reply);
    } else {
      tmp.deferred.reject(reply);
    }
  },

  


  _handleNotificationReply: function(reply) {
    debug("handleNotificationReply()");
    if (typeof reply.updates !== 'object') {
      debug("No 'updates' field in response. Type = " + typeof reply.updates);
      return;
    }

    debug("Reply updates: " + reply.updates.length);
    for (var i = 0; i < reply.updates.length; i++) {
      var update = reply.updates[i];
      debug("Update: " + update.channelID + ": " + update.version);
      if (typeof update.channelID !== "string") {
        debug("Invalid update literal at index " + i);
        continue;
      }

      if (update.version === undefined) {
        debug("update.version does not exist");
        continue;
      }

      var version = update.version;

      if (typeof version === "string") {
        version = parseInt(version, 10);
      }

      if (typeof version === "number" && version >= 0) {
        
        
        this._receivedUpdate(update.channelID, version);
        this._sendAck(update.channelID, version);
      }
    }
  },

  
  _sendAck: function(channelID, version) {
    debug("sendAck()");
    this._send('ack', {
      updates: [{channelID: channelID, version: version}]
    });
  },

  


  _sendRequest: function(action, data) {
    debug("sendRequest() " + action);
    if (typeof data.channelID !== "string") {
      debug("Received non-string channelID");
      return;
    }

    var deferred = Promise.defer();

    if (Object.keys(this._pendingRequests).length == 0) {
      
      if (!this._requestTimeoutTimer)
        this._requestTimeoutTimer = Cc["@mozilla.org/timer;1"]
                                      .createInstance(Ci.nsITimer);
      this._requestTimeoutTimer.init(this,
                                     this._requestTimeout,
                                     Ci.nsITimer.TYPE_REPEATING_SLACK);
    }

    this._pendingRequests[data.channelID] = { deferred: deferred,
                                              ctime: Date.now() };

    this._send(action, data);
    return deferred.promise;
  },

  _send: function(action, data) {
    debug("send()");
    this._requestQueue.push([action, data]);
    debug("Queued " + action);
    this._processNextRequestInQueue();
  },

  _processNextRequestInQueue: function() {
    debug("_processNextRequestInQueue()");

    if (this._requestQueue.length == 0) {
      debug("Request queue empty");
      return;
    }

    if (this._currentState != STATE_READY) {
      if (!this._ws) {
        
        this._beginWSSetup();
      }
      else {
        
        
      }
      return;
    }

    [action, data] = this._requestQueue.shift();
    data.messageType = action;
    if (!this._ws) {
      
      
      
      
      debug("This should never happen!");
      this._shutdownWS();
    }

    this._ws.sendMsg(JSON.stringify(data));
    
    setTimeout(this._processNextRequestInQueue.bind(this), 0);
  },

  _receivedUpdate: function(aChannelID, aLatestVersion) {
    debug("Updating: " + aChannelID + " -> " + aLatestVersion);

    var compareRecordVersionAndNotify = function(aPushRecord) {
      debug("compareRecordVersionAndNotify()");
      if (!aPushRecord) {
        debug("No record for channel ID " + aChannelID);
        return;
      }

      if (aPushRecord.version == null ||
          aPushRecord.version < aLatestVersion) {
        debug("Version changed, notifying app and updating DB");
        aPushRecord.version = aLatestVersion;
        this._notifyApp(aPushRecord);
        this._updatePushRecord(aPushRecord)
          .then(
            null,
            function(e) {
              debug("Error updating push record");
            }
          );
      }
      else {
        debug("No significant version change: " + aLatestVersion);
      }
    }

    var recoverNoSuchChannelID = function(aChannelIDFromServer) {
      debug("Could not get channelID " + aChannelIDFromServer + " from DB");
    }

    this._db.getByChannelID(aChannelID,
                            compareRecordVersionAndNotify.bind(this),
                            recoverNoSuchChannelID.bind(this));
  },

  _notifyAllAppsRegister: function() {
    debug("notifyAllAppsRegister()");
    let messenger = Cc["@mozilla.org/system-message-internal;1"]
                      .getService(Ci.nsISystemMessagesInternal);
    messenger.broadcastMessage('push-register', {});
  },

  _notifyApp: function(aPushRecord) {
    if (!aPushRecord || !aPushRecord.pageURL || !aPushRecord.manifestURL) {
      debug("notifyApp() something is undefined.  Dropping notification");
      return;
    }

    debug("notifyApp() " + aPushRecord.pageURL +
          "  " + aPushRecord.manifestURL);
    var pageURI = Services.io.newURI(aPushRecord.pageURL, null, null);
    var manifestURI = Services.io.newURI(aPushRecord.manifestURL, null, null);
    var message = {
      pushEndpoint: aPushRecord.pushEndpoint,
      version: aPushRecord.version
    };
    let messenger = Cc["@mozilla.org/system-message-internal;1"]
                      .getService(Ci.nsISystemMessagesInternal);
    messenger.sendMessage('push', message, pageURI, manifestURI);
  },

  _updatePushRecord: function(aPushRecord) {
    debug("updatePushRecord()");
    var deferred = Promise.defer();
    this._db.put(aPushRecord, deferred.resolve, deferred.reject);
    return deferred.promise;
  },

  _dropRegistrations: function() {
    var deferred = Promise.defer();
    this._db.drop(deferred.resolve, deferred.reject);
    return deferred.promise;
  },

  receiveMessage: function(aMessage) {
    debug("receiveMessage(): " + aMessage.name);

    if (kCHILD_PROCESS_MESSAGES.indexOf(aMessage.name) == -1) {
      debug("Invalid message from child " + aMessage.name);
      return;
    }

    let mm = aMessage.target.QueryInterface(Ci.nsIMessageSender);
    let json = aMessage.data;
    this[aMessage.name.slice("Push:".length).toLowerCase()](json, mm);
  },

  



  register: function(aPageRecord, aMessageManager) {
    debug("register()");

    let uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                          .getService(Ci.nsIUUIDGenerator);
    
    var channelID = uuidGenerator.generateUUID().toString()
                      .slice(1)
                      .slice(0, -1);

    this._sendRequest("register", {channelID: channelID})
      .then(
        this._onRegisterSuccess.bind(this, aPageRecord, channelID),
        this._onRegisterError.bind(this, aPageRecord, aMessageManager)
      )
      .then(
        function(message) {
          aMessageManager.sendAsyncMessage("PushService:Register:OK", message);
        },
        function() {
          aMessageManager.sendAsyncMessage("PushService:Register:KO", message);
      });
  },

  



  _onRegisterSuccess: function(aPageRecord, generatedChannelID, data) {
    debug("_onRegisterSuccess()");
    var deferred = Promise.defer();
    var message = { requestID: aPageRecord.requestID };

    if (typeof data.channelID !== "string") {
      debug("Invalid channelID " + message);
      message["error"] = "Invalid channelID received";
      throw message;
    }
    else if (data.channelID != generatedChannelID) {
      debug("Server replied with different channelID " + data.channelID +
            " than what UA generated " + generatedChannelID);
      message["error"] = "Server sent 200 status code but different channelID";
      throw message;
    }

    try {
      Services.io.newURI(data.pushEndpoint, null, null);
    }
    catch (e) {
      debug("Invalid pushEndpoint " + data.pushEndpoint);
      message["error"] = "Invalid pushEndpoint " + data.pushEndpoint;
      throw message;
    }

    var record = {
      channelID: data.channelID,
      pushEndpoint: data.pushEndpoint,
      pageURL: aPageRecord.pageURL,
      manifestURL: aPageRecord.manifestURL,
      version: null
    };

    this._updatePushRecord(record)
      .then(
        function() {
          message["pushEndpoint"] = data.pushEndpoint;
          deferred.resolve(message);
        },
        function(error) {
          
          this._sendRequest("unregister", {channelID: record.channelID});
          message["error"] = error;
          deferred.reject(message);
        }
      );

    return deferred.promise;
  },

  



  _onRegisterError: function(aPageRecord, aMessageManager, reply) {
    debug("_onRegisterError()");
    switch (reply.status) {
      case kERROR_CHID_CONFLICT:
        if (typeof aPageRecord._attempts !== "number")
          aPageRecord._attempts = 0;

        if (aPageRecord._attempts < kCONFLICT_RETRY_ATTEMPTS) {
          aPageRecord._attempts++;
          
          debug("CONFLICT: trying again");
          this.register(aPageRecord, aMessageManager);
          return;
        }
        throw { requestID: aPageRecord.requestID, error: "conflict" };
      default:
        debug("General failure " + reply.status);
        throw { requestID: aPageRecord.requestID, error: reply.error };
    }
  },

  























  unregister: function(aPageRecord, aMessageManager) {
    debug("unregister()");

    var fail = function(error) {
      debug("unregister() fail() error " + error);
      var message = {requestID: aPageRecord.requestID, error: error};
      aMessageManager.sendAsyncMessage("PushService:Unregister:KO", message);
    }

    this._db.getByPushEndpoint(aPageRecord.pushEndpoint, function(record) {
      
      if (record.manifestURL !== aPageRecord.manifestURL) {
        aMessageManager.sendAsyncMessage("PushService:Unregister:OK", {
          requestID: aPageRecord.requestID,
          pushEndpoint: aPageRecord.pushEndpoint
        });
        return;
      }

      this._db.delete(record.channelID, function() {
        
        
        this._sendRequest("unregister", {channelID: record.channelID});
        aMessageManager.sendAsyncMessage("PushService:Unregister:OK", {
          requestID: aPageRecord.requestID,
          pushEndpoint: aPageRecord.pushEndpoint
        });
      }.bind(this), fail);
    }.bind(this), fail);
  },

  


  registrations: function(aPageRecord, aMessageManager) {
    debug("registrations()");

    if (aPageRecord.manifestURL) {
      this._db.getAllByManifestURL(aPageRecord.manifestURL,
        this._onRegistrationsSuccess.bind(this, aPageRecord, aMessageManager),
        this._onRegistrationsError.bind(this, aPageRecord, aMessageManager));
    }
    else {
      this._onRegistrationsError(aPageRecord, aMessageManager);
    }
  },

  _onRegistrationsSuccess: function(aPageRecord,
                                    aMessageManager,
                                    pushRecords) {
    var registrations = [];
    pushRecords.forEach(function(pushRecord) {
      registrations.push({
          __exposedProps__: { pushEndpoint: 'r', version: 'r' },
          pushEndpoint: pushRecord.pushEndpoint,
          version: pushRecord.version
      });
    });
    aMessageManager.sendAsyncMessage("PushService:Registrations:OK", {
      requestID: aPageRecord.requestID,
      registrations: registrations
    });
  },

  _onRegistrationsError: function(aPageRecord, aMessageManager) {
    aMessageManager.sendAsyncMessage("PushService:Registrations:KO", {
      requestID: aPageRecord.requestID,
      error: "Database error"
    });
  },

  
  _wsOnStart: function(context) {
    debug("wsOnStart()");
    if (this._currentState != STATE_WAITING_FOR_WS_START) {
      debug("NOT in STATE_WAITING_FOR_WS_START. Current state " +
            this._currentState + ". Skipping");
      return;
    }

    if (this._retryTimeoutTimer)
      this._retryTimeoutTimer.cancel();

    
    this._retryFailCount = 0;

    var data = {
      messageType: "hello",
    }

    if (this._UAID)
      data["uaid"] = this._UAID;

    var networkState = this._getNetworkState();
    if (networkState.ip) {
      
      data["wakeup_hostport"] = {
        ip: networkState.ip,
        port: this._udpPort
      };

      data["mobilenetwork"] = {
        mcc: networkState.mcc,
        mnc: networkState.mnc
      };
    }

    function sendHelloMessage(ids) {
      
      data["channelIDs"] = ids.map ?
                           ids.map(function(el) { return el.channelID; }) : [];
      this._ws.sendMsg(JSON.stringify(data));
      this._currentState = STATE_WAITING_FOR_HELLO;
    }

    this._db.getAllChannelIDs(sendHelloMessage.bind(this),
                              sendHelloMessage.bind(this));
  },

  






  _wsOnStop: function(context, statusCode) {
    debug("wsOnStop()");
    if (statusCode != Cr.NS_OK &&
        !(statusCode == Cr.NS_BASE_STREAM_CLOSED && this._willBeWokenUpByUDP)) {
      debug("Socket error " + statusCode);
      this._socketError(statusCode);
    }

    this._shutdownWS();
  },

  _wsOnMessageAvailable: function(context, message) {
    debug("wsOnMessageAvailable() " + message);
    var reply = undefined;
    try {
      reply = JSON.parse(message);
    } catch(e) {
      debug("Parsing JSON failed. text : " + message);
      return;
    }

    if (typeof reply.messageType != "string") {
      debug("messageType not a string " + reply.messageType);
      return;
    }

    
    
    var handlers = ["Hello", "Register", "Unregister", "Notification"];

    
    
    var handlerName = reply.messageType[0].toUpperCase() +
                      reply.messageType.slice(1).toLowerCase();

    if (handlers.indexOf(handlerName) == -1) {
      debug("No whitelisted handler " + handlerName + ". messageType: " +
            reply.messageType);
      return;
    }

    var handler = "_handle" + handlerName + "Reply";

    if (typeof this[handler] !== "function") {
      debug("Handler whitelisted but not implemented! " + handler);
      return;
    }

    this[handler](reply);
  },

  








  _wsOnServerClose: function(context, aStatusCode, aReason) {
    debug("wsOnServerClose() " + aStatusCode + " " + aReason);

    
    if (aStatusCode == kUDP_WAKEUP_WS_STATUS_CODE) {
      debug("Server closed with promise to wake up");
      this._willBeWokenUpByUDP = true;
      
      this._listenForUDPWakeup();
    }
  },

  _listenForUDPWakeup: function() {
    debug("listenForUDPWakeup()");

    if (this._udpServer) {
      debug("UDP Server already running");
      return;
    }

    if (!this._getNetworkState().ip) {
      debug("No IP");
      return;
    }

    if (!this._prefs.get("udp.wakeupEnabled")) {
      debug("UDP support disabled");
      return;
    }

    this._udpServer = Cc["@mozilla.org/network/server-socket-udp;1"]
                        .createInstance(Ci.nsIUDPServerSocket);
    this._udpServer.init(this._udpPort, false);
    this._udpServer.asyncListen(this);
    debug("listenForUDPWakeup listening on " + this._udpPort);
  },

  



  onPacketReceived: function(aServ, aMessage) {
    debug("Recv UDP datagram on port: " + this._udpPort);
    this._beginWSSetup();
  },

  





  onStopListening: function(aServ, aStatus) {
    debug("UDP Server socket was shutdown. Status: " + aStatus);
    this._beginWSSetup();
  },

  




  _getNetworkState: function() {
    debug("getNetworkState()");

    var networkManager = Cc["@mozilla.org/network/manager;1"]
                           .getService(Ci.nsINetworkManager);
    if (networkManager.active &&
        networkManager.active.type ==
                      Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE) {
      debug("Running on mobile data");
      var mcp = Cc["@mozilla.org/ril/content-helper;1"]
                  .getService(Ci.nsIMobileConnectionProvider);
      if (mcp.iccInfo) {
        return {
          mcc: mcp.iccInfo.mcc,
          mnc: mcp.iccInfo.mnc,
          ip: networkManager.active.ip
        }
      }
    }
    else {
      debug("Running on wifi");
    }

    return {
      mcc: 0,
      mnc: 0,
      ip: undefined
    };
  }
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PushService]);
