



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/JNI.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

function paymentSuccess(aRequestId) {
  return function(aResult) {
    closePaymentTab(aRequestId, function() {
      cpmm.sendAsyncMessage("Payment:Success", { result: aResult,
                                    requestId: aRequestId });
    });
  }
}

function paymentFailed(aRequestId) {
  return function(aErrorMsg) {
    closePaymentTab(aRequestId, function() {
      cpmm.sendAsyncMessage("Payment:Failed", { errorMsg: aErrorMsg,
                                    requestId: aRequestId });
    });
  }
}

let paymentTabs = {};
let cancelTabCallbacks = {};
function paymentCanceled(aRequestId) {
  return function() {
    paymentFailed(aRequestId)();
  }
}
function closePaymentTab(aId, aCallback) {
  if (paymentTabs[aId]) {
    paymentTabs[aId].browser.removeEventListener("TabClose", cancelTabCallbacks[aId]);
    delete cancelTabCallbacks[aId];

    
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
    if (!this._bundle) {
      this._bundle = Services.strings.createBundle("chrome://browser/locale/payments.properties");
    }
    return this._bundle;
  },

  confirmPaymentRequest: function confirmPaymentRequest(aRequestId,
                                                        aRequests,
                                                        aSuccessCb,
                                                        aErrorCb) {
    let _error = this._error(aErrorCb);

    let listItems = [];

    
    if (aRequests.length == 1) {
      aSuccessCb.onresult(aRequestId, aRequests[0].type);
      return;
    }

    
    for (let i = 0; i < aRequests.length; i++) {
      let request = aRequests[i];
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
        aSuccessCb.onresult(aRequestId, aRequests[data.button].type);
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

    
    tab.browser.addEventListener("DOMWindowCreated", function loadPaymentShim() {
      let frame = tab.browser.contentDocument.defaultView;
      try {
        frame.wrappedJSObject.mozPaymentProvider = {
          __exposedProps__: {
            paymentSuccess: 'r',
            paymentFailed: 'r',
            mnc: 'r',
            mcc: 'r',
          },

          _getNetworkInfo: function(type) {
            let jenv = JNI.GetForThread();
            let jMethodName = "get" + type.toUpperCase();
            let jGeckoNetworkManager = JNI.LoadClass(jenv, "org/mozilla/gecko/GeckoNetworkManager", {
              static_methods: [
                { name: jMethodName, sig: "()I" },
              ],
            });
            let val = jGeckoNetworkManager[jMethodName]();
            JNI.UnloadClasses(jenv);

            if (val < 0)
              return null;
            return val;
          },

          get mnc() {
            delete this.mnc;
            return this.mnc = this._getNetworkInfo("mnc");
          },

          get mcc() {
            delete this.mcc;
            return this.mcc = this._getNetworkInfo("mcc");
          },

          paymentSuccess: paymentSuccess(aRequestId),
          paymentFailed: paymentFailed(aRequestId)
        };
      } catch (e) {
        _error(aRequestId, "ERROR_ADDING_METHODS");
      } finally {
        tab.browser.removeEventListener("DOMWindowCreated", loadPaymentShim);
      }
    }, true);

    
    paymentTabs[aRequestId] = tab;
    cancelTabCallbacks[aRequestId] = paymentCanceled(aRequestId);

    
    tab.browser.addEventListener("TabClose", cancelTabCallbacks[aRequestId]);
  },

  cleanup: function cleanup() {
    
  },

  classID: Components.ID("{3c6c9575-f57e-427b-a8aa-57bc3cbff48f}"), 
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentUIGlue])
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentUI]);
