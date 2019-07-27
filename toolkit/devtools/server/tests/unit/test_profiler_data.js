


"use strict";






const Profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
const INITIAL_WAIT_TIME = 100; 
const MAX_WAIT_TIME = 20000; 

function run_test()
{
  get_chrome_actors((client, form) => {
    let actor = form.profilerActor;
    activate_profiler(client, actor, startTime => {
      test_data(client, actor, startTime, () => {
        deactivate_profiler(client, actor, () => {
          client.close(do_test_finished);
        })
      });
    });
  })

  do_test_pending();
}

function activate_profiler(client, actor, callback)
{
  client.request({ to: actor, type: "startProfiler" }, response => {
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

function test_data(client, actor, startTime, callback)
{
  function attempt(delay)
  {
    
    let funcLine = Components.stack.lineNumber - 3;

    
    let start = Date.now();
    let stack;
    do_print("Attempt: delay = " + delay);
    while (Date.now() - start < delay) { stack = Components.stack; }
    do_print("Attempt: finished waiting.");

    client.request({ to: actor, type: "getProfile", startTime  }, response => {
      
      
      do_check_eq(typeof response.profile, "object");
      do_check_eq(typeof response.profile.meta, "object");
      do_check_eq(typeof response.profile.meta.platform, "string");
      do_check_eq(typeof response.profile.threads, "object");
      do_check_eq(typeof response.profile.threads[0], "object");
      do_check_eq(typeof response.profile.threads[0].samples, "object");

      
      
      
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

      
      
      let loc = stack.name + " (" + stack.filename + ":" + funcLine + ")";

      do_check_true(response.profile.threads[0].samples.some(sample => {
        return typeof sample.frames == "object" &&
               sample.frames.length != 0 &&
               sample.frames.some(f => (f.location == loc));
      }));

      callback();
    });
  }

  
  attempt(INITIAL_WAIT_TIME);
}
