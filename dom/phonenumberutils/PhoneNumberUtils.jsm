


"use strict";

this.EXPORTED_SYMBOLS = ["PhoneNumberUtils"];

const DEBUG = false;
function debug(s) { if(DEBUG) dump("-*- PhoneNumberutils: " + s + "\n"); }

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/PhoneNumberNormalizer.jsm");
Cu.import("resource://gre/modules/mcc_iso3166_table.jsm");

#ifdef MOZ_B2G_RIL
XPCOMUtils.defineLazyServiceGetter(this, "mobileConnection",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIMobileConnectionProvider");
#endif

this.PhoneNumberUtils = {
  init: function() {
    ppmm.addMessageListener(["PhoneNumberService:FuzzyMatch"], this);
  },
  
  
  
  

  
  _mcc: '724',

  getCountryName: function getCountryName() {
    let mcc;
    let countryName;

#ifdef MOZ_B2G_RIL
    
    let voice = mobileConnection.voiceConnectionInfo;
    if (voice && voice.network && voice.network.mcc) {
      mcc = voice.network.mcc;
    }

    
    let iccInfo = mobileConnection.iccInfo;
    if (!mcc && iccInfo.mcc) {
      mcc = iccInfo.mcc;
    }

    
    if (!mcc) {
      try {
        mcc = Services.prefs.getCharPref("ril.lastKnownSimMcc");
      } catch (e) {}
    }

    
    if (!mcc) {
      mcc = this._mcc;
    }
#else
    mcc = this._mcc;
#endif

    countryName = MCC_ISO3166_TABLE[mcc];
    if (DEBUG) debug("MCC: " + mcc + "countryName: " + countryName);
    return countryName;
  },

  parse: function(aNumber) {
    if (DEBUG) debug("call parse: " + aNumber);
    let result = PhoneNumber.Parse(aNumber, this.getCountryName());
    if (DEBUG) {
      if (result) {
        debug("InternationalFormat: " + result.internationalFormat);
        debug("InternationalNumber: " + result.internationalNumber);
        debug("NationalNumber: " + result.nationalNumber);
        debug("NationalFormat: " + result.nationalFormat);
      } else {
        debug("No result!\n");
      }
    }
    return result;
  },

  parseWithMCC: function(aNumber, aMCC) {
    let countryName = MCC_ISO3166_TABLE[aMCC];
    if (DEBUG) debug("found country name: " + countryName);
    return PhoneNumber.Parse(aNumber, countryName);
  },

  isPlainPhoneNumber: function isPlainPhoneNumber(aNumber) {
    var isPlain = PhoneNumber.IsPlain(aNumber);
    if (DEBUG) debug("isPlain(" + aNumber + ") " + isPlain);
    return isPlain;
  },

  normalize: function Normalize(aNumber, aNumbersOnly) {
    let normalized = PhoneNumberNormalizer.Normalize(aNumber, aNumbersOnly);
    if (DEBUG) debug("normalize(" + aNumber + "): " + normalized + ", " + aNumbersOnly);
    return normalized;
  },

  fuzzyMatch: function fuzzyMatch(aNumber1, aNumber2) {
    let normalized1 = this.normalize(aNumber1);
    let normalized2 = this.normalize(aNumber2);
    if (DEBUG) debug("Normalized Number1: " + normalized1 + ", Number2: " + normalized2);
    if (normalized1 === normalized2) {
      return true;
    }
    let parsed1 = this.parse(aNumber1);
    let parsed2 = this.parse(aNumber2);
    if (parsed1 && parsed2) {
      if (parsed1.internationalNumber === parsed2.internationalNumber
          || parsed1.nationalNumber === parsed2.nationalNumber) {
        return true;
      }
    }
    let countryName = this.getCountryName();
    let ssPref = "dom.phonenumber.substringmatching." + countryName;
    if (Services.prefs.getPrefType(ssPref) == Ci.nsIPrefBranch.PREF_INT) {
      let val = Services.prefs.getIntPref(ssPref);
      if (normalized1.slice(-val) === normalized2.slice(-val)) {
        return true;
      }
    }
    return false;
  },

  receiveMessage: function(aMessage) {
    if (DEBUG) debug("receiveMessage " + aMessage.name);
    let mm = aMessage.target;
    let msg = aMessage.data;

    switch (aMessage.name) {
      case "PhoneNumberService:FuzzyMatch":
        mm.sendAsyncMessage("PhoneNumberService:FuzzyMatch:Return:OK", {
          requestID: msg.requestID,
          result: this.fuzzyMatch(msg.options.number1, msg.options.number2)
        });
        break;
      default:
        if (DEBUG) debug("WRONG MESSAGE NAME: " + aMessage.name);
    }
  }
};

let inParent = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime)
                 .processType == Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
if (inParent) {
  Cu.import("resource://gre/modules/PhoneNumber.jsm");
  XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                     "@mozilla.org/parentprocessmessagemanager;1",
                                     "nsIMessageListenerManager");
  PhoneNumberUtils.init();
}

