



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const RIL_MMSSERVICE_CONTRACTID = "@mozilla.org/mms/rilmmsservice;1";
const RIL_MMSSERVICE_CID = Components.ID("{217ddd76-75db-4210-955d-8806cd8d87f9}");

const DEBUG = false;

const kNetworkInterfaceStateChangedTopic = "network-interface-state-changed";
const kXpcomShutdownObserverTopic        = "xpcom-shutdown";



const HTTP_STATUS_OK = 200;

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

  proxyInfo: null,
  MMSC: null,

  
  proxyFilterRefCount: 0,

  


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

  










  sendMmsRequest: function sendMmsRequest(method, url, callback) {
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
      xhr.setRequestHeader("Content-Length", 0);

      
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

      
      xhr.send();
    } catch (e) {
      debug("xhr error, can't send: " + e.message);
      releaseProxyFilterAndCallback(0, null);
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
      
    }

    function retrCallback(error, retr) {
      callback(MMS.translatePduErrorToStatus(error), retr);
    }

    let url = msg.headers["x-mms-content-location"].uri;
    this.sendMmsRequest("GET", url, (function (status, data) {
      if (!data) {
        callback.call(null, MMS.MMS_PDU_STATUS_DEFERRED, null);
      } else if (!this.parseStreamAndDispatch(data, retrCallback)) {
        callback.call(null, MMS.MMS_PDU_STATUS_UNRECOGNISED, null);
      }
    }).bind(this));
  },

  







  handleRetrieveConfirmation: function handleRetrieveConfirmation(msg, callback) {
    function callbackIfValid(status, msg) {
      if (callback) {
        callback(status, msg)
      }
    }

    let status = msg.headers["x-mms-retrieve-status"];
    if ((status != null) && (status != MMS.MMS_PDU_ERROR_OK)) {
      callbackIfValid(status, msg);
      return;
    }

    

    callbackIfValid(MMS.MMS_PDU_ERROR_OK, msg);
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

