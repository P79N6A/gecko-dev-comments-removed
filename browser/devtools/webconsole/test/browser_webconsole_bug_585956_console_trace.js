




const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-585956-console-trace.html";

function test() {
  Task.spawn(runner).then(finishTest);

  function* runner() {
    let {tab} = yield loadTab("data:text/html;charset=utf8,<p>hello");
    let hud = yield openConsole(tab);

    content.location = TEST_URI;

    let [result] = yield waitForMessages({
      webconsole: hud,
      messages: [{
        name: "console.trace output",
        consoleTrace: {
          file: "test-bug-585956-console-trace.html",
          fn: "window.foobar585956c",
        },
      }],
    });

    let node = [...result.matched][0];
    ok(node, "found trace log node");

    let obj = node._messageObject;
    ok(obj, "console.trace message object");

    
    let stacktrace = [
      { filename: TEST_URI, lineNumber: 9, functionName: "window.foobar585956c", language: 2 },
      { filename: TEST_URI, lineNumber: 14, functionName: "foobar585956b", language: 2 },
      { filename: TEST_URI, lineNumber: 18, functionName: "foobar585956a", language: 2 },
      { filename: TEST_URI, lineNumber: 21, functionName: null, language: 2 }
    ];

    ok(obj._stacktrace, "found stacktrace object");
    is(obj._stacktrace.toSource(), stacktrace.toSource(), "stacktrace is correct");
    isnot(node.textContent.indexOf("bug-585956"), -1, "found file name");
  }
}
