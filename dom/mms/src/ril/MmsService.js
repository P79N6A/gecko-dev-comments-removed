





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

Cu.import("resource://gre/modules/NetUtil.jsm");

const RIL_MMSSERVICE_CONTRACTID = "@mozilla.org/mms/rilmmsservice;1";
const RIL_MMSSERVICE_CID = Components.ID("{217ddd76-75db-4210-955d-8806cd8d87f9}");

const DEBUG = false;

const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kXpcomShutdownObserverTopic        = "xpcom-shutdown";
const kPrefenceChangedObserverTopic      = "nsPref:changed";



const HTTP_STATUS_OK = 200;

const CONFIG_SEND_REPORT_NEVER       = 0;
const CONFIG_SEND_REPORT_DEFAULT_NO  = 1;
const CONFIG_SEND_REPORT_DEFAULT_YES = 2;
const CONFIG_SEND_REPORT_ALWAYS      = 3;

const TIME_TO_BUFFER_MMS_REQUESTS    = 30000;
const TIME_TO_RELEASE_MMS_CONNECTION = 30000;

const PREF_RETRIEVAL_MODE = 'dom.mms.retrieval_mode';
const RETRIEVAL_MODE_MANUAL = "manual";
const RETRIEVAL_MODE_AUTOMATIC = "automatic";
const RETRIEVAL_MODE_NEVER = "never";



const DELIVERY_RECEIVED = "received";
const DELIVERY_NOT_DOWNLOADED = "not-downloaded";
const DELIVERY_SENDING = "sending";
const DELIVERY_SENT = "sent";
const DELIVERY_ERROR = "error";

const MAX_RETRY_COUNT = Services.prefs.getIntPref("dom.mms.retrievalRetryCount");
const DELAY_TIME_TO_RETRY = Services.prefs.getIntPref("dom.mms.retrievalRetryInterval");

XPCOMUtils.defineLazyServiceGetter(this, "gpps",
                                   "@mozilla.org/network/protocol-proxy-service;1",
                                   "nsIProtocolProxyService");

XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyServiceGetter(this, "gRIL",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageDatabaseService",
                                   "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1",
                                   "nsIRilMobileMessageDatabaseService");

XPCOMUtils.defineLazyGetter(this, "MMS", function () {
  let MMS = {};
  Cu.import("resource://gre/modules/MmsPduHelper.jsm", MMS);
  return MMS;
});

XPCOMUtils.defineLazyGetter(this, "gMmsConnection", function () {
  let conn = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

    
    mmsc: null,
    proxy: null,
    port: null,

    proxyInfo: null,
    settings: ["ril.mms.mmsc",
               "ril.mms.mmsproxy",
               "ril.mms.mmsport"],
    connected: false,

    
    
    
    pendingCallbacks: [],

    
    refCount: 0,

    connectTimer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),

    disconnectTimer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),

    


    onConnectTimerTimeout: function onConnectTimerTimeout() {
      debug("onConnectTimerTimeout: " + this.pendingCallbacks.length
            + " pending callbacks");
      while (this.pendingCallbacks.length) {
        let callback = this.pendingCallbacks.shift();
        callback(false);
      }
    },

    


    onDisconnectTimerTimeout: function onDisconnectTimerTimeout() {
      debug("onDisconnectTimerTimeout: deactivate the MMS data call.");
      if (this.connected) {
        gRIL.deactivateDataCallByType("mms");
      }
    },

    init: function init() {
      Services.obs.addObserver(this, kNetworkInterfaceStateChangedTopic,
                               false);
      Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
      this.settings.forEach(function(name) {
        Services.prefs.addObserver(name, this, false);
      }, this);

      try {
        this.mmsc = Services.prefs.getCharPref("ril.mms.mmsc");
        this.proxy = Services.prefs.getCharPref("ril.mms.mmsproxy");
        this.port = Services.prefs.getIntPref("ril.mms.mmsport");
        this.updateProxyInfo();
      } catch (e) {
        debug("Unable to initialize the MMS proxy settings from the" +
              "preference. This could happen at the first-run. Should be" +
              "available later.");
        this.clearMmsProxySettings();
      }
    },

    










    acquire: function acquire(callback) {
      this.connectTimer.cancel();

      
      
      if (!this.connected) {
        debug("acquire: buffer the MMS request and setup the MMS data call.");
        this.pendingCallbacks.push(callback);
        gRIL.setupDataCallByType("mms");

        
        
        this.connectTimer.
          initWithCallback(this.onConnectTimerTimeout.bind(this),
                           TIME_TO_BUFFER_MMS_REQUESTS,
                           Ci.nsITimer.TYPE_ONE_SHOT);
        return false;
      }

      this.refCount++;

      callback(true);
      return true;
    },

    


    release: function release() {
      this.refCount--;
      if (this.refCount <= 0) {
        this.refCount = 0;

        
        
        this.disconnectTimer.
          initWithCallback(this.onDisconnectTimerTimeout.bind(this),
                           TIME_TO_RELEASE_MMS_CONNECTION,
                           Ci.nsITimer.TYPE_ONE_SHOT);
      }
    },

    


    updateProxyInfo: function updateProxyInfo() {
      if (this.proxy === null || this.port === null) {
        debug("updateProxyInfo: proxy or port is not yet decided." );
        return;
      }

      this.proxyInfo =
        gpps.newProxyInfo("http", this.proxy, this.port,
                          Ci.nsIProxyInfo.TRANSPARENT_PROXY_RESOLVES_HOST,
                          -1, null);
      debug("updateProxyInfo: " + JSON.stringify(this.proxyInfo));
    },

    


    clearMmsProxySettings: function clearMmsProxySettings() {
      this.mmsc = null;
      this.proxy = null;
      this.port = null;
      this.proxyInfo = null;
    },

    shutdown: function shutdown() {
      Services.obs.removeObserver(this, kNetworkInterfaceStateChangedTopic);
      this.settings.forEach(function(name) {
        Services.prefs.removeObserver(name, this);
      }, this);
      this.connectTimer.cancel();
      this.onConnectTimerTimeout();
      this.disconnectTimer.cancel();
      this.onDisconnectTimerTimeout();
    },

    

    observe: function observe(subject, topic, data) {
      switch (topic) {
        case kNetworkInterfaceStateChangedTopic: {
          this.connected =
            gRIL.getDataCallStateByType("mms") ==
              Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED;

          if (!this.connected) {
            return;
          }

          debug("Got the MMS network connected! Resend the buffered " +
                "MMS requests: number: " + this.pendingCallbacks.length);
          this.connectTimer.cancel();
          while (this.pendingCallbacks.length) {
            let callback = this.pendingCallbacks.shift();
            callback(true);
          }
          break;
        }
        case kPrefenceChangedObserverTopic: {
          try {
            switch (data) {
              case "ril.mms.mmsc":
                this.mmsc = Services.prefs.getCharPref("ril.mms.mmsc");
                break;
              case "ril.mms.mmsproxy":
                this.proxy = Services.prefs.getCharPref("ril.mms.mmsproxy");
                this.updateProxyInfo();
                break;
              case "ril.mms.mmsport":
                this.port = Services.prefs.getIntPref("ril.mms.mmsport");
                this.updateProxyInfo();
                break;
              default:
                break;
            }
          } catch (e) {
            debug("Failed to update the MMS proxy settings from the" +
                  "preference.");
            this.clearMmsProxySettings();
          }
          break;
        }
        case kXpcomShutdownObserverTopic: {
          Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
          this.shutdown();
        }
      }
    }
  };
  conn.init();

  return conn;
});

function MmsProxyFilter(url) {
  this.url = url;
}
MmsProxyFilter.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolProxyFilter]),

  

  applyFilter: function applyFilter(proxyService, uri, proxyInfo) {
    let url = uri.prePath + uri.path;
    if (url.endsWith("/")) {
      url = url.substr(0, url.length - 1);
    }

    if (this.url != url) {
      debug("applyFilter: content uri = " + this.url +
            " is not matched url = " + url + " .");
      return proxyInfo;
    }
    
    debug("applyFilter: MMSC is matched: " +
          JSON.stringify({ url: this.url,
                           proxyInfo: gMmsConnection.proxyInfo }));
    return gMmsConnection.proxyInfo ? gMmsConnection.proxyInfo : proxyInfo;
  }
};

XPCOMUtils.defineLazyGetter(this, "gMmsTransactionHelper", function () {
  return {
    












    sendRequest: function sendRequest(method, url, istream, callback) {
      
      

      gMmsConnection.acquire((function (method, url, istream, callback,
                                        connected) {
        if (!connected) {
          
          gMmsConnection.release();
          if (callback) {
            callback(0, null);
          }
          return;
        }

        debug("sendRequest: register proxy filter to " + url);
        let proxyFilter = new MmsProxyFilter(url);
        gpps.registerFilter(proxyFilter, 0);

        let releaseMmsConnectionAndCallback = (function (httpStatus, data) {
          gpps.unregisterFilter(proxyFilter);
          
          gMmsConnection.release();
          if (callback) {
            callback(httpStatus, data);
          }
        }).bind(this);

        try {
          let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                    .createInstance(Ci.nsIXMLHttpRequest);

          
          xhr.open(method, url, true);
          xhr.responseType = "arraybuffer";
          if (istream) {
            xhr.setRequestHeader("Content-Type",
                                 "application/vnd.wap.mms-message");
            xhr.setRequestHeader("Content-Length", istream.available());
          } else {
            xhr.setRequestHeader("Content-Length", 0);
          }

          
          let uaProfUrl, uaProfTagname = "x-wap-profile";
          try {
            uaProfUrl = Services.prefs.getCharPref('wap.UAProf.url');
            uaProfTagname = Services.prefs.getCharPref('wap.UAProf.tagname');
          } catch (e) {}

          if (uaProfUrl) {
            xhr.setRequestHeader(uaProfTagname, uaProfUrl);
          }

          
          xhr.onerror = function () {
            debug("xhr error, response headers: " +
                  xhr.getAllResponseHeaders());
            releaseMmsConnectionAndCallback(xhr.status, null);
          };
          xhr.onreadystatechange = function () {
            if (xhr.readyState != Ci.nsIXMLHttpRequest.DONE) {
              return;
            }

            let data = null;
            switch (xhr.status) {
              case HTTP_STATUS_OK: {
                debug("xhr success, response headers: "
                      + xhr.getAllResponseHeaders());

                let array = new Uint8Array(xhr.response);
                if (false) {
                  for (let begin = 0; begin < array.length; begin += 20) {
                    let partial = array.subarray(begin, begin + 20);
                    debug("res: " + JSON.stringify(partial));
                  }
                }

                data = {array: array, offset: 0};
                break;
              }
              default: {
                debug("xhr done, but status = " + xhr.status);
                break;
              }
            }

            releaseMmsConnectionAndCallback(xhr.status, data);
          }

          
          xhr.send(istream);
        } catch (e) {
          debug("xhr error, can't send: " + e.message);
          releaseMmsConnectionAndCallback(0, null);
        }
      }).bind(this, method, url, istream, callback));
    }
  };
});













function NotifyResponseTransaction(transactionId, status, reportAllowed) {
  let headers = {};

  
  headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_NOTIFYRESP_IND;
  headers["x-mms-transaction-id"] = transactionId;
  headers["x-mms-mms-version"] = MMS.MMS_VERSION;
  headers["x-mms-status"] = status;
  
  headers["x-mms-report-allowed"] = reportAllowed;

  this.istream = MMS.PduHelper.compose(null, {headers: headers});
}
NotifyResponseTransaction.prototype = {
  



  run: function run(callback) {
    let requestCallback;
    if (callback) {
      requestCallback = function (httpStatus, data) {
        
        
        
        callback(httpStatus);
      };
    }
    gMmsTransactionHelper.sendRequest("POST", gMmsConnection.mmsc,
                                      this.istream, requestCallback);
  }
};







function RetrieveTransaction(contentLocation) {
  this.contentLocation = contentLocation;
}
RetrieveTransaction.prototype = {
  




  run: function run(callback) {
    this.retryCount = 0;
    let that = this;
    this.retrieve((function retryCallback(mmsStatus, msg) {
      if (MMS.MMS_PDU_STATUS_DEFERRED == mmsStatus &&
          that.retryCount < MAX_RETRY_COUNT) {
        that.retryCount++;
        let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        timer.initWithCallback((function (){
                                 this.retrieve(retryCallback);
                               }).bind(that),
                               DELAY_TIME_TO_RETRY,
                               Ci.nsITimer.TYPE_ONE_SHOT);
        return;
      }
      if (callback) {
        callback(mmsStatus, msg);
      }
    }).bind(this));
  },

  




  retrieve: function retrieve(callback) {
    gMmsTransactionHelper.sendRequest("GET", this.contentLocation, null,
                                      (function (httpStatus, data) {
      if ((httpStatus != HTTP_STATUS_OK) || !data) {
        callback(MMS.MMS_PDU_STATUS_DEFERRED, null);
        return;
      }

      let retrieved = MMS.PduHelper.parse(data, null);
      if (!retrieved || (retrieved.type != MMS.MMS_PDU_TYPE_RETRIEVE_CONF)) {
        callback(MMS.MMS_PDU_STATUS_UNRECOGNISED, null);
        return;
      }

      
      if (retrieved.headers["x-mms-delivery-report"] == null) {
        retrieved.headers["x-mms-delivery-report"] = false;
      }

      let retrieveStatus = retrieved.headers["x-mms-retrieve-status"];
      if ((retrieveStatus != null) &&
          (retrieveStatus != MMS.MMS_PDU_ERROR_OK)) {
        callback(MMS.translatePduErrorToStatus(retrieveStatus),
                        retrieved);
        return;
      }

      callback(MMS.MMS_PDU_STATUS_RETRIEVED, retrieved);
    }).bind(this));
  }
};





function SendTransaction(msg) {
  msg.headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_SEND_REQ;
  if (!msg.headers["x-mms-transaction-id"]) {
    
    let tid = gUUIDGenerator.generateUUID().toString();
    msg.headers["x-mms-transaction-id"] = tid;
  }
  msg.headers["x-mms-mms-version"] = MMS.MMS_VERSION;

  
  msg.headers["from"] = null;

  msg.headers["date"] = new Date();
  msg.headers["x-mms-message-class"] = "personal";
  msg.headers["x-mms-expiry"] = 7 * 24 * 60 * 60;
  msg.headers["x-mms-priority"] = 129;
  msg.headers["x-mms-read-report"] = true;
  msg.headers["x-mms-delivery-report"] = true;

  
  

  let messageSize = 0;

  if (msg.content) {
    messageSize = msg.content.length;
  } else if (msg.parts) {
    for (let i = 0; i < msg.parts.length; i++) {
      if (msg.parts[i].content.size) {
        messageSize += msg.parts[i].content.size;
      } else {
        messageSize += msg.parts[i].content.length;
      }
    }

    let contentType = {
      params: {
        
        
        type: msg.parts[0].headers["content-type"].media,
      },
    };

    
    
    
    
    if (contentType.params.type === "application/smil") {
      contentType.media = "application/vnd.wap.multipart.related";

      
      
      contentType.params.start = msg.parts[0].headers["content-id"];
    } else {
      contentType.media = "application/vnd.wap.multipart.mixed";
    }

    
    msg.headers["content-type"] = contentType;
  }

  
  msg.headers["x-mms-message-size"] = messageSize;
  

  debug("msg: " + JSON.stringify(msg));

  this.msg = msg;
}
SendTransaction.prototype = {
  istreamComposed: false,

  





  loadBlobs: function loadBlobs(parts, callback) {
    let callbackIfValid = function callbackIfValid() {
      debug("All parts loaded: " + JSON.stringify(parts));
      if (callback) {
        callback();
      }
    }

    if (!parts || !parts.length) {
      callbackIfValid();
      return;
    }

    let numPartsToLoad = parts.length;
    for each (let part in parts) {
      if (!(part.content instanceof Ci.nsIDOMBlob)) {
        numPartsToLoad--;
        if (!numPartsToLoad) {
          callbackIfValid();
          return;
        }
        continue;
      }
      let fileReader = Cc["@mozilla.org/files/filereader;1"]
                       .createInstance(Ci.nsIDOMFileReader);
      fileReader.addEventListener("loadend",
        (function onloadend(part, event) {
        let arrayBuffer = event.target.result;
        part.content = new Uint8Array(arrayBuffer);
        numPartsToLoad--;
        if (!numPartsToLoad) {
          callbackIfValid();
        }
      }).bind(null, part));
      fileReader.readAsArrayBuffer(part.content);
    };
  },

  




  run: function run(callback) {
    if (!this.istreamComposed) {
      this.loadBlobs(this.msg.parts, (function () {
        this.istream = MMS.PduHelper.compose(null, this.msg);
        this.istreamComposed = true;
        this.run(callback);
      }).bind(this));
      return;
    }

    let callbackIfValid = function callbackIfValid(mmsStatus, msg) {
      if (callback) {
        callback(mmsStatus, msg);
      }
    }

    if (!this.istream) {
      callbackIfValid(MMS.MMS_PDU_ERROR_PERMANENT_FAILURE, null);
      return;
    }

    this.retryCount = 0;
    let retryCallback = (function (mmsStatus, msg) {
      if ((MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE == mmsStatus ||
            MMS.MMS_PDU_ERROR_PERMANENT_FAILURE == mmsStatus) &&
          this.retryCount < MAX_RETRY_COUNT) {
        this.retryCount++;

        let timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
        timer.initWithCallback(this.send.bind(this, retryCallback),
                               DELAY_TIME_TO_RETRY,
                               Ci.nsITimer.TYPE_ONE_SHOT);
        return;
      }

      callbackIfValid(mmsStatus, msg);
    }).bind(this);
    this.send(retryCallback);
  },

  




  send: function send(callback) {
    gMmsTransactionHelper.sendRequest("POST", gMmsConnection.mmsc, this.istream,
                                      function (httpStatus, data) {
      if (httpStatus != HTTP_STATUS_OK) {
        callback(MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE, null);
        return;
      }

      if (!data) {
        callback(MMS.MMS_PDU_ERROR_PERMANENT_FAILURE, null);
        return;
      }

      let response = MMS.PduHelper.parse(data, null);
      if (!response || (response.type != MMS.MMS_PDU_TYPE_SEND_CONF)) {
        callback(MMS.MMS_PDU_RESPONSE_ERROR_UNSUPPORTED_MESSAGE, null);
        return;
      }

      let responseStatus = response.headers["x-mms-response-status"];
      callback(responseStatus, response);
    });
  }
};











function AcknowledgeTransaction(transactionId, reportAllowed) {
  let headers = {};

  
  headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_ACKNOWLEDGE_IND;
  headers["x-mms-transaction-id"] = transactionId;
  headers["x-mms-mms-version"] = MMS.MMS_VERSION;
  
  headers["x-mms-report-allowed"] = reportAllowed;

  this.istream = MMS.PduHelper.compose(null, {headers: headers});
}
AcknowledgeTransaction.prototype = {
  



  run: function run(callback) {
    let requestCallback;
    if (callback) {
      requestCallback = function (httpStatus, data) {
        
        
        
        callback(httpStatus);
      };
    }
    gMmsTransactionHelper.sendRequest("POST", gMmsConnection.mmsc,
                                      this.istream, requestCallback);
  }
};




function MmsService() {
  
}
MmsService.prototype = {

  classID:   RIL_MMSSERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMmsService,
                                         Ci.nsIWapPushApplication]),
  



  confSendDeliveryReport: CONFIG_SEND_REPORT_DEFAULT_YES,

  







  getReportAllowed: function getReportAllowed(config, wish) {
    if ((config == CONFIG_SEND_REPORT_DEFAULT_NO)
        || (config == CONFIG_SEND_REPORT_DEFAULT_YES)) {
      if (wish != null) {
        config += (wish ? 1 : -1);
      }
    }
    return config >= CONFIG_SEND_REPORT_DEFAULT_YES;
  },

  





  convertFromIntermediateToSavable: function convertFromIntermediateToSavable(intermediate) {
    intermediate.type = "mms";
    intermediate.delivery = DELIVERY_NOT_DOWNLOADED;
    intermediate.timestamp = Date.now();
    intermediate.sender = null;
    if (intermediate.headers.from) {
      intermediate.sender = intermediate.headers.from.address;
    } else {
      record.sender = "anonymous";
    }
    intermediate.receivers = [];
    return intermediate;
  },

  







  mergeRetrievalConfirmationIntoRecord: function mergeRetrievalConfirmationIntoRecord(intermediate, record) {
    if (intermediate.headers["Date"]) {
      record.timestamp = Date.parse(intermediate.headers["Date"]);
    }
    if (intermediate.headers.from) {
      record.sender = intermediate.headers.from.address;
    } else {
      record.sender = "anonymous";
    }
    record.receivers = [];
    
    for each (let type in ["cc", "to"]) {
      if (intermediate.headers[type]) {
        if (intermediate.headers[type] instanceof Array) {
          for (let index in intermediate.headers[type]) {
            record.receivers.push(intermediate.headers[type][index].address)
          }
        } else {
          record.receivers.push(intermediate.headers[type].address);
        }
      }
    }

    record.delivery = DELIVERY_RECEIVED;
    for (let field in intermediate.headers) {
      record.headers[field] = intermediate.headers[field];
    }
    if (intermediate.parts) {
      record.parts = intermediate.parts;
    }
    if (intermediate.content) {
      record.content = intermediate.content;
    }
    return record;
  },

  






  retrieveMessage: function retrieveMessage(contentLocation, callback) {
    
    
    
    

    let transaction = new RetrieveTransaction(contentLocation);
    transaction.run(callback);
  },

  





  handleNotificationIndication: function handleNotificationIndication(notification) {
    

    let url = notification.headers["x-mms-content-location"].uri;
    
    

    let transactionId = notification.headers["x-mms-transaction-id"];
    
    let wish = notification.headers["x-mms-delivery-report"];

    notification = this.convertFromIntermediateToSavable(notification);

    gMobileMessageDatabaseService.saveReceivedMessage(notification,
      (function (rv, messageRecord) {
        
        
        let success = Components.isSuccessCode(rv);
        if (!success) {
          
          
          
          debug("Could not store MMS " + JSON.stringify(notification) +
                ", error code " + rv);
          
          
          
          return;
        }

        let retrievalMode = RETRIEVAL_MODE_MANUAL;
        try {
          retrievalMode = Services.prefs.getCharPref(PREF_RETRIEVAL_MODE);
        } catch (e) {}

        if (RETRIEVAL_MODE_AUTOMATIC !== retrievalMode) {
          let mmsStatus = RETRIEVAL_MODE_NEVER === retrievalMode ?
              MMS.MMS_PDU_STATUS_REJECTED : MMS.MMS_PDU_STATUS_DEFERRED;
          
          let reportAllowed = this.getReportAllowed(this.confSendDeliveryReport, wish);

          let transaction = new NotifyResponseTransaction(transactionId,
                                                          mmsStatus,
                                                          reportAllowed);
          transaction.run();
        }

        this.retrieveMessage(url, (function responseNotify(mmsStatus,
                                                           retrievedMsg) {
          
          
          
          if ((wish == null) && retrievedMsg) {
            wish = retrievedMsg.headers["x-mms-delivery-report"];
          }
          let reportAllowed = this.getReportAllowed(
            this.confSendDeliveryReport, wish);

          
          debug("retrievedMsg = " + JSON.stringify(retrievedMsg));

          
          
          if (MMS.MMS_PDU_STATUS_RETRIEVED !== mmsStatus) {
            let transaction =
              new NotifyResponseTransaction(transactionId, mmsStatus, reportAllowed);
            transaction.run();
          }

          messageRecord = this.mergeRetrievalConfirmationIntoRecord(retrievedMsg, messageRecord);
          gMobileMessageDatabaseService.saveReceivedMessage(messageRecord,
            (function (rv, messageRecord) {
              
              
              let success = Components.isSuccessCode(rv);
              if (!success) {
                
                
                
                debug("Could not store MMS " + messageRecord.id +
                      ", error code " + rv);

                let transaction = new NotifyResponseTransaction(
                  transactionId, MMS.MMS_PDU_STATUS_DEFERRED, reportAllowed);
                transaction.run();
                return;
              }
            }).bind(this)
          );
        }).bind(this));
        return;
      }).bind(this)
    );
  },

  





  handleDeliveryIndication: function handleDeliveryIndication(msg) {
    
    let messageId = msg.headers["message-id"];
    debug("handleDeliveryIndication: got delivery report for " + messageId);
  },

  

  hasSupport: function hasSupport() {
    return true;
  },

  

  receiveWapPush: function receiveWapPush(array, length, offset, options) {
    let data = {array: array, offset: offset};
    let msg = MMS.PduHelper.parse(data, null);
    if (!msg) {
      return false;
    }
    debug("receiveWapPush: msg = " + JSON.stringify(msg));

    switch (msg.type) {
      case MMS.MMS_PDU_TYPE_NOTIFICATION_IND:
        this.handleNotificationIndication(msg);
        break;
      case MMS.MMS_PDU_TYPE_DELIVERY_IND:
        this.handleDeliveryIndication(msg);
        break;
      default:
        debug("Unsupported X-MMS-Message-Type: " + msg.type);
        break;
    }
  },
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MmsService]);

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-@- MmsService: " + s + "\n");
  };
} else {
  debug = function (s) {};
}
