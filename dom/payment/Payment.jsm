



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = [];

const PAYMENT_IPC_MSG_NAMES = ["Payment:Pay",
                               "Payment:Success",
                               "Payment:Failed"];

const PREF_PAYMENTPROVIDERS_BRANCH = "dom.payment.provider.";
const PREF_PAYMENT_BRANCH = "dom.payment.";

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
    
    this.registeredProviders = null;

    this.messageManagers = {};

    
    
    
    let paymentPrefs = prefService.getBranch(PREF_PAYMENT_BRANCH);
    this.checkHttps = true;
    try {
      if (paymentPrefs.getPrefType("skipHTTPSCheck")) {
        this.checkHttps = !paymentPrefs.getBoolPref("skipHTTPSCheck");
      }
    } catch(e) {}

    for each (let msgname in PAYMENT_IPC_MSG_NAMES) {
      ppmm.addMessageListener(msgname, this);
    }

    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  


  receiveMessage: function receiveMessage(aMessage) {
    let name = aMessage.name;
    let msg = aMessage.json;
    debug("Received '" + name + "' message from content process");

    switch (name) {
      case "Payment:Pay": {
        
        if (!this.registeredProviders) {
          this.registeredProviders = {};
          this.registerPaymentProviders();
        }

        
        
        let requestId = msg.requestId;
        this.messageManagers[requestId] = aMessage.target;

        
        
        
        let paymentRequests = [];
        let jwtTypes = [];
        for (let i in msg.jwts) {
          let pr = this.getPaymentRequestInfo(requestId, msg.jwts[i]);
          if (!pr) {
            continue;
          }
          if (!(pr instanceof Ci.nsIDOMPaymentRequestInfo)) {
            return;
          }
          
          if (jwtTypes[pr.type]) {
            this.paymentFailed(requestId,
                               "PAY_REQUEST_ERROR_DUPLICATED_JWT_TYPE");
            return;
          }
          jwtTypes[pr.type] = true;
          paymentRequests.push(pr);
        }

        if (!paymentRequests.length) {
          this.paymentFailed(requestId,
                             "PAY_REQUEST_ERROR_NO_VALID_REQUEST_FOUND");
          return;
        }

        
        
        
        
        let glue = Cc["@mozilla.org/payment/ui-glue;1"]
                   .createInstance(Ci.nsIPaymentUIGlue);
        if (!glue) {
          debug("Could not create nsIPaymentUIGlue instance");
          this.paymentFailed(requestId,
                             "INTERNAL_ERROR_CREATE_PAYMENT_GLUE_FAILED");
          return;
        }

        let confirmPaymentSuccessCb = function successCb(aRequestId,
                                                         aResult) {
          
          let selectedProvider = this.registeredProviders[aResult];
          if (!selectedProvider || !selectedProvider.uri) {
            debug("Could not retrieve a valid provider based on user's " +
                  "selection");
            this.paymentFailed(aRequestId,
                               "INTERNAL_ERROR_NO_VALID_SELECTED_PROVIDER");
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
            this.paymentFailed(aRequestId,
                               "INTERNAL_ERROR_NO_JWT_ASSOCIATED_TO_REQUEST");
            return;
          }

          this.showPaymentFlow(aRequestId, selectedProvider, jwt);
        };

        let confirmPaymentErrorCb = this.paymentFailed;

        glue.confirmPaymentRequest(requestId,
                                   paymentRequests,
                                   confirmPaymentSuccessCb.bind(this),
                                   confirmPaymentErrorCb.bind(this));
        break;
      }
      case "Payment:Success":
      case "Payment:Failed": {
        let mm = this.messageManagers[msg.requestId];
        mm.sendAsyncMessage(name, {
          requestId: msg.requestId,
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

  


  paymentFailed: function paymentFailed(aRequestId, aErrorMsg) {
    let mm = this.messageManagers[aRequestId];
    mm.sendAsyncMessage("Payment:Failed", {
      requestId: aRequestId,
      errorMsg: aErrorMsg
    });
  },

  



  getPaymentRequestInfo: function getPaymentRequestInfo(aRequestId, aJwt) {
    if (!aJwt) {
      this.paymentFailed(aRequestId, "INTERNAL_ERROR_CALL_WITH_MISSING_JWT");
      return true;
    }

    
    

    
    
    let segments = aJwt.split('.');
    if (segments.length !== 3) {
      debug("Error getting payment provider's uri. " +
            "Not enough or too many segments");
      this.paymentFailed(aRequestId,
                         "PAY_REQUEST_ERROR_WRONG_SEGMENTS_COUNT");
      return true;
    }

    let payloadObject;
    try {
      
      
      
      
      
      segments[1] = segments[1].replace("-", "+", "g").replace("_", "/", "g");
      let payload = atob(segments[1]);
      debug("Payload " + payload);
      if (!payload.length) {
        this.paymentFailed(aRequestId, "PAY_REQUEST_ERROR_EMPTY_PAYLOAD");
        return true;
      }
      payloadObject = JSON.parse(payload);
      if (!payloadObject) {
        this.paymentFailed(aRequestId,
                           "PAY_REQUEST_ERROR_ERROR_PARSING_JWT_PAYLOAD");
        return true;
      }
    } catch (e) {
      this.paymentFailed(aRequestId,
                         "PAY_REQUEST_ERROR_ERROR_DECODING_JWT");
      return true;
    }

    if (!payloadObject.typ) {
      this.paymentFailed(aRequestId,
                         "PAY_REQUEST_ERROR_NO_TYP_PARAMETER");
      return true;
    }

    if (!payloadObject.request) {
      this.paymentFailed(aRequestId,
                         "PAY_REQUEST_ERROR_NO_REQUEST_PARAMETER");
      return true;
    }

    
    
    
    
    
    
    let provider = this.registeredProviders[payloadObject.typ];
    if (!provider) {
      debug("Not registered payment provider for jwt type: " +
            payloadObject.typ);
      return false;
    }

    if (!provider.uri || !provider.name) {
      this.paymentFailed(aRequestId,
                         "INTERNAL_ERROR_WRONG_REGISTERED_PAY_PROVIDER");
      return true;
    }

    
    if (this.checkHttps && !/^https/.exec(provider.uri.toLowerCase())) {
      
      debug("Payment provider uris must be https: " + provider.uri);
      this.paymentFailed(aRequestId,
                         "INTERNAL_ERROR_NON_HTTPS_PROVIDER_URI");
      return true;
    }

    let pldRequest = payloadObject.request;
    let request = Cc["@mozilla.org/payment/request-info;1"]
                  .createInstance(Ci.nsIDOMPaymentRequestInfo);
    if (!request) {
      this.paymentFailed(aRequestId,
                         "INTERNAL_ERROR_ERROR_CREATING_PAY_REQUEST");
      return true;
    }
    request.wrappedJSObject.init(aJwt,
                                 payloadObject.typ,
                                 provider.name);
    return request;
  },

  showPaymentFlow: function showPaymentFlow(aRequestId,
                                            aPaymentProvider,
                                            aJwt) {
    let paymentFlowInfo = Cc["@mozilla.org/payment/flow-info;1"]
                          .createInstance(Ci.nsIPaymentFlowInfo);
    paymentFlowInfo.uri = aPaymentProvider.uri;
    paymentFlowInfo.requestMethod = aPaymentProvider.requestMethod;
    paymentFlowInfo.jwt = aJwt;

    let glue = Cc["@mozilla.org/payment/ui-glue;1"]
               .createInstance(Ci.nsIPaymentUIGlue);
    if (!glue) {
      debug("Could not create nsIPaymentUIGlue instance");
      this.paymentFailed(aRequestId,
                         "INTERNAL_ERROR_CREATE_PAYMENT_GLUE_FAILED");
      return false;
    }
    glue.showPaymentFlow(aRequestId,
                         paymentFlowInfo,
                         this.paymentFailed.bind(this));
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
