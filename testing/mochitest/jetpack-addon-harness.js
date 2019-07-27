
var gConfig;

if (Cc === undefined) {
  var Cc = Components.classes;
  var Ci = Components.interfaces;
  var Cu = Components.utils;
}

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Services",
  "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
  "resource://gre/modules/AddonManager.jsm");


window.addEventListener("load", function testOnLoad() {
  window.removeEventListener("load", testOnLoad);
  window.addEventListener("MozAfterPaint", function testOnMozAfterPaint() {
    window.removeEventListener("MozAfterPaint", testOnMozAfterPaint);
    setTimeout(testInit, 0);
  });
});

let sdkpath = null;


function realPath(chrome) {
  return chrome.substring("chrome://mochitests/content/jetpack-addon/".length)
               .replace(".xpi", "");
}


function installAddon(url) {
  return new Promise(function(resolve, reject) {
    AddonManager.getInstallForURL(url, function(install) {
      install.addListener({
        onDownloadEnded: function(install) {
          
          const options = {
            test: {
              iterations: 1,
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
          setPrefs("extensions." + install.addon.id + ".sdk", options);

          
          
          if (sdkpath) {
            let paths = {}
            for (let path of ["dev", "diffpatcher", "framescript", "method", "node", "sdk", "toolkit"]) {
              paths[path] = sdkpath + path;
            }
            setPrefs("extensions.modules." + install.addon.id + ".path", paths);
          }
        },

        onInstallEnded: function(install, addon) {
          resolve(addon);
        },

        onDownloadCancelled: function(install) {
          reject("Download cancelled: " + install.error);
        },

        onDownloadFailed: function(install) {
          reject("Download failed: " + install.error);
        },

        onInstallCancelled: function(install) {
          reject("Install cancelled: " + install.error);
        },

        onInstallFailed: function(install) {
          reject("Install failed: " + install.error);
        }
      });

      install.install();
    }, "application/x-xpinstall");
  });
}


function uninstallAddon(oldAddon) {
  return new Promise(function(resolve, reject) {
    AddonManager.addAddonListener({
      onUninstalled: function(addon) {
        if (addon.id != oldAddon.id)
          return;

        dump("TEST-INFO | jetpack-addon-harness.js | Uninstalled test add-on " + addon.id + "\n");

        
        
        setTimeout(resolve, 500);
      }
    });

    oldAddon.uninstall();
  });
}


function waitForResults() {
  return new Promise(function(resolve, reject) {
    Services.obs.addObserver(function(subject, topic, data) {
      Services.obs.removeObserver(arguments.callee, "sdk:test:results");

      resolve(JSON.parse(data));
    }, "sdk:test:results", false);
  });
}


let testAddon = Task.async(function*({ url }) {
  dump("TEST-INFO | jetpack-addon-harness.js | Installing test add-on " + realPath(url) + "\n");
  let addon = yield installAddon(url);

  let results = yield waitForResults();

  dump("TEST-INFO | jetpack-addon-harness.js | Uninstalling test add-on " + addon.id + "\n");
  yield uninstallAddon(addon);

  dump("TEST-INFO | jetpack-addon-harness.js | Testing add-on " + realPath(url) + " is complete\n");
  return results;
});


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

  
  let config = readConfig();
  getTestList(config, function(links) {
    try {
      let fileNames = [];
      let fileNameRegexp = /.+\.xpi$/;
      arrayOfTestFiles(links, fileNames, fileNameRegexp);

      if (config.startAt || config.endAt) {
        fileNames = skipTests(fileNames, config.startAt, config.endAt);
      }

      
      try {
        let sdklibs = Services.prefs.getCharPref("extensions.sdk.path");
        
        let sdkfile = Cc["@mozilla.org/file/local;1"].
                      createInstance(Ci.nsIFile);
        sdkfile.initWithPath(sdklibs);
        sdkpath = Services.io.newFileURI(sdkfile).spec;
      }
      catch (e) {
        
      }

      let passed = 0;
      let failed = 0;

      function finish() {
        if (passed + failed == 0) {
          dump("TEST-UNEXPECTED-FAIL | jetpack-addon-harness.js | " +
               "No tests to run. Did you pass an invalid --test-path?\n");
        }
        else {
          dump("Jetpack Addon Test Summary\n");
          dump("\tPassed: " + passed + "\n" +
               "\tFailed: " + failed + "\n" +
               "\tTodo: 0\n");
        }

        if (config.closeWhenDone) {
          dump("TEST-INFO | jetpack-addon-harness.js | Shutting down.\n");

          const appStartup = Cc['@mozilla.org/toolkit/app-startup;1'].
                             getService(Ci.nsIAppStartup);
          appStartup.quit(appStartup.eAttemptQuit);
        }
      }

      function testNextAddon() {
        if (fileNames.length == 0)
          return finish();

        let filename = fileNames.shift();
        dump("TEST-INFO | jetpack-addon-harness.js | Starting test add-on " + realPath(filename.url) + "\n");
        testAddon(filename).then(results => {
          passed += results.passed;
          failed += results.failed;
        }).then(testNextAddon, error => {
          
          
          
          failed++;
          dump("TEST-UNEXPECTED-FAIL | jetpack-addon-harness.js | Error testing " + realPath(filename.url) + ": " + error + "\n");
          finish();
        });
      }

      testNextAddon();
    }
    catch (e) {
      dump("TEST-UNEXPECTED-FAIL | jetpack-addon-harness.js | error starting test harness (" + e + ")\n");
      dump(e.stack);
    }
  });
}
