






var gDebuggee;
var gClient;
var gThreadClient;
var gBpClient;

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
    let source = gThreadClient.source(aPacket.frame.where.source);
    source.setBreakpoint({
      line: 4
    }, function ({error}, bpClient) {
      gBpClient = bpClient;
      do_check_true(!error, "Should not get an error: " + error);
      gThreadClient.resume(test_black_box_dbg_statement);
    });
  });

  Components.utils.evalInSandbox(
    "" + function doStuff(k) { 
      debugger;                
      k(100);                  
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
          Math.abs(n);        
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

function test_black_box_dbg_statement() {
  gThreadClient.getSources(function ({error, sources}) {
    do_check_true(!error, "Should not get an error: " + error);
    let sourceClient = gThreadClient.source(sources.filter(s => s.url == BLACK_BOXED_URL)[0]);

    sourceClient.blackBox(function ({error}) {
      do_check_true(!error, "Should not get an error: " + error);

      gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        do_check_eq(aPacket.why.type, "breakpoint",
                    "We should pass over the debugger statement.");
        gBpClient.remove(function ({error}) {
          do_check_true(!error, "Should not get an error: " + error);
          gThreadClient.resume(test_unblack_box_dbg_statement.bind(null, sourceClient));
        });
      });
      gDebuggee.runTest();
    });
  });
}

function test_unblack_box_dbg_statement(aSourceClient) {
  aSourceClient.unblackBox(function ({error}) {
    do_check_true(!error, "Should not get an error: " + error);

    gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
      do_check_eq(aPacket.why.type, "debuggerStatement",
                  "We should stop at the debugger statement again");
      finishClient(gClient);
    });
    gDebuggee.runTest();
  });
}
