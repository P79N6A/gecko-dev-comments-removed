


var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");
  gDebuggee.eval(function stopMe(arg1) {
    debugger;
  }.toString());

  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestGlobalClientAndResume(gClient, "test-grips", function(aResponse, aThreadClient) {
      gThreadClient = aThreadClient;
      test_named_function();
    });
  });
  do_test_pending();
}

function test_named_function()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    let args = aPacket.frame.arguments;

    do_check_eq(args[0].class, "Function");
    do_check_eq(args[0].name, "stopMe");

    let objClient = gThreadClient.pauseGrip(args[0]);
    objClient.getSignature(function(aResponse) {
      do_check_eq(aResponse.name, "stopMe");
      do_check_eq(aResponse.parameters.length, 1);
      do_check_eq(aResponse.parameters[0], "arg1");

      gThreadClient.resume(test_inferred_name_function);
    });

  });

  gDebuggee.eval("stopMe(stopMe)");
}

function test_inferred_name_function() {
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    let args = aPacket.frame.arguments;

    do_check_eq(args[0].class, "Function");
    
    do_check_eq(args[0].name, undefined);
    do_check_eq(args[0].displayName, "o.m");

    let objClient = gThreadClient.pauseGrip(args[0]);
    objClient.getSignature(function(aResponse) {
      do_check_eq(aResponse.name, null);
      do_check_eq(aResponse.parameters.length, 3);
      do_check_eq(aResponse.parameters[0], "foo");
      do_check_eq(aResponse.parameters[1], "bar");
      do_check_eq(aResponse.parameters[2], "baz");

      gThreadClient.resume(test_anonymous_function);
    });
  });

  gDebuggee.eval("var o = { m: function(foo, bar, baz) { } }; stopMe(o.m)");
}

function test_anonymous_function() {
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    let args = aPacket.frame.arguments;

    do_check_eq(args[0].class, "Function");
    
    do_check_eq(args[0].name, undefined);
    do_check_eq(args[0].displayName, undefined);

    let objClient = gThreadClient.pauseGrip(args[0]);
    objClient.getSignature(function(aResponse) {
      do_check_eq(aResponse.name, null);
      do_check_eq(aResponse.parameters.length, 3);
      do_check_eq(aResponse.parameters[0], "foo");
      do_check_eq(aResponse.parameters[1], "bar");
      do_check_eq(aResponse.parameters[2], "baz");

      gThreadClient.resume(function() {
        finishClient(gClient);
      });
    });
  });

  gDebuggee.eval("stopMe(function(foo, bar, baz) { })");
}

