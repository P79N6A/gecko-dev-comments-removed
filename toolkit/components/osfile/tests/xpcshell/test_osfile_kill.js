"use strict";

Components.utils.import("resource://gre/modules/osfile.jsm");



var AsyncFrontGlobal = Components.utils.import(
                         "resource://gre/modules/osfile/osfile_async_front.jsm",
                          null);
var Scheduler = AsyncFrontGlobal.Scheduler;














































add_task(function* test_kill_race() {
  
  
  
  
  yield OS.File.exists('foo.foo');

  do_print('issuing first request');
  let firstRequest = OS.File.exists('foo.bar');
  let secondRequest;
  let secondResolved = false;

  
  
  
  
  
  Scheduler.queue.then(function() {
    do_print('issuing second request');
    secondRequest = OS.File.exists('foo.baz');
    secondRequest.then(function() {
      secondResolved = true;
    });
  });

  do_print('issuing kill request');
  let killRequest = Scheduler.kill({ reset: true, shutdown: false });

  
  
  yield killRequest;
  
  
  
  
  yield OS.File.exists('foo.goz');

  ok(secondResolved,
     'The second request was resolved so we avoided the bug. Victory!');
});

function run_test() {
  run_next_test();
}
