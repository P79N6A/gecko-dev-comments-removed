



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

function paymentSuccess(aRequestId) {
  return paymentCallback(aRequestId, "Payment:Success");
}

function paymentFailed(aRequestId) {
  return paymentCallback(aRequestId, "Payment:Failed");
}

function paymentCallback(aRequestId, aMsg) {
  return function(aResult) {
    closePaymentTab(aRequestId, function() {
      cpmm.sendAsyncMessage(aMsg, { result: aResult,
                                    requestId: aRequestId });
    });
  }
}

let paymentTabs = {};

function closePaymentTab(aId, aCallback) {
  if (paymentTabs[aId]) {
    
    let content = Services.wm.getMostRecentWindow("navigator:browser");
    if (content) {
      content.BrowserApp.closeTab(paymentTabs[aId]);
    }

    paymentTabs[aId] = null;
  }

  aCallback();
}

function PaymentUI() {
}

PaymentUI.prototype = {
  get bundle() {
    delete this.bundle;
    return this.bundle = Services.strings.createBundle("chrome://browser/locale/payments.properties");
  },

  sendMessageToJava: function(aMsg) {
    let data = Services.androidBridge.handleGeckoMessage(JSON.stringify(aMsg));
    return JSON.parse(data);
  },

  confirmPaymentRequest: function confirmPaymentRequest(aRequestId,
                                                        aRequests,
                                                        aSuccessCb,
                                                        aErrorCb) {
    let _error = this._error(aErrorCb);

    let listItems = [];

    
    if (aRequests.length == 1) {
      aSuccessCb.onresult(aRequestId, aRequests[0].wrappedJSObject.type);
      return;
    }

    
    for (let i = 0; i < aRequests.length; i++) {
      let request = aRequests[i].wrappedJSObject;
      let requestText = request.providerName;
      if (request.productPrice) {
        requestText += " (" + request.productPrice[0].amount + " " +
                              request.productPrice[0].currency + ")";
      }
      listItems.push({ label: requestText });
    }

    let p = new Prompt({
      window: null,
      title: this.bundle.GetStringFromName("payments.providerdialog.title"),
    }).setSingleChoiceItems(listItems).show(function(data) {
      if (data.button > -1 && aSuccessCb) {
        aSuccessCb.onresult(aRequestId, aRequests[data.button].wrappedJSObject.type);
      } else {
        _error(aRequestId, "USER_CANCELED");
      }
    });
  },

  _error: function(aCallback) {
    return function _error(id, msg) {
      if (aCallback) {
        aCallback.onresult(id, msg);
      }
    };
  },

  showPaymentFlow: function showPaymentFlow(aRequestId,
                                            aPaymentFlowInfo,
                                            aErrorCb) {
    let _error = this._error(aErrorCb);

    
    let content = Services.wm.getMostRecentWindow("navigator:browser");
    if (!content) {
      _error(aRequestId, "NO_CONTENT_WINDOW");
      return;
    }

    
    
    
    let tab = content.BrowserApp.addTab(aPaymentFlowInfo.uri + aPaymentFlowInfo.jwt);

    
    tab.browser.addEventListener("DOMContentLoaded", function loadPaymentShim() {
      let frame = tab.browser.contentDocument.defaultView;
      try {
        frame.wrappedJSObject.paymentSuccess = paymentSuccess(aRequestId);
        frame.wrappedJSObject.paymentFailed = paymentFailed(aRequestId);
      } catch (e) {
        _error(aRequestId, "ERROR_ADDING_METHODS");
      } finally {
        tab.browser.removeEventListener("DOMContentLoaded", loadPaymentShim);
      }
    }, true);

    
    tab.browser.addEventListener("TabClose", function paymentCanceled() {
      paymentFailed(aRequestId)();
    });

    
    paymentTabs[aRequestId] = tab;
  },

  cleanup: function cleanup() {
    
  },

  classID: Components.ID("{3c6c9575-f57e-427b-a8aa-57bc3cbff48f}"), 
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentUIGlue])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentUI]);
