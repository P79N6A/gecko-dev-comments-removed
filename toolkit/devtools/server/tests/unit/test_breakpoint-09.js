






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function () {
    attachTestTabAndResume(gClient, "test-stack", function (aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_remove_breakpoint();
    });
  });
  do_test_pending();
}

function test_remove_breakpoint()
{
  let done = false;
  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    let path = getFilePath('test_breakpoint-09.js');
    let location = { url: path, line: gDebuggee.line0 + 2};
    gThreadClient.setBreakpoint(location, function (aResponse, bpClient) {
      gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
        
        do_check_eq(aPacket.type, "paused");
        do_check_eq(aPacket.frame.where.url, path);
        do_check_eq(aPacket.frame.where.line, location.line);
        do_check_eq(aPacket.why.type, "breakpoint");
        do_check_eq(aPacket.why.actors[0], bpClient.actor);
        
        do_check_eq(gDebuggee.a, undefined);

        
        bpClient.remove(function (aResponse) {
          done = true;
          gThreadClient.addOneTimeListener("paused",
                                           function (aEvent, aPacket) {
            
            gThreadClient.resume(function () {
              do_check_true(false);
            });
          });
          gThreadClient.resume();
        });

      });
      
      gThreadClient.resume();

    });

  });

  gDebuggee.eval("var line0 = Error().lineNumber;\n" +
                 "function foo(stop) {\n" + 
                 "  this.a = 1;\n" +        
                 "  if (stop) return;\n" +  
                 "  delete this.a;\n" +     
                 "  foo(true);\n" +         
                 "}\n" +                    
                 "debugger;\n" +            
                 "foo();\n");               
  if (!done) {
    do_check_true(false);
  }
  finishClient(gClient);
}
