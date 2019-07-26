








"use strict";

let { classes: Cc, interfaces: Ci, utils: Cu }  = Components;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const PREF_DEBUG = "dom.payment.debug";

let _debug;
try {
  _debug = Services.prefs.getPrefType(PREF_DEBUG) == Ci.nsIPrefBranch.PREF_BOOL
           && Services.prefs.getBoolPref(PREF_DEBUG);
} catch(e){
  _debug = false;
}

function LOG(s) {
  if (!_debug) {
    return;
  }
  dump("== Payment flow == " + s + "\n");
}

if (_debug) {
  LOG("Frame script injected");
}

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

XPCOMUtils.defineLazyServiceGetter(this, "uuidgen",
                                   "@mozilla.org/uuid-generator;1",
                                   "nsIUUIDGenerator");

#ifdef MOZ_B2G_RIL
XPCOMUtils.defineLazyServiceGetter(this, "iccProvider",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIIccProvider");

XPCOMUtils.defineLazyServiceGetter(this, "smsService",
                                   "@mozilla.org/sms/smsservice;1",
                                   "nsISmsService");

const kSilentSmsReceivedTopic = "silent-sms-received";

const MOBILEMESSAGECALLBACK_CID =
  Components.ID("{b484d8c9-6be4-4f94-ab60-c9c7ebcc853d}");




function SilentSmsRequest() {
}
SilentSmsRequest.prototype = {
  __exposedProps__: {
    onsuccess: 'rw',
    onerror: 'rw'
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileMessageCallback]),

  classID: MOBILEMESSAGECALLBACK_CID,

  set onsuccess(aSuccessCallback) {
    this._onsuccess = aSuccessCallback;
  },

  set onerror(aErrorCallback) {
    this._onerror = aErrorCallback;
  },

  notifyMessageSent: function notifyMessageSent(aMessage) {
    if (_DEBUG) {
      _debug("Silent message successfully sent");
    }
    this._onsuccess(aMessage);
  },

  notifySendMessageFailed: function notifySendMessageFailed(aError) {
    if (_DEBUG) {
      _debug("Error sending silent message " + aError);
    }
    this._onerror(aError);
  }
};
#endif

const kClosePaymentFlowEvent = "close-payment-flow-dialog";

let gRequestId;

let gBrowser = Services.wm.getMostRecentWindow("navigator:browser");

let PaymentProvider = {
#ifdef MOZ_B2G_RIL
  __exposedProps__: {
    paymentSuccess: 'r',
    paymentFailed: 'r',
    iccIds: 'r',
    mcc: 'r',
    mnc: 'r',
    sendSilentSms: 'r',
    observeSilentSms: 'r',
    removeSilentSmsObserver: 'r'
  },
#else
  __exposedProps__: {
    paymentSuccess: 'r',
    paymentFailed: 'r'
  },
#endif

  _closePaymentFlowDialog: function _closePaymentFlowDialog(aCallback) {
    
    
    
    let id = kClosePaymentFlowEvent + "-" + uuidgen.generateUUID().toString();

    let content = gBrowser.getContentWindow();
    if (!content) {
      return;
    }

    let detail = {
      type: kClosePaymentFlowEvent,
      id: id,
      requestId: gRequestId
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

    gBrowser.shell.sendChromeEvent(detail);

#ifdef MOZ_B2G_RIL
    this._cleanUp();
#endif
  },

  paymentSuccess: function paymentSuccess(aResult) {
    if (_debug) {
      LOG("paymentSuccess " + aResult);
    }

    PaymentProvider._closePaymentFlowDialog(function notifySuccess() {
      if (!gRequestId) {
        return;
      }
      cpmm.sendAsyncMessage("Payment:Success", { result: aResult,
                                                 requestId: gRequestId });
    });
  },

  paymentFailed: function paymentFailed(aErrorMsg) {
    if (_debug) {
      LOG("paymentFailed " + aErrorMsg);
    }

    PaymentProvider._closePaymentFlowDialog(function notifyError() {
      if (!gRequestId) {
        return;
      }
      cpmm.sendAsyncMessage("Payment:Failed", { errorMsg: aErrorMsg,
                                                requestId: gRequestId });
    });
  },

#ifdef MOZ_B2G_RIL
  
  get iccInfo() {
    delete this.iccInfo;
    return this.iccInfo = iccProvider.getIccInfo(0);
  },

  get iccIds() {
    return [this.iccInfo.iccid];
  },

  get mcc() {
    return [this.iccInfo.mcc];
  },

  get mnc() {
    return [this.iccInfo.mnc];
  },

  _silentNumbers: null,
  _silentSmsObservers: null,

  sendSilentSms: function sendSilentSms(aNumber, aMessage) {
    if (_debug) {
      LOG("Sending silent message " + aNumber + " - " + aMessage);
    }

    let request = new SilentSmsRequest();
    smsService.send(aNumber, aMessage, true, request);
    return request;
  },

  observeSilentSms: function observeSilentSms(aNumber, aCallback) {
    if (_debug) {
      LOG("observeSilentSms " + aNumber);
    }

    if (!this._silentSmsObservers) {
      this._silentSmsObservers = {};
      this._silentNumbers = [];
      Services.obs.addObserver(this._onSilentSms.bind(this),
                               kSilentSmsReceivedTopic,
                               false);
    }

    if (!this._silentSmsObservers[aNumber]) {
      this._silentSmsObservers[aNumber] = [];
      this._silentNumbers.push(aNumber);
      smsService.addSilentNumber(aNumber);
    }

    if (this._silentSmsObservers[aNumber].indexOf(aCallback) == -1) {
      this._silentSmsObservers[aNumber].push(aCallback);
    }
  },

  removeSilentSmsObserver: function removeSilentSmsObserver(aNumber, aCallback) {
    if (_debug) {
      LOG("removeSilentSmsObserver " + aNumber);
    }

    if (!this._silentSmsObservers || !this._silentSmsObservers[aNumber]) {
      if (_debug) {
        LOG("No observers for " + aNumber);
      }
      return;
    }

    let index = this._silentSmsObservers[aNumber].indexOf(aCallback);
    if (index != -1) {
      this._silentSmsObservers[aNumber].splice(index, 1);
      if (this._silentSmsObservers[aNumber].length == 0) {
        this._silentSmsObservers[aNumber] = null;
        this._silentNumbers.splice(this._silentNumbers.indexOf(aNumber), 1);
        smsService.removeSilentNumber(aNumber);
      }
    } else if (_debug) {
      LOG("No callback found for " + aNumber);
    }
  },

  _onSilentSms: function _onSilentSms(aSubject, aTopic, aData) {
    if (_debug) {
      LOG("Got silent message! " + aSubject.sender + " - " + aSubject.body);
    }

    let number = aSubject.sender;
    if (!number || this._silentNumbers.indexOf(number) == -1) {
      if (_debug) {
        LOG("No observers for " + number);
      }
      return;
    }

    this._silentSmsObservers[number].forEach(function(callback) {
      callback(aSubject);
    });
  },

  _cleanUp: function _cleanUp() {
    if (_debug) {
      LOG("Cleaning up!");
    }

    if (!this._silentNumbers) {
      return;
    }

    while (this._silentNumbers.length) {
      let number = this._silentNumbers.pop();
      smsService.removeSilentNumber(number);
    }
    this._silentNumbers = null;
    this._silentSmsObservers = null;
    Services.obs.removeObserver(this._onSilentSms, kSilentSmsReceivedTopic);
  }
#endif
};



addMessageListener("Payment:LoadShim", function receiveMessage(aMessage) {
  gRequestId = aMessage.json.requestId;
});

addEventListener("DOMWindowCreated", function(e) {
  content.wrappedJSObject.mozPaymentProvider = PaymentProvider;
});

#ifdef MOZ_B2G_RIL



gBrowser.getContentWindow().addEventListener("mozContentEvent", function(e) {
  if (e.detail.type === "cancel") {
    PaymentProvider._cleanUp();
  }
});
#endif
