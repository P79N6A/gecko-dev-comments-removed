const isWinXP = navigator.userAgent.indexOf("Windows NT 5.1") != -1;
const isOSXLion = navigator.userAgent.indexOf("Mac OS X 10.7") != -1;



if (window.opener) {
  
  var testName = location.pathname.split('/').pop();

  
  window.ok = function(a, msg) {
    opener.ok(a, testName + ": " + msg);
  };

  window.is = function(a, b, msg) {
    opener.is(a, b, testName + ": " + msg);
  };

  window.isnot = function(a, b, msg) {
    opener.isnot(a, b, testName + ": " + msg);
  };

  window.todo = function(a, msg) {
    opener.todo(a, testName + ": " + msg);
  };

  window.todo_is = function(a, b, msg) {
    opener.todo_is(a, b, testName + ": " + msg);
  };

  window.todo_isnot = function(a, b, msg) {
    opener.todo_isnot(a, b, testName + ": " + msg);
  };

  
  var SimpleTest = SimpleTest || {};

  SimpleTest.waitForExplicitFinish = function() {
    dump("[POINTERLOCK] Starting " + testName+ "\n");
  };

  SimpleTest.finish = function () {
    dump("[POINTERLOCK] Finishing " + testName+ "\n");
    opener.nextTest();
  };
} else {
  
  

  
  SpecialPowers.setBoolPref("full-screen-api.enabled", true);

  
  SpecialPowers.setBoolPref("full-screen-api.allow-trusted-requests-only", false);
}

addLoadEvent(function() {
  if (typeof start !== 'undefined') {
    SimpleTest.waitForFocus(start);
  }
});

