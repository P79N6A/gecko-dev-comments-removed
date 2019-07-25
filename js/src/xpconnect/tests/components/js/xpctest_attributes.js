


































Components.utils.import("resource:///modules/XPCOMUtils.jsm");

function TestObjectReadWrite() {}
TestObjectReadWrite.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces["nsIXPCTestObjectReadWrite"]]),
  contractID: "@mozilla.org/js/xpc/test/js/ObjectReadWrite;1",
  classID: Components.ID("{8ff41d9c-66e9-4453-924a-7d8de0a5e966}"),

  
  stringProperty: "XPConnect Read-Writable String",
  booleanProperty: true,
  shortProperty: 32767,
  longProperty: 2147483647,
  floatProperty: 5.5,
  charProperty: "X"
};


function TestObjectReadOnly() {}
TestObjectReadOnly.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces["nsIXPCTestObjectReadOnly"]]),
  contractID: "@mozilla.org/js/xpc/test/js/ObjectReadOnly;1",
  classID: Components.ID("{916c4247-253d-4ed0-a425-adfedf53ecc8}"),

  
  strReadOnly: "XPConnect Read-Only String",
  boolReadOnly: true,
  shortReadOnly: 32767,
  longReadOnly: 2147483647,
  floatReadOnly: 5.5,
  charReadOnly: "X"
};


var NSGetFactory = XPCOMUtils.generateNSGetFactory([TestObjectReadWrite, TestObjectReadOnly]);
