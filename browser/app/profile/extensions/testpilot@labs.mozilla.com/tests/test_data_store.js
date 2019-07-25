EXPORTED_SYMBOLS = ["runAllTests"];
var Cu = Components.utils;
var testsRun = 0;
var testsPassed = 0;

function cheapAssertEqual(a, b, errorMsg) {
  testsRun += 1;
  if (a == b) {
    dump("UNIT TEST PASSED.\n");
    testsPassed += 1;
  } else {
    dump("UNIT TEST FAILED: ");
    dump(errorMsg + "\n");
    dump(a + " does not equal " + b + "\n");
  }
}

function cheapAssertEqualArrays(a, b, errorMsg) {
  testsRun += 1;
  let equal = true;
  if (a.length != b.length) {
    equal = false;
  } else {
    for (let i = 0; i < a.length; i++) {
      if (a[i] != b[i]) {
        equal = false;
      }
    }
  }
  if (equal) {
    dump("UNIT TEST PASSED.\n");
    testsPassed += 1;
  } else {
    dump("UNIT TEST FAILED: ");
    dump(errorMsg + "\n");
    dump(a + " does not equal " + b + "\n");
  }
}

function cheapAssertRange(lowerBound, value, upperBound, errorMsg) {
  testsRun += 1;
  if (lowerBound <= value && value <= upperBound) {
    dump("UNIT TEST PASSED.\n");
    testsPassed += 1;
  } else {
    dump("UNIT TEST FAILED: ");
    dump(errorMsg + "\n");
    dump(value + " is outside the range of " + lowerBound + " to "
         + upperBound + "\n");
  }
}

function cheapAssertFail(errorMsg) {
  testsRun += 1;
  dump("UNIT TEST FAILED: ");
  dump(errorMsg + "\n");
}

function testFirefoxVersionCheck() {
  Cu.import("resource://testpilot/modules/setup.js");
  cheapAssertEqual(true, TestPilotSetup._isNewerThanFirefox("4.0"));
  cheapAssertEqual(false, TestPilotSetup._isNewerThanFirefox("3.5"));
  cheapAssertEqual(false, TestPilotSetup._isNewerThanFirefox("3.6"));
}

function testStringSanitizer() {
  Cu.import("resource://testpilot/modules/string_sanitizer.js");
  var evilString = "I *have* (evil) ^characters^ [hahaha];";
  dump("Sanitized evil string is " + sanitizeString(evilString) + "\n");
  cheapAssertEqual(sanitizeString(evilString),
                   "I ?have? ?evil? ?characters? ?hahaha??");
}

function testTheDataStore() {
  
  Cu.import("resource://testpilot/modules/experiment_data_store.js");

  var columns =  [{property: "prop_a", type: TYPE_INT_32, displayName: "Length"},
                  {property: "prop_b", type: TYPE_INT_32, displayName: "Type",
                   displayValue: ["Spam", "Egg", "Sausage", "Baked Beans"]},
                  {property: "prop_c", type: TYPE_DOUBLE, displayName: "Depth"},
                  {property: "prop_s", type: TYPE_STRING, displayName: "Text"}
                  ];

  var fileName = "testpilot_storage_unit_test.sqlite";
  var tableName = "testpilot_storage_unit_test";
  var storedCount = 0;
  var store = new ExperimentDataStore(fileName, tableName, columns);


  store.storeEvent({prop_a: 13, prop_b: 3, prop_c: 0.001, prop_s: "How"},
                   function(stored) {
                     storedCount++;
                     if (storedCount == 4) {
                       _testTheDataStore(store);
                     }
                   });
  store.storeEvent({prop_a: 26, prop_b: 2, prop_c: 0.002, prop_s: " do"},
                    function(stored) {
                     storedCount++;
                     if (storedCount == 4) {
                       _testTheDataStore(store);
                     }
                   });
  store.storeEvent({prop_a: 39, prop_b: 1, prop_c: 0.003, prop_s: " you"},
                    function(stored) {
                     storedCount++;
                     if (storedCount == 4) {
                       _testTheDataStore(store);
                     }
                   });
  store.storeEvent({prop_a: 52, prop_b: 0, prop_c: 0.004, prop_s: " do?"},
                    function(stored) {
                     storedCount++;
                     if (storedCount == 4) {
                       _testTheDataStore(store);
                     }
                   });


  function _testTheDataStore(store) {
    cheapAssertEqualArrays(store.getHumanReadableColumnNames(), ["Length", "Type", "Depth", "Text"],
                   "Human readable column names are not correct.");
    cheapAssertEqualArrays(store.getPropertyNames(), ["prop_a", "prop_b", "prop_c", "prop_s"],
                        "Property names are not correct.");
    store.getAllDataAsJSON(false, function(json) {
      var expectedJson = [{prop_a: 13, prop_b: 3, prop_c: 0.001, prop_s: "How"},
                      {prop_a: 26, prop_b: 2, prop_c: 0.002, prop_s: " do"},
                       {prop_a: 39, prop_b: 1, prop_c: 0.003, prop_s: " you"},
                      {prop_a: 52, prop_b: 0, prop_c: 0.004, prop_s: " do?"}];
      cheapAssertEqual(JSON.stringify(json),
                       JSON.stringify(expectedJson),
                       "Stringified JSON does not match expectations.");

      _testTheDataStore2(store);
    });
  }

  function _testTheDataStore2(store) {
    
    store.getAllDataAsJSON(true, function(json) {
      var expectedJson = [{prop_a: 13, prop_b: "Baked Beans", prop_c: 0.001, prop_s: "How"},
                      {prop_a: 26, prop_b: "Sausage", prop_c: 0.002, prop_s: " do"},
                      {prop_a: 39, prop_b: "Egg", prop_c: 0.003, prop_s: " you"},
                      {prop_a: 52, prop_b: "Spam", prop_c: 0.004, prop_s: " do?"}];
      cheapAssertEqual(JSON.stringify(json),
                       JSON.stringify(expectedJson),
                       "JSON with human-radable values does not match expectations.");
      _testDataStoreWipe(store);
    });
  }

  function _testDataStoreWipe(store) {
    
    store.wipeAllData(function(wiped) {
      store.getAllDataAsJSON(false, function(json) {
        var expectedJson = [];
        cheapAssertEqual(JSON.stringify(json),
                         JSON.stringify(expectedJson),
                        "JSON after wipe fails to be empty.");
      });
    });
  }
};


function testTheCuddlefishPreferencesFilesystem() {

  
  
  
  
  var Cuddlefish = {};
  Cu.import("resource://testpilot/modules/lib/cuddlefish.js",
                          Cuddlefish);
  let cfl = new Cuddlefish.Loader({rootPaths: ["resource://testpilot/modules/",
                                               "resource://testpilot/modules/lib/"]});
  let remoteLoaderModule = cfl.require("remote-experiment-loader");
  let prefName = "extensions.testpilot.unittest.prefCodeStore";
  let pfs = new remoteLoaderModule.PreferencesStore(prefName);
  let contents1 = "function foo(x, y) { return x * y; }";
  let contents2 = "function bar(x, y) { return x / y; }";

  let earlyBoundDate = new Date();
  pfs.setFile("foo.js", contents1);
  pfs.setFile("bar.js", contents2);
  let lateBoundDate = new Date();

  let path = pfs.resolveModule("/", "foo.js");
  cheapAssertEqual(path, "foo.js", "resolveModule does not return expected path.");
  path = pfs.resolveModule("/", "bar.js");
  cheapAssertEqual(path, "bar.js", "resolveModule does not return expected path.");
  path = pfs.resolveModule("/", "baz.js");
  cheapAssertEqual(path, null, "Found a match for nonexistent file.");

  let file = pfs.getFile("foo.js");
  cheapAssertEqual(file.contents, contents1, "File contents do not match.");
  cheapAssertRange(earlyBoundDate, pfs.getFileModifiedDate("foo.js"), lateBoundDate,
                   "File modified date not correct.");

  file = pfs.getFile("bar.js");
  cheapAssertEqual(file.contents, contents2, "File contents do not match.");
  cheapAssertRange(earlyBoundDate, pfs.getFileModifiedDate("bar.js"), lateBoundDate,
                   "File modified date not correct.");

  delete pfs;
  let pfs2 = new remoteLoaderModule.PreferencesStore(prefName);

  file = pfs2.getFile("foo.js");
  cheapAssertEqual(file.contents, contents1, "File contents do not match after reloading.");
  file = pfs2.getFile("bar.js");
  cheapAssertEqual(file.contents, contents2, "File contents do not match after reloading.");

  let contents3 = "function foo(x, y) { return (x + y)/2; }";

  pfs2.setFile("foo.js", contents3);
  file = pfs2.getFile("foo.js");
  cheapAssertEqual(file.contents, contents3, "File contents do not newly set contents.");
}

function testRemoteLoader() {
  
  
  var Cuddlefish = {};
  Cu.import("resource://testpilot/modules/lib/cuddlefish.js",
                          Cuddlefish);
  let cfl = new Cuddlefish.Loader({rootPaths: ["resource://testpilot/modules/",
                                               "resource://testpilot/modules/lib/"]});
  let remoteLoaderModule = cfl.require("remote-experiment-loader");

  var indexJson = '{"experiments": [{"name": "Foo Study", '
                  + '"filename": "foo.js"}]}';

  var theRemoteFile = "exports.foo = function(x, y) { return x * y; }";

  let getFileFunc = function(url, callback) {
    if (url.indexOf("index.json") > -1) {
      if (indexJson != "") {
        callback(indexJson);
      } else {
        callback(null);
      }
    } else if (url.indexOf("foo.js") > -1) {
      callback(theRemoteFile);
    } else {
      callback(null);
    }
  };

  let remoteLoader = new remoteLoaderModule.RemoteExperimentLoader(getFileFunc);

  remoteLoader.checkForUpdates(function(success) {
    if (success) {
      let foo = remoteLoader.getExperiments()["foo.js"];
      cheapAssertEqual(foo.foo(6, 7), 42, "Foo doesn't work.");
    } else {
      cheapAssertFail("checkForUpdates returned failure.");
    }

    




    theRemoteFile = "exports.foo = function(x, y) { return x + y; }";
    remoteLoader.checkForUpdates( function(success) {
      if (success) {
         let foo = remoteLoader.getExperiments()["foo.js"];
        cheapAssertEqual(foo.foo(6, 7), 13, "2nd version of Foo doesn't work.");
      } else {
        cheapAssertFail("checkForUpdates 2nd time returned failure.");
      }
      

      indexJson = "";
      remoteLoader.checkForUpdates( function(success) {
        cheapAssertEqual(success, false, "3rd check for updates should have failed.");
        let foo = remoteLoader.getExperiments()["foo.js"];
        cheapAssertEqual(foo.foo(6, 7), 13, "Should still have the 2nd version of Foo.");
      });
    });
  });
}

function testRemotelyLoadTabsExperiment() {

  
  
  
}

function runAllTests() {
  testTheDataStore();
  testFirefoxVersionCheck();
  testStringSanitizer();
  
  
  dump("TESTING COMPLETE.  " + testsPassed + " out of " + testsRun +
       " tests passed.");
}








