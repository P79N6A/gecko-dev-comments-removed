








































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

let jsterm;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testJSTerm, false);
}

function checkResult(msg, desc, lines) {
  let labels = jsterm.outputNode.querySelectorAll(".webconsole-msg-output");
  is(labels.length, lines, "correct number of results shown for " + desc);
  is(labels[lines-1].textContent.trim(), msg, "correct message shown for " +
    desc);
}

function testJSTerm()
{
  browser.removeEventListener("DOMContentLoaded", testJSTerm, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  hud = HUDService.hudReferences[hudId];
  jsterm = hud.jsterm;
  let outputNode = hudBox.querySelector(".hud-output-node");

  jsterm.clearOutput();
  jsterm.execute("'id=' + $('header').getAttribute('id')");
  checkResult('"id=header"', "$() worked", 1);

  jsterm.clearOutput();
  jsterm.execute("headerQuery = $$('h1')");
  jsterm.execute("'length=' + headerQuery.length");
  checkResult('"length=1"', "$$() worked", 2);

  jsterm.clearOutput();
  jsterm.execute("xpathQuery = $x('.//*', document.body);");
  jsterm.execute("'headerFound='  + (xpathQuery[0] == headerQuery[0])");
  checkResult('"headerFound=true"', "$x() worked", 2);

  
  jsterm.execute("clear()");
  let group = jsterm.outputNode.querySelector(".hud-group");
  ok(!group, "clear() worked");

  jsterm.clearOutput();
  jsterm.execute("'keysResult=' + (keys({b:1})[0] == 'b')");
  checkResult('"keysResult=true"', "keys() worked", 1);

  jsterm.clearOutput();
  jsterm.execute("'valuesResult=' + (values({b:1})[0] == 1)");
  checkResult('"valuesResult=true"', "values() worked", 1);

  jsterm.clearOutput();
  jsterm.execute("help()");
  let output = jsterm.outputNode.querySelector(".webconsole-msg-output");
  ok(!group, "help() worked");

  jsterm.execute("help");
  output = jsterm.outputNode.querySelector(".webconsole-msg-output");
  ok(!output, "help worked");

  jsterm.execute("?");
  output = jsterm.outputNode.querySelector(".webconsole-msg-output");
  ok(!output, "? worked");

  jsterm.clearOutput();
  jsterm.execute("pprint({b:2, a:1})");
  
  let label = jsterm.outputNode.querySelector(".webconsole-msg-output");
  is(label.textContent.trim(), "a: 1\n  b: 2", "pprint() worked");

  
  jsterm.clearOutput();
  jsterm.execute("[] instanceof Array");
  checkResult("true", "[] instanceof Array == true", 1);

  jsterm.clearOutput();
  jsterm.execute("({}) instanceof Object");
  checkResult("true", "({}) instanceof Object == true", 1);

  
  jsterm.clearOutput();
  jsterm.execute("document");
  let label = jsterm.outputNode.querySelector(".webconsole-msg-output");
  is(label.textContent.trim().search(/\[object XrayWrapper/), -1,
    "check for non-existence of [object XrayWrapper ");

  
  jsterm.clearOutput();
  jsterm.execute("pprint(window)");
  let labels = jsterm.outputNode.querySelectorAll(".webconsole-msg-output");
  is(labels.length, 1, "one line of output for pprint(window)");

  jsterm.clearOutput();
  jsterm.execute("keys(window)");
  labels = jsterm.outputNode.querySelectorAll(".webconsole-msg-output");
  is(labels.length, 1, "one line of output for keys(window)");

  jsterm.clearOutput();
  jsterm.execute("pprint('hi')");
  
  let label = jsterm.outputNode.querySelector(".webconsole-msg-output");
  is(label.textContent.trim(), '0: "h"\n  1: "i"', 'pprint("hi") worked');

  
  jsterm.clearOutput();
  jsterm.execute("pprint(print)");
  label = jsterm.outputNode.querySelector(".webconsole-msg-output");
  isnot(label.textContent.indexOf("SEVERITY_LOG"), -1,
        "pprint(function) shows function source");

  
  jsterm.clearOutput();
  jsterm.execute("null");
  checkResult("null", "null is null", 1);

  finishTest();
}
