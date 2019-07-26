



XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/commonjs/sdk/core/promise.js");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
  "resource://gre/modules/Task.jsm");

registerCleanupFunction(function() {
  
  try {
    Services.prefs.clearUserPref("datareporting.healthreport.about.reportUrl");
    let policy = Cc["@mozilla.org/datareporting/service;1"]
                 .getService(Ci.nsISupports)
                 .wrappedJSObject
                 .policy;
        policy.recordHealthReportUploadEnabled(true,
                                           "Resetting after tests.");
  } catch (ex) {}
});

let gTests = [

{
  desc: "Test the remote commands",
  setup: function ()
  {
    Services.prefs.setCharPref("datareporting.healthreport.about.reportUrl",
                               "https://example.com/browser/browser/base/content/test/healthreport_testRemoteCommands.html");
  },
  run: function ()
  {
    let deferred = Promise.defer();

    let policy = Cc["@mozilla.org/datareporting/service;1"]
                 .getService(Ci.nsISupports)
                 .wrappedJSObject
                 .policy;

    let results = 0;
    try {
      let win = gBrowser.contentWindow;
      win.addEventListener("message", function testLoad(e) {
        if (e.data.type == "testResult") {
          ok(e.data.pass, e.data.info);
          results++;
        }
        else if (e.data.type == "testsComplete") {
          is(results, e.data.count, "Checking number of results received matches the number of tests that should have run");
          win.removeEventListener("message", testLoad, false, true);
          deferred.resolve();
        }

      }, false, true);

    } catch(e) {
      ok(false, "Failed to get all commands");
      deferred.reject();
    }
    return deferred.promise;
  }
},


]; 

function test()
{
  waitForExplicitFinish();

  
  requestLongerTimeout(10);

  Task.spawn(function () {
    for (let test of gTests) {
      info(test.desc);
      test.setup();

      yield promiseNewTabLoadEvent("about:healthreport");

      yield test.run();

      gBrowser.removeCurrentTab();
    }

    finish();
  });
}

function promiseNewTabLoadEvent(aUrl, aEventType="load")
{
  let deferred = Promise.defer();
  let tab = gBrowser.selectedTab = gBrowser.addTab(aUrl);
  tab.linkedBrowser.addEventListener(aEventType, function load(event) {
    tab.linkedBrowser.removeEventListener(aEventType, load, true);
    let iframe = tab.linkedBrowser.contentDocument.getElementById("remote-report");
      iframe.addEventListener("load", function frameLoad(e) {
        iframe.removeEventListener("load", frameLoad, false);
        deferred.resolve();
      }, false);
    }, true);
  return deferred.promise;
}

