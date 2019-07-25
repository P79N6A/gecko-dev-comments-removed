






var gTab1 = null;
var gTab1Actor = null;

var gClient = null;

function test()
{
  DebuggerServer.addActors("chrome://mochitests/content/browser/browser/devtools/debugger/test/testactors.js");

  let transport = DebuggerServer.connectPipe();
  gClient = new DebuggerClient(transport);
  gClient.connect(function(aType, aTraits) {
    is(aType, "browser", "Root actor should identify itself as a browser.");
    get_tab();
  });
}

function get_tab()
{
  gTab1 = addTab(TAB1_URL, function() {
    get_tab_actor_for_url(gClient, TAB1_URL, function(aGrip) {
      gTab1Actor = aGrip.actor;
      gClient.request({ to: gTab1Actor, type: "attach" }, function(aResponse) {
        gClient.request({ to: gTab1Actor, type: "testContextActor1" }, function(aResponse) {
          navigate_tab(aResponse.actor);
        });
      });
    });
  });
}

function navigate_tab(aTestActor)
{
  gClient.addOneTimeListener("tabNavigated", function(aEvent, aResponse) {
    gClient.request({ to: aTestActor, type: "ping" }, function(aResponse) {
      
      
      
      todo(aResponse.error, "noSuchActor", "testContextActor1 should have gone away with the navigation.");
      finish_test();
    });
  });
  gTab1.linkedBrowser.loadURI(TAB2_URL);
}

function finish_test()
{
  gClient.close(function() {
    removeTab(gTab1);
    finish();
  });
}
