function LOG(aMsg) {
  dump("TEST_XMPP_TRANSPORT_HTTP: " + aMsg + "\n");
}

Components.utils.import( "resource://weave/xmpp/xmppClient.js" );

var tests = [];


tests.push(function run_test_bad_server() {
  LOG("starting test: bad server");

  var transport = new HTTPPollingTransport("this is not a server URL", false, 4000);
  transport.connect();
  transport.setCallbackObject({
    onIncomingData: function(aData) {
      do_throw("onIncomingData was called instead of onTransportError, for a bad URL");
    },
    onTransportError: function(aErrorMessage) {
      do_check_true(/^Unable to send message to server:/.test(aErrorMessage));
      
      tests.shift()();
    }
  });
  transport.send();
});

tests.push(function run_test_bad_url() {
  LOG("starting test: bad url");
  
  var serverUrl = "http://localhost:5280/http-polly-want-a-cracker";
  var transport = new HTTPPollingTransport(serverUrl, false, 4000);
  transport.connect();
  transport.setCallbackObject({
    onIncomingData: function(aData) {
      do_throw("onIncomingData was called instead of onTransportError, for a bad URL");
    },
    onTransportError: function(aErrorMessage) {
      LOG("ERROR: " + aErrorMessage);
      do_check_true(/^Provided URL is not valid./.test(aErrorMessage));
      do_test_finished();
    }
  });
  transport.send();
});

function run_test() {
  
  return;

  
  do_test_pending();

  tests.shift()();
}
