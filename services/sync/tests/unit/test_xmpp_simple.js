function LOG(aMsg) {
  dump("TEST_XMPP_SIMPLE: " + aMsg + "\n");
}

Components.utils.import( "resource://weave/xmpp/xmppClient.js" );

var serverUrl = "http://localhost:5280/http-poll";
var jabberDomain = "localhost";

function run_test() {
  
  

  
  do_test_pending();

  var testMessage = "Hello Bob.";

  var aliceHandler = {
    handle: function(msgText, from) {
      LOG("ALICE RCVD from " + from + ": " + msgText);
    }
  };
  var aliceClient = getClientForUser("alice", "iamalice", aliceHandler);

  var bobHandler = {
    handle: function(msgText, from) {
      LOG("BOB RCVD from " + from + ": " + msgText);
      do_check_eq(from.split("/")[0], "alice@" + jabberDomain);
      do_check_eq(msgText, testMessage);
      LOG("messages checked out");

      aliceClient.disconnect();
      bobClient.disconnect();
      LOG("disconnected");

      do_test_finished();
    }
  };
  var bobClient = getClientForUser("bob", "iambob", bobHandler);
  bobClient.announcePresence();


  
  aliceClient.sendMessage("bob@" + jabberDomain, testMessage);
}

function getClientForUser(aName, aPassword, aHandler) {
  
  
  var transport = new HTTPPollingTransport(serverUrl, false, 4000);

  var auth = new PlainAuthenticator();

  var client = new XmppClient(aName, jabberDomain, aPassword,
                             transport, auth);

  client.registerMessageHandler(aHandler);

  
  client.connect(jabberDomain);
  client.waitForConnection();

  
  
  if ( client._connectionStatus == client.FAILED ) {
    do_throw("connection failed");
  }

  return client;
}
