







var gDebuggee;
var gClient;
var gThreadClient;
var gCallback;

function run_test()
{
  run_test_with_server(DebuggerServer, function () {
    run_test_with_server(WorkerDebuggerServer, do_test_finished);
  });
  do_test_pending();
};

function run_test_with_server(aServer, aCallback)
{
  gCallback = aCallback;
  initTestDebuggerServer(aServer);
  gDebuggee = addTestGlobal("test-stack", aServer);
  gClient = new DebuggerClient(aServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-stack", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_nested_breakpoint();
    });
  });
}

function test_nested_breakpoint()
{
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let path = getFilePath('test_breakpoint-06.js');
    let location = { url: path, line: gDebuggee.line0 + 5};
    gThreadClient.setBreakpoint(location, function (aResponse, bpClient) {
      
      do_check_eq(aResponse.actualLocation.url, location.url);
      do_check_eq(aResponse.actualLocation.line, location.line + 1);
      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.frame.where.url, path);
        do_check_eq(aPacket.frame.where.line, location.line + 1);
        do_check_eq(aPacket.why.type, "breakpoint");
        do_check_eq(aPacket.why.actors[0], bpClient.actor);
        
        do_check_eq(gDebuggee.a, 1);
        do_check_eq(gDebuggee.b, undefined);

        
        bpClient.remove(function (aResponse) {
          gThreadClient.resume(function () {
            gClient.close(gCallback);
          });
        });

      });
      
      gThreadClient.resume();

    });

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "function foo() {\n" +     
                 "  function bar() {\n" +   
                 "    function baz() {\n" + 
                 "      this.a = 1;\n" +    
                 "      // A comment.\n" +  
                 "      this.b = 2;\n" +    
                 "    }\n" +                
                 "    baz();\n" +           
                 "  }\n" +                  
                 "  bar();\n" +             
                 "}\n" +                    
                 "debugger;\n" +            
                 "foo();\n");               
}
