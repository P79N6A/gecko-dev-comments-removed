


































Components.utils.import("resource:///modules/XPCOMUtils.jsm");

function TestParams() {
}


function f(a, b) {
    var rv = b.value;
    b.value = a;
    return rv;
};

TestParams.prototype = {

  
  QueryInterface: XPCOMUtils.generateQI([Components.interfaces["nsIXPCTestParams"]]),
  contractID: "@mozilla.org/js/xpc/test/js/Params;1",
  classID: Components.ID("{e3b86f4e-49c0-487c-a2b0-3a986720a044}"),

  
  testBoolean: f,
  testOctet: f,
  testShort: f,
  testLong: f,
  testLongLong: f,
  testUnsignedShort: f,
  testUnsignedLong: f,
  testUnsignedLongLong: f,
  testFloat: f,
  testDouble: f,
  testChar: f,
  testString: f,
  testWchar: f,
  testWstring: f,
  testDOMString: f,
  testAString: f,
  testAUTF8String: f,
  testACString: f,
  testJsval: f
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([TestParams]);
