





var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-grips");
  gDebuggee.eval(function stopMe() {
    debugger;
  }.toString());

  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-grips", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      add_pause_listener();
    });
  });
  do_test_pending();
}

function add_pause_listener()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket) {
    const [funcGrip, objGrip] = aPacket.frame.arguments;
    const func = gThreadClient.pauseGrip(funcGrip);
    const obj = gThreadClient.pauseGrip(objGrip);
    test_definition_site(func, obj);
  });

  eval_code();
}

function eval_code() {
  gDebuggee.eval([
    "this.line0 = Error().lineNumber;",
    "function f() {}",
    "stopMe(f, {});"
  ].join("\n"));
}

function test_definition_site(func, obj) {
  func.getDefinitionSite(({ error, url, line, column }) => {
    do_check_true(!error);
    do_check_eq(url, getFilePath("test_objectgrips-13.js"));
    do_check_eq(line, gDebuggee.line0 + 1);
    do_check_eq(column, 0);

    test_bad_definition_site(obj);
  });
}

function test_bad_definition_site(obj) {
  try {
    obj.getDefinitionSite(() => do_check_true(false));
  } catch (e) {
    gThreadClient.resume(() => finishClient(gClient));
  }
}
