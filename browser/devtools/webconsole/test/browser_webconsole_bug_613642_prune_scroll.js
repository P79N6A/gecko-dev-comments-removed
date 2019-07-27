








"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for " +
                 "bug 613642: maintain scroll with pruning of old messages";

let hud;

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  hud = yield openConsole();

  hud.jsterm.clearOutput();

  let outputNode = hud.outputNode;

  Services.prefs.setIntPref("devtools.hud.loglimit.console", 140);
  let scrollBoxElement = outputNode.parentNode;

  for (let i = 0; i < 150; i++) {
    content.console.log("test message " + i);
  }

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "test message 149",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });

  let oldScrollTop = scrollBoxElement.scrollTop;
  isnot(oldScrollTop, 0, "scroll location is not at the top");

  let firstNode = outputNode.firstChild;
  ok(firstNode, "found the first message");

  let msgNode = outputNode.children[80];
  ok(msgNode, "found the 80th message");

  
  msgNode.scrollIntoView(false);

  isnot(scrollBoxElement.scrollTop, oldScrollTop,
        "scroll location updated (scrolled to message)");

  oldScrollTop = scrollBoxElement.scrollTop;

  
  content.console.log("hello world");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "hello world",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });

  
  
  isnot(scrollBoxElement.scrollTop, oldScrollTop,
        "scroll location updated (added a message)");

  isnot(outputNode.firstChild, firstNode,
        "first message removed");

  Services.prefs.clearUserPref("devtools.hud.loglimit.console");

  hud = null;
});
