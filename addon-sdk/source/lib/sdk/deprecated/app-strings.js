



"use strict";

module.metadata = {
  "stability": "deprecated"
};

const {Cc,Ci} = require("chrome");
const apiUtils = require("./api-utils");







exports.StringBundle = apiUtils.publicConstructor(function StringBundle(url) {

  let stringBundle = Cc["@mozilla.org/intl/stringbundle;1"].
                     getService(Ci.nsIStringBundleService).
                     createBundle(url);

  this.__defineGetter__("url", function () url);

  









  this.get = function strings_get(name, args) {
    try {
      if (args)
        return stringBundle.formatStringFromName(name, args, args.length);
      else
        return stringBundle.GetStringFromName(name);
    }
    catch(ex) {
      
      
      throw new Error("String '" + name + "' could not be retrieved from the " +
                      "bundle due to an unknown error (it doesn't exist?).");
    }
  },

  



  apiUtils.addIterator(
    this,
    function keysValsGen() {
      let enumerator = stringBundle.getSimpleEnumeration();
      while (enumerator.hasMoreElements()) {
        let elem = enumerator.getNext().QueryInterface(Ci.nsIPropertyElement);
        yield [elem.key, elem.value];
      }
    }
  );
});
