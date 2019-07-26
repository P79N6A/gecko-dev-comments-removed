



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

Cu.import("resource://gre/modules/WspPduHelper.jsm", this);

const DEBUG = false; 




XPCOMUtils.defineLazyGetter(this, "SI", function () {
  let SI = {};
  Cu.import("resource://gre/modules/SiPduHelper.jsm", SI);
  return SI;
});

XPCOMUtils.defineLazyGetter(this, "SL", function () {
  let SL = {};
  Cu.import("resource://gre/modules/SlPduHelper.jsm", SL);
  return SL;
});

XPCOMUtils.defineLazyGetter(this, "CP", function () {
  let CP = {};
  Cu.import("resource://gre/modules/CpPduHelper.jsm", CP);
  return CP;
});

XPCOMUtils.defineLazyServiceGetter(this, "gSystemMessenger",
                                   "@mozilla.org/system-message-internal;1",
                                   "nsISystemMessagesInternal");




this.WapPushManager = {

  







  processMessage: function processMessage(data, options) {
    try {
      PduHelper.parse(data, true, options);
      debug("options: " + JSON.stringify(options));
    } catch (ex) {
      debug("Failed to parse sessionless WSP PDU: " + ex.message);
      return;
    }

    let appid = options.headers["x-wap-application-id"];
    if (!appid) {
      
      debug("Push message doesn't contains X-Wap-Application-Id.");
    }

    
    if (appid == "x-wap-application:mms.ua") {
      let mmsService = Cc["@mozilla.org/mms/rilmmsservice;1"]
                       .getService(Ci.nsIMmsService);
      mmsService.QueryInterface(Ci.nsIWapPushApplication)
                .receiveWapPush(data.array, data.array.length, data.offset, options);
      return;
    }

    













    let contentType = options.headers["content-type"].media;
    let msg;

    if (contentType === "text/vnd.wap.si" ||
        contentType === "application/vnd.wap.sic") {
      msg = SI.PduHelper.parse(data, contentType);
    } else if (contentType === "text/vnd.wap.sl" ||
               contentType === "application/vnd.wap.slc") {
      msg = SL.PduHelper.parse(data, contentType);
    } else if (contentType === "text/vnd.wap.connectivity-xml" ||
               contentType === "application/vnd.wap.connectivity-wbxml") {
      msg = CP.PduHelper.parse(data, contentType);
    } else {
      
      msg = {
        contentType: contentType,
        content: data.array
      };
    }

    gSystemMessenger.broadcastMessage("wappush-received", {
      contentType:    msg.contentType,
      content:        msg.content
    });
  },

  









  receiveWdpPDU: function receiveWdpPDU(array, length, offset, options) {
    if ((options.bearer == null) || !options.sourceAddress
        || (options.sourcePort == null) || !array) {
      debug("Incomplete WDP PDU");
      return;
    }

    if (options.destinationPort != WDP_PORT_PUSH) {
      debug("Not WAP Push port: " + options.destinationPort);
      return;
    }

    this.processMessage({array: array, offset: offset}, options);
  },
};

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-*- WapPushManager: " + s + "\n");
  };
} else {
  debug = function (s) {};
}

this.EXPORTED_SYMBOLS = ALL_CONST_SYMBOLS.concat([
  "WapPushManager",
]);

