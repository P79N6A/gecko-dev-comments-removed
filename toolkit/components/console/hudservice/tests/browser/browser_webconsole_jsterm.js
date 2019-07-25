







































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

let jsterm;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testJSTerm, false);
}

function checkResult(msg, desc, lines) {
  let labels = jsterm.outputNode.querySelectorAll(".jsterm-output-line");
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
  hud = HUDService.hudWeakReferences[hudId].get();
  jsterm = hud.jsterm;
  let outputNode = hudBox.querySelector(".hud-output-node");

  jsterm.clearOutput();
  jsterm.execute("'id=' + $('header').getAttribute('id')");
  checkResult("id=header", "$() worked", 1);

  jsterm.clearOutput();
  jsterm.execute("headerQuery = $$('h1')");
  jsterm.execute("'length=' + headerQuery.length");
  checkResult("length=1", "$$() worked", 2);

  jsterm.clearOutput();
  jsterm.execute("xpathQuery = $x('.//*', document.body);");
  jsterm.execute("'headerFound='  + (xpathQuery[0] == headerQuery[0])");
  checkResult("headerFound=true", "$x() worked", 2);

  
  jsterm.execute("clear()");
  checkResult("undefined", "clear() worked", 1);

  jsterm.clearOutput();
  jsterm.execute("'keysResult=' + (keys({b:1})[0] == 'b')");
  checkResult("keysResult=true", "keys() worked", 1);

  jsterm.clearOutput();
  jsterm.execute("'valuesResult=' + (values({b:1})[0] == 1)");
  checkResult("valuesResult=true", "values() worked", 1);

  jsterm.clearOutput();
  jsterm.execute("pprint({b:2, a:1})");
  
  let label = jsterm.outputNode.querySelector(".jsterm-output-line");
  is(label.textContent.trim(), "a: 1\n  b: 2", "pprint() worked");

  finishTest();
}
