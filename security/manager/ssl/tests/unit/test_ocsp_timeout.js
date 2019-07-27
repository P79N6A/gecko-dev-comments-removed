



"use strict";












let gSocketListener = {
  onSocketAccepted: function(serverSocket, socketTransport) {
    socketTransport.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 30);
    socketTransport.setTimeout(Ci.nsISocketTransport.TIMEOUT_READ_WRITE, 30);
  },

  onStopListening: function(serverSocket, status) {}
};

function run_test() {
  do_get_profile();

  add_tls_server_setup("OCSPStaplingServer");

  let socket = Cc["@mozilla.org/network/server-socket;1"]
                 .createInstance(Ci.nsIServerSocket);
  socket.init(8888, true, -1);
  socket.asyncListen(gSocketListener);

  add_tests_in_mode(true);
  add_tests_in_mode(false);

  add_test(function() { socket.close(); run_next_test(); });
  run_next_test();
}

function add_tests_in_mode(useHardFail) {
  let startTime;
  add_test(function () {
    Services.prefs.setBoolPref("security.OCSP.require", useHardFail);
    startTime = new Date();
    run_next_test();
  });

  add_connection_test("ocsp-stapling-none.example.com", useHardFail
                      ? getXPCOMStatusFromNSS(SEC_ERROR_OCSP_SERVER_ERROR)
                      : Cr.NS_OK, clearSessionCache);

  
  add_test(function() {
    let endTime = new Date();
    let timeDifference = endTime - startTime;
    do_print(`useHardFail = ${useHardFail}`);
    do_print(`startTime = ${startTime.getTime()} (${startTime})`);
    do_print(`endTime = ${endTime.getTime()} (${endTime})`);
    do_print(`timeDifference = ${timeDifference}ms`);

    
    
    if (useHardFail) {
      do_check_true(timeDifference > 10000);
    } else {
      do_check_true(timeDifference > 2000);
    }
    
    
    
    
    do_check_true(timeDifference < 60000);
    clearOCSPCache();
    run_next_test();
  });
}
