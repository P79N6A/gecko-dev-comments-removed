



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let EXPORTED_SYMBOLS = [];

const PAYMENT_IPC_MSG_NAMES = ["Payment:Pay",
                               "Payment:Success",
                               "Payment:Failed"];

const PREF_PAYMENTPROVIDERS_BRANCH = "dom.payment.provider.";

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "prefService",
                                   "@mozilla.org/preferences-service;1",
                                   "nsIPrefService");

function debug (s) {
  
};

let PaymentManager =  {
  init: function init() {
    this.requestId = null;

    
    this.registeredProviders = null;

    this.messageManagers = {};

    for each (let msgname in PAYMENT_IPC_MSG_NAMES) {
      ppmm.addMessageListener(msgname, this);
    }

    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  


  receiveMessage: function receiveMessage(aMessage) {
    let name = aMessage.name;
    let msg = aMessage.json;
    debug("Received '" + name + "' message from content process");

    if (msg.requestId) {
      this.requestId = msg.requestId;
    }

    switch (name) {
      case "Payment:Pay": {
        
        if (!this.registeredProviders) {
          this.registeredProviders = {};
          this.registerPaymentProviders();
        }

        
        
        this.messageManagers[this.requestId] = aMessage.target;

        
        
        
        let paymentRequests = [];
        let jwtTypes = [];
        for (let i in msg.jwts) {
          let pr = this.getPaymentRequestInfo(msg.jwts[i]);
          if (!pr) {
            continue;
          }
          
          if (jwtTypes[pr.type]) {
            this.paymentFailed("DUPLICATE_JWT_TYPE");
            return;
          }
          jwtTypes[pr.type] = true;
          paymentRequests.push(pr);
        }

        if (!paymentRequests.length) {
          this.paymentFailed("NO_VALID_PAYMENT_REQUEST");
          return;
        }

        
        
        
        
        let glue = Cc["@mozilla.org/payment/ui-glue;1"]
                   .createInstance(Ci.nsIPaymentUIGlue);
        if (!glue) {
          debug("Could not create nsIPaymentUIGlue instance");
          this.paymentFailed("CREATE_PAYMENT_GLUE_FAILED");
          return;
        }

        let confirmPaymentSuccessCb = function successCb(aResult) {
          
          let selectedProvider = this.registeredProviders[aResult];
          if (!selectedProvider || !selectedProvider.uri) {
            debug("Could not retrieve a valid provider based on user's " +
                  "selection");
            this.paymentFailed("NO_VALID_SELECTED_PROVIDER");
            return;
          }

          let jwt;
          for (let i in paymentRequests) {
            if (paymentRequests[i].type == aResult) {
              jwt = paymentRequests[i].jwt;
              break;
            }
          }
          if (!jwt) {
            debug("The selected request has no JWT information associated");
            this.paymentFailed("NO_JWT_ASSOCIATED_TO_REQUEST");
            return;
          }

          this.showPaymentFlow(selectedProvider, jwt);
        };

        let confirmPaymentErrorCb = this.paymentFailed;

        glue.confirmPaymentRequest(paymentRequests,
                                   confirmPaymentSuccessCb.bind(this),
                                   confirmPaymentErrorCb.bind(this));
        break;
      }
      case "Payment:Success":
      case "Payment:Failed": {
        let mm = this.messageManagers[this.requestId];
        mm.sendAsyncMessage(name, {
          requestId: this.requestId,
          result: msg.result,
          errorMsg: msg.errorMsg
        });
        break;
      }
    }
  },

  


  registerPaymentProviders: function registerPaymentProviders() {
    let paymentProviders = prefService
                           .getBranch(PREF_PAYMENTPROVIDERS_BRANCH)
                           .getChildList("");

    
    let nums = [];
    for (let i in paymentProviders) {
      let match = /^(\d+)\.uri$/.exec(paymentProviders[i]);
      if (!match) {
        continue;
      } else {
        nums.push(match[1]);
      }
    }

    
    for (let i in nums) {
      let branch = prefService
                   .getBranch(PREF_PAYMENTPROVIDERS_BRANCH + nums[i] + ".");
      let vals = branch.getChildList("");
      if (vals.length == 0) {
        return;
      }
      try {
        let type = branch.getCharPref("type");
        if (type in this.registeredProviders) {
          continue;
        }
        this.registeredProviders[type] = {
          name: branch.getCharPref("name"),
          uri: branch.getCharPref("uri"),
          description: branch.getCharPref("description"),
          requestMethod: branch.getCharPref("requestMethod")
        };
        debug("Registered Payment Providers: " +
              JSON.stringify(this.registeredProviders[type]));
      } catch (ex) {
        debug("An error ocurred registering a payment provider. " + ex);
      }
    }
  },

  


  paymentFailed: function paymentFailed(aErrorMsg) {
    let mm = this.messageManagers[this.requestId];
    mm.sendAsyncMessage("Payment:Failed", {
      requestId: this.requestId,
      errorMsg: aErrorMsg
    });
  },

  



  getPaymentRequestInfo: function getPaymentRequestInfo(aJwt) {
    if (!aJwt) {
      return null;
    }

    
    

    
    
    let segments = aJwt.split('.');
    if (segments.length !== 3) {
      debug("Error getting payment provider's uri. " +
            "Not enough or too many segments");
      return null;
    }

    let payloadObject;
    try {
      
      
      
      let payload = atob(segments[1]);
      debug("Payload " + payload);

      
      
      if (payload.charAt(0) === '"') {
        payload = payload.substr(1);
      }
      if (payload.charAt(payload.length - 1) === '"') {
        payload = payload.slice(0, -1);
      }
      payload = payload.replace(/\\/g, '');

      payloadObject = JSON.parse(payload);
    } catch (e) {
      debug("Error decoding jwt " + e);
      return null;
    }

    if (!payloadObject || !payloadObject.typ || !payloadObject.request) {
      debug("Error decoding jwt. Not valid jwt. " +
            "No payload or jwt type or request found");
      return null;
    }

    
    
    let provider = this.registeredProviders[payloadObject.typ];
    if (!provider || !provider.uri || !provider.name) {
      debug("Not registered payment provider for jwt type: " +
            payloadObject.typ);
      return null;
    }

    
    if (!/^https/.exec(provider.uri.toLowerCase())) {
      debug("Payment provider uris must be https: " + provider.uri);
      return null;
    }

    let pldRequest = payloadObject.request;
    let request;
    if (pldRequest.refund) {
      
      request = Cc["@mozilla.org/payment/request-refund-info;1"]
                .createInstance(Ci.nsIDOMPaymentRequestRefundInfo);

      
      if (!request || !pldRequest.reason) {
        debug("Not valid refund request");
        return null;
      }
      request.wrappedJSObject.init(aJwt,
                                   payloadObject.typ,
                                   provider.name,
                                   pldRequest.reason);
    } else {
      
      request = Cc["@mozilla.org/payment/request-payment-info;1"]
                .createInstance(Ci.nsIDOMPaymentRequestPaymentInfo);

      
      
      if (!request || !pldRequest.name || !pldRequest.description ||
          !pldRequest.price) {
        debug("Not valid payment request");
        return null;
      }

      
      
      let productPrices = [];
      if (!Array.isArray(pldRequest.price)) {
        pldRequest.price = [pldRequest.price];
      }

      for (let i in pldRequest.price) {
        if (!pldRequest.price[i].country || !pldRequest.price[i].currency ||
            !pldRequest.price[i].amount) {
          debug("Not valid payment request. " +
                "Price parameter is not well formed");
          return null;
        }
        let price = Cc["@mozilla.org/payment/product-price;1"]
                    .createInstance(Ci.nsIDOMPaymentProductPrice);
        price.wrappedJSObject.init(pldRequest.price[i].country,
                                   pldRequest.price[i].currency,
                                   pldRequest.price[i].amount);
        productPrices.push(price);
      }
      request.wrappedJSObject.init(aJwt,
                                   payloadObject.typ,
                                   provider.name,
                                   pldRequest.name,
                                   pldRequest.description,
                                   productPrices);
    }

    return request;
  },

  showPaymentFlow: function showPaymentFlow(aPaymentProvider, aJwt) {
    let paymentFlowInfo = Cc["@mozilla.org/payment/flow-info;1"]
                          .createInstance(Ci.nsIPaymentFlowInfo);
    paymentFlowInfo.uri = aPaymentProvider.uri;
    paymentFlowInfo.requestMethod = aPaymentProvider.requestMethod;
    paymentFlowInfo.jwt = aJwt;

    let glue = Cc["@mozilla.org/payment/ui-glue;1"]
               .createInstance(Ci.nsIPaymentUIGlue);
    if (!glue) {
      debug("Could not create nsIPaymentUIGlue instance");
      this.paymentFailed("CREATE_PAYMENT_GLUE_FAILED");
      return false;
    }
    glue.showPaymentFlow(paymentFlowInfo, this.paymentFailed);
  },

  

  observe: function observe(subject, topic, data) {
    if (topic == "xpcom-shutdown") {
      for each (let msgname in PAYMENT_IPC_MSG_NAMES) {
        ppmm.removeMessageListener(msgname, this);
      }
      this.registeredProviders = null;
      this.messageManagers = null;

      Services.obs.removeObserver(this, "xpcom-shutdown");
    }
  },
};

PaymentManager.init();
