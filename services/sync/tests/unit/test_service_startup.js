Cu.import("resource://weave/util.js");
Cu.import("resource://weave/identity.js");
Cu.import("resource://weave/ext/Observers.js");

function run_test() {
  _("When imported, Weave.Service.onStartup is called");

  
  let observerCalled = false;
  Observers.add("weave:service:ready", function (subject, data) {
    observerCalled = true;
  });
  Svc.Prefs.set("registerEngines", "Tab,Bookmarks,Form,History");
  Svc.Prefs.set("username", "johndoe");

  try {
    Cu.import("resource://weave/service.js");

    _("Service is enabled.");
    do_check_eq(Weave.Service.enabled, true);

    _("Engines are registered.");
    let engines = Weave.Engines.getAll();
    do_check_true(Utils.deepEquals([engine.name for each (engine in engines)],
                                   ['tabs', 'bookmarks', 'forms', 'history']));

    _("Identities are registered.");
    do_check_eq(ID.get('WeaveID').username, "johndoe");
    do_check_eq(ID.get('WeaveCryptoID').username, "johndoe");

    _("Observers are notified of startup");
    do_check_true(observerCalled);

  } finally {
    Svc.Prefs.resetBranch("");
  }
}
