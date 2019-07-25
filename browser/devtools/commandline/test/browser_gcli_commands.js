




let imported = {};
Components.utils.import("resource:///modules/HUDService.jsm", imported);

const TEST_URI = "data:text/html;charset=utf-8,gcli-commands";

function test() {
  DeveloperToolbarTest.test(TEST_URI, function(browser, tab) {
    testEcho();
    testConsoleClear();
    testConsoleOpenClose(tab);

    imported = undefined;
    finish();
  });
}

function testEcho() {
  DeveloperToolbarTest.exec({
    typed: "echo message",
    args: { message: "message" },
    outputMatch: /^message$/,
  });
}

function testConsoleClear() {
  DeveloperToolbarTest.exec({
    typed: "console clear",
    args: {},
    blankOutput: true,
  });
}

function testConsoleOpenClose(tab) {
  DeveloperToolbarTest.exec({
    typed: "console open",
    args: {},
    blankOutput: true,
  });

  let hud = imported.HUDService.getHudByWindow(content);
  ok(hud.hudId in imported.HUDService.hudReferences, "console open");

  DeveloperToolbarTest.exec({
    typed: "console close",
    args: {},
    blankOutput: true,
  });

  ok(!(hud.hudId in imported.HUDService.hudReferences), "console closed");
}
