


"use strict";

Cu.import("resource://gre/modules/CloudSync.jsm");

function run_test () {
  run_next_test();
}

add_task(function test_module_load () {
  ok(CloudSync);
  let cloudSync = CloudSync();
  ok(cloudSync.adapters);
  ok(cloudSync.bookmarks);
  ok(cloudSync.local);
  ok(cloudSync.tabs);
});
