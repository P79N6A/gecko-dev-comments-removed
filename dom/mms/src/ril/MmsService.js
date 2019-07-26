



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

XPCOMUtils.defineLazyServiceGetter(this, "gpps",
                                   "@mozilla.org/network/protocol-proxy-service;1",
                                   "nsIProtocolProxyService");

XPCOMUtils.defineLazyServiceGetter(this, "gUUIDGenerator",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyGetter(this, "MMS", function () {
  let MMS = {};
  Cu.import("resource://gre/modules/MmsPduHelper.jsm", MMS);
  return MMS;
});

XPCOMUtils.defineLazyGetter(this, "gRIL", function () {
  return Cc["@mozilla.org/telephony/system-worker-manager;1"].
           getService(Ci.nsIInterfaceRequestor).
           getInterface(Ci.nsIRadioInterfaceLayer);
});




function MmsService() {
  Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
  Services.obs.addObserver(this, kNetworkInterfaceStateChangedTopic, false);
  this.mmsProxySettings.forEach(function(name) {
    Services.prefs.addObserver(name, this, false);
  }, this);

  try {
    this.mmsc = Services.prefs.getCharPref("ril.mms.mmsc");
    this.mmsProxy = Services.prefs.getCharPref("ril.mms.mmsproxy");
    this.mmsPort = Services.prefs.getIntPref("ril.mms.mmsport");
    this.updateMmsProxyInfo();
  } catch (e) {
    debug("Unable to initialize the MMS proxy settings from the preference. " +
          "This could happen at the first-run. Should be available later.");
    this.clearMmsProxySettings();
  }

  try {
    this.urlUAProf = Services.prefs.getCharPref('wap.UAProf.url');
  } catch (e) {
    this.urlUAProf = "";
  }
  try {
    this.tagnameUAProf = Services.prefs.getCharPref('wap.UAProf.tagname');
  } catch (e) {
    this.tagnameUAProf = "x-wap-profile";
  }
}
MmsService.prototype = {

  classID:   RIL_MMSSERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMmsService,
                                         Ci.nsIWapPushApplication,
                                         Ci.nsIObserver,
                                         Ci.nsIProtocolProxyFilter]),

  



  confSendDeliveryReport: CONFIG_SEND_REPORT_DEFAULT_YES,

  
  mmsc: null,
  mmsProxy: null,
  mmsPort: null,
  mmsProxyInfo: null,
  mmsProxySettings: ["ril.mms.mmsc",
                     "ril.mms.mmsproxy",
                     "ril.mms.mmsport"],
  mmsNetworkConnected: false,

  
  mmsConnRefCount: 0,

  
  urlUAProf: null,
  tagnameUAProf: null,

  
  
  
  mmsRequestQueue: [],
  timerToClearQueue: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),

  timerToReleaseMmsConnection: Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer),
  isProxyFilterRegistered: false,

  







  getReportAllowed: function getReportAllowed(config, wish) {
    if ((config == CONFIG_SEND_REPORT_DEFAULT_NO)
        || (config == CONFIG_SEND_REPORT_DEFAULT_YES)) {
      if (wish != null) {
        config += (wish ? 1 : -1);
      }
    }
    return config >= CONFIG_SEND_REPORT_DEFAULT_YES;
  },

  


  timerToClearQueueCb: function timerToClearQueueCb() {
    debug("timerToClearQueueCb: clear the buffered MMS requests due to " +
          "the timeout or cancel: number: " + this.mmsRequestQueue.length);
    while (this.mmsRequestQueue.length) {
      let mmsRequest = this.mmsRequestQueue.shift();
      if (mmsRequest.callback) {
        mmsRequest.callback(0, null);
      }
    }
  },

  














  acquireMmsConnection: function acquireMmsConnection(method, url, istream, callback) {
    
    
    if (!this.mmsNetworkConnected) {
      debug("acquireMmsConnection: " +
            "buffer the MMS request and setup the MMS data call.");
      this.mmsRequestQueue.push({method: method,
                                 url: url,
                                 istream: istream,
                                 callback: callback});
      gRIL.setupDataCallByType("mms");

      
      
      this.timerToClearQueue.
        initWithCallback(this.timerToClearQueueCb.bind(this),
                         TIME_TO_BUFFER_MMS_REQUESTS,
                         Ci.nsITimer.TYPE_ONE_SHOT);
      return false;
    }

    if (!this.mmsConnRefCount && !this.isProxyFilterRegistered) {
      debug("acquireMmsConnection: register the MMS proxy filter.");
      gpps.registerFilter(this, 0);
      this.isProxyFilterRegistered = true;
    }
    this.mmsConnRefCount++;
    return true;
  },

  


  timerToReleaseMmsConnectionCb: function timerToReleaseMmsConnectionCb() {
    if (this.mmsConnRefCount) {
      return;
    }

    debug("timerToReleaseMmsConnectionCb: " +
          "unregister the MMS proxy filter and deactivate the MMS data call.");
    if (this.isProxyFilterRegistered) {
      gpps.unregisterFilter(this);
      this.isProxyFilterRegistered = false;
    }
    if (this.mmsNetworkConnected) {
      gRIL.deactivateDataCallByType("mms");
    }
  },

  


  releaseMmsConnection: function releaseMmsConnection() {
    this.mmsConnRefCount--;
    if (this.mmsConnRefCount <= 0) {
      this.mmsConnRefCount = 0;

      
      
      this.timerToReleaseMmsConnection.
        initWithCallback(this.timerToReleaseMmsConnectionCb.bind(this),
                         TIME_TO_RELEASE_MMS_CONNECTION,
                         Ci.nsITimer.TYPE_ONE_SHOT);
    }
  },

  












  sendMmsRequest: function sendMmsRequest(method, url, istream, callback) {
    debug("sendMmsRequest: method: " + method + "url: " + url +
          "istream: " + istream + "callback: " + callback);

    if (!this.acquireMmsConnection(method, url, istream, callback)) {
      return;
    }

    let that = this;
    function releaseMmsConnectionAndCallback(status, data) {
      
      that.releaseMmsConnection();
      if (callback) {
        callback(status, data);
      }
    }

    try {
      let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);

      
      xhr.open(method, url, true);
      xhr.responseType = "arraybuffer";
      if (istream) {
        xhr.setRequestHeader("Content-Type", "application/vnd.wap.mms-message");
        xhr.setRequestHeader("Content-Length", istream.available());
      } else {
        xhr.setRequestHeader("Content-Length", 0);
      }

      if(this.urlUAProf !== "") {
        xhr.setRequestHeader(this.tagnameUAProf, this.urlUAProf);
      }

      
      xhr.onerror = function () {
        debug("xhr error, response headers: " + xhr.getAllResponseHeaders());
        releaseMmsConnectionAndCallback(xhr.status, null);
      };
      xhr.onreadystatechange = function () {
        if (xhr.readyState != Ci.nsIXMLHttpRequest.DONE) {
          return;
        }

        let data = null;
        switch (xhr.status) {
          case HTTP_STATUS_OK: {
            debug("xhr success, response headers: " + xhr.getAllResponseHeaders());

            let array = new Uint8Array(xhr.response);
            if (false) {
              for (let begin = 0; begin < array.length; begin += 20) {
                debug("res: " + JSON.stringify(array.subarray(begin, begin + 20)));
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
  },

  











  sendNotificationResponse: function sendNotificationResponse(tid, status, ra) {
    debug("sendNotificationResponse: tid = " + tid + ", status = " + status
          + ", reportAllowed = " + ra);

    let headers = {};

    
    headers["x-mms-message-type"] = MMS.MMS_PDU_TYPE_NOTIFYRESP_IND;
    headers["x-mms-transaction-id"] = tid;
    headers["x-mms-mms-version"] = MMS.MMS_VERSION;
    headers["x-mms-status"] = status;
    
    headers["x-mms-report-allowed"] = ra;

    let istream = MMS.PduHelper.compose(null, {headers: headers});
    this.sendMmsRequest("POST", this.mmsc, istream);
  },

  


  sendSendRequest: function sendSendRequest(msg, callback) {
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
        messageSize += msg.parts[i].content.length;
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

    let istream = MMS.PduHelper.compose(null, msg);
    if (!istream) {
      debug("sendSendRequest: failed to compose M-Send.ind PDU");
      callback(MMS.MMS_PDU_ERROR_PERMANENT_FAILURE, null);
      return;
    }

    this.sendMmsRequest("POST", this.MMSC, istream, (function (status, data) {
      if (status != HTTP_STATUS_OK) {
        callback(MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE, null);
      } else if (!data) {
        callback(MMS.MMS_PDU_ERROR_PERMANENT_FAILURE, null);
      } else if (!this.parseStreamAndDispatch(data, {msg: msg, callback: callback})) {
        callback(MMS.MMS_PDU_RESPONSE_ERROR_UNSUPPORTED_MESSAGE, null);
      }
    }).bind(this));
  },

  








  parseStreamAndDispatch: function parseStreamAndDispatch(data, options) {
    let msg = MMS.PduHelper.parse(data, null);
    if (!msg) {
      return false;
    }
    debug("parseStreamAndDispatch: msg = " + JSON.stringify(msg));

    switch (msg.type) {
      case MMS.MMS_PDU_TYPE_SEND_CONF:
        this.handleSendConfirmation(msg, options);
        break;
      case MMS.MMS_PDU_TYPE_NOTIFICATION_IND:
        this.handleNotificationIndication(msg, options);
        break;
      case MMS.MMS_PDU_TYPE_RETRIEVE_CONF:
        this.handleRetrieveConfirmation(msg, options);
        break;
      case MMS.MMS_PDU_TYPE_DELIVERY_IND:
        this.handleDeliveryIndication(msg, options);
        break;
      default:
        debug("Unsupported X-MMS-Message-Type: " + msg.type);
        return false;
    }

    return true;
  },

  





  handleSendConfirmation: function handleSendConfirmation(msg, options) {
    let status = msg.headers["x-mms-response-status"];
    if (status == null) {
      return;
    }

    if (status == MMS.MMS_PDU_ERROR_OK) {
      
      
      
      let messageId = msg.headers["message-id"];
      options.msg.headers["message-id"] = messageId;
    } else if (this.isTransientError(status)) {
      return;
    }

    if (options.callback) {
      options.callback(status, msg);
    }
  },

  





  handleNotificationIndication: function handleNotificationIndication(msg) {
    function callback(status, retr) {
      if (this.isTransientError(status)) {
        return;
      }

      let tid = msg.headers["x-mms-transaction-id"];

      
      let wish = msg.headers["x-mms-delivery-report"];
      
      
      if ((wish == null) && retr) {
        wish = retr.headers["x-mms-delivery-report"];
      }
      let ra = this.getReportAllowed(this.confSendDeliveryReport, wish);

      this.sendNotificationResponse(tid, status, ra);
    }

    function retrCallback(error, retr) {
      callback.call(this, MMS.translatePduErrorToStatus(error), retr);
    }

    let url = msg.headers["x-mms-content-location"].uri;
    this.sendMmsRequest("GET", url, null, (function (status, data) {
      if (status != HTTP_STATUS_OK) {
        callback.call(this, MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE, null);
      } else if (!data) {
        callback.call(this, MMS.MMS_PDU_STATUS_DEFERRED, null);
      } else if (!this.parseStreamAndDispatch(data, retrCallback.bind(this))) {
        callback.call(this, MMS.MMS_PDU_STATUS_UNRECOGNISED, null);
      }
    }).bind(this));
  },

  







  handleRetrieveConfirmation: function handleRetrieveConfirmation(msg, callback) {
    function callbackIfValid(status, msg) {
      if (callback) {
        callback(status, msg)
      }
    }

    
    if (msg.headers["x-mms-delivery-report"] == null) {
      msg.headers["x-mms-delivery-report"] = false;
    }

    let status = msg.headers["x-mms-retrieve-status"];
    if ((status != null) && (status != MMS.MMS_PDU_ERROR_OK)) {
      callbackIfValid(status, msg);
      return;
    }
    
    
    
  },

  


  handleDeliveryIndication: function handleDeliveryIndication(msg) {
    let messageId = msg.headers["message-id"];
    debug("handleDeliveryIndication: got delivery report for " + messageId);
  },

  


  updateMmsProxyInfo: function updateMmsProxyInfo() {
    if (this.mmsProxy === null || this.mmsPort === null) {
      debug("updateMmsProxyInfo: mmsProxy or mmsPort is not yet decided." );
      return;
    }

    this.mmsProxyInfo =
      gpps.newProxyInfo("http",
                        this.mmsProxy,
                        this.mmsPort,
                        Ci.nsIProxyInfo.TRANSPARENT_PROXY_RESOLVES_HOST,
                        -1, null);
    debug("updateMmsProxyInfo: " + JSON.stringify(this.mmsProxyInfo));
  },

  


  clearMmsProxySettings: function clearMmsProxySettings() {
    this.mmsc = null;
    this.mmsProxy = null;
    this.mmsPort = null;
    this.mmsProxyInfo = null;
  },

  





  isTransientError: function isTransientError(status) {
    return (status >= MMS.MMS_PDU_ERROR_TRANSIENT_FAILURE &&
            status < MMS.MMS_PDU_ERROR_PERMANENT_FAILURE);
  },

  

  hasSupport: function hasSupport() {
    return true;
  },

  

  receiveWapPush: function receiveWapPush(array, length, offset, options) {
    this.parseStreamAndDispatch({array: array, offset: offset});
  },

  

  observe: function observe(subject, topic, data) {
    switch (topic) {
      case kNetworkInterfaceStateChangedTopic: {
        this.mmsNetworkConnected =
          gRIL.getDataCallStateByType("mms") ==
            Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED;

        if (!this.mmsNetworkConnected) {
          return;
        }

        debug("Got the MMS network connected! Resend the buffered " +
              "MMS requests: number: " + this.mmsRequestQueue.length);
        this.timerToClearQueue.cancel();
        while (this.mmsRequestQueue.length) {
          let mmsRequest = this.mmsRequestQueue.shift();
          this.sendMmsRequest(mmsRequest.method,
                              mmsRequest.url,
                              mmsRequest.istream,
                              mmsRequest.callback);
        }
        break;
      }
      case kXpcomShutdownObserverTopic: {
        Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
        Services.obs.removeObserver(this, kNetworkInterfaceStateChangedTopic);
        this.mmsProxySettings.forEach(function(name) {
          Services.prefs.removeObserver(name, this);
        }, this);
        this.timerToClearQueue.cancel();
        this.timerToClearQueueCb();
        this.timerToReleaseMmsConnection.cancel();
        this.timerToReleaseMmsConnectionCb();
        break;
      }
      case kPrefenceChangedObserverTopic: {
        try {
          switch (data) {
            case "ril.mms.mmsc":
              this.mmsc = Services.prefs.getCharPref("ril.mms.mmsc");
              break;
            case "ril.mms.mmsproxy":
              this.mmsProxy = Services.prefs.getCharPref("ril.mms.mmsproxy");
              this.updateMmsProxyInfo();
              break;
            case "ril.mms.mmsport":
              this.mmsPort = Services.prefs.getIntPref("ril.mms.mmsport");
              this.updateMmsProxyInfo();
              break;
            default:
              break;
          }
        } catch (e) {
          debug("Failed to update the MMS proxy settings from the preference.");
          this.clearMmsProxySettings();
        }
        break;
      }
    }
  },

  

  applyFilter: function applyFilter(service, uri, proxyInfo) {
    if (!this.mmsNetworkConnected) {
      debug("applyFilter: the MMS network is not connected.");
      return proxyInfo;
     }

    if (this.mmsc === null || uri.prePath != this.mmsc) {
      debug("applyFilter: MMSC is not matched.");
      return proxyInfo;
    }

    if (this.mmsProxyInfo === null) {
      debug("applyFilter: MMS proxy info is not yet decided.");
      return proxyInfo;
    }

    
    debug("applyFilter: MMSC is matched: " +
          JSON.stringify({ mmsc: this.mmsc,
                           mmsProxyInfo: this.mmsProxyInfo }));
    return this.mmsProxyInfo;
  }
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

