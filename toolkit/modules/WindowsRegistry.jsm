



"use strict";
const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

this.EXPORTED_SYMBOLS = ["WindowsRegistry"];

const WindowsRegistry = {
  











  readRegKey: function(aRoot, aPath, aKey) {
    const kRegMultiSz = 7;
    let registry = Cc["@mozilla.org/windows-registry-key;1"].
                   createInstance(Ci.nsIWindowsRegKey);
    try {
      registry.open(aRoot, aPath, Ci.nsIWindowsRegKey.ACCESS_READ);
      if (registry.hasValue(aKey)) {
        let type = registry.getValueType(aKey);
        switch (type) {
          case kRegMultiSz:
            
            let str = registry.readStringValue(aKey);
            return [v for each (v in str.split("\0")) if (v)];
          case Ci.nsIWindowsRegKey.TYPE_STRING:
            return registry.readStringValue(aKey);
          case Ci.nsIWindowsRegKey.TYPE_INT:
            return registry.readIntValue(aKey);
          default:
            throw new Error("Unsupported registry value.");
        }
      }
    } catch (ex) {
    } finally {
      registry.close();
    }
    return undefined;
  },
};
