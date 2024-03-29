



"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/JNI.jsm");

function PaymentProviderStrategy() {
}

PaymentProviderStrategy.prototype = {
  get paymentServiceId() {
    
    return null;
  },

  set paymentServiceId(aServiceId) {
    
  },

  _getNetworkInfo: function(type) {
    let jenv = JNI.GetForThread();
    let jMethodName = "get" + type.toUpperCase();
    let jGeckoNetworkManager = JNI.LoadClass(
      jenv, "org/mozilla/gecko/GeckoNetworkManager", {
      static_methods: [
        { name: jMethodName, sig: "()I" },
      ],
    });
    let val = jGeckoNetworkManager[jMethodName]();
    JNI.UnloadClasses(jenv);

    if (val < 0) {
      return null;
    }
    return val;
  },

  get iccInfo() {
    if (!this._iccInfo) {
      
      this._iccInfo = [{
        mcc: this._getNetworkInfo("mcc"),
        mnc: this._getNetworkInfo("mnc")
      }];
    }
    return this._iccInfo;
  },

  cleanup: function() {
    
  },

  classID: Components.ID("{a497520d-37e7-4f18-8c27-5b8938e9490d}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIPaymentProviderStrategy])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PaymentProviderStrategy]);
