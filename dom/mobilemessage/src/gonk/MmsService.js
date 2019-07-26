





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");

const RIL_MMSSERVICE_CONTRACTID = "@mozilla.org/mms/rilmmsservice;1";
const RIL_MMSSERVICE_CID = Components.ID("{217ddd76-75db-4210-955d-8806cd8d87f9}");

let DEBUG = false;


try {
  let debugPref = Services.prefs.getBoolPref("mms.debugging.enabled");
  DEBUG = DEBUG || debugPref;
} catch (e) {}

const kSmsSendingObserverTopic           = "sms-sending";
const kSmsSentObserverTopic              = "sms-sent";
const kSmsFailedObserverTopic            = "sms-failed";
const kSmsReceivedObserverTopic          = "sms-received";
const kSmsRetrievingObserverTopic        = "sms-retrieving";
const kSmsDeliverySuccessObserverTopic   = "sms-delivery-success";
const kSmsDeliveryErrorObserverTopic     = "sms-delivery-error";

const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kXpcomShutdownObserverTopic        = "xpcom-shutdown";
const kPrefenceChangedObserverTopic      = "nsPref:changed";
const kMobileMessageDeletedObserverTopic = "mobile-message-deleted";



const HTTP_STATUS_OK = 200;


const _HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS = 0;
const _HTTP_STATUS_USER_CANCELLED = -1;
const _HTTP_STATUS_RADIO_DISABLED = -2;
const _HTTP_STATUS_NO_SIM_CARD = -3;
const _HTTP_STATUS_ACQUIRE_TIMEOUT = 4;


const _MMS_ERROR_MESSAGE_DELETED = -1;
const _MMS_ERROR_RADIO_DISABLED = -2;
const _MMS_ERROR_NO_SIM_CARD = -3;

const CONFIG_SEND_REPORT_NEVER       = 0;
const CONFIG_SEND_REPORT_DEFAULT_NO  = 1;
const CONFIG_SEND_REPORT_DEFAULT_YES = 2;
const CONFIG_SEND_REPORT_ALWAYS      = 3;

const TIME_TO_BUFFER_MMS_REQUESTS    = 30000;
const PREF_TIME_TO_RELEASE_MMS_CONNECTION =
  Services.prefs.getIntPref("network.gonk.ms-release-mms-connection");

const PREF_RETRIEVAL_MODE      = 'dom.mms.retrieval_mode';
const RETRIEVAL_MODE_MANUAL    = "manual";
const RETRIEVAL_MODE_AUTOMATIC = "automatic";
const RETRIEVAL_MODE_AUTOMATIC_HOME = "automatic-home";
const RETRIEVAL_MODE_NEVER     = "never";



const DELIVERY_RECEIVED       = "received";
const DELIVERY_NOT_DOWNLOADED = "not-downloaded";
const DELIVERY_SENDING        = "sending";
const DELIVERY_SENT           = "sent";
const DELIVERY_ERROR          = "error";

const DELIVERY_STATUS_SUCCESS        = "success";
const DELIVERY_STATUS_PENDING        = "pending";
const DELIVERY_STATUS_ERROR          = "error";
const DELIVERY_STATUS_REJECTED       = "rejected";
const DELIVERY_STATUS_MANUAL         = "manual";
const DELIVERY_STATUS_NOT_APPLICABLE = "not-applicable";

const PREF_SEND_RETRY_COUNT =
  Services.prefs.getIntPref("dom.mms.sendRetryCount");

const PREF_SEND_RETRY_INTERVAL =
  Services.prefs.getIntPref("dom.mms.sendRetryInterval");

const PREF_RETRIEVAL_RETRY_COUNT =
  Services.prefs.getIntPref("dom.mms.retrievalRetryCount");

const PREF_RETRIEVAL_RETRY_INTERVALS = (function () {
  let intervals =
    Services.prefs.getCharPref("dom.mms.retrievalRetryIntervals").split(",");
  for (let i = 0; i < PREF_RETRIEVAL_RETRY_COUNT; ++i) {
    intervals[i] = parseInt(intervals[i], 10);
    
    
    if (!intervals[i]) {
      intervals[i] = 600000;
    }
  }
  intervals.length = PREF_RETRIEVAL_RETRY_COUNT;
  return intervals;
})();

XPCOMUtils.defineLazyServiceGetter(this, "gpps",
                                   "@mozilla.org/network/protocol-proxy-service;1",
                                   "nsIProtocolProxyService");

XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyGetter(this, "gRadioInterface", function () {
  let ril = Cc["@mozilla.org/ril;1"].getService(Ci["nsIRadioInterfaceLayer"]);
  
  return ril.getRadioInterface(0);
});

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageDatabaseService",
                                   "@mozilla.org/mobilemessage/rilmobilemessagedatabaseservice;1",
                                   "nsIRilMobileMessageDatabaseService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

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

    
    radioDisabled: false,

    proxyInfo: null,
    settings: ["ril.mms.mmsc",
               "ril.mms.mmsproxy",
               "ril.mms.mmsport",
               "ril.radio.disabled"],
    connected: false,

    
    
    
    pendingCallbacks: [],

    
    refCount: 0,

    connectTimer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),

    disconnectTimer: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),

    


    flushPendingCallbacks: function flushPendingCallbacks(status) {
      if (DEBUG) debug("flushPendingCallbacks: " + this.pendingCallbacks.length
                       + " pending callbacks with status: " + status);
      while (this.pendingCallbacks.length) {
        let callback = this.pendingCallbacks.shift();
        let connected = (status == _HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS);
        callback(connected, status);
      }
    },

    


    onDisconnectTimerTimeout: function onDisconnectTimerTimeout() {
      if (DEBUG) debug("onDisconnectTimerTimeout: deactivate the MMS data call.");
      if (this.connected) {
        gRadioInterface.deactivateDataCallByType("mms");
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
        if (DEBUG) debug("Unable to initialize the MMS proxy settings from " +
                         "the preference. This could happen at the first-run. " +
                         "Should be available later.");
        this.clearMmsProxySettings();
      }

      try {
        this.radioDisabled = Services.prefs.getBoolPref("ril.radio.disabled");
      } catch (e) {
        if (DEBUG) debug("Getting preference 'ril.radio.disabled' fails.");
        this.radioDisabled = false;
      }

      this.connected = gRadioInterface.getDataCallStateByType("mms") ==
        Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED;
    },

    




    isVoiceRoaming: function isVoiceRoaming() {
      let isRoaming = gRadioInterface.rilContext.voice.roaming;
      if (DEBUG) debug("isVoiceRoaming = " + isRoaming);
      return isRoaming;
    },

    










    acquire: function acquire(callback) {
      this.connectTimer.cancel();

      
      
      if (!this.connected) {
        this.pendingCallbacks.push(callback);

        let errorStatus;
        if (this.radioDisabled) {
          if (DEBUG) debug("Error! Radio is disabled when sending MMS.");
          errorStatus = _HTTP_STATUS_RADIO_DISABLED;
        } else if (gRadioInterface.rilContext.cardState != "ready") {
          if (DEBUG) debug("Error! SIM card is not ready when sending MMS.");
          errorStatus = _HTTP_STATUS_NO_SIM_CARD;
        }
        if (errorStatus != null) {
          this.flushPendingCallbacks(errorStatus);
          return true;
        }

        if (DEBUG) debug("acquire: buffer the MMS request and setup the MMS data call.");
        gRadioInterface.setupDataCallByType("mms");

        
        
        this.connectTimer.
          initWithCallback(this.flushPendingCallbacks.bind(this, _HTTP_STATUS_ACQUIRE_TIMEOUT),
                           TIME_TO_BUFFER_MMS_REQUESTS,
                           Ci.nsITimer.TYPE_ONE_SHOT);
        return false;
      }

      this.refCount++;

      callback(true, _HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS);
      return true;
    },

    


    release: function release() {
      this.refCount--;
      if (this.refCount <= 0) {
        this.refCount = 0;

        
        if (PREF_TIME_TO_RELEASE_MMS_CONNECTION < 1000) {
          this.onDisconnectTimerTimeout();
          return;
        }

        
        
        this.disconnectTimer.
          initWithCallback(this.onDisconnectTimerTimeout.bind(this),
                           PREF_TIME_TO_RELEASE_MMS_CONNECTION,
                           Ci.nsITimer.TYPE_ONE_SHOT);
      }
    },

    


    updateProxyInfo: function updateProxyInfo() {
      if (this.proxy === null || this.port === null) {
        if (DEBUG) debug("updateProxyInfo: proxy or port is not yet decided." );
        return;
      }

      if (!this.port) {
        this.port = 80;
        if (DEBUG) debug("updateProxyInfo: port is 0. Set to defult port 80.");
      }

      this.proxyInfo =
        gpps.newProxyInfo("http", this.proxy, this.port,
                          Ci.nsIProxyInfo.TRANSPARENT_PROXY_RESOLVES_HOST,
                          -1, null);
      if (DEBUG) debug("updateProxyInfo: " + JSON.stringify(this.proxyInfo));
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
      this.flushPendingCallbacks(_HTTP_STATUS_RADIO_DISABLED);
      this.disconnectTimer.cancel();
      this.onDisconnectTimerTimeout();
    },

    

    observe: function observe(subject, topic, data) {
      switch (topic) {
        case kNetworkInterfaceStateChangedTopic: {
          this.connected =
            gRadioInterface.getDataCallStateByType("mms") ==
              Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED;

          if (!this.connected) {
            return;
          }

          if (DEBUG) debug("Got the MMS network connected! Resend the buffered " +
                           "MMS requests: number: " + this.pendingCallbacks.length);
          this.connectTimer.cancel();
          this.flushPendingCallbacks(_HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS)
          break;
        }
        case kPrefenceChangedObserverTopic: {
          if (data == "ril.radio.disabled") {
            try {
              this.radioDisabled = Services.prefs.getBoolPref("ril.radio.disabled");
            } catch (e) {
              if (DEBUG) debug("Updating preference 'ril.radio.disabled' fails.");
              this.radioDisabled = false;
            }
            return;
          }

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
            if (DEBUG) debug("Failed to update the MMS proxy settings from the" +
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
  this.uri = Services.io.newURI(url, null, null);
}
MmsProxyFilter.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolProxyFilter]),

  

  applyFilter: function applyFilter(proxyService, uri, proxyInfo) {
    if (!this.uri.equals(uri)) {
      if (DEBUG) debug("applyFilter: content uri = " + JSON.stringify(this.uri) +
                       " is not matched with uri = " + JSON.stringify(uri) + " .");
      return proxyInfo;
    }
    
    if (DEBUG) debug("applyFilter: MMSC/Content Location is matched with: " +
                     JSON.stringify({ uri: JSON.stringify(this.uri),
                                      proxyInfo: gMmsConnection.proxyInfo }));
    return gMmsConnection.proxyInfo ? gMmsConnection.proxyInfo : proxyInfo;
  }
};

XPCOMUtils.defineLazyGetter(this, "gMmsTransactionHelper", function () {
  let helper = {
    












    sendRequest: function sendRequest(method, url, istream, callback) {
      
      let cancellable = {
        callback: callback,

        isDone: false,
        isCancelled: false,

        cancel: function cancel() {
          if (this.isDone) {
            
            return;
          }

          this.isCancelled = true;
          if (this.isAcquiringConn) {
            
            
            
            this.done(_HTTP_STATUS_USER_CANCELLED, null);
          } else if (this.xhr) {
            
            this.xhr.abort();
          }
        },

        done: function done(httpStatus, data) {
          this.isDone = true;
          if (!this.callback) {
            return;
          }

          if (this.isCancelled) {
            this.callback(_HTTP_STATUS_USER_CANCELLED, null);
          } else {
            this.callback(httpStatus, data);
          }
        }
      };

      cancellable.isAcquiringConn =
        !gMmsConnection.acquire((function (connected, errorCode) {

        cancellable.isAcquiringConn = false;

        if (!connected || cancellable.isCancelled) {
          gMmsConnection.release();

          if (!cancellable.isDone) {
            cancellable.done(cancellable.isCancelled ?
                             _HTTP_STATUS_USER_CANCELLED : errorCode, null);
          }
          return;
        }

        if (DEBUG) debug("sendRequest: register proxy filter to " + url);
        let proxyFilter = new MmsProxyFilter(url);
        gpps.registerFilter(proxyFilter, 0);

        cancellable.xhr = this.sendHttpRequest(method, url, istream, proxyFilter,
                                               cancellable.done.bind(cancellable));
      }).bind(this));

      return cancellable;
    },

    sendHttpRequest: function sendHttpRequest(method, url, istream, proxyFilter,
                                              callback) {
      let releaseMmsConnectionAndCallback = function (httpStatus, data) {
        gpps.unregisterFilter(proxyFilter);
        
        gMmsConnection.release();
        callback(httpStatus, data);
      };

      try {
        let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);

        
        xhr.open(method, url, true);
        xhr.responseType = "arraybuffer";
        if (istream) {
          xhr.setRequestHeader("Content-Type",
                               "application/vnd.wap.mms-message");
          xhr.setRequestHeader("Content-Length", istream.available());
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
          if (DEBUG) debug("xhr error, response headers: " +
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
              if (DEBUG) debug("xhr success, response headers: "
                               + xhr.getAllResponseHeaders());
              let array = new Uint8Array(xhr.response);
              if (false) {
                for (let begin = 0; begin < array.length; begin += 20) {
                  let partial = array.subarray(begin, begin + 20);
                  if (DEBUG) debug("res: " + JSON.stringify(partial));
                }
              }

              data = {array: array, offset: 0};
              break;
            }

            default: {
              if (DEBUG) debug("xhr done, but status = " + xhr.status +
                               ", statusText = " + xhr.statusText);
              break;
            }
          }
          releaseMmsConnectionAndCallback(xhr.status, data);
        };
        
        xhr.send(istream);
        return xhr;
      } catch (e) {
        if (DEBUG) debug("xhr error, can't send: " + e.message);
        releaseMmsConnectionAndCallback(0, null);
        return null;
      }
    },

    







    countRecipients: function countRecipients(recipients) {
      if (recipients && recipients.address) {
        return 1;
      }
      let totalRecipients = 0;
      if (!Array.isArray(recipients)) {
        return 0;
      }
      totalRecipients += recipients.length;
      for (let ix = 0; ix < recipients.length; ++ix) {
        if (recipients[ix].address.length > MMS.MMS_MAX_LENGTH_RECIPIENT) {
          throw new Error("MMS_MAX_LENGTH_RECIPIENT error");
        }
        if (recipients[ix].type === "email") {
          let found = recipients[ix].address.indexOf("<");
          let lenMailbox = recipients[ix].address.length - found;
          if(lenMailbox > MMS.MMS_MAX_LENGTH_MAILBOX_PORTION) {
            throw new Error("MMS_MAX_LENGTH_MAILBOX_PORTION error");
          }
        }
      }
      return totalRecipients;
    },

    








    checkMaxValuesParameters: function checkMaxValuesParameters(msg) {
      let subject = msg.headers["subject"];
      if (subject && subject.length > MMS.MMS_MAX_LENGTH_SUBJECT) {
        return false;
      }

      let totalRecipients = 0;
      try {
        totalRecipients += this.countRecipients(msg.headers["to"]);
        totalRecipients += this.countRecipients(msg.headers["cc"]);
        totalRecipients += this.countRecipients(msg.headers["bcc"]);
      } catch (ex) {
        if (DEBUG) debug("Exception caught : " + ex);
        return false;
      }

      if (totalRecipients < 1 ||
          totalRecipients > MMS.MMS_MAX_TOTAL_RECIPIENTS) {
        return false;
      }

      if (!Array.isArray(msg.parts)) {
        return true;
      }
      for (let i = 0; i < msg.parts.length; i++) {
        if (msg.parts[i].headers["content-type"] &&
          msg.parts[i].headers["content-type"].params) {
          let name = msg.parts[i].headers["content-type"].params["name"];
          if (name && name.length > MMS.MMS_MAX_LENGTH_NAME_CONTENT_TYPE) {
            return false;
          }
        }
      }
      return true;
    },

    translateHttpStatusToMmsStatus: function translateHttpStatusToMmsStatus(httpStatus, defaultStatus) {
      switch(httpStatus) {
        case _HTTP_STATUS_USER_CANCELLED:
          return _MMS_ERROR_MESSAGE_DELETED;
        case _HTTP_STATUS_RADIO_DISABLED:
          return _MMS_ERROR_RADIO_DISABLED;
        case _HTTP_STATUS_NO_SIM_CARD:
          return _MMS_ERROR_NO_SIM_CARD;
        case HTTP_STATUS_OK:
          return MMS.MMS_PDU_ERROR_OK;
        default:
          return defaultStatus;
      }
    }
  };

  return helper;
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






function CancellableTransaction(cancellableId) {
  this.cancellableId = cancellableId;
  this.isCancelled = false;
}
CancellableTransaction.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  
  timer: null,

  
  
  runCallback: null,

  isObserversAdded: false,

  registerRunCallback: function registerRunCallback(callback) {
    if (!this.isObserversAdded) {
      Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
      Services.obs.addObserver(this, kMobileMessageDeletedObserverTopic, false);
      this.isObserversAdded = true;
    }

    this.runCallback = callback;
    this.isCancelled = false;
  },

  removeObservers: function removeObservers() {
    if (this.isObserversAdded) {
      Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
      Services.obs.removeObserver(this, kMobileMessageDeletedObserverTopic);
      this.isObserversAdded = false;
    }
  },

  runCallbackIfValid: function runCallbackIfValid(mmsStatus, msg) {
    this.removeObservers();

    if (this.runCallback) {
      this.runCallback(mmsStatus, msg);
      this.runCallback = null;
    }
  },

  
  
  cancellable: null,

  cancelRunning: function cancelRunning() {
    this.isCancelled = true;

    if (this.timer) {
      
      
      this.timer.cancel();
      this.timer = null;
      this.runCallbackIfValid(_MMS_ERROR_MESSAGE_DELETED, null);
      return;
    }

    if (this.cancellable) {
      
      
      this.cancellable.cancel();
      this.cancellable = null;
    }
  },

  

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case kXpcomShutdownObserverTopic: {
        this.cancelRunning();
        break;
      }
      case kMobileMessageDeletedObserverTopic: {
        data = JSON.parse(data);
        if (data.id != this.cancellableId) {
          return;
        }

        this.cancelRunning();
        break;
      }
    }
  }
};







function RetrieveTransaction(cancellableId, contentLocation) {
  
  CancellableTransaction.call(this, cancellableId);

  this.contentLocation = contentLocation;
}
RetrieveTransaction.prototype = Object.create(CancellableTransaction.prototype, {
  




  run: {
    value: function run(callback) {
      this.registerRunCallback(callback);

      this.retryCount = 0;
      let retryCallback = (function (mmsStatus, msg) {
        if (MMS.MMS_PDU_STATUS_DEFERRED == mmsStatus &&
            this.retryCount < PREF_RETRIEVAL_RETRY_COUNT) {
          let time = PREF_RETRIEVAL_RETRY_INTERVALS[this.retryCount];
          if (DEBUG) debug("Fail to retrieve. Will retry after: " + time);

          if (this.timer == null) {
            this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
          }

          this.timer.initWithCallback(this.retrieve.bind(this, retryCallback),
                                      time, Ci.nsITimer.TYPE_ONE_SHOT);
          this.retryCount++;
          return;
        }
        this.runCallbackIfValid(mmsStatus, msg);
      }).bind(this);

      this.retrieve(retryCallback);
    },
    enumerable: true,
    configurable: true,
    writable: true
  },

  




  retrieve: {
    value: function retrieve(callback) {
      this.timer = null;

      this.cancellable =
        gMmsTransactionHelper.sendRequest("GET", this.contentLocation, null,
                                          (function (httpStatus, data) {
        let mmsStatus = gMmsTransactionHelper
                        .translateHttpStatusToMmsStatus(httpStatus,
                                                        MMS.MMS_PDU_STATUS_DEFERRED);
        if (mmsStatus != MMS.MMS_PDU_ERROR_OK) {
           callback(mmsStatus, null);
           return;
        }
        if (!data) {
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
          callback(MMS.translatePduErrorToStatus(retrieveStatus), retrieved);
          return;
        }

        callback(MMS.MMS_PDU_STATUS_RETRIEVED, retrieved);
      }).bind(this));
    },
    enumerable: true,
    configurable: true,
    writable: true
  }
});






function SendTransaction(cancellableId, msg, requestDeliveryReport) {
  
  CancellableTransaction.call(this, cancellableId);

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
  msg.headers["x-mms-delivery-report"] = requestDeliveryReport;

  if (!gMmsTransactionHelper.checkMaxValuesParameters(msg)) {
    
    if (DEBUG) debug("Check max values parameters fail.");
    throw new Error("Check max values parameters fail.");
  }

  if (msg.parts) {
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

  if (DEBUG) debug("msg: " + JSON.stringify(msg));

  this.msg = msg;
}
SendTransaction.prototype = Object.create(CancellableTransaction.prototype, {
  istreamComposed: {
    value: false,
    enumerable: true,
    configurable: true,
    writable: true
  },

  





  loadBlobs: {
    value: function loadBlobs(parts, callback) {
      let callbackIfValid = function callbackIfValid() {
        if (DEBUG) debug("All parts loaded: " + JSON.stringify(parts));
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
    enumerable: true,
    configurable: true,
    writable: true
  },

  




  run: {
    value: function run(callback) {
      this.registerRunCallback(callback);

      if (!this.istreamComposed) {
        this.loadBlobs(this.msg.parts, (function () {
          this.istream = MMS.PduHelper.compose(null, this.msg);
          this.istreamComposed = true;
          if (this.isCancelled) {
            this.runCallbackIfValid(_MMS_ERROR_MESSAGE_DELETED, null);
          } else {
            this.run(callback);
          }
        }).bind(this));
        return;
      }

      if (!this.istream) {
        this.runCallbackIfValid(MMS.MMS_PDU_ERROR_PERMANENT_FAILURE, null);
        return;
      }

      this.retryCount = 0;
      let retryCallback = (function (mmsStatus, msg) {
        if ((MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE == mmsStatus ||
              MMS.MMS_PDU_ERROR_PERMANENT_FAILURE == mmsStatus) &&
            this.retryCount < PREF_SEND_RETRY_COUNT) {
          if (DEBUG) {
            debug("Fail to send. Will retry after: " + PREF_SEND_RETRY_INTERVAL);
          }

          if (this.timer == null) {
            this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
          }

          this.retryCount++;

          this.timer.initWithCallback(this.send.bind(this, retryCallback),
                                      PREF_SEND_RETRY_INTERVAL,
                                      Ci.nsITimer.TYPE_ONE_SHOT);
          return;
        }

        this.runCallbackIfValid(mmsStatus, msg);
      }).bind(this);

      
      this.send(retryCallback);
    },
    enumerable: true,
    configurable: true,
    writable: true
  },

  




  send: {
    value: function send(callback) {
      this.timer = null;

      this.cancellable =
        gMmsTransactionHelper.sendRequest("POST", gMmsConnection.mmsc,
                                          this.istream,
                                          function (httpStatus, data) {
        let mmsStatus = gMmsTransactionHelper.
                          translateHttpStatusToMmsStatus(httpStatus,
                            MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE);
        if (httpStatus != HTTP_STATUS_OK) {
          callback(mmsStatus, null);
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
    },
    enumerable: true,
    configurable: true,
    writable: true
  }
});











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
  if (DEBUG) {
    let macro = (MMS.MMS_VERSION >> 4) & 0x0f;
    let minor = MMS.MMS_VERSION & 0x0f;
    debug("Running protocol version: " + macro + "." + minor);
  }

  
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

  getMsisdn: function getMsisdn() {
    let iccInfo = gRadioInterface.rilContext.iccInfo;
    let number = iccInfo ? iccInfo.msisdn : null;

    
    
    if (number === undefined || number === "undefined") {
      return null;
    }
    return number;
  },

  







  convertIntermediateToSavable: function convertIntermediateToSavable(intermediate,
                                                                      retrievalMode) {
    intermediate.type = "mms";
    intermediate.delivery = DELIVERY_NOT_DOWNLOADED;

    switch(retrievalMode) {
      case RETRIEVAL_MODE_MANUAL:
        intermediate.deliveryStatus = [DELIVERY_STATUS_MANUAL];
        break;
      case RETRIEVAL_MODE_NEVER:
        intermediate.deliveryStatus = [DELIVERY_STATUS_REJECTED];
        break;
      case RETRIEVAL_MODE_AUTOMATIC:
        intermediate.deliveryStatus = [DELIVERY_STATUS_PENDING];
        break;
      case RETRIEVAL_MODE_AUTOMATIC_HOME:
        if (gMmsConnection.isVoiceRoaming()) {
          intermediate.deliveryStatus = [DELIVERY_STATUS_MANUAL];
        } else {
          intermediate.deliveryStatus = [DELIVERY_STATUS_PENDING];
        }
        break;
    }

    intermediate.timestamp = Date.now();
    intermediate.sender = null;
    intermediate.transactionId = intermediate.headers["x-mms-transaction-id"];
    if (intermediate.headers.from) {
      intermediate.sender = intermediate.headers.from.address;
    } else {
      intermediate.sender = "anonymous";
    }
    intermediate.receivers = [];
    intermediate.msisdn = this.getMsisdn();
    return intermediate;
  },

  









  mergeRetrievalConfirmation: function mergeRetrievalConfirmation(intermediate, savable) {
    savable.timestamp = Date.now();
    if (intermediate.headers.from) {
      savable.sender = intermediate.headers.from.address;
    } else {
      savable.sender = "anonymous";
    }
    savable.receivers = [];
    
    for each (let type in ["cc", "to"]) {
      if (intermediate.headers[type]) {
        if (intermediate.headers[type] instanceof Array) {
          for (let index in intermediate.headers[type]) {
            savable.receivers.push(intermediate.headers[type][index].address)
          }
        } else {
          savable.receivers.push(intermediate.headers[type].address);
        }
      }
    }

    savable.delivery = DELIVERY_RECEIVED;
    savable.deliveryStatus = [DELIVERY_STATUS_SUCCESS];
    for (let field in intermediate.headers) {
      savable.headers[field] = intermediate.headers[field];
    }
    if (intermediate.parts) {
      savable.parts = intermediate.parts;
    }
    if (intermediate.content) {
      savable.content = intermediate.content;
    }
    return savable;
  },

  








  retrieveMessage: function retrieveMessage(aContentLocation, aCallback, aDomMessage) {
    
    Services.obs.notifyObservers(aDomMessage, kSmsRetrievingObserverTopic, null);

    let transaction = new RetrieveTransaction(aDomMessage.id, aContentLocation);
    transaction.run(aCallback);
  },

  








  broadcastMmsSystemMessage: function broadcastMmsSystemMessage(aName, aDomMessage) {
    if (DEBUG) debug("Broadcasting the MMS system message: " + aName);

    
    
    
    gSystemMessenger.broadcastMessage(aName, {
      type:           aDomMessage.type,
      id:             aDomMessage.id,
      threadId:       aDomMessage.threadId,
      delivery:       aDomMessage.delivery,
      deliveryStatus: aDomMessage.deliveryStatus,
      sender:         aDomMessage.sender,
      receivers:      aDomMessage.receivers,
      timestamp:      aDomMessage.timestamp,
      read:           aDomMessage.read,
      subject:        aDomMessage.subject,
      smil:           aDomMessage.smil,
      attachments:    aDomMessage.attachments,
      expiryDate:     aDomMessage.expiryDate
    });
  },

  






  broadcastSentMessageEvent: function broadcastSentMessageEvent(aDomMessage) {
    
    this.broadcastMmsSystemMessage("sms-sent", aDomMessage);

    
    Services.obs.notifyObservers(aDomMessage, kSmsSentObserverTopic, null);
  },

  






  broadcastReceivedMessageEvent :function broadcastReceivedMessageEvent(aDomMessage) {
    
    this.broadcastMmsSystemMessage("sms-received", aDomMessage);

    
    Services.obs.notifyObservers(aDomMessage, kSmsReceivedObserverTopic, null);
  },

  


  retrieveMessageCallback: function retrieveMessageCallback(wish,
                                                            savableMessage,
                                                            mmsStatus,
                                                            retrievedMessage) {
    if (DEBUG) debug("retrievedMessage = " + JSON.stringify(retrievedMessage));

    
    
    
    if (wish == null && retrievedMessage) {
      wish = retrievedMessage.headers["x-mms-delivery-report"];
    }

    let reportAllowed = this.getReportAllowed(this.confSendDeliveryReport,
                                              wish);
    
    
    
    
    if (MMS.MMS_PDU_STATUS_RETRIEVED !== mmsStatus) {
      if (mmsStatus != _MMS_ERROR_RADIO_DISABLED &&
          mmsStatus != _MMS_ERROR_NO_SIM_CARD) {
        let transaction = new NotifyResponseTransaction(transactionId,
                                                        mmsStatus,
                                                        reportAllowed);
        transaction.run();
      }
      
      
      gMobileMessageDatabaseService
        .setMessageDeliveryByMessageId(savableMessage.id,
                                       null,
                                       null,
                                       DELIVERY_STATUS_ERROR,
                                       null,
                                       (function (rv, domMessage) {
        this.broadcastReceivedMessageEvent(domMessage);
      }).bind(this));
      return;
    }

    savableMessage = this.mergeRetrievalConfirmation(retrievedMessage,
                                                     savableMessage);
    let transactionId = savableMessage.headers["x-mms-transaction-id"];

    gMobileMessageDatabaseService.saveReceivedMessage(savableMessage,
        (function (rv, domMessage) {
      let success = Components.isSuccessCode(rv);

      
      
      
      
      
      let transaction =
        new NotifyResponseTransaction(transactionId,
                                      success ? MMS.MMS_PDU_STATUS_RETRIEVED
                                              : MMS.MMS_PDU_STATUS_DEFERRED,
                                      reportAllowed);
      transaction.run();

      if (!success) {
        
        
        
        if (DEBUG) debug("Could not store MMS " + domMessage.id +
                         ", error code " + rv);
        return;
      }

      this.broadcastReceivedMessageEvent(domMessage);
    }).bind(this));
  },

  


  saveReceivedMessageCallback: function saveReceivedMessageCallback(retrievalMode,
                                                                    savableMessage,
                                                                    rv,
                                                                    domMessage) {
    let success = Components.isSuccessCode(rv);
    if (!success) {
      
      
      
      if (DEBUG) debug("Could not store MMS " + JSON.stringify(savableMessage) +
            ", error code " + rv);
      
      
      
      return;
    }

    
    let wish = savableMessage.headers["x-mms-delivery-report"];
    let transactionId = savableMessage.headers["x-mms-transaction-id"];

    this.broadcastReceivedMessageEvent(domMessage);

    
    
    if ((retrievalMode !== RETRIEVAL_MODE_AUTOMATIC) &&
        gMmsConnection.isVoiceRoaming()) {
      return;
    }

    if (RETRIEVAL_MODE_MANUAL === retrievalMode ||
        RETRIEVAL_MODE_NEVER === retrievalMode) {
      let mmsStatus = RETRIEVAL_MODE_NEVER === retrievalMode
                    ? MMS.MMS_PDU_STATUS_REJECTED
                    : MMS.MMS_PDU_STATUS_DEFERRED;

      
      let reportAllowed = this.getReportAllowed(this.confSendDeliveryReport,
                                                wish);

      let transaction = new NotifyResponseTransaction(transactionId,
                                                      mmsStatus,
                                                      reportAllowed);
      transaction.run();
      return;
    }
    let url = savableMessage.headers["x-mms-content-location"].uri;

    
    
    this.retrieveMessage(url,
                         this.retrieveMessageCallback.bind(this,
                                                           wish,
                                                           savableMessage),
                         domMessage);
  },

  





  handleNotificationIndication: function handleNotificationIndication(notification) {
    let transactionId = notification.headers["x-mms-transaction-id"];
    gMobileMessageDatabaseService.getMessageRecordByTransactionId(transactionId,
        (function (aRv, aMessageRecord) {
      if (Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR === aRv
          && aMessageRecord) {
        if (DEBUG) debug("We already got the NotificationIndication with transactionId = "
                         + transactionId + " before.");
        return;
      }

      let retrievalMode = RETRIEVAL_MODE_MANUAL;
      try {
        retrievalMode = Services.prefs.getCharPref(PREF_RETRIEVAL_MODE);
      } catch (e) {}

      let savableMessage = this.convertIntermediateToSavable(notification, retrievalMode);

      gMobileMessageDatabaseService
        .saveReceivedMessage(savableMessage,
                             this.saveReceivedMessageCallback.bind(this,
                                                                   retrievalMode,
                                                                   savableMessage));
    }).bind(this));
  },

  





  handleDeliveryIndication: function handleDeliveryIndication(aMsg) {
    if (DEBUG) {
      debug("handleDeliveryIndication: got delivery report" +
            JSON.stringify(aMsg));
    }

    let headers = aMsg.headers;
    let envelopeId = headers["message-id"];
    let address = headers.to.address;
    let mmsStatus = headers["x-mms-status"];
    if (DEBUG) {
      debug("Start updating the delivery status for envelopeId: " + envelopeId +
            " address: " + address + " mmsStatus: " + mmsStatus);
    }

    
    
    
    
    let deliveryStatus;
    switch (mmsStatus) {
      case MMS.MMS_PDU_STATUS_RETRIEVED:
        deliveryStatus = DELIVERY_STATUS_SUCCESS;
        break;
      case MMS.MMS_PDU_STATUS_EXPIRED:
      case MMS.MMS_PDU_STATUS_REJECTED:
      case MMS.MMS_PDU_STATUS_UNRECOGNISED:
      case MMS.MMS_PDU_STATUS_UNREACHABLE:
        deliveryStatus = DELIVERY_STATUS_REJECTED;
        break;
      case MMS.MMS_PDU_STATUS_DEFERRED:
        deliveryStatus = DELIVERY_STATUS_PENDING;
        break;
      case MMS.MMS_PDU_STATUS_INDETERMINATE:
        deliveryStatus = DELIVERY_STATUS_NOT_APPLICABLE;
        break;
      default:
        if (DEBUG) debug("Cannot handle this MMS status. Returning.");
        return;
    }

    if (DEBUG) debug("Updating the delivery status to: " + deliveryStatus);
    gMobileMessageDatabaseService
      .setMessageDeliveryByEnvelopeId(envelopeId,
                                      address,
                                      null,
                                      deliveryStatus,
                                      function notifySetDeliveryResult(aRv, aDomMessage) {
      if (DEBUG) debug("Marking the delivery status is done.");
      

      let topic;
      if (mmsStatus === MMS.MMS_PDU_STATUS_RETRIEVED) {
        topic = kSmsDeliverySuccessObserverTopic;
      } else if (mmsStatus === MMS.MMS_PDU_STATUS_REJECTED) {
        topic = kSmsDeliveryErrorObserverTopic;
      }
      if (!topic) {
        if (DEBUG) debug("Needn't fire event for this MMS status. Returning.");
        return;
      }

      
      Services.obs.notifyObservers(aDomMessage, topic, null);
    });
  },

  




















  createSavableFromParams: function createSavableFromParams(aParams, aMessage) {
    if (DEBUG) debug("createSavableFromParams: aParams: " + JSON.stringify(aParams));

    let isAddrValid = true;
    let smil = aParams.smil;

    
    let headers = aMessage["headers"] = {};
    let receivers = aParams.receivers;
    if (receivers.length != 0) {
      let headersTo = headers["to"] = [];
      for (let i = 0; i < receivers.length; i++) {
        let receiver = receivers[i];
        let type = MMS.Address.resolveType(receiver);
        let address;
        if (type == "PLMN") {
          address = PhoneNumberUtils.normalize(receiver, false);
          if (!PhoneNumberUtils.isPlainPhoneNumber(address)) {
            isAddrValid = false;
          }
          if (DEBUG) debug("createSavableFromParams: normalize phone number " +
                           "from " + receiver + " to " + address);
        } else {
          address = receiver;
          isAddrValid = false;
          if (DEBUG) debug("Error! Address is invalid to send MMS: " + address);
        }
        headersTo.push({"address": address, "type": type});
      }
    }
    if (aParams.subject) {
      headers["subject"] = aParams.subject;
    }

    
    let attachments = aParams.attachments;
    if (attachments.length != 0 || smil) {
      let parts = aMessage["parts"] = [];

      
      if (smil) {
        let part = {
          "headers": {
            "content-type": {
              "media": "application/smil",
              "params": {
                "name": "smil.xml",
                "charset": {
                  "charset": "utf-8"
                }
              }
            },
            "content-location": "smil.xml",
            "content-id": "<smil>"
          },
          "content": smil
        };
        parts.push(part);
      }

      
      for (let i = 0; i < attachments.length; i++) {
        let attachment = attachments[i];
        let content = attachment.content;
        let location = attachment.location;

        let params = {
          "name": location
        };

        if (content.type && content.type.indexOf("text/") == 0) {
          params.charset = {
            "charset": "utf-8"
          };
        }

        let part = {
          "headers": {
            "content-type": {
              "media": content.type,
              "params": params
            },
            "content-location": location,
            "content-id": attachment.id
          },
          "content": content
        };
        parts.push(part);
      }
    }

    
    aMessage["type"] = "mms";
    aMessage["timestamp"] = Date.now();
    aMessage["receivers"] = receivers;
    aMessage["sender"] = this.getMsisdn();
    try {
      aMessage["deliveryStatusRequested"] =
        Services.prefs.getBoolPref("dom.mms.requestStatusReport");
    } catch (e) {
      aMessage["deliveryStatusRequested"] = false;
    }

    if (DEBUG) debug("createSavableFromParams: aMessage: " +
                     JSON.stringify(aMessage));

    return isAddrValid ? Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR
                       : Ci.nsIMobileMessageCallback.INVALID_ADDRESS_ERROR;
  },

  

  send: function send(aParams, aRequest) {
    if (DEBUG) debug("send: aParams: " + JSON.stringify(aParams));

    
    

    
    if (aParams == null || typeof aParams != "object") {
      if (DEBUG) debug("Error! 'aParams' should be a non-null object.");
      throw Cr.NS_ERROR_INVALID_ARG;
      return;
    }

    
    if (!Array.isArray(aParams.receivers)) {
      if (DEBUG) debug("Error! 'receivers' should be an array.");
      throw Cr.NS_ERROR_INVALID_ARG;
      return;
    }

    
    if (aParams.subject != null && typeof aParams.subject != "string") {
      if (DEBUG) debug("Error! 'subject' should be a string if passed.");
      throw Cr.NS_ERROR_INVALID_ARG;
      return;
    }

    
    if (aParams.smil != null && typeof aParams.smil != "string") {
      if (DEBUG) debug("Error! 'smil' should be a string if passed.");
      throw Cr.NS_ERROR_INVALID_ARG;
      return;
    }

    
    if (!Array.isArray(aParams.attachments)) {
      if (DEBUG) debug("Error! 'attachments' should be an array.");
      throw Cr.NS_ERROR_INVALID_ARG;
      return;
    }

    let self = this;

    let sendTransactionCb = function sendTransactionCb(aDomMessage,
                                                       aErrorCode,
                                                       aEnvelopeId) {
      if (DEBUG) {
        debug("The returned status of sending transaction: " +
              "aErrorCode: " + aErrorCode + " aEnvelopeId: " + aEnvelopeId);
      }

      
      
      if (aErrorCode == Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR) {
        aRequest.notifySendMessageFailed(aErrorCode);
        Services.obs.notifyObservers(aDomMessage, kSmsFailedObserverTopic, null);
        return;
      }

      let isSentSuccess = (aErrorCode == Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR);
      gMobileMessageDatabaseService
        .setMessageDeliveryByMessageId(aDomMessage.id,
                                       null,
                                       isSentSuccess ? DELIVERY_SENT : DELIVERY_ERROR,
                                       isSentSuccess ? null : DELIVERY_STATUS_ERROR,
                                       aEnvelopeId,
                                       function notifySetDeliveryResult(aRv, aDomMessage) {
        if (DEBUG) debug("Marking the delivery state/staus is done. Notify sent or failed.");
        
        if (!isSentSuccess) {
          if (DEBUG) debug("Sending MMS failed.");
          aRequest.notifySendMessageFailed(aErrorCode);
          Services.obs.notifyObservers(aDomMessage, kSmsFailedObserverTopic, null);
          return;
        }

        if (DEBUG) debug("Sending MMS succeeded.");

        
        self.broadcastSentMessageEvent(aDomMessage);

        
        aRequest.notifyMessageSent(aDomMessage);
      });
    };

    let savableMessage = {};
    let errorCode = this.createSavableFromParams(aParams, savableMessage);
    gMobileMessageDatabaseService
      .saveSendingMessage(savableMessage,
                          function notifySendingResult(aRv, aDomMessage) {
      if (DEBUG) debug("Saving sending message is done. Start to send.");

      
      Services.obs.notifyObservers(aDomMessage, kSmsSendingObserverTopic, null);

      if (errorCode !== Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR) {
        if (DEBUG) debug("Error! The params for sending MMS are invalid.");
        sendTransactionCb(aDomMessage, errorCode);
        return;
      }

      
      let sendTransaction;
      try {
        sendTransaction =
          new SendTransaction(aDomMessage.id, savableMessage,
                              savableMessage["deliveryStatusRequested"]);
      } catch (e) {
        if (DEBUG) debug("Exception: fail to create a SendTransaction instance.");
        sendTransactionCb(aDomMessage,
                          Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      sendTransaction.run(function callback(aMmsStatus, aMsg) {
        if (DEBUG) debug("The sending status of sendTransaction.run(): " + aMmsStatus);
        let errorCode;
        if (aMmsStatus == _MMS_ERROR_MESSAGE_DELETED) {
          errorCode = Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR;
        } else if (aMmsStatus == _MMS_ERROR_RADIO_DISABLED) {
          errorCode = Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR;
        } else if (aMmsStatus == _MMS_ERROR_NO_SIM_CARD) {
          errorCode = Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
        } else if (aMmsStatus != MMS.MMS_PDU_ERROR_OK) {
          errorCode = Ci.nsIMobileMessageCallback.INTERNAL_ERROR;
        } else {
          errorCode = Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR;
        }
        let envelopeId = null;
        if (aMsg) {
          envelopeId = aMsg.headers ? aMsg.headers["message-id"] : null;
        }
        sendTransactionCb(aDomMessage, errorCode, envelopeId);
      });
    });
  },

  retrieve: function retrieve(aMessageId, aRequest) {
    if (DEBUG) debug("Retrieving message with ID " + aMessageId);
    gMobileMessageDatabaseService.getMessageRecordById(aMessageId,
        (function notifyResult(aRv, aMessageRecord, aDomMessage) {
      if (Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR != aRv) {
        if (DEBUG) debug("Function getMessageRecordById() return error.");
        aRequest.notifyGetMessageFailed(aRv);
        return;
      }
      if ("mms" != aMessageRecord.type) {
        if (DEBUG) debug("Type of message record is not 'mms'.");
        aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      if (!aMessageRecord.headers) {
        if (DEBUG) debug("Must need the MMS' headers to proceed the retrieve.");
        aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      if (!aMessageRecord.headers["x-mms-content-location"]) {
        if (DEBUG) debug("Can't find mms content url in database.");
        aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      if (DELIVERY_NOT_DOWNLOADED != aMessageRecord.delivery) {
        if (DEBUG) debug("Delivery of message record is not 'not-downloaded'.");
        aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }
      if (DELIVERY_STATUS_PENDING == aMessageRecord.deliveryStatus) {
        if (DEBUG) debug("Delivery status of message record is 'pending'.");
        aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
        return;
      }

      
      
      
      if (aMessageRecord.headers["x-mms-expiry"] != undefined) {
        let expiryDate = aMessageRecord.timestamp +
                         aMessageRecord.headers["x-mms-expiry"] * 1000;
        if (expiryDate < Date.now()) {
          if (DEBUG) debug("The message to be retrieved is expired.");
          aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR);
          return;
        }
      }

      let url =  aMessageRecord.headers["x-mms-content-location"].uri;
      
      let wish = aMessageRecord.headers["x-mms-delivery-report"];
      let responseNotify = function responseNotify(mmsStatus, retrievedMsg) {
        
        
        if (mmsStatus == _MMS_ERROR_MESSAGE_DELETED) {
          aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR);
          return;
        }

        
        
        
        if (MMS.MMS_PDU_STATUS_RETRIEVED !== mmsStatus) {
          if (DEBUG) debug("RetrieveMessage fail after retry.");
          let errorCode = Ci.nsIMobileMessageCallback.INTERNAL_ERROR;
          if (mmsStatus == _MMS_ERROR_RADIO_DISABLED) {
            errorCode = Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR;
          } else if (mmsStatus == _MMS_ERROR_NO_SIM_CARD) {
            errorCode = Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
          }
          gMobileMessageDatabaseService
            .setMessageDeliveryByMessageId(aMessageId,
                                           null,
                                           null,
                                           DELIVERY_STATUS_ERROR,
                                           null,
                                           function () {
            aRequest.notifyGetMessageFailed(errorCode);
          });
          return;
        }
        
        
        
        
        
        
        let transactionId = retrievedMsg.headers["x-mms-transaction-id"];

        
        
        
        if (wish == null && retrievedMsg) {
          wish = retrievedMsg.headers["x-mms-delivery-report"];
        }
        let reportAllowed = this.getReportAllowed(this.confSendDeliveryReport,
                                                  wish);

        if (DEBUG) debug("retrievedMsg = " + JSON.stringify(retrievedMsg));
        aMessageRecord = this.mergeRetrievalConfirmation(retrievedMsg, aMessageRecord);
        gMobileMessageDatabaseService.saveReceivedMessage(aMessageRecord,
                                                          (function (rv, domMessage) {
          let success = Components.isSuccessCode(rv);
          if (!success) {
            
            
            
            if (DEBUG) debug("Could not store MMS " + domMessage.id +
                  ", error code " + rv);
            aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
            return;
          }

          
          this.broadcastReceivedMessageEvent(domMessage);

          
          aRequest.notifyMessageGot(domMessage);

          
          
          
          
          
          let transaction = new AcknowledgeTransaction(transactionId, reportAllowed);
          transaction.run();
        }).bind(this));
      };
      
      gMobileMessageDatabaseService
        .setMessageDeliveryByMessageId(aMessageId,
                                       null,
                                       null,
                                       DELIVERY_STATUS_PENDING,
                                       null,
                                       (function (rv) {
          let success = Components.isSuccessCode(rv);
          if (!success) {
            if (DEBUG) debug("Could not change the delivery status: MMS " +
                             domMessage.id + ", error code " + rv);
            aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.INTERNAL_ERROR);
            return;
          }

          this.retrieveMessage(url,
                               responseNotify.bind(this),
                               aDomMessage);
        }).bind(this));
    }).bind(this));
  },

  

  receiveWapPush: function receiveWapPush(array, length, offset, options) {
    let data = {array: array, offset: offset};
    let msg = MMS.PduHelper.parse(data, null);
    if (!msg) {
      return false;
    }
    if (DEBUG) debug("receiveWapPush: msg = " + JSON.stringify(msg));

    switch (msg.type) {
      case MMS.MMS_PDU_TYPE_NOTIFICATION_IND:
        this.handleNotificationIndication(msg);
        break;
      case MMS.MMS_PDU_TYPE_DELIVERY_IND:
        this.handleDeliveryIndication(msg);
        break;
      default:
        if (DEBUG) debug("Unsupported X-MMS-Message-Type: " + msg.type);
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
