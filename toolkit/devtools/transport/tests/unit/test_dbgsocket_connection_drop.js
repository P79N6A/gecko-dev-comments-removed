









Cu.import("resource://gre/modules/devtools/dbg-server.jsm");
Cu.import("resource://gre/modules/devtools/dbg-client.jsm");

const { RawPacket } = devtools.require("devtools/toolkit/transport/packets");

let port = 2929;

function run_test() {
  do_print("Starting test at " + new Date().toTimeString());
  initTestDebuggerServer();

  add_test(test_socket_conn_drops_after_invalid_header);
  add_test(test_socket_conn_drops_after_invalid_header_2);
  add_test(test_socket_conn_drops_after_too_large_length);
  add_test(test_socket_conn_drops_after_too_long_header);
  run_next_test();
}

function test_socket_conn_drops_after_invalid_header() {
  return test_helper('fluff30:27:{"to":"root","type":"echo"}');
}

function test_socket_conn_drops_after_invalid_header_2() {
  return test_helper('27asd:{"to":"root","type":"echo"}');
}

function test_socket_conn_drops_after_too_large_length() {
  
  return test_helper('4305724038957487634549823475894325:');
}

function test_socket_conn_drops_after_too_long_header() {
  
  let rawPacket = '4305724038957487634549823475894325';
  for (let i = 0; i < 8; i++) {
    rawPacket += rawPacket;
  }
  return test_helper(rawPacket + ':');
}

function test_helper(payload) {
  try_open_listener();

  let transport = debuggerSocketConnect("127.0.0.1", port);
  transport.hooks = {
    onPacket: function(aPacket) {
      this.onPacket = function(aPacket) {
        do_throw(new Error("This connection should be dropped."));
        transport.close();
      }

      
      transport._outgoing.push(new RawPacket(transport, payload));
      transport._flushOutgoing();
    },
    onClosed: function(aStatus) {
      do_check_true(true);
      run_next_test();
    },
  };
  transport.ready();
}

function try_open_listener() {
  try {
    do_check_true(DebuggerServer.openListener(port));
  } catch (e) {
    
    port = Math.floor(Math.random() * (65000 - 2000 + 1)) + 2000;
    try_open_listener();
  }
}
