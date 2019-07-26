var NotificationTest = (function () {
  "use strict";

  function info(msg, name) {
    SimpleTest.info("::Notification Tests::" + (name || ""), msg);
  }

  function setup_testing_env() {
    SimpleTest.waitForExplicitFinish();
    
    SpecialPowers.setBoolPref("notification.prompt.testing", true);
  }

  function teardown_testing_env() {
    SimpleTest.finish();
  }

  function executeTests(tests, callback) {
    
    
    var context = {};

    (function executeRemainingTests(remainingTests) {
      if (!remainingTests.length) {
        return callback();
      }

      var nextTest = remainingTests.shift();
      var finishTest = executeRemainingTests.bind(null, remainingTests);
      var startTest = nextTest.call.bind(nextTest, context, finishTest);

      try {
        startTest();
        
        
        if (nextTest.length === 0) {
          finishTest();
        }
      } catch (e) {
        ok(false, "Test threw exception!");
        finishTest();
      }
    })(tests);
  }

  
  return {
    run: function (tests, callback) {
      setup_testing_env();

      addLoadEvent(function () {
        executeTests(tests, function () {
          teardown_testing_env();
          callback && callback();
        });
      });
    },

    allowNotifications: function () {
      SpecialPowers.setBoolPref("notification.prompt.testing.allow", true);
    },

    denyNotifications: function () {
      SpecialPowers.setBoolPref("notification.prompt.testing.allow", false);
    },

    clickNotification: function (notification) {
      
    },

    info: info
  };
})();
