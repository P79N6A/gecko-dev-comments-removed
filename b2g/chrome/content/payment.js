








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

#ifdef MOZ_B2G_RIL
XPCOMUtils.defineLazyServiceGetter(this, "mobileConnection",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIMobileConnectionProvider");
#endif


const kClosePaymentFlowEvent = "close-payment-flow-dialog";

let _requestId;

let PaymentProvider = {

  __exposedProps__: {
    paymentSuccess: 'r',
    paymentFailed: 'r',
    iccIds: 'r'
  },

  _closePaymentFlowDialog: function _closePaymentFlowDialog(aCallback) {
    
    
    
    let id = kClosePaymentFlowEvent + "-" + uuidgen.generateUUID().toString();

    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content) {
      return;
    }

    let detail = {
      type: kClosePaymentFlowEvent,
      id: id,
      requestId: _requestId
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
  },

  paymentSuccess: function paymentSuccess(aResult) {
    this._closePaymentFlowDialog(function notifySuccess() {
      if (!_requestId) {
        return;
      }
      cpmm.sendAsyncMessage("Payment:Success", { result: aResult,
                                                 requestId: _requestId });
    });
  },

  paymentFailed: function paymentFailed(aErrorMsg) {
    this._closePaymentFlowDialog(function notifyError() {
      if (!_requestId) {
        return;
      }
      cpmm.sendAsyncMessage("Payment:Failed", { errorMsg: aErrorMsg,
                                                requestId: _requestId });
    });
  },

  get iccIds() {
#ifdef MOZ_B2G_RIL
    
    
    
    
    return [mobileConnection.iccInfo.iccid];
#else
    return null;
#endif
  },

};



addMessageListener("Payment:LoadShim", function receiveMessage(aMessage) {
  _requestId = aMessage.json.requestId;
});

addEventListener("DOMWindowCreated", function(e) {
  content.wrappedJSObject.mozPaymentProvider = PaymentProvider;
});
