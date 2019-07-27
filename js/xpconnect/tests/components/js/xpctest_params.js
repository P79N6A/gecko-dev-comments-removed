


Components.utils.import("resource:///modules/XPCOMUtils.jsm");

function TestParams() {
}


function f(a, b) {
    var rv = b.value;
    b.value = a;
    return rv;
};


function f_is(aIs, a, bIs, b, rvIs) {

    
    var rv = b.value;
    rvIs.value = bIs.value;

    
    b.value = a;
    bIs.value = aIs;

    return rv;
}

function f_size_and_iid(aSize, aIID, a, bSize, bIID, b, rvSize, rvIID) {

    
    rvIID.value = bIID.value;
    bIID.value = aIID;

    
    return f_is(aSize, a, bSize, b, rvSize);
}

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
  testJsval: f,
  testShortArray: f_is,
  testDoubleArray: f_is,
  testStringArray: f_is,
  testWstringArray: f_is,
  testInterfaceArray: f_is,
  testSizedString: f_is,
  testSizedWstring: f_is,
  testInterfaceIs: f_is,
  testInterfaceIsArray: f_size_and_iid,
  testOutAString: function(o) { o.value = "out"; }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([TestParams]);
