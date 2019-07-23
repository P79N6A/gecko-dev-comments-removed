



































function logResult(testname, passed) {
  dump((passed ? "passed" : "FAILED") + ":  " + testname + "\n");
}

function runTestsArray(callee, tests) {
  var succeeded = true;
  for (i in tests) {    
    var result;
    var passed = false;
    var comment = tests[i][3] ? tests[i][3] : tests[i][0];

    try {
      result = callee[tests[i][0]](tests[i][1]);
      passed = (result == tests[i][2] ? true : false);
    } catch (e) {
      succeeded = false;
      dump("*** TEST " + comment + " threw exception: " + e + "\n");
      logResult(comment, false);
      continue;
    }

    logResult(comment, passed);

    if (!passed) {
      succeeded = false;
      dump("*** TEST " + comment + " FAILED: expected " + tests[i][2] +
          ", returned " + result + "\n");
    }
  }

  if (!succeeded) {
    throw Components.results.NS_ERROR_FAILURE;
  }
}

function runEchoTests(tests) {
  var javaEcho = tests.QueryInterface(Components.interfaces.nsIEcho);

  var testString = "this is a test string";
  var emptyString = "";
  var utf8String =
        "Non-Ascii 1 byte chars: éâäàåç, 2 byte chars: \u1234 \u1235 \u1236";

  var tests = new Array(
    
    [ "In2OutOneDOMString", testString, testString,
        "In2OutOneDOMString w/ '" + testString + "'"],
    [ "In2OutOneDOMString", emptyString, emptyString,
        "In2OutOneDOMString w/ empty string" ],
    [ "In2OutOneDOMString", null, null,
        "In2OutOneDOMString w/ null value" ],
    
    [ "In2OutOneAString", testString, testString,
        "In2OutOneAString w/ '" + testString + "'" ],
    [ "In2OutOneAString", emptyString, emptyString,
        "In2OutOneAString w/ empty string" ],
    [ "In2OutOneAString", utf8String, utf8String,
        "In2OutOneAString w/ '" + utf8String + "'" ],
    [ "In2OutOneAString", null, null,
        "In2OutOneAString w/ null value" ],
    
    [ "In2OutOneUTF8String", testString, testString,
        "In2OutOneUTF8String w/ '" + testString + "'" ],
    [ "In2OutOneUTF8String", emptyString, emptyString,
        "In2OutOneUTF8String w/ empty string" ],
    [ "In2OutOneUTF8String", utf8String, utf8String,
        "In2OutOneUTF8String w/ '" + utf8String + "'" ],
    [ "In2OutOneUTF8String", null, null,
        "In2OutOneUTF8String w/ null value" ],
    
    [ "In2OutOneCString", testString, testString,
        "In2OutOneCString w/ '" + testString + "'" ],
    [ "In2OutOneCString", emptyString, emptyString,
        "In2OutOneCString w/ empty string" ],
    [ "In2OutOneCString", null, null,
        "In2OutOneCString w/ null value" ],
    
    [ "In2OutOneString", testString, testString,
        "In2OutOneString w/ '" + testString + "'" ],
    [ "In2OutOneString", emptyString, emptyString,
        "In2OutOneString w/ empty string" ],
    [ "In2OutOneString", null, null,
        "In2OutOneString w/ null value" ]
  );

  runTestsArray(javaEcho, tests);

  var passed = false;
  try {
    javaEcho.aString = testString;
    if (javaEcho.aString == testString)
      passed = true;
  } catch(e) { alert("exception: " + e); }
  logResult("set/getAString w/ '" + testString + "'", passed);

  passed = false;
  try {
    javaEcho.aString = emptyString;
    if (javaEcho.aString == emptyString)
      passed = true;
  } catch(e) { }
  logResult("set/getAString w/ empty string", passed);

  passed = false;
  try {
    javaEcho.aString = null;
    if (javaEcho.aString == null)
      passed = true;
  } catch(e) { }
  logResult("set/getAString w/ null value", passed);
}

function runInTests(tests) {
  var javaIn = tests.QueryInterface(Components.interfaces.nsIXPCTestIn);

  var testString = "this is a test string";
  var emptyString = "";
  var utf8String =
        "Non-Ascii 1 byte chars: éâäàåç, 2 byte chars: \u1234 \u1235 \u1236";

  var minInt = -Math.pow(2,31);
  var maxInt = Math.pow(2,31) - 1;
  var maxUInt = Math.pow(2,32) - 1;
  var minShort = -Math.pow(2,15);
  var maxShort = Math.pow(2,15) - 1;
  var maxUShort = Math.pow(2,16) - 1;
  var charA = 'a';
  var charZ = 'Z';
  var charWide = '\u1234';
  var maxOctet = Math.pow(2,8) - 1;


  
  
  var lowLong = -9223372036854775000;
  var highLong = 9223372036854775000;
  var minFloat = 1.40129846432481707e-45;
  var maxFloat = 3.40282346638528860e+38;
  var minDouble = 2.2250738585072014e-308;
  var maxDouble = 1.7976931348623157e+308;

  tests = new Array(
    [ "EchoLong", minInt, minInt, "EchoLong w/ " + minInt ],
    [ "EchoLong", maxInt, maxInt, "EchoLong w/ " + maxInt ],
    [ "EchoShort", minShort, minShort, "EchoShort w/ " + minShort ],
    [ "EchoShort", maxShort, maxShort, "EchoShort w/ " + maxShort ],
    [ "EchoChar", charA, charA, "EchoChar w/ " + charA ],
    [ "EchoChar", charZ, charZ, "EchoChar w/ " + charZ ],
    [ "EchoBoolean", true, true, "EchoBoolean w/ " + true ],
    [ "EchoBoolean", false, false, "EchoBoolean w/ " + false ],
    [ "EchoOctet", 0, 0, "EchoOctet w/ " + 0 ],
    [ "EchoOctet", maxOctet, maxOctet, "EchoOctet w/ " + maxOctet ],
    [ "EchoLongLong", lowLong, lowLong, "EchoLongLong w/ " + lowLong ],
    [ "EchoLongLong", highLong, highLong, "EchoLongLong w/ " + highLong ],
    [ "EchoUnsignedShort", maxUShort, maxUShort,
        "EchoUnsignedShort w/ " + maxUShort ],
    [ "EchoUnsignedShort", 0, 0, "EchoUnsignedShort w/ zero" ],
    
    [ "EchoUnsignedShort", -1, maxUShort, "EchoUnsignedShort w/ -1" ],
    [ "EchoUnsignedLong", maxUInt, maxUInt, "EchoUnsignedLong w/ " + maxUInt ],
    [ "EchoUnsignedLong", 0, 0, "EchoUnsignedLong w/ zero" ],
    [ "EchoUnsignedLong", -1, maxUInt, "EchoUnsignedLong w/ -1" ],
    [ "EchoFloat", minFloat, minFloat, "EchoFloat w/ " + minFloat ],
    [ "EchoFloat", maxFloat, maxFloat, "EchoFloat w/ " + maxFloat ],
    [ "EchoDouble", minDouble, minDouble, "EchoDouble w/ " + minDouble ],
    [ "EchoDouble", maxDouble, maxDouble, "EchoDouble w/ " + maxDouble ],
    [ "EchoWchar", charA, charA, "EchoWchar w/ " + charA ],
    [ "EchoWchar", charZ, charZ, "EchoWchar w/ " + charZ ],
    [ "EchoWchar", charWide, charWide, "EchoWchar w/ " + charWide ],
    [ "EchoString", testString, testString,
        "EchoString w/ '" + testString + "'" ],
    [ "EchoString", emptyString, emptyString, "EchoString w/ empty string" ],
    [ "EchoString", utf8String, utf8String,
        "EchoString w/ '" + utf8String + "'" ],
    [ "EchoString", null, null, "EchoString w/ null value" ]
  );

  runTestsArray(javaIn, tests);




  var highULong = 18446744073709550000;

  tests = new Array(
    [ "EchoPRBool", false, false, "EchoPRBool w/ false" ],
    [ "EchoPRBool", true, true, "EchoPRBool w/ true" ],
    [ "EchoPRInt32", minInt, minInt, "EchoPRInt32 w/ " + minInt ],
    [ "EchoPRInt32", maxInt, maxInt, "EchoPRInt32 w/ " + maxInt ],
    [ "EchoPRInt16", minShort , minShort, "EchoPRInt16 w/ " + minShort ],
    [ "EchoPRInt16", maxShort, maxShort, "EchoPRInt16 w/ " + maxShort ],
    [ "EchoPRInt64", lowLong, lowLong, "EchoPRInt64 w/ " + lowLong ],
    [ "EchoPRInt64", highLong, highLong, "EchoPRInt64 w/ " + highLong ],
    [ "EchoPRUint8", maxOctet, maxOctet, "EchoPRUint8 w/ " + maxOctet ],
    [ "EchoPRUint8", 0, 0, "EchoPRUint8 w/ zero" ],
    [ "EchoPRUint8", -1, maxOctet, "EchoPRUint8 w/ -1" ],
    [ "EchoPRUint16", maxUShort, maxUShort, "EchoPRUint16 w/ " + maxUShort ],
    [ "EchoPRUint16", 0, 0, "EchoPRUint16 w/ zero" ],
    [ "EchoPRUint16", -1, maxUShort, "EchoPRUint16 w/ -1" ],
    [ "EchoPRUint32", maxUInt, maxUInt, "EchoPRUint32 w/ " + maxUInt ],
    [ "EchoPRUint32", 0, 0, "EchoPRUint32 w/ zero" ],
    [ "EchoPRUint32", -1, maxUInt, "EchoPRUint32 w/ -1" ],
  


    [ "EchoPRUint64", 0, 0, "EchoPRUint64 w/ zero" ]
  );

  runTestsArray(javaIn, tests);
}





const nsIJXTestParams = Components.interfaces.nsIJXTestParams;
const nsISupports = Components.interfaces.nsISupports;

const CLASS_ID = Components.ID("{f0882957-bcc1-4854-a2cb-94051bad4193}");
const CLASS_NAME = "JavaXPCOM Test Params Javascript XPCOM Component";
const CONTRACT_ID = "@mozilla.org/javaxpcom/tests/params;1";






function TestParams() {
};


TestParams.prototype = {

  
  runTests: function(tests) {
    runEchoTests(tests);
    runInTests(tests);
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(nsIJXTestParams) &&    
        !aIID.equals(nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};




var TestParamsFactory = {
  createInstance: function (aOuter, aIID)
  {
    if (aOuter != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return (new TestParams()).QueryInterface(aIID);
  }
};




var TestParamsModule = {
  _firstTime: true,
  registerSelf: function(aCompMgr, aFileSpec, aLocation, aType)
  {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.registerFactoryLocation(CLASS_ID, CLASS_NAME, 
        CONTRACT_ID, aFileSpec, aLocation, aType);
  },

  unregisterSelf: function(aCompMgr, aLocation, aType)
  {
    aCompMgr = aCompMgr.
        QueryInterface(Components.interfaces.nsIComponentRegistrar);
    aCompMgr.unregisterFactoryLocation(CLASS_ID, aLocation);        
  },
  
  getClassObject: function(aCompMgr, aCID, aIID)
  {
    if (!aIID.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    if (aCID.equals(CLASS_ID))
      return TestParamsFactory;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  canUnload: function(aCompMgr)
  {
    return true;
  }
};




function NSGetModule(aCompMgr, aFileSpec)
{
  return TestParamsModule;
}
