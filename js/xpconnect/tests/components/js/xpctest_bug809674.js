


Components.utils.import("resource:///modules/XPCOMUtils.jsm");

function TestBug809674() {}
TestBug809674.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces["nsIXPCTestBug809674"]]),
  contractID: "@mozilla.org/js/xpc/test/js/Bug809674;1",
  classID: Components.ID("{2df46559-da21-49bf-b863-0d7b7bbcbc73}"),

  
  jsvalProperty: {},
};


this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TestBug809674]);
