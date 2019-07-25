














function test() {
  addTab("data:text/html,test for bug 592442");
  browser.addEventListener("load", testExtraneousClosingBrackets, true);
}

function testExtraneousClosingBrackets(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  openConsole();
  let hudId = HUDService.displaysIndex()[0];
  let jsterm = HUDService.hudWeakReferences[hudId].get().jsterm;

  jsterm.setInputValue("document.getElementById)");

  let error = false;
  try {
    jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  }
  catch (ex) {
    error = true;
  }

  ok(!error, "no error was thrown when an extraneous bracket was inserted");

  HUDService.deactivateHUDForContext(tab);
  finishTest();
}

