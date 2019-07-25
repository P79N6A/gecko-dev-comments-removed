



const TEST_URI = "data:text/html;charset=utf-8,<p>test for bug 577721";

const POSITION_PREF = "devtools.webconsole.position";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
  registerCleanupFunction(testEnd);
}

function testEnd() {
  Services.prefs.clearUserPref(POSITION_PREF);
}

function consoleOpened(hudRef) {
  let hudBox = hudRef.HUDBox;

  
  document.addEventListener("popupshown", function popupShown() {
    document.removeEventListener("popupshown", popupShown, false);

    ok(hudRef.consolePanel, "console is in a panel");

    document.addEventListener("popuphidden", function popupHidden() {
      document.removeEventListener("popuphidden", popupHidden, false);
      executeSoon(finishTest);
    }, false);

    
    let menu = document.getElementById("webConsole");
    menu.click();
  }, false);

  hudRef.positionConsole("window");
}
