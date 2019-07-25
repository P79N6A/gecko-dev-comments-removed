var Cu = Components.utils;

Cu.import( "resource://weave/xmpp/xmppClient.js" );

function LOG(aMsg) {
  dump("TEST_XMPP: " + aMsg + "\n");
}

var serverUrl = "http://localhost:5280/http-poll";
var jabberDomain = "localhost";

var timer = Cc["@mozilla.org/timer;1"].createInstance( Ci.nsITimer );
var threadManager = Cc["@mozilla.org/thread-manager;1"].getService();

function run_test() {
  
  return;

  
  var transport = new HTTPPollingTransport(serverUrl, false, 4000);
  var auth = new PlainAuthenticator();
  var alice = new XmppClient("alice", jabberDomain, "iamalice",
    	            		       transport, auth);

  
  LOG("connecting");
  alice.connect( jabberDomain );
  alice.waitForConnection();
  do_check_eq( alice._connectionStatus, alice.CONNECTED);
  LOG("connected");

  
  LOG("disconnecting");
  alice.disconnect();
  do_check_eq( alice._connectionStatus, alice.NOT_CONNECTED);
  LOG("disconnected");

  
  LOG("reconnecting");
  alice.connect( jabberDomain );
  alice.waitForConnection();
  LOG("reconnected");
  do_check_eq( alice._connectionStatus, alice.CONNECTED);
  alice.disconnect();

  



























































};
