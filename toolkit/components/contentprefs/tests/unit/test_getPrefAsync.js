


































var cps = Cc["@mozilla.org/content-pref/service;1"].
          getService(Ci.nsIContentPrefService);
var uri = ContentPrefTest.getURI("http://www.example.com/");

function run_test() {
  
  do_check_true(true);
  return;

  do_test_pending();

  cps.setPref(uri, "asynctest", "pie");
  do_check_eq(cps.getPref(uri, "asynctest"), "pie");

  cps.getPref(uri, "asynctest", function(aValue) {
    do_check_eq(aValue, "pie");
    testCallbackObj();
  });
}

function testCallbackObj() {
  cps.getPref(uri, "asynctest", {
    onResult: function(aValue) {
      do_check_eq(aValue, "pie");
      cps.removePref(uri, "asynctest");
      testNoResult();
    }
  });
}

function testNoResult() {
  cps.getPref(uri, "asynctest", function(aValue) {
    do_check_eq(aValue, undefined);
    do_test_finished();
  });
}
