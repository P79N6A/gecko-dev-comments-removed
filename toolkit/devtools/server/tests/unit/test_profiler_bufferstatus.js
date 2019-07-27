


"use strict";





const Profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
const INITIAL_WAIT_TIME = 100; 
const MAX_WAIT_TIME = 20000; 
const MAX_PROFILER_ENTRIES = 10000000;

function run_test()
{
  
  
  Profiler.StopProfiler();

  get_chrome_actors((client, form) => {
    let actor = form.profilerActor;
    check_empty_buffer(client, actor, () => {
      activate_profiler(client, actor, startTime => {
        wait_for_samples(client, actor, () => {
          check_buffer(client, actor, () => {
            deactivate_profiler(client, actor, () => {
              client.close(do_test_finished);
            });
          });
        });
      });
    });
  })

  do_test_pending();
}

function check_buffer(client, actor, callback)
{
  client.request({ to: actor, type: "isActive" }, response => {
    do_check_true(typeof response.position === "number");
    do_check_true(typeof response.totalSize === "number");
    do_check_true(typeof response.generation === "number");
    do_check_true(response.position > 0 && response.position < response.totalSize);
    do_check_true(response.totalSize === MAX_PROFILER_ENTRIES);
    
    do_check_true(response.generation === 0);

    callback();
  });
}

function check_empty_buffer(client, actor, callback)
{
  client.request({ to: actor, type: "isActive" }, response => {
    do_check_false(Profiler.IsActive());
    do_check_false(response.isActive);
    do_check_true(response.position === void 0);
    do_check_true(response.totalSize === void 0);
    do_check_true(response.generation === void 0);
    do_check_false(response.isActive);
    do_check_eq(response.currentTime, undefined);
    calback();
  });
}

function activate_profiler(client, actor, callback)
{
  client.request({ to: actor, type: "startProfiler", entries: MAX_PROFILER_ENTRIES }, response => {
    do_check_true(response.started);
    client.request({ to: actor, type: "isActive" }, response => {
      do_check_true(response.isActive);
      callback(response.currentTime);
    });
  });
}

function deactivate_profiler(client, actor, callback)
{
  client.request({ to: actor, type: "stopProfiler" }, response => {
    do_check_false(response.started);
    client.request({ to: actor, type: "isActive" }, response => {
      do_check_false(response.isActive);
      callback();
    });
  });
}

function wait_for_samples(client, actor, callback)
{
  function attempt(delay)
  {
    
    let funcLine = Components.stack.lineNumber - 3;

    
    let start = Date.now();
    let stack;
    do_print("Attempt: delay = " + delay);
    while (Date.now() - start < delay) { stack = Components.stack; }
    do_print("Attempt: finished waiting.");

    client.request({ to: actor, type: "getProfile" }, response => {
      
      
      
      if (response.profile.threads[0].samples.length == 0) {
        if (delay < MAX_WAIT_TIME) {
          
          do_print("Attempt: no samples, going around again.");
          return attempt(delay * 2);
        } else {
          
          do_print("Attempt: waited a long time, but no samples were collected.");
          do_print("Giving up.");
          do_check_true(false);
          return;
        }
      }
      callback();
    });
  }

  
  attempt(INITIAL_WAIT_TIME);
}
