



let SocialService = Components.utils.import("resource://gre/modules/SocialService.jsm", {}).SocialService;
let MockRegistrar = Components.utils.import("resource://testing-common/MockRegistrar.jsm", {}).MockRegistrar;









function runTests(tests, cbPreTest, cbPostTest, cbFinish) {
  let testIter = (function*() {
    for (let name in tests) {
      if (tests.hasOwnProperty(name)) {
        yield [name, tests[name]];
      }
    }
  })();

  if (cbPreTest === undefined) {
    cbPreTest = function(cb) {cb()};
  }
  if (cbPostTest === undefined) {
    cbPostTest = function(cb) {cb()};
  }

  function runNextTest() {
    let result = testIter.next();
    if (result.done) {
      
      (cbFinish || finish)();
      return;
    }
    let [name, func] = result.value;
    
    
    executeSoon(function() {
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
    });
  }
  runNextTest();
}




const ALERTS_SERVICE_CONTRACT_ID = "@mozilla.org/alerts-service;1";

function MockAlertsService() {}

MockAlertsService.prototype = {

    showAlertNotification: function(imageUrl, title, text, textClickable,
                                    cookie, alertListener, name) {
        let obData = JSON.stringify({
          imageUrl: imageUrl,
          title: title,
          text:text,
          textClickable: textClickable,
          cookie: cookie,
          name: name
        });
        Services.obs.notifyObservers(null, "social-test:notification-alert", obData);

        if (textClickable) {
          
          alertListener.observe(null, "alertclickcallback", cookie);
        }

        alertListener.observe(null, "alertfinished", cookie);
    },

    QueryInterface: function(aIID) {
        if (aIID.equals(Ci.nsISupports) ||
            aIID.equals(Ci.nsIAlertsService))
            return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
    }
};

let originalAlertsServiceCID;
function replaceAlertsService() {
  originalAlertsServiceCID =
    MockRegistrar.register(ALERTS_SERVICE_CONTRACT_ID, MockAlertsService);
}

function restoreAlertsService() {
  MockRegistrar.unregister(originalAlertsServiceCID);
}

