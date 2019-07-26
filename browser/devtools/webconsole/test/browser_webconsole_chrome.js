






function test() {
  addTab("about:credits");
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testChrome);
  }, true);
}

function testChrome(hud) {
  ok(hud, "we have a console");

  ok(hud.iframe, "we have the console iframe");

  let jsterm = hud.jsterm;
  ok(jsterm, "we have a jsterm");

  let input = jsterm.inputNode;
  ok(hud.outputNode, "we have an output node");

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, function() {
    is(jsterm.completeNode.value, "    ment", "'docu' completion");
    executeSoon(finishTest);
  });
}

