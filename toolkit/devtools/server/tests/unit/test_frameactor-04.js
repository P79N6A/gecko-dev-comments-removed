






var gDebuggee;
var gClient;
var gThreadClient;

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-stack");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-stack", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_pause_frame();
    });
  });
  do_test_pending();
}

var gFrames = [
  
  { type: "call", callee: { name: "depth3" } },
  { type: "call", callee: { name: "depth2" } },
  { type: "call", callee: { name: "depth1" } },

  
  { type: "call", callee: { name: undefined } },

  
  { type: "eval", callee: { name: undefined } },
];

var gSliceTests = [
  { start: 0, count: undefined, resetActors: true },
  { start: 0, count: 1 },
  { start: 2, count: 2 },
  { start: 1, count: 15 },
  { start: 15, count: undefined },
];

function test_frame_slice() {
  if (gSliceTests.length == 0) {
    gThreadClient.resume(function() { finishClient(gClient); });
    return;
  }

  let test = gSliceTests.shift();
  gThreadClient.getFrames(test.start, test.count, function(aResponse) {
    var testFrames = gFrames.slice(test.start, test.count ? test.start + test.count : undefined);
    do_check_eq(testFrames.length, aResponse.frames.length);
    for (var i = 0; i < testFrames.length; i++) {
      let expected = testFrames[i];
      let actual = aResponse.frames[i];

      if (test.resetActors) {
        expected.actor = actual.actor;
      }

      for (let key of ["type", "callee-name"]) {
        do_check_eq(expected[key] || undefined, actual[key]);
      }
    }
    test_frame_slice();
  });
}

function test_pause_frame()
{
  gThreadClient.addOneTimeListener("paused", function(aEvent, aPacket1) {
    test_frame_slice();
  });

  gDebuggee.eval("(" + function() {
    function depth3() {
      debugger;
    }
    function depth2() {
      depth3();
    }
    function depth1() {
      depth2();
    };
    depth1();
  } + ")()");
}
