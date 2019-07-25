



let SocialService = Components.utils.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

function ensureSocialEnabled() {
  let initiallyEnabled = SocialService.enabled;
  SocialService.enabled = true;
  registerCleanupFunction(function () {
    SocialService.enabled = initiallyEnabled;
  });
}









function runTests(tests, cbPreTest, cbPostTest) {
  waitForExplicitFinish();
  let testIter = Iterator(tests);

  if (cbPreTest === undefined) {
    cbPreTest = function(cb) {cb()};
  }
  if (cbPostTest === undefined) {
    cbPostTest = function(cb) {cb()};
  }

  let runNextTest = function() {
    let name, func;
    try {
      [name, func] = testIter.next();
    } catch (err if err instanceof StopIteration) {
      
      finish();
      return;
    }
    
    
    window.setTimeout(function() {
      function cleanupAndRunNextTest() {
        info("sub-test " + name + " complete");
        cbPostTest(runNextTest);
      }
      cbPreTest(function() {
        info("sub-test " + name + " starting");
        try {
          func.call(tests, cleanupAndRunNextTest);
        } catch (ex) {
          ok(false, "sub-test " + name + " failed: " + ex.toString() +"\n"+ex.stack);
          cleanupAndRunNextTest();
        }
      })
    }, 0)
  }
  runNextTest();
}
