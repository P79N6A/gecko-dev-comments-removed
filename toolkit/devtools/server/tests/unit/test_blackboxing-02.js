








var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-black-box");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-black-box", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_black_box();
    });
  });
  do_test_pending();
}

const BLACK_BOXED_URL = "http://example.com/blackboxme.js";
const SOURCE_URL = "http://example.com/source.js";

function test_black_box()
{
  gClient.addOneTimeListener("paused", function  (aEvent, aPacket) {
    gThreadClient.eval(aPacket.frame.actor, "doStuff", function(aResponse) {
      gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
        let obj = gThreadClient.pauseGrip(aPacket.why.frameFinished.return);
        obj.getDefinitionSite(runWithSource);
      });
    });

    function runWithSource(aPacket) {
      let source = gThreadClient.source(aPacket.source);
      source.setBreakpoint({
        line: 2
      }, function (aResponse) {
        do_check_true(!aResponse.error, "Should be able to set breakpoint.");
        gThreadClient.resume(test_black_box_breakpoint);
      });
    }
  });

  Components.utils.evalInSandbox(
    "" + function doStuff(k) { 
      let arg = 15;            
      k(arg);                  
    },                         
    gDebuggee,
    "1.8",
    BLACK_BOXED_URL,
    1
  );

  Components.utils.evalInSandbox(
    "" + function runTest() { 
      doStuff(                
        function (n) {        
          debugger;           
        }                     
      );                      
    }                         
    + "\n debugger;",         
    gDebuggee,
    "1.8",
    SOURCE_URL,
    1
  );
}

function test_black_box_breakpoint() {
  gThreadClient.getSources(function ({error, sources}) {
    do_check_true(!error, "Should not get an error: " + error);
    let sourceClient = gThreadClient.source(sources.filter(s => s.url == BLACK_BOXED_URL)[0]);
    sourceClient.blackBox(function ({error}) {
      do_check_true(!error, "Should not get an error: " + error);

      gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        do_check_eq(aPacket.why.type, "debuggerStatement",
                    "We should pass over the breakpoint since the source is black boxed.");
        gThreadClient.resume(test_unblack_box_breakpoint.bind(null, sourceClient));
      });
      gDebuggee.runTest();
    });
  });
}

function test_unblack_box_breakpoint(aSourceClient) {
  aSourceClient.unblackBox(function ({error}) {
    do_check_true(!error, "Should not get an error: " + error);
    gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
      do_check_eq(aPacket.why.type, "breakpoint",
                  "We should hit the breakpoint again");

      
      gClient.addOneTimeListener(
        "paused",
        gThreadClient.resume.bind(
          gThreadClient,
          finishClient.bind(null, gClient)));
      gThreadClient.resume();
    });
    gDebuggee.runTest();
  });
}
