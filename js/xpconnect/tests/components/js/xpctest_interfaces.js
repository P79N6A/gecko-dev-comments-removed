


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

function TestInterfaceAll() {}
TestInterfaceAll.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces["nsIXPCTestInterfaceA"],
                                         Components.interfaces["nsIXPCTestInterfaceB"],
                                         Components.interfaces["nsIXPCTestInterfaceC"]]),
  contractID: "@mozilla.org/js/xpc/test/js/TestInterfaceAll;1",
  classID: Components.ID("{90ec5c9e-f6da-406b-9a38-14d00f59db76}"),

  
  name: "TestInterfaceAllDefaultName",

  
  someInteger: 42
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([TestInterfaceA, TestInterfaceB, TestInterfaceAll]);

