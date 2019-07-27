



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");
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
XPCOMUtils.defineLazyServiceGetter(this, "gIccService",
                                   "@mozilla.org/icc/iccservice;1",
                                   "nsIIccService");




this.WapPushManager = {

  







  processMessage: function processMessage(data, options) {
    try {
      PduHelper.parse(data, true, options);
      debug("options: " + JSON.stringify(options));
    } catch (ex) {
      debug("Failed to parse sessionless WSP PDU: " + ex.message);
      return;
    }

    
    
    

    












    let contentType = options.headers["content-type"].media;
    let msg;
    let authInfo = null;
    if (contentType === "application/vnd.wap.mms-message") {
      let mmsService = Cc["@mozilla.org/mms/gonkmmsservice;1"]
                       .getService(Ci.nsIMmsService);
      mmsService.QueryInterface(Ci.nsIWapPushApplication)
                .receiveWapPush(data.array, data.array.length, data.offset, options);
      return;
    } else if (contentType === "text/vnd.wap.si" ||
        contentType === "application/vnd.wap.sic") {
      msg = SI.PduHelper.parse(data, contentType);
    } else if (contentType === "text/vnd.wap.sl" ||
               contentType === "application/vnd.wap.slc") {
      msg = SL.PduHelper.parse(data, contentType);
    } else if (contentType === "text/vnd.wap.connectivity-xml" ||
               contentType === "application/vnd.wap.connectivity-wbxml") {
      
      if (contentType === "application/vnd.wap.connectivity-wbxml") {
        let params = options.headers["content-type"].params;
        let sec = params && params.sec;
        let mac = params && params.mac;
        authInfo = CP.Authenticator.check(data.array.subarray(data.offset),
                                          sec, mac, function getNetworkPin() {
          let icc = gIccService.getIccByServiceId(options.serviceId);
          let imsi = icc ? icc.imsi : null;
          return CP.Authenticator.formatImsi(imsi);
        });
      }

      msg = CP.PduHelper.parse(data, contentType);
    } else {
      
      msg = {
        contentType: contentType,
        content: data.array
      };
      msg.content.length = data.array.length;
    }

    let sender = PhoneNumberUtils.normalize(options.sourceAddress, false);
    let parsedSender = PhoneNumberUtils.parse(sender);
    if (parsedSender && parsedSender.internationalNumber) {
      sender = parsedSender.internationalNumber;
    }

    gSystemMessenger.broadcastMessage("wappush-received", {
      sender:         sender,
      contentType:    msg.contentType,
      content:        msg.content,
      authInfo:       authInfo,
      serviceId:      options.serviceId
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

