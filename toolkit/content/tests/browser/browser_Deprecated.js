



const Ci = Components.interfaces;
const Cu = Components.utils;
const PREF_DEPRECATION_WARNINGS = "devtools.errorconsole.deprecation_warnings";

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/Deprecated.jsm", this);



function basicDeprecatedFunction () {
  Deprecated.warning("this method is deprecated.", "http://example.com");
  return true;
}

function deprecationFunctionBogusCallstack () {
  Deprecated.warning("this method is deprecated.", "http://example.com", {
    caller: {}
  });
  return true;
}

function deprecationFunctionCustomCallstack () {
  
  function getStack () {
    return Components.stack;
  }
  Deprecated.warning("this method is deprecated.", "http://example.com",
    getStack());
  return true;
}

let tests = [

{
  deprecatedFunction: basicDeprecatedFunction,
  expectedObservation: function (aMessage) {
    testAMessage(aMessage);
    ok(aMessage.errorMessage.indexOf("basicDeprecatedFunction") > 0,
      "Callstack is correctly logged.");
  }
},

{
  deprecatedFunction: function () {
    Deprecated.warning("this method is deprecated.");
    return true;
  },
  expectedObservation: function (aMessage) {
    ok(aMessage.errorMessage.indexOf("must provide a URL") > 0,
      "Deprecation warning logged an empty URL argument.");
  }
},


{
  deprecatedFunction: deprecationFunctionBogusCallstack,
  expectedObservation: function (aMessage) {
    testAMessage(aMessage);
    ok(aMessage.errorMessage.indexOf("deprecationFunctionBogusCallstack") > 0,
      "Callstack is correctly logged.");
  }
},

{
  deprecatedFunction: basicDeprecatedFunction,
  expectedObservation: null,
  
  logWarnings: false
},

{
  deprecatedFunction: deprecationFunctionCustomCallstack,
  expectedObservation: function (aMessage) {
    testAMessage(aMessage);
    ok(aMessage.errorMessage.indexOf("deprecationFunctionCustomCallstack") > 0,
      "Callstack is correctly logged.");
  },
  
  logWarnings: true
}];


let idx = -1;

function test() {
  waitForExplicitFinish();

  
  ok(Deprecated, "Deprecated object exists");

  nextTest();
}


function testAMessage (aMessage) {
  ok(aMessage.errorMessage.indexOf("DEPRECATION WARNING: " +
    "this method is deprecated.") === 0,
    "Deprecation is correctly logged.");
  ok(aMessage.errorMessage.indexOf("http://example.com") > 0,
    "URL is correctly logged.");
}

function nextTest() {
  idx++;

  if (idx == tests.length) {
    finish();
    return;
  }

  info("Running test #" + idx);
  let test = tests[idx];

  
  if (typeof test.logWarnings !== "undefined") {
    Services.prefs.setBoolPref(PREF_DEPRECATION_WARNINGS, test.logWarnings);
  }

  
  let consoleListener = {
    observe: function (aMessage) {
      
      if (!(aMessage instanceof Ci.nsIScriptError)) {
        return;
      }
      if (aMessage.errorMessage.indexOf("DEPRECATION WARNING: ") < 0 &&
          aMessage.errorMessage.indexOf("must provide a URL") < 0) {
        return;
      }
      ok(aMessage instanceof Ci.nsIScriptError,
        "Deprecation log message is an instance of type nsIScriptError.");


      if (test.expectedObservation === null) {
        ok(false, "Deprecated warning not expected");
      }
      else {
        test.expectedObservation(aMessage);
      }

      Services.console.unregisterListener(consoleListener);
      executeSoon(nextTest);
    }
  };
  Services.console.registerListener(consoleListener);
  test.deprecatedFunction();
  if (test.expectedObservation === null) {
    executeSoon(function() {
      Services.console.unregisterListener(consoleListener);
      executeSoon(nextTest);
    });
  }
}
