


"use strict";

this.EXPORTED_SYMBOLS = ["PhoneNumberUtils"];

const DEBUG = false;
function debug(s) { if(DEBUG) dump("-*- PhoneNumberutils: " + s + "\n"); }

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
XPCOMUtils.defineLazyModuleGetter(this, "PhoneNumberNormalizer",
                                  "resource://gre/modules/PhoneNumberNormalizer.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MCC_ISO3166_TABLE",
                                  "resource://gre/modules/mcc_iso3166_table.jsm");

#ifdef MOZ_B2G_RIL
XPCOMUtils.defineLazyServiceGetter(this, "mobileConnection",
                                   "@mozilla.org/mobileconnection/mobileconnectionservice;1",
                                   "nsIMobileConnectionService");
XPCOMUtils.defineLazyServiceGetter(this, "icc",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIIccProvider");
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
    
    
    
    
    
    
    let clientId = 0;

    
    let connection = mobileConnection.getItemByServiceId(clientId);
    let voice = connection && connection.voice;
    if (voice && voice.network && voice.network.mcc) {
      mcc = voice.network.mcc;
    }

    
    let iccInfo = icc.getIccInfo(clientId);
    if (!mcc && iccInfo && iccInfo.mcc) {
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

    
    if (!mcc) {
      try {
        mcc = Services.prefs.getCharPref("ril.lastKnownSimMcc");
      } catch (e) {}
    }

    if (!mcc) {
      mcc = this._mcc;
    }
#endif

    countryName = MCC_ISO3166_TABLE[mcc];
    if (DEBUG) debug("MCC: " + mcc + "countryName: " + countryName);
    return countryName;
  },

  parse: function(aNumber) {
    if (DEBUG) debug("call parse: " + aNumber);
    let result = PhoneNumber.Parse(aNumber, this.getCountryName());

    if (result) {
      let countryName = result.countryName || this.getCountryName();
      let number = null;
      if (countryName) {
        if (Services.prefs.getPrefType("dom.phonenumber.substringmatching." + countryName) == Ci.nsIPrefBranch.PREF_INT) {
          let val = Services.prefs.getIntPref("dom.phonenumber.substringmatching." + countryName);
          if (val) {
            number = result.internationalNumber || result.nationalNumber;
            if (number && number.length > val) {
              number = number.slice(-val);
            }
          }
        }
      }
      Object.defineProperty(result, "nationalMatchingFormat", { value: number, enumerable: true });
      if (DEBUG) {
        debug("InternationalFormat: " + result.internationalFormat);
        debug("InternationalNumber: " + result.internationalNumber);
        debug("NationalNumber: " + result.nationalNumber);
        debug("NationalFormat: " + result.nationalFormat);
        debug("CountryName: " + result.countryName);
        debug("NationalMatchingFormat: " + result.nationalMatchingFormat);
      }
    } else if (DEBUG) {
      debug("NO PARSING RESULT!");
    }
    return result;
  },

  parseWithMCC: function(aNumber, aMCC) {
    let countryName = MCC_ISO3166_TABLE[aMCC];
    if (DEBUG) debug("found country name: " + countryName);
    return PhoneNumber.Parse(aNumber, countryName);
  },

  parseWithCountryName: function(aNumber, countryName) {
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
      if ((parsed1.internationalNumber && parsed1.internationalNumber === parsed2.internationalNumber)
          || (parsed1.nationalNumber && parsed1.nationalNumber === parsed2.nationalNumber)) {
        return true;
      }
    }
    let countryName = this.getCountryName();
    let ssPref = "dom.phonenumber.substringmatching." + countryName;
    if (Services.prefs.getPrefType(ssPref) == Ci.nsIPrefBranch.PREF_INT) {
      let val = Services.prefs.getIntPref(ssPref);
      if (normalized1.length > val && normalized2.length > val
         && normalized1.slice(-val) === normalized2.slice(-val)) {
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
  XPCOMUtils.defineLazyModuleGetter(this, "PhoneNumber",
                                    "resource://gre/modules/PhoneNumber.jsm");
  XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                     "@mozilla.org/parentprocessmessagemanager;1",
                                     "nsIMessageListenerManager");
  PhoneNumberUtils.init();
}

