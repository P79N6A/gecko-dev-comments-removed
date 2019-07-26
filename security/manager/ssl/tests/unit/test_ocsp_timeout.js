



"use strict";












let gSocketListener = {
  onSocketAccepted: function(serverSocket, socketTransport) {
    socketTransport.setTimeout(Ci.nsISocketTransport.TIMEOUT_CONNECT, 30);
    socketTransport.setTimeout(Ci.nsISocketTransport.TIMEOUT_READ_WRITE, 30);
  },

  onStopListening: function(serverSocket, socketTransport) {}
};

const ua = Cc["@mozilla.org/network/protocol;1?name=http"]
             .getService(Ci.nsIHttpProtocolHandler).userAgent;
const gIsWinXP = ua.indexOf("Windows NT 5.1") != -1;

function run_test() {
  do_get_profile();

  add_tls_server_setup("OCSPStaplingServer");

  let socket = Cc["@mozilla.org/network/server-socket;1"]
                 .createInstance(Ci.nsIServerSocket);
  socket.init(8080, true, -1);
  socket.asyncListen(gSocketListener);

  add_tests_in_mode(true, true);
  add_tests_in_mode(false, true);
  add_tests_in_mode(true, false);
  add_tests_in_mode(false, false);

  add_test(function() { socket.close(); run_next_test(); });
  run_next_test();
}

function add_tests_in_mode(useMozillaPKIX, useHardFail) {
  let startTime;
  add_test(function () {
    Services.prefs.setBoolPref("security.use_mozillapkix_verification",
                               useMozillaPKIX);
    Services.prefs.setBoolPref("security.OCSP.require", useHardFail);
    startTime = new Date();
    run_next_test();
  });

  add_connection_test("ocsp-stapling-none.example.com", useHardFail
                      ? getXPCOMStatusFromNSS(SEC_ERROR_OCSP_SERVER_ERROR)
                      : Cr.NS_OK, clearSessionCache);

  
  add_test(function() {
    let endTime = new Date();
    
    
    
    
    if (useHardFail) {
      do_check_true((endTime - startTime) > 10000 || gIsWinXP);
    } else {
      do_check_true((endTime - startTime) > 2000);
    }
    
    
    
    
    do_check_true((endTime - startTime) < 60000);
    clearOCSPCache();
    run_next_test();
  });
}
