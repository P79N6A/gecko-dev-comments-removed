







"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();

  let jsterm = hud.jsterm;
  let outputNode = jsterm.outputNode;

  jsterm.clearOutput();
  jsterm.execute("console.log('foo', 'bar');");

  let [function_call, result, console_message] = yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "console.log('foo', 'bar');",
      category: CATEGORY_INPUT,
    },
    {
      text: "undefined",
      category: CATEGORY_OUTPUT,
    },
    {
      text: '"foo" "bar"',
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });

  let fncall_node = [...function_call.matched][0];
  let result_node = [...result.matched][0];
  let console_message_node = [...console_message.matched][0];
  is(fncall_node.nextElementSibling, result_node,
     "console.log() is followed by undefined");
  is(result_node.nextElementSibling, console_message_node,
     "undefined is followed by 'foo' 'bar'");
});
