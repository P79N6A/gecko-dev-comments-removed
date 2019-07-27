




"use strict";

do_get_profile();

function connect_and_teardown() {
  let socketTransportService =
    Cc["@mozilla.org/network/socket-transport-service;1"]
      .getService(Ci.nsISocketTransportService);

  let tearDown = false;

  let reader = {
    onInputStreamReady: function(stream) {
      try {
        stream.available();
        Assert.ok(false, "stream.available() should have thrown");
      }
      catch (e) {
        Assert.equal(e.result, Components.results.NS_ERROR_FAILURE,
                     "stream should be in an error state");
        Assert.ok(tearDown, "this should be as a result of logoutAndTeardown");
        run_next_test();
      }
    }
  };

  let sink = {
    onTransportStatus: function(transport, status, progress, progressmax) {
      if (status == Ci.nsISocketTransport.STATUS_CONNECTED_TO) {
        
        
        
        tearDown = true;
        Cc["@mozilla.org/security/sdr;1"].getService(Ci.nsISecretDecoderRing)
          .logoutAndTeardown();
      }
    }
  };

  Services.prefs.setCharPref("network.dns.localDomains",
                             "ocsp-stapling-none.example.com");
  let transport = socketTransportService.createTransport(
    ["ssl"], 1, "ocsp-stapling-none.example.com", 8443, null);
  transport.setEventSink(sink, Services.tm.currentThread);

  let inStream = transport.openInputStream(0, 0, 0)
                          .QueryInterface(Ci.nsIAsyncInputStream);
  inStream.asyncWait(reader, Ci.nsIAsyncInputStream.WAIT_CLOSURE_ONLY, 0,
                     Services.tm.currentThread);
}

function run_test() {
  add_tls_server_setup("OCSPStaplingServer");
  add_test(connect_and_teardown);
  run_next_test();
}
