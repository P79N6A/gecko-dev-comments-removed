


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
XPCOMUtils.defineLazyServiceGetter(this, "ril",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIRILContentHelper");
#endif

this.PhoneNumberUtils = {
  
  
  
  

  
  _mcc: '724',

  _getCountryName: function() {
    let mcc;
    let countryName;

#ifdef MOZ_B2G_RIL
    
    if (ril.voiceConnectionInfo && ril.voiceConnectionInfo.network) {
      mcc = ril.voiceConnectionInfo.network.mcc;
    }

    
    if (!mcc) {
      mcc = ril.iccInfo.mcc;
    }

    
    if (!mcc && ril.voiceConnectionInfo) {
      mcc = ril.voiceConnectionInfo.lastKnownMcc;
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

  isViablePhoneNumber: function IsViablePhoneNumber(aNumber) {
    let viable = PhoneNumber.IsViable(aNumber);
    if (DEBUG) debug("IsViable(" + aNumber + "): " + viable);
    return viable;
  }
};
