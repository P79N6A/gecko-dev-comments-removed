






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.ready(function () {
    attachTestGlobalClientAndResume(gClient, "test-stack", function (aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_simple_listscripts();
    });
  });
  do_test_pending();
}

function test_simple_listscripts()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    var path = getFilePath('test_listscripts-01.js');
    gThreadClient.scripts(function (aResponse) {
        
        do_check_eq(aResponse.scripts[0].url, path);
        do_check_eq(aResponse.scripts[0].startLine, 41);
        do_check_eq(aResponse.scripts[0].lineCount, 4);
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
