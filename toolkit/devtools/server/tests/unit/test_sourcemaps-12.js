







var gDebuggee;
var gClient;
var gThreadClient;

Components.utils.import('resource:///modules/devtools/SourceMap.jsm');

function run_test() {
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-source-map");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-source-map", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      define_code();
    });
  });
  do_test_pending();
}

function define_code() {
  let { code, map } = (new SourceNode(null, null, null, [
    new SourceNode(1, 0, "a.js", "function runTest() {\n"),
    
    new SourceNode(2, 0, "a.js", "  debugger;\n"),
    new SourceNode(2, 0, "a.js", "  var sum = 0;\n"),
    new SourceNode(2, 0, "a.js", "  for (var i = 0; i < 5; i++) {\n"),
    new SourceNode(2, 0, "a.js", "    sum += i;\n"),
    new SourceNode(2, 0, "a.js", "  }\n"),
    
    new SourceNode(3, 0, "a.js", "  sum;\n"),
    new SourceNode(3, 0, "a.js", "}\n"),
  ])).toStringWithSourceMap({
    file: "abc.js",
    sourceRoot: "http://example.com/"
  });

  code += "//# sourceMappingURL=data:text/json," + map.toString();

  Components.utils.evalInSandbox(code, gDebuggee, "1.8",
                                 "http://example.com/abc.js", 1);

  run_code();
}

function run_code() {
  gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    do_check_eq(aPacket.why.type, "debuggerStatement");
    step_in();
  });
  gDebuggee.runTest();
}

function step_in() {
  gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    do_check_eq(aPacket.why.type, "resumeLimit");
    let { frame: { environment, where: { source, line } } } = aPacket;
    
    do_check_eq(source.url, "http://example.com/a.js");
    do_check_eq(line, 3);
    
    
    do_check_eq(environment.bindings.variables.sum.value, 10);
    finishClient(gClient);
  });
  gThreadClient.stepIn();
}

