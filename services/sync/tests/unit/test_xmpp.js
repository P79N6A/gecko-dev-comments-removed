var Cu = Components.utils;

Cu.import( "resource://weave/xmpp/xmppClient.js" );

var serverUrl = "http://127.0.0.1:5280/http-poll";
var jabberDomain = "jonathan-dicarlos-macbook-pro.local";

var timer = Cc["@mozilla.org/timer;1"].createInstance( Ci.nsITimer );
var threadManager = Cc["@mozilla.org/thread-manager;1"].getService();

function run_test() {

  
  return;

  
  var transport = new HTTPPollingTransport( serverUrl,
					    false,
					    4000 );
  var auth = new PlainAuthenticator();
  var alice = new XmppClient( "alice", jabberDomain, "iamalice",
			       transport, auth );

  











  
  var testIsOver = false;
  var sometext = "bla bla how you doin bla";
  var transport2 = new HTTPPollingTransport( serverUrl, false, 4000 );
  var auth2 = new PlainAuthenticator();
  var bob = new XmppClient( "bob", jabberDomain, "iambob", transport2, auth2 );

  
  
  var timerResponder = {
  notify: function( timer ) {
      testIsOver = true;
      do_throw( "Timed out waiting for message." );
    }
  };
  timer.initWithCallback( timerResponder, 20000, timer.TYPE_ONE_SHOT );


  
  var aliceMessageHandler = {
  handle: function( msgText, from ) {
      dump( "Alice got a message.\n" );
      do_check_eq( msgText, sometext );
      do_check_eq( from, "bob@" + jabberDomain );
      timer.cancel();
      testIsOver = true;
    }
  };
  alice.registerMessageHandler( aliceMessageHandler );

  
  bob.connect( jabberDomain );
  bob.waitForConnection();
  do_check_neq( bob._connectionStatus, bob.FAILED );
  alice.connect( jabberDomain );
  alice.waitForConnection();
  do_check_neq( alice._connectionStatus, alice.FAILED );

  
  bob.sendMessage( "alice@" + jabberDomain, sometext );
  
  var currentThread = threadManager.currentThread;
  while( !testIsOver ) {
    currentThread.processNextEvent( true );
  }

  alice.disconnect();
  bob.disconnect();
};
