



"use strict";

const { interfaces: Ci, utils: Cu } = Components;

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
          closePaymentWindow(aRequestId, function() {
            cpmm.sendAsyncMessage(aMsg, { result: aResult,
                                          requestId: aRequestId });
          });
        };
}

let payments = {};

function closePaymentWindow(aId, aCallback) {
  if (payments[aId]) {
    payments[aId].handled = true;
    payments[aId].win.close();
    payments[aId] = null;
  }

  aCallback();
}

function PaymentUI() {}

PaymentUI.prototype = {
  classID: Components.ID("{ede1124f-72e8-4a31-9567-3270d46f21fb}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentUIGlue]),

  confirmPaymentRequest: function(aRequestId, aRequests, aSuccessCb, aErrorCb) {
    
    
    if (aRequests.length == 1) {
      aSuccessCb.onresult(aRequestId, aRequests[0].wrappedJSObject.type);
      return;
    }

    let items = [];

    
    for (let i = 0; i < aRequests.length; i++) {
      let request = aRequests[i].wrappedJSObject;
      let requestText = request.providerName;
      if (request.productPrice && Array.isArray(request.productPrice)) {
        
        requestText += " (" + request.productPrice[0].amount + " " +
                              request.productPrice[0].currency + ")";
      }
      items.push(requestText);
    }

    let selected = {};

    let bundle = Services.strings.
                   createBundle("chrome://webapprt/locale/webapp.properties");
    let result = Services.prompt.
                   select(null, bundle.GetStringFromName("paymentDialog.title"),
                          bundle.GetStringFromName("paymentDialog.message"),
                          items.length, items, selected);
    if (result) {
      aSuccessCb.onresult(aRequestId,
                          aRequests[selected.value].wrappedJSObject.type);
    } else {
      aErrorCb.onresult(aRequestId, "USER_CANCELLED");
    }
  },

  showPaymentFlow: function(aRequestId, aPaymentFlowInfo, aErrorCb) {
    let win = Services.ww.
                openWindow(null,
                           "chrome://webapprt/content/webapp.xul",
                           "_blank",
                           "chrome,dialog=no,resizable,scrollbars,centerscreen",
                           null);

    
    
    payments[aRequestId] = { win: win, handled: false };

    
    
    win.addEventListener("DOMWindowCreated", function() {
      let browserElement = win.document.getElementById("content");
      browserElement.
        setAttribute("src", aPaymentFlowInfo.uri + aPaymentFlowInfo.jwt);

      browserElement.addEventListener("DOMWindowCreated", function() {
        win.document.getElementById("content").contentDocument.defaultView
           .wrappedJSObject.mozPaymentProvider = {
             __exposedProps__: {
               paymentSuccess: 'r',
               paymentFailed: 'r'
             },
             paymentSuccess: paymentSuccess(aRequestId),
             paymentFailed: paymentFailed(aRequestId)
           };
      }, true);
    });

    let winObserver = function(aClosedWin, aTopic) {
      if (aTopic == "domwindowclosed") {
        
        if (aClosedWin == win) {
          Services.ww.unregisterNotification(winObserver);
          if (!payments[aRequestId].handled) {
            aErrorCb.onresult(aRequestId, "USER_CANCELLED");
          }
        }
      }
    }

    Services.ww.registerNotification(winObserver);
  },

  cleanup: function() {
  },
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentUI]);
