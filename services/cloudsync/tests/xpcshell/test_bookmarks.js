


"use strict";

Cu.import("resource://gre/modules/CloudSync.jsm");

function run_test () {
  run_next_test();
}

add_task(function test_get_remote_tabs () {
  let rootFolder = yield CloudSync().bookmarks.getRootFolder("TEST");
  ok(rootFolder.id);
});
