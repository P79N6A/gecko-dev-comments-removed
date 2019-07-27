


"use strict";






const Profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);

function run_test()
{
  
  
  Profiler.StopProfiler();

  get_chrome_actors((client1, form1) => {
    let actor1 = form1.profilerActor;
    get_chrome_actors((client2, form2) => {
      let actor2 = form2.profilerActor;
      test_activate(client1, actor1, client2, actor2, () => {
        do_test_finished();
      });
    });
  });

  do_test_pending();
}

function test_activate(client1, actor1, client2, actor2, callback) {
  
  client1.request({ to: actor1, type: "isActive" }, response => {
    do_check_false(Profiler.IsActive());
    do_check_false(response.isActive);
    do_check_eq(response.currentTime, undefined);

    
    client1.request({ to: actor1, type: "startProfiler" }, response => {
      do_check_true(Profiler.IsActive());
      do_check_true(response.started);

      
      client2.request({ to: actor2, type: "isActive" }, response => {
        do_check_true(Profiler.IsActive());
        do_check_true(response.isActive);
        do_check_true(response.currentTime > 0);

        let origConnectionClosed = DebuggerServer._connectionClosed;

        DebuggerServer._connectionClosed = function(conn) {
          origConnectionClosed.call(this, conn);

          
          
          
          do_check_true(Profiler.IsActive());

          DebuggerServer._connectionClosed = function(conn) {
            origConnectionClosed.call(this, conn);

            
            
            do_check_false(Profiler.IsActive());

            callback();
          }
          client2.close();
        };
        client1.close();
      });
    });
  });
}
