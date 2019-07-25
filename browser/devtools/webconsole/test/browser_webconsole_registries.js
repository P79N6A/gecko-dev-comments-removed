










































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testRegistries, false);
}

function testRegistries() {
  browser.removeEventListener("DOMContentLoaded", testRegistries, false);

  openConsole();

  let hud = HUDService.getHudByWindow(content);
  ok(hud, "we have a HUD");
  ok(HUDService.hudReferences[hud.hudId], "we have a HUD in hudReferences");

  let windowID = HUDService.getWindowId(content);
  is(HUDService.windowIds[windowID], hud.hudId, "windowIds are working");

  finishTest();
}

