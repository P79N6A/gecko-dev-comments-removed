






var gDebuggee;
var gClient;
var gThreadClient;

Components.utils.import('resource:///modules/devtools/SourceMap.jsm');

function run_test()
{
  initTestDebuggerServer();
  gDebuggee = addTestGlobal("test-source-map");
  gClient = new DebuggerClient(DebuggerServer.connectPipe());
  gClient.connect(function() {
    attachTestTabAndResume(gClient, "test-source-map", function(aResponse, aTabClient, aThreadClient) {
      gThreadClient = aThreadClient;
      test_minified();
    });
  });
  do_test_pending();
}

function test_minified()
{
  let newSourceFired = false;

  gThreadClient.addOneTimeListener("newSource", function _onNewSource(aEvent, aPacket) {
    do_check_eq(aEvent, "newSource");
    do_check_eq(aPacket.type, "newSource");
    do_check_true(!!aPacket.source);

    do_check_eq(aPacket.source.url, "http://example.com/foo.js",
                "The new source should be foo.js");
    do_check_eq(aPacket.source.url.indexOf("foo.min.js"), -1,
                "The new source should not be the minified file");

    newSourceFired = true;
  });

  gThreadClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    do_check_eq(aEvent, "paused");
    do_check_eq(aPacket.why.type, "debuggerStatement");

    let location = {
      line: 5
    };

    getSource(gThreadClient, "http://example.com/foo.js").then(source => {
      source.setBreakpoint(location, function (aResponse, bpClient) {
        do_check_true(!aResponse.error);
        testHitBreakpoint();
      });
    })
  });

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  let code = '(function(){debugger;function r(r){var n=r+r;var u=null;return n}for(var n=0;n<10;n++){r(n)}})();\n//# sourceMappingURL=data:text/json,{"file":"foo.min.js","version":3,"sources":["foo.js"],"names":["foo","n","bar","unused","i"],"mappings":"CAAC,WACC,QACA,SAASA,GAAIC,GACX,GAAIC,GAAMD,EAAIA,CACd,IAAIE,GAAS,IACb,OAAOD,GAET,IAAK,GAAIE,GAAI,EAAGA,EAAI,GAAIA,IAAK,CAC3BJ,EAAII"}';

  Components.utils.evalInSandbox(code, gDebuggee, "1.8",
                                 "http://example.com/foo.min.js", 1);
}

function testHitBreakpoint(timesHit=0) {
  gClient.addOneTimeListener("paused", function (aEvent, aPacket) {
    ++timesHit;

    do_check_eq(aEvent, "paused");
    do_check_eq(aPacket.why.type, "breakpoint");

    if (timesHit === 10) {
      gThreadClient.resume(() => finishClient(gClient));
    } else {
      testHitBreakpoint(timesHit);
    }
  });

  gThreadClient.resume();
}
