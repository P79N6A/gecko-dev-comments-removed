var CameraTest = (function() {
  'use strict';

  

















  const PREF_TEST_ENABLED = "camera.control.test.enabled";
  const PREF_TEST_HARDWARE = "camera.control.test.hardware";
  var oldTestEnabled;
  var oldTestHw;
  var testMode;

  function testHardwareSet(test, callback) {
    SpecialPowers.pushPrefEnv({'set': [[PREF_TEST_HARDWARE, test]]}, function() {
      var setTest = SpecialPowers.getCharPref(PREF_TEST_HARDWARE);
      ise(setTest, test, "Test subtype set to " + setTest);
      if (callback) {
        callback(setTest);
      }
    });
  }

  function testHardwareDone(callback) {
    testMode = null;
    if (oldTestHw) {
      SpecialPowers.pushPrefEnv({'set': [[PREF_TEST_HARDWARE, oldTestHw]]}, callback);
    } else {
      SpecialPowers.pushPrefEnv({'clear': [[PREF_TEST_HARDWARE]]}, callback);
    }
  }

  function testBegin(mode, callback) {
    SimpleTest.waitForExplicitFinish();
    try {
      oldTestEnabled = SpecialPowers.getCharPref(PREF_TEST_ENABLED);
    } catch(e) { }
    SpecialPowers.pushPrefEnv({'set': [[PREF_TEST_ENABLED, mode]]}, function() {
      var setMode = SpecialPowers.getCharPref(PREF_TEST_ENABLED);
      ise(setMode, mode, "Test mode set to " + setMode);
      if (setMode === "hardware") {
        try {
          oldTestHw = SpecialPowers.getCharPref(PREF_TEST_HARDWARE);
        } catch(e) { }
        testMode = {
          set: testHardwareSet,
          done: testHardwareDone
        };
        if (callback) {
          callback(testMode);
        }
      }
    });
  }

  function testEnd(callback) {
    function allDone(cb) {
      function cb2() {
        SimpleTest.finish();
        if (cb) {
          cb();
        }
      }
      if (oldTestEnabled) {
        SpecialPowers.pushPrefEnv({'set': [[PREF_TEST_ENABLED, oldTestEnabled]]}, cb2);
      } else {
        SpecialPowers.pushPrefEnv({'clear': [[PREF_TEST_ENABLED]]}, cb2);
      }
    }

    if (testMode) {
      testMode.done(function() {
        allDone(callback);
      });
      testMode = null;
    } else {
      allDone(function() {
        if (callback) {
          callback();
        }
      });
    }
  }

  ise(SpecialPowers.sanityCheck(), "foo", "SpecialPowers passed sanity check");
  return {
    begin: testBegin,
    end: testEnd
  };

})();
