




const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let jsterm, testDriver;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, function(hud) {
      testDriver = testJSTerm(hud);
      testDriver.next();
    });
  }, true);
}

function nextTest() {
  testDriver.next();
}

function checkResult(msg, desc) {
  waitForMessages({
    webconsole: jsterm.hud.owner,
    messages: [{
      name: desc,
      category: CATEGORY_OUTPUT,
    }],
  }).then(([result]) => {
    let node = [...result.matched][0].querySelector(".message-body");
    if (typeof msg == "string") {
      is(node.textContent.trim(), msg,
        "correct message shown for " + desc);
    }
    else if (typeof msg == "function") {
      ok(msg(node), "correct message shown for " + desc);
    }

    nextTest();
  });
}

function testJSTerm(hud)
{
  jsterm = hud.jsterm;
  const HELP_URL = "https://developer.mozilla.org/docs/Tools/Web_Console/Helpers";

  jsterm.clearOutput();
  jsterm.execute("$('#header').getAttribute('id')");
  checkResult('"header"', "$() worked");
  yield undefined;

  jsterm.clearOutput();
  jsterm.execute("$$('h1').length");
  checkResult("1", "$$() worked");
  yield undefined;

  jsterm.clearOutput();
  jsterm.execute("$x('.//*', document.body)[0] == $$('h1')[0]");
  checkResult("true", "$x() worked");
  yield undefined;

  
  jsterm.execute("clear()");

  waitForSuccess({
    name: "clear() worked",
    validatorFn: function()
    {
      return jsterm.outputNode.childNodes.length == 0;
    },
    successFn: nextTest,
    failureFn: nextTest,
  });

  yield undefined;

  jsterm.clearOutput();
  jsterm.execute("keys({b:1})[0] == 'b'");
  checkResult("true", "keys() worked", 1);
  yield undefined;

  jsterm.clearOutput();
  jsterm.execute("values({b:1})[0] == 1");
  checkResult("true", "values() worked", 1);
  yield undefined;

  jsterm.clearOutput();

  let openedLinks = 0;
  let onExecuteCalls = 0;
  let oldOpenLink = hud.openLink;
  hud.openLink = (url) => {
    if (url == HELP_URL) {
      openedLinks++;
    }
  };

  function onExecute() {
    onExecuteCalls++;
    if (onExecuteCalls == 3) {
      nextTest();
    }
  }

  jsterm.execute("help()", onExecute);
  jsterm.execute("help", onExecute);
  jsterm.execute("?", onExecute);
  yield undefined;

  let output = jsterm.outputNode.querySelector(".message[category='output']");
  ok(!output, "no output for help() calls");
  is(openedLinks, 3, "correct number of pages opened by the help calls");
  hud.openLink = oldOpenLink;

  jsterm.clearOutput();
  jsterm.execute("pprint({b:2, a:1})");
  checkResult("\"  b: 2\n  a: 1\"", "pprint()");
  yield undefined;

  
  jsterm.clearOutput();
  jsterm.execute("[] instanceof Array");
  checkResult("true", "[] instanceof Array == true");
  yield undefined;

  jsterm.clearOutput();
  jsterm.execute("({}) instanceof Object");
  checkResult("true", "({}) instanceof Object == true");
  yield undefined;

  
  jsterm.clearOutput();
  jsterm.execute("document");
  checkResult(function(node) {
    return node.textContent.search(/\[object xraywrapper/i) == -1;
  }, "document - no XrayWrapper");
  yield undefined;

  
  jsterm.clearOutput();
  jsterm.execute("pprint(window)");
  checkResult(null, "pprint(window)");
  yield undefined;

  jsterm.clearOutput();
  jsterm.execute("keys(window)");
  checkResult(null, "keys(window)");
  yield undefined;

  
  jsterm.clearOutput();
  jsterm.execute("pprint('hi')");
  checkResult("\"  0: \"h\"\n  1: \"i\"\"", "pprint('hi')");
  yield undefined;

  
  jsterm.clearOutput();
  jsterm.execute("pprint(function() { var someCanaryValue = 42; })");
  checkResult(function(node) {
    return node.textContent.indexOf("someCanaryValue") > -1;
  }, "pprint(function) shows source");
  yield undefined;

  
  jsterm.clearOutput();
  jsterm.execute("null");
  checkResult("null", "null is null");
  yield undefined;

  jsterm.clearOutput();
  jsterm.execute("undefined");
  checkResult("undefined", "undefined is printed");
  yield undefined;

  jsterm = testDriver = null;
  executeSoon(finishTest);
  yield undefined;
}
