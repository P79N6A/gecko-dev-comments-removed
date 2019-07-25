


































Components.utils.import("resource:///modules/XPCOMUtils.jsm");

function TestInterfaceA() {}
TestInterfaceA.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces["nsIXPCTestInterfaceA"]]),
  contractID: "@mozilla.org/js/xpc/test/js/TestInterfaceA;1",
  classID: Components.ID("{3c8fd2f5-970c-42c6-b5dd-cda1c16dcfd8}"),

  
  name: "TestInterfaceADefaultName"
};

function TestInterfaceB() {}
TestInterfaceB.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces["nsIXPCTestInterfaceB"]]),
  contractID: "@mozilla.org/js/xpc/test/js/TestInterfaceB;1",
  classID: Components.ID("{ff528c3a-2410-46de-acaa-449aa6403a33}"),

  
  name: "TestInterfaceADefaultName"
};


var NSGetFactory = XPCOMUtils.generateNSGetFactory([TestInterfaceA, TestInterfaceB]);
