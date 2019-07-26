


Cu.import("resource://testing-common/services/sync/utils.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/browserid_identity.js");
Cu.import("resource://services-sync/service.js");

function run_test() {
  initTestLogging("Trace");
  run_next_test();
}

add_task(function* test_startover() {
  let oldValue = Services.prefs.getBoolPref("services.sync-testing.startOverKeepIdentity", true);
  Services.prefs.setBoolPref("services.sync-testing.startOverKeepIdentity", false);

  ensureLegacyIdentityManager();
  yield configureIdentity({username: "johndoe"});

  
  let xps = Cc["@mozilla.org/weave/service;1"]
            .getService(Components.interfaces.nsISupports)
            .wrappedJSObject;
  do_check_false(xps.fxAccountsEnabled);

  
  
  do_check_false(Service.identity instanceof BrowserIDManager);

  Service.serverURL = "https://localhost/";
  Service.clusterURL = Service.serverURL;

  Service.login();
  
  do_check_true(Service.clusterURL.length > 0);

  
  let oldIdentity = Service.identity;
  let oldClusterManager = Service._clusterManager;
  let deferred = Promise.defer();
  Services.obs.addObserver(function observeStartOverFinished() {
    Services.obs.removeObserver(observeStartOverFinished, "weave:service:start-over:finish");
    deferred.resolve();
  }, "weave:service:start-over:finish", false);

  Service.startOver();
  yield deferred.promise; 

  
  do_check_true(xps.fxAccountsEnabled);
  
  do_check_true(Service.identity instanceof BrowserIDManager);
  
  do_check_eq(Service.clusterURL, "");

  
  do_check_neq(oldIdentity, Service.identity);
  do_check_neq(oldClusterManager, Service._clusterManager);

  
  Services.prefs.setBoolPref("services.sync-testing.startOverKeepIdentity", oldValue);
});
