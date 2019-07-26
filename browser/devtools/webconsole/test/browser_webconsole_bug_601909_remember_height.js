










const MINIMUM_CONSOLE_HEIGHT = 150;



const MINIMUM_PAGE_HEIGHT = 50;
const HEIGHT_PREF = "devtools.hud.height";

let hud, newHeight, height, innerHeight, testDriver;

function testGen()
{
  height = parseInt(hud.style.height);

  toggleConsole();
  yield;

  is(newHeight, height, "same height after reopening the console");
  is(Services.prefs.getIntPref(HEIGHT_PREF), HUDService.lastConsoleHeight,
    "pref is correct");

  setHeight(Math.ceil(innerHeight * 0.5));
  toggleConsole();
  yield;

  is(newHeight, height, "same height after reopening the console");
  is(Services.prefs.getIntPref(HEIGHT_PREF), HUDService.lastConsoleHeight,
    "pref is correct");

  setHeight(MINIMUM_CONSOLE_HEIGHT - 1);
  toggleConsole();
  yield;

  is(newHeight, MINIMUM_CONSOLE_HEIGHT, "minimum console height is respected");
  is(Services.prefs.getIntPref(HEIGHT_PREF), HUDService.lastConsoleHeight,
    "pref is correct");

  setHeight(innerHeight - MINIMUM_PAGE_HEIGHT + 1);
  toggleConsole();
  yield;

  is(newHeight, innerHeight - MINIMUM_PAGE_HEIGHT,
    "minimum page height is respected");
  is(Services.prefs.getIntPref(HEIGHT_PREF), HUDService.lastConsoleHeight,
    "pref is correct");

  setHeight(Math.ceil(innerHeight * 0.6));
  Services.prefs.setIntPref(HEIGHT_PREF, -1);
  toggleConsole();
  yield;

  is(newHeight, height, "same height after reopening the console");
  is(Services.prefs.getIntPref(HEIGHT_PREF), -1, "pref is not updated");

  closeConsole();
  HUDService.lastConsoleHeight = 0;
  Services.prefs.setIntPref(HEIGHT_PREF, 0);

  hud = testDriver = null;
  executeSoon(finishTest);

  yield;
}

function toggleConsole()
{
  closeConsole(null, function() {
    openConsole(null, function() {
      let hudId = HUDService.getHudIdByWindow(content);
      hud = HUDService.hudReferences[hudId].iframe;
      newHeight = parseInt(hud.style.height);

      testDriver.next();
    });
  });
}

function setHeight(aHeight)
{
  height = aHeight;
  hud.style.height = height + "px";
}

function test()
{
  addTab("data:text/html;charset=utf-8,Web Console test for bug 601909");
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    innerHeight = content.innerHeight;
    openConsole(null, function(aHud) {
      hud = aHud.iframe;
      testDriver = testGen();
      testDriver.next();
    });
  }, true);
}

