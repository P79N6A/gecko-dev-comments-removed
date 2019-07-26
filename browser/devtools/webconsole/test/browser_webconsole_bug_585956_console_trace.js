




const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-585956-console-trace.html";

function test() {
  addTab("data:text/html;charset=utf8,<p>hello");
  browser.addEventListener("load", tabLoaded, true);

  function tabLoaded() {
    browser.removeEventListener("load", tabLoaded, true);

    openConsole(null, function(hud) {
      content.location = TEST_URI;

      waitForMessages({
        webconsole: hud,
        messages: [{
          name: "console.trace output",
          consoleTrace: {
            file: "test-bug-585956-console-trace.html",
            fn: "window.foobar585956c",
          },
        }],
      }).then(performChecks);
    });
  }

  function performChecks(results) {
    let node = [...results[0].matched][0];

    
    let stacktrace = [
      { filename: TEST_URI, lineNumber: 9, functionName: "window.foobar585956c", language: 2 },
      { filename: TEST_URI, lineNumber: 14, functionName: "foobar585956b", language: 2 },
      { filename: TEST_URI, lineNumber: 18, functionName: "foobar585956a", language: 2 },
      { filename: TEST_URI, lineNumber: 21, functionName: null, language: 2 }
    ];

    ok(node, "found trace log node");
    ok(node._stacktrace, "found stacktrace object");
    is(node._stacktrace.toSource(), stacktrace.toSource(), "stacktrace is correct");
    isnot(node.textContent.indexOf("bug-585956"), -1, "found file name");

    finishTest();
  }
}
