






var gDebuggee;
var gClient;
var gThreadClient;

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
  gClient.addOneTimeListener("paused", function () {
    gThreadClient.setBreakpoint({
      url: SOURCE_URL,
      line: 2
    }, function (aResponse) {
      do_check_true(!aResponse.error, "Should be able to set breakpoint.");
      gThreadClient.resume(test_black_box_default);
    });
  });

  Components.utils.evalInSandbox(
    "" + function doStuff(k) { 
      let arg = 15;            
      k(arg);                  
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
          debugger;           
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

function test_black_box_default() {
  gThreadClient.getSources(function (aResponse) {
    do_check_true(!aResponse.error, "Should be able to get sources.");

    let sourceClient = gThreadClient.source(
      aResponse.sources.filter(s => s.url == BLACK_BOXED_URL)[0]);
    do_check_true(!sourceClient.isBlackBoxed,
                  "By default the source is not black boxed.");

    
    runTest(
      function onSteppedLocation(aLocation) {
        do_check_eq(aLocation.url, BLACK_BOXED_URL,
                    "Should step into `doStuff`.");
        do_check_eq(aLocation.line, 2,
                    "Should step into `doStuff`.");
      },
      function onDebuggerStatementFrames(aFrames) {
        do_check_true(!aFrames.some(f => f.isBlackBoxed));
      },
      test_black_boxing.bind(null, sourceClient)
    );
  });
}

function test_black_boxing(aSourceClient) {
  aSourceClient.blackBox(function (aResponse) {
    do_check_true(!aResponse.error, "Should not get an error black boxing.");
    do_check_true(aSourceClient.isBlackBoxed,
       "The source client should report itself as black boxed correctly.");

    
    
    runTest(
      function onSteppedLocation(aLocation) {
        do_check_eq(aLocation.url, SOURCE_URL,
                    "Should step through `doStuff`.");
        do_check_eq(aLocation.line, 3,
                    "Should step through `doStuff`.");
      },
      function onDebuggerStatementFrames(aFrames) {
        for (let f of aFrames) {
          if (f.where.url == BLACK_BOXED_URL) {
            do_check_true(f.isBlackBoxed, "Should be black boxed");
          } else {
            do_check_true(!f.isBlackBoxed, "Should not be black boxed")
          }
        }
      },
      test_unblack_boxing.bind(null, aSourceClient)
    );
  });
}

function test_unblack_boxing(aSourceClient) {
  aSourceClient.unblackBox(function (aResponse) {
    do_check_true(!aResponse.error, "Should not get an error un-black boxing");
    do_check_true(!aSourceClient.isBlackBoxed, "The source is not black boxed.");

    
    runTest(
      function onSteppedLocation(aLocation) {
        do_check_eq(aLocation.url, BLACK_BOXED_URL,
                    "Should step into `doStuff`.");
        do_check_eq(aLocation.line, 2,
                    "Should step into `doStuff`.");
      },
      function onDebuggerStatementFrames(aFrames) {
        do_check_true(!aFrames.some(f => f.isBlackBoxed));
      },
      finishClient.bind(null, gClient)
    );
  });
}

function runTest(aOnSteppedLocation, aOnDebuggerStatementFrames, aFinishedCallback) {
  gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    do_check_eq(aPacket.why.type, "breakpoint");
    gClient.addOneTimeListener("paused", function () {
      gClient.addOneTimeListener("paused", function () {
        getCurrentLocation(function (aLocation) {
          aOnSteppedLocation(aLocation);
          gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
            do_check_eq(aPacket.why.type, "debuggerStatement");
            gThreadClient.getFrames(0, 100, function ({frames}) {
              aOnDebuggerStatementFrames(frames);
              gThreadClient.resume(aFinishedCallback);
            });
          });
          gThreadClient.resume();
        });
      });
      gThreadClient.stepIn();
    });
    gThreadClient.stepIn();
  });

  gDebuggee.runTest();
}

function getCurrentLocation(aCallback) {
  gThreadClient.getFrames(0, 1, function ({frames, error}) {
    do_check_true(!error, "Should not get an error: " + error);
    let [{where}] = frames;
    aCallback(where);
  });
}
