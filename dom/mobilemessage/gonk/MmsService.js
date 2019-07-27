





"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

const GONK_MMSSERVICE_CONTRACTID = "@mozilla.org/mms/gonkmmsservice;1";
const GONK_MMSSERVICE_CID = Components.ID("{9b069b8c-8697-11e4-a406-474f5190272b}");

let DEBUG = false;
function debug(s) {
  dump("-@- MmsService: " + s + "\n");
};

const kSmsSendingObserverTopic           = "sms-sending";
const kSmsSentObserverTopic              = "sms-sent";
const kSmsFailedObserverTopic            = "sms-failed";
const kSmsReceivedObserverTopic          = "sms-received";
const kSmsRetrievingObserverTopic        = "sms-retrieving";
const kSmsDeliverySuccessObserverTopic   = "sms-delivery-success";
const kSmsDeliveryErrorObserverTopic     = "sms-delivery-error";
const kSmsReadSuccessObserverTopic       = "sms-read-success";
const kSmsReadErrorObserverTopic         = "sms-read-error";
const kSmsDeletedObserverTopic           = "sms-deleted";

const NS_XPCOM_SHUTDOWN_OBSERVER_ID      = "xpcom-shutdown";
const kNetworkConnStateChangedTopic      = "network-connection-state-changed";

const kPrefRilRadioDisabled              = "ril.radio.disabled";
const kPrefMmsDebuggingEnabled           = "mms.debugging.enabled";



const HTTP_STATUS_OK = 200;


const _HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS  =  0;
const _HTTP_STATUS_USER_CANCELLED              = -1;
const _HTTP_STATUS_RADIO_DISABLED              = -2;
const _HTTP_STATUS_NO_SIM_CARD                 = -3;
const _HTTP_STATUS_ACQUIRE_TIMEOUT             = -4;
const _HTTP_STATUS_FAILED_TO_ROUTE             = -5;


const _MMS_ERROR_MESSAGE_DELETED               = -1;
const _MMS_ERROR_RADIO_DISABLED                = -2;
const _MMS_ERROR_NO_SIM_CARD                   = -3;
const _MMS_ERROR_SIM_CARD_CHANGED              = -4;
const _MMS_ERROR_SHUTDOWN                      = -5;
const _MMS_ERROR_USER_CANCELLED_NO_REASON      = -6;
const _MMS_ERROR_SIM_NOT_MATCHED               = -7;
const _MMS_ERROR_FAILED_TO_ROUTE               = -8;

const CONFIG_SEND_REPORT_NEVER       = 0;
const CONFIG_SEND_REPORT_DEFAULT_NO  = 1;
const CONFIG_SEND_REPORT_DEFAULT_YES = 2;
const CONFIG_SEND_REPORT_ALWAYS      = 3;

const NS_PREFBRANCH_PREFCHANGE_TOPIC_ID = "nsPref:changed";

const TIME_TO_BUFFER_MMS_REQUESTS    = 30000;
const PREF_TIME_TO_RELEASE_MMS_CONNECTION =
  Services.prefs.getIntPref("network.gonk.ms-release-mms-connection");

const kPrefRetrievalMode       = 'dom.mms.retrieval_mode';
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

const PREF_SEND_RETRY_INTERVAL = (function () {
  let intervals =
    Services.prefs.getCharPref("dom.mms.sendRetryInterval").split(",");
  for (let i = 0; i < PREF_SEND_RETRY_COUNT; ++i) {
    intervals[i] = parseInt(intervals[i], 10);
    
    
    if (!intervals[i]) {
      intervals[i] = 60000;
    }
  }
  intervals.length = PREF_SEND_RETRY_COUNT;
  return intervals;
})();

const PREF_RETRIEVAL_RETRY_COUNT =
  Services.prefs.getIntPref("dom.mms.retrievalRetryCount");

const PREF_RETRIEVAL_RETRY_INTERVALS = (function() {
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

const kPrefRilNumRadioInterfaces = "ril.numRadioInterfaces";
const kPrefDefaultServiceId = "dom.mms.defaultServiceId";

XPCOMUtils.defineLazyServiceGetter(this, "gpps",
                                   "@mozilla.org/network/protocol-proxy-service;1",
                                   "nsIProtocolProxyService");

XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/iccservice;1",
                                   "nsIIccService");

XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageDatabaseService",
                                   "@mozilla.org/mobilemessage/gonkmobilemessagedatabaseservice;1",
                                   "nsIGonkMobileMessageDatabaseService");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileMessageService",
                                   "@mozilla.org/mobilemessage/mobilemessageservice;1",
                                   "nsIMobileMessageService");

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");

XPCOMUtils.defineLazyServiceGetter(this, "gRil",
                                   "@mozilla.org/ril;1",
                                   "nsIRadioInterfaceLayer");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkManager",
                                   "@mozilla.org/network/manager;1",
                                   "nsINetworkManager");

XPCOMUtils.defineLazyServiceGetter(this, "gMobileConnectionService",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");

XPCOMUtils.defineLazyServiceGetter(this, "gNetworkService",
                                   "@mozilla.org/network/service;1",
                                   "nsINetworkService");

XPCOMUtils.defineLazyGetter(this, "MMS", function() {
  let MMS = {};
  Cu.import("resource://gre/modules/MmsPduHelper.jsm", MMS);
  return MMS;
});






function getDefaultServiceId() {
  let id = Services.prefs.getIntPref(kPrefDefaultServiceId);
  let numRil = Services.prefs.getIntPref(kPrefRilNumRadioInterfaces);

  if (id >= numRil || id < 0) {
    id = 0;
  }

  return id;
}




function getRadioDisabledState() {
  let state;
  try {
    state = Services.prefs.getBoolPref(kPrefRilRadioDisabled);
  } catch (e) {
    if (DEBUG) debug("Getting preference 'ril.radio.disabled' fails.");
    state = false;
  }

  return state;
}




function MmsConnection(aServiceId) {
  this.serviceId = aServiceId;
  this.radioInterface = gRil.getRadioInterface(aServiceId);
  this.pendingCallbacks = [];
  this.hostsToRoute = [];
  this.connectTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this.disconnectTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
};

MmsConnection.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  
  mmsc:     "",
  mmsProxy: "",
  mmsPort:  -1,

  setApnSetting: function(network) {
    this.mmsc = network.mmsc;
    this.mmsProxy = network.mmsProxy;
    this.mmsPort = network.mmsPort;
  },

  get proxyInfo() {
    if (!this.mmsProxy) {
      if (DEBUG) debug("getProxyInfo: MMS proxy is not available.");
      return null;
    }

    let port = this.mmsPort;

    if (port <= 0) {
      port = 80;
      if (DEBUG) debug("getProxyInfo: port is not valid. Set to defult (80).");
    }

    let proxyInfo =
      gpps.newProxyInfo("http", this.mmsProxy, port,
                        Ci.nsIProxyInfo.TRANSPARENT_PROXY_RESOLVES_HOST,
                        -1, null);
    if (DEBUG) debug("getProxyInfo: " + JSON.stringify(proxyInfo));

    return proxyInfo;
  },

  connected: false,

  
  
  
  pendingCallbacks: null,

  
  refCount: 0,

  
  hostsToRoute: null,

  
  networkInterface: null,

  connectTimer: null,

  disconnectTimer: null,

  


  flushPendingCallbacks: function(status) {
    if (DEBUG) debug("flushPendingCallbacks: " + this.pendingCallbacks.length
                     + " pending callbacks with status: " + status);
    while (this.pendingCallbacks.length) {
      let callback = this.pendingCallbacks.shift();
      let connected = (status == _HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS);
      callback(connected, status);
    }
  },

  


  onDisconnectTimerTimeout: function() {
    if (DEBUG) debug("onDisconnectTimerTimeout: deactivate the MMS data call.");

    if (!this.connected) {
      return;
    }

    let deactivateMmsDataCall = (aError) => {
      if (aError) debug("Failed to removeHostRoute: " + aError);

      
      this.hostsToRoute = [];
      this.networkInterface = null;

      this.radioInterface.deactivateDataCallByType(Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS);
    };

    let promises =
      this.hostsToRoute.map((aHost) => {
        return gNetworkManager.removeHostRoute(this.networkInterface, aHost);
      });

    return Promise.all(promises)
      .then(() => deactivateMmsDataCall(),
            (aError) => deactivateMmsDataCall(aError));
  },

  init: function() {
    Services.obs.addObserver(this, kNetworkConnStateChangedTopic,
                             false);
    Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
  },

  




  isVoiceRoaming: function() {
    let connection =
      gMobileConnectionService.getItemByServiceId(this.serviceId);
    let isRoaming = connection && connection.voice && connection.voice.roaming;
    if (DEBUG) debug("isVoiceRoaming = " + isRoaming);
    return isRoaming;
  },

  








  getPhoneNumber: function() {
    let number;
    
    try {
      let iccInfo = null;
      let baseIccInfo = this.getIccInfo();
      if (baseIccInfo.iccType === 'ruim' || baseIccInfo.iccType === 'csim') {
        iccInfo = baseIccInfo.QueryInterface(Ci.nsICdmaIccInfo);
        number = iccInfo.mdn;
      } else {
        iccInfo = baseIccInfo.QueryInterface(Ci.nsIGsmIccInfo);
        number = iccInfo.msisdn;
      }
    } catch (e) {
      if (DEBUG) {
       debug("Exception - QueryInterface failed on iccinfo for GSM/CDMA info");
      }
      return null;
    }

    return number;
  },

  


  getIccInfo: function() {
    let icc = gIccService.getIccByServiceId(this.serviceId);
    return icc ? icc.iccInfo : null;
  },

  


  getCardState: function() {
    let icc = gIccService.getIccByServiceId(this.serviceId);
    return icc ? icc.cardState : Ci.nsIIcc.CARD_STATE_UNKNOWN;
  },

  


  getIccId: function() {
    let iccInfo = this.getIccInfo();

    if (!iccInfo) {
      return null;
    }

    return iccInfo.iccid;
  },

  











  acquire: function(callback) {
    this.refCount++;
    this.connectTimer.cancel();
    this.disconnectTimer.cancel();

    
    
    if (!this.connected) {
      this.pendingCallbacks.push(callback);

      let errorStatus;
      if (getRadioDisabledState()) {
        if (DEBUG) debug("Error! Radio is disabled when sending MMS.");
        errorStatus = _HTTP_STATUS_RADIO_DISABLED;
      } else if (this.getCardState() != Ci.nsIIcc.CARD_STATE_READY) {
        if (DEBUG) debug("Error! SIM card is not ready when sending MMS.");
        errorStatus = _HTTP_STATUS_NO_SIM_CARD;
      }
      if (errorStatus != null) {
        this.flushPendingCallbacks(errorStatus);
        return true;
      }

      
      
      this.connectTimer.
        initWithCallback(() => this.flushPendingCallbacks(_HTTP_STATUS_ACQUIRE_TIMEOUT),
                         TIME_TO_BUFFER_MMS_REQUESTS,
                         Ci.nsITimer.TYPE_ONE_SHOT);

      
      if (DEBUG) debug("acquire: buffer the MMS request and setup the MMS data call.");
      this.radioInterface.setupDataCallByType(Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS);

      return false;
    }

    callback(true, _HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS);
    return true;
  },

  


  release: function() {
    this.refCount--;
    if (this.refCount <= 0) {
      this.refCount = 0;

      
      if (PREF_TIME_TO_RELEASE_MMS_CONNECTION < 1000) {
        this.onDisconnectTimerTimeout();
        return;
      }

      
      
      this.disconnectTimer.
        initWithCallback(() => this.onDisconnectTimerTimeout(),
                         PREF_TIME_TO_RELEASE_MMS_CONNECTION,
                         Ci.nsITimer.TYPE_ONE_SHOT);
    }
  },

  







  ensureRouting: function(url) {
    let host = this.mmsProxy;

    if (!this.mmsProxy) {
      host = url;
    }

    try {
      let uri = Services.io.newURI(host, null, null);
      host = uri.host;
    } catch (e) {}

    return gNetworkManager.addHostRoute(this.networkInterface, host)
      .then(() => {
        if (this.hostsToRoute.indexOf(host) < 0) {
          this.hostsToRoute.push(host);
        }
      });
  },

  shutdown: function() {
    Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    Services.obs.removeObserver(this, kNetworkConnStateChangedTopic);

    this.connectTimer.cancel();
    this.flushPendingCallbacks(_HTTP_STATUS_RADIO_DISABLED);
    this.disconnectTimer.cancel();
    this.onDisconnectTimerTimeout();
  },

  

  observe: function(subject, topic, data) {
    switch (topic) {
      case kNetworkConnStateChangedTopic: {
        
        if (!(subject instanceof Ci.nsIRilNetworkInterface)) {
          return;
        }

        
        let network = subject.QueryInterface(Ci.nsIRilNetworkInterface);
        if (network.serviceId != this.serviceId ||
            network.type != Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS) {
          return;
        }

        let connected =
          network.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED;

        
        
        if (connected == this.connected) {
          return;
        }

        this.connected = connected;
        if (!this.connected) {
          this.hostsToRoute = [];
          this.networkInterface = null;
          return;
        }

        
        
        this.setApnSetting(network);

        
        this.networkInterface = network;

        if (DEBUG) debug("Got the MMS network connected! Resend the buffered " +
                         "MMS requests: number: " + this.pendingCallbacks.length);
        this.connectTimer.cancel();
        this.flushPendingCallbacks(_HTTP_STATUS_ACQUIRE_CONNECTION_SUCCESS);
        break;
      }
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID: {
        this.shutdown();
      }
    }
  }
};

XPCOMUtils.defineLazyGetter(this, "gMmsConnections", function() {
  return {
    _connections: null,
    getConnByServiceId: function(id) {
      if (!this._connections) {
        this._connections = [];
      }

      let conn = this._connections[id];
      if (conn) {
        return conn;
      }

      conn = this._connections[id] = new MmsConnection(id);
      conn.init();
      return conn;
    },
    getConnByIccId: function(aIccId) {
      if (!aIccId) {
        
        
        
        
        
        
        return this.getConnByServiceId(0);
      }

      let numCardAbsent = 0;
      let numRadioInterfaces = gRil.numRadioInterfaces;
      for (let clientId = 0; clientId < numRadioInterfaces; clientId++) {
        let mmsConnection = this.getConnByServiceId(clientId);
        let iccId = mmsConnection.getIccId();
        if (iccId === null) {
          numCardAbsent++;
          continue;
        }

        if (iccId === aIccId) {
          return mmsConnection;
        }
      }

      throw ((numCardAbsent === numRadioInterfaces)?
               _MMS_ERROR_NO_SIM_CARD: _MMS_ERROR_SIM_NOT_MATCHED);
    },
  };
});




function MmsProxyFilter(mmsConnection, url) {
  this.mmsConnection = mmsConnection;
  this.uri = Services.io.newURI(url, null, null);
}
MmsProxyFilter.prototype = {

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolProxyFilter]),

  

  applyFilter: function(proxyService, uri, proxyInfo) {
    if (!this.uri.equals(uri)) {
      if (DEBUG) debug("applyFilter: content uri = " + JSON.stringify(this.uri) +
                       " is not matched with uri = " + JSON.stringify(uri) + " .");
      return proxyInfo;
    }

    
    let mmsProxyInfo = this.mmsConnection.proxyInfo;

    if (DEBUG) {
      debug("applyFilter: MMSC/Content Location is matched with: " +
            JSON.stringify({ uri: JSON.stringify(this.uri),
                             mmsProxyInfo: mmsProxyInfo }));
    }

    return mmsProxyInfo ? mmsProxyInfo : proxyInfo;
  }
};

XPCOMUtils.defineLazyGetter(this, "gMmsTransactionHelper", function() {
  let helper = {
    














    sendRequest: function(mmsConnection, method, url, istream, callback) {
      
      let cancellable = {
        callback: callback,

        isDone: false,
        isCancelled: false,

        cancel: function() {
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

        done: function(httpStatus, data) {
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
        !mmsConnection.acquire((connected, errorCode) => {

        cancellable.isAcquiringConn = false;

        if (!connected || cancellable.isCancelled) {
          mmsConnection.release();

          if (!cancellable.isDone) {
            cancellable.done(cancellable.isCancelled ?
                             _HTTP_STATUS_USER_CANCELLED : errorCode, null);
          }
          return;
        }

        
        if (!url) {
          url = mmsConnection.mmsc;
        }

        let startTransaction = netId => {
          if (DEBUG) debug("sendRequest: register proxy filter to " + url);
          let proxyFilter = new MmsProxyFilter(mmsConnection, url);
          gpps.registerFilter(proxyFilter, 0);

          cancellable.xhr =
            this.sendHttpRequest(mmsConnection, method,
                                 url, istream, proxyFilter, netId,
                                 (aHttpStatus, aData) =>
                                   cancellable.done(aHttpStatus, aData));
        };

        let onRejected = aReason => {
          debug(aReason);
          mmsConnection.release();
          cancellable.done(_HTTP_STATUS_FAILED_TO_ROUTE, null);
        };

        
        
        mmsConnection.ensureRouting(url)
          .then(() => gNetworkService.getNetId(mmsConnection.networkInterface.name),
                (aReason) => onRejected('Failed to ensureRouting: ' + aReason))
          .then((netId) => startTransaction(netId),
                (aReason) => onRejected('Failed to getNetId: ' + aReason));
      });

      return cancellable;
    },

    sendHttpRequest: function(mmsConnection, method, url, istream, proxyFilter,
                              netId, callback) {
      let releaseMmsConnectionAndCallback = (httpStatus, data) => {
        gpps.unregisterFilter(proxyFilter);
        
        mmsConnection.release();
        callback(httpStatus, data);
      };

      try {
        let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);

        
        xhr.networkInterfaceId = netId;
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

        
        xhr.onreadystatechange = () => {
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

    







    countRecipients: function(recipients) {
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

    








    checkMaxValuesParameters: function(msg) {
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

    translateHttpStatusToMmsStatus: function(httpStatus, cancelledReason,
                                             defaultStatus) {
      switch(httpStatus) {
        case _HTTP_STATUS_USER_CANCELLED:
          return cancelledReason;
        case _HTTP_STATUS_RADIO_DISABLED:
          return _MMS_ERROR_RADIO_DISABLED;
        case _HTTP_STATUS_NO_SIM_CARD:
          return _MMS_ERROR_NO_SIM_CARD;
        case _HTTP_STATUS_FAILED_TO_ROUTE:
          return _MMS_ERROR_FAILED_TO_ROUTE;
        case HTTP_STATUS_OK:
          return MMS.MMS_PDU_ERROR_OK;
        default:
          return defaultStatus;
      }
    }
  };

  return helper;
});















function NotifyResponseTransaction(mmsConnection, transactionId, status,
                                   reportAllowed) {
  this.mmsConnection = mmsConnection;
  let headers = {};

  
  headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_NOTIFYRESP_IND;
  headers["x-mms-transaction-id"] = transactionId;
  headers["x-mms-mms-version"] = MMS.MMS_VERSION;
  headers["x-mms-status"] = status;
  
  headers["x-mms-report-allowed"] = reportAllowed;

  this.istream = MMS.PduHelper.compose(null, {headers: headers});
}
NotifyResponseTransaction.prototype = {
  



  run: function(callback) {
    let requestCallback;
    if (callback) {
      requestCallback = (httpStatus, data) => {
        
        
        
        callback(httpStatus);
      };
    }
    gMmsTransactionHelper.sendRequest(this.mmsConnection,
                                      "POST",
                                      null,
                                      this.istream,
                                      requestCallback);
  }
};









function CancellableTransaction(cancellableId, serviceId) {
  this.cancellableId = cancellableId;
  this.serviceId = serviceId;
  this.isCancelled = false;
}
CancellableTransaction.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  
  timer: null,

  
  
  runCallback: null,

  isObserversAdded: false,

  cancelledReason: _MMS_ERROR_USER_CANCELLED_NO_REASON,

  registerRunCallback: function(callback) {
    if (!this.isObserversAdded) {
      Services.obs.addObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, false);
      Services.obs.addObserver(this, kSmsDeletedObserverTopic, false);
      Services.prefs.addObserver(kPrefRilRadioDisabled, this, false);
      Services.prefs.addObserver(kPrefDefaultServiceId, this, false);
      this.isObserversAdded = true;
    }

    this.runCallback = callback;
    this.isCancelled = false;
  },

  removeObservers: function() {
    if (this.isObserversAdded) {
      Services.obs.removeObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
      Services.obs.removeObserver(this, kSmsDeletedObserverTopic);
      Services.prefs.removeObserver(kPrefRilRadioDisabled, this);
      Services.prefs.removeObserver(kPrefDefaultServiceId, this);
      this.isObserversAdded = false;
    }
  },

  runCallbackIfValid: function(mmsStatus, msg) {
    this.removeObservers();

    if (this.runCallback) {
      this.runCallback(mmsStatus, msg);
      this.runCallback = null;
    }
  },

  
  
  cancellable: null,

  cancelRunning: function(reason) {
    this.isCancelled = true;
    this.cancelledReason = reason;

    if (this.timer) {
      
      
      this.timer.cancel();
      this.timer = null;
      this.runCallbackIfValid(reason, null);
      return;
    }

    if (this.cancellable) {
      
      
      this.cancellable.cancel();
      this.cancellable = null;
    }
  },

  

  observe: function(subject, topic, data) {
    switch (topic) {
      case NS_XPCOM_SHUTDOWN_OBSERVER_ID: {
        this.cancelRunning(_MMS_ERROR_SHUTDOWN);
        break;
      }
      case kSmsDeletedObserverTopic: {
        if (subject && subject.deletedMessageIds &&
            subject.deletedMessageIds.indexOf(this.cancellableId) >= 0) {
          this.cancelRunning(_MMS_ERROR_MESSAGE_DELETED);
        }
        break;
      }
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID: {
        if (data == kPrefRilRadioDisabled) {
          if (getRadioDisabledState()) {
            this.cancelRunning(_MMS_ERROR_RADIO_DISABLED);
          }
        } else if (data === kPrefDefaultServiceId &&
                   this.serviceId != getDefaultServiceId()) {
          this.cancelRunning(_MMS_ERROR_SIM_CARD_CHANGED);
        }
        break;
      }
    }
  }
};







function RetrieveTransaction(mmsConnection, cancellableId, contentLocation) {
  this.mmsConnection = mmsConnection;

  
  CancellableTransaction.call(this, cancellableId, mmsConnection.serviceId);

  this.contentLocation = contentLocation;
}
RetrieveTransaction.prototype = Object.create(CancellableTransaction.prototype, {
  




  run: {
    value: function(callback) {
      this.registerRunCallback(callback);

      this.retryCount = 0;
      let retryCallback = (mmsStatus, msg) => {
        if (MMS.MMS_PDU_STATUS_DEFERRED == mmsStatus &&
            this.retryCount < PREF_RETRIEVAL_RETRY_COUNT) {
          let time = PREF_RETRIEVAL_RETRY_INTERVALS[this.retryCount];
          if (DEBUG) debug("Fail to retrieve. Will retry after: " + time);

          if (this.timer == null) {
            this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
          }

          this.timer.initWithCallback(() => this.retrieve(retryCallback),
                                      time, Ci.nsITimer.TYPE_ONE_SHOT);
          this.retryCount++;
          return;
        }
        this.runCallbackIfValid(mmsStatus, msg);
      };

      this.retrieve(retryCallback);
    },
    enumerable: true,
    configurable: true,
    writable: true
  },

  




  retrieve: {
    value: function(callback) {
      this.timer = null;

      this.cancellable =
        gMmsTransactionHelper.sendRequest(this.mmsConnection,
                                          "GET", this.contentLocation, null,
                                          (httpStatus, data) => {
        let mmsStatus = gMmsTransactionHelper
                        .translateHttpStatusToMmsStatus(httpStatus,
                                                        this.cancelledReason,
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
      });
    },
    enumerable: true,
    configurable: true,
    writable: true
  }
});






function SendTransaction(mmsConnection, cancellableId, msg, requestDeliveryReport) {
  this.mmsConnection = mmsConnection;

  
  CancellableTransaction.call(this, cancellableId, mmsConnection.serviceId);

  msg.headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_SEND_REQ;
  if (!msg.headers["x-mms-transaction-id"]) {
    
    let tid = gUUIDGenerator.generateUUID().toString();
    msg.headers["x-mms-transaction-id"] = tid;
  }
  msg.headers["x-mms-mms-version"] = MMS.MMS_VERSION;

  
  
  let phoneNumber = mmsConnection.getPhoneNumber();
  let from = (phoneNumber) ? { address: phoneNumber, type: "PLMN" } : null;
  msg.headers["from"] = from;

  msg.headers["date"] = new Date();
  msg.headers["x-mms-message-class"] = "personal";
  msg.headers["x-mms-expiry"] = 7 * 24 * 60 * 60;
  msg.headers["x-mms-priority"] = 129;
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
    value: function(parts, callback) {
      let callbackIfValid = () => {
        if (DEBUG) debug("All parts loaded: " + JSON.stringify(parts));
        if (callback) {
          callback();
        }
      };

      if (!parts || !parts.length) {
        callbackIfValid();
        return;
      }

      let numPartsToLoad = parts.length;
      parts.forEach((aPart) => {
        if (!(aPart.content instanceof Ci.nsIDOMBlob)) {
          numPartsToLoad--;
          if (!numPartsToLoad) {
            callbackIfValid();
          }
          return;
        }

        let fileReader = Cc["@mozilla.org/files/filereader;1"]
                         .createInstance(Ci.nsIDOMFileReader);
        fileReader.addEventListener("loadend", (aEvent) => {
          let arrayBuffer = aEvent.target.result;
          aPart.content = new Uint8Array(arrayBuffer);
          numPartsToLoad--;
          if (!numPartsToLoad) {
            callbackIfValid();
          }
        });
        fileReader.readAsArrayBuffer(aPart.content);
      });
    },
    enumerable: true,
    configurable: true,
    writable: true
  },

  




  run: {
    value: function(callback) {
      this.registerRunCallback(callback);

      if (!this.istreamComposed) {
        this.loadBlobs(this.msg.parts, () => {
          this.istream = MMS.PduHelper.compose(null, this.msg);
          this.istreamSize = this.istream.available();
          this.istreamComposed = true;
          if (this.isCancelled) {
            this.runCallbackIfValid(_MMS_ERROR_MESSAGE_DELETED, null);
          } else {
            this.run(callback);
          }
        });
        return;
      }

      if (!this.istream) {
        this.runCallbackIfValid(MMS.MMS_PDU_ERROR_PERMANENT_FAILURE, null);
        return;
      }

      this.retryCount = 0;
      let retryCallback = (mmsStatus, msg) => {
        if ((MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE == mmsStatus ||
              MMS.MMS_PDU_ERROR_PERMANENT_FAILURE == mmsStatus) &&
            this.retryCount < PREF_SEND_RETRY_COUNT) {
          if (DEBUG) {
            debug("Fail to send. Will retry after: " + PREF_SEND_RETRY_INTERVAL[this.retryCount]);
          }

          if (this.timer == null) {
            this.timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
          }

          
          
          if (this.istreamSize == null ||
              this.istreamSize != this.istream.available()) {
            this.istream = MMS.PduHelper.compose(null, this.msg);
          }

          this.timer.initWithCallback(() => this.send(retryCallback),
                                      PREF_SEND_RETRY_INTERVAL[this.retryCount],
                                      Ci.nsITimer.TYPE_ONE_SHOT);
          this.retryCount++;
          return;
        }

        this.runCallbackIfValid(mmsStatus, msg);
      };

      
      this.send(retryCallback);
    },
    enumerable: true,
    configurable: true,
    writable: true
  },

  




  send: {
    value: function(callback) {
      this.timer = null;

      this.cancellable =
        gMmsTransactionHelper.sendRequest(this.mmsConnection,
                                          "POST",
                                          null,
                                          this.istream,
                                          (httpStatus, data) => {
        let mmsStatus = gMmsTransactionHelper.
                          translateHttpStatusToMmsStatus(
                            httpStatus,
                            this.cancelledReason,
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
        if (DEBUG) {
          debug("Parsed M-Send.conf: " + JSON.stringify(response));
        }
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













function AcknowledgeTransaction(mmsConnection, transactionId, reportAllowed) {
  this.mmsConnection = mmsConnection;
  let headers = {};

  
  headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_ACKNOWLEDGE_IND;
  headers["x-mms-transaction-id"] = transactionId;
  headers["x-mms-mms-version"] = MMS.MMS_VERSION;
  
  headers["x-mms-report-allowed"] = reportAllowed;

  this.istream = MMS.PduHelper.compose(null, {headers: headers});
}
AcknowledgeTransaction.prototype = {
  



  run: function(callback) {
    let requestCallback;
    if (callback) {
      requestCallback = (httpStatus, data) => {
        
        
        
        callback(httpStatus);
      };
    }
    gMmsTransactionHelper.sendRequest(this.mmsConnection,
                                      "POST",
                                      null,
                                      this.istream,
                                      requestCallback);
  }
};












function ReadRecTransaction(mmsConnection, messageID, toAddress) {
  this.mmsConnection = mmsConnection;
  let headers = {};

  
  headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_READ_REC_IND;
  headers["x-mms-mms-version"] = MMS.MMS_VERSION;
  headers["message-id"] = messageID;
  let type = MMS.Address.resolveType(toAddress);
  let to = {address: toAddress,
            type: type}
  headers["to"] = to;
  
  
  let phoneNumber = mmsConnection.getPhoneNumber();
  let from = (phoneNumber) ? { address: phoneNumber, type: "PLMN" } : null;
  headers["from"] = from;
  headers["x-mms-read-status"] = MMS.MMS_PDU_READ_STATUS_READ;

  this.istream = MMS.PduHelper.compose(null, {headers: headers});
  if (!this.istream) {
    throw Cr.NS_ERROR_FAILURE;
  }
}
ReadRecTransaction.prototype = {
  run: function() {
    gMmsTransactionHelper.sendRequest(this.mmsConnection,
                                      "POST",
                                      null,
                                      this.istream,
                                      null);
  }
};




function MmsService() {
  this._updateDebugFlag();
  if (DEBUG) {
    let macro = (MMS.MMS_VERSION >> 4) & 0x0f;
    let minor = MMS.MMS_VERSION & 0x0f;
    debug("Running protocol version: " + macro + "." + minor);
  }

  Services.prefs.addObserver(kPrefDefaultServiceId, this, false);
  Services.prefs.addObserver(kPrefMmsDebuggingEnabled, this, false);
  this.mmsDefaultServiceId = getDefaultServiceId();

  
}
MmsService.prototype = {

  classID:   GONK_MMSSERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMmsService,
                                         Ci.nsIWapPushApplication,
                                         Ci.nsIObserver]),
  



  confSendDeliveryReport: CONFIG_SEND_REPORT_DEFAULT_YES,

  _updateDebugFlag: function() {
    try {
      DEBUG = Services.prefs.getBoolPref(kPrefMmsDebuggingEnabled);
    } catch (e) {}
  },

  







  getReportAllowed: function(config, wish) {
    if ((config == CONFIG_SEND_REPORT_DEFAULT_NO)
        || (config == CONFIG_SEND_REPORT_DEFAULT_YES)) {
      if (wish != null) {
        config += (wish ? 1 : -1);
      }
    }
    return config >= CONFIG_SEND_REPORT_DEFAULT_YES;
  },

  









  convertIntermediateToSavable: function(mmsConnection, intermediate,
                                         retrievalMode) {
    intermediate.type = "mms";
    intermediate.delivery = DELIVERY_NOT_DOWNLOADED;

    let deliveryStatus;
    switch (retrievalMode) {
      case RETRIEVAL_MODE_MANUAL:
        deliveryStatus = DELIVERY_STATUS_MANUAL;
        break;
      case RETRIEVAL_MODE_NEVER:
        deliveryStatus = DELIVERY_STATUS_REJECTED;
        break;
      case RETRIEVAL_MODE_AUTOMATIC:
        deliveryStatus = DELIVERY_STATUS_PENDING;
        break;
      case RETRIEVAL_MODE_AUTOMATIC_HOME:
        if (mmsConnection.isVoiceRoaming()) {
          deliveryStatus = DELIVERY_STATUS_MANUAL;
        } else {
          deliveryStatus = DELIVERY_STATUS_PENDING;
        }
        break;
      default:
        deliveryStatus = DELIVERY_STATUS_NOT_APPLICABLE;
        break;
    }
    
    intermediate.deliveryStatus = deliveryStatus;

    intermediate.timestamp = Date.now();
    intermediate.receivers = [];
    intermediate.phoneNumber = mmsConnection.getPhoneNumber();
    intermediate.iccId = mmsConnection.getIccId();
    return intermediate;
  },

  











  mergeRetrievalConfirmation: function(mmsConnection, intermediate, savable) {
    
    savable.timestamp = Date.now();
    savable.sentTimestamp = intermediate.headers["date"].getTime();

    savable.receivers = [];
    
    for each (let type in ["cc", "to"]) {
      if (intermediate.headers[type]) {
        if (intermediate.headers[type] instanceof Array) {
          for (let index in intermediate.headers[type]) {
            savable.receivers.push(intermediate.headers[type][index].address);
          }
        } else {
          savable.receivers.push(intermediate.headers[type].address);
        }
      }
    }

    savable.delivery = DELIVERY_RECEIVED;
    
    savable.deliveryStatus = DELIVERY_STATUS_SUCCESS;
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

  










  retrieveMessage: function(aMmsConnection, aContentLocation, aCallback,
                            aDomMessage) {
    
    Services.obs.notifyObservers(aDomMessage, kSmsRetrievingObserverTopic, null);

    let transaction = new RetrieveTransaction(aMmsConnection,
                                              aDomMessage.id,
                                              aContentLocation);
    transaction.run(aCallback);
  },

  








  broadcastMmsSystemMessage: function(aName, aDomMessage) {
    if (DEBUG) debug("Broadcasting the MMS system message: " + aName);

    
    
    
    try {
      gSystemMessenger.broadcastMessage(aName, {
        iccId:               aDomMessage.iccId,
        type:                aDomMessage.type,
        id:                  aDomMessage.id,
        threadId:            aDomMessage.threadId,
        delivery:            aDomMessage.delivery,
        deliveryInfo:        aDomMessage.deliveryInfo,
        sender:              aDomMessage.sender,
        receivers:           aDomMessage.receivers,
        timestamp:           aDomMessage.timestamp,
        sentTimestamp:       aDomMessage.sentTimestamp,
        read:                aDomMessage.read,
        subject:             aDomMessage.subject,
        smil:                aDomMessage.smil,
        attachments:         aDomMessage.attachments,
        expiryDate:          aDomMessage.expiryDate,
        readReportRequested: aDomMessage.readReportRequested
      });
    } catch (e) {
      if (DEBUG) {
        debug("Failed to _broadcastSmsSystemMessage: " + e);
      }
    }
  },

  






  broadcastSentMessageEvent: function(aDomMessage) {
    
    this.broadcastMmsSystemMessage(kSmsSentObserverTopic, aDomMessage);

    
    Services.obs.notifyObservers(aDomMessage, kSmsSentObserverTopic, null);
  },

  






  broadcastSentFailureMessageEvent: function(aDomMessage) {
    
    this.broadcastMmsSystemMessage(kSmsFailedObserverTopic, aDomMessage);

    
    Services.obs.notifyObservers(aDomMessage, kSmsFailedObserverTopic, null);
  },

  






  broadcastReceivedMessageEvent: function(aDomMessage) {
    
    this.broadcastMmsSystemMessage(kSmsReceivedObserverTopic, aDomMessage);

    
    Services.obs.notifyObservers(aDomMessage, kSmsReceivedObserverTopic, null);
  },

  


  retrieveMessageCallback: function(mmsConnection, wish, savableMessage,
                                    mmsStatus, retrievedMessage) {
    if (DEBUG) debug("retrievedMessage = " + JSON.stringify(retrievedMessage));

    let transactionId = savableMessage.headers["x-mms-transaction-id"];

    
    
    
    if (wish == null && retrievedMessage) {
      wish = retrievedMessage.headers["x-mms-delivery-report"];
    }

    let reportAllowed = this.getReportAllowed(this.confSendDeliveryReport,
                                              wish);
    
    
    
    
    if (MMS.MMS_PDU_STATUS_RETRIEVED !== mmsStatus) {
      if (mmsStatus != _MMS_ERROR_RADIO_DISABLED &&
          mmsStatus != _MMS_ERROR_NO_SIM_CARD &&
          mmsStatus != _MMS_ERROR_SIM_CARD_CHANGED) {
        let transaction = new NotifyResponseTransaction(mmsConnection,
                                                        transactionId,
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
                                       (rv, domMessage) =>
                                         this.broadcastReceivedMessageEvent(domMessage));
      return;
    }

    savableMessage = this.mergeRetrievalConfirmation(mmsConnection,
                                                     retrievedMessage,
                                                     savableMessage);
    gMobileMessageDatabaseService.saveReceivedMessage(savableMessage,
                                                      (rv, domMessage) => {
      let success = Components.isSuccessCode(rv);

      
      
      
      
      
      let transaction =
        new NotifyResponseTransaction(mmsConnection,
                                      transactionId,
                                      success ? MMS.MMS_PDU_STATUS_RETRIEVED
                                              : MMS.MMS_PDU_STATUS_DEFERRED,
                                      reportAllowed);
      transaction.run();

      if (!success) {
        
        
        
        if (DEBUG) debug("Could not store MMS , error code " + rv);
        return;
      }

      this.broadcastReceivedMessageEvent(domMessage);
    });
  },

  


  saveReceivedMessageCallback: function(mmsConnection, retrievalMode,
                                        savableMessage, rv, domMessage) {
    let success = Components.isSuccessCode(rv);
    if (!success) {
      
      
      
      if (DEBUG) debug("Could not store MMS " + JSON.stringify(savableMessage) +
            ", error code " + rv);
      
      
      
      return;
    }

    
    let wish = savableMessage.headers["x-mms-delivery-report"];
    let transactionId = savableMessage.headers["x-mms-transaction-id"];

    this.broadcastReceivedMessageEvent(domMessage);

    
    
    if (retrievalMode !== RETRIEVAL_MODE_AUTOMATIC &&
        mmsConnection.isVoiceRoaming()) {
      return;
    }

    if (RETRIEVAL_MODE_MANUAL === retrievalMode ||
        RETRIEVAL_MODE_NEVER === retrievalMode) {
      let mmsStatus = RETRIEVAL_MODE_NEVER === retrievalMode
                    ? MMS.MMS_PDU_STATUS_REJECTED
                    : MMS.MMS_PDU_STATUS_DEFERRED;

      
      let reportAllowed = this.getReportAllowed(this.confSendDeliveryReport,
                                                wish);

      let transaction = new NotifyResponseTransaction(mmsConnection,
                                                      transactionId,
                                                      mmsStatus,
                                                      reportAllowed);
      transaction.run();
      return;
    }

    let url = savableMessage.headers["x-mms-content-location"].uri;

    
    
    this.retrieveMessage(mmsConnection,
                         url,
                         (aMmsStatus, aRetrievedMsg) =>
                           this.retrieveMessageCallback(mmsConnection,
                                                        wish,
                                                        savableMessage,
                                                        aMmsStatus,
                                                        aRetrievedMsg),
                         domMessage);
  },

  







  handleNotificationIndication: function(serviceId, notification) {
    let transactionId = notification.headers["x-mms-transaction-id"];
    gMobileMessageDatabaseService
      .getMessageRecordByTransactionId(transactionId, (aRv, aMessageRecord) => {
      if (Components.isSuccessCode(aRv) && aMessageRecord) {
        if (DEBUG) debug("We already got the NotificationIndication with transactionId = "
                         + transactionId + " before.");
        return;
      }

      let retrievalMode = RETRIEVAL_MODE_MANUAL;
      try {
        retrievalMode = Services.prefs.getCharPref(kPrefRetrievalMode);
      } catch (e) {}

      
      
      if ((retrievalMode == RETRIEVAL_MODE_AUTOMATIC ||
           retrievalMode == RETRIEVAL_MODE_AUTOMATIC_HOME) &&
          serviceId != this.mmsDefaultServiceId) {
        if (DEBUG) {
          debug("Switch to 'manual' mode to download MMS for non-active SIM: " +
                "serviceId = " + serviceId + " doesn't equal to " +
                "mmsDefaultServiceId = " + this.mmsDefaultServiceId);
        }

        retrievalMode = RETRIEVAL_MODE_MANUAL;
      }

      let mmsConnection = gMmsConnections.getConnByServiceId(serviceId);

      let savableMessage = this.convertIntermediateToSavable(mmsConnection,
                                                             notification,
                                                             retrievalMode);

      gMobileMessageDatabaseService
        .saveReceivedMessage(savableMessage,
                             (aRv, aDomMessage) =>
                               this.saveReceivedMessageCallback(mmsConnection,
                                                                retrievalMode,
                                                                savableMessage,
                                                                aRv,
                                                                aDomMessage));
    });
  },

  





  handleDeliveryIndication: function(aMsg) {
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
      .setMessageDeliveryStatusByEnvelopeId(envelopeId, address, deliveryStatus,
                                            (aRv, aDomMessage) => {
      if (DEBUG) debug("Marking the delivery status is done.");
      

      let topic;
      if (mmsStatus === MMS.MMS_PDU_STATUS_RETRIEVED) {
        topic = kSmsDeliverySuccessObserverTopic;
        
        this.broadcastMmsSystemMessage(topic, aDomMessage);
      } else if (mmsStatus === MMS.MMS_PDU_STATUS_REJECTED) {
        topic = kSmsDeliveryErrorObserverTopic;
        
        this.broadcastMmsSystemMessage(topic, aDomMessage);
      } else {
        if (DEBUG) debug("Needn't fire event for this MMS status. Returning.");
        return;
      }

      
      Services.obs.notifyObservers(aDomMessage, topic, null);
    });
  },

  





  handleReadOriginateIndication: function(aIndication) {

    let headers = aIndication.headers;
    let envelopeId = headers["message-id"];
    let address = headers.from.address;
    let mmsReadStatus = headers["x-mms-read-status"];
    if (DEBUG) {
      debug("Start updating the read status for envelopeId: " + envelopeId +
            ", address: " + address + ", mmsReadStatus: " + mmsReadStatus);
    }

    
    
    
    let readStatus = mmsReadStatus == MMS.MMS_PDU_READ_STATUS_READ
                   ? MMS.DOM_READ_STATUS_SUCCESS
                   : MMS.DOM_READ_STATUS_ERROR;
    if (DEBUG) debug("Updating the read status to: " + readStatus);

    gMobileMessageDatabaseService
      .setMessageReadStatusByEnvelopeId(envelopeId, address, readStatus,
                                        (aRv, aDomMessage) => {
      if (!Components.isSuccessCode(aRv)) {
        if (DEBUG) debug("Failed to update read status: " + aRv);
        return;
      }

      if (DEBUG) debug("Marking the read status is done.");
      let topic;
      if (mmsReadStatus == MMS.MMS_PDU_READ_STATUS_READ) {
        topic = kSmsReadSuccessObserverTopic;

        
        this.broadcastMmsSystemMessage(topic, aDomMessage);
      } else {
        topic = kSmsReadErrorObserverTopic;
      }

      
      Services.obs.notifyObservers(aDomMessage, topic, null);
    });
  },

  






















  createSavableFromParams: function(aMmsConnection, aParams, aMessage) {
    if (DEBUG) debug("createSavableFromParams: aParams: " + JSON.stringify(aParams));

    let isAddrValid = true;
    let smil = aParams.smil;

    
    let headers = aMessage["headers"] = {};

    let receivers = aParams.receivers;
    let headersTo = headers["to"] = [];
    if (receivers.length != 0) {
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
          if (type == "Others") {
            isAddrValid = false;
            if (DEBUG) debug("Error! Address is invalid to send MMS: " + address);
          }
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
    aMessage["sender"] = aMmsConnection.getPhoneNumber();
    aMessage["iccId"] = aMmsConnection.getIccId();
    try {
      aMessage["deliveryStatusRequested"] =
        Services.prefs.getBoolPref("dom.mms.requestStatusReport");
    } catch (e) {
      aMessage["deliveryStatusRequested"] = false;
    }
    try {
      headers["x-mms-read-report"] =
        Services.prefs.getBoolPref("dom.mms.requestReadReport");
    } catch (e) {
      headers["x-mms-read-report"] = false;
    }

    if (DEBUG) debug("createSavableFromParams: aMessage: " +
                     JSON.stringify(aMessage));

    return isAddrValid ? Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR
                       : Ci.nsIMobileMessageCallback.INVALID_ADDRESS_ERROR;
  },

  

  mmsDefaultServiceId: 0,

  send: function(aServiceId, aParams, aRequest) {
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

    let sendTransactionCb = (aDomMessage, aErrorCode, aEnvelopeId) => {
      if (DEBUG) {
        debug("The returned status of sending transaction: " +
              "aErrorCode: " + aErrorCode + " aEnvelopeId: " + aEnvelopeId);
      }

      
      
      if (aErrorCode == Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR) {
        aRequest.notifySendMessageFailed(aErrorCode, aDomMessage);
        this.broadcastSentFailureMessageEvent(aDomMessage);
        return;
      }

      let isSentSuccess = (aErrorCode == Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR);
      gMobileMessageDatabaseService
        .setMessageDeliveryByMessageId(aDomMessage.id,
                                       null,
                                       isSentSuccess ? DELIVERY_SENT : DELIVERY_ERROR,
                                       isSentSuccess ? null : DELIVERY_STATUS_ERROR,
                                       aEnvelopeId,
                                       (aRv, aDomMessage) => {
        if (DEBUG) debug("Marking the delivery state/staus is done. Notify sent or failed.");
        
        if (!isSentSuccess) {
          if (DEBUG) debug("Sending MMS failed.");
          aRequest.notifySendMessageFailed(aErrorCode, aDomMessage);
          this.broadcastSentFailureMessageEvent(aDomMessage);
          return;
        }

        if (DEBUG) debug("Sending MMS succeeded.");

        
        this.broadcastSentMessageEvent(aDomMessage);

        
        aRequest.notifyMessageSent(aDomMessage);
      });
    };

    let mmsConnection = gMmsConnections.getConnByServiceId(aServiceId);

    let savableMessage = {};
    let errorCode = this.createSavableFromParams(mmsConnection, aParams,
                                                 savableMessage);
    gMobileMessageDatabaseService
      .saveSendingMessage(savableMessage,
                          (aRv, aDomMessage) => {
      if (!Components.isSuccessCode(aRv)) {
        if (DEBUG) debug("Error! Fail to save sending message! rv = " + aRv);
        aRequest.notifySendMessageFailed(
          gMobileMessageDatabaseService.translateCrErrorToMessageCallbackError(aRv),
          aDomMessage);
        this.broadcastSentFailureMessageEvent(aDomMessage);
        return;
      }

      if (DEBUG) debug("Saving sending message is done. Start to send.");

      Services.obs.notifyObservers(aDomMessage, kSmsSendingObserverTopic, null);

      if (errorCode !== Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR) {
        if (DEBUG) debug("Error! The params for sending MMS are invalid.");
        sendTransactionCb(aDomMessage, errorCode, null);
        return;
      }

      
      if (getRadioDisabledState()) {
        if (DEBUG) debug("Error! Radio is disabled when sending MMS.");
        sendTransactionCb(aDomMessage,
                          Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR,
                          null);
        return;
      }

      
      
      
      if (mmsConnection.serviceId != this.mmsDefaultServiceId) {
        if (DEBUG) debug("RIL service is not active to send MMS.");
        sendTransactionCb(aDomMessage,
                          Ci.nsIMobileMessageCallback.NON_ACTIVE_SIM_CARD_ERROR,
                          null);
        return;
      }

      
      let sendTransaction;
      try {
        sendTransaction =
          new SendTransaction(mmsConnection, aDomMessage.id, savableMessage,
                              savableMessage["deliveryStatusRequested"]);
      } catch (e) {
        if (DEBUG) debug("Exception: fail to create a SendTransaction instance.");
        sendTransactionCb(aDomMessage,
                          Ci.nsIMobileMessageCallback.INTERNAL_ERROR, null);
        return;
      }
      sendTransaction.run((aMmsStatus, aMsg) => {
        if (DEBUG) debug("The sending status of sendTransaction.run(): " + aMmsStatus);
        let errorCode;
        if (aMmsStatus == _MMS_ERROR_MESSAGE_DELETED) {
          errorCode = Ci.nsIMobileMessageCallback.NOT_FOUND_ERROR;
        } else if (aMmsStatus == _MMS_ERROR_RADIO_DISABLED) {
          errorCode = Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR;
        } else if (aMmsStatus == _MMS_ERROR_NO_SIM_CARD) {
          errorCode = Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
        } else if (aMmsStatus == _MMS_ERROR_SIM_CARD_CHANGED) {
          errorCode = Ci.nsIMobileMessageCallback.NON_ACTIVE_SIM_CARD_ERROR;
        } else if (aMmsStatus != MMS.MMS_PDU_ERROR_OK) {
          errorCode = Ci.nsIMobileMessageCallback.INTERNAL_ERROR;
        } else {
          errorCode = Ci.nsIMobileMessageCallback.SUCCESS_NO_ERROR;
        }
        let envelopeId =
          aMsg && aMsg.headers && aMsg.headers["message-id"] || null;
        sendTransactionCb(aDomMessage, errorCode, envelopeId);
      });
    });
  },

  retrieve: function(aMessageId, aRequest) {
    if (DEBUG) debug("Retrieving message with ID " + aMessageId);
    gMobileMessageDatabaseService
      .getMessageRecordById(aMessageId, (aRv, aMessageRecord, aDomMessage) => {
      if (!Components.isSuccessCode(aRv)) {
        if (DEBUG) debug("Function getMessageRecordById() return error: " + aRv);
        aRequest.notifyGetMessageFailed(
          gMobileMessageDatabaseService.translateCrErrorToMessageCallbackError(aRv));
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
      let deliveryStatus = aMessageRecord.deliveryInfo[0].deliveryStatus;
      if (DELIVERY_STATUS_PENDING == deliveryStatus) {
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

      
      
      
      
      
      if (getRadioDisabledState()) {
        if (DEBUG) debug("Error! Radio is disabled when retrieving MMS.");
        aRequest.notifyGetMessageFailed(
          Ci.nsIMobileMessageCallback.RADIO_DISABLED_ERROR);
        return;
      }

      
      
      let mmsConnection;
      try {
        mmsConnection = gMmsConnections.getConnByIccId(aMessageRecord.iccId);
      } catch (e) {
        if (DEBUG) debug("Failed to get connection by IccId. e= " + e);
        let error = (e === _MMS_ERROR_SIM_NOT_MATCHED) ?
                      Ci.nsIMobileMessageCallback.SIM_NOT_MATCHED_ERROR :
                      Ci.nsIMobileMessageCallback.NO_SIM_CARD_ERROR;
        aRequest.notifyGetMessageFailed(error);
        return;
      }

      
      
      
      if (mmsConnection.serviceId != this.mmsDefaultServiceId) {
        if (DEBUG) debug("RIL service is not active to retrieve MMS.");
        aRequest.notifyGetMessageFailed(Ci.nsIMobileMessageCallback.NON_ACTIVE_SIM_CARD_ERROR);
        return;
      }

      let url =  aMessageRecord.headers["x-mms-content-location"].uri;
      
      let wish = aMessageRecord.headers["x-mms-delivery-report"];
      let responseNotify = (mmsStatus, retrievedMsg) => {
        
        
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
          } else if (mmsStatus == _MMS_ERROR_SIM_CARD_CHANGED) {
            errorCode = Ci.nsIMobileMessageCallback.NON_ACTIVE_SIM_CARD_ERROR;
          }
          gMobileMessageDatabaseService
            .setMessageDeliveryByMessageId(aMessageId,
                                           null,
                                           null,
                                           DELIVERY_STATUS_ERROR,
                                           null,
                                           () => aRequest.notifyGetMessageFailed(errorCode));
          return;
        }
        
        
        
        
        
        
        let transactionId = retrievedMsg.headers["x-mms-transaction-id"];

        
        
        
        if (wish == null && retrievedMsg) {
          wish = retrievedMsg.headers["x-mms-delivery-report"];
        }
        let reportAllowed = this.getReportAllowed(this.confSendDeliveryReport,
                                                  wish);

        if (DEBUG) debug("retrievedMsg = " + JSON.stringify(retrievedMsg));
        aMessageRecord = this.mergeRetrievalConfirmation(mmsConnection,
                                                         retrievedMsg,
                                                         aMessageRecord);

        gMobileMessageDatabaseService.saveReceivedMessage(aMessageRecord,
                                                          (rv, domMessage) => {
          let success = Components.isSuccessCode(rv);
          if (!success) {
            
            
            
            if (DEBUG) debug("Could not store MMS, error code " + rv);
            aRequest.notifyGetMessageFailed(
              gMobileMessageDatabaseService.translateCrErrorToMessageCallbackError(rv));
            return;
          }

          
          this.broadcastReceivedMessageEvent(domMessage);

          
          aRequest.notifyMessageGot(domMessage);

          
          
          
          
          
          let transaction = new AcknowledgeTransaction(mmsConnection,
                                                       transactionId,
                                                       reportAllowed);
          transaction.run();
        });
      };

      
      gMobileMessageDatabaseService
        .setMessageDeliveryByMessageId(aMessageId,
                                       null,
                                       null,
                                       DELIVERY_STATUS_PENDING,
                                       null,
                                       (rv) => {
          let success = Components.isSuccessCode(rv);
          if (!success) {
            if (DEBUG) debug("Could not change the delivery status, error code " + rv);
            aRequest.notifyGetMessageFailed(
              gMobileMessageDatabaseService.translateCrErrorToMessageCallbackError(rv));
            return;
          }

          this.retrieveMessage(mmsConnection,
                               url,
                               (aMmsStatus, aRetrievedMsg) =>
                                 responseNotify(aMmsStatus, aRetrievedMsg),
                               aDomMessage);
        });
    });
  },

  sendReadReport: function(messageID, toAddress, iccId) {
    if (DEBUG) {
      debug("messageID: " + messageID + " toAddress: " +
            JSON.stringify(toAddress));
    }

    
    
    let mmsConnection;
    try {
      mmsConnection = gMmsConnections.getConnByIccId(iccId);
    } catch (e) {
      if (DEBUG) debug("Failed to get connection by IccId. e = " + e);
      return;
    }

    try {
      let transaction =
        new ReadRecTransaction(mmsConnection, messageID, toAddress);
      transaction.run();
    } catch (e) {
      if (DEBUG) debug("sendReadReport fail. e = " + e);
    }
  },

  

  receiveWapPush: function(array, length, offset, options) {
    let data = {array: array, offset: offset};
    let msg = MMS.PduHelper.parse(data, null);
    if (!msg) {
      return false;
    }
    if (DEBUG) debug("receiveWapPush: msg = " + JSON.stringify(msg));

    switch (msg.type) {
      case MMS.MMS_PDU_TYPE_NOTIFICATION_IND:
        this.handleNotificationIndication(options.serviceId, msg);
        break;
      case MMS.MMS_PDU_TYPE_DELIVERY_IND:
        this.handleDeliveryIndication(msg);
        break;
      case MMS.MMS_PDU_TYPE_READ_ORIG_IND:
        this.handleReadOriginateIndication(msg);
        break;
      default:
        if (DEBUG) debug("Unsupported X-MMS-Message-Type: " + msg.type);
        break;
    }
  },

  

  observe: function(aSubject, aTopic, aData) {
    switch (aTopic) {
      case NS_PREFBRANCH_PREFCHANGE_TOPIC_ID:
        if (aData === kPrefDefaultServiceId) {
          this.mmsDefaultServiceId = getDefaultServiceId();
        } else if (aData === kPrefMmsDebuggingEnabled) {
          this._updateDebugFlag();
        }
        break;
    }
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MmsService]);
