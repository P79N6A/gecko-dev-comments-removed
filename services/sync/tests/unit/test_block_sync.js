

Cu.import("resource://services-sync/main.js");
Cu.import("resource://services-sync/util.js");


add_task(function *() {
  Assert.ok(!Weave.Service.scheduler.isBlocked, "sync is not blocked.")
  Assert.ok(!Svc.Prefs.has("scheduler.blocked-until"), "have no blocked pref");
  Weave.Service.scheduler.blockSync();

  Assert.ok(Weave.Service.scheduler.isBlocked, "sync is blocked.")
  Assert.ok(Svc.Prefs.has("scheduler.blocked-until"), "have the blocked pref");

  Weave.Service.scheduler.unblockSync();
  Assert.ok(!Weave.Service.scheduler.isBlocked, "sync is not blocked.")
  Assert.ok(!Svc.Prefs.has("scheduler.blocked-until"), "have no blocked pref");

  
  let until = Date.now() + 1000;
  Weave.Service.scheduler.blockSync(until);
  Assert.ok(Weave.Service.scheduler.isBlocked, "sync is blocked.")
  Assert.ok(Svc.Prefs.has("scheduler.blocked-until"), "have the blocked pref");

  
  yield new Promise((resolve, reject) => {
    CommonUtils.namedTimer(resolve, 1000, {}, "timer");
  });

  
  Assert.ok(!Weave.Service.scheduler.isBlocked, "sync is not blocked.")
  Assert.ok(!Svc.Prefs.has("scheduler.blocked-until"), "have no blocked pref");
});

function run_test() {
  run_next_test();
}
