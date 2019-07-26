



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PAYMENTREQUESTINFO_CID =
  Components.ID("{0a58c67d-f003-48da-81d1-bd8f605f4b1c}");



function PaymentRequestInfo() {
  this.wrappedJSObject = this;
};

PaymentRequestInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMPaymentRequestInfo]),
  classID: PAYMENTREQUESTINFO_CID,
  classInfo: XPCOMUtils.generateCI({
    classID: PAYMENTREQUESTINFO_CID,
    contractID: "@mozilla.org/payment/request-info;1",
    classDescription: "Payment request information",
    flags: Ci.nsIClassInfo.DOM_OBJECT,
    interfaces: [Ci.nsIDOMPaymentRequestInfo]
  }),
  jwt: null,
  type: null,
  providerName: null,

  init: function init(aJwt, aType, aProviderName) {
    this.jwt = aJwt;
    this.type = aType;
    this.providerName = aProviderName;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentRequestInfo]);
