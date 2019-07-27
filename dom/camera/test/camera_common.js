var CameraTest = (function() {
  'use strict';

  






























  const PREF_TEST_ENABLED = "camera.control.test.enabled";
  const PREF_TEST_HARDWARE = "camera.control.test.hardware";
  const PREF_TEST_EXTRA_PARAMETERS = "camera.control.test.hardware.gonk.parameters";
  const PREF_TEST_FAKE_LOW_MEMORY = "camera.control.test.is_low_memory";
  var oldTestEnabled;
  var oldTestHw;
  var testMode;

  function testHardwareSetFakeParameters(parameters, callback) {
    SpecialPowers.pushPrefEnv({'set': [[PREF_TEST_EXTRA_PARAMETERS, parameters]]}, function() {
      var setParams = SpecialPowers.getCharPref(PREF_TEST_EXTRA_PARAMETERS);
      ise(setParams, parameters, "Extra test parameters '" + setParams + "'");
      if (callback) {
        callback(setParams);
      }
    });
  }

  function testHardwareClearFakeParameters(callback) {
    SpecialPowers.pushPrefEnv({'clear': [[PREF_TEST_EXTRA_PARAMETERS]]}, callback);
  }

  function testHardwareSetFakeLowMemoryPlatform(callback) {
    SpecialPowers.pushPrefEnv({'set': [[PREF_TEST_FAKE_LOW_MEMORY, true]]}, function() {
      var setParams = SpecialPowers.getBoolPref(PREF_TEST_FAKE_LOW_MEMORY);
      ise(setParams, true, "Fake low memory platform");
      if (callback) {
        callback(setParams);
      }
    });
  }

  function testHardwareClearFakeLowMemoryPlatform(callback) {
    SpecialPowers.pushPrefEnv({'clear': [[PREF_TEST_FAKE_LOW_MEMORY]]}, callback);
  }

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
          setFakeParameters: testHardwareSetFakeParameters,
          clearFakeParameters: testHardwareClearFakeParameters,
          setFakeLowMemoryPlatform: testHardwareSetFakeLowMemoryPlatform,
          clearFakeLowMemoryPlatform: testHardwareClearFakeLowMemoryPlatform,
          done: testHardwareDone
        };
        if (callback) {
          callback(testMode);
        }
      }
    });
  }

  function testEnd(callback) {
    
    function allCleanedUp() {
      SimpleTest.finish();
      if (callback) {
        callback();
      }
    }
    function cleanUpTestEnabled() {
      var next = allCleanedUp;
      if (oldTestEnabled) {
        SpecialPowers.pushPrefEnv({'set': [[PREF_TEST_ENABLED, oldTestEnabled]]}, next);
      } else {
        SpecialPowers.pushPrefEnv({'clear': [[PREF_TEST_ENABLED]]}, next);
      }
    }
    function cleanUpTest() {
      var next = cleanUpTestEnabled;
      if (testMode) {
        testMode.done(next);
        testMode = null;
      } else {
        next();
      }
    }
    function cleanUpLowMemoryPlatform() {
      var next = cleanUpTest;
      if (testMode) {
        testMode.clearFakeLowMemoryPlatform(next);
      } else {
        next();
      }
    }
    function cleanUpExtraParameters() {
      var next = cleanUpLowMemoryPlatform;
      if (testMode) {
        testMode.clearFakeParameters(next);
      } else {
        next();
      }
    }

    cleanUpExtraParameters();
  }

  ise(SpecialPowers.sanityCheck(), "foo", "SpecialPowers passed sanity check");
  return {
    begin: testBegin,
    end: testEnd
  };

})();
