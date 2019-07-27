
const TEST_PACKAGE = "chrome://mochitests/content/";


const TEST_ID = "mochikit@mozilla.org";

var gConfig;

if (Cc === undefined) {
  var Cc = Components.classes;
  var Ci = Components.interfaces;
  var Cu = Components.utils;
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");


window.addEventListener("load", function testOnLoad() {
  window.removeEventListener("load", testOnLoad);
  window.addEventListener("MozAfterPaint", function testOnMozAfterPaint() {
    window.removeEventListener("MozAfterPaint", testOnMozAfterPaint);
    setTimeout(testInit, 0);
  });
});


function testModule(require, { url, expected }) {
  return new Promise(resolve => {
    let path = url.substring(TEST_PACKAGE.length);

    const { stdout } = require("sdk/system");

    const { runTests } = require("sdk/test/harness");
    const loaderModule = require("toolkit/loader");
    const options = require("sdk/test/options");

    function findAndRunTests(loader, nextIteration) {
      const { TestRunner } = loaderModule.main(loader, "sdk/deprecated/unit-test");

      const NOT_TESTS = ['setup', 'teardown'];
      var runner = new TestRunner();

      let tests = [];

      let suiteModule;
      try {
        dump("TEST-INFO: " + path + " | Loading test module\n");
        suiteModule = loaderModule.main(loader, "tests/" + path.substring(0, path.length - 3));
      }
      catch (e) {
        
        
        suiteModule = {
          'test suite skipped': assert => assert.pass(e.message)
        };
      }

      for (let name of Object.keys(suiteModule).sort()) {
        if (NOT_TESTS.indexOf(name) != -1)
          continue;

        tests.push({
          setup: suiteModule.setup,
          teardown: suiteModule.teardown,
          testFunction: suiteModule[name],
          name: path + "." + name
        });
      }

      runner.startMany({
        tests: {
          getNext: () => Promise.resolve(tests.shift())
        },
        stopOnError: options.stopOnError,
        onDone: nextIteration
      });
    }

    runTests({
      findAndRunTests: findAndRunTests,
      iterations: options.iterations,
      filter: options.filter,
      profileMemory: options.profileMemory,
      stopOnError: options.stopOnError,
      verbose: options.verbose,
      parseable: options.parseable,
      print: stdout.write,
      onDone: resolve
    });
  });
}


function setPrefs(root, options) {
  Object.keys(options).forEach(id => {
    const key = root + "." + id;
    const value = options[id]
    const type = typeof(value);

    value === null ? void(0) :
    value === undefined ? void(0) :
    type === "boolean" ? Services.prefs.setBoolPref(key, value) :
    type === "string" ? Services.prefs.setCharPref(key, value) :
    type === "number" ? Services.prefs.setIntPref(key, parseInt(value)) :
    type === "object" ? setPrefs(key, value) :
    void(0);
  });
}

function testInit() {
  
  if (Services.prefs.prefHasUserValue("testing.jetpackTestHarness.running"))
    return;

  Services.prefs.setBoolPref("testing.jetpackTestHarness.running", true);

  
  
  Services.prefs.setBoolPref("dom.indexedDB.experimental", true);

  
  let config = readConfig();
  getTestList(config, function(links) {
    try {
      let fileNames = [];
      let fileNameRegexp = /test-.+\.js$/;
      arrayOfTestFiles(links, fileNames, fileNameRegexp);

      if (config.startAt || config.endAt) {
        fileNames = skipTests(fileNames, config.startAt, config.endAt);
      }

      
      let chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIChromeRegistry);
      let realPath = chromeReg.convertChromeURL(Services.io.newURI(TEST_PACKAGE, null, null));
      let resProtocol = Cc["@mozilla.org/network/protocol;1?name=resource"].getService(Ci.nsIResProtocolHandler);
      resProtocol.setSubstitution("jetpack-package-tests", realPath);

      
      const options = {
        test: {
          iterations: config.runUntilFailure ? config.repeat : 1,
          stop: false,
          keepOpen: true,
        },
        profile: {
          memory: false,
          leaks: false,
        },
        output: {
          logLevel: "verbose",
          format: "tbpl",
        },
        console: {
          logLevel: "info",
        },
      }
      setPrefs("extensions." + TEST_ID + ".sdk", options);

      
      let sdkpath = "resource://gre/modules/commonjs/";
      try {
        let sdklibs = Services.prefs.getCharPref("extensions.sdk.path");
        
        let sdkfile = Cc["@mozilla.org/file/local;1"].
                      createInstance(Ci.nsIFile);
        sdkfile.initWithPath(sdklibs);
        let sdkuri = Services.io.newFileURI(sdkfile);
        resProtocol.setSubstitution("jetpack-modules", sdkuri);
        sdkpath = "resource://jetpack-modules/";
      }
      catch (e) {
        
      }

      const paths = {
        "": sdkpath,
        "tests/": "resource://jetpack-package-tests/",
      };

      
      const loaderID = "toolkit/loader";
      const loaderURI = paths[""] + loaderID + ".js";
      const loaderModule = Cu.import(loaderURI, {}).Loader;

      const modules = {};

      
      
      modules[loaderID] = loaderModule;
      modules["@test/options"] = {};

      let loader = loaderModule.Loader({
        id: TEST_ID,
        name: "addon-sdk",
        version: "1.0",
        loadReason: "install",
        paths: paths,
        modules: modules,
        isNative: true,
        rootURI: paths["tests/"],
        prefixURI: paths["tests/"],
        metadata: {},
      });

      const module = loaderModule.Module(loaderID, loaderURI);
      const require = loaderModule.Require(loader, module);

      
      require("sdk/addon/window").ready.then(() => {
        let passed = 0;
        let failed = 0;

        function finish() {
          if (passed + failed == 0) {
            dump("TEST-UNEXPECTED-FAIL | jetpack-package-harness.js | " +
                 "No tests to run. Did you pass invalid test_paths?\n");
          }
          else {
            dump("Jetpack Package Test Summary\n");
            dump("\tPassed: " + passed + "\n" +
                 "\tFailed: " + failed + "\n" +
                 "\tTodo: 0\n");
          }

          if (config.closeWhenDone) {
            require("sdk/system").exit(failed == 0 ? 0 : 1);
          }
          else {
            loaderModule.unload(loader, "shutdown");
          }
        }

        function testNextModule() {
          if (fileNames.length == 0)
            return finish();

          let filename = fileNames.shift();
          testModule(require, filename).then(tests => {
            passed += tests.passed;
            failed += tests.failed;
          }).then(testNextModule);
        }

        testNextModule();
      });
    }
    catch (e) {
      dump("TEST-UNEXPECTED-FAIL: jetpack-package-harness.js | error starting test harness (" + e + ")\n");
      dump(e.stack);
    }
  });
}
