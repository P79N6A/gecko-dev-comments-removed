







"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for " +
                 "bug 664131: Expand console object with group methods";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  let jsterm = hud.jsterm;

  hud.jsterm.clearOutput();

  yield jsterm.execute("console.group('bug664131a')");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131a",
      consoleGroup: 1,
    }],
  });

  yield jsterm.execute("console.log('bug664131a-inside')");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131a-inside",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      groupDepth: 1,
    }],
  });

  yield jsterm.execute('console.groupEnd("bug664131a")');
  yield jsterm.execute('console.log("bug664131-outside")');

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131-outside",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      groupDepth: 0,
    }],
  });

  yield jsterm.execute('console.groupCollapsed("bug664131b")');

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131b",
      consoleGroup: 1,
    }],
  });

  
  hud.jsterm.clearOutput();
  yield jsterm.execute('console.log("bug664131-cleared")');

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131-cleared",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      groupDepth: 0,
    }],
  });
});
