






var gDebuggee;
var gClient;
var gThreadClient;

var gNumTimesSourcesSent = 0;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.request = (function (request) {
    return function (aRequest, aOnResponse) {
      if (aRequest.type === "sources") {
        ++gNumTimesSourcesSent;
      }
      return request.call(this, aRequest, aOnResponse);
    };
  }(gClient.request));
  gClient.connect(function () {
    attachTestGlobalClientAndResume(gClient, "test-stack", function (aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_simple_listsources();
    });
  });
  do_test_pending();
}

function test_simple_listsources()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    gThreadClient.getSources(function (aResponse) {
      do_check_true(aResponse.sources.some(function (s) {
        return s.url.match(/test_listsources-01.js$/);
      }));

      do_check_true(gNumTimesSourcesSent <= 1,
                    "Should only send one sources request at most, even though we"
                    + " might have had to send one to determine feature support.");

      gThreadClient.resume(function () {
        finishClient(gClient);
      });
    });
  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
       "debugger;\n" +   
       "var a = 1;\n" +  
       "var b = 2;\n");  
}
