


"use strict";

this.EXPORTED_SYMBOLS = ["PhoneNumberUtils"];

const DEBUG = false;
function debug(s) { if(DEBUG) dump("-*- PhoneNumberutils: " + s + "\n"); }

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import("resource://gre/modules/PhoneNumber.jsm");
Cu.import("resource://gre/modules/mcc_iso3166_table.jsm");

#ifdef MOZ_B2G_RIL
XPCOMUtils.defineLazyServiceGetter(this, "mobileConnection",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIMobileConnectionProvider");
#endif

this.PhoneNumberUtils = {
  
  
  
  

  
  _mcc: '724',

  _getCountryName: function() {
    let mcc;
    let countryName;

#ifdef MOZ_B2G_RIL
    
    if (mobileConnection.voiceConnectionInfo &&
        mobileConnection.voiceConnectionInfo.network) {
      mcc = mobileConnection.voiceConnectionInfo.network.mcc;
    }

    
    if (!mcc) {
      mcc = mobileConnection.iccInfo.mcc;
    }

    
    if (!mcc && mobileConnection.voiceConnectionInfo) {
      mcc = mobileConnection.voiceConnectionInfo.lastKnownMcc;
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
    let result = PhoneNumber.Parse(aNumber, this._getCountryName());
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

  normalize: function Normalize(aNumber) {
    var normalized = PhoneNumber.Normalize(aNumber);
    if (DEBUG) debug("normalize(" + aNumber + "): " + normalized);
    return normalized;
  }
};
