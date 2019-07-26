


"use strict";

module.metadata = {
  "stability": "stable"
};

const core = require("./l10n/core");
const { getRulesForLocale } = require("./l10n/plural-rules");


let pluralMappingFunction = getRulesForLocale(core.language()) ||
                            getRulesForLocale("en");

exports.get = function get(k) {
  
  
  if (typeof k !== "string")
    throw new Error("First argument of localization method should be a string");

  
  let localized = core.get(k) || k;

  
  
  
  
  
  if (arguments.length <= 1)
    return localized;

  let args = arguments;

  if (typeof localized == "object" && "other" in localized) {
    
    
    
    
    
    let n = arguments[1];

    
    
    
    
    
    if (n === 0 && "zero" in localized)
      localized = localized["zero"];
    else if (n === 1 && "one" in localized)
      localized = localized["one"];
    else if (n === 2 && "two" in localized)
      localized = localized["two"];
    else {
      let pluralForm = pluralMappingFunction(n);
      if (pluralForm in localized)
        localized = localized[pluralForm];
      else 
        localized = localized["other"];
    }

    
    args = [null, n];
  }

  
  
  
  
  
  
  
  
  let offset = 1;
  localized = localized.replace(/%(\d*)(s|d)/g, function (v, n) {
      let rv = args[n != "" ? n : offset];
      offset++;
      return rv;
    });

  return localized;
}
