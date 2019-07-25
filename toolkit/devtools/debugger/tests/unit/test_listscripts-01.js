






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
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
    gThreadClient.getScripts(function (aResponse) {
      let script = aResponse.scripts
        .filter(function (s) {
          return s.url.match(/test_listscripts-01.js$/);
        })[0];
      
      do_check_true(!!script);
      do_check_eq(script.url, path);
      do_check_eq(script.startLine, 46);
      do_check_eq(script.lineCount, 4);
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
