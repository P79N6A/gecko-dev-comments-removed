var Cu = Components.utils;

Cu.import( "resource://weave/xmpp/xmppClient.js" );

function LOG(aMsg) {
  dump("TEST_XMPP_SIMPLE: " + aMsg + "\n");
}

var serverUrl = "http://127.0.0.1:5280/http-poll";
var jabberDomain = Cc["@mozilla.org/network/dns-service;1"].
                   getService(Ci.nsIDNSService).myHostName;

var timer = Cc["@mozilla.org/timer;1"].createInstance( Ci.nsITimer );
var threadManager = Cc["@mozilla.org/thread-manager;1"].getService();

var alice;

function run_test() {
  
  return;

  
  var transport = new HTTPPollingTransport( serverUrl,
					    false,
					    4000 );
  var auth = new PlainAuthenticator();
  alice = new XmppClient( "alice", jabberDomain, "iamalice",
			       transport, auth );

  
  alice.connect( jabberDomain );
  alice.waitForConnection();
  do_check_eq( alice._connectionStatus, alice.CONNECTED);

  
  alice.disconnect();
  LOG("disconnected");
  alice.connect( jabberDomain );
  LOG("wait");
  alice.waitForConnection();
  LOG("waited");
  do_check_eq( alice._connectionStatus, alice.CONNECTED);

  


























































  alice.disconnect();
  
};
