



"use strict";

this.EXPORTED_SYMBOLS = ["DialNumberUtils"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/systemlibs.js");

const DEFAULT_EMERGENCY_NUMBERS = ["112", "911"];

const MMI_MATCH_GROUP_FULL_MMI = 1;
const MMI_MATCH_GROUP_PROCEDURE = 2;
const MMI_MATCH_GROUP_SERVICE_CODE = 3;
const MMI_MATCH_GROUP_SIA = 4;
const MMI_MATCH_GROUP_SIB = 5;
const MMI_MATCH_GROUP_SIC = 6;
const MMI_MATCH_GROUP_PWD_CONFIRM = 7;
const MMI_MATCH_GROUP_DIALING_NUMBER = 8;

this.DialNumberUtils = {
  



  isEmergency: function(aNumber) {
    
    let numbers = libcutils.property_get("ril.ecclist") ||
                  libcutils.property_get("ro.ril.ecclist");
    if (numbers) {
      numbers = numbers.split(",");
    } else {
      
      numbers = DEFAULT_EMERGENCY_NUMBERS;
    }
    return numbers.indexOf(aNumber) != -1;
  },

  _mmiRegExp: (function() {
    
    let procedure = "(\\*[*#]?|##?)";

    
    
    let serviceCode = "(\\d{2,3})";

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let si = "\\*([^*#]*)";
    let allSi = "";
    for (let i = 0; i < 4; ++i) {
      allSi = "(?:" + si + allSi + ")?";
    }

    let fullmmi = "(" + procedure + serviceCode + allSi + "#)";

    
    let optionalDialString = "([^#]+)?";

    return new RegExp("^" + fullmmi + optionalDialString + "$");
  })(),

  











  parseMMI: function(aString) {
    let matches = this._mmiRegExp.exec(aString);
    if (matches) {
      return {
        fullMMI: matches[MMI_MATCH_GROUP_FULL_MMI],
        procedure: matches[MMI_MATCH_GROUP_PROCEDURE],
        serviceCode: matches[MMI_MATCH_GROUP_SERVICE_CODE],
        sia: matches[MMI_MATCH_GROUP_SIA],
        sib: matches[MMI_MATCH_GROUP_SIB],
        sic: matches[MMI_MATCH_GROUP_SIC],
        pwd: matches[MMI_MATCH_GROUP_PWD_CONFIRM],
        dialNumber: matches[MMI_MATCH_GROUP_DIALING_NUMBER]
      };
    }

    return null;
  }
};
