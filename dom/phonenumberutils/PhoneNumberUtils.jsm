


"use strict";

this.EXPORTED_SYMBOLS = ["PhoneNumberUtils"];

const DEBUG = true;
function debug(s) { dump("-*- PhoneNumberutils: " + s + "\n"); }

const Cu = Components.utils;

Cu.import("resource://gre/modules/PhoneNumber.jsm");
Cu.import("resource://gre/modules/mcc_iso3166_table.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ril",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIRILContentHelper");

this.PhoneNumberUtils = {
  
  
  
  
  _getCountryName: function() {
    let mcc;
    let countryName;
    
    if (ril.voiceConnectionInfo && ril.voiceConnectionInfo.network)
      mcc = ril.voiceConnectionInfo.network.mcc;

    
    if (!mcc)
      mcc = ril.iccInfo.mcc || '724';

    countryName = MCC_ISO3166_TABLE[mcc];
    debug("MCC: " + mcc + "countryName: " + countryName);
    return countryName;
  },

  parse: function(aNumber) {
    let result = PhoneNumber.Parse(aNumber, this._getCountryName());
    debug("InternationalFormat: " + result.internationalFormat);
    debug("InternationalNumber: " + result.internationalNumber);
    debug("NationalNumber: " + result.nationalNumber);
    debug("NationalFormat: " + result.nationalFormat);
    return result;
  },

  parseWithMCC: function(aNumber, aMCC) {
    let countryName = MCC_ISO3166_TABLE[aMCC];
    debug("found country name: " + countryName);
    return PhoneNumber.Parse(aNumber, countryName);
  },
};
