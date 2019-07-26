


Cu.import("resource:///modules/devtools/dbg-server.jsm");
Cu.import("resource:///modules/devtools/dbg-client.jsm");

function run_test()
{
  
  DebuggerServer.init(function () true);
  DebuggerServer.addActors("resource://test/testactors.js");

  add_test(test_socket_conn);
  add_test(test_socket_shutdown);
  add_test(test_pipe_conn);

  run_next_test();
}

function really_long() {
  let ret = "0123456789";
  for (let i = 0; i < 18; i++) {
    ret += ret;
  }
  return ret;
}

function test_socket_conn()
{
  do_check_eq(DebuggerServer._socketConnections, 0);
  do_check_true(DebuggerServer.openListener(2929));
  do_check_eq(DebuggerServer._socketConnections, 1);
  
  do_check_true(DebuggerServer.openListener(2929));
  do_check_eq(DebuggerServer._socketConnections, 1);

  let unicodeString = "(╯°□°）╯︵ ┻━┻";
  let transport = debuggerSocketConnect("127.0.0.1", 2929);
  transport.hooks = {
    onPacket: function(aPacket) {
      this.onPacket = function(aPacket) {
        do_check_eq(aPacket.unicode, unicodeString);
        transport.close();
      }
      
      
      transport.send({to: "root",
                      type: "echo",
                      reallylong: really_long(),
                      unicode: unicodeString});
      do_check_eq(aPacket.from, "root");
    },
    onClosed: function(aStatus) {
      run_next_test();
    },
  };
  transport.ready();
}

function test_socket_shutdown()
{
  do_check_eq(DebuggerServer._socketConnections, 1);
  do_check_true(DebuggerServer.closeListener());
  do_check_eq(DebuggerServer._socketConnections, 0);
  
  do_check_false(DebuggerServer.closeListener());
  do_check_eq(DebuggerServer._socketConnections, 0);

  let transport = debuggerSocketConnect("127.0.0.1", 2929);
  transport.hooks = {
    onPacket: function(aPacket) {
      
      do_check_true(false);
    },

    onClosed: function(aStatus) {
      do_check_eq(aStatus, Cr.NS_ERROR_CONNECTION_REFUSED);
      run_next_test();
    }
  };

  transport.ready();
}

function test_pipe_conn()
{
  let transport = DebuggerServer.connectPipe();
  transport.hooks = {
    onPacket: function(aPacket) {
      do_check_eq(aPacket.from, "root");
      transport.close();
    },
    onClosed: function(aStatus) {
      run_next_test();
    }
  };

  transport.ready();
}
