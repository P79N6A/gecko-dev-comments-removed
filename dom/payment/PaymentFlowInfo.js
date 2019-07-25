



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function PaymentFlowInfo() {
};

PaymentFlowInfo.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentFlowInfo]),
  classID: Components.ID("{b8bce4e7-fbf0-4719-a634-b1bf9018657c}"),
  uri: null,
  jwt: null,
  requestMethod: null
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentFlowInfo]);
