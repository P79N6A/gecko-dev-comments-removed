









const TEST_URI = "http://example.com/";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    Services.prefs.setIntPref("devtools.webconsole.fontSize", 10);
    openConsole(null, testFontSizeChange);
  }, true);
}

function testFontSizeChange(hud) {
  let inputNode = hud.jsterm.inputNode;
  let outputNode = hud.jsterm.outputNode;
  outputNode.focus();

  EventUtils.synthesizeKey("-", { accelKey: true });
  is(inputNode.style.fontSize, "10px", "input font stays at same size with ctrl+-");
  is(outputNode.style.fontSize, inputNode.style.fontSize, "output font stays at same size with ctrl+-");

  EventUtils.synthesizeKey("=", { accelKey: true });
  is(inputNode.style.fontSize, "11px", "input font increased with ctrl+=");
  is(outputNode.style.fontSize, inputNode.style.fontSize, "output font stays at same size with ctrl+=");

  EventUtils.synthesizeKey("-", { accelKey: true });
  is(inputNode.style.fontSize, "10px", "font decreased with ctrl+-");
  is(outputNode.style.fontSize, inputNode.style.fontSize, "output font stays at same size with ctrl+-");

  EventUtils.synthesizeKey("0", { accelKey: true });
  is(inputNode.style.fontSize, "", "font reset with ctrl+0");
  is(outputNode.style.fontSize, inputNode.style.fontSize, "output font stays at same size with ctrl+0");

  finishTest();
}
