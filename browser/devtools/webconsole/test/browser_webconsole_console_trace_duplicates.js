



function test() {
  const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug_939783_console_trace_duplicates.html";

  Task.spawn(runner).then(finishTest);

  function* runner() {
    const {tab} = yield loadTab("data:text/html;charset=utf8,<p>hello");
    const hud = yield openConsole(tab);

    content.location = TEST_URI;

    
    
    
    
    yield waitForMessages({
      webconsole: hud,
      messages: [{
        name: "console.trace output for foo1()",
        text: "foo1()",
        consoleTrace: {
          file: "test-bug_939783_console_trace_duplicates.html",
          fn: "foo3()",
        },
      }, {
        name: "console.trace output for foo1()",
        text: "foo1()",
        consoleTrace: {
          file: "test-bug_939783_console_trace_duplicates.html",
          fn: "foo3()",
        },
      }, {
        name: "console.trace output for foo1b()",
        text: "foo1b()",
        consoleTrace: {
          file: "test-bug_939783_console_trace_duplicates.html",
          fn: "foo3()",
        },
      }],
    });
  }
}
