



"use strict";

const DEBUG = false;
function debug(s) { dump("-*- PhoneNumberService.js: " + s + "\n"); }

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");
Cu.import("resource://gre/modules/PhoneNumberNormalizer.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");



function PhoneNumberService()
{
  if (DEBUG) debug("Constructor");
}

PhoneNumberService.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  receiveMessage: function(aMessage) {
    if (DEBUG) debug("receiveMessage: " + aMessage.name);
    let msg = aMessage.json;

    let req = this.getRequest(msg.requestID);
    if (!req) {
      return;
    }

    switch (aMessage.name) {
      case "PhoneNumberService:FuzzyMatch:Return:KO":
        Services.DOMRequest.fireError(req.request, msg.errorMsg);
        break;
      case "PhoneNumberService:FuzzyMatch:Return:OK":
        Services.DOMRequest.fireSuccess(req.request, msg.result);
        break;
      default:
        if (DEBUG) debug("Wrong message: " + aMessage.name);
    }
    this.removeRequest(msg.requestID);
  },

  fuzzyMatch: function(aNumber1, aNumber2) {
    if (DEBUG) debug("fuzzyMatch: " + aNumber1 + ", " + aNumber2);
    let request = this.createRequest();

    if ((aNumber1 && !aNumber2) || (aNumber2 && !aNumber1)) {
      
      
      Services.DOMRequest.fireSuccessAsync(request, false);
    } else if ((aNumber1 === aNumber2) ||
        (PhoneNumberNormalizer.Normalize(aNumber1) === PhoneNumberNormalizer.Normalize(aNumber2))) {
      
      Services.DOMRequest.fireSuccessAsync(request, true);
    } else {
      
      let options = { number1: aNumber1, number2: aNumber2 };
      cpmm.sendAsyncMessage("PhoneNumberService:FuzzyMatch",
                           {requestID: this.getRequestId({request: request}),
                            options: options});
    }

    return request;
  },

  normalize: function(aNumber) {
    if (DEBUG) debug("normalize: " + aNumber);
    return PhoneNumberNormalizer.Normalize(aNumber);
  },

  init: function(aWindow) {
    if (DEBUG) debug("init call");
    this.initDOMRequestHelper(aWindow, [
      "PhoneNumberService:FuzzyMatch:Return:OK",
      "PhoneNumberService:FuzzyMatch:Return:KO"
    ]);
  },

  classID : Components.ID("{e2768710-eb17-11e2-91e2-0800200c9a66}"),
  contractID : "@mozilla.org/phoneNumberService;1",
  QueryInterface : XPCOMUtils.generateQI([Ci.nsIDOMGlobalPropertyInitializer,
                                          Ci.nsISupportsWeakReference]),
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PhoneNumberService]);
