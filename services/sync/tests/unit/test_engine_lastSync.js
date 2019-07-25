Cu.import("resource://weave/engines.js");

function run_test() {
  
  let engine = new SyncEngine();
  engine.lastSync = 123.45;
  do_check_eq(engine.lastSync, 123.45);
}
