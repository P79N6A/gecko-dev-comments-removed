




let imported = {};
Components.utils.import("resource:///modules/HUDService.jsm", imported);

const TEST_URI = "data:text/html;charset=utf-8,gcli-commands";

function test() {
  DeveloperToolbarTest.test(TEST_URI, function(browser, tab) {
    testEcho();
    testConsole(tab);

    imported = undefined;
    finish();
  });
}

function testEcho() {
  






}

function testConsole(tab) {
  DeveloperToolbarTest.exec({
    typed: "console open",
    args: {},
    blankOutput: true,
  });

  let hud = imported.HUDService.getHudByWindow(content);
  ok(hud.hudId in imported.HUDService.hudReferences, "console open");

  hud.jsterm.execute("pprint(window)");

  






  DeveloperToolbarTest.exec({
    typed: "console clear",
    args: {},
    blankOutput: true,
  });

  let labels = hud.jsterm.outputNode.querySelectorAll(".webconsole-msg-output");
  is(labels.length, 0, "no output in console");

  DeveloperToolbarTest.exec({
    typed: "console close",
    args: {},
    blankOutput: true,
  });

  ok(!(hud.hudId in imported.HUDService.hudReferences), "console closed");
}
