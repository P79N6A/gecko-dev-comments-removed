








"use strict";

dump("======================= payment.js ======================= \n");

let { classes: Cc, interfaces: Ci, utils: Cu }  = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

const kClosePaymentFlowEvent = "close-payment-flow-dialog";

function paymentSuccess(aResult) {
  closePaymentFlowDialog(function notifySuccess() {
    cpmm.sendAsyncMessage("Payment:Success", { result: aResult });
  });
}

function paymentFailed(aErrorMsg) {
  closePaymentFlowDialog(function notifyError() {
    cpmm.sendAsyncMessage("Payment:Failed", { errorMsg: aErrorMsg });
  });
}

function closePaymentFlowDialog(aCallback) {
  
  
  
  let randomId = uuidgen.generateUUID().toString();
  let id = kClosePaymentFlowEvent + "-" + randomId;

  let browser = Services.wm.getMostRecentWindow("navigator:browser");
  let content = browser.getContentWindow();
  if (!content) {
    return;
  }

  let detail = {
    type: kClosePaymentFlowEvent,
    id: id
  };

  
  
  
  
  content.addEventListener("mozContentEvent",
                           function closePaymentFlowReturn(evt) {
    if (evt.detail.id == id && aCallback) {
      aCallback();
    }

    content.removeEventListener("mozContentEvent",
                                closePaymentFlowReturn);
  });

  browser.shell.sendChromeEvent(detail);
}

addEventListener("DOMContentLoaded", function(e) {
  content.wrappedJSObject.paymentSuccess = paymentSuccess;
  content.wrappedJSObject.paymentFailed = paymentFailed;
});
