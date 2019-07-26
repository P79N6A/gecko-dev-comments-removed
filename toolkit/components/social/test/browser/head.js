



let SocialService = Components.utils.import("resource://gre/modules/SocialService.jsm", {}).SocialService;

function ensureSocialEnabled() {
  let initiallyEnabled = SocialService.enabled;
  SocialService.enabled = true;
  registerCleanupFunction(function () {
    SocialService.enabled = initiallyEnabled;
  });
}









function runTests(tests, cbPreTest, cbPostTest, cbFinish) {
  let testIter = Iterator(tests);

  if (cbPreTest === undefined) {
    cbPreTest = function(cb) {cb()};
  }
  if (cbPostTest === undefined) {
    cbPostTest = function(cb) {cb()};
  }

  function runNextTest() {
    let name, func;
    try {
      [name, func] = testIter.next();
    } catch (err if err instanceof StopIteration) {
      
      (cbFinish || finish)();
      return;
    }
    
    
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



const FAKE_CID = Cc["@mozilla.org/uuid-generator;1"].
    getService(Ci.nsIUUIDGenerator).generateUUID();

const ALERTS_SERVICE_CONTRACT_ID = "@mozilla.org/alerts-service;1";
const ALERTS_SERVICE_CID = Components.ID(Cc[ALERTS_SERVICE_CONTRACT_ID].number);

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

var factory = {
    createInstance: function(aOuter, aIID) {
        if (aOuter != null)
            throw Cr.NS_ERROR_NO_AGGREGATION;
        return new MockAlertsService().QueryInterface(aIID);
    }
};

function replaceAlertsService() {
  Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
            .registerFactory(FAKE_CID, "",
                             ALERTS_SERVICE_CONTRACT_ID,
                             factory)
}

function restoreAlertsService() {
  Components.manager.QueryInterface(Ci.nsIComponentRegistrar)
            .registerFactory(ALERTS_SERVICE_CID, "",
                             ALERTS_SERVICE_CONTRACT_ID,
                             null);
}

