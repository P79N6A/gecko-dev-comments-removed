






const TEST_URI = "data:text/html;charset=utf-8,Web Console test for bug 664131: Expand console object with group methods";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  let jsterm = hud.jsterm;
  let outputNode = hud.outputNode;

  hud.jsterm.clearOutput();

  jsterm.execute("console.group('bug664131a')")

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131a",
      consoleGroup: 1,
    }],
  });

  jsterm.execute("console.log('bug664131a-inside')")

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131a-inside",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      groupDepth: 1,
    }],
  });

  jsterm.execute('console.groupEnd("bug664131a")');
  jsterm.execute('console.log("bug664131-outside")');

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131-outside",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
      groupDepth: 0,
    }],
  });

  jsterm.execute('console.groupCollapsed("bug664131b")');

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug664131b",
      consoleGroup: 1,
    }],
  });

  
  hud.jsterm.clearOutput();
  jsterm.execute('console.log("bug664131-cleared")');

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
