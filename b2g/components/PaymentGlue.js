



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Promise.jsm");


const kOpenPaymentConfirmationEvent = "open-payment-confirmation-dialog";
const kOpenPaymentFlowEvent = "open-payment-flow-dialog";
const kClosePaymentFlowEvent = "close-payment-flow-dialog";


const kPaymentFlowCancelled = "payment-flow-cancelled";

const PREF_DEBUG = "dom.payment.debug";

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

function PaymentUI() {
  try {
    this._debug =
      Services.prefs.getPrefType(PREF_DEBUG) == Ci.nsIPrefBranch.PREF_BOOL
      && Services.prefs.getBoolPref(PREF_DEBUG);
  } catch(e) {
    this._debug = false;
  }
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

    
    
    
    let id = kOpenPaymentConfirmationEvent + "-" + this.getRandomId();
    let detail = {
      type: kOpenPaymentConfirmationEvent,
      id: id,
      requestId: aRequestId,
      paymentRequests: aRequests
    };

    
    
    
    this._handleSelection = (function _handleSelection(evt) {
      let msg = evt.detail;
      if (msg.id != id) {
        return;
      }

      if (msg.userSelection && aSuccessCb) {
        aSuccessCb.onresult(aRequestId, msg.userSelection);
      } else if (msg.errorMsg) {
        _error(msg.errorMsg);
      }

      SystemAppProxy.removeEventListener("mozContentEvent", this._handleSelection);
      this._handleSelection = null;
    }).bind(this);
    SystemAppProxy.addEventListener("mozContentEvent", this._handleSelection);

    SystemAppProxy.dispatchEvent(detail);
  },

  showPaymentFlow: function showPaymentFlow(aRequestId,
                                            aPaymentFlowInfo,
                                            aErrorCb) {
    let _error = (errorMsg) => {
      if (aErrorCb) {
        aErrorCb.onresult(aRequestId, errorMsg);
      }
    };

    
    let id = kOpenPaymentFlowEvent + "-" + this.getRandomId();
    let detail = {
      type: kOpenPaymentFlowEvent,
      id: id,
      requestId: aRequestId
    };

    this._setPaymentRequest = (event) => {
      let message = event.detail;
      if (message.id != id) {
        return;
      }

      let frame = message.frame;
      let docshell = frame.contentWindow
                          .QueryInterface(Ci.nsIInterfaceRequestor)
                          .getInterface(Ci.nsIWebNavigation)
                          .QueryInterface(Ci.nsIDocShell);
      docshell.paymentRequestId = aRequestId;
      frame.src = aPaymentFlowInfo.uri + aPaymentFlowInfo.jwt;
      SystemAppProxy.removeEventListener("mozContentEvent",
                                         this._setPaymentRequest);
    };
    SystemAppProxy.addEventListener("mozContentEvent",
                                    this._setPaymentRequest);

    
    
    this._notifyPayFlowClosed = (evt) => {
      let msg = evt.detail;
      if (msg.id != id) {
        return;
      }

      if (msg.type != 'cancel') {
        return;
      }

      if (msg.errorMsg) {
        _error(msg.errorMsg);
      }

      SystemAppProxy.removeEventListener("mozContentEvent",
                                         this._notifyPayFlowClosed);
      this._notifyPayFlowClosed = null;

      Services.obs.notifyObservers(null, kPaymentFlowCancelled, null);
    };
    SystemAppProxy.addEventListener("mozContentEvent",
                                    this._notifyPayFlowClosed);

    SystemAppProxy.dispatchEvent(detail);
  },

  closePaymentFlow: function(aRequestId) {
    return new Promise((aResolve) => {
      
      
      
      let id = kClosePaymentFlowEvent + "-" + uuidgen.generateUUID().toString();

      let detail = {
        type: kClosePaymentFlowEvent,
        id: id,
        requestId: aRequestId
      };

      
      
      
      
      SystemAppProxy.addEventListener("mozContentEvent",
                                      (function closePaymentFlowReturn() {
        SystemAppProxy.removeEventListener("mozContentEvent",
                                    closePaymentFlowReturn);
        this.cleanup();
        aResolve();
      }).bind(this));

      SystemAppProxy.dispatchEvent(detail);
    });
  },

  cleanup: function cleanup() {
    if (this._handleSelection) {
      SystemAppProxy.removeEventListener("mozContentEvent",
                                         this._handleSelection);
      this._handleSelection = null;
    }

    if (this._notifyPayFlowClosed) {
      SystemAppProxy.removeEventListener("mozContentEvent",
                                         this._notifyPayFlowClosed);
      this._notifyPayFlowClosed = null;
    }
  },

  getRandomId: function getRandomId() {
    return uuidgen.generateUUID().toString();
  },

  classID: Components.ID("{8b83eabc-7929-47f4-8b48-4dea8d887e4b}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentUIGlue])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentUI]);
