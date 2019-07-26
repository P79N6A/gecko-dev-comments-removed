


Cu.import("resource://services-common/observers.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://services-sync/engines.js");

function run_test() {
  _("When imported, Service.onStartup is called");
  initTestLogging("Trace");

  Svc.Prefs.set("registerEngines", "Tab,Bookmarks,Form,History");
  new SyncTestingInfrastructure();

  
  Identity.username = "johndoe";

  Cu.import("resource://services-sync/service.js");

  _("Service is enabled.");
  do_check_eq(Service.enabled, true);

  _("Engines are registered.");
  let engines = Engines.getAll();
  do_check_true(Utils.deepEquals([engine.name for each (engine in engines)],
                                 ['tabs', 'bookmarks', 'forms', 'history']));

  _("Observers are notified of startup");
  do_test_pending();
  do_check_false(Status.ready);
  Observers.add("weave:service:ready", function (subject, data) {
    do_check_true(Status.ready);

    
    Svc.Prefs.resetBranch("");
    do_test_finished();
  });
}
