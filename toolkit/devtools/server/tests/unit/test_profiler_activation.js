


"use strict";

const Profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);

function connectClient(callback) {
  let client = new DebuggerClient(DebuggerServer.connectPipe());
  client.connect(function () {
    client.listTabs(function(response) {
      callback(client, response.profilerActor);
    });
  });
}

function run_test()
{
  
  
  Profiler.StopProfiler();

  DebuggerServer.init(function () { return true; });
  DebuggerServer.addBrowserActors();

  connectClient((client1, actor1) => {
    connectClient((client2, actor2) => {
      activate_first(client1, actor1, client2, actor2);
    });
  })

  do_test_pending();
}

function activate_first(client1, actor1, client2, actor2) {
  
  client1.request({ to: actor1, type: "startProfiler", features: ['js']}, startResponse => {
    
    do_check_true(Profiler.IsActive());

    
    
    client2.request({ to: actor2, type: "getFeatures" }, featureResponse => {

      let connectionClosed = DebuggerServer._connectionClosed;
      DebuggerServer._connectionClosed = function(conn) {
        connectionClosed.call(this, conn);

        
        
        do_check_false(Profiler.IsActive());

        DebuggerServer._connectionClosed = function(conn) {
          connectionClosed.call(this, conn);

          
          
          do_check_false(Profiler.IsActive());
          do_test_finished();
        }
        client2.close();
      };
      client1.close();
    });
  });
}
