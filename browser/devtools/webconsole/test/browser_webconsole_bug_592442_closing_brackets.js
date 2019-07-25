














function test() {
  addTab("data:text/html;charset=utf-8,test for bug 592442");
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testExtraneousClosingBrackets);
  }, true);
}

function testExtraneousClosingBrackets(hud) {
  let jsterm = hud.jsterm;

  jsterm.setInputValue("document.getElementById)");

  let error = false;
  try {
    jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  }
  catch (ex) {
    error = true;
  }

  ok(!error, "no error was thrown when an extraneous bracket was inserted");

  finishTest();
}

