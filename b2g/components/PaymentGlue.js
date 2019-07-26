



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");



const kPaymentShimFile = "chrome://browser/content/payment.js";


const kOpenPaymentConfirmationEvent = "open-payment-confirmation-dialog";
const kOpenPaymentFlowEvent = "open-payment-flow-dialog";

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

function debug (s) {
  
};

function PaymentUI() {
}

PaymentUI.prototype = {

  confirmPaymentRequest: function confirmPaymentRequest(aRequestId,
                                                        aRequests,
                                                        aSuccessCb,
                                                        aErrorCb) {
    let _error = function _error(errorMsg) {
      if (aErrorCb) {
        aErrorCb.onresult(aRequestId, errorMsg);
      }
    };

    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content) {
      _error("NO_CONTENT_WINDOW");
      return;
    }

    
    
    
    let id = kOpenPaymentConfirmationEvent + "-" + this.getRandomId();
    let detail = {
      type: kOpenPaymentConfirmationEvent,
      id: id,
      requestId: aRequestId,
      paymentRequests: aRequests
    };

    
    
    
    content.addEventListener("mozContentEvent", function handleSelection(evt) {
      let msg = evt.detail;
      if (msg.id != id) {
        return;
      }

      if (msg.userSelection && aSuccessCb) {
        aSuccessCb.onresult(aRequestId, msg.userSelection);
      } else if (msg.errorMsg) {
        _error(msg.errorMsg);
      }

      content.removeEventListener("mozContentEvent", handleSelection);
    });

    browser.shell.sendChromeEvent(detail);
  },

  showPaymentFlow: function showPaymentFlow(aRequestId,
                                            aPaymentFlowInfo,
                                            aErrorCb) {
    let _error = function _error(errorMsg) {
      if (aErrorCb) {
        aErrorCb.onresult(aRequestId, errorMsg);
      }
    };

    
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content) {
      _error("NO_CONTENT_WINDOW");
      return;
    }

    let id = kOpenPaymentFlowEvent + "-" + this.getRandomId();
    let detail = {
      type: kOpenPaymentFlowEvent,
      id: id,
      requestId: aRequestId,
      uri: aPaymentFlowInfo.uri,
      method: aPaymentFlowInfo.requestMethod,
      jwt: aPaymentFlowInfo.jwt
    };

    
    
    
    content.addEventListener("mozContentEvent", (function loadPaymentShim(evt) {
      if (evt.detail.id != id) {
        content.removeEventListener("mozContentEvent", loadPaymentShim);
        return;
      }

      
      
      if (!evt.detail.frame && !evt.detail.errorMsg) {
        _error("ERROR_LOADING_PAYMENT_SHIM");
        return;
      }
      let frame = evt.detail.frame;
      let frameLoader = frame.QueryInterface(Ci.nsIFrameLoaderOwner)
                        .frameLoader;
      let mm = frameLoader.messageManager;
      try {
        mm.loadFrameScript(kPaymentShimFile, true);
        mm.sendAsyncMessage("Payment:LoadShim", { requestId: aRequestId });
      } catch (e) {
        debug("Error loading " + kPaymentShimFile + " as a frame script: " + e);
        _error("ERROR_LOADING_PAYMENT_SHIM");
      } finally {
        content.removeEventListener("mozContentEvent", loadPaymentShim);
      }
    }).bind(this));

    
    
    this._notifyPayFlowClosed = function _notifyPayFlowClosed (evt) {
      if (evt.detail.id != id) {
        return;
      }
      if (evt.detail.errorMsg) {
        _error(evt.detail.errorMsg);
        content.removeEventListener("mozContentEvent",
                                    this._notifyPayFlowClosed);
        return;
      }
    };
    content.addEventListener("mozContentEvent",
                             this._notifyPayFlowClosed.bind(this));

    browser.shell.sendChromeEvent(detail);
  },

  cleanup: function cleanup() {
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content) {
      return;
    }
    content.removeEventListener("mozContentEvent", this._notifyPayFlowClosed);
  },

  getRandomId: function getRandomId() {
    return uuidgen.generateUUID().toString();
  },

  classID: Components.ID("{8b83eabc-7929-47f4-8b48-4dea8d887e4b}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentUIGlue])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentUI]);
