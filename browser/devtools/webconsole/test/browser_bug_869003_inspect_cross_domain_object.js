







"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-bug-869003-top-window.html";

let test = asyncTest(function* () {
  
  
  
  requestLongerTimeout(2);

  yield loadTab("data:text/html;charset=utf8,<p>hello");
  let hud = yield openConsole();

  content.location = TEST_URI;

  let [result] = yield waitForMessages({
    webconsole: hud,
    messages: [{
      name: "console.log message",
      text: "foobar",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      objects: true,
    }],
  });

  let msg = [...result.matched][0];
  ok(msg, "message element");

  let body = msg.querySelector(".message-body");
  ok(body, "message body");

  let clickable = result.clickableElements[0];
  ok(clickable, "clickable object found");
  ok(body.textContent.includes('{ hello: "world!",'), "message text check");

  executeSoon(() => {
    EventUtils.synthesizeMouse(clickable, 2, 2, {}, hud.iframeWindow)
  });

  let aVar = yield hud.jsterm.once("variablesview-fetched");
  ok(aVar, "variables view fetched");
  ok(aVar._variablesView, "variables view object");

  [result] = yield findVariableViewProperties(aVar, [
    { name: "hello", value: "world!" },
    { name: "bug", value: 869003 },
  ], { webconsole: hud });

  let prop = result.matchedProp;
  ok(prop, "matched the |hello| property in the variables view");

  
  aVar = yield updateVariablesViewProperty({
    property: prop,
    field: "value",
    string: "'omgtest'",
    webconsole: hud,
  });

  info("onFetchAfterUpdate");

  yield findVariableViewProperties(aVar, [
    { name: "hello", value: "omgtest" },
    { name: "bug", value: 869003 },
  ], { webconsole: hud });
});

