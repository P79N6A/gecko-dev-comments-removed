








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

let requestId;

function paymentSuccess(aResult) {
  closePaymentFlowDialog(function notifySuccess() {
    if (!requestId) {
      return;
    }
    cpmm.sendAsyncMessage("Payment:Success", { result: aResult,
                                               requestId: requestId });
  });
}

function paymentFailed(aErrorMsg) {
  closePaymentFlowDialog(function notifyError() {
    if (!requestId) {
      return;
    }
    cpmm.sendAsyncMessage("Payment:Failed", { errorMsg: aErrorMsg,
                                              requestId: requestId });
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
    id: id,
    requestId: requestId
  };

  
  
  
  
  content.addEventListener("mozContentEvent",
                           function closePaymentFlowReturn(evt) {
    if (evt.detail.id == id && aCallback) {
      aCallback();
    }

    content.removeEventListener("mozContentEvent",
                                closePaymentFlowReturn);

    let glue = Cc["@mozilla.org/payment/ui-glue;1"]
                 .createInstance(Ci.nsIPaymentUIGlue);
    glue.cleanup();
  });

  browser.shell.sendChromeEvent(detail);
}



addMessageListener("Payment:LoadShim", function receiveMessage(aMessage) {
  requestId = aMessage.json.requestId;
});

addEventListener("DOMWindowCreated", function(e) {
  content.wrappedJSObject.paymentSuccess = paymentSuccess;
  content.wrappedJSObject.paymentFailed = paymentFailed;
});
