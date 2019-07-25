


Cu.import("resource:///modules/devtools/dbg-server.jsm");
Cu.import("resource:///modules/devtools/dbg-client.jsm");

function run_test()
{
  
  
  check_except(function() {
    DebuggerServer.openListener(2929, true);
  });
  check_except(DebuggerServer.closeListener);
  check_except(DebuggerServer.connectPipe);

  DebuggerServer.init();

  
  
  check_except(function() {
    DebuggerServer.openListener(2929, true);
  });
  check_except(DebuggerServer.closeListener);
  check_except(DebuggerServer.connectPipe);

  DebuggerServer.addActors("resource://test/testactors.js");

  
  DebuggerServer.openListener(2929, true);
  DebuggerServer.closeListener();

  
  let client1 = DebuggerServer.connectPipe();
  client1.hooks = {
    onPacket: function(aPacket1) {
      do_check_eq(aPacket1.from, "root");
      do_check_eq(aPacket1.applicationType, "xpcshell-tests");

      
      
      let client2 = DebuggerServer.connectPipe();
      client2.hooks = {
        onPacket: function(aPacket2) {
          do_check_eq(aPacket2.from, "root");
          do_check_neq(aPacket1.testConnectionPrefix,
                       aPacket2.testConnectionPrefix);
          client2.close();
        },
        onClosed: function(aResult) {
          client1.close();
        },
      };
      client2.ready();
    },

    onClosed: function(aResult) {
      do_test_finished();
    },
  };

  client1.ready();
  do_test_pending();
}
