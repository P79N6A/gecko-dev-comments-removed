



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

const RIL_MMSSERVICE_CONTRACTID = "@mozilla.org/mms/rilmmsservice;1";
const RIL_MMSSERVICE_CID = Components.ID("{217ddd76-75db-4210-955d-8806cd8d87f9}");

const DEBUG = false;

const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kXpcomShutdownObserverTopic        = "xpcom-shutdown";


const FILE_OPEN_MODE = FileUtils.MODE_CREATE
                     | FileUtils.MODE_WRONLY
                     | FileUtils.MODE_TRUNCATE;


const STORAGE_STREAM_SEGMENT_SIZE = 4096;



const HTTP_STATUS_OK = 200;

const CONFIG_SEND_REPORT_NEVER       = 0;
const CONFIG_SEND_REPORT_DEFAULT_NO  = 1;
const CONFIG_SEND_REPORT_DEFAULT_YES = 2;
const CONFIG_SEND_REPORT_ALWAYS      = 3;

XPCOMUtils.defineLazyServiceGetter(this, "gpps",
                                   "@mozilla.org/network/protocol-proxy-service;1",
                                   "nsIProtocolProxyService");

XPCOMUtils.defineLazyGetter(this, "MMS", function () {
  let MMS = {};
  Cu.import("resource://gre/modules/MmsPduHelper.jsm", MMS);
  return MMS;
});




function MmsService() {
  Services.obs.addObserver(this, kXpcomShutdownObserverTopic, false);
  Services.obs.addObserver(this, kNetworkInterfaceStateChangedTopic, false);
}
MmsService.prototype = {

  classID:   RIL_MMSSERVICE_CID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMmsService,
                                         Ci.nsIWapPushApplication,
                                         Ci.nsIObserver,
                                         Ci.nsIProtocolProxyFilter]),

  



  confSendDeliveryReport: CONFIG_SEND_REPORT_DEFAULT_YES,

  proxyInfo: null,
  MMSC: null,

  
  proxyFilterRefCount: 0,

  







  getReportAllowed: function getReportAllowed(config, wish) {
    if ((config == CONFIG_SEND_REPORT_DEFAULT_NO)
        || (config == CONFIG_SEND_REPORT_DEFAULT_YES)) {
      if (wish != null) {
        config += (wish ? 1 : -1);
      }
    }
    return config >= CONFIG_SEND_REPORT_DEFAULT_YES;
  },

  


  acquireProxyFilter: function acquireProxyFilter() {
    if (!this.proxyFilterRefCount) {
      debug("Register proxy filter");
      gpps.registerFilter(this, 0);
    }
    this.proxyFilterRefCount++;
  },

  


  releaseProxyFilter: function releaseProxyFilter() {
    this.proxyFilterRefCount--;
    if (this.proxyFilterRefCount <= 0) {
      this.proxyFilterRefCount = 0;

      debug("Unregister proxy filter");
      gpps.unregisterFilter(this);
    }
  },

  












  sendMmsRequest: function sendMmsRequest(method, url, istream, callback) {
    let that = this;
    function releaseProxyFilterAndCallback(status, data) {
      
      that.releaseProxyFilter(false);
      if (callback) {
        callback(status, data);
      }
    }

    this.acquireProxyFilter();

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

      
      xhr.onerror = function () {
        debug("xhr error, response headers: " + xhr.getAllResponseHeaders());
        releaseProxyFilterAndCallback(xhr.status, null);
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

        releaseProxyFilterAndCallback(xhr.status, data);
      }

      
      xhr.send(istream);
    } catch (e) {
      debug("xhr error, can't send: " + e.message);
      releaseProxyFilterAndCallback(0, null);
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
    this.sendMmsRequest("POST", this.MMSC, istream);
  },

  










  saveContentToFile: function saveContentToFile(file, data, callback) {
    
    let sstream = Cc["@mozilla.org/storagestream;1"]
                  .createInstance(Ci.nsIStorageStream);
    sstream.init(STORAGE_STREAM_SEGMENT_SIZE, data.length, null);
    let bostream = Cc["@mozilla.org/binaryoutputstream;1"]
                   .createInstance(Ci.nsIBinaryOutputStream);
    bostream.setOutputStream(sstream.getOutputStream(0));
    bostream.writeByteArray(data, data.length);
    bostream.close();

    
    let ofstream = FileUtils.openSafeFileOutputStream(file, FILE_OPEN_MODE);
    return NetUtil.asyncCopy(sstream.newInputStream(0), ofstream, callback);
  },

  





  saveMessageContent: function saveMessageContent(msg, callback) {
    function saveCallback(obj, counter, status) {
      obj.saved = Components.isSuccessCode(status);
      debug("saveMessageContent: " + obj.file.path + ", saved: " + obj.saved);

      
      
      counter.count++;
      if (counter.count >= counter.max) {
        if (callback) {
          callback(msg);
        }
      }
    }

    let tid = msg.headers["x-mms-transaction-id"];
    if (msg.parts) {
      let counter = {max: msg.parts.length, count: 0};

      msg.parts.forEach((function (part, index) {
        part.file = FileUtils.getFile("ProfD", ["mms", tid, index], true);
        if (!part.content) {
          saveCallback(part, counter, Cr.NS_ERROR_NOT_AVAILABLE);
        } else {
          this.saveContentToFile(part.file, part.content,
                                 saveCallback.bind(null, part, counter));
        }
      }).bind(this));
    } else if (msg.content) {
      msg.file = FileUtils.getFile("ProfD", ["mms", tid, "content"], true);
      this.saveContentToFile(msg.file, msg.content,
                             saveCallback.bind(null, msg, {max: 1, count: 0}));
    } else {
      
      if (callback) {
        callback(msg);
      }
    }
  },

  








  parseStreamAndDispatch: function parseStreamAndDispatch(data, options) {
    let msg = MMS.PduHelper.parse(data, null);
    if (!msg) {
      return false;
    }
    debug("parseStreamAndDispatch: msg = " + JSON.stringify(msg));

    switch (msg.type) {
      case MMS.MMS_PDU_TYPE_NOTIFICATION_IND:
        this.handleNotificationIndication(msg, options);
        break;
      case MMS.MMS_PDU_TYPE_RETRIEVE_CONF:
        this.handleRetrieveConfirmation(msg, options);
        break;
      default:
        debug("Unsupported X-MMS-Message-Type: " + msg.type);
        return false;
    }

    return true;
  },

  





  handleNotificationIndication: function handleNotificationIndication(msg) {
    function callback(status, retr) {
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
      if (!data) {
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

    this.saveMessageContent(msg, callbackIfValid.bind(null, MMS.MMS_PDU_ERROR_OK));
  },

  





  updateProxyInfo: function updateProxyInfo(enabled) {
    try {
      if (enabled) {
        this.MMSC = Services.prefs.getCharPref("ril.data.mmsc");
        this.proxyInfo = gpps.newProxyInfo("http",
                                           Services.prefs.getCharPref("ril.data.mmsproxy"),
                                           Services.prefs.getIntPref("ril.data.mmsport"),
                                           Ci.nsIProxyInfo.TRANSPARENT_PROXY_RESOLVES_HOST,
                                           -1, null);
        debug("updateProxyInfo: "
              + JSON.stringify({MMSC: this.MMSC, proxyInfo: this.proxyInfo}));
        return;
      }
    } catch (e) {
      
    }

    this.MMSC = null;
    this.proxyInfo = null;
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
        let iface = subject.QueryInterface(Ci.nsINetworkInterface);
        if ((iface.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE)
            || (iface.type == Ci.nsINetworkInterface.NETWORK_TYPE_MOBILE_MMS)) {
          this.updateProxyInfo(iface.state == Ci.nsINetworkInterface.NETWORK_STATE_CONNECTED);
        }
        break;
      }
      case kXpcomShutdownObserverTopic: {
        Services.obs.removeObserver(this, kXpcomShutdownObserverTopic);
        Services.obs.removeObserver(this, kNetworkInterfaceStateChangedTopic);
        break;
      }
    }
  },

  

  applyFilter: function applyFilter(service, uri, proxyInfo) {
    if (uri.prePath == this.MMSC) {
      debug("applyFilter: match " + uri.spec);
      return this.proxyInfo;
    }

    return proxyInfo;
  },
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([MmsService]);

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-@- MmsService: " + s + "\n");
  };
} else {
  debug = function (s) {};
}

