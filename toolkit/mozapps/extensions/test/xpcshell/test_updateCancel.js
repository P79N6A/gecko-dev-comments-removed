





Components.utils.import("resource://gre/modules/Promise.jsm");


Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, false);
Services.prefs.setBoolPref(PREF_EM_STRICT_COMPATIBILITY, false);

createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9.2");


Components.utils.import("resource://testing-common/httpd.js");

const profileDir = gProfD.clone();
profileDir.append("extensions");


function run_test() {
  
  run_next_test();
}











function makeCancelListener() {
  let updated = Promise.defer();
  return {
    onUpdateAvailable: function(addon, install) {
      updated.reject("Should not have seen onUpdateAvailable notification");
    },

    onUpdateFinished: function(aAddon, aError) {
      do_print("onUpdateCheckFinished: " + aAddon.id + " " + aError);
      updated.resolve(aError);
    },
    promise: updated.promise
  };
}


let httpReceived = Promise.defer();
function dataHandler(aRequest, aResponse) {
  asyncResponse = aResponse;
  aResponse.processAsync();
  httpReceived.resolve([aRequest, aResponse]);
}
var testserver = new HttpServer();
testserver.registerDirectory("/addons/", do_get_file("addons"));
testserver.registerPathHandler("/data/test_update.rdf", dataHandler);
testserver.start(-1);
gPort = testserver.identity.primaryPort;


writeInstallRDFForExtension({
  id: "addon1@tests.mozilla.org",
  version: "1.0",
  updateURL: "http://localhost:" + gPort + "/data/test_update.rdf",
  targetApplications: [{
    id: "xpcshell@tests.mozilla.org",
    minVersion: "1",
    maxVersion: "1"
  }],
  name: "Test Addon 1",
}, profileDir);

add_task(function cancel_during_check() {
  startupManager();

  let a1 = yield promiseAddonByID("addon1@tests.mozilla.org");
  do_check_neq(a1, null);

  let listener = makeCancelListener();
  a1.findUpdates(listener, AddonManager.UPDATE_WHEN_USER_REQUESTED);

  
  let [request, response] = yield httpReceived.promise;

  
  do_check_true(a1.cancelUpdate());

  let updateResult = yield listener.promise;
  do_check_eq(AddonManager.UPDATE_STATUS_CANCELLED, updateResult);

  
  let file = do_get_cwd();
  file.append("data");
  file.append("test_update.rdf");
  let data = loadFile(file);
  response.write(data);
  response.finish();

  
  do_check_false(a1.cancelUpdate());

  yield true;
});



add_task(function shutdown_during_check() {
  
  httpReceived = Promise.defer();

  let a1 = yield promiseAddonByID("addon1@tests.mozilla.org");
  do_check_neq(a1, null);

  let listener = makeCancelListener();
  a1.findUpdates(listener, AddonManager.UPDATE_WHEN_USER_REQUESTED);

  
  let [request, response] = yield httpReceived.promise;

  shutdownManager();

  let updateResult = yield listener.promise;
  do_check_eq(AddonManager.UPDATE_STATUS_CANCELLED, updateResult);

  
  let file = do_get_cwd();
  file.append("data");
  file.append("test_update.rdf");
  let data = loadFile(file);
  response.write(data);
  response.finish();

  
  do_check_false(a1.cancelUpdate());

  yield testserver.stop(Promise.defer().resolve);
});
